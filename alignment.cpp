/*
 * alignment.cpp
 *	 Alignment-related functions
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


//***********************************************************************
//						getAlignDiff
//***********************************************************************
// This function returns the distance on the alignment scale between
// any two different creatures. If either creature is neutral, or if
// both creatures are of the same moral character (i.e. both good or
// both evil) it will return 0. It is to be used for when opposing
// ethos has an effect on anything.

int getAlignDiff(Creature *crt1, Creature *crt2) {

	int		alignDiff = 0;

	if(	(crt1->getAdjustedAlignment() < NEUTRAL && crt2->getAdjustedAlignment() > NEUTRAL)  ||
		(crt2->getAdjustedAlignment() < NEUTRAL && crt1->getAdjustedAlignment() > NEUTRAL)
	) {
		alignDiff = abs(crt1->getAdjustedAlignment()) + abs(crt2->getAdjustedAlignment());
	}

	return(alignDiff);
}

//***********************************************************************
//						getAdjustedAlignment
//***********************************************************************

int Monster::getAdjustedAlignment() const {
	if(alignment <= -200)
		return(BLOODRED);
	else if(alignment > -200 && alignment < -50)
		return(REDDISH);
	else if(alignment >= -50 && alignment < 0)
		return(PINKISH);
	else if(alignment == 0)
		return(NEUTRAL);
	else if(alignment > 0 && alignment < 50)
		return(LIGHTBLUE);
	else if(alignment >= 50 && alignment < 200)
		return(BLUISH);
	else if(alignment >= 200)
		return(ROYALBLUE);
	return(NEUTRAL);
}

int Player::getAdjustedAlignment() const {
	if(alignment <= -400)
		return(BLOODRED);
	else if(alignment <= -200 && alignment > -400)
		return(REDDISH);
	else if(alignment <= -100 && alignment > -200)
		return(PINKISH);
	else if(alignment > -100 && alignment < 100)
		return(NEUTRAL);
	else if(alignment >= 100 && alignment < 200)
		return(LIGHTBLUE);
	else if(alignment >= 200 && alignment < 400)
		return(BLUISH);
	else if(alignment >= 400)
		return(ROYALBLUE);
	return(NEUTRAL);
}

//***********************************************************************
//						alignColor
//***********************************************************************

bstring Creature::alignColor() const {
	if(getAdjustedAlignment() < NEUTRAL)
		return("^r");
	else if(getAdjustedAlignment() > NEUTRAL)
		return("^c");
	else
		return("^x");
}

//********************************************************************
//						cmdChooseAlignment
//********************************************************************

int cmdChooseAlignment(Player* player, cmd* cmnd) {
	char syntax[] = "Syntax: alignment lawful\n"
					"        alignment chaotic\n"
					"Note: Tieflings must be chaotic.\n\n";

	if(player->isStaff() && !player->isCt())
		return(0);

	if(!player->ableToDoCommand())
		return(0);

	if(player->flagIsSet(P_CHOSEN_ALIGNMENT)) {
		player->print("You have already chosen your alignment.\n");
		if(player->flagIsSet(P_CHAOTIC))
			player->print("In order to convert to lawful, use the 'convert' command. HELP CONVERT.\n");
		return(0);
	} else if(player->getLevel() < ALIGNMENT_LEVEL) {
		player->print("You cannot choose your alignment until level %d.\n", ALIGNMENT_LEVEL);
		return(0);
	} else if(player->getLevel() > ALIGNMENT_LEVEL) {
		player->print("Your alignment has already been chosen.\n");
		return(0);
	}

	if(cmnd->num < 2) {
		player->print(syntax);
		return(0);
	}


	if(!strcasecmp(cmnd->str[1], "lawful")) {
		if(player->getRace() == TIEFLING) {
			player->print("Tieflings are required to be chaotic.\n\n");
		} else {
			broadcast("^B### %s chooses to adhere to the order of LAW!", player->getName());
			player->setFlag(P_CHOSEN_ALIGNMENT);
		}
	} else if(!strcasecmp(cmnd->str[1], "chaotic")) {
		broadcast("^R### %s chooses to embrace the whims of CHAOS!", player->getName());
		player->setFlag(P_CHAOTIC);
		player->setFlag(P_CHOSEN_ALIGNMENT);
	} else {
		player->print(syntax);
		return(0);
	}
	return(0);
}

//*********************************************************************
//						alignAdjustAcThaco
//*********************************************************************
// This function will be called whenever alignment changes,
// primarily in combat after killing a mob. It is initially
// designed to be only for monks and werewolves, but it can
// be applied to any class by altering the function. -TC

void Player::alignAdjustAcThaco() {
	if(cClass != MONK && cClass != WEREWOLF)
		return;

	computeAC();
	computeAttackPower();
}

//*********************************************************************
//						alignmentString
//*********************************************************************
// Get alignment string

char *alignmentString(Creature* player) {
	static char returnStr[1024];

	strcpy(returnStr, "");

	if(player->flagIsSet(P_OUTLAW))
		strcat(returnStr, "Outlawed");

	if(	(player->flagIsSet(P_NO_PKILL) ||
		player->flagIsSet(P_DIED_IN_DUEL) ||
		player->getRoomParent()->isPkSafe()) && (player->flagIsSet(P_CHAOTIC) ||
		player->getClan())
	)
		strcat(returnStr, "Neutral");
	else if(player->flagIsSet(P_CHAOTIC))
		strcat(returnStr, "Chaotic");
	else
		strcat(returnStr, "Lawful");

	return (returnStr);
}

//*********************************************************************
//						alignInOrder
//*********************************************************************
// basic alignment settings for races
// some extra checks may be need (ie, hands when royal blue)
// not set up for all clerics yet

bool Player::alignInOrder() const {
	switch(deity) {
	case GRADIUS:
		return(getAdjustedAlignment() == NEUTRAL || getAdjustedAlignment() == LIGHTBLUE);
	case LINOTHAN:
	case ENOCH:
	case KAMIRA:
		return(getAdjustedAlignment() >= BLUISH);
	case ARAMON:
	case ARACHNUS:
		return(getAdjustedAlignment() <= PINKISH);
	default:
		break;
	}
	return(true);
}

//*********************************************************************
//						isAntiGradius
//*********************************************************************

bool Creature::isAntiGradius() const {
	return(antiGradius(race));
}

//*********************************************************************
//						antiGradius
//*********************************************************************

bool antiGradius(int race) {
	return(	race == ORC ||
			race == OGRE ||
			race == GOBLIN ||
			race == TROLL ||
			race == KOBOLD );
}

//********************************************************************
//						adjustAlignment
//********************************************************************

void Player::adjustAlignment(Monster *victim) {
	int adjust = victim->getAlignment() / 8;

	if(victim->getAlignment() < 0 && victim->getAlignment() > -8)
		adjust = -1;
	if(victim->getAlignment() > 0 && victim->getAlignment() < 8)
		adjust = 1;

	bool toGood = adjust < 0;
	bool toEvil = adjust > 0;
	bool toNeutral = ((alignment > 0 && toEvil) ||
					  (alignment < 0 && toGood) );

	// handle all the move-away-from-neutral cases
	if(!toNeutral) {
		// if they're gradius, an anti-gradius race can only help their alignment.
		// same goes with the paly/dk war.
		if(deity == GRADIUS) {
			if(victim->isAntiGradius())
				adjust = 0;
		 	if(cClass == PALADIN && victim->getClass() == DEATHKNIGHT)
		 		adjust = 0;
		}
		// werewolves and vampires can always kill each other
		if(isEffected("vampirism") && victim->isEffected("lycanthropy"))
		  	adjust = 0;
		if(isEffected("lycanthropy") && victim->isEffected("vampirism"))
		  	adjust = 0;
		if(deity == CERIS && victim->isUndead())
			adjust /= 2;
	}

	// paladin / deathknight war
	// gradius paladins are taken care of above
	if(deity != GRADIUS) {
		if(cClass == PALADIN && victim->getClass() == DEATHKNIGHT && toEvil)
			adjust = 0;
		if(cClass == DEATHKNIGHT && victim->getClass() == PALADIN && toGood)
			adjust = 0;
	}

	alignment -= adjust;
	alignment = MAX(-1000, MIN(1000, alignment));
}

//*********************************************************************
//						cmdConvert
//*********************************************************************
// This function allows a player to convert from chaotic to lawful alignment.

int cmdConvert(Player* player, cmd* cmnd) {
	if(!player->ableToDoCommand())
		return(0);

	if(!player->flagIsSet(P_CHAOTIC)) {
		player->print("You cannot convert. You're lawful.\n");
		return(0);
	} else if(player->getRace() == TIEFLING) {
		player->print("Tieflings are required to be chaotic.\n");
		return(0);
	} else if(player->isNewVampire()) {
		player->print("Vampires are required to be chaotic.\n");
		return(0);
	}

	if(cmnd->num < 2 || strcasecmp(cmnd->str[1], "yes")) {
		player->print("To prevent accidental conversion you must confirm you want to convert,\n");
		player->print("to do so type 'convert yes'\n");
		player->print("Remember, you will NEVER be able to be chaotic again.\n");
		return(0);
	}

	if(player->getClass() == BUILDER) {
		broadcast(isStaff, "^G### %s just converted to lawful alignment.", player->getName());
		logn("log.convert","%s converted to lawful.", player->getName());
	} else
		broadcast("^G### %s just converted to lawful alignment.", player->getName());
	player->clearFlag(P_CHAOTIC);

	if(player->getClass() == CLERIC)
		player->clearFlag(P_PLEDGED);

	return(0);
}
