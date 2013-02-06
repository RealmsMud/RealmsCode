/*
 * levelGain.cpp
 *	 Level Gain
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
#include "bank.h"
#include "web.h"


char statStr[][5] = {
	"NONE", "STR", "DEX", "CON", "INT", "PTY", "CHA"
};

char saveStr[][5] = {
	"LCK", "POI", "DEA", "BRE", "MEN", "SPL"
};

//*********************************************************************
//						findStat
//*********************************************************************

int findStat(bstring &stat) {
	for(int i = 0 ; i < 7 ; i++) {
		if(stat == statStr[i]) {
			return(i);
		}
	}
	return(-1);
}

//*********************************************************************
//						findSave
//*********************************************************************

int findSave(bstring &save) {
	for(int i = 0 ; i < 6 ; i++) {
		if(save == saveStr[i]) {
			return(i);
		}
	}
	return(-1);
}

//*********************************************************************
//						LevelGain
//*********************************************************************

LevelGain::LevelGain(xmlNodePtr rootNode) {
	load(rootNode);
}

LevelGain::~LevelGain() {
	std::list<SkillGain*>::iterator sgIt;
	SkillGain* sGain;

	for(sgIt = skills.begin() ; sgIt != skills.end() ; sgIt++) {
		sGain = (*sgIt);
		delete sGain;
	}
	skills.clear();
}

//*********************************************************************
//						load
//*********************************************************************

void LevelGain::load(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;
	bstring temp;
	while(curNode) {

		if(NODE_NAME(curNode, "HpGain")) {
			xml::copyToNum(hp, curNode);
		} else if(NODE_NAME(curNode, "MpGain")) {
			xml::copyToNum(mp, curNode);
		} else if(NODE_NAME(curNode, "Stat")) {
			xml::copyToBString(temp, curNode);
			stat = findStat(temp);
		} else if(NODE_NAME(curNode, "Save")) {
			xml::copyToBString(temp, curNode);
			save = findSave(temp);
		} else if(NODE_NAME(curNode, "Skills")) {
			xmlNodePtr skillNode = curNode->children;
			while(skillNode) {
				if(NODE_NAME(skillNode, "Skill")) {
					SkillGain* skillGain = new SkillGain(skillNode);
					skills.push_back(skillGain);
				}
				skillNode = skillNode->next;
			}

		}
		curNode = curNode->next;
	}
}

//*********************************************************************
//						hasSkills
//*********************************************************************

bool LevelGain::hasSkills() { return(!skills.empty()); }

//*********************************************************************
//						getSkillBegin
//*********************************************************************

std::list<SkillGain*>::const_iterator LevelGain::getSkillBegin() { return(skills.begin()); }

//*********************************************************************
//						getSkillEnd
//*********************************************************************

std::list<SkillGain*>::const_iterator LevelGain::getSkillEnd() { return(skills.end()); }

//*********************************************************************
//						getStatStr
//*********************************************************************

bstring LevelGain::getStatStr() { return(statStr[stat]); }

//*********************************************************************
//						getSaveStr
//*********************************************************************

bstring LevelGain::getSaveStr() { return(saveStr[save]); }

//*********************************************************************
//						getStat
//*********************************************************************

int LevelGain::getStat() { return(stat); }

//*********************************************************************
//						getSave
//*********************************************************************

int LevelGain::getSave() { return(save); }

//*********************************************************************
//						getHp
//*********************************************************************

int LevelGain::getHp() { return(hp); }

//*********************************************************************
//						getMp
//*********************************************************************

int LevelGain::getMp() { return(mp); }

//*********************************************************************
//						doTrain
//*********************************************************************

void doTrain(Player* player) {
	player->upLevel();
	player->setFlag(P_JUST_TRAINED);
	broadcast("### %s just made a level!", player->getCName());
	player->print("Congratulations, you made a level!\n\n");

	if(player->canChooseCustomTitle()) {
		player->setFlag(P_CAN_CHOOSE_CUSTOM_TITLE);
		if(!player->flagIsSet(P_CANNOT_CHOOSE_CUSTOM_TITLE))
			player->print("You may now choose a custom title. Type ""help title"" for more details.\n");
	}

	logn("log.train", "%s just trained to level %d in room %s.\n",
		player->getCName(), player->getLevel(), player->getRoomParent()->fullName().c_str());
	updateRecentActivity();
}

//*********************************************************************
//						upLevel
//*********************************************************************
// This function should be called whenever a player goes up a level.
// It raises there hit points and magic points appropriately, and if
// it is initializing a new character, it sets up the character.

void Player::upLevel() {
	int	a=0;
	bool relevel=false;

	if(isStaff()) {
		level++;
		return;
	}

	PlayerClass *pClass = gConfig->classes[getClassString()];
	const RaceData* rData = gConfig->getRace(race);
	LevelGain *lGain = 0;

	if(level == actual_level)
		actual_level++;
	else
		relevel = true;

	level++;

	// Check for level info
	if(!pClass) {
		print("Error: Can't find your class!\n");
		if(!isStaff()) {
			bstring errorStr = "Error: Can't find class: " + getClassString();
			merror(errorStr.c_str(), FATAL);
		}
		return;
	} else {
		print("Checking Leveling Information for [%s:%d].\n", pClass->getName().c_str(), level);
		lGain = pClass->getLevelGain(level);
		if(!lGain && level != 1) {
			print("Error: Can't find any information for your level!\n");
			if(!isStaff()) {
				bstring errorStr = "Error: Can't find level info for " + getClassString() + level;
				merror(errorStr.c_str(), FATAL);
			}
			return;
		}
	}

	if(!relevel) {
		// Only check weapon gains on a real level
		checkWeaponSkillGain();
	}

    if(cClass == FIGHTER && !cClass2 && flagIsSet(P_PTESTER)) {
        focus.setInitial(100);
        focus.addModifier("UnFocused", -100, MOD_CUR);
    }

	if(level == 1) {
		statistics.startLevelHistoryTracking();

		hp.setInitial(pClass->getBaseHp());
		if(cClass != BERSERKER && cClass != LICH)
			mp.setInitial(pClass->getBaseMp());

		hp.restore();
		mp.restore();
		damage = pClass->damage;

		if(!rData) {
			print("Error: Can't find your race, no saving throws for you!\n");
		} else {
			saves[POI].chance = rData->getSave(POI);
			saves[DEA].chance = rData->getSave(DEA);
			saves[BRE].chance = rData->getSave(BRE);
			saves[MEN].chance = rData->getSave(MEN);
			saves[SPL].chance = rData->getSave(SPL);
			setFlag(P_SAVING_THROWSPELL_LEARN);

			checkSkillsGain(rData->getSkillBegin(), rData->getSkillEnd());
		}
		// Base Skills

		if(!pClass) {
			print("Error: Can't find your class, no skills for you!\n");
		} else {
			checkSkillsGain(pClass->getSkillBegin(), pClass->getSkillEnd());
		}

	} else {
		bstring modName = bstring("Level") + level;

		// Calculate gains here
		int statGain = lGain->getStat();
        int saveGain = lGain->getSave();
        int hpAmt = lGain->getHp();
        int mpAmt = 0;
        if(cClass != BERSERKER && cClass != LICH) {
            mpAmt = lGain->getMp();
        }

        LevelInfo* levelInfo = statistics.getLevelInfo(level);

        // If we have a level info, it's a relevel so use the previous gains!
        if(levelInfo) {
            statGain = levelInfo->getStatUp();
            saveGain = levelInfo->getSaveGain();
            hpAmt = levelInfo->getHpGain();
            mpAmt = levelInfo->getMpGain();
        }



        // Add gains here
		StatModifier* newMod = new StatModifier(modName, 10, MOD_CUR_MAX);
		switch(statGain) {
		case STR:
			addStatModifier("strength", newMod);
			print("You have become stronger.\n");
			break;
		case DEX:
			addStatModifier("dexterity",newMod);
			print("You have become more dextrous.\n");
			break;
		case CON:
		    addStatModifier("constitution", newMod);
			print("You have become healthier.\n");
			break;
		case INT:
		    addStatModifier("intelligence", newMod);
			print("You have become more intelligent.\n");
			break;
		case PTY:
		    addStatModifier("piety", newMod);
			print("You have become more pious.\n");
			break;
		}


		addStatModifier("hp", modName, hpAmt, MOD_CUR_MAX );

        if(cClass != BERSERKER && cClass != LICH) {
            addStatModifier("mp", modName, mpAmt, MOD_CUR_MAX );
        }



		switch (saveGain) {
		case POI:
			saves[POI].chance += 3;
			print("You are now more resistant to poison.\n");
			break;
		case DEA:
			saves[DEA].chance += 3;
			print("You are now more able to resist traps and death.\n");
			break;
		case BRE:
			saves[BRE].chance += 3;
			print("You are now more resistant to breath weapons and explosions.\n");
			break;
		case MEN:
			saves[MEN].chance += 3;
			print("You are now more resistant to mind dominating attacks.\n");
			break;
		case SPL:
			saves[SPL].chance += 3;
			print("You are now more resistant to magical spells.\n");
			break;
		}

		if(!relevel) {
	        statistics.setLevelInfo(level, new LevelInfo(level, hpAmt, mpAmt, statGain, saveGain, time(0)));

		    // Saving throw bug fix: Spells and mental saving throws will now be
			// properly reset so they can increase like the other ones  -Bane
			for(a=POI; a<= SPL;a++)
				saves[a].gained = 0;
		}

		// Give out skills here
		if(lGain->hasSkills()) {
			print("Ok you should have some skills, seeing what they are.\n");
			checkSkillsGain(lGain->getSkillBegin(), lGain->getSkillEnd());
		} else {
			print("No skills for you at this level.\n");
		}

	}




	if(!negativeLevels) {
		hp.restore();
		mp.restore();
	}

	if(cClass == LICH && !relevel) {
		if(level == 7)
			print("Your body has deteriorated slightly.\n");
		if(level == 13)
			print("Your body has decayed immensely.\n");
		if(level == 19)
			print("What's left of your flesh hangs on your bones.\n");
		if(level == 25)
			print("You are now nothing but a dried up and brittle set of bones.\n");
	}


	if((cClass == MAGE || cClass2 == MAGE) && level == 7 && !relevel) {
		print("You have learned the armor spell.\n");
		learnSpell(S_ARMOR);
	}

	if(	cClass == CLERIC &&
		level >= 13 &&
		deity == CERIS &&
		!spellIsKnown(S_REJUVENATE)
	) {
		print("%s has granted you the rejuvinate spell.\n", gConfig->getDeity(deity)->getName().c_str());
		learnSpell(S_REJUVENATE);
	}


	if(	cClass == CLERIC &&
		level >= 19 &&
	    deity == CERIS &&
		!spellIsKnown(S_RESURRECT)
	) {
		print("%s has granted you the resurrect spell.\n", gConfig->getDeity(deity)->getName().c_str());
		learnSpell(S_RESURRECT);
	}

	if(	cClass == CLERIC &&
	    !cClass2 &&
		level >= 22 &&
	    deity == ARAMON &&
		!spellIsKnown(S_BLOODFUSION)
	) {
		print("%s has granted you the bloodfusion spell.\n", gConfig->getDeity(deity)->getName().c_str());
		learnSpell(S_BLOODFUSION);
	}

	updateGuild(this, GUILD_LEVEL);
	update();

	// getting [custom] as a title lets you pick a new one,
	// even if you have already picked one.
	if(!relevel) {

	}
	/*	if(cClass == BARD)
	pick_song(player);*/
}

