/*
 * abjuration.cpp
 *	 Abjuration spells
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
#include "math.h"
#include "effects.h"
#include "move.h"


//*********************************************************************
//						protection from room damage spells
//*********************************************************************

int splHeatProtection(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "heat-protection", "heat-protection"));
}
int splEarthShield(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "an", "earth-shield", "earth-shield"));
}
int splBreatheWater(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "breathe-water", "breathe-water"));
}
int splWarmth(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "warmth", "warmth", true));
}
int splWindProtection(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "wind-protection", "wind-protection"));
}
int splStaticField(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "static-field", "static-field"));
}


//*********************************************************************
//						splProtection
//*********************************************************************
// This function allows a spellcaster to cast a protection spell either
// on themself or on another player, improving the armor class by a
// score of 10.

int splProtection(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "protection", "protection"));
}


//*********************************************************************
//						splUndeadWard
//*********************************************************************

int splUndeadWard(Creature* player, cmd* cmnd, SpellData* spellData) {
	if(!player->isCt() && spellData->how == CAST) {
		if(player->getClass() != CLERIC && player->getClass() != PALADIN ) {
			player->print("Only clerics and paladins may cast that spell.\n");
			return(0);
		}
		if(player->getDeity() != ENOCH && player->getDeity() != LINOTHAN && player->getDeity() != CERIS) {
			player->print("%s will not allow you to cast that spell.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
			return(0);
		}
		if(!player->getDeity()) {
			// this should never happen
			player->print("You must belong to a religion to cast that spell.\n");
			return(0);
		}
	}

	return(splGeneric(player, cmnd, spellData, "an", "undead-ward", "undead-ward"));
}

//*********************************************************************
//						splBless
//*********************************************************************
// This function allows a player to cast a bless spell on themself or
// on another player, reducing the target's thaco by 1.

int splBless(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "bless", "bless"));
}

//*********************************************************************
//						splDrainShield
//*********************************************************************

int splDrainShield(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "drain-shield", "drain-shield"));
}

//*********************************************************************
//						reflect magic spells
//*********************************************************************
// The strength of the spell is the chance to reflect

int addReflectMagic(Creature* player, cmd* cmnd, SpellData* spellData, const char* article, const char* spell, int strength, unsigned int level) {
	if(spellData->how == CAST) {
		if(!isMageLich(player))
			return(0);
		if(spellData->level < level) {
			player->print("You are not powerful enough to cast this spell.\n");
			return(0);
		}
	}
	return(splGeneric(player, cmnd, spellData, article, spell, "reflect-magic", strength));
}

int splBounceMagic(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(addReflectMagic(player, cmnd, spellData, "a", "bounce magic", 33, 9));
}
int splReboundMagic(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(addReflectMagic(player, cmnd, spellData, "a", "rebound magic", 66, 16));
}
int splReflectMagic(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(addReflectMagic(player, cmnd, spellData, "a", "reflect-magic", 100, 21));
}

//*********************************************************************
//						splResistMagic
//*********************************************************************
// This function allows players to cast the resist-magic spell. It
// will allow the player to resist magical attacks from monsters

int splResistMagic(Creature* player, cmd* cmnd, SpellData* spellData) {
	int strength=0;

	if(spellData->how == CAST && !player->isStaff()) {
		if(	player->getClass() != MAGE &&
			player->getClass() != LICH
		) {
			player->print("Only mages and liches may cast this spell.\n");
			return(0);
		}
		if(spellData->level < 16) {
			player->print("You are not experienced enough to cast that spell.\n");
			return(0);
		}
	}

	if(spellData->how == CAST)
		strength = spellData->level;

	return(splGeneric(player, cmnd, spellData, "a", "resist-magic", "resist-magic", strength));
}


//*********************************************************************
//						splFreeAction
//*********************************************************************

int splFreeAction(Creature* player, cmd* cmnd, SpellData* spellData) {
	Creature* target=0;
	Player	*pTarget=0;
	long 	t = time(0);

	if(spellData->how != POTION && !player->isCt() && player->getClass() != DRUID && player->getClass() != CLERIC) {
		player->print("Your class is unable to cast that spell.\n");
		return(0);
	}


	if(cmnd->num == 2) {
		target = player;

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			player->print("Free-action spell cast.\nYou can now move freely.\n");
			broadcast(player->getSock(), player->getParent(), "%M casts a free-action spell on %sself.", player, player->himHer());
		} else if(spellData->how == POTION)
			player->print("You can now move freely.\n");

	} else {
		if(noPotion(player, spellData))
			return(0);

		cmnd->str[2][0] = up(cmnd->str[2][0]);

		target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

		if(!target || (target && target->isMonster())) {
			player->print("You don't see that person here.\n");
			return(0);
		}

		if(checkRefusingMagic(player, target))
			return(0);

		player->print("Free-action cast on %s.\n", target->getCName());
		target->print("%M casts a free-action spell on you.\n%s", player, "You can now move freely.\n");
		broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a free-action spell on %N.",player, target);
	}

	pTarget = target->getAsPlayer();

	if(pTarget) {
		pTarget->setFlag(P_FREE_ACTION);

		pTarget->lasttime[LT_FREE_ACTION].ltime = t;
		if(spellData->how == CAST) {
			pTarget->lasttime[LT_FREE_ACTION].interval = MAX(300, 900 +
				bonus((int) player->intelligence.getCur()) * 300);

			if(player->getRoomParent()->magicBonus()) {
				pTarget->print("The room's magical properties increase the power of your spell.\n");
				pTarget->lasttime[LT_FREE_ACTION].interval += 300L;
			}
		} else
			pTarget->lasttime[LT_FREE_ACTION].interval = 600;

		pTarget->clearFlag(P_STUNNED);
		pTarget->removeEffect("hold-person");
		pTarget->removeEffect("slow");

		pTarget->computeAC();
		pTarget->computeAttackPower();
	}
	return(1);
}

//*********************************************************************
//						splRemoveFear
//*********************************************************************

int splRemoveFear(Creature* player, cmd* cmnd, SpellData* spellData) {
	Creature* target=0;

	if(cmnd->num == 2) {
		target = player;

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {

			player->print("You cast remove-fear on yourself.\n");
			if(player->isEffected("fear"))
				player->print("You feel brave again.\n");
			else
				player->print("Nothing happens.\n");
			broadcast(player->getSock(), player->getParent(), "%M casts remove-fear on %sself.", player, player->himHer());
		} else if(spellData->how == POTION && player->isEffected("fear"))
			player->print("You feel brave again.\n");
		else if(spellData->how == POTION)
			player->print("Nothing happens.\n");

	} else {
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
			player->print("You cast remove-fear on %N.\n", target);
			broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts remove-fear on %N.", player, target);
			if(target->isPlayer()) {
				target->print("%M casts remove-fear on you.\n", player);
				if(target->isEffected("fear"))
					target->print("You feel brave again.\n");
				else
					player->print("Nothing happens.\n");
			}
		}
	}
	if(target->inCombat(false))
		player->smashInvis();

	target->removeEffect("fear", true, false);
	return(1);
}

//*********************************************************************
//						splRemoveSilence
//*********************************************************************

int splRemoveSilence(Creature* player, cmd* cmnd, SpellData* spellData) {
	Creature* target=0;

	if(cmnd->num == 2) {
		target = player;
		if(spellData->how == POTION && player->isEffected("silence") && !player->flagIsSet(P_DM_SILENCED))
			player->print("You can speak again.\n");
		else {
			player->print("Nothing happens.\n");
			return(0);
		}

	} else {
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
			player->print("You cast remove-silence on %N.\n", target);
			broadcast(player->getSock(), target->getSock(), player->getParent(),
				"%M casts remove-silence on %N.", player, target);

			logCast(player, target, "remove-silence");
			if(target->isPlayer()) {
				target->print("%M casts remove-silence on you.\n", player);
				if(	(target->flagIsSet(P_DM_SILENCED) && player->isCt()) ||
					 !target->flagIsSet(P_DM_SILENCED)
				)
					target->print("You can speak again.\n");
				else
					target->print("Nothing happens.\n");
			}
		}
	}
	if(target->inCombat(false))
		player->smashInvis();

	target->removeEffect("silence");

	if(player->isCt() && target->isPlayer())
		target->clearFlag(P_DM_SILENCED);

	return(1);
}

//*********************************************************************
//						splDispelAlign
//*********************************************************************

int splDispelAlign(Creature* player, cmd* cmnd, SpellData* spellData, const char* spell, int align) {
	Creature* target=0;
	Damage damage;


	if((target = player->findMagicVictim(cmnd->str[2], cmnd->val[2], spellData, true, false, "Cast on what?\n", "You don't see that here.\n")) == NULL)
		return(0);

	if(player == target) {
		// mobs can't cast on themselves!
		if(!player->getAsPlayer())
			return(0);

		// turn into a boolean!
		// are they in alignment?
		if(align == PINKISH)
			align = player->getAlignment() <= align;
		else
			align = player->getAlignment() >= align;

		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND)
			player->print("You cannot cast that spell on yourself.\n");
		else if(spellData->how == POTION && !align)
			player->print("Nothing happens.\n");
		else if(spellData->how == POTION && align) {
			player->smashInvis();

			damage.set(mrand(spellData->level * 2, spellData->level * 4));
			damage.add(MAX(0, bonus((int) player->piety.getCur())));
			player->modifyDamage(player, MAGICAL, damage);

			player->print("You feel as if your soul is savagely ripped apart!\n");
			player->printColor("You take %s%d^x points of damage!\n", player->customColorize("*CC:DAMAGE*").c_str(), damage.get());
			broadcast(player->getSock(), player->getParent(), "%M screams and doubles over in pain!", player);
			player->hp.decrease(damage.get());
			if(player->hp.getCur() < 1) {
				player->print("Your body explodes. You're dead!\n");
				broadcast(player->getSock(), player->getParent(), "%M's body explodes! %s's dead!", player,
						player->upHeShe());
				player->getAsPlayer()->die(EXPLODED);
			}
		}

	} else {
		if(spell_fail(player, spellData->how))
			return(0);

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

		if(!target) {
			player->print("That's not here.\n");
			return(0);
		}

		if(!player->canAttack(target))
			return(0);

		if(	(target->getLevel() > spellData->level + 7) ||
			(!player->isStaff() && target->isStaff())
		) {
			player->print("Your magic would be too weak to harm %N.\n", target);
			return(0);
		}

		player->smashInvis();
		// turn into a boolean!
		// are they in alignment?
		if(align == PINKISH)
			align = target->getAlignment() <= align;
		else
			align = target->getAlignment() >= align;



		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			player->print("You cast %s on %N.\n", spell, target);
			broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a %s spell on %N.", player, spell, target);

			if(target->getAsPlayer()) {
				target->wake("Terrible nightmares disturb your sleep!");
				target->print("%M casts a %s spell on you.\n", player, spell);
			}

			if(!align) {
				player->print("Nothing happens.\n");
			} else {
				damage.set(mrand(spellData->level * 2, spellData->level * 4));
				damage.add(abs(player->getAlignment()/100));
				damage.add(MAX(0, bonus((int) player->piety.getCur())));
				target->modifyDamage(player, MAGICAL, damage);

				if(target->chkSave(SPL, player,0))
					damage.set(damage.get() / 2);

				player->printColor("The spell did %s%d^x damage.\n", player->customColorize("*CC:DAMAGE*").c_str(), damage.get());
				player->print("%M screams in agony as %s soul is ripped apart!\n",
					target, target->hisHer());
				target->printColor("You take %s%d^x points of damage as your soul is ripped apart!\n", target->customColorize("*CC:DAMAGE*").c_str(), damage.get());
				broadcastGroup(false, target, "%M cast a %s spell on ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n",
					player, spell, target, damage.get(), target->heShe(), target->getStatusStr(damage.get()));

				broadcast(player->getSock(), target->getSock(), player->getParent(), "%M screams and doubles over in pain!", target);

				// if the target is reflecting magic, force printing a message (it will say 0 damage)
				player->doReflectionDamage(damage, target, target->isEffected("reflect-magic") ? REFLECTED_MAGIC : REFLECTED_NONE);

				if(spellData->how == CAST && player->isPlayer())
					player->getAsPlayer()->statistics.offensiveCast();

				player->doDamage(target, damage.get(), NO_CHECK);
				if(target->hp.getCur() < 1) {
					target->print("Your body explodes! You die!\n");
					broadcast(player->getSock(), target->getSock(), player->getParent(),
						"%M's body explodes! %s's dead!", target, target->upHeShe());
					player->print("%M's body explodes! %s's dead!\n", target, target->upHeShe());
					target->die(player);
					return(2);
				}
			}

		}
	}

	return(1);
}

//*********************************************************************
//						splDispelEvil
//*********************************************************************

int splDispelEvil(Creature* player, cmd* cmnd, SpellData* spellData) {

	if(spellData->how != POTION && !player->isStaff()) {
		if(player->getClass() != CLERIC && player->getClass() != PALADIN && player->isPlayer()) {
			player->print("Your class is unable to cast that spell.\n");
			return(0);
		}
		if(player->getClass() == CLERIC && player->getDeity() != ENOCH && player->getDeity() != LINOTHAN) {
			player->print("%s will not allow you to cast that spell.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
			return(0);
		}
		if(player->getAlignment() < LIGHTBLUE) {
			player->print("You have to be good to cast that spell.\n");
			return(0);
		}
	}

	return(splDispelAlign(player, cmnd, spellData, "dispel-evil", PINKISH));
}

//*********************************************************************
//						splDispelGood
//*********************************************************************

int splDispelGood(Creature* player, cmd* cmnd, SpellData* spellData) {

	if(spellData->how != POTION && !player->isStaff()) {
		if(player->getClass() != CLERIC && player->getClass() != DEATHKNIGHT && player->isPlayer()) {
			player->print("Your class is unable to cast that spell.\n");
			return(0);
		}
		if(player->getClass() == CLERIC && player->getDeity() != ARAMON && player->getDeity() != ARACHNUS) {
			player->print("Your deity will not allow you to cast that vile spell.\n");
			return(0);
		}
		if(player->getAlignment() > PINKISH) {
			player->print("You have to be evil to cast that spell.\n");
			return(0);
		}
	}

	return(splDispelAlign(player, cmnd, spellData, "dispel-good", LIGHTBLUE));
}

//*********************************************************************
//						splArmor
//*********************************************************************
// This spell allows a mage to cast an armor shield around themself which gives themself an
// AC of 6. It sets a pool of virtual HP to effect.strength which is equal to the caster's
// max HP. The armor can take that much damage before dispelling, or its time can run out.
// This spell does NOT absorb damage, it only protects by giving better AC. -TC

int splArmor(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player	*pPlayer = player->getAsPlayer();
	int	mpNeeded=0, multi=0;

	if(!pPlayer)
		return(0);

	mpNeeded = spellData->level;

	if(spellData->how == CAST && !pPlayer->checkMp(mpNeeded))
		return(0);

	if(!pPlayer->isCt()) {
		if(pPlayer->getClass() != MAGE && pPlayer->getSecondClass() != MAGE) {
			if(spellData->how == CAST) {
				player->print("The arcane nature of that spell eludes you.\n");
				return(0);
			} else {
				player->print("Nothing happens.\n");
				return(0);
			}
		}
	}


	if(	pPlayer->getSecondClass() == MAGE ||
		(pPlayer->getClass() == MAGE && pPlayer->getSecondClass() != MAGE)
	) {
		multi=1;
	}

	if(spell_fail(pPlayer, spellData->how)) {
		if(spellData->how == CAST)
			pPlayer->subMp(mpNeeded);
		return(0);
	}

	// Cast armor on self

	int strength = 0;
	int duration = 0;
	if(spellData->how == CAST) {
		duration = 1800 + bonus((int)pPlayer->intelligence.getCur());

		if(spellData->how == CAST)
			pPlayer->subMp(mpNeeded);
		if(pPlayer->getRoomParent()->magicBonus()) {
			player->print("The room's magical properties increase the power of your spell.\n");
			duration += 600L;
		}

		if(!multi)
			strength = pPlayer->hp.getMax();
		else
			strength = pPlayer->hp.getMax()/2;
	} else {
		duration = 300L;
		strength = pPlayer->hp.getMax()/2;
	}

	pPlayer->addEffect("armor", duration, strength, player, true, player);
	pPlayer->computeAC();

	if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
		player->print("Armor spell cast.\n");
		broadcast(pPlayer->getSock(), pPlayer->getRoomParent(),"%M casts an armor spell on %sself.", pPlayer, pPlayer->himHer());
	}

	return(1);
}

//*********************************************************************
//						splStoneskin
//*********************************************************************
// This spell allows a mage or lich to use magic to make their skin
// stone-hard to ward off physical attacks.

int splStoneskin(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player	*pPlayer = player->getAsPlayer();
	int		mpNeeded=0, multi=0;

	if(player->getClass() != LICH)
		mpNeeded = player->mp.getMax()/2;
	else
		mpNeeded = player->hp.getMax()/2;

	if(spellData->how == CAST && !player->checkMp(mpNeeded))
		return(0);

	if(!player->isCt()) {
		if(player->getClass() != LICH && player->getClass() != MAGE && (!pPlayer || pPlayer->getSecondClass() != MAGE)) {
			if(spellData->how == CAST) {
				player->print("The arcane nature of that spell eludes you.\n");
				return(0);
			} else {
				player->print("Nothing happens.\n");
				return(0);
			}
		}
		if(spellData->how == CAST && spellData->level < 13) {
			player->print("You are not powerful enough to cast this spell.\n");
			return(0);
		}
	}


	if(	pPlayer && (pPlayer->getSecondClass() == MAGE ||
		(pPlayer->getClass() == MAGE && pPlayer->getSecondClass()))
	) {
		multi=1;
	}

	if(spell_fail(player, spellData->how) && spellData->how != POTION) {
		if(spellData->how == CAST)
			player->subMp(mpNeeded);
		return(0);
	}

	int duration = 0;
	int strength = 0;
	// Cast stoneskin on self
	if(spellData->how == CAST) {
		duration = 240 + 10*bonus((int)player->intelligence.getCur());
		if(spellData->how == CAST)
			player->subMp(mpNeeded);
		if(player->getRoomParent()->magicBonus()) {
			player->print("The room's magical properties increase the power of your spell.\n");
			duration += 300L;
		}

		if(!multi)
			strength = spellData->level;
		else
			strength = MAX(1, spellData->level-3);
	} else {
		duration = 180L;
		strength = spellData->level/2;
	}

	player->addEffect("stoneskin", duration, strength, player, true, player);

	if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
		player->print("Stoneskin spell cast.\nYou feel impervious.\n");
		broadcast(player->getSock(), player->getParent(),"%M casts a stoneskin spell on %sself.", player,
			player->himHer());
	} else if(spellData->how == POTION)
		player->print("You feel impervious.\n");

	return(1);
}

//*********************************************************************
//						doDispelMagic
//*********************************************************************

int doDispelMagic(Creature* player, cmd* cmnd, SpellData* spellData, const char* spell, int numDispel) {
	Creature* target=0;
	int		chance=0;

	if(spellData->how == CAST &&
		player->getClass() != MAGE &&
	 	player->getClass() != LICH &&
		player->getClass() != CLERIC &&
		player->getClass() != DRUID &&
		!player->isStaff()
	) {
		player->print("Only mages, liches, clerics, and druids may cast that spell.\n");
		return(0);
	}

	if(cmnd->num == 2) {
		target = player;
		broadcast(player->getSock(), player->getParent(), "%M casts a %s spell on %sself.", player, spell, player->himHer());


		player->print("You cast a %s spell on yourself.\n", spell);
		player->print("Your spells begin to dissolve away.\n");


		player->doDispelMagic(numDispel);
	} else {
		if(noPotion(player, spellData))
			return(0);

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findPlayer(player, cmnd, 2);
		if(!target) {
			// dispel-magic on an exit
			cmnd->str[2][0] = low(cmnd->str[2][0]);
			Exit* exit = findExit(player, cmnd, 2);

			if(exit) {
				player->printColor("You cast a %s spell on the %s^x.\n", spell, exit->getCName());
				broadcast(player->getSock(), player->getParent(), "%M casts a %s spell on the %s^x.", player, spell, exit->getCName());

				if(exit->flagIsSet(X_PORTAL))
					Move::deletePortal(player->getRoomParent(), exit);
				else
					exit->doDispelMagic(player->getRoomParent());
				return(0);
			}

			player->print("You don't see that player here.\n");
			return(0);
		}

		if(player->isPlayer() && player->getRoomParent()->isPkSafe() && (!player->isCt()) && !target->flagIsSet(P_OUTLAW)) {
			player->print("That spell is not allowed here.\n");
			return(0);
		}

		if(!target->isEffected("petrification")) {
			if(!player->canAttack(target))
				return(0);
		} else {
			player->print("You cast a %s spell on %N.\n%s body returns to flesh.\n",
				spell, target, target->upHisHer());
			if(mrand(1,100) < 50) {

				target->print("%M casts a %s spell on you.\nYour body returns to flesh.\n", player, spell);
				broadcast(player->getSock(), target->getSock(), player->getParent(),
					"%M casts a %s spell on %N.\n%s body returns to flesh.\n",
					player, spell, target, target->upHisHer());
				target->removeEffect("petrification");
				return(1);
			} else {
				target->print("%M casts a dispel-magic spell on you.\n", player);
				broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a dispel-magic spell on %N.\n",player, target);
				return(0);
			}


		}


		chance = 50 - (10*(bonus((int) player->intelligence.getCur()) -
			bonus((int) target->intelligence.getCur())));

		chance += (spellData->level - target->getLevel())*10;

		chance = MIN(chance, 90);

		if((target->isEffected("resist-magic")) && target->isPlayer())
			chance /=2;




		if(	player->isCt() ||
			(mrand(1,100) <= chance && !target->chkSave(SPL, player, 0)))
		{
			player->print("You cast a %s spell on %N.\n", spell, target);

			logCast(player, target, spell);

			broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a %s spell on %N.", player, spell, target);
			target->print("%M casts a %s spell on you.\n", player, spell);
			target->print("Your spells begin to dissolve away.\n");


			target->doDispelMagic(numDispel);

			for(Monster* pet : target->pets) {
			    if(pet) {
	                if(player->isCt() || !pet->chkSave(SPL, player, 0)) {
	                    player->print("Your spell bansished %N's %s!\n", target, pet->getCName());
	                    target->print("%M's spell banished your %s!\n%M fades away.\n", player, pet->getCName(), pet);
	                    gServer->delActive(pet);
	                    pet->die(pet->getMaster());
	                }
			    }
			}
		} else {
			player->printColor("^yYour spell fails against %N.\n", target);
			target->print("%M tried to cast a dispel-magic spell on you.\n", player);
			broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to cast a dispel-magic spell on %N.", player, target);
			return(0);
		}
	}
	return(1);
}

//*********************************************************************
//						dispel-magic spells
//*********************************************************************

int splCancelMagic(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(doDispelMagic(player, cmnd, spellData, "cancel-magic", mrand(1,2)));
}

int splDispelMagic(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(doDispelMagic(player, cmnd, spellData, "dispel-magic", mrand(3,5)));
}

int splAnnulMagic(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(doDispelMagic(player, cmnd, spellData, "annul-magic", -1));
}

//********************************************************************
//						doDispelMagic
//********************************************************************
// num = -1 means dispel all effects

void Creature::doDispelMagic(int num) {
	EffectInfo* effect=0;
	std::list<bstring> effList;
	std::list<bstring>::const_iterator it;

	// create a list of possible effects
	effList.push_back("anchor");
	effList.push_back("hold-person");
	effList.push_back("strength");
	effList.push_back("haste");
	effList.push_back("fortitude");
	effList.push_back("insight");
	effList.push_back("prayer");
	effList.push_back("enfeeblement");
	effList.push_back("slow");
	effList.push_back("weakness");
	effList.push_back("prayer");
	effList.push_back("damnation");
	effList.push_back("confusion");

	effList.push_back("enlarge");
	effList.push_back("reduce");
	effList.push_back("darkness");
	effList.push_back("infravision");
	effList.push_back("invisibility");
	effList.push_back("detect-invisible");
	effList.push_back("undead-ward");
	effList.push_back("bless");
	effList.push_back("detect-magic");
	effList.push_back("protection");
	effList.push_back("tongues");
	effList.push_back("comprehend-languages");
	effList.push_back("true-sight");
	effList.push_back("fly");
	effList.push_back("levitate");
	effList.push_back("drain-shield");
	effList.push_back("resist-magic");
	effList.push_back("know-aura");
	effList.push_back("warmth");
	effList.push_back("breathe-water");
	effList.push_back("earth-shield");
	effList.push_back("reflect-magic");
	effList.push_back("camouflage");
	effList.push_back("heat-protection");
	effList.push_back("farsight");
	effList.push_back("pass-without-trace");
	effList.push_back("resist-earth");
	effList.push_back("resist-air");
	effList.push_back("resist-fire");
	effList.push_back("resist-water");
	effList.push_back("resist-cold");
	effList.push_back("resist-electricity");
	effList.push_back("wind-protection");
	effList.push_back("static-field");

	effList.push_back("illusion");
	effList.push_back("blur");
	effList.push_back("fire-shield");
	effList.push_back("greater-invisibility");

	// true we'll show, false don't remove perm effects

	// num = -1 means dispel all effects
	if(num <= -1) {
		for(it = effList.begin() ; it != effList.end() ; it++)
			removeEffect(*it, true, false);
		return;
	}

	int numEffects=0, choice=0;
	while(num > 0) {
		// how many effects are we under?
		for(it = effList.begin() ; it != effList.end() ; it++) {
			effect = getEffect(*it);
			if(effect && !effect->isPermanent())
				numEffects++;
		}

		// none? we're done
		if(!numEffects)
			return;
		// which effect shall we dispel?
		choice = mrand(1, numEffects);
		numEffects = 0;

		// find it and get rid of it!
		for(it = effList.begin() ; it != effList.end() && choice ; it++) {
			effect = getEffect(*it);
			if(effect && !effect->isPermanent()) {
				numEffects++;
				if(choice == numEffects) {
					removeEffect(*it, true, false);
					// stop the loop!
					choice = 0;
				}
			}
		}

		num--;
	}
}

//*********************************************************************
//						doDispelMagic
//*********************************************************************

void Exit::doDispelMagic(BaseRoom* parent) {
	bringDownTheWall(getEffect("wall-of-fire"), parent, this);
	bringDownTheWall(getEffect("wall-of-thorns"), parent, this);

	// true we'll show, false don't remove perm effects
	removeEffect("concealed", true, false);
}

//*********************************************************************
//						fire shield spells
//*********************************************************************
// This adds the actual fire-shield effect, include mage-lich restrict and
// strength of the spell. Strength and level restrict follow this chart:
//	1=1, 2=4, 3=9, 4=16

int addFireShield(Creature* player, cmd* cmnd, SpellData* spellData, const char* article, const char* spell, int strength) {
	if(spellData->how == CAST) {
		if(!isMageLich(player))
			return(0);
		if(spellData->level < pow(strength, 2)) {
			player->print("You are not powerful enough to cast this spell.\n");
			return(0);
		}
	}

	return(splGeneric(player, cmnd, spellData, article, spell, "fire-shield", (int)pow(strength, 2)));
}

int splRadiation(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(addFireShield(player, cmnd, spellData, "a", "radiation", 1));
}
int splFieryRetribution(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(addFireShield(player, cmnd, spellData, "a", "fiery retribution", 2));
}
int splAuraOfFlame(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(addFireShield(player, cmnd, spellData, "an", "aura of flame", 3));
}
int splBarrierOfCombustion(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(addFireShield(player, cmnd, spellData, "a", "barrier of combustion", 4));
}

//*********************************************************************
//						doReflectionDamage
//*********************************************************************
// return true if we were killed by the reflection
// passing printZero as REFLECTED_MAGIC or REFLECTED_PHYSICAL will print, even if damage is 0

bool Creature::doReflectionDamage(Damage damage, Creature* target, ReflectedDamageType printZero) {
	int dmg = damage.getReflected() ? damage.getReflected() : damage.getPhysicalReflected();

	// don't quit if we want to print 0
	if(!dmg && printZero == REFLECTED_NONE)
		return(false);

	if(damage.getReflected() || printZero == REFLECTED_MAGIC) {
		target->printColor("Your shield of magic reflects %s%d^x damage.\n", target->customColorize("*CC:DAMAGE*").c_str(), dmg);
		printColor("%M's shield of magic flares up and hits you for %s%d^x damage.\n", target, customColorize("*CC:DAMAGE*").c_str(), dmg);
		broadcastGroup(false, this, "%M's shield of magic flares up and hits %N for *CC:DAMAGE*%d^x damage.\n",
			target, this, dmg);
	} else {
		switch(damage.getPhysicalReflectedType()) {
		case REFLECTED_FIRE_SHIELD:
		default:
			target->printColor("Your shield of fire reflects %s%d^x damage.\n", target->customColorize("*CC:DAMAGE*").c_str(), dmg);
			printColor("%M's shield of fire flares up and hits you for %s%d^x damage.\n", target, customColorize("*CC:DAMAGE*").c_str(), dmg);
			broadcastGroup(false, this, "%M's shield of fire flares up and hits %N for *CC:DAMAGE*%d^x damage.\n",
				target, this, dmg);
			break;
		}
	}

	if(damage.getDoubleReflected()) {
		Damage reflectedDamage;
		reflectedDamage.setReflected(damage.getDoubleReflected());
		target->doReflectionDamage(reflectedDamage, this);
	}

	// we may have gotten this far for printing reasons (even on zero), but really, go no further
	if(!dmg)
		return(false);

	if(target->isPlayer() && isMonster())
		getAsMonster()->adjustThreat(target, dmg);

	hp.decrease(dmg);
	if(hp.getCur() <= 0)
		return(true);
	return(false);
}
