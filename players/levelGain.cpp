/*
 * levelGain.cpp
 *   Level Gain
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <fmt/format.h>                             // for format
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <ctime>                                    // for time
#include <list>                                     // for list, list<>::con...
#include <map>                                      // for map
#include <string>                                   // for string, allocator

#include "bank.hpp"                                 // for Bank
#include "config.hpp"                               // for Config, gConfig
#include "creatureStreams.hpp"                      // for Streamable
#include "deityData.hpp"                            // for DeityData
#include "dice.hpp"                                 // for Dice
#include "flags.hpp"                                // for P_CHOSEN_ALIGNMENT
#include "global.hpp"                               // for CreatureClass, POI
#include "levelGain.hpp"                            // for LevelGain
#include "magic.hpp"                                // for S_BLOODFUSION
#include "money.hpp"                                // for GOLD, Money
#include "mud.hpp"                                  // for ALIGNMENT_LEVEL
#include "mudObjects/players.hpp"                   // for Player
#include "mudObjects/rooms.hpp"                     // for BaseRoom
#include "playerClass.hpp"                          // for PlayerClass
#include "proto.hpp"                                // for updateGuild, broa...
#include "raceData.hpp"                             // for RaceData
#include "server.hpp"                               // for GOLD_OUT, Server
#include "skillGain.hpp"                            // for SkillGain
#include "statistics.hpp"                           // for LevelInfo, Statis...
#include "stats.hpp"                                // for Stat, MOD_CUR_MAX
#include "structs.hpp"                              // for saves
#include "web.hpp"                                  // for updateRecentActivity
#include "xml.hpp"                                  // for NODE_NAME, copyToNum

class cmd;

char statStr[][5] = {
    "NONE", "STR", "DEX", "CON", "INT", "PTY", "CHA"
};

char saveStr[][5] = {
    "LCK", "POI", "DEA", "BRE", "MEN", "SPL"
};

//*********************************************************************
//                      findStat
//*********************************************************************

int findStat(std::string &stat) {
    for(int i = 0 ; i < 7 ; i++) {
        if(stat == statStr[i]) {
            return(i);
        }
    }
    return(-1);
}

//*********************************************************************
//                      findSave
//*********************************************************************

int findSave(std::string &save) {
    for(int i = 0 ; i < 6 ; i++) {
        if(save == saveStr[i]) {
            return(i);
        }
    }
    return(-1);
}

//*********************************************************************
//                      LevelGain
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
//                      load
//*********************************************************************

void LevelGain::load(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    std::string temp;
    while(curNode) {

        if(NODE_NAME(curNode, "HpGain")) {
            xml::copyToNum(hp, curNode);
        } else if(NODE_NAME(curNode, "MpGain")) {
            xml::copyToNum(mp, curNode);
        } else if(NODE_NAME(curNode, "Stat")) {
            xml::copyToString(temp, curNode);
            stat = findStat(temp);
        } else if(NODE_NAME(curNode, "Save")) {
            xml::copyToString(temp, curNode);
            save = findSave(temp);
        } else if(NODE_NAME(curNode, "Skills")) {
            xmlNodePtr skillNode = curNode->children;
            while(skillNode) {
                if(NODE_NAME(skillNode, "Skill")) {
                    auto* skillGain = new SkillGain(skillNode);
                    skills.push_back(skillGain);
                }
                skillNode = skillNode->next;
            }

        }
        curNode = curNode->next;
    }
}

bool LevelGain::hasSkills() { return(!skills.empty()); }
std::list<SkillGain*>::const_iterator LevelGain::getSkillBegin() { return(skills.begin()); }
std::list<SkillGain*>::const_iterator LevelGain::getSkillEnd() { return(skills.end()); }
std::string LevelGain::getStatStr() { return(statStr[stat]); }
std::string LevelGain::getSaveStr() { return(saveStr[save]); }
int LevelGain::getStat() { return(stat); }
int LevelGain::getSave() { return(save); }
int LevelGain::getHp() { return(hp); }
int LevelGain::getMp() { return(mp); }

//*********************************************************************
//                      doTrain
//*********************************************************************

void doTrain(std::shared_ptr<Player> player) {
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
//                      upLevel
//*********************************************************************
// This function should be called whenever a player goes up a level.
// It raises there hit points and magic points appropriately, and if
// it is initializing a new character, it sets up the character.

void Player::upLevel() {
    int a=0;
    bool relevel=false;

    if(isStaff()) {
        level++;
        return;
    }

    PlayerClass *pClass = gConfig->classes[getClassString()];
    const RaceData* rData = gConfig->getRace(race);
    LevelGain *lGain = nullptr;

    if(level == actual_level)
        actual_level++;
    else
        relevel = true;

    level++;

    // Check for level info
    if(!pClass) {
        print("Error: Can't find your class!\n");
        if(!isStaff()) {
            std::string errorStr = "Error: Can't find class: " + getClassString();
            throw std::runtime_error(errorStr);
        }
        return;
    } else {
        print("Checking Leveling Information for [%s:%d].\n", pClass->getName().c_str(), level);
        lGain = pClass->getLevelGain(level);
        if(!lGain && level != 1) {
            print("Error: Can't find any information for your level!\n");
            if(!isStaff()) {
                std::string errorStr = fmt::format("Error: Can't find level info for {} {}", getClassString(), level);
                throw std::runtime_error(errorStr);
            }
            return;
        }
    }

    if(!relevel) {
        // Only check weapon gains on a real level
        checkWeaponSkillGain();
    }

    if(cClass == CreatureClass::FIGHTER && !hasSecondClass() && flagIsSet(P_PTESTER)) {
        focus.setInitial(100);
        focus.addModifier("UnFocused", -100, MOD_CUR);
    }

    if(level == 1) {
        statistics.startLevelHistoryTracking();

        hp.setInitial(pClass->getBaseHp());
        if(cClass != CreatureClass::BERSERKER && cClass != CreatureClass::LICH)
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
        std::string modName = fmt::format("Level{}", level);

        // Calculate gains here
        int statGain = lGain->getStat();
        int saveGain = lGain->getSave();
        int hpAmt = lGain->getHp();
        int mpAmt = 0;
        if(cClass != CreatureClass::BERSERKER && cClass != CreatureClass::LICH) {
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
        auto* newMod = new StatModifier(modName, 10, MOD_CUR_MAX);
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

        if(cClass != CreatureClass::BERSERKER && cClass != CreatureClass::LICH) {
            addStatModifier("mp", modName, mpAmt, MOD_CUR_MAX );
        }

        
             

        switch (saveGain) {
        case POI:
            logn("log.saveswtf", "upLevel():%s %s[L%d]'s POI save just went up. Prev: %d%, New: %d%, Gains: %d\n", 
                                relevel ? "[RE-LEVEL]":"", getCName(), level, saves[POI].chance, saves[POI].chance+3, relevel ? saves[POI].gained:0);
            saves[POI].chance += 3;
            printColor("^GYou are now more resistant to poison.\n");
            break;
        case DEA:
            logn("log.saveswtf", "upLevel():%s %s[L%d]'s DEA save just went up. Prev: %d%, New: %d%, Gains: %d\n", 
                                relevel ? "[RE-LEVEL]":"", getCName(), level, saves[DEA].chance, saves[DEA].chance+3, relevel ? saves[DEA].gained:0);
            saves[DEA].chance += 3;
            printColor("^DYou are now more able to resist traps and death.\n");
            break;
        case BRE:
             logn("log.saveswtf", "upLevel():%s %s[L%d]'s BRE save just went up. Prev: %d%, New: %d%, Gains: %d\n", 
                                relevel ? "[RE-LEVEL]":"", getCName(), level, saves[BRE].chance, saves[BRE].chance+3, relevel ? saves[BRE].gained:0);
            saves[BRE].chance += 3;
            printColor("^RYou are now more resistant to breath weapons and explosions.\n");
            break;
        case MEN:
            logn("log.saveswtf", "upLevel():%s %s[L%d]'s MEN save just went up. Prev: %d%, New: %d%, Gains: %d\n", 
                                relevel ? "[RE-LEVEL]":"", getCName(), level, saves[MEN].chance, saves[MEN].chance+3, relevel ? saves[MEN].gained:0);
            saves[MEN].chance += 3;
            print("^YYou are now more resistant to mind dominating attacks.\n");
            break;
        case SPL:
            logn("log.saveswtf", "upLevel():%s %s[L%d]'s SPL save just went up. Prev: %d%, New: %d%, Gains: %d\n", 
                                relevel ? "[RE-LEVEL]":"", getCName(), level, saves[SPL].chance, saves[SPL].chance+3, relevel ? saves[SPL].gained:0);
            saves[SPL].chance += 3;
            print("^MYou are now more resistant to magical spells.\n");
            break;
        }

        if(!relevel) {
            statistics.setLevelInfo(level, new LevelInfo(level, hpAmt, mpAmt, statGain, saveGain, time(nullptr)));

            // Saving throw bug fix: Spells and mental saving throws will now be
            // properly reset so they can increase like the other ones  -Bane
            for(a=POI; a<= SPL;a++) 
                saves[a].gained = 0;

            logn("log.saveswtf", "upLevel(): %s[%d] - all saves gained plyReset to 0\n", getCName(), level);
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

    if(cClass == CreatureClass::LICH && !relevel) {
        if(level == 10)
            print("Your body has deteriorated slightly.\n");
        if(level == 20)
            print("Your body has decayed immensely.\n");
        if(level == 30)
            print("What's left of your flesh hangs on your bones.\n");
        if(level == 40)
            print("You are now nothing but a dried up and brittle set of bones.\n");
    }


    if((cClass == CreatureClass::MAGE || cClass2 == CreatureClass::MAGE) && level == 7 && !relevel && !spellIsKnown(S_ARMOR)) {
        print("You have learned the armor spell.\n");
        learnSpell(S_ARMOR);
    }

    if( cClass == CreatureClass::CLERIC &&
        level >= 13 &&
        deity == CERIS &&
        !spellIsKnown(S_REJUVENATE)
    ) {
        print("%s has granted you the rejuvinate spell.\n", gConfig->getDeity(deity)->getName().c_str());
        learnSpell(S_REJUVENATE);
    }


    if( cClass == CreatureClass::CLERIC &&
        level >= 19 &&
        deity == CERIS &&
        !spellIsKnown(S_RESURRECT)
    ) {
        print("%s has granted you the resurrect spell.\n", gConfig->getDeity(deity)->getName().c_str());
        learnSpell(S_RESURRECT);
    }

    if( cClass == CreatureClass::CLERIC &&
        !hasSecondClass() &&
        level >= 22 &&
        deity == ARAMON &&
        !spellIsKnown(S_BLOODFUSION)
    ) {
        print("%s has granted you the bloodfusion spell.\n", gConfig->getDeity(deity)->getName().c_str());
        learnSpell(S_BLOODFUSION);
    }

    if( (cClass == CreatureClass::RANGER || cClass == CreatureClass::DRUID || (cClass == CreatureClass::CLERIC && getDeity() == MARA)) &&
        level >= 10 &&
        !spellIsKnown(S_TRACK)
    ) {
        print("You have learned the track spell.\n");
        learnSpell(S_TRACK);
    }

    updateGuild(Containable::downcasted_shared_from_this<Player>(), GUILD_LEVEL);
    update();

    // getting [custom] as a title lets you pick a new one,
    // even if you have already picked one.
    if(!relevel) {

    }
    /*  if(cClass == CreatureClass::BARD)
    pick_song(player);*/
}

