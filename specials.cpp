/*
 * specials.cpp
 *   Special attacks and such
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "commands.h"
#include "effects.h"
#include "specials.h"

bool Creature::runSpecialAttacks(Creature* victim) {
	if(specials.empty())
		return(false);

	SpecialAttack* attack;

	std::list<SpecialAttack*>::const_iterator eIt;
	for(eIt = specials.begin() ; eIt != specials.end() ; eIt++) {
		attack = *eIt;
		// Try running untill we get one that procs
		if(useSpecial(attack, victim))
			return(true);
	}

	// If we get here, we couldn't find any specials to run
	return(false);
}

SpecialAttack* Creature::getSpecial(const bstring& special) {
	std::list<SpecialAttack*>::const_iterator eIt;
	for(eIt = specials.begin() ; eIt != specials.end() ; eIt++) {
		if((*eIt) && (*eIt)->name == special)
			return((*eIt));
	}
	return(NULL);

}
bool Creature::useSpecial(const bstring& special, Creature* victim) {
	return(useSpecial(getSpecial(special), victim));
}

bool Creature::useSpecial(SpecialAttack* attack, Creature* victim) {
	if(!attack)
		return(false);

	if(attack->flagIsSet(SA_REQUIRE_HIDE) && !isHidden())
		return(false);

	long i, t;
	i = attack->ltime.ltime;
	t = time(0);
	// See if we can use this attack yet
	if(t - i < attack->delay)
		return(false); 	// Not enough time has passed yet

	// Check random chance that this attack goes off -- Chance of 101 means always go off
	if(attack->chance < mrand(1, 100))
		return(false);

	// Check time here

	if(attack->limit) {
		if(attack->used >= attack->limit)
			return(false);
	}

	if(attack->flagIsSet(SA_REQUIRE_HIDE) && isHidden())
		unhide();

	attack->ltime.ltime = time(0);
	attack->ltime.interval = attack->delay;

	if(!attack->isAreaAttack() && !attack->flagIsSet(SA_SINGLE_TARGET))
		attack->setFlag(SA_SINGLE_TARGET);

	if(attack->flagIsSet(SA_SINGLE_TARGET)) {
		return(doSpecial(attack, victim));
	}


	bool attacked = false;
	if(attack->isAreaAttack() && getRoom()) {
		attack->printRoomString(this);

		Creature* target;
		BaseRoom* room = getRoom();
		ctag *cp=0;
		if(attack->flagIsSet(SA_AE_PLAYER) || attack->flagIsSet(SA_AE_ALL)) {
			// First hit players
			cp = room->first_ply;
			while(cp) {
				target = cp->crt;
				cp = cp->next_tag;
				doSpecial(attack, target);
				attacked = true;
			}
			// Second, hit pets
			cp = room->first_mon;
			while(cp) {
				target = cp->crt;
				cp = cp->next_tag;
				if(target->isPet()) {
					doSpecial(attack, target);
					attacked = true;
				}
			}
		}
		if(attack->flagIsSet(SA_AE_MONSTER) || attack->flagIsSet(SA_AE_ALL)) {
			// Hit all non pets
			cp = room->first_mon;
			while(cp) {
				target = cp->crt;
				cp = cp->next_tag;
				if(!target->isPet()) {
					doSpecial(attack, target);
					attacked = true;
				}
			}
		}
	}
	if(attacked && attack->limit)
		attack->used++;
	return(attacked);
}


// Run the given special on a target, should only be called from useSpecial
bool Creature::doSpecial(SpecialAttack* attack, Creature* victim) {
	Player* pVictim = victim->getPlayer();
	Monster* mThis = getMonster();

	if(victim->isMonster() && victim->flagIsSet(M_NO_CIRCLE) && attack->getName() == "circle")
		return(false);

	if(attack->flagIsSet(SA_NO_UNDEAD) && victim->isUndead())
		return(false);

	if(attack->flagIsSet(SA_BREATHING_TARGETS) && victim->doesntBreathe())
		return(false);

	if(attack->flagIsSet(SA_UNDEAD_ONLY) && !victim->isUndead())
		return(false);

	if(attack->flagIsSet(SA_NO_UNDEAD_WARD) && isUndead() && victim->isEffected("undead-ward"))
		return(false);

	if(attack->flagIsSet(SA_NO_DRAIN_SHIELD) && victim->isEffected("drain-shield"))
		return(false);

	if(attack->flagIsSet(SA_NO_ATTACK_ON_LOW_HP) && hp.getCur() <= (hp.getMax()/5))
		return(false);

	if(	attack->flagIsSet(SA_NO_UNCONSCIOUS) &&
		pVictim && (
			pVictim->isEffected("blindness") ||
			pVictim->isEffected("petrification") ||
			pVictim->flagIsSet(P_UNCONSCIOUS)
		)
	)
		return(false);

	if(attack->flagIsSet(SA_HOLY_WAR)) {
		if(cClass != PALADIN && cClass != DEATHKNIGHT)
			return(false);

		if((cClass == PALADIN && victim->getClass() != DEATHKNIGHT) ||
		   (cClass == DEATHKNIGHT && victim->getClass() != PALADIN))
			return(false);
	}

	if(attack->flagIsSet(SA_TARGET_NEEDS_MANA) && victim->mp.getCur() <= 0)
		return(false);

	if(attack->flagIsSet(SA_SINGLE_TARGET)) {
		if(attack->limit)
			attack->used++;
	}

	if(attack->type == SPECIAL_WEAPON) {
		// General attack, so compute an attack result
		int resultFlags = NO_CRITICAL | NO_FUMBLE; // We'll be nice for now...no critical backstabs on mobs :)
		if(attack->flagIsSet(SA_NO_DODGE))
			resultFlags |= NO_DODGE;
		if(attack->flagIsSet(SA_NO_PARRY))
			resultFlags |= NO_PARRY;
		if(attack->flagIsSet(SA_NO_BLOCK))
			resultFlags |= NO_BLOCK;

		AttackResult result = getAttackResult(victim, NULL, resultFlags);
		switch(result) {
			case ATTACK_DODGE:
				attack->printFailStrings(this, victim);
				victim->dodge(this);
				victim->doLagProtect();
				return(true);
			case ATTACK_PARRY:
				attack->printFailStrings(this, victim);
				victim->parry(this);
				victim->doLagProtect();
				return(true);
			case ATTACK_MISS:
				attack->printFailStrings(this, victim);
				victim->doLagProtect();
				return(true);
			case ATTACK_BLOCK:
				printColor("^C%M partially blocked your attack!\n", victim);
				victim->printColor("^CYou manage to partially block %N's attack!\n", this);
				break;
			default:
				// Hits etc, do nothing, fall through to the rest of the code
				break;
		}
	}
	Damage attackDamage;
	attackDamage.set(-1);
	bool saved = false;
	// Some attacks don't do any damage
	if(attack->saveType > SAVE_NONE && attack->saveType <= SAVE_SPELL) {
		int bns = 0;
		switch(attack->saveBonus) {
			case BONUS_LEVEL_INT:
				bns = 10*((int)victim->getLevel() - (int)level)+ 3*bonus(victim->intelligence.getCur());
				break;
			case BONUS_LEVEL_CON:
				bns = 10*((int)victim->getLevel() - (int)level)+ 3*bonus(victim->constitution.getCur());
				break;
			case BONUS_LEVEL_DEX:
				bns = 10*((int)victim->getLevel() - (int)level)+ 3*bonus(victim->dexterity.getCur());

			default:
				break;
		}
		int maxBonus = attack->maxBonus;

		if(maxBonus == -1)
			maxBonus = 0;
		else if(maxBonus == 0)
			maxBonus = 100;

		bns = MAX(0,MIN(bns,maxBonus));
		// Save types are saves + 1 to account for NO_SAVE
		if(victim->chkSave(attack->saveType - 1, this, bns))
			saved = true;
	} else {
		int chance = 50;
		switch(attack->saveType) {
			case SAVE_STRENGTH:
				break;
			case SAVE_DEXTERITY:
				chance = 50 + (level - victim->getLevel()) * 10 +
						(dexterity.getCur() - victim->dexterity.getCur())/5 ;
				break;
			case SAVE_CONSTITUTION:
				break;
			case SAVE_INTELLIGENCE:
				break;
			case SAVE_PIETY:
				break;
			case SAVE_LEVEL:
				chance = 35 + ((level - victim->getLevel()) * 20);
				break;
			default:
				break;
		}
		//chance -= attack->saveBonus;
		chance = MAX(1, MIN(90, chance));
		// Chance is chance for the attacker to succeed...so invert the check to see if the victim saved
		if(mrand(1, 100) > chance)
			saved = true;
	}

	// Can't be confused with insight, and always confused with feelblemind
	if(attack->type == SPECIAL_CONFUSE) {
		if(victim->isEffected("insight"))
			saved = true;
		else if(victim->isEffected("feeblemind"))
			saved = false;
	}

	if(saved) {
		attack->printRoomSaveString(this, victim);
		attack->printTargetSaveString(this, victim);

		// This flag can be used on attacks like circle where nothing happens when they save vs dex
		if(attack->flagIsSet(SA_SAVE_NO_DAMAGE)) {
			victim->doLagProtect();
			return(true); // The attack went off, but got saved
		}
	}

	if(attack->type == SPECIAL_EXP_DRAIN) {
		attackDamage.set(attack->damage.roll());
		attackDamage.set(MIN((unsigned)attackDamage.get(), victim->getExperience()));

		if(saved == true)
			attackDamage.set(attackDamage.get() / 2);
	}
	else if(attack->type == SPECIAL_PETRIFY) {
		if(pVictim) {
			if((pVictim->isEffected("resist-earth") || pVictim->isEffected("resist-magic")) && mrand(1,100) <= 50)
				return(false);

			if(!saved) {
				pVictim->addEffect("petrification", 0, 0, 0, true, this);
				pVictim->clearAsEnemy(); // clears player from all mob's enemy lists in the room.
				remFromGroup(pVictim);
			}
		}
	}
	else if(attack->type == SPECIAL_CONFUSE) {
		if(pVictim->isEffected("resist-magic") && mrand(1,100) <= 75)
			return(false);


	}
	else if(attack->type == SPECIAL_STEAL) {
		if(pVictim && !saved)
			mThis->steal(pVictim);
	}
	else if(attack->type != SPECIAL_NO_DAMAGE) {
		// Compute damage
		if(attack->flagIsSet(SA_HALF_HP_DAMAGE))
			attackDamage.set(victim->hp.getCur() / 2);
		else if(attack->flagIsSet(SA_RESIST_MAGIC_NO_DAMAGE) && victim->isEffected("resist-magic"))
			attackDamage.set(0);
		else {
			int dn = attack->damage.getNumber();
			int ds = attack->damage.getSides();
			int dp = attack->damage.getPlus();
			if(attack->flagIsSet(SA_INTELLIGENCE_REDUCE)) {
				if(victim->intelligence.getCur() < 210)
					; // no reduction
				else if(victim->intelligence.getCur() < 240)
					dn -= 1; // slight reduction
				else
					dn -= 2; // more reduction!

				// Can't reduce it past 1
				dn = MAX(dn, 1);
			}
			attackDamage.set(dice(dn, ds, dp));
		}

		// Now check for a save to adjust damage
		if(saved == true)
			attackDamage.set(attackDamage.get() / 2);

		// Now check for realm resists, item disolving, etc
		if(attack->type != SPECIAL_NO_TYPE) {
			// Is the attack elemental damage?
			if(attack->type >= SPECIAL_EARTH && attack->type <= SPECIAL_COLD) {
				// SPECIAL-realm matches up with the corresponding Realm so we can just
				// pass that, after casting it to a Realm
				attackDamage.set(victim->checkRealmResist(attackDamage.get(), (Realm)attack->type));
			}
			// TODO: Check for warmth/heat-protection on elemental attacks
//			if(	pVictim && (
//					(realm == COLD && (pVictim->isEffected("warmth") || pVictim->isEffected("alwayscold"))) ||
//					(realm == FIRE && pVictim->isEffected("heat-protection") || pVictim->isEffected("alwayswarm"))
//			) )
		}

		if(attack->flagIsSet(SA_EARTH_SHIELD_REDUCE))
			attackDamage.set(attackDamage.get() / 2);

		// More adjustments
		if(attack->type == SPECIAL_WEAPON || attack->flagIsSet(SA_CHECK_PHYSICAL_DAMAGE))
			victim->modifyDamage(this, PHYSICAL, attackDamage);
		else if(attack->flagIsSet(SA_CHECK_NEGATIVE_ENERGY))
			victim->modifyDamage(this, NEGATIVE_ENERGY, attackDamage);

		if(attack->flagIsSet(SA_BERSERK_REDUCE))
			attackDamage.set(mrand(1, 10));

		// Put here to avoid other damage reducers
		if(!saved && attack->flagIsSet(SA_CAN_DISINTEGRATE) && mrand(1,100) < 2) {
			victim->printColor("^Y%M seriously damages you!\n", this);
			attackDamage.set(victim->hp.getCur() - 5);
			attackDamage.set(MAX(attackDamage.get(), 1));
		}
	}


	if(pVictim && !isPet()) {
		if(attack->flagIsSet(SA_ACID)) {
			// Check for disolve
			if(attack->saveType == SAVE_NONE || !pVictim->chkSave(attack->saveType-1, this, attack->saveBonus))
				pVictim->loseAcid();
		}
		if(attack->flagIsSet(SA_DISSOLVE)) {
			if(attack->saveType == SAVE_NONE || !pVictim->chkSave(attack->saveType-1, this,attack->saveBonus))
				pVictim->dissolveItem(this);
		}
	}
	if(mThis) {
		if(attack->flagIsSet(SA_POISON)) {
			// Check for poison
			mThis->tryToPoison(victim, attack);
		}
		if(attack->flagIsSet(SA_DISEASE)) {
			// Check for disease
			mThis->tryToDisease(victim, attack);
		}
		if(attack->flagIsSet(SA_BLIND)) {
			// Check for blind
			mThis->tryToBlind(victim, attack);
		}
		if(attack->flagIsSet(SA_ZAP_MANA)) {
			mThis->zapMp(victim, attack);
		}
	}
	if(attack->flagIsSet(SA_SINGLE_TARGET))
		attack->printRoomString(this, victim);

	if(attackDamage.get() != -1) {
		attack->printTargetString(this, victim, attackDamage.get());

		if(attack->flagIsSet(SA_DRAINS_DAMAGE)) {
			victim->printColor("^r%M feeds on your energy.\n", this);
			hp.increase(attackDamage.get() / 2);
		}
		if(attack->type == SPECIAL_EXP_DRAIN) {
			// Damage is experience, not hp
			victim->addExperience(attackDamage.get() * -1);
		} else {
			if(isPet() && following)
				following->printColor("%M %s %N for %s%d^x damage.\n", this, attack->verb.c_str(), victim, following->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());

			broadcastGroup(false, victim, "^M%M^x %s ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n", this, attack->verb.c_str(),
				victim, attackDamage.get(), victim->heShe(), victim->getStatusStr(attackDamage.get()));

			if(attack->flagIsSet(SA_CHECK_DIE_ROB)) {
				if(doDamage(victim, attackDamage.get(), CHECK_DIE_ROB))
					return(true);
			} else {
				if(doDamage(victim, attackDamage.get(), CHECK_DIE))
					return(true);
			}
		}

		// TODO: Dom: make a flag for this?
		// 5% chance to get porphyria when bitten by a vampire
		if(attack->getName() == "Vampiric Bite")
			victim->addPorphyria(this, 5);

	} else {
		attack->printTargetString(this, victim);
	}

	// Stun the target if they didn't save
	if(attack->stunLength && !saved) {
		int stunLength = 0;
		if(attack->flagIsSet(SA_RANDOMIZE_STUN)) {
			stunLength = mrand(attack->stunLength - 3, attack->stunLength + 3);
			stunLength = MAX(1, stunLength);
		} else
			stunLength = attack->stunLength;
		victim->print("You have been stunned for %d seconds!\n", stunLength);
		victim->stun(stunLength);
	}
	victim->doLagProtect();
	return(true);
}

bool SpecialAttack::isAreaAttack() const {
	return(flagIsSet(SA_AE_ENEMY) || flagIsSet(SA_AE_ALL) || flagIsSet(SA_AE_PLAYER) || flagIsSet(SA_AE_MONSTER));
}
bool SpecialAttack::flagIsSet(int flag) const {
	return(flags[flag/8] & 1<<(flag%8));
}
void SpecialAttack::setFlag(int flag) {
	flags[flag/8] |= 1<<(flag%8);
}
void SpecialAttack::clearFlag(int flag) {
	flags[flag/8] &= ~(1<<(flag%8));
}

bstring SpecialAttack::modifyAttackString(const bstring& input, Creature* viewer, Creature* attacker, Creature* target, int dmg) {
	bstring toReturn = input;
	/*			Web Editor
	 * 			 _   _  ____ _______ ______
	 *			| \ | |/ __ \__   __|  ____|
	 *			|  \| | |  | | | |  | |__
	 *			| . ` | |  | | | |  |  __|
	 *			| |\  | |__| | | |  | |____
	 *			|_| \_|\____/  |_|  |______|
	 *
	 * 		If you change anything here, make sure the changes are reflected in the web
	 * 		editor! Either edit the PHP yourself or tell Dominus to make the changes.
	 */
	toReturn.Replace("*ATTACKER*", attacker->getCrtStr(viewer, CAP).c_str());
	toReturn.Replace("*LOW-ATTACKER*", attacker->getCrtStr(viewer).c_str());
	toReturn.Replace("*A-DEITY*", gConfig->getDeity(attacker->getDeity())->getName().c_str());
	toReturn.Replace("*A-HESHE*", attacker->heShe());
	toReturn.Replace("*A-HISHER*", attacker->hisHer());
	toReturn.Replace("*A-UPHESHE*", attacker->upHeShe());
	toReturn.Replace("*A-UPHISHER*", attacker->upHisHer());

	if(target) {
		toReturn.Replace("*T-HESHE*", target->heShe());
		toReturn.Replace("*T-HISHER*", target->hisHer());
		toReturn.Replace("*T-UPHESHE*", target->upHeShe());
		toReturn.Replace("*T-UPHISHER*", target->upHisHer());

		toReturn.Replace("*TARGET*", target->getCrtStr(viewer, CAP).c_str());
		toReturn.Replace("*VICTIM*", target->getCrtStr(viewer, CAP).c_str());
		toReturn.Replace("*LOW-TARGET*", target->getCrtStr(viewer).c_str());
		toReturn.Replace("*LOW-VICTIM*", target->getCrtStr(viewer).c_str());
	}
	toReturn.Replace("*CR*", "\n");
	if(dmg != -1)
		toReturn.Replace("*DAMAGE*", xml::numToStr(dmg).c_str());
	toReturn = viewer->customColorize(toReturn);
	return(toReturn);
}

