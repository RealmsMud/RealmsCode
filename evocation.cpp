/*
 * evocation.cpp
 *	 Evocation spells
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


//*********************************************************************
//						getRandColor
//*********************************************************************

char getRandColor() {
	switch(mrand(1,7)) {
	case 1:
		return('r');
	case 2:
		return('b');
	case 3:
		return('y');
	case 4:
		return('m');
	case 5:
		return('w');
	case 6:
		return('c');
	case 7:
		return('g');
	default:
		return('w');
	}
}

//*********************************************************************
//						splMagicMissile
//*********************************************************************

int splMagicMissile(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player	*pPlayer = player->getPlayer();
	int		maxMissiles=0, mpNeeded=0, canCast=0, num=0;
	int		missileDmg = 0, a=0;
	char	colorCh=0;
	Creature *target=0;
	Monster	*mTarget=0;

	if(!player->spellIsKnown(S_MAGIC_MISSILE) && spellData->how == CAST) {
		player->print("You do not know that spell.\n");
		return(0);
	}

	if(	spellData->how == CAST &&
		(	player->getClass() == MAGE ||
			player->getClass() == LICH ||
			player->isStaff() ||
			(!pPlayer || pPlayer->getSecondClass() == MAGE) ) )
		canCast=1;


	if(spellData->how == CAST && !canCast) {
		player->print("You are unable to cast that spell.\n");
		return(0);
	}

	if(cmnd->num < 2) {
		player->print("Magic-missile must be cast on a target creature.\n");
		return(0);
	}
	if((target = player->findMagicVictim(cmnd->str[2], cmnd->val[2], spellData, true, false, "Cast on what?\n", "You don't see that here.\n")) == NULL)
		return(0);

//	target = player->getRoom()->findCreature(player, cmnd->str[2], cmnd->val[2], true, true);
//	if(!target || target == player) {
//		player->print("That is not here.\n");
//		return(0);
//	}
	mTarget = target->getMonster();

	// default is to cast max number of missiles.

	if(!player->canAttack(target))
		return(0);

	if(spellData->how == CAST) {
		maxMissiles = spellData->level / 2;
	} else {
		maxMissiles = mrand(2,4);
		missileDmg = mrand (2,4)+1;
	}

	num = MAX(1,maxMissiles);
	mpNeeded = 2*num;

	if(cmnd->num > 3 && spellData->how == CAST) {
		if(cmnd->str[3][0] != 'n') {
			player->print("Syntax: cast magic-missile (target) n #\n");
			return(0);
		}

		num = cmnd->val[3];
		if(num > maxMissiles) {
			player->print("You can only cast a maximum of %d missiles.\n", MAX(1,maxMissiles));
			return(0);
		}

		mpNeeded = 2*num;

	}

	if(player->getClass() != LICH) {
		if(mpNeeded > player->mp.getCur()) {
			player->print("Not enough magic points.\n");
			return(0);
		}
	} else if(mpNeeded >= player->hp.getCur()) {
		player->print("Not enough hit points.\n");
		return(0);
	}

	if(spellData->how == CAST) {
		if(player->getClass() != LICH)
			player->mp.decrease(mpNeeded);
		else
			player->hp.decrease(mpNeeded);
	}

	player->smashInvis();

	target->print("%M casts a magic missile spell.\n", player);
	broadcast(player->getSock(), target->getSock(), player->getRoom(), "%M casts a magic-missile spell.", player);
	if(mTarget)
		mTarget->addEnemy(player);

	if(spellData->how == CAST && player->isPlayer())
		player->getPlayer()->statistics.offensiveCast();

	if(mTarget && mrand(1,100) <= mTarget->getMagicResistance()) {
		player->print("Your missiles have no effect on %N.\n", mTarget);
		return(0);
	}

	while(a < num) {
		a++;
		missileDmg = mrand(2,5) + bonus(player->intelligence.getCur())/2;
		if(mrand(1,100) <= 25 && target->isEffected("resist-magic")) {
			player->print("Your magic-missile deflects off of %N.\n", target);
			broadcast(player->getSock(), target->getSock(), player->getRoom(), "%M's magic-missile deflects off of %N.", player, target);
			target->print("%M's magic-missile deflects off of you.\n", player);
			continue;
		}
		colorCh = getRandColor();

		player->printColor("^@^%cYour magic-missile strikes %N for %d damage.\n", colorCh, target, missileDmg);
		target->printColor("^@^%c%M's magic-missile strikes you for %d damage!\n", colorCh, player, missileDmg);
		broadcast(player->getSock(), target->getSock(), player->getRoom(),
			"^@^%c%M's magic-missile strikes %N!", colorCh, player, target);
		if(player->doDamage(target, missileDmg, CHECK_DIE))
			return(1);
	}

	return(1);
}


//*********************************************************************
//						doOffensive
//*********************************************************************
// The actual routine for damage.

int doOffensive(Creature *caster, Creature* target, SpellData* spellData, const char *spellname, osp_t *osp, bool multi) {
	Player	*pTarget = target->getPlayer(), *pCaster = caster->getPlayer();
	Monster	*mTarget = target->getMonster();
	Monster	*mCaster = caster->getMonster();
	BaseRoom* room = caster->getRoom();
	int		m=0, bns=0;
	Damage damage;
	int		slvl=0, skillPercent=0;
	//unsigned long addrealm=0;
	bstring skill = "";
	int dmgType = MAGICAL;
	if(osp->drain)
		dmgType = MAGICAL_NEGATIVE;

	if(osp->realm != NO_REALM)
		skill = realmSkill(osp->realm);
	else
		skill = spellData->skill;

	if(skill == "") {
		caster->print("The spell unexpectedly fails.\n");
		return(0);
	}

	if(!caster->isStaff()) {
		if(caster->getClass() != LICH) {
			if(caster->mp.getCur() < osp->mp && spellData->how == CAST) {
				caster->print("Not enough magic points.\n");
				return(0);
			}
		} else {
			if(caster->hp.getCur() <= osp->mp && spellData->how == CAST) {
				caster->print("Sure, and die in the process?\n");
				return(0);
			}
			if(osp->mp > (int)spellData->level*5 && spellData->how == CAST) {
				caster->print("You are not experienced enough to cast that spell.\n");
				return(0);
			}
		}
	}

	if(!caster->spellIsKnown(osp->splno) && spellData->how == CAST) {
		caster->print("You don't know that spell.\n");
		return(0);
	}


	caster->smashInvis();

	if(osp->bonus_type) {
		bns = bonus((int) caster->intelligence.getCur());
		if(caster->getClass() == MAGE || caster->getClass() == LICH || caster->isStaff())
			bns = (bns * 3)/2;

		if(spellData->how == CAST || spellData->how == WAND || spellData->how == SCROLL) {
			skillPercent = (int)caster->getSkillLevel(skill) * 100 / MAXALVL;

			switch(osp->bonus_type) {
			case 1:
				bns += skillPercent / 8;
				break;
			case 2:
				bns += skillPercent / 4;
				break;
			case 3:
				bns += skillPercent / 3;
				break;
			default:
				break;
			}
		}

		if(spellData->how == WAND || spellData->how == SCROLL)
			bns /= 2;

		if( (room->flagIsSet(R_ROOM_REALM_BONUS) && room->hasRealmBonus(osp->realm)) ||
			(room->flagIsSet(R_OPPOSITE_REALM_BONUS) && room->hasOppositeRealmBonus(osp->realm)))
			bns *= 2;
		else if( (room->flagIsSet(R_ROOM_REALM_BONUS) && room->hasOppositeRealmBonus(osp->realm)) ||
				(room->flagIsSet(R_OPPOSITE_REALM_BONUS) && room->hasRealmBonus(osp->realm)))
			bns = MIN(-bns, -5);
	}



	// Cast on self
	if(caster == target) {
		damage.set(MAX(1, osp->damage.roll() + bns));
		caster->modifyDamage(caster, dmgType, damage);
		caster->hp.decrease(damage.get());

		if(spellData->how == CAST && caster->getClass() != LICH)
			caster->mp.decrease(osp->mp);
		else if(spellData->how == CAST && caster->getClass() == LICH)
			caster->hp.decrease(osp->mp);

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			caster->print("You cast a %s spell on yourself.\n", spellname);

			if(!multi && spellData->how == CAST && caster->isPlayer())
				caster->getPlayer()->statistics.offensiveCast();
			if(caster->negAuraRepel())
				caster->printColor("^cYour negative aura repelled some of the damage.\n");

			if(caster->isPlayer())
				caster->getPlayer()->statistics.magicDamage(damage.get(), (bstring)"a " + spellname + " spell");
			caster->printColor("The spell did %s%d^x damage.\n", caster->customColorize("*CC:DAMAGE*").c_str(), damage.get());
			broadcast(caster->getSock(), room, "%M casts a %s spell on %sself.",
				caster, spellname, caster->himHer());
			broadcastGroup(false, caster, "%M cast a %s spell on %sself for *CC:DAMAGE*%d^x damage, %s%s\n",
				caster, spellname, caster->himHer(), damage.get(), caster->heShe(), caster->getStatusStr());
		} else if(spellData->how == POTION) {
			caster->print("Yuck! That's terrible!\n");
			caster->print("%d hit points removed.\n", damage.get());
		}

		if(caster->hp.getCur() < 1) {
			caster->print("Don't be stupid.\n");
			caster->hp.setCur(1);
			return(2);
		}

	// Cast on monster or caster
	} else {

		if(mTarget && pCaster) {
			// Activates lag protection.
			if(pCaster->flagIsSet(P_LAG_PROTECTION_SET))
				pCaster->setFlag(P_LAG_PROTECTION_ACTIVE);
		}

		if(!caster->canAttack(target))
			return(0);

		if(pCaster)
			log_immort(false, pCaster, "%s cast a %s on %s.\n", pCaster->name, get_spell_name(osp->splno), target->name);

		if(pTarget && pCaster) {
			if(pTarget != pCaster && !pTarget->flagIsSet(P_OUTLAW)) {
				if(room->isPkSafe() && !pCaster->isDm()) {
					pCaster->print("No killing allowed in this room.\n");
					return(0);
				}
			}
			if(pCaster->vampireCharmed(pTarget) || (pCaster->flagIsSet(P_CHARMED) && pTarget->hasCharm(pCaster->name))) {
				pCaster->print("You just can't bring yourself to do that.\n");
				return(0);
			}
		}

		if(spellData->how == CAST && caster->getClass() != LICH)
			caster->mp.decrease(osp->mp);
		else if(spellData->how == CAST && caster->getClass() == LICH)
			caster->hp.decrease(osp->mp);

		if(spell_fail(caster, spellData->how))
			return(0);

		if(mTarget) {
			if(mrand(1,100) < mTarget->getMagicResistance()) {
				caster->printColor("^MYour spell has no effect on %N.\n", mTarget);
				if(pCaster)
					broadcast(pCaster->getSock(), room, "%M's spell has no effect on %N.", caster, mTarget);
				return(0);
			}
		}

		slvl = get_spell_lvl(osp->splno);

		if(slvl > 0 && mCaster && mCaster->isPet() && !pTarget) {

			if( (target->flagIsSet(M_NO_LEVEL_FOUR) && slvl <= 4) ||
				(target->flagIsSet(M_NO_LEVEL_THREE) && slvl <= 3) ||
				(target->flagIsSet(M_NO_LEVEL_TWO) && slvl <= 2) ||
				(target->flagIsSet(M_NO_LEVEL_ONE) && slvl <= 1)
			) {
				broadcast(isCt, caster->getSock(), room,
					"*DM* %M tried to cast a %s spell on %N.",
					caster, get_spell_name(osp->splno), target);
				if(caster->getClass() != LICH) {
					caster->mp.increase(osp->mp);
				} else {
					caster->hp.increase(osp->mp);
				}
				return(0);
			}
		}


		if(!pTarget && (get_spell_lvl(osp->splno) > 0)) {

			if( (target->flagIsSet(M_NO_LEVEL_FOUR) && slvl <= 4) ||
			        (target->flagIsSet(M_NO_LEVEL_THREE) && slvl <= 3) ||
			        (target->flagIsSet(M_NO_LEVEL_TWO) && slvl <= 2) ||
			        (target->flagIsSet(M_NO_LEVEL_ONE) && slvl <= 1) ) {
				caster->print("Your %s was not powerful enough to harm %N!!\n", get_spell_name(osp->splno), target);
				caster->smashInvis();
				return(1);
			}
		}

		damage.set(osp->damage.roll() + bns);
		target->modifyDamage(caster, dmgType, damage, osp->realm);
		damage.set(MAX(0, damage.get()));

		m = MIN(target->hp.getCur(), damage.get());

		//addrealm = (m * target->getExperience()) / MAX(1, target->hp.getMax());
		//addrealm = MIN(addrealm, target->getExperience());
		//if(mTarget && !mTarget->isPet())
		//	caster->addRealm(addrealm, osp->realm);

		if(spellData->how == CAST && pCaster && osp->realm != NO_REALM)
			pCaster->checkImprove(skill, true);

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {

			caster->print("You cast a %s spell on %N.\n", spellname, target);
			if(target->negAuraRepel()) {
				caster->printColor("^c%M's negative aura repels some of the damage.\n", target);
				target->printColor("^cYour negative aura repelled some of the damage.\n");
			}

			if(!multi && spellData->how == CAST && caster->isPlayer())
				caster->getPlayer()->statistics.offensiveCast();
			if(caster->isPlayer())
				caster->getPlayer()->statistics.magicDamage(damage.get(), (bstring)"a " + spellname + " spell");

			caster->printColor("The spell did %s%d^x damage.\n", caster->customColorize("*CC:DAMAGE*").c_str(), damage.get());
			broadcast(caster->getSock(), target->getSock(), room, "%M casts a %s spell on %N.",
				caster, spellname, target);

			if(mCaster) {
				target->printColor("^r%M casts a %s spell on you for ^R%d^r damage.\n", caster, spellname, damage.get());
			} else  {
				target->printColor("%M casts a %s spell on you for %s%d^x damage.\n", caster, spellname, target->customColorize("*CC:DAMAGE*").c_str(), damage.get());
			}

			if(!pTarget) {
				//if(is_charm_crt(target->name, caster))
				//del_charm_crt(target, caster);

				// BUGFIX:  Fix the pet casting exp bug here
			    mTarget->addEnemy(caster);
				mTarget->adjustThreat(caster, m);
			}

			// pet should attack the target they just casted on
			if(mCaster)
				mCaster->addEnemy(target);

			target->hp.decrease(damage.get());

			broadcastGroup(false, target, "%M cast a %s spell on ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n",
				caster, spellname, target, damage.get(), target->heShe(), target->getStatusStr());

			if(caster->getClass() != LICH && caster->hp.getCur() < caster->hp.getMax() && damage.getDrain()) {
				caster->print("%M's life force revitalizes your strength.\n", target);
				caster->hp.increase(damage.getDrain());
			}
		}

		bool wasKilled = false, meKilled = false;

		meKilled = caster->doReflectionDamage(damage, target);
		wasKilled = target->hp.getCur() < 1;

		if(	(wasKilled && (
				pCaster ||
				(mCaster && !mCaster->flagIsSet(M_GREEDY) && !mCaster->flagIsSet(M_POLICE))
			)) ||
			meKilled)
		{
			Creature::simultaneousDeath(caster, target, false, false);
			return(2);
		} else if( mCaster &&
			pTarget &&
			(mCaster->flagIsSet(M_GREEDY) || mCaster->flagIsSet(M_POLICE)) &&
		    (pTarget->hp.getCur() < pTarget->hp.getMax()/10))
		{
			pTarget->printColor("^r%M knocks you unconscious.\n", mCaster);
			pTarget->knockUnconscious(39);
			broadcast(pTarget->getSock(), room, "%M knocks %N unconscious.", mCaster, pTarget);
			pTarget->hp.setCur(pTarget->hp.getMax()/20);

			pTarget->clearAsEnemy();
			// Prevent negative hp bug
			if(damage.getReflected())
				mCaster->hp.setCur(1);

			mCaster->clearEnemy(pTarget);
			if(mCaster->flagIsSet(M_POLICE)) {
				if(!mCaster->jail.id && !pTarget->isStaff()) {
					broadcast(pTarget->getSock(), room, "%M picks %N up and hauls %s off to jail.", mCaster, pTarget, pTarget->himHer());
					broadcast("### %M was hauled off to jail by %N", pTarget, mCaster);
				} else {
					broadcast(pTarget->getSock(), room, "%M picks %N up and hauls %s off.", mCaster, pTarget, pTarget->himHer());
				}
				mCaster->toJail(pTarget);
				return(2);
			}
			if(mCaster->flagIsSet(M_GREEDY)) {
				broadcast(pTarget->getSock(), room, "%M rummages through %N's inventory.", mCaster, pTarget);
				broadcast("### %M was mugged by %N.", pTarget, mCaster);
				mCaster->grabCoins(pTarget);
				return(2);
			}
			return(1);
		}

		Creature::simultaneousDeath(caster, target, false, false);

	}

	return(1);
}

//*********************************************************************
//						splOffensive
//*********************************************************************
// This function is called by all spells whose sole purpose is to do
// damage to a creature.
Creature* Creature::findMagicVictim(bstring toFind, int num, SpellData* spellData, bool aggressive, bool selfOk, bstring noVictim, bstring notFound) {
	Creature* victim=0;
		Player	*pVictim=0;
		if(toFind == "") {
			if(spellData->how != POTION) {
				if(hasAttackableTarget()) {
					return(getTarget());
				}
				if(!noVictim.empty())
					bPrint(noVictim);
				return(NULL);
			} else {
				return(this);
			}
		} else {
			if(spellData->how == POTION) {
				bPrint("You can only use a potion on yourself.\n");
				return(NULL);
			}
			if(toFind == ".") {
				// Cast offensive spell on self
				return(this);
			} else {
				victim = getRoom()->findCreature(this, toFind.c_str(), num, true, true);
				pVictim = victim->getPlayer();

				if(!victim || (aggressive && (pVictim || victim->isPet()) && toFind.length() < 3)
						|| (!selfOk && victim == this)) {
					if(!notFound.empty())
						bPrint(notFound);
					return(NULL);
				}
				if(isMonster()) {
					if(victim == this) {
						// for monster casting we need to make sure its not on itself
						victim = getRoom()->findCreature(this, toFind.c_str(), 2, true, true);
						// look for second creature with same name
						if(!victim || victim == this)
							return(NULL);
					}
				}
				return(victim);
			}
		}
}
int splOffensive(Creature* player, cmd* cmnd, SpellData* spellData, char *spellname, osp_t *osp) {
	Creature* target=0;

	if((target = player->findMagicVictim(cmnd->str[2], cmnd->val[2], spellData, true, false, "Cast on what?\n", "You don't see that here.\n")) == NULL)
		return(0);

	return(doOffensive(player, target, spellData, spellname, osp));
}

//*********************************************************************
//						doMultiOffensive
//*********************************************************************
// the actual routine to do the AOE damage

int doMultiOffensive(Creature* player, Creature* target, int *found_something, int *something_died, SpellData* spellData, char *spellname, osp_t *osp) {
	int		ret=0;

	if(!*found_something)
		player->getRoom()->wake("Loud noises disturb your sleep.", true);

	*(found_something) = 1;
	// check here so doOffensive doesn't abort our routine prematurely
	if(!player->canSee(target))
		return(-1);
	if(!player->canAttack(target))
		return(-1);

	ret = doOffensive(player, target, spellData, spellname, osp, true);
	if(ret == 0)
		return(0);
	if(ret == 2)
		*(something_died) = 1;
	return(1);
}


//*********************************************************************
//						splMultiOffensive
//*********************************************************************
// this type of spell causes damage to everyone in a given room

int splMultiOffensive(Creature* player, cmd* cmnd, SpellData* spellData, char *spellname, osp_t *osp) {
	Creature* target=0;
	ctag	*cp=0;
	int		monsters=0, players=0, len=0;
	int		something_died=0, found_something=0;


	if(player->isMonster())
		return(0);

	if(spellData->how == CAST && !player->checkMp(5))
		return(0);

	if(cmnd->num == 2)
		monsters = 1;
	else {
		len = strlen(cmnd->str[2]);
		if(!strncmp(cmnd->str[2], "monsters", len)) {
			monsters = 1;
		} else if(!strncmp(cmnd->str[2], "all", len)) {
			monsters = 1;
			players = 1;
		} else if(!strncmp(cmnd->str[2], "players", len)) {
			players = 1;
		} else {
			player->print("Usage: cast %s [<monsters>|<players>|<all>]\n", spellname);
			return(0);
		}
	}

	cmnd->num = 3;
	if(spellData->how == CAST)
		player->subMp(5);

	if(monsters) {
		cp = player->getRoom()->first_mon;
		while(cp) {
			target = cp->crt;
			cp = cp->next_tag;

			// skip all pets - they are treated as players
			if(target->isPet())
				continue;

			if(!doMultiOffensive(player, target, &found_something, &something_died, spellData, spellname, osp))
				return(found_something);

		}
	}
	if(players) {
		cp = player->getRoom()->first_ply;
		while(cp) {
			target = cp->crt;
			cp = cp->next_tag;

			// skip self
			if(target == player)
				continue;

			if(!doMultiOffensive(player, target, &found_something, &something_died, spellData, spellname, osp))
				return(found_something);

		}
		cp = player->getRoom()->first_mon;
		while(cp) {
			target = cp->crt;
			cp = cp->next_tag;

			// only do pets
			if(!target->isPet())
				continue;
			if(target->getMaster() == player)
				continue;

			if(!doMultiOffensive(player, target, &found_something, &something_died, spellData, spellname, osp))
				return(found_something);

		}
	}

	if(!found_something && spellData->how == CAST) {
		if(player->getClass() == LICH)
			player->hp.increase(5);
		else
			player->mp.increase(5);
		player->print("You don't see anything here to cast it on!\n");
	}

	if(found_something && spellData->how == CAST && player->isPlayer())
		player->getPlayer()->statistics.offensiveCast();
	return(found_something + something_died);
}

//*********************************************************************
//						splDarkness
//*********************************************************************
// This spell allows a player to cast darkness spell.

int splDarkness(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player* pPlayer = player->getPlayer();
	Creature* target=0;
	Object* object=0;

	player->smashInvis();

	if(spell_fail(player, spellData->how))
		return(0);

	if(cmnd->num == 2) {
		target = player;
		if(spellData->how == CAST && !player->checkMp(15))
			return(0);

		player->print("You cast a darkness spell.\n");
		broadcast(player->getSock(), player->getRoom(), "%M casts a darkness spell.", player);

		if(spellData->how == CAST)
			player->subMp(15);

	} else {
		if(noPotion(player, spellData))
			return(0);

		if(spellData->how == CAST && !player->checkMp(20))
			return(0);

		if(	spellData->how == CAST &&
			player->getClass() != MAGE &&
			player->getClass() != LICH &&
			player->getClass() != BARD &&
			player->getClass() != CLERIC &&
			player->getClass() != DRUID &&
			!player->isCt())
		{
			player->print("Your class is unable to cast that spell upon others.\n");
			return(0);
		}

		cmnd->str[2][0] = up(cmnd->str[2][0]);

		target = player->getRoom()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

		if(target) {

			if(target->inCombat(false)) {
				player->print("Not in the middle of combat.\n");
				return(0);
			}

			if(target->isMonster()) {
				if(!player->canAttack(target))
					return(0);
				target->getMonster()->addEnemy(player);
			} else {
				if(	target->getLevel() < 4 &&
					!player->checkStaff("You cannot cast that spell on %N, yet.\n", target)
				)
					return(0);
				// this purposely does not check the refuse list
				if(target->flagIsSet(P_LINKDEAD)) {
					player->print("%M doesn't want that cast on them right now.\n", target);
					return(0);
				}
			}

			player->print("You cast a darkness spell on %N.\n", target);
			broadcast(player->getSock(), target->getSock(), player->getRoom(), "%M casts a darkness spell on %N.", player, target);
			target->print("%M casts a darkness spell on you.\n", player);

			if(spellData->how == CAST)
				player->subMp(20);

			if(!player->isStaff() || target->isStaff()) {
				if(target->isStaff() || target->chkSave(SPL, player, -25)) {
					player->printColor("^y%M resisted your darkness spell!\n", target);
					target->printColor("^yYou resist %N's darkness spell!\n", player);
					return(1);
				}
			}

		// only enchant objects if a pPlayer
		} else if(pPlayer && spellData->how == CAST) {

			object = findObject(pPlayer, pPlayer->first_obj, cmnd, 2);

			if(object) {

				if(spellData->how == CAST && !player->checkMp(25))
					return(0);

				if(!canEnchant(pPlayer, spellData))
					return(0);

				if(!pPlayer->spellIsKnown(S_ENCHANT)) {
					pPlayer->print("You must know the enchant spell to cast darkness on objects.\n");
					return(0);
				}

				if(object->flagIsSet(O_DARKNESS)) {
					pPlayer->printColor("%O is already enchanted with darkness.\n", object);
					return(0);
				}

				if(!decEnchant(pPlayer, spellData->how))
					return(0);

				if(spellData->how == CAST)
					player->subMp(25);

				object->setFlag(O_DARKNESS);

				pPlayer->printColor("%O begins to emanate darkness.\n", object);
				broadcast(pPlayer->getSock(), pPlayer->getRoom(), "%M enchants %1P with darkness.", pPlayer, object);

				if(!pPlayer->isDm())
					log_immort(true, pPlayer, "%s enchants a %s in room %s.\n", pPlayer->name, object->name,
						pPlayer->getRoom()->fullName().c_str());

				pPlayer->setFlag(P_DARKNESS);
				object->clearFlag(O_JUST_BOUGHT);
				return(1);
			}

		}

		if(!target && !object) {
			player->print("You don't see that here.\n");
			return(0);
		}

	}

	// final routines for creatures only
	if(target) {
		if(spellData->how == CAST) {
			if(player->getRoom()->magicBonus()) {
				player->print("The room's magical properties increase the power of your spell.\n");
			}
			target->addEffect("darkness", -2, -2, player, true, player);
		} else {
			target->addEffect("darkness");
		}
	}

	return(1);
}