//*********************************************************************
//                      downLevel
//*********************************************************************
// This function is called when a player loses a level due to dying or
// for some other reason. The appropriate stats are downgraded.

void Player::downLevel() {

    if(isStaff()) {
        level--;
        return;
    }


    PlayerClass *pClass = gConfig->classes[getClassString()];
    LevelGain *lGain = nullptr;

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

    std::string toRemove = fmt::format("Level{}", level);

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

    if(cClass != CreatureClass::LICH)
        mp.restore();

    switch (saveLost) {
    case POI:
        logn("log.saveswtf", "downLevel(): %s[L%d]'s POI save just went down. Prev: %d%, New: %d%, Gains: %d\n", 
                                getCName(), level, saves[POI].chance, saves[POI].chance-3, negativeLevels ? saves[POI].gained:0);
        saves[POI].chance -= 3;
        if(!negativeLevels)
            saves[POI].gained = 0;
        print("You are now less resistant to poison.\n");
        break;
    case DEA:
        logn("log.saveswtf", "downLevel(): %s[L%d]'s DEA save just went down. Prev: %d%, New: %d%, Gains: %d\n", 
                                getCName(), level, saves[DEA].chance, saves[DEA].chance-3, negativeLevels ? saves[DEA].gained:0);
        saves[DEA].chance -= 3;
        if(!negativeLevels)
            saves[DEA].gained = 0;
        print("You are now less able to resist traps and death.\n");
        break;
    case BRE:
        logn("log.saveswtf", "downLevel(): %s[L%d]'s BRE save just went down. Prev: %d%, New: %d%, Gains: %d\n", 
                                getCName(), level, saves[BRE].chance, saves[BRE].chance-3, negativeLevels ? saves[BRE].gained:0);
        saves[BRE].chance -= 3;
        if(!negativeLevels)
            saves[BRE].gained = 0;
        print("You are now less resistant to breath weapons and explosions.\n");
        break;
    case MEN:
        logn("log.saveswtf", "downLevel(): %s[L%d]'s MEN save just went down. Prev: %d%, New: %d%, Gains: %d\n", 
                                getCName(), level, saves[MEN].chance, saves[MEN].chance-3, negativeLevels ? saves[MEN].gained:0);
        saves[MEN].chance -= 3;
        if(!negativeLevels)
            saves[MEN].gained = 0;
        print("You are now less resistant to mind dominating attacks.\n");
        break;
    case SPL:
       logn("log.saveswtf", "downLevel(): %s[L%d]'s SPL save just went down. Prev: %d%, New: %d%, Gains: %d\n", 
                                getCName(), level, saves[SPL].chance, saves[SPL].chance-3, negativeLevels ? saves[SPL].gained:0);
        saves[SPL].chance -= 3;
        if(!negativeLevels)
            saves[SPL].gained = 0;
        print("You are now less resistant to magical spells.\n");
        break;
    }
    //  }

    updateGuild(Containable::downcasted_shared_from_this<Player>(), GUILD_DIE);
    setMonkDice();

}

