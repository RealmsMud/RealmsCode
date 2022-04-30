/*
 * playerInfo.cpp
 *   Player Information
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

#include <cstdio>                  // for sprintf
#include <ctime>                   // for ctime
#include <iomanip>                 // for operator<<, setw, setfill
#include <locale>                  // for locale
#include <ostream>                 // for operator<<, basic_ostream, ostring...
#include <string>                  // for char_traits, allocator, operator<<

#include "catRef.hpp"              // for CatRef
#include "clans.hpp"               // for Clan
#include "cmd.hpp"                 // for cmd
#include "commands.hpp"            // for spellsUnder, cmdDaily, cmdScore
#include "config.hpp"              // for Config, gConfig
#include "deityData.hpp"           // for DeityData
#include "effects.hpp"             // for EffectInfo
#include "flags.hpp"               // for P_AFK, P_PTESTER, P_CHAOTIC, P_CHA...
#include "global.hpp"              // for CreatureClass, CreatureClass::CLERIC
#include "location.hpp"            // for Location
#include "magic.hpp"               // for S_HEAL, S_TELEPORT, S_TRACK
#include "money.hpp"               // for Money, GOLD
#include "mud.hpp"                 // for DL_RESURRECT, DL_HANDS, DL_BROAD
#include "mudObjects/players.hpp"  // for Player
#include "mudObjects/rooms.hpp"    // for BaseRoom
#include "proto.hpp"               // for up, getClassName, get_class_string
#include "raceData.hpp"            // for RaceData
#include "server.hpp"              // for Server, gServer
#include "size.hpp"                // for getSizeName
#include "socket.hpp"              // for Socket
#include "statistics.hpp"          // for Statistics
#include "stats.hpp"               // for Stat
#include "structs.hpp"             // for daily, saves
#include "utils.hpp"               // for MIN
#include "xml.hpp"                 // for loadPlayer

class Object;


//*********************************************************************
//                      cmdScore
//*********************************************************************
// This function shows a player their current hit points, magic points,
// experience, gold and level.

int cmdScore(Player* player, cmd* cmnd) {
    Player  *target = player;
    bool online=true;

    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }
    
    player->clearFlag(P_AFK);


    if(player->isDm() && cmnd->num > 1) {
        cmnd->str[1][0] = up(cmnd->str[1][0]);
        target = gServer->findPlayer(cmnd->str[1]);
        if(!target) {
            loadPlayer(cmnd->str[1], &target);
            online = false;
        }
        if(!target) {
            player->print("That player does not exist.\n");
            return(0);
        }
    }

    target->score(player);

    if(!online)
        delete target;;
    return(0);
}

//*********************************************************************
//                      score
//*********************************************************************
// This function shows a player their current hit points, magic points,
// experience, gold and level.

void Player::score(const Player* viewer) {
    const EffectInfo* eff=nullptr;
    int     i=0;

    std::ostringstream oStr;


    viewer->print("%s the %s (level %d)", getCName(), getTitle().c_str(), getLevel());

    oStr << gServer->delayedActionStrings(this);

    if(isFleeing())
        oStr << " ^r*Fleeing*";
    if(flagIsSet(P_HIDDEN))
        oStr << " ^c*Hidden*";
    if(isEffected("courage"))
        oStr << " ^r*Brave*";
    if(isEffected("fear"))
        oStr << " ^b*Fearful*";
    if(flagIsSet(P_SITTING))
        oStr << " ^x*Sitting*";
    if(isEffected("mist"))
        oStr << " ^c*Misted*";
    if(isEffected("armor"))
        oStr << " ^y*Armor Spell*";
    if(isEffected("berserk"))
        oStr << " ^r*Berserked*";
    if(isPoisoned())
        oStr << " ^g^#*Poisoned*";


    if(isEffected("frenzy"))
        oStr << " ^c*Frenzied*";
    if(isEffected("haste", true))
        oStr << " ^c*Hasted*";
    if(isEffected("slow", true))
        oStr << " ^g*Slowed*";
    if(isEffected("strength", true))
        oStr << " ^y*Strength*";
    if(isEffected("enfeeblement", true))
        oStr << " ^y*Enfeeblement*";
    if(isEffected("insight", true))
        oStr << " ^y*Insight*";
    if(isEffected("feeblemind", true))
        oStr << " ^y*Feeblemind*";
    if(isEffected("fortitude", true))
        oStr << " ^c*Fortitude*";
    if(isEffected("weakness", true))
        oStr << " ^g*Weakness*";
    if(isEffected("prayer", true))
        oStr << " ^y*Prayer*";
    if(isEffected("damnation", true))
        oStr << " ^y*Damnation*";


    eff = getEffect("drunkenness");
    if(eff)
        oStr << " ^o*" << eff->getDisplayName() << "^o*";

    if(isEffected("confusion"))
        oStr << " ^b^#*Confused*";
    if(flagIsSet(P_CHARMED))
        oStr << " ^C*Charmed*";
    if(flagIsSet(P_FOCUSED))
        oStr << " ^c*Focused*";
    if(isEffected("silence"))
        oStr << " ^m^#*Muted*";
    if(isDiseased())
        oStr << " ^r^#*Diseased*";

    if(isEffected("camouflage"))
        oStr << " ^g*Camouflaged*";
    if(isEffected("petrification"))
        oStr << " ^c*Petrified*";
    if(isEffected("hold-person"))
        oStr << " ^c*Magically Held*";
    if(isEffected("pray") || isEffected("dkpray"))
        oStr << " ^y*Prayed*";
    if(flagIsSet(P_UNCONSCIOUS)) {
        if(flagIsSet(P_SLEEPING))
            oStr << " ^c*Sleeping*";
        else
            oStr << " ^c*Unconscious*";
    }
    if(isBraindead())
        oStr << " ^y*Brain-dead*";
    if(isEffected("wounded"))
        oStr << " ^r*Wounded*";

    viewer->printColor(oStr.str().c_str());

    viewer->printColor("\n ^g%3d/%3d Hit Points", hp.getCur(), hp.getMax());
    if(getClass() == CreatureClass::FIGHTER && flagIsSet(P_PTESTER))
        viewer->printColor("  ^g%3d/%3d Focus Points", focus.getCur(), focus.getMax());
    else if(hasMp())
        viewer->printColor("  ^g%3d/%3d Magic Points", mp.getCur(), mp.getMax());

    viewer->printColor("     ^rArmor: %d (%.0f%%)\n", getArmor(),
        (getDamageReduction(this)*100.0));
    viewer->printColor(" ^y%7ld Experience    %s    XP Needed: %7s\n",
        getExperience(), coins.str().c_str(),
        expToLevel(false).c_str());

    viewer->printColor("\n^MBank Balance: %s.\n",
        bank.str().c_str());
    viewer->printColor("Total Inventory Assets: %ld gold.\n",
        getInventoryValue());
    // if offline, player won't be in a room
    if(getConstRoomParent() && getConstRoomParent()->isWinter())
        viewer->printColor("Winter Protection: ^c%d%%\n", (int)(20 * winterProtection()) * 5);

    viewer->print("\n");
    viewer->printColor("Your size is: ^y%s^x.\n", getSizeName(getSize()).c_str());

    // show spells under also
    spellsUnder(viewer, this, viewer != this);

    if(getWarnings()) {
        viewer->printColor("\n^BWarnings: ");
        for(i=0; i<getWarnings(); i++)
            viewer->print("* ");
        viewer->print("\n");
    }

    // if they aren't logged on, they won't have a socket
    if(getSock()) {
        switch(getSock()->mccpEnabled()) {
            case 1:
                viewer->printColor("^yMCCP V1 Enabled\n");
                break;
            case 2:
                viewer->printColor("^yMCCP V2 Enabled\n");
                break;
            default:
                viewer->printColor("^rMCCP Disabled\n");
                break;
        }
    }

    if(!statsAddUp())
        viewer->printColor("^RError:^x Your stats do not add up.\n");
}






//*********************************************************************
//                      cmdDaily
//*********************************************************************

int cmdDaily(Player* player, cmd* cmnd) {
    Player* target = player;
    bool online = true;
    int     loop=0;

    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    if(player->isDm() && cmnd->num > 1) {
        cmnd->str[1][0] = up(cmnd->str[1][0]);
        target = gServer->findPlayer(cmnd->str[1]);
        if(!target) {
            loadPlayer(cmnd->str[1], &target);
            online = false;
        }
        if(!target) {
            player->print("That player does not exist.\n");
            return(0);
        }
    }

    while(loop++ <= DAILYLAST)
        update_daily(&target->daily[loop]);

    player->print("Daily actions remaining:\n\n");

    player->print("Broadcasts:     %d of %d remaining.\n", target->daily[DL_BROAD].cur, target->daily[DL_BROAD].max);
    if( target->isCt() || ((target->getClass() == CreatureClass::CLERIC || target->getClass() == CreatureClass::PALADIN) && target->spellIsKnown(S_HEAL)) ) {
        player->print("Heals:          %d of %d remaining.\n", target->daily[DL_FHEAL].cur, target->daily[DL_FHEAL].max);
    }
    if( target->isCt() || ((target->getClass() == CreatureClass::RANGER || target->getClass() == CreatureClass::DRUID) && target->spellIsKnown(S_TRACK))  ) {
        player->print("Tracks:         %d of %d remaining.\n", target->daily[DL_TRACK].cur,target->daily[DL_TRACK].max);
    }
    if( target->isCt() || ((target->getClass() == CreatureClass::LICH || (target->getClass() == CreatureClass::MAGE && !target->hasSecondClass())) && target->getLevel() >= 10) ) {
        player->print("Enchants:       %d of %d remaining.\n", target->daily[DL_ENCHA].cur, target->daily[DL_ENCHA].max);
    }
    if( target->isCt() || (target->getClass() == CreatureClass::MAGE && target->getLevel() >= 10)) {
        player->print("Transmutes:     %d of %d remaining.\n", target->daily[DL_RCHRG].cur, target->daily[DL_RCHRG].max);
    }
    if( target->isCt() || (target->getLevel() >= 9 && target->spellIsKnown(S_TELEPORT))) {
        player->print("Teleports:      %d of %d remaining.\n", target->daily[DL_TELEP].cur, target->daily[DL_TELEP].max);
    }
    if( target->isCt() || (target->getLevel() >= 16 && target->getClass() == CreatureClass::PALADIN)) {
        player->print("Lay on Hands:   %d of %d remaining.\n", target->daily[DL_HANDS].cur, target->daily[DL_HANDS].max);
    }

    if( target->isCt() || (target->getLevel() >= 16 && target->getClass() == CreatureClass::DEATHKNIGHT)) {
        player->print("Harm touches:   %d of %d remaining.\n", target->daily[DL_HANDS].cur, target->daily[DL_HANDS].max);
    }

    if( target->isDm() || (target->getLevel() >= 22 && target->getClass() == CreatureClass::CLERIC && target->getDeity() == ARAMON && !target->hasSecondClass())) {
        player->print("Bloodfusions:   %d of %d remaining.\n", target->daily[DL_RESURRECT].cur, target->daily[DL_RESURRECT].max);
    }
    if( target->isDm() || (target->getLevel() >= 19 && target->getClass() == CreatureClass::CLERIC && target->getDeity() == CERIS)) {
        player->print("Resurrections:  %d of %d remaining.\n", target->daily[DL_RESURRECT].cur, target->daily[DL_RESURRECT].max);
    }

    player->print("\n");
    player->print("Can defecate?   %s\n", target->canDefecate() ? "Yes" : "No");
    player->print("Can be resurrected?   %s\n", (target->daily[DL_RESURRECT].cur > 0) ? "Yes" : "No");


    if(!online)
        delete target;;
    return(0);
}


//*********************************************************************
//                      showSavingThrow
//*********************************************************************
// formatted saving throws

std::string showSavingThrow(const Player* viewer, const Player* player, std::string text, int st) {
    char str[100];

    if( (viewer && viewer->isCt()) ||
        player->isCt()
    ) {
        sprintf(str, "(%3d%%)", player->saves[st].chance);
        text += str;
    }
    if( st==POI &&
        player->immuneToPoison() &&
        player->immuneToDisease()
    ) {
        text += "^WN/A^x";
    } else {
        int save = MIN((1+player->saves[st].chance)/10, MAX_SAVE_COLOR-1);
        sprintf(str, "^%c%s^x", get_save_color(save), get_save_string(save));
        text += str;
    }
    return(text);
}


//*********************************************************************
//                      information
//*********************************************************************
// This function displays a player's entire list of information, including
// player stats, proficiencies, level and class.

void Player::information(const Player* viewer, bool online) {
    int     numItems=0;
    long    i=0;
    bool    auth = viewer && viewer->isCt();
    std::ostringstream oStr;
    std::string txt = "";

    if(!auth)
        update();

    oStr.setf(std::ios::left, std::ios::adjustfield);
    // no commas when displaying staff screens
    oStr.imbue(std::locale(isStaff() ? "C" : ""));
    oStr << std::setfill(' ')
         << "+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+\n";
    if(!lastPassword.empty() && viewer && viewer->isDm())
        oStr << "| Last Password: " << std::setw(14) << lastPassword << "                               ^W|\\^x            |\n";
    else
        oStr << "|                                                             ^W|\\^x            |\n";
    if(clan && auth)
        oStr << "+ Clan: " << std::setw(21) << gConfig->getClan(clan)->getName() << "                                 ^W<<\\         _^x +\n";
    else
        oStr << "+                                                             ^W<<\\         _^x +\n";

    txt = getName();
    if(viewer && viewer->isCt()) {
        txt += "(" + getId() + ")";
    }
    txt += " the ";
    txt += getTitle();

    // use i to center the title for us
    i = 47 - txt.length();

    oStr << "|                                                              ^W/ \\       //^x |\n"
        << "+ ^W.------------------------------------------------------------{o}______/|^x  +\n"
        << "|^W<^x " << std::setw((int)i/2) << " " << txt << std::setw(i-(int)i/2) << " " << " ^W=====:::{*}///////////]^x  |\n"
        << "+ ^W`------------------------------------------------------------{o}~~~~~~\\|^x  +\n";

    if(auth) {
        oStr << "| Password: " << std::setw(14) << password << " Pkills (in/won): ";
        oStr.setf(std::ios::right, std::ios::adjustfield);
        oStr << std::setw(5) << statistics.getPkwon();
        oStr.setf(std::ios::left, std::ios::adjustfield);
        oStr << "/" << std::setw(5) << statistics.getPkin() << " (";
        oStr.setf(std::ios::right, std::ios::adjustfield);
        oStr << std::setw(3) << statistics.pkRank();
        oStr << "%) ^W\\ /       \\\\^x |\n";
        oStr.setf(std::ios::left, std::ios::adjustfield);
    } else
        oStr << "|                                                              ^W\\ /       \\\\^x |\n";
    oStr << "+ ^W__________________________________                          <</         ~^x +\n"
        << "|^W/\\                                 \\                         |/^x            |\n"
        << "+^W\\_|^x Level: " << std::setw(2) << level << "    Race: " << std::setw(13)
            << gConfig->getRace(race)->getName(12) << "^W|  _________________________________^x   +\n";

    txt = !hasSecondClass() ? get_class_string(static_cast<int>(cClass)) : getClassName(this);
    if(deity) {
        txt += ", ";
        txt += gConfig->getDeity(deity)->getName();
    }

    oStr << "|  ^W|^x Class: " << std::setw(23) << txt << "  ^W| /\\                                \\^x  |\n"
        << "+  ^W|^x Alignment: ";

    oStr.setf(std::ios::right, std::ios::adjustfield);
    oStr << std::setw(7) << (flagIsSet(P_CHAOTIC) ? "Chaotic" : "Lawful");
    oStr.setf(std::ios::left, std::ios::adjustfield);

    txt = "Neutral";
    if(getAdjustedAlignment() < NEUTRAL)
        txt = "Evil";
    else if(getAdjustedAlignment() > NEUTRAL)
        txt = "Good";

    for(i=0; i<MAXWEAR; i++)
        if(ready[i])
            numItems++;
    numItems += countBagInv();


    oStr << " " << std::setw(13) << txt << "^W| \\_|^x Total Experience : " << std::setw(11) << experience << "^W|^x  +\n"
        << "|  ^W|^x Time Played:                    ^W|   |^x Experience Needed: "
        << std::setw(11) << expToLevel(false) << "^W|^x  |\n"
        << "+  ^W|^x ";
    oStr.setf(std::ios::right, std::ios::adjustfield);
    oStr << std::setw(31) << getTimePlayed();
    oStr.setf(std::ios::left, std::ios::adjustfield);
    oStr << " ^W|   |^x Gold             : " << std::setw(11) << coins[GOLD] << "^W|^x  +\n"
        << "|  ^W|   ______________________________|_  |^x Inventory Weight : " << std::setw(4) << getWeight() << "       ^W|^x  |\n"
        << "+   ^W\\_/_______________________________/  |^x Inventory Bulk   : "
            << std::setw(3) << getTotalBulk() << "(" << std::setw(3) << getMaxBulk() << ")   ^W|^x  +\n"
        << "|                                        ^W|^x Total Items      : " << std::setw(4) << numItems << "       ^W|^x  |\n"
        << "+       Hit Points   : ";

    oStr.setf(std::ios::right, std::ios::adjustfield);
    // no more commas
    oStr.imbue(std::locale("C"));
    oStr << std::setw(5) << hp.getCur() << "/" << std::setw(5) << hp.getMax()
        << "       ^W|   ____________________________|_^x +\n"
        << "|       ";

    if(cClass == CreatureClass::FIGHTER && !hasSecondClass() && flagIsSet(P_PTESTER))
        oStr << "Focus Points : " << std::setw(5) << focus.getCur() << "/" << std::setw(5) << focus.getMax();
    else if(hasMp())
        oStr << "Magic Points : " << std::setw(5) << mp.getCur() << "/" << std::setw(5) << mp.getMax();
    else {
        oStr.setf(std::ios::left, std::ios::adjustfield);
        oStr << "Armor        :   " << std::setw(9) << armor;
    }

    oStr.setf(std::ios::left, std::ios::adjustfield);
    oStr << "        ^W\\_/______________________________/^x|\n";

    if(!hasMp())
        oStr << "+                                                                           +\n";
    else
        oStr << "+       Armor        :   " << std::setw(9) << armor << "                                          +\n";

    oStr << "|                                                                           |\n"
        << "+^W     /\\       ^x             _         ^WSaving Throws^x                         +\n"
        << "|^W____/_ \\____  ^x Str: " << (strength.getCur() < 100 ? " " : "") << strength.getCur() << "^W |\\_\\\\-\\^x       " << std::setw(41) << showSavingThrow(viewer, this, "Poison/Disease:    ", POI) << "|\n"
        << "+^W\\  ___\\ \\  /^x   Dex: " << (dexterity.getCur() < 100 ? " " : "") << dexterity.getCur() << "^W |   \\\\-|^x      " << std::setw(41) << showSavingThrow(viewer, this, "Death/Traps:       ", DEA) << "+\n"
        << "|^W \\/ /  \\/ /  ^x  Con: " << (constitution.getCur() < 100 ? " " : "") << constitution.getCur() << "^W  \\ /~\\\\^x       " << std::setw(41) << showSavingThrow(viewer, this, "Breath/Explosions: ", BRE) << "|\n"
        << "+^W / /\\__/_/\\  ^x  Int: " << (intelligence.getCur() < 100 ? " " : "") << intelligence.getCur() << "^W   `   \\\\^x      " << std::setw(41) << showSavingThrow(viewer, this, "Mental Attacks:    ", MEN) << "+\n"
        << "|^W/__\\ \\_____\\^x   Pie: " << (piety.getCur() < 100 ? " " : "") << piety.getCur() << "^W        \\\\^x     " << std::setw(41) << showSavingThrow(viewer, this, "Spells:            ", SPL) << "|\n"
        << "+^W    \\  /                        \\^x                                          +\n"
        << "|^W     \\/                          `^x                                         |\n"
        << "+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+-=-+\n";

    if(!auth && !isStaff())
        oStr << "^WMore Information:^x age, daily, effects, factions, forum, languages, property,\n"
             << "                  quests, recipes, score/health, skills, spells, statistics\n";

    if(viewer)
        viewer->printColor("%s\n", oStr.str().c_str());
    else {
        printColor("%s", oStr.str().c_str());
        hasNewMudmail();
        print("\n");
    }
    if(!statsAddUp()) {
        if(viewer)
            viewer->printColor("^RError:^x %s's stats do not add up.\n", getCName());
        else
            printColor("^RError:^x Your stats do not add up.\n");
    }

    if(auth) {
        showAge(viewer);
        viewer->print("Bank: %-10lu  \n", bank[GOLD]);
        if(getRoomParent())
            viewer->print("Room: %s  \n", getRoomParent()->fullName().c_str());
        else
            viewer->print("Room: %s  \n", currentLocation.room.displayStr().c_str());
        if(!online)
            viewer->print("Last login:  \n%s", ctime(&lastLogin));
        else
            viewer->print("Currently logged on.\n");
    }

}

//*********************************************************************
//                      cmdChecksaves
//*********************************************************************

int cmdChecksaves(Player* player, cmd* cmnd) {
    Player* target = player;
    bool online = true;
   
   std::ostringstream oStr;

    player->clearFlag(P_AFK);

    if(player->isBraindead()) {
        *player << "You are brain-dead. You can't do that!\n";
        return(0);
    }

    if(player->isCt() && cmnd->num > 1) {
        cmnd->str[1][0] = up(cmnd->str[1][0]);
        target = gServer->findPlayer(cmnd->str[1]);
        if(!target) {
            loadPlayer(cmnd->str[1], &target);
            online = false;
        }
        if(!target) {
            *player << "That player does not exist.\n";
            return(0);
        }
    }

    oStr << "^c" << (!online ? target->getCName() : "Your") << (!online ? "'s": "") << " current saving throws and amounts gained this level:\n\n";
    oStr << "^xSave                        Gained          Rating^x\n";
    oStr << "-------------------------------------------------------------------\n"; 
    oStr << (target->saves[POI].gained == 5 ? "^R":"") << "Poison/Disease              " << target->saves[POI].gained << "               " << showSavingThrow(player, target, " ", POI) << "\n";
    oStr << (target->saves[DEA].gained == 5 ? "^R":"") << "Death/Traps                 " << target->saves[DEA].gained << "               " << showSavingThrow(player, target, " ", DEA) << "\n";
    oStr << (target->saves[BRE].gained == 5 ? "^R":"") << "Breath/Explosions           " << target->saves[BRE].gained << "               " << showSavingThrow(player, target, " ", BRE) << "\n";
    oStr << (target->saves[MEN].gained == 5 ? "^R":"") << "Mental Attacks              " << target->saves[MEN].gained << "               " << showSavingThrow(player, target, " ", MEN) << "\n";
    oStr << (target->saves[SPL].gained == 5 ? "^R":"") << "Spells                      " << target->saves[SPL].gained << "               " << showSavingThrow(player, target, " ", SPL) << "\n";
    oStr << "-------------------------------------------------------------------\n";
    oStr << "^RRED^x indicates that the save currently has gains maxed out.\nTraining to the next level resets gains to 0.\n";



    *player << ColorOn << oStr.str() << "\n" << ColorOff;
    

    if(!online)
        delete target;;
    return(0);
}