void SpecialAttack::printToRoom(BaseRoom* room, const bstring& str, Creature* attacker, Creature* target, int dmg ) {
	if(str.empty())
		return;

	ctag* cp = room->first_ply;
	Player* player=0;
	bstring toPrint = "";
	while(cp) {
		player = cp->crt->getPlayer();
		cp = cp->next_tag;

		if(!player || player == target)
			continue;

		toPrint = modifyAttackString(str, player, attacker, target, dmg);
		player->printColor("%s\n", toPrint.c_str());
	}

}
void SpecialAttack::printFailStrings(Creature* attacker, Creature* target) {
	printToRoom(target->getRoom(), roomFailStr, attacker, target);
	bstring toPrint = modifyAttackString(targetFailStr, target, attacker, target);
	target->printColor("%s\n", toPrint.c_str());
	return;
}
void SpecialAttack::printRoomString(Creature* attacker, Creature* target) {
	return(printToRoom(attacker->getRoom(), roomStr, attacker, target));
}

void SpecialAttack::printTargetString(Creature* attacker, Creature* target, int dmg) {
	bstring toPrint = modifyAttackString(targetStr, target, attacker, target, dmg);
	target->printColor("%s\n", toPrint.c_str());
	return;
}

void SpecialAttack::printRoomSaveString(Creature* attacker, Creature* target) {
	return(printToRoom(attacker->getRoom(), roomSaveStr, attacker, target));
}
void SpecialAttack::printTargetSaveString(Creature* attacker, Creature* target, int dmg) {
	bstring toPrint = modifyAttackString(targetSaveStr, target, attacker, target, dmg);
	target->printColor("%s\n", toPrint.c_str());
	return;
}