//*********************************************************************
//                      cmdTrain
//*********************************************************************
// This function allows a player to train if they are in the correct
// training location and has enough gold and experience.  If so, the
// character goes up a level.

int cmdTrain(const std::shared_ptr<Player>& player, cmd* cmnd) {
    unsigned long goldneeded=0, expneeded=0, bankneeded=0, maxgold=0;

    if(player->getClass() == CreatureClass::BUILDER) {
        player->print("You don't need to do that!\n");
        return(0);
    }

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
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

    expneeded = Config::expNeeded(player->getLevel());

    if(player->getLevel() < 19)
        maxgold = 750000;
    else if(player->getLevel() < 22)
        maxgold = 2000000;
    else
        maxgold = ((player->getLevel()-22)*500000) + 3000000;

    goldneeded = std::min(maxgold, expneeded / 2L);

    if(player->getRace() == HUMAN)
        goldneeded += goldneeded/3/10; // Humans have +10% training costs.

    // Level training cost for levels 1-3 is free
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
    Server::logGold(GOLD_OUT, player, Money(goldneeded, GOLD), nullptr, "Training");

    doTrain(player);


    if(!player->flagIsSet(P_CHOSEN_ALIGNMENT) && player->getLevel() == ALIGNMENT_LEVEL) {
        player->print("You may now choose your alignment. You must do so before you can reach level %d.\n", ALIGNMENT_LEVEL+1);
        player->print("Use the 'alignment' command to do so. HELP ALIGNMENT.\n");
    }
    return(0);
}