//*********************************************************************
//						downLevel
//*********************************************************************
// This function is called when a player loses a level due to dying or
// for some other reason. The appropriate stats are downgraded.

void Player::downLevel() {

	if(isStaff()) {
		level--;
		return;
	}


	PlayerClass *pClass = gConfig->classes[getClassString()];
	LevelGain *lGain = 0;

	// Check for level info
	if(!pClass) {
		print("Error: Can't find your class!\n");
		if(!isStaff()) {
			return;
		}
		return;
	} else {
		lGain = pClass->getLevelGain(level);
		if(!lGain) {
			return;
		}
	}

	int saveLost = 0;
    LevelInfo* levelInfo = statistics.getLevelInfo(level);
    if(!levelInfo) {
        saveLost = lGain->getSave();
    } else {
        saveLost = levelInfo->getSaveGain();
    }

	bstring toRemove = bstring("Level") + level;

	hp.removeModifier(toRemove);
	mp.removeModifier(toRemove);

	if(strength.removeModifier(toRemove))
        *this << "You have lost strength.\n";
	if(dexterity.removeModifier(toRemove))
	    *this << "You have lost dexterity.\n";
	if(constitution.removeModifier(toRemove))
	    *this << "You have lost constitution.\n";
	if(intelligence.removeModifier(toRemove))
	    *this << "You have lost intelligence.\n";
	if(piety.removeModifier(toRemove))
	    *this << "You have lost piety.\n";

	level--;

	hp.restore();

	if(cClass != LICH)
		mp.restore();

	switch (saveLost) {
	case POI:
		saves[POI].chance -= 3;
		if(!negativeLevels)
			saves[POI].gained = 0;
		print("You are now less resistant to poison.\n");
		break;
	case DEA:
		saves[DEA].chance -= 3;
		if(!negativeLevels)
			saves[DEA].gained = 0;
		print("You are now less able to resist traps and death.\n");
		break;
	case BRE:
		saves[BRE].chance -= 3;
		if(!negativeLevels)
			saves[BRE].gained = 0;
		print("You are now less resistant to breath weapons and explosions.\n");
		break;
	case MEN:
		saves[MEN].chance -= 3;
		if(!negativeLevels)
			saves[MEN].gained = 0;
		print("You are now less resistant to mind dominating attacks.\n");
		break;
	case SPL:
		saves[SPL].chance -= 3;
		if(!negativeLevels)
			saves[SPL].gained = 0;
		print("You are now less resistant to magical spells.\n");
		break;
	}
	//	}

	updateGuild(this, GUILD_DIE);
	setMonkDice();

}