void SpecialAttack::reset() {
	delay = chance = stunLength = limit  = used = maxBonus = 0;
	type = SPECIAL_NO_TYPE;
	saveType = SAVE_NONE;
	saveBonus = BONUS_NONE;
	ltime.interval = ltime.ltime = ltime.misc = 0;
	int i;
	for(i = 0 ; i < 8 ; i++)
		flags[i] = 0;

	return;
}
bstring Creature::getSpecialsFullList() const {
	std::ostringstream specialsStr;

	specialsStr << "Special Attacks for " << name << ":\n";

	int num = 0;
	SpecialAttack* attack;
	std::list<SpecialAttack*>::const_iterator sIt;
	for(sIt = specials.begin() ; sIt != specials.end() ; sIt++) {
		attack = (*sIt);
		specialsStr << *attack;
		num++;
	}

	specialsStr << "\n" << num << " effect(s) found.\n";
	bstring toPrint = specialsStr.str();

	return(toPrint);
}
int dmSpecials(Player* player, cmd* cmnd) {
	Creature* target=0;
	Monster* mTarget=0;

	if(player->getClass() == BUILDER && !player->canBuildMonsters())
		return(cmdNoAuth(player));

	if(cmnd->num > 1) {
		target = player->getRoom()->findCreature(player, cmnd);

		if(target && player->getClass() == BUILDER) {
			mTarget = target->getMonster();
			if(mTarget) {
				if(mTarget->info.id && !player->checkBuilder(mTarget->info)) {
					player->print("Error: monster index not in any of your alotted ranges.\n");
					return(0);
				}
			}
		}

		if(!target) {
			if(player->isCt()) {
				cmnd->str[1][0] = up(cmnd->str[1][0]);
				target = gServer->findPlayer(cmnd->str[1]);
			}

			if(!target || !player->canSee(target)) {
				player->print("Target not found.\n");
				return(0);
			}
		}
	} else {
		player->print("Target not found.\n");
		return(0);
	}
	if(cmnd->num > 2) {
		if(!strncmp(cmnd->str[2], "-d", 2)) {
			target->delSpecials();
			player->print("Deleted all specials for %N\n", target);
			return(0);
		}
		SpecialAttack* attack;

		/*			Web Editor
		 * 			 _   _  ____ _______ ______
		 *			| \ | |/ __ \__   __|  ____|
		 *			|  \| | |  | | | |  | |__
		 *			| . ` | |  | | | |  |  __|
		 *			| |\  | |__| | | |  | |____
		 *			|_| \_|\____/  |_|  |______|
		 *
		 * 		If you change anything here, make sure the changes are reflected in the web
		 * 		editor! Either edit the PHP yourself or tell Dominus to make the changes.
		 */
		if(!(attack = target->addSpecial(cmnd->str[2]))) {
			player->printColor("The special ^W%s^x does not exist!\n", cmnd->str[2]);
			player->print("Pick from this list:\n");
            player->print("     backstab          bash\n");
            player->print("     kick              bite\n");
            player->print("     circle            smother\n");
            player->print("     fire-breath       lightning-breath\n");
            player->print("     acid-breath       frost-cone\n");
            player->print("     gas-breath        wind-blast\n");
            player->print("     psychic-blast     energy-drain\n");
            player->print("     touch-of-death    turn\n");
            player->print("     renounce          tail-slap\n");
            player->print("     trample           poisonous-sting\n");
            player->print("     petrifying-gaze   petrifying-breath\n");
            player->print("     confusing-gaze    zap-mana\n");
            player->print("     death-gaze\n");
			return(0);
		} else {
			player->printColor("Added special ^W%s^x to ^W%M^x.\n", attack->getName().c_str(), target);
			return(0);
		}
	} else {
		bstring toPrint = target->getSpecialsFullList();
		player->printColor("%s\n", toPrint.c_str());
	}

	return(0);
}
bool Creature::delSpecials() {
	for(SpecialAttack* special : specials) {
		delete special;
	}
	specials.clear();
	return(true);
}
std::ostream& operator<<(std::ostream& out, const SpecialAttack& attack) {
	//out.setf(std::ios::right, std::ios::adjustfield);

	out << "Special Attack:\n";
	out << "Name: " << attack.name << "\tType: " << attack.type << "\n";

	if(!attack.targetStr.empty())
		out << "TargetStr:     " << attack.targetStr << "\n";
	if(!attack.targetFailStr.empty())
		out << "TargetFailStr: " << attack.targetFailStr << "\n";
	if(!attack.selfStr.empty())
		out << "SelfStr:       " << attack.selfStr << "\n";
	if(!attack.roomStr.empty())
		out << "RoomStr:       " << attack.roomStr << "\n";
	if(!attack.roomFailStr.empty())
		out << "RoomFailStr:   " << attack.roomFailStr << "\n";

	out << "SaveType: ";
	switch(attack.saveType) {
	case SAVE_NONE:
		out << "None";
		break;
	case SAVE_LUCK:
		out << "Luck";
		break;
	case SAVE_POISON:
		out << "Poison";
		break;
	case SAVE_DEATH:
		out << "Death";
		break;
	case SAVE_BREATH:
		out << "Breath";
		break;
	case SAVE_MENTAL:
		out << "Mental";
		break;
	case SAVE_SPELL:
		out << "Spell";
		break;
	default:
		break;
	}
	out << "\tSaveBonus: " << attack.saveBonus << "\n";
	if(!attack.targetSaveStr.empty())
		out << "TargetSaveStr: " << attack.targetSaveStr << "\n";
	if(!attack.selfSaveStr.empty())
		out << "SelfSaveStr:   " << attack.selfSaveStr << "\n";
	if(!attack.roomSaveStr.empty())
		out << "RoomSaveStr:   " << attack.roomSaveStr << "\n";

	out << "Chance: " << attack.chance << "%\t Delay: " << attack.delay << "\n"
		<< "StunLength: " << attack.stunLength << "\t Used/Limit: " << attack.used << "/" << attack.limit << "\n";

	out << "Damage: " << attack.damage.str()
		<< " (Low: " << attack.damage.low() << " High: " << attack.damage.high()
		<< " Avg: " << attack.damage.average() << ")\n"

		<< "Flags: ";
	int i, num = 0;
	for(i = SA_NO_FLAG + 1; i < SA_MAX_FLAG ; i++) {
		if(attack.flagIsSet(i)) {
			if(num != 0)
				out << ", ";
			out << gConfig->getSpecialFlag(i) << "(" << i << ")";
			num++;
		}

	}
	if(num == 0)
		out << "None";
	out << ".\n";
	out << "\n";
	return out;
}