//*********************************************************************
//                      checkWeaponSkillGain
//*********************************************************************

void Player::checkWeaponSkillGain() {
    int numWeapons = 0;
    // Everyone gets a new weapon skill every title
    if(((level+2)%3) == 0)
        numWeapons ++;
    switch(cClass) {
        case CreatureClass::FIGHTER:
            if(!hasSecondClass()) {
                if(level%4 == 0)
                    numWeapons++;
            } else {
                // Mutli fighters get weapon skills like other fighting classes
                if(level%8 == 8)
                    numWeapons++;
            }
            break;
        case CreatureClass::BERSERKER:
            if(level%6)
                numWeapons++;
            break;
        case CreatureClass::THIEF:
        case CreatureClass::RANGER:
        case CreatureClass::ROGUE:
        case CreatureClass::BARD:
        case CreatureClass::PALADIN:
        case CreatureClass::DEATHKNIGHT:
        case CreatureClass::ASSASSIN:
            if(level/8)
                numWeapons++;
            break;
        case CreatureClass::CLERIC:
        case CreatureClass::DRUID:
        case CreatureClass::PUREBLOOD:
            if(hasSecondClass()) {
                // Cle/Ass
                if(level/8)
                    numWeapons++;
            } else {
                if(level/12)
                    numWeapons++;
            }
            break;
        case CreatureClass::MAGE:
            if(hasSecondClass()) {
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