//*********************************************************************
//						cmdTrain
//*********************************************************************
// This function allows a player to train if they are in the correct
// training location and has enough gold and experience.  If so, the
// character goes up a level.

int cmdTrain(Player* player, cmd* cmnd) {
	unsigned long goldneeded=0, expneeded=0, bankneeded=0, maxgold=0;

	if(player->getClass() == BUILDER) {
		player->print("You don't need to do that!\n");
		return(0);
	}

	if(!player->ableToDoCommand())
		return(0);

	if(player->isBlind()) {
		player->printColor("^CYou can't do that! You're blind!\n");
		return(0);
	}
	if(!player->flagIsSet(P_SECURITY_CHECK_OK)) {
		player->print("You are not allowed to do that.\n");
		return(0);
	}

	if(!player->flagIsSet(P_PASSWORD_CURRENT)) {
		player->print("Your password is no longer current.\n");
		player->print("In order to train, you must change it.\n");
		player->print("Use the \"password\" command to do this.\n");
		return(0);
	}

	if(!player->flagIsSet(P_CHOSEN_ALIGNMENT) && player->getLevel() == ALIGNMENT_LEVEL) {
		player->print("You must choose your alignment before you can train to level %d.\n", ALIGNMENT_LEVEL+1);
		player->print("Use the 'alignment' command to do so. HELP ALIGNMENT.\n");
		return(0);
	}


	if(player->getNegativeLevels()) {
		player->print("To train, all of your life essence must be present.\n");
		return(0);
	}

	expneeded = gConfig->expNeeded(player->getLevel());

	if(player->getLevel() < 19)
		maxgold = 750000;
	else if(player->getLevel() < 22)
		maxgold = 2000000;
	else
		maxgold = ((player->getLevel()-22)*500000) + 3000000;

	goldneeded = MIN(maxgold, expneeded / 2L);

	if(player->getRace() == HUMAN)
		goldneeded += goldneeded/3/10; // Humans have +10% training costs.

	// Leveling cost temporarily suspended. -TC
	if(player->getLevel() <= 3)  // Free for levels 1-3 to train.
		goldneeded = 0;

	if(expneeded > player->getExperience()) {
		player->print("You need %s more experience.\n", player->expToLevel(false).c_str());
		return(0);
	}


	if((goldneeded > (player->coins[GOLD] + player->bank[GOLD])) && !player->flagIsSet(P_FREE_TRAIN)) {
		player->print("You don't have enough gold.\n");
		player->print("You need %ld gold to train.\n", goldneeded);
		return(0);
	}

	if(player->getRoomParent()->whatTraining() != player->getClass()) {
		player->print("This is not your training location.\n");
		return(0);
	}


	if(player->getLevel() >= MAXALVL) {
		player->print("You couldn't possibly train any more!\n");
		return(0);
	}

	// Prevent power leveling!
	if(player->flagIsSet(P_JUST_TRAINED)) {
		player->print("You just trained! You must leave the room and re-enter.\n");
		return(0);
	}


	if(!player->flagIsSet(P_FREE_TRAIN)) {
		if(goldneeded > player->coins[GOLD]) {
			bankneeded = goldneeded - player->coins[GOLD];
			player->coins.set(0, GOLD);
			player->bank.sub(bankneeded, GOLD);
			player->print("You use %ld gold coins from your bank account.\n", bankneeded);
			Bank::log(player->getCName(), "WITHDRAW (train) %ld [Balance: %ld]\n",
				bankneeded, player->bank[GOLD]);
		} else
			player->coins.sub(goldneeded, GOLD);
	}
	gServer->logGold(GOLD_OUT, player, Money(goldneeded, GOLD), NULL, "Training");

	doTrain(player);


	if(!player->flagIsSet(P_CHOSEN_ALIGNMENT) && player->getLevel() == ALIGNMENT_LEVEL) {
		player->print("You may now choose your alignment. You must do so before you can reach level %d.\n", ALIGNMENT_LEVEL+1);
		player->print("Use the 'alignment' command to do so. HELP ALIGNMENT.\n");
	}
	return(0);
}