bstring Creature::getSpecialsList() const {
	std::ostringstream specialStr;

	specialStr << "Special Attacks: ";
	int num = 0;
	std::list<SpecialAttack*>::const_iterator aIt;
	SpecialAttack* attack;
	for(aIt = specials.begin() ; aIt != specials.end() ; aIt++) {
		attack = (*aIt);
		if(num != 0)
			specialStr << ", ";
		specialStr << attack->name;
		num++;
	}
	if(num == 0)
		specialStr << "None";
	specialStr << ".\n";

	bstring toPrint = specialStr.str();
	return(toPrint);
}

SpecialAttack::SpecialAttack() {
	reset();
}
SpecialAttack::SpecialAttack(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;
	reset();

	while(curNode) {
			if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
		else if(NODE_NAME(curNode, "Verb")) xml::copyToBString(verb, curNode);
		else if(NODE_NAME(curNode, "TargetStr")) xml::copyToBString(targetStr, curNode);
		else if(NODE_NAME(curNode, "TargetFailStr")) xml::copyToBString(targetFailStr, curNode);
		else if(NODE_NAME(curNode, "RoomFailStr")) xml::copyToBString(roomFailStr, curNode);
		else if(NODE_NAME(curNode, "SelfStr")) xml::copyToBString(selfStr, curNode);
		else if(NODE_NAME(curNode, "RoomStr")) xml::copyToBString(roomStr, curNode);

		else if(NODE_NAME(curNode, "TargetSaveStr")) xml::copyToBString(targetSaveStr, curNode);
		else if(NODE_NAME(curNode, "SelfSaveStr")) xml::copyToBString(selfSaveStr, curNode);
		else if(NODE_NAME(curNode, "RoomSaveStr")) xml::copyToBString(roomSaveStr, curNode);

		else if(NODE_NAME(curNode, "Delay")) xml::copyToNum(delay, curNode);
		else if(NODE_NAME(curNode, "Chance")) xml::copyToNum(chance, curNode);
		else if(NODE_NAME(curNode, "StunLength")) xml::copyToNum(stunLength, curNode);
		else if(NODE_NAME(curNode, "Limit")) xml::copyToNum(limit, curNode);
		else if(NODE_NAME(curNode, "MaxBonus")) xml::copyToNum(maxBonus, curNode);
		else if(NODE_NAME(curNode, "SaveType")) saveType = (SpecialSaveType)xml::toNum<int>(xml::getCString(curNode));
		else if(NODE_NAME(curNode, "SaveBonus")) saveBonus = (SaveBonus)xml::toNum<int>(xml::getCString(curNode));
		else if(NODE_NAME(curNode, "Type")) type = (SpecialType)xml::toNum<int>(xml::getCString(curNode));
		else if(NODE_NAME(curNode, "Flags")) loadBits(curNode, flags);
		else if(NODE_NAME(curNode, "LastTime")) loadLastTime(curNode, &ltime);
		else if(NODE_NAME(curNode, "Dice")) damage.load(curNode);

		curNode = curNode->next;
	}
}

