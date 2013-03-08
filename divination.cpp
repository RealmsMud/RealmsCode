/*
 * divination.cpp
 *	 Divination spells
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
#include "move.h"


//*********************************************************************
//						splTrueSight
//*********************************************************************
// This function allows players to detect misted vampires.

int splTrueSight(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "true-sight", "true-sight"));
}

//*********************************************************************
//						splFarsight
//*********************************************************************
// allows target to see further in the overland map

int splFarsight(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "farsight", "farsight"));
}

//*********************************************************************
//						splDetectMagic
//*********************************************************************
// This function allows players to cast the detect-magic spell which
// allows the spell-castee to see magic items.

int splDetectMagic(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "detect-magic", "detect-magic"));
}

//*********************************************************************
//						splDetectInvisibility
//*********************************************************************
// This function allows players to cast the detect-invisibility spell which
// allows the target to see invisible items, monsters, and exits.

int splDetectInvisibility(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "detect-invisibility", "detect-invisible"));
}

//*********************************************************************
//						splKnowAura
//*********************************************************************
// This spell allows the caster to determine what alignment another
// target or player is by looking at it.

int splKnowAura(Creature* player, cmd* cmnd, SpellData* spellData) {
	return(splGeneric(player, cmnd, spellData, "a", "know-aura", "know-aura"));
}

//*********************************************************************
//						splFortune
//*********************************************************************
// This allows bards to tell the luck of a given player.

int splFortune(Creature* player, cmd* cmnd, SpellData* spellData) {
	Creature* target=0;
	int		luk;

	Player	*pPlayer = player->getAsPlayer();

	if(player->getClass() != BARD && !player->isCt()) {
		player->print("Only bards may cast that spell.\n");
		return(0);
	}

	if(cmnd->num == 2) {


		if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
			player->print("Fortune spell cast on yourself.\n");
			luk = pPlayer->getLuck() / 10;
			luk = MAX(luk, 1);
			switch(luk) {
			case 1:
				player->print("Your death will be tragic.\n.");
				break;
			case 2:
				player->print("A black cat must have crossed your path.\n");
				break;
			case 3:
				player->print("If it weren't for bad luck you'd have no luck at all.\n");
				break;
			case 4:
				player->print("Your karma is imbalanced.\n");
				break;
			case 5:
				player->print("Your future is uncertain.\n");
				break;
			case 6:
				player->print("Without intervention you may find yourself in a dire situation.\n");
				break;
			case 7:
				player->print("Long range prospects look good.\n");
				break;
			case 8:
				player->print("Count your blessings, for others are less fortunate.\n");
				break;
			case 9:
				player->print("The fates have smiled upon you.\n");
				break;
			case 10:
				player->print("Your death would be tragic.\n");
				break;
			default:
				player->print("You can't tell right now.\n");
			}

			broadcast(player->getSock(), player->getParent(), "%M reads %s fortune.", player, player->hisHer());
		} else if(spellData->how == POTION)
			player->print("Nothing happens.\n");


	} else {
		if(player->noPotion( spellData))
			return(0);

		cmnd->str[2][0] = up(cmnd->str[2][0]);
		target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);


		if(!target) {
			player->print("That's not here.\n");
			return(0);
		}

		if(target->isMonster())
			luk = target->getAlignment() / 10;
		else
			luk = target->getAsPlayer()->getLuck() / 10;

		luk = MAX(luk, 1);
		player->print("Fortune spell cast on %N.\n", target);

		switch (luk) {
		case 1:
			player->print("%M's death will be swift and certain.\n", target);
			break;
		case 2:
			player->print("You sense %N's karma is imbalanced.\n", target);
			break;
		case 3:
			player->print("A black cat must have crossed %N's path.\n", target);
			break;
		case 4:
			player->print("%M's aura reeks of danger.\n", target);
			break;
		case 5:
			player->print("Without intervention %N may end up in a dire situation.\n", target);
			break;
		case 6:
			player->print("%M's future is uncertain.\n", target);
			break;
		case 7:
			player->print("Long range prospects look good for %N.\n", target);
			break;
		case 8:
			player->print("%M should count their blessings, as others are less fortunate.\n", target);
			break;
		case 9:
			player->print("The fates smile upon %N.\n", target);
			break;
		case 10:
			player->print("%M's death will be tragic and unexpected.\n", target);
			break;
		default:
			player->print("You can't tell right now.\n");
		}
		broadcast(player->getSock(), player->getParent(), "%M checks %N's fortune.", player, target);

	}
	return(1);
}

//*********************************************************************
//						splClairvoyance
//*********************************************************************

int splClairvoyance(Creature* player, cmd* cmnd, SpellData* spellData) {
	Player	*pPlayer = player->getAsPlayer();
	if(!pPlayer)
		return(0);

	Player	*target=0;
	int		chance=0;

	if(pPlayer->getClass() == BUILDER) {
		pPlayer->print("You cannot cast this spell.\n");
		return(0);
	}

	if(cmnd->num < 3) {
		pPlayer->print("Cast clairvoyance on whom?\n");
		return(0);
	}

	lowercize(cmnd->str[2], 1);
	target = gServer->findPlayer(cmnd->str[2]);

	if(!target || pPlayer == target || !pPlayer->canSee(target)) {
		pPlayer->print("That player is not logged on.\n");
		return(0);
	}

	if(Move::tooFarAway(pPlayer, target, "clair"))
		return(0);

	broadcast(pPlayer->getSock(), pPlayer->getRoomParent(), "%M casts clairvoyance.", pPlayer);
	if(spellData->how == CAST)
		pPlayer->print("You attempt to focus on %N.\n", target);

	chance = 50 + (spellData->level - target->getLevel()) * 5 +
			(bonus((int) pPlayer->intelligence.getCur()) - bonus((int) target->intelligence.getCur())) * 5;

	chance += (pPlayer->getClass() == MAGE) ? 5 : 0;
	chance = MIN(85, chance);

	if(target->isStaff())
		chance = 0;
	if( target->getRoomParent()->flagIsSet(R_NO_CLAIR_ROOM) ||
		target->getRoomParent()->flagIsSet(R_LIMBO) ||
		target->getRoomParent()->flagIsSet(R_VAMPIRE_COVEN) ||
		target->getRoomParent()->isConstruction() ||
		target->getRoomParent()->flagIsSet(R_SHOP_STORAGE)
	)
		chance = 0;

	if(pPlayer->isStaff() || mrand(1, 100) < chance) {

		chance += (target->getClass() == MAGE) ? 5 : 0;
		chance = MIN(85, chance);

		chance = 60 + ((int)target->getLevel() - (int)spellData->level) * 5 +
			(bonus((int) target->intelligence.getCur()) - bonus((int) pPlayer->intelligence.getCur())) * 5;
		chance += (target->getClass() == MAGE) ? 5 : 0;
		chance = MIN(85, chance);

		if(!pPlayer->isStaff()) {
			if(!target->chkSave(MEN, pPlayer, 0)) {
				display_rom(target, pPlayer);
				if(mrand(1, 100) < chance) {
					// display a different string if the target can't see
					if(target->flagIsSet(P_SLEEPING) || target->isBlind()) {
						target->print("%M temporarily sees your surroundings.\n", pPlayer);
					} else {
						target->print("%M temporarily sees through your eyes.\n", pPlayer);
					}

				}
			} else {
				pPlayer->print("You failed to locate %N.\n", target);
				if(mrand(1, 100) < chance)
					target->print("%M tried to connect to your mind.\n", pPlayer);
			}
		} else {
			display_rom(target, pPlayer);
		}

	} else {
		pPlayer->print("Your mind is unable to connect.\n");

		chance = 65 + ((int)target->getLevel() - (int)spellData->level) * 5 +
				(bonus((int) target->intelligence.getCur()) - bonus((int) pPlayer->intelligence.getCur())) * 5;

		if(!pPlayer->isStaff() && mrand(1, 100) < chance)
			target->print("%M attempts to connect to your mind.\n", pPlayer);
	}

	return(1);
}

//*********************************************************************
//						splComprehendLanguages
//*********************************************************************

int splComprehendLanguages(Creature* player, cmd* cmnd, SpellData* spellData) {
	if(spellData->how == CAST && player->getClass() != MAGE && player->getClass() != BARD && !player->isStaff()) {
		player->print("Only mages and bards may cast that spell.\n");
		return(0);
	}

	return(splGeneric(player, cmnd, spellData, "a", "comprehend-languages", "comprehend-languages"));
}

//*********************************************************************
//						splTongues
//*********************************************************************

int splTongues(Creature* player, cmd* cmnd, SpellData* spellData) {
	if(spellData->how == CAST && player->getClass() != MAGE && player->getClass() != BARD && !player->isStaff()) {
		player->print("Only mages and bards may cast that spell.\n");
		return(0);
	}

	return(splGeneric(player, cmnd, spellData, "a", "tongues", "tongues"));
}

//*********************************************************************
//						splDetectHidden
//*********************************************************************

int splDetectHidden(Creature* player, cmd* cmnd, SpellData* spellData) {
	if(!player->isPlayer())
		return(0);

	if(spellData->how == CAST) {
		if(!player->isMageLich())
			return(0);
		player->print("You cast a detect-hidden spell.\n");
		broadcast(player->getSock(), player->getParent(), "%M casts a detect-hidden spell.", player);
	}

	Player* viewer = player->getAsPlayer();
	display_rom(viewer, viewer, spellData->level);
	return(1);
}