//*********************************************************************
//						checkWeaponSkillGain
//*********************************************************************

void Player::checkWeaponSkillGain() {
	int numWeapons = 0;
	// Everyone gets a new weapon skill every title
	if(((level+2)%3) == 0)
		numWeapons ++;
	switch(cClass) {
		case FIGHTER:
			if(!cClass2) {
				if(level%4 == 0)
					numWeapons++;
			} else {
				// Mutli fighters get weapon skills like other fighting classes
				if(level%8 == 8)
					numWeapons++;
			}
			break;
		case BERSERKER:
			if(level%6)
				numWeapons++;
			break;
		case THIEF:
		case RANGER:
		case ROGUE:
		case BARD:
		case PALADIN:
		case DEATHKNIGHT:
		case ASSASSIN:
			if(level/8)
				numWeapons++;
			break;
		case CLERIC:
		case DRUID:
		case PUREBLOOD:
			if(cClass2) {
				// Cle/Ass
				if(level/8)
					numWeapons++;
			} else {
				if(level/12)
					numWeapons++;
			}
			break;
		case MAGE:
			if(cClass2) {
				if(level/12)
					numWeapons++;
			}
			break;
		default:
			break;

	}
	if(numWeapons != 0) {
		weaponTrains += numWeapons;
		print("You can now learn %d more weapon skill(s)!\n", numWeapons);
	}
}