bool SpecialAttack::save(xmlNodePtr rootNode) const {
	xmlNodePtr attackNode = xml::newStringChild(rootNode, "SpecialAttack");
	xml::newStringChild(attackNode, "Name", name);
	xml::newStringChild(attackNode, "Verb", verb);

	xml::newStringChild(attackNode, "TargetStr", targetStr);
	xml::newStringChild(attackNode, "TargetFailStr", targetFailStr);
	xml::newStringChild(attackNode, "RoomFailStr", roomFailStr);
	xml::newStringChild(attackNode, "SelfStr", selfStr);
	xml::newStringChild(attackNode, "RoomStr", roomStr);

	xml::newStringChild(attackNode, "TargetSaveStr", targetSaveStr);
	xml::newStringChild(attackNode, "SelfSaveStr", selfSaveStr);
	xml::newStringChild(attackNode, "RoomSaveStr", roomSaveStr);

	xml::newNumChild(attackNode, "Limit", limit);
	xml::newNumChild(attackNode, "Delay", delay);
	xml::newNumChild(attackNode, "Chance", chance);
	xml::newNumChild(attackNode, "StunLength", stunLength);
	xml::newNumChild(attackNode, "Type", type);
	xml::newNumChild(attackNode, "SaveType", saveType);
	xml::newNumChild(attackNode, "SaveBonus", saveBonus);
	xml::newNumChild(attackNode, "MaxBonus", maxBonus);

	saveBits(attackNode, "Flags", SA_MAX_FLAG, flags);
	saveLastTime(attackNode, 0, ltime);
	damage.save(attackNode, "Dice");

	return(true);
}

SpecialAttack* Creature::addSpecial(const bstring specialName) {
	SpecialAttack* attack=0;

	/*			Web Editor
	 * 			 _   _  ____ _______ ______
	 *			| \ | |/ __ \__   __|  ____|
	 *			|  \| | |  | | | |  | |__
	 *			| . ` | |  | | | |  |  __|
	 *			| |\  | |__| | | |  | |____
	 *			|_| \_|\____/  |_|  |______|
	 *
	 * 		If you change anything here, make sure the changes are reflected in the web
	 * 		editor! Either edit the PHP yourself or tell Dominus to make the changes.
	 */
	if(specialName == "backstab") {
		attack = new SpecialAttack();
		attack->name = "Backstab";
		attack->verb = "backstabbed";


		attack->type = SPECIAL_WEAPON;
		attack->targetStr = "^r*ATTACKER* attempts to backstab you!*CR**ATTACKER* backstabs you for ^R*DAMAGE*^r damage!^x";
		attack->targetFailStr = "^c*ATTACKER* attempts to backstab you.*CR**A-UPHISHER* backstab failed!^x";
		attack->roomStr = "^r*ATTACKER* backstabs *LOW-VICTIM*!^x";
		attack->roomFailStr = "^c*ATTACKER*'s backstab missed!^x";
		attack->chance = 101; // Always goes off
		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_REQUIRE_HIDE);
		attack->setFlag(SA_NO_PARRY);	// Being stabbed from behind...can't parry that
		attack->setFlag(SA_NO_BLOCK);
		attack->setFlag(SA_CHECK_DIE_ROB);
		attack->damage.setNumber(damage.getNumber() * 3);
		attack->damage.setSides(damage.getSides());
		attack->damage.setPlus(damage.getPlus() * 3);
		// No need for a delay here because the attack requires hide

		specials.push_back(attack);
	} else if(specialName == "bash") {
		attack = new SpecialAttack();
		attack->name = "Bash";
		attack->verb = "bashed";

		attack->type = SPECIAL_WEAPON;
		attack->targetStr = "^r*ATTACKER* BASHES you for ^R*DAMAGE*^r damage.^x";
		attack->roomStr = "^r*ATTACKER* BASHES *LOW-TARGET*.^x";
		attack->targetSaveStr = attack->targetFailStr = "^c*ATTACKER* tried to bash you.^x";
		attack->roomSaveStr = attack->roomFailStr = "^c*ATTACKER* tried to bash *LOW-TARGET*!^x";
		attack->chance = 101; // Always goes off
		attack->stunLength = 5;
		attack->saveType = SAVE_DEXTERITY;
		attack->setFlag(SA_SAVE_NO_DAMAGE);
		attack->setFlag(SA_RANDOMIZE_STUN);
		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_CHECK_DIE_ROB);
		attack->damage.setNumber(damage.getNumber()/2);
		attack->damage.setSides(damage.getSides());
		attack->damage.setPlus(damage.getPlus()/2);
		attack->limit = 1; // Can only perform this attack once

		specials.push_back(attack);

	} else if(specialName == "kick") {
		attack = new SpecialAttack();
		attack->name = "Kick";
		attack->verb = "kicked";

		attack->type = SPECIAL_WEAPON;
		attack->targetStr = "^r*ATTACKER* kicks you for ^R*DAMAGE*^r damage.^x";
		attack->roomStr = "^r*ATTACKER* kicks *LOW-TARGET*.^x";
		attack->targetSaveStr = attack->targetFailStr = "^c*ATTACKER* tried to kick you.^x";
		attack->roomSaveStr = attack->roomFailStr = "^c*ATTACKER* tried to kick *LOW-TARGET*!^x";
		attack->saveType = SAVE_DEXTERITY;
		attack->chance = 101; // Always goes off
		attack->delay = 12;

		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_CHECK_DIE_ROB);
		attack->setFlag(SA_UNDEAD_WARD_REDUCE);
		attack->setFlag(SA_SAVE_NO_DAMAGE);
		attack->damage.setNumber(2);
		attack->damage.setSides(3);
		attack->damage.setPlus(bonus(strength.getCur()) + cClass == FIGHTER ? level / 4 : 0);

