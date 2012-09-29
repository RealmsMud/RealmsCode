/*
 * transmutation.cpp
 *	 Transmutation spells
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
#include "effects.h"
#include "move.h"

//*********************************************************************
//						splLevitate
//*********************************************************************
// This function allows players to cast the levitate spell, thus allowing
// them to levitate over traps or up mountain cliffs.

int splLevitate(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "levitation", "levitate"));
}

//*********************************************************************
//						splEntangle
//*********************************************************************

int splEntangle(Creature* player, cmd* cmnd, SpellData* spellData) {
	int		bns=0, nohold=0, dur=0, statmod=0;
	Creature* target=0;

	if(	player->getClass() != DRUID &&
		player->getClass() != RANGER &&
		!player->isCt() && spellData->how == CAST
	) {
		player->print("You are unable to cast that spell.\n");
		return(0);
	}


	if(!player->getRoomParent()->isOutdoors() && !player->isCt()) {
		player->print("You cannot cast that spell indoors.\n");
		return(0);
	}



	// Cast entangle
	if(cmnd->num == 2) {
		if(spellData->how != POTION) {
			player->print("Entangle whom?\n");
			return(0);
		} else if(player->isEffected("mist")) {
			player->print("Nothing happens.\n");
			return(0);
		} else {

			player->print("Weeds and vines reach out to engangle you!\n");
			broadcast(player->getSock(), player->getParent(), "%M becomes entangled by vines and weeds!", player);

			player->unhide();
			player->addEffect("hold-person", 0, 0, player, true, player);
			return(1);
		}

	// Cast entangle on another player
	} else {
		if(noPotion(player, spellData))
			return(0);

		cmnd->str[2][0] = up(cmnd->str[2][0]);

		target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], true, true);

		if(!target) {
			player->print("Cast on whom?\n");
			return(0);
		}


		if(player->isPlayer() && target->isPlayer() && target->inCombat(false) && !player->flagIsSet(P_OUTLAW)) {
			player->print("Not in the middle of combat.\n");
			return(0);
		}

		if(target->isPlayer() && !player->isCt() && target->isCt()) {
			player->printColor("^yYour spell failed.\n");
			return(0);
		}

		if(target->isPlayer() && target->isEffected("hold-person")) {
			player->printColor("%M is already held fast.\n^yYour spell fails.\n", target);
			return(0);
		}

		if(target->isPlayer() && !player->isDm() && target->isDm()) {
			player->printColor("^yYour spell failed.\n");
			return(0);
		}


		statmod = (bonus((int)target->strength.getCur()) + bonus((int)target->dexterity.getCur()))/2;

		player->print("You cast an entangle spell on %N.\n", target);
		target->print("%M casts an entangle spell on you.\n", player);
		broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts an entangle spell on %N.", player, target);


		if(	target->isPlayer() &&
			(	target->flagIsSet(P_UNCONSCIOUS) ||
				target->isEffected("petrification") ||
				(	player->getClass() != RANGER &&
					player->getClass() != DRUID &&
					target->isEffected("mist")
				)
			)
		) {
			player->printColor("^yYour spell failed.\n");
			return(0);
		}

		if(target->isPlayer() && mrand(1,100) <= 50 && target->isEffected("resist-magic")) {
			player->printColor("^yYour spell failed.\n");
			return(0);
		}

		if(spellData->how == CAST)
			bns = 5*((int)player->getLevel() - (int)target->getLevel()) + 2*crtWisdom(target) - 2*bonus((int)player->intelligence.getCur());

		if(target->isMonster()) {
			if(	target->flagIsSet(M_DM_FOLLOW) ||
				target->flagIsSet(M_PERMENANT_MONSTER) ||
				target->isEffected("resist-magic") ||
				target->flagIsSet(M_RESIST_STUN_SPELL) ||
				target->isEffected("reflect-magic")
			)
				nohold = 1;

			if(	target->flagIsSet(M_LEVEL_BASED_STUN) &&
				(((int)target->getLevel() - (int)player->getLevel()) > ((player->getClass() == LICH || player->getClass() == MAGE) ? 6:4))
			)
				nohold = 1;
		}

		if(target->isMonster()) {
			player->smashInvis();
			if((!target->chkSave(SPL, player, bns) && !nohold) || player->isCt()) {

				if(spellData->how == CAST)
					dur = mrand(9,18) + 2*bonus((int)player->intelligence.getCur()) - statmod;
				else
					dur = mrand(9,12);

				target->stun(dur);


				player->print("%M is held in place by vines and weeds!\n", target);
				broadcast(player->getSock(), player->getParent(), "%M's spell entangles %N in place!", player, target);


				if(player->isCt())
					player->print("*DM* %d seconds.\n", dur);

				if(target->isMonster())
					target->getAsMonster()->addEnemy(player);

			} else {
				player->print("%M resisted your spell.\n", target);
				broadcast(player->getSock(), target->getRoomParent(), "%M resisted %N's entangle spell.",target, player);
				if(target->isMonster())
					target->getAsMonster()->addEnemy(player);
			}

		} else {
			player->smashInvis();
			if(!target->chkSave(SPL, player, bns) || player->isCt()) {

				target->print("You are entangled in vines and weeds!\nYou are unable to move!\n");

				broadcast(target->getSock(), player->getRoomParent(), "%M becomes entangled in vines and weeds!", target);

				if(spellData->how == CAST)
					dur = mrand(12,18) + 2*bonus((int)player->intelligence.getCur()) - statmod;
				else
					dur = mrand(9,12);

				if(player->isCt())
					player->print("*DM* %d seconds.\n", dur);

				target->addEffect("hold-person", 0, 0, player, true, player);
				//*********************************************************************
			} else {
				player->print("%M resisted your spell.\n", target);
				broadcast(player->getSock(), target->getSock(), target->getRoomParent(), "%M resisted %N's hold-person spell.",target, player);
				target->print("%M tried to cast a hold-person spell on you.\n", player);
			}
		}
	}

	return(1);
}

//*********************************************************************
//						splPassWithoutTrace
//*********************************************************************
// This function makes a target player able to pass through rooms without
// leaving a trace

int splPassWithoutTrace(Creature* player, cmd* cmnd, SpellData* spellData) {
	if(	spellData->how == CAST &&
		!player->isStaff() &&
		player->getClass() != DRUID &&
		player->getClass() != RANGER &&
		player->getDeity() != LINOTHAN
	) {
		player->print("Only druids, rangers, and followers of Linothan may cast this spell.\n");
		return(0);
	}

	return(splGeneric(player, cmnd, spellData, "a", "pass-without-trace", "pass-without-trace"));
}

//*********************************************************************
//						splFly
//*********************************************************************
// This function allows players to cast the fly spell. It will
// allow the player to fly to areas that are otherwise unreachable
// by foot.

int splFly(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "fly", "fly", MAX(1,spellData->level/3)));
}

//*********************************************************************
//						splInfravision
//*********************************************************************
// This spell allows a player to cast an infravision spell which will
// allow them to see in a dark/magically dark room

int splInfravision(Creature* player, cmd* cmnd, SpellData* spellData) {
	Creature* target=0;

	if(cmnd->num == 2) {
		target = player;
		if(spellData->how == CAST && !player->checkMp(5))
			return(0);

		player->print("You cast an infravision spell.\n");
		broadcast(player->getSock(), player->getParent(), "%M casts an infravision spell.", player);

		if(spellData->how == CAST) {
			player->subMp(5);
			if(player->getRoomParent()->magicBonus())
				player->print("The room's magical properties increase the power of your spell.\n");
		}

	} else {
		if(noPotion(player, spellData))
			return(0);

		if(spellData->how == CAST && !player->checkMp(10))
			return(0);

		if(	spellData->how == CAST &&
			player->getClass() != MAGE &&
			player->getClass() != LICH &&
			player->getClass() != BARD &&
			player->getClass() != CLERIC &&
			player->getClass() != DRUID &&
			!player->isStaff()
		) {
			player->print("Your class is unable to cast that spell upon others.\n");
			return(0);
		}

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

		if(!target) {
			player->print("You don't see that here.\n");
			return(0);
		}

		if(checkRefusingMagic(player, target))
			return(0);

		if(target->isMonster()) {
			player->print("%M doesn't need that spell right now.\n", target);
			return(0);
		}

		if(spellData->how == CAST) {
			player->subMp(10);
			if(player->getRoomParent()->magicBonus())
				player->print("The room's magical properties increase the power of your spell.\n");

		}

		player->print("You cast an infravision spell on %N.\n", target);
		broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts an infravision spell on %N.", player, target);
		target->print("%M casts an infravision spell on you.\n", player);

	}
	if(target->inCombat(false))
		player->smashInvis();

	if(spellData->how == CAST) {
		if(player->getRoomParent()->magicBonus())
			player->print("The room's magical properties increase the power of your spell.\n");
		target->addEffect("infravision", -2, -2, player, true, player);
	} else {
		target->addEffect("infravision");
	}

	return(1);
}

//*********************************************************************
//						splKnock
//*********************************************************************

int splKnock(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player	*pPlayer = player->getAsPlayer();
	int			chance=0, canCast=0, mpNeeded=0;
	Exit		*exit=0;

	if(spellData->how == CAST) {
		if(	player->getClass() == MAGE ||
			player->getClass() == LICH ||
			player->getClass() == BARD ||
			(!pPlayer || pPlayer->getSecondClass() == MAGE) ||
			player->isStaff()
		)
			canCast=1;
		if(player->getClass() == CLERIC && player->getDeity() == KAMIRA)
			canCast=1;
		if(!player->spellIsKnown(S_KNOCK)) {
			player->print("You do not know that spell.\n");
			return(0);
		}
		if(!canCast) {
			player->print("You cannot cast that spell.\n");
			return(0);
		}
		if(cmnd->num < 3) {
			player->print("Syntax: cast knock <exit>\n");
			return(0);
		}
	} else {
		if(cmnd->num < 3) {
			player->print("Syntax: use <object> <exit>\n");
			return(0);
		}
	}

	//if(!canCast && spellData->how != CAST) {
	//	player->print("The magic will not work for you.\n");
	//	return(0);
	//}


	exit = findExit(player, cmnd, 2);

	if(!exit || !exit->flagIsSet(X_LOCKED)) {
		player->print("You don't see that locked exit here.\n");
		return(0);
	}

	const Monster* guard = player->getRoomParent()->getGuardingExit(exit, player->getAsConstPlayer());
	if(guard && !player->checkStaff("%M %s %s that.\n", guard, guard->flagIsSet(M_FACTION_NO_GUARD) ? "doesn't like you enough to let you" : "won't let you", spellData->how == CAST ? "cast" : "do"))
		return(0);


	if(exit->getLevel() > player->getLevel()) {
		player->print("You aren't powerful enough to magically unlock that exit.\n");
		return(0);
	}
	if(exit->flagIsSet(X_UNPICKABLE) || exit->flagIsSet(X_PORTAL)) {
		player->print("The spell fizzles.\n");
		return(0);
	}

	if(spellData->how == CAST) {
		mpNeeded = 10 + exit->getLevel();
		if(	pPlayer &&
			pPlayer->getClass() != MAGE &&
			pPlayer->getSecondClass() != MAGE &&
			pPlayer->getClass() != LICH &&
			!pPlayer->isStaff()
		)
			mpNeeded *=2;

		if(player->getClass() != LICH) {
			if(player->mp.getCur() < mpNeeded) {
				player->print("You need %d magic points to cast that spell.\n", mpNeeded);
				return(0);
			}
			player->mp.decrease(mpNeeded);
		} else {
			if(player->hp.getCur() < mpNeeded) {
				player->print("You need %d hit points to cast that spell.\n", mpNeeded);
				return(0);
			}
			player->hp.decrease(mpNeeded);
		}
	}

	if(player->getClass() == CLERIC && player->getDeity() == KAMIRA)
		chance = 10*((int)player->getLevel() - exit->getLevel()) + (2*bonus((int)player->piety.getCur()));
	else
		chance = 10*((int)player->getLevel() - exit->getLevel()) + (2*bonus((int)player->intelligence.getCur()));

	if(spellData->how == CAST)
		player->printColor("You cast a knock spell on the \"%s^x\" exit.\n", exit->name);
	broadcast(player->getSock(), player->getParent(), "%M casts a knock spell at the %s^x.", player, exit->name);


	if(player->isStaff() || mrand(1,100) <= chance) {
		player->print("Click!");
		if(spellData->how == CAST)
			player->print(" Your knock spell was successful!");
		player->print("\n");
		broadcast(player->getSock(), player->getParent(), "%M's knock spell was successful.", player);
		exit->clearFlag(X_LOCKED);
	} else {
		player->printColor("^yYour spell failed.\n");
		broadcast(player->getSock(), player->getParent(), "%M's knock spell failed.", player);
	}

	return(1);
}

//*********************************************************************
//						splDisintegrate
//*********************************************************************

int splDisintegrate(Creature* player, cmd* cmnd, SpellData* spellData) {
	Creature *target=0;
	int	saved=0, bns=0;

	if(spellData->how == CAST && !isMageLich(player)) {
		player->print("The arcane nature of that spell eludes you.\n");
		return(0);
	}

	if(player->getLevel() < 13) {
		player->print("You are not powerful enough to cast that spell.\n");
		return(0);
	}


	if(cmnd->num < 2) {
		if(spellData->how == CAST) {
			player->print("Casting a disintegrate spell on yourself would not be very smart.\n");
			return(0);
		} else if(spellData->how == WAND) {
			player->print("Nothing happens.\n");
			return(0);
		} else {
			if(!player->chkSave(DEA, player, 30+(50-player->getLevel()))) {
				player->printColor("^GYou are engulfed in an eerie green light.\n");
				player->printColor("You've been disintegrated!\n");
				//	die(player, DISINTEGRATED);
				return(0);
			} else {
				player->print("Ewww! That tastes HORRIBLE.\n");
				player->print("You vomit uncontrollabley.\n");
				player->stun(mrand(20,30));
				return(0);
			}
		}
	} else {
		if(noPotion(player, spellData))
			return(0);

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

		if(!target || player == target) {
			// disintegrate can destroy walls of force
			cmnd->str[2][0] = low(cmnd->str[2][0]);
			Exit* exit = findExit(player, cmnd, 2);
			EffectInfo* effect=0;

			if(exit) {
				effect = exit->getEffect("wall-of-force");
				
				if(	(effect && !effect->getExtra()) ||
					exit->flagIsSet(X_PORTAL)
				) {
					player->printColor("You cast a disintegration spell on the %s^x.\n", exit->name);
					broadcast(player->getSock(), player->getParent(), "%M casts a disintegration spell on the %s^x.", player, exit->name);

					if(exit->flagIsSet(X_PORTAL)) {
						broadcast(0, player->getRoomParent(), "^GAn eerie green light engulfs the %s^x!", exit->name);
						Move::deletePortal(player->getRoomParent(), exit);
					} else {
						broadcast(0, player->getRoomParent(), "^GAn eerie green light engulfs the wall of force blocking the %s^x!", exit->name);
						bringDownTheWall(effect, player->getRoomParent(), exit);
					}
					return(0);
				}
			}

			player->print("You don't see that here.\n");
			return(0);
		}

		if(!player->canAttack(target))
			return(0);

		player->printColor("^GYou cast a disintegration spell on %N.\n", target);
		target->printColor("^G%M casts a disintegration spell on you.\n", player);
		broadcast(player->getSock(), target->getSock(), player->getParent(), "^G%M casts a disintegration spell on %N.", player, target);

		bns = ((int)target->getLevel() - (int)player->getLevel())  * 25;

		if(target->mFlagIsSet(M_PERMENANT_MONSTER))
			bns = 50;

		if(target->isPlayer() && target->isEffected("resist-magic"))
			bns += MIN(30, target->saves[DEA].chance);

		if(spellData->how == CAST && player->isPlayer())
			player->getAsPlayer()->statistics.offensiveCast();

		saved = target->chkSave(DEA, player, bns);

		if(saved) {
		    player->doDamage(target, target->hp.getCur()/2);
			target->print("Negative energy rips through your body.\n");
			target->stun(mrand(10,15));
			player->print("%M avoided disintegration.\n", target);
		} else {
			target->printColor("^GYou are engulfed in an eerie green light.\n");
			target->printColor("You've been disintegrated!^x\n");
			broadcast(target->getSock(), player->getRoomParent(),
				"^G%M is engulfed in an eerie green light\n%M was disintegrated!", target, target);

			target->hp.setCur(1);
			target->mp.setCur(1);

			// goodbye inventory
			if(target->getAsMonster()) {
				ObjectSet::iterator it;
				Object* obj;
				for( it = target->objects.begin() ; it != target->objects.end() ; ) {
					obj = (*it++);
					delete obj;
				}
				target->objects.clear();
				target->coins.zero();
			}
			target->die(player);
		}

	}

	return(1);
}

//*********************************************************************
//						splStatChange
//*********************************************************************

int splStatChange(Creature* player, cmd* cmnd, SpellData* spellData, bstring effect, bool good) {
	Creature* target=0;
	if(	spellData->how == CAST &&
		player->getClass() != MAGE &&
		player->getClass() != LICH &&
		player->getClass() != DRUID &&
		player->getClass() != CLERIC &&
		!player->isCt()
	) {
		player->print( "You cannot cast that spell.\n");
		return(0);
	}

	// cast on self
	if(cmnd->num == 2) {
		target = player;

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			player->print("You cast %s on yourself.\n", effect.c_str());
			broadcast(player->getSock(), player->getParent(), "%M casts %s on %sself.", player, effect.c_str(), player->himHer());
		}
	// cast on another
	} else {
		if(noPotion(player, spellData))
			return(0);

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

		if(!target) {
			player->print( "That person is not here.\n");
			return(0);
		}

		if(checkRefusingMagic(player, target))
			return(0);

		player->print( "You cast %s on %N.\n", effect.c_str(), target);
		target->print("%M casts %s on you.\n", player, effect.c_str());
		broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts %s on %N.", player, effect.c_str(), target);

	}

	if(target->inCombat(false))
		player->smashInvis();

	if(!good && !player->isDm() && target != player && target->chkSave(SPL, player, 0)) {
		player->print( "%M avoided your spell.\n", target);
		return(1);
	}

	if(spellData->how == CAST) {
		if(player->getRoomParent()->magicBonus())
			player->print( "The room's magical properties increase the power of your spell.\n");
		target->addEffect(effect, -2, -2, player, true, player);
	} else {
		target->addEffect(effect);
	}

	return(1);
}

//*********************************************************************
//						stat changing spells
//*********************************************************************

int splHaste(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splStatChange(player, cmnd, spellData, "haste", true));
}
int splSlow(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splStatChange(player, cmnd, spellData, "slow", false));
}
int splStrength(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splStatChange(player, cmnd, spellData, "strength", true));
}
int splEnfeeblement(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splStatChange(player, cmnd, spellData, "enfeeblement", false));
}
int splInsight(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splStatChange(player, cmnd, spellData, "insight", true));
}
int splFeeblemind(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splStatChange(player, cmnd, spellData, "feeblemind", false));
}
int splPrayer(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splStatChange(player, cmnd, spellData, "prayer", true));
}
int splDamnation(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splStatChange(player, cmnd, spellData, "damnation", false));
}
int splFortitude(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splStatChange(player, cmnd, spellData, "fortitude", true));
}
int splWeakness(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splStatChange(player, cmnd, spellData, "weakness", false));
}

//*********************************************************************
//						splStoneToFlesh
//*********************************************************************

int splStoneToFlesh(Creature* player, cmd* cmnd, SpellData* spellData) {
	Creature* target=0;

	if(cmnd->num < 3) {
		player->print("On whom?\n");
		return(0);
	}

	if(	spellData->how == CAST &&
		player->getClass() != MAGE &&
		player->getClass() != LICH &&
		player->getClass() != DRUID &&
		player->getClass() != CLERIC &&
		player->getClass() != BARD &&
		player->getClass() != PALADIN &&
		player->getClass() != DEATHKNIGHT &&
		player->getClass() != RANGER &&
		!player->isCt()
	) {
		player->print("You are unable to cast that spell.\n");
		return(0);
	}

	if(noPotion(player, spellData))
		return(0);

	cmnd->str[2][0] = up(cmnd->str[2][0]);
	target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);
	if(!target) {
		player->print("That's not here.\n");
		return(0);
	}

	if(checkRefusingMagic(player, target))
		return(0);

	if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
		player->print("You cast stone-to-flesh on %N.\n", target);
		broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts stone-to-flesh on %N.", player, target);
		target->print("%M casts stone-to-flesh on you.\n", player);
		if(target->isEffected("petrification"))
			target->print("Your body returns to flesh.\n");
		else
			player->print("Nothing happens.\n");
	}

	target->removeEffect("petrification");
	return(1);
}

//*********************************************************************
//						splDeafness
//*********************************************************************

int splDeafness(Creature* player, cmd* cmnd, SpellData* spellData) {
	Creature* target=0;
	int		bns=0, canCast=0, mpCost=0;
	long	dur=0;

	if(player->getClass() == BUILDER) {
		player->print("You cannot cast this spell.\n");
		return(0);
	}

	if(	player->getClass() == LICH ||
		player->getClass() == MAGE ||
		player->getClass() == CLERIC ||
		player->isCt()
	)
		canCast = 1;

	if(!canCast && player->isPlayer() && spellData->how == CAST) {
		player->print("You are unable to cast that spell.\n");
		return(0);
	}

	player->smashInvis();

	if(spellData->how == CAST) {
		dur = mrand(180,300) + 3*bonus((int) player->intelligence.getCur());

		player->subMp(mpCost);
	} else if(spellData->how == SCROLL)
		dur = mrand(30,120) + bonus((int) player->intelligence.getCur());
	else
		dur = mrand(30,60);



	if(spell_fail(player, spellData->how))
		return(0);

	// silence on self
	if(cmnd->num == 2) {
		target = player;
		if(player->isEffected("resist-magic"))
			dur /= 2;

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			broadcast(player->getSock(), player->getParent(), "%M casts deafness on %sself.", player, player->himHer());
		} else if(spellData->how == POTION)
			player->print("Your throat goes dry and you cannot speak.\n");

	// silence a monster or player
	} else {
		if(noPotion(player, spellData))
			return(0);

		target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

		if(!target || target == player) {
			player->print("That's not here.\n");
			return(0);
		}

		if(!player->canAttack(target))
			return(0);

		if(player->isPlayer() && target->mFlagIsSet(M_PERMENANT_MONSTER)) {
			if(!dec_daily(&player->daily[DL_SILENCE]) && !player->isCt()) {
				player->print("You have done that enough times for today.\n");
				return(0);
			}
		}

		bns = ((int)target->getLevel() - (int)player->getLevel())*10;

		if(	(target->isPlayer() && target->isEffected("resist-magic")) ||
			(target->isMonster() && target->isEffected("resist-magic"))
		) {
			bns += 50;
			dur /= 2;
		}

		if(target->isPlayer() && player->isCt())
			dur = 600L;

		if(target->mFlagIsSet(M_PERMENANT_MONSTER))
			bns = (target->saves[SPL].chance)/3;

		target->wake("Terrible nightmares disturb your sleep!");

		if(target->chkSave(SPL, player, bns) && !player->isCt()) {
			target->print("%M tried to cast a deafness spell on you!\n", player);
			broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to cast a deafness spell on %N!", player, target);
			player->print("Your spell fizzles.\n");
			return(0);
		}


		if(player->isPlayer() && target->isPlayer()) {
			if(!dec_daily(&player->daily[DL_SILENCE]) && !player->isCt()) {
				player->print("You have done that enough times for today.\n");
				return(0);
			}
		}


		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			player->print("Deafness casted on %s.\n", target->name);
			broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a deafness spell on %N.", player, target);

			logCast(player, target, "silence");

			target->print("%M casts a deafness spell on you.\n", player);
		}

		if(target->isMonster())
			target->getAsMonster()->addEnemy(player);
	}

	target->addEffect("deafness", dur, player->getLevel());

	if(spellData->how == CAST && player->isPlayer())
		player->getAsPlayer()->statistics.offensiveCast();

	return(1);
}

//*********************************************************************
//						splRegeneration
//*********************************************************************

int splRegeneration(Creature* player, cmd* cmnd, SpellData* spellData) {
	if(spellData->how != POTION && player->isPlayer() && !player->isStaff()) {
		switch(player->getClass()) {
			case CLERIC:
			case DRUID:
				break;
			default:
				player->print("Only clerics and druids may cast this spell.\n");
				return(0);
		}
	}
	return(splGeneric(player, cmnd, spellData, "a", "regeneration", "regeneration", spellData->level));
}

//*********************************************************************
//						splWellOfMagic
//*********************************************************************

int splWellOfMagic(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "well-of-magic", "well-of-magic", spellData->level));
}