//		if(flagIsSet(OLD_M_TRAMPLE)) {
//			attack->dice[0] *= 2;
//			attack->dice[2] *= 2;
//		}

		specials.push_back(attack);

	} else if(specialName == "bite") {
		attack = new SpecialAttack();
		if(isEffected("vampirism")) {
			attack->name = "Vampiric Bite";
			attack->setFlag(SA_NIGHT_ONLY);
			attack->setFlag(SA_NO_UNDEAD);
			attack->saveType = SAVE_DEATH;
			// Vampires bite for more!
			attack->damage.setNumber(level);
			attack->damage.setSides(3);
			attack->damage.setPlus(level);
			attack->targetSaveStr = "^rYou manage to shove *LOW-ATTACKER* off of you.^x";
			attack->roomSaveStr = "*TARGET* shoves *LOW-ATTACKER* away, interrupting *A-HISHER* bloody feast!";
		} else {
			attack->name = "Bite";
			attack->saveType = SAVE_LEVEL;
			attack->damage.setNumber(level);
			attack->damage.setSides(2);
			attack->damage.setPlus(level);
			// Using level save as old chance for sucess, so no damage on sucessful save
			attack->setFlag(SA_SAVE_NO_DAMAGE);
		}

		attack->verb = "bit";
		attack->chance = 101; // Always goes off

		// Kinda a weapon attack
		attack->type = SPECIAL_WEAPON;
		attack->delay = 30;

		attack->stunLength = 5;
		attack->setFlag(SA_RANDOMIZE_STUN);
		attack->setFlag(SA_SINGLE_TARGET);
		attack->targetStr = "^r*ATTACKER* bites you for ^R*DAMAGE*^r damage.^x";
		attack->roomStr = "^r*ATTACKER* bites *LOW-TARGET*!^x";
		attack->targetFailStr = "^r*ATTACKER* tried to bite you.^x";
		attack->roomFailStr = "^r*ATTACKER* tried to bite *LOW-TARGET*.^x";

		// Can be dodged, or parried, but not blocked
		attack->setFlag(SA_NO_BLOCK);

		specials.push_back(attack);
	} else if(specialName == "circle") {
		attack = new SpecialAttack();
		attack->name = "Circle";
		attack->verb = "circled";
		attack->type = SPECIAL_NO_DAMAGE;
		attack->stunLength = 7;
		attack->delay = 10;
		attack->chance = 25;
		attack->saveType = SAVE_DEXTERITY;
		attack->saveBonus = BONUS_LEVEL_DEX;

		attack->setFlag(SA_SAVE_NO_DAMAGE);
		attack->setFlag(SA_RANDOMIZE_STUN);
		attack->setFlag(SA_SINGLE_TARGET);

		attack->targetStr = "^r*ATTACKER* circles you!^x";
		attack->roomStr = "^r*ATTACKER* circles *LOW-TARGET*!^x";
		attack->targetSaveStr = "^c*ATTACKER* tried to circle you.^x";
		attack->roomSaveStr = "^c*ATTACKER* tried to circle *LOW-TARGET*!^x";

		specials.push_back(attack);

	} else if(specialName == "smother") {
		attack = new SpecialAttack();
		attack->name = "Earth Smother";
		attack->verb = "smothered";
		attack->type = SPECIAL_EARTH;
		attack->saveType = SAVE_BREATH;
		attack->saveBonus = BONUS_LEVEL_CON;
		attack->setFlag(SA_SINGLE_TARGET);
		attack->chance = 25;
		attack->delay = 60;

		attack->targetStr = "^Y*ATTACKER* smothers you for *DAMAGE* damage.^x";
		attack->roomStr = "^Y*ATTACKER* smothers *LOW-TARGET*.^x";
		attack->targetSaveStr = "^cYou almost avoid the pressing earth.^x";
		attack->roomSaveStr = "*TARGET* almost avoids the pressing earth.";
		attack->damage.setNumber(level);
		attack->damage.setSides(4);
		attack->damage.setPlus(0);

		specials.push_back(attack);

	} else if(specialName == "fire-breath") {
		attack = new SpecialAttack();
		attack->name = "Fire Breath";
		attack->verb = "breathes fire on";
		attack->type = SPECIAL_FIRE;
		attack->saveType = SAVE_BREATH;
		attack->targetStr = "^r*ATTACKER* breathes fire on you!*CR*You take ^R*DAMAGE*^r damage!^x";
		attack->roomStr = "^r*ATTACKER* breathes fire on *LOW-TARGET*!^x";
		attack->targetSaveStr = "^cYou almost manage to dive out of the way.^x";
		attack->roomSaveStr = "*TARGET* almost dives out of the way.";
		attack->damage.setNumber(level);
		attack->damage.setSides(4);
		attack->damage.setPlus(0);
		attack->setFlag(SA_SINGLE_TARGET);
		attack->delay = 60;
		attack->chance = 20; // 20 percent chance to go off every time

		specials.push_back(attack);
	} else if(specialName == "lightning-breath") {
		attack = new SpecialAttack();
		attack->name = "Lightning Breath";
		attack->verb = "breathes lightning on";
		attack->type = SPECIAL_ELECTRIC;
		attack->saveType = SAVE_BREATH;
		attack->targetStr = "^W*ATTACKER* breathes lightning on you!*CR*You take ^R*DAMAGE*^W damage!^x";
		attack->roomStr = "^W*ATTACKER* breathes lightning on *LOW-TARGET*!^x";
		attack->targetSaveStr = "^cYou almost manage to dive out of the way.^x";
		attack->roomSaveStr = "*TARGET* almost dives out of the way.";
		attack->damage.setNumber(level);
		attack->damage.setSides(4);
		attack->damage.setPlus(0);
		attack->setFlag(SA_SINGLE_TARGET);
		attack->delay = 60;
		attack->chance = 20; // 20 percent chance to go off every time

		specials.push_back(attack);

	} else if(specialName == "acid-breath") {
		attack = new SpecialAttack();
		attack->name = "Acid Breath";
		attack->verb = "breathed acid on";

		attack->type = SPECIAL_BREATH;
		attack->saveType = SAVE_BREATH;
		attack->targetStr = "^g*ATTACKER* spits acid on you!*CR*You take ^G*DAMAGE*^g damage!^x";
		attack->roomStr = "^g*ATTACKER* spits acid on *LOW-TARGET*!^x";
		attack->targetSaveStr = "You dove to the side, only being partially hit!";
		attack->roomSaveStr = "*TARGET* dove to the ground, only being partially immersed.";
		attack->damage.setNumber(level);
		attack->damage.setSides(3);
		attack->damage.setPlus(0);
		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_ACID);
		attack->delay = 60;
		attack->chance = 20; // 20 percent chance to go off every time

		specials.push_back(attack);

	} else if(specialName == "frost-cone") {
		attack = new SpecialAttack();
		attack->name = "Frost Cone";
		attack->verb = "breathed frost on";
		attack->type = SPECIAL_COLD;
		attack->saveType = SAVE_BREATH;
		attack->targetStr = "^b*ATTACKER* breathes frost on you!*CR*You take ^B*DAMAGE*^b damage!^x";
		attack->roomStr = "^b*ATTACKER* breathes frost on *LOW-TARGET*!^x";
		attack->targetSaveStr = "^cYou almost manage to dive out of the way.^x";
		attack->roomSaveStr = "*TARGET* almost dives out of the way.";
		attack->damage.setNumber(level);
		attack->damage.setSides(4);
		attack->damage.setPlus(0);
		attack->setFlag(SA_SINGLE_TARGET);
		attack->delay = 60;
		attack->chance = 20; // 20 percent chance to go off every time

		specials.push_back(attack);

	} else if(specialName == "gas-breath") {
		attack = new SpecialAttack();
		attack->name = "Gas Breath";
		attack->verb = "breathed gas on";
		attack->type = SPECIAL_BREATH;
		attack->saveType = SAVE_POISON;
		attack->targetStr = "^G*ATTACKER* breathes poisonous gas on you!*CR*You take ^g*DAMAGE*^G damage!^x";
		attack->roomStr = "^G*ATTACKER*'s poisonous breath fills the room!^x";
		attack->targetSaveStr = "^cYou manage to hold your breath, coughing erratically.^x";
		attack->damage.setNumber(level);
		attack->damage.setSides(2);
		attack->damage.setPlus(1);
		attack->delay = 60;
		attack->setFlag(SA_AE_PLAYER);
		attack->setFlag(SA_POISON);
		attack->setFlag(SA_BREATHING_TARGETS);
		attack->chance = 20; // 20 percent chance to go off every time

		specials.push_back(attack);

	} else if(specialName == "wind-blast") {
		attack = new SpecialAttack();
		attack->name = "Wind Blast";
		attack->verb = "breathes a blast of turbulent wind at";
		attack->type = SPECIAL_WIND;
		attack->saveType = SAVE_BREATH;
		attack->targetStr = "^y*ATTACKER* breathes a blast of turbulent wind at you!*CR*You take ^Y*DAMAGE*^y damage!^x";
		attack->roomStr = "^y*ATTACKER* breathes a blast of turbulent wind at *LOW-TARGET*!^x";
		attack->targetSaveStr = "^cYou almost manage to dive out of the way.^x";
		attack->roomSaveStr = "*TARGET* almost dives out of the way.";
		attack->damage.setNumber(level);
		attack->damage.setSides(4);
		attack->damage.setPlus(0);
		attack->setFlag(SA_SINGLE_TARGET);
		attack->delay = 60;
		attack->chance = 20; // 20 percent chance to go off every time

	} else if(specialName == "psychic-blast") {
		attack = new SpecialAttack();
		attack->name = "Psychic Blast";
		attack->verb = "blasts psychic energy at";
		attack->type = SPECIAL_GENERAL;
		attack->saveType = SAVE_MENTAL;
		attack->saveBonus = BONUS_LEVEL_INT;
		attack->targetStr = "^C*ATTACKER* blasts you with psychic energy!*CR*You take ^c*DAMAGE*^C damage!^x";
		attack->roomStr = "^C*ATTACKER* blasts *LOW-TARGET* with psychic energy^x";
		attack->targetSaveStr = "^cYou fight to lock your mind, avoiding the brunt of the attack.^x";
		attack->damage.setNumber(level);
		attack->damage.setSides(4);
		attack->damage.setPlus(0);
		attack->delay = 60;
		attack->chance = 20; // 20 percent chance to go off every time

		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_INTELLIGENCE_REDUCE);
		attack->setFlag(SA_BERSERK_REDUCE);
		attack->setFlag(SA_UNDEAD_WARD_REDUCE);
		specials.push_back(attack);

	} else if(specialName == "energy-drain") {
		attack = new SpecialAttack();
		attack->name = "Energy Drain";
		attack->verb = "drains";
		attack->type = SPECIAL_EXP_DRAIN;
		attack->saveType = SAVE_DEATH;
		attack->targetStr = "^M*ATTACKER* drains your life energy!*CR*You lose *CC:DAMAGE**DAMAGE*^M experience!^x";
		attack->roomStr = "^M*ATTACKER* drains *LOW-TARGET*'s life energy!^x";
		attack->targetSaveStr = "^c*ATTACKER* only partially drains your life energy.^x";
		attack->damage.setNumber(level);
		attack->damage.setSides(10);
		attack->damage.setPlus(level*10);
		attack->delay = 60;
		attack->chance = 10; // 10 percent chance to go off every time

		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_NO_UNDEAD_WARD);
		attack->setFlag(SA_NO_DRAIN_SHIELD);
		specials.push_back(attack);

	} else if(specialName == "touch-of-death") {
		attack = new SpecialAttack();
		attack->name = "Touch of Death";
		attack->verb = "used the touch of death on";
		// Weapon like attack
		attack->type = SPECIAL_WEAPON;
		attack->saveType = SAVE_DEATH;
		attack->saveBonus = BONUS_LEVEL_CON;
		attack->limit = 1;
		attack->chance = 25;

		attack->targetStr = "^r*ATTACKER* used the touch of death on you!^x";
		attack->roomStr = "*ATTACKER* used the touch of death on *LOW-TARGET*.";
		attack->targetFailStr = "^c*ATTACKER* failed the touch of death on you.^x";
		attack->roomFailStr = "*ATTACKER* failed the touch of death on *LOW-TARGET*.";
		attack->targetSaveStr = "^R*ATTACKER*'s touch of death was only a glancing blow!^x";

		attack->setFlag(SA_HALF_HP_DAMAGE);
		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_NO_UNDEAD_WARD);
		attack->setFlag(SA_NO_UNDEAD);

		specials.push_back(attack);
	} else if(specialName == "turn") {
		attack = new SpecialAttack();
		attack->name = "Turn";
		attack->verb = "turns";
		attack->type = SPECIAL_GENERAL;
		attack->saveType = SAVE_LEVEL;
		attack->delay = 30;
		attack->chance = 101;
		attack->targetStr = "^r*ATTACKER* turns you!^x";
		attack->roomStr = "*ATTACKER* turns *LOW-TARGET*.";
		attack->targetSaveStr = "^c*ATTACKER* failed to turn you.^x";
		attack->roomSaveStr = "*ATTACKER* failed to turn *LOW-TARGET*.";

		attack->setFlag(SA_HALF_HP_DAMAGE);
		attack->setFlag(SA_CAN_DISINTEGRATE);
		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_NO_UNDEAD_WARD);
		attack->setFlag(SA_UNDEAD_ONLY);
		attack->setFlag(SA_SAVE_NO_DAMAGE);

		specials.push_back(attack);
	} else if(specialName == "renounce") {
		attack = new SpecialAttack();
		attack->name = "Renounce";
		attack->verb = "renounces";
		attack->type = SPECIAL_GENERAL;
		attack->saveType = SAVE_DEATH;
		attack->saveBonus = BONUS_LEVEL_CON;
		attack->delay = 60;
		attack->chance = 101;


		attack->targetStr = "^r*ATTACKER* renounced you for *CC:DAMAGE**DAMAGE*^r damage!^x";
		attack->roomStr = "*ATTACKER* renounced *LOW-TARGET* with the power of *A-DEITY*!";
		attack->targetSaveStr = "^c*ATTACKER* failed to renounce you with the power of *A-DEITY*.^x";
		attack->roomSaveStr = "*ATTACKER* tried to renounced *LOW-TARGET* with the power of *A-DEITY*.";

		attack->setFlag(SA_HALF_HP_DAMAGE);
		attack->setFlag(SA_CAN_DISINTEGRATE);
		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_HOLY_WAR);
		attack->setFlag(SA_SAVE_NO_DAMAGE);

		specials.push_back(attack);
	} else if(specialName == "tail-slap") {
		attack = new SpecialAttack();
		attack->name = "Tail Slap";
		attack->verb = "tail-slapped";

		// Weapon like attack
		attack->type = SPECIAL_WEAPON;
		attack->saveType = SAVE_LUCK; // Chance to reduce damage
		attack->setFlag(SA_NO_ATTACK_ON_LOW_HP);
		attack->setFlag(SA_SINGLE_TARGET);

		attack->chance = 101;
		attack->delay = 20;

		attack->damage.setNumber(1);
		attack->damage.setSides(level/2);
		attack->damage.setPlus(level);

		attack->targetStr = "^r*ATTACKER* tail-slapped you for ^R*DAMAGE*^r damage.^x";
		attack->roomStr = "^R*ATTACKER* tail-slaps *LOW-TARGET*.^x";
		attack->targetFailStr = "^c*ATTACKER* tried to slap you with *A-HISHER* tail!";
		attack->roomFailStr = "*ATTACKER* tried to slap *LOW-TARGET* with *A-HISHER* tail.";

		attack->setFlag(SA_NO_UNDEAD_WARD);
		attack->setFlag(SA_CHECK_DIE_ROB);
		attack->setFlag(SA_RANDOMIZE_STUN);
		attack->stunLength = 6;

		specials.push_back(attack);

	} else if(specialName == "trample") {
		attack = new SpecialAttack();
		attack->name = "Trample";
		attack->verb = "trampled";

		// Use weapon logic, dodge & ripsote included
		attack->type = SPECIAL_WEAPON;
		attack->setFlag(SA_NO_BLOCK);
		attack->setFlag(SA_SINGLE_TARGET);

		attack->saveType = SAVE_LUCK; // Chance to reduce damage

		attack->setFlag(SA_NO_UNDEAD_WARD);
		attack->setFlag(SA_CHECK_DIE_ROB);
		attack->setFlag(SA_RANDOMIZE_STUN);
		attack->stunLength = 5;

		attack->delay = 90;
		attack->chance = 25;

		attack->damage.setNumber(damage.getNumber() * 2);
		attack->damage.setSides(damage.getSides());
		attack->damage.setPlus(damage.getPlus() * 2);

		attack->targetStr = "^r*ATTACKER* tramples you for ^R*DAMAGE*^r damage!^x";
		attack->roomStr = "^R*ATTACKER* trampled *LOW-VICTIM*!^x";
		attack->targetFailStr = "^c*ATTACKER* almost trampled you.^x";
		attack->roomFailStr = "*ATTACKER* almost trampled *LOW-VICTIM*.";

		specials.push_back(attack);

	} else if(specialName == "poisonous-sting") {
		attack = new SpecialAttack();
		attack->name = "Poisonous Sting";
		attack->verb = "stung";

		// Use weapon logic, dodge & ripsote included
		attack->type = SPECIAL_WEAPON;
		attack->saveType = SAVE_LUCK; // Chance to reduce damage

		attack->setFlag(SA_NO_BLOCK);
		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_NO_UNDEAD_WARD);
		attack->setFlag(SA_CHECK_DIE_ROB);
		attack->setFlag(SA_RANDOMIZE_STUN);
		attack->setFlag(SA_POISON);
		attack->stunLength = 4;

		attack->damage.setNumber(damage.getNumber() * 3 / 2);
		attack->damage.setSides(damage.getSides());
		attack->damage.setPlus(damage.getPlus() * 3 / 2);

		attack->chance = 33;

		attack->targetStr = "^r*ATTACKER* stings you for ^R*DAMAGE*^r damage!^x";
		attack->roomStr = "^R*ATTACKER* stung *LOW-VICTIM*!^x";
		attack->targetFailStr = "^c*ATTACKER* almost stung you.^x";
		attack->roomFailStr = "*ATTACKER* almost stung *LOW-VICTIM*.";

		specials.push_back(attack);

	} else if(specialName == "petrifying-gaze") {
		attack = new SpecialAttack();
		attack->name = "Petrifying Gaze";
		attack->type = SPECIAL_PETRIFY;
		attack->saveType = SAVE_DEATH;
		attack->saveBonus = BONUS_LEVEL_CON;
		attack->maxBonus = 60;

		attack->delay = 30;
		attack->chance = intelligence.getCur()/5;

		attack->targetStr = "^D*ATTACKER* locks *A-HISHER* gaze on you.*CR**A-UPHISHER* gaze turned you to stone!^x";
		attack->roomStr = "*ATTACKER* petrified *LOW-TARGET* with *A-HISHER* gaze!";
		attack->targetSaveStr = "^D*ATTACKER* locks *A-HISHER* gaze on you.*CR*^cYou avoided *ATTACKER*'s petrifying gaze.^x";
		attack->roomSaveStr = "^c*ATTACKER* tried to petrify *LOW-TARGET* with *A-HISHER* gaze.^x";
		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_NO_UNDEAD_WARD);

		specials.push_back(attack);
	} else if(specialName == "petrifying-breath") {
		attack = new SpecialAttack();
		attack->name = "Petrifying Breath";
		attack->type = SPECIAL_PETRIFY;
		attack->saveType = SAVE_BREATH;
		attack->saveBonus = BONUS_LEVEL_DEX;
		attack->maxBonus = 75;

		attack->chance = 20; // 20 percent chance to go off every time
		attack->delay = 60;

		attack->targetStr = "^Y*ATTACKER* breathes a cloud of choking smoke at you!^x";
		attack->roomStr = "^Y*ATTACKER*'s cloud of choking smoke fills the room!^x";
		attack->targetSaveStr = "^cYou managed to avoid *LOW-ATTACKER*'s smoking breath.^x";
		attack->roomSaveStr = "*TARGET* managed to avoid *LOW-ATTACKER*'s smoking breath.";

		attack->setFlag(SA_AE_PLAYER);
		specials.push_back(attack);
	} else if(specialName == "confusing-gaze") {
		attack = new SpecialAttack();
		attack->name = "Confusing Gaze";
		attack->type = SPECIAL_CONFUSE;
		attack->saveType = SAVE_MENTAL;
		attack->saveBonus = BONUS_LEVEL_CON;

		attack->targetStr = "^g%*ATTACKER*'s enigmatic gaze clouds your mind.^x";
		attack->roomStr = "*ATTACKER* fixes *LOW-TARGET* with *A-HISHER* enigmatic gaze.";
		attack->targetSaveStr = "^cYou avoided *LOW-ATTACKER*'s enigmatic gaze.^x";
		attack->roomSaveStr = "*ATTACKER* tried to confuse *LOW-TARGET* with *A-HISHER* gaze.";

		attack->delay = 30;
		attack->chance = intelligence.getCur()/5;

		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_NO_UNCONSCIOUS);
		specials.push_back(attack);
	} else if(specialName == "zap-mana") {
		attack = new SpecialAttack();
		attack->name = "Zap Mana";
		attack->type = SPECIAL_NO_DAMAGE;
		attack->setFlag(SA_SINGLE_TARGET);
		attack->chance = intelligence.getCur()/10;


		specials.push_back(attack);
	} else if(specialName == "death-gaze") {
		attack = new SpecialAttack();
		attack->name = "Death Gaze";
		attack->saveType = SAVE_DEATH;
		attack->saveBonus = BONUS_LEVEL_CON;

		attack->targetStr = "^m*ATTACKER* locks *A-HISHER* gaze on you.*CR**A-HISHER*'s deadly gaze drains your life.^x";
		attack->roomStr = "*ATTACKER* drains *LOW-TARGET*'s life with *A-HISHER* gaze!";
		attack->targetSaveStr = "You avoided *LOW-ATTACKER*'s deadly gaze.";
		attack->roomSaveStr = "*ATTACKER* tried to kill *LOW-TARGET* with *A-HISHER* gaze.";


		attack->setFlag(SA_NO_UNCONSCIOUS);
		attack->setFlag(SA_SINGLE_TARGET);
		attack->setFlag(SA_HALF_HP_DAMAGE);

		attack->delay = 30;
		attack->chance = intelligence.getCur()/5;
		specials.push_back(attack);
	} else {
		std::cout << "Unknown special name - " << specialName << "." << std::endl;
	}

	return(attack);
}

bstring SpecialAttack::getName() {
	return(name);
}
