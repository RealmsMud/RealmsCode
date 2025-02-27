/*
 * dmcrt.cpp
 *   Staff functions related to creatures
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

#include <boost/algorithm/string/replace.hpp>  // for replace_all
#include <boost/iterator/iterator_traits.hpp>  // for iterator_value<>::type
#include <boost/algorithm/string/trim.hpp>     // for trim
#include <cctype>                              // for isdigit, isspace, isalpha
#include <cstdio>                              // for sprintf
#include <cstdlib>                             // for atoi, qsort
#include <cstring>                             // for strcmp, strcpy, strlen
#include <ctime>                               // for time
#include <deque>                               // for _Deque_iterator
#include <iomanip>                             // for operator<<, setw, setfill
#include <list>                                // for list, operator==, list...
#include <locale>                              // for locale
#include <map>                                 // for operator==, _Rb_tree_c...
#include <set>                                 // for set
#include <sstream>                             // for operator<<, basic_ostream
#include <string>                              // for string, operator<<
#include <regex>
#include <string_view>                         // for operator<<

#include "calendar.hpp"                        // for cDay
#include "carry.hpp"                           // for Carry
#include "catRef.hpp"                          // for CatRef
#include "clans.hpp"                           // for Clan
#include "cmd.hpp"                             // for cmd
#include "commands.hpp"                        // for getFullstrText, cmdNoAuth
#include "config.hpp"                          // for Config, gConfig
#include "creatureStreams.hpp"                 // for Streamable
#include "deityData.hpp"                       // for DeityData
#include "dice.hpp"                            // for Dice
#include "dm.hpp"                              // for dmInvis, dmLastCommand
#include "effects.hpp"                         // for EffectInfo, EFFECT_MAX...
#include "factions.hpp"                        // for Faction, Faction::MAX_...
#include "flags.hpp"                           // for M_DM_FOLLOW, M_CUSTOM
#include "global.hpp"                          // for PROMPT, CreatureClass
#include "hooks.hpp"                           // for Hooks
#include "lasttime.hpp"                        // for lasttime
#include "location.hpp"                        // for Location
#include "magic.hpp"                           // for MAXSPELL
#include "monType.hpp"                         // for getName, getHitdice, size
#include "money.hpp"                           // for GOLD, Money
#include "move.hpp"                            // for getString
#include "mud.hpp"                             // for PLYCRT, LT_AGE, LT_JAILED
#include "mudObjects/container.hpp"            // for Container, ObjectSet
#include "mudObjects/creatures.hpp"            // for Creature, NUM_ASSIST_MOB
#include "mudObjects/monsters.hpp"             // for Monster
#include "mudObjects/objects.hpp"              // for Object
#include "mudObjects/players.hpp"              // for Player
#include "mudObjects/rooms.hpp"                // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"          // for UniqueRoom
#include "paths.hpp"
#include "proto.hpp"                           // for log_immort, mprofic
#include "raceData.hpp"                        // for RaceData
#include "range.hpp"                           // for Range
#include "realm.hpp"                           // for COLD, EARTH, ELEC, FIRE
#include "server.hpp"                          // for Server, gServer, Monst...
#include "size.hpp"                            // for getSizeName, getSize
#include "socket.hpp"                          // for Socket
#include "statistics.hpp"                      // for Statistics
#include "stats.hpp"                           // for Stat
#include "structs.hpp"                         // for saves, ttag, PFNCOMPARE
#include "threat.hpp"                          // for operator<<
#include "unique.hpp"                          // for Lore
#include "wanderInfo.hpp"                      // for WanderInfo
#include "xml.hpp"                             // for loadMonster
#include "toNum.hpp"



//*********************************************************************
//                      dmCreateMob
//*********************************************************************
// This function allows a staff member to create a creature that will appear
// in the room they are located in.

int dmCreateMob(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Monster> monster=nullptr;
    std::shared_ptr<BaseRoom> room = player->getRoomParent();
    int     l=0, total=1;
    std::string noMonsters = "^mNo monsters were summoned.\n";

    CatRef  cr;
    getCatRef(getFullstrText(cmnd->fullstr, 1), cr, player);


    if(cr.id && !player->checkBuilder(cr)) {
        player->print("Error: monster index not in any of your alotted ranges.\n");
        return(0);
    }
    if(!player->checkBuilder(player->getUniqueRoomParent())) {
        player->print("Error: room number not in any of your alotted ranges.\n");
        return(0);
    }
    if(!player->builderCanEditRoom("create monsters"))
        return(0);

    WanderInfo* wander = player->getRoomParent()->getWanderInfo();

    if(cr.id < 1) {
        if(!wander) {
            player->printColor(noMonsters.c_str());
            return(0);
        }
        cr = wander->getRandom();
        if(!cr.id) {
            player->printColor(noMonsters.c_str());
            return(0);
        }
    }

    if(getFullstrText(cmnd->fullstr, 2).starts_with('n'))
        total = std::min<int>(toNum<int>(getFullstrText(cmnd->fullstr, 3)), MAX_MOBS_IN_ROOM);

    total = std::min(total, MAX_MOBS_IN_ROOM - room->countCrt());

    if(total < 1) {
        player->printColor(noMonsters.c_str());
        return(0);
    }

    if(!loadMonster(cr, monster)) {
        player->print("Error (%s)\n", cr.displayStr().c_str());
        return(0);
    }

    if(!monster->flagIsSet(M_CUSTOM))
        monster->validateAc();

    if(!monster->getName()[0] || monster->getName()[0] == ' ') {
        player->print("Error (%s)\n", cr.displayStr().c_str());
        return(0);
    }


    for(l=0; l<total;) {

        monster->initMonster(cmnd->str[1][0] != 'r', cmnd->str[1][0] == 'p');

        if(!l)
            monster->addToRoom(room, total);
        else
            monster->addToRoom(room, 0);

        gServer->addActive(monster);
        l++;
        if(l < total)
            loadMonster(cr, monster);
    }

    if(!player->isDm())
        log_immort(false,player, "%s created %d %s's in room %s.\n", player->getCName(), total, monster->getCName(),
            player->getRoomParent()->fullName().c_str());

    return(0);
}

//*********************************************************************
//                      statCrt
//*********************************************************************
//  Display information on creature given to player given.

std::string Creature::statCrt(int statFlags) {
    const std::shared_ptr<const Player>pTarget = getAsConstPlayer();
    const std::shared_ptr<const Monster> mTarget = getAsConstMonster();
    std::ostringstream crtStr;
    std::string str = "";
    int     i=0, n=0;
    long    t = time(nullptr);
    char    tmp[10], spl[128][20];
    std::string txt = "";

    // set left aligned
    crtStr.setf(std::ios::left, std::ios::adjustfield);
    crtStr.imbue(std::locale(""));

    if(pTarget && pTarget->getSock()) {
        std::shared_ptr<Socket> sock = pTarget->getSock();
        crtStr << "\n" << pTarget->fullName() << " the " << pTarget->getTitle() << ": ";
        //crtStr.setfill('0');
        crtStr.setf(std::ios::right, std::ios::adjustfield);
        crtStr << "Idle: " << std::setfill('0') << std::setw(2) << ((t - sock->ltime) / 60L) << ":"
               << std::setfill('0') << std::setw(2) << ((t - sock->ltime) % 60L) << std::setfill(' ')
               << "                Toughness: " << Statistics::calcToughness(getAsCreature()) << "\n";
        crtStr.setf(std::ios::left, std::ios::adjustfield);
        //crtStr.setFill(' ');
        crtStr << "Term(" << sock->getTermType() << ")";
        if(sock->mxpEnabled())
            crtStr << "MXP Enabled";
        crtStr << " size: " << sock->getTermCols() << " x " << sock->getTermRows() << "\n";
        crtStr << "Host: " << sock->getHostname() << " Ip: " << sock->getIp() << "\n";

        if(statFlags & ISDM) {
            crtStr << "Password: " << pTarget->getPassword();
            if(!pTarget->getLastPassword().empty())
                crtStr << " Previous password: " << pTarget->getLastPassword();
            crtStr << "\n";
        }

        if(!pTarget->getForum().empty())
            crtStr << "^gForum Account:^x " << pTarget->getForum() << "\n";

        crtStr << "Room: " << pTarget->getConstRoomParent()->fullName() << "\n";
        crtStr << "Cmd : ";
        if(!pTarget->isDm() || (statFlags & ISDM))
            crtStr << dmLastCommand(pTarget);
        else
            crtStr << "l\n"; // Make it look like they just 'looked'
        crtStr << "\n";

    } else {
        std::string mobName = mTarget->getName();
        std::string mobPlural = mTarget->plural;

        boost::replace_all(mobName, "^", "^^");
        boost::replace_all(mobPlural, "^", "^^");

        if(mobPlural.empty()) {
            crtStr << "Name: " << mobName << "^x";
        } else {
            crtStr << "Name:   " << mobName << "^x";
        }

        if(!mTarget->flagIsSet(M_CUSTOM))
            crtStr << " (" << monType::getName(mTarget->type) << ":"
                   << monType::getHitdice(mTarget->type) << "HD)\n";
        else
            crtStr << " (CUSTOM:" << monType::getHitdice(mTarget->type) << "HD)\n";

        if(!mobPlural.empty())
            crtStr << "Plural: " << mobPlural << "^x\n";
        

        // Stop the comma for index
        crtStr.imbue(std::locale("C"));
        crtStr << "\nIndex: " << mTarget->info.displayStr("", 'y')
               << "                Toughness: " << Statistics::calcToughness(getAsCreature()) << "\n";
        crtStr.imbue(std::locale(""));

        if(!mTarget->getPrimeFaction().empty())
            crtStr << "\n^yPrime Faction:^x " << mTarget->getPrimeFaction() << "\n";

        crtStr << "Desc: " << mTarget->getDescription() << "\n";
        crtStr << "Talk: " << mTarget->getTalk() << "\n";
        crtStr << "Keys: [" << mTarget->key[0] << "] [" << mTarget->key[1]
                            << "] [" << mTarget->key[2] << "]\n";

        for(i=0; i<3; i++) {
            if(mTarget->attack[i][0])
                crtStr << "Attack type " << (i+1) << ": " << mTarget->attack[i] << "\n";
        }

        for(i=0; i<3; i++) {
            if(mTarget->movetype[i][0])
                crtStr << "Movement type " << (i+1) << ": " << mTarget->movetype[i] << "\n";
        }

        if(mTarget->ttalk[0])
            crtStr << "After trade talk: " << mTarget->ttalk << "\n";

        // stop the comma
        crtStr.imbue(std::locale("C"));
        if(mTarget->jail.id)
            crtStr << "\nJail Room: " << mTarget->jail.displayStr() << "\n";
        crtStr.imbue(std::locale(""));
        crtStr << "Aggro String: " << mTarget->aggroString << "\n";
        if(mTarget->flagIsSet(M_LEVEL_RESTRICTED))
            crtStr << "Max Lvl: " << mTarget->getMaxLevel() << "\n";
    }

    crtStr << "\nID: <" << getId() << ">" << " Registered(Slf/Svr): " << (isRegistered() ? "Y" : "N") << "/" << (gServer->lookupCrtId(getId()) != nullptr ? "Y" : "N") << "\n";

    crtStr << "Level: " << level;
    if(pTarget) {
        if(pTarget->getActualLevel() && pTarget->getLevel() != pTarget->getActualLevel())
            crtStr << "(" << pTarget->getActualLevel() << ")";
        if(pTarget->getNegativeLevels())
            crtStr << "   Neg lvls: " << pTarget->getNegativeLevels();
    }
    crtStr << "   Race: ";
    if(race)
        crtStr << gConfig->getRace(race)->getName();
    crtStr << "(" << race << ")\n";

    crtStr << "Class: " << std::setw(20) << getClassString();
    crtStr << "  Alignment: ";
    if(pTarget) {
        if(pTarget->flagIsSet(P_CHAOTIC))
            crtStr << "Chaotic ";
        else
            crtStr <<  "Lawful ";
    }
    crtStr << alignment << "\n";

    if(pTarget && pTarget->hasSecondClass())
        crtStr << "Second Class: " << get_class_string(static_cast<int>(pTarget->getSecondClass())) << "\n";
    if(deity)
        crtStr << "Deity: " << gConfig->getDeity(deity)->getName() << "(" << deity << ")\n";

    if(size)
        crtStr << "Size: ^Y" << getSizeName(size) << "^x\n";

    if(clan)
        crtStr << "Clan: " << gConfig->getClan(clan)->getName() << "(" << clan << ")\n";
    if(pTarget && pTarget->getGuild())
        crtStr << "^mGuild:^x " << getGuildName(pTarget->getGuild()) << "(" << pTarget->getGuild() << ")"
               << "Rank: " << pTarget->getGuildRank();

    if(mTarget && mTarget->getNumWander() > 1)
        crtStr << "Numwander: " << mTarget->getNumWander() << "\n";

    crtStr << "\n";

    crtStr << "Exp: " << experience << "  Gold: " << coins[GOLD];
    if(pTarget)
        crtStr << "  Bank: " << pTarget->bank[GOLD];
    crtStr << "\n";

    if((statFlags & ISCT) && mTarget)
        crtStr << "Last modified by: " << mTarget->last_mod << "\n";
    if(pTarget) {
        crtStr << "Lost Experience: " << pTarget->statistics.getLostExperience() << "\n";
        cDay* b = pTarget->getBirthday();
        if(b)
            crtStr << "Birthday:  Day:" << b->getDay() <<
                      "  Month:" << b->getMonth() <<
                      "  Year:" << b->getYear() << "\n";

        crtStr << "Character born: " << pTarget->getCreatedStr() << "\n";
        crtStr << "Initial stats: Str[" << pTarget->strength.getInitial() <<
                                "] Dex[" << pTarget->dexterity.getInitial() <<
                                "] Con[" << pTarget->constitution.getInitial() <<
                                "] Int[" << pTarget->intelligence.getInitial() <<
                                "] Pty[" << pTarget->piety.getInitial() << "]\n";
    }

    str = "";
    for(i=0; i<LANGUAGE_COUNT; i++) {
        if(languageIsKnown(LUNKNOWN + i)) {
            str += get_language_adj(i);
            str += ", ";
        }
    }

    if(str.empty())
        str = "None";
    else
        str = str.substr(0, str.length() - 2);

    crtStr << "Languages known: " << str << ".\n"
        << "Currently speaking: " << get_language_adj(current_language) << "\n";


    if(mTarget && mTarget->getMobTrade()) {
        crtStr << "NPC Trade: " << mTarget->getMobTradeName() << "(" << mTarget->getMobTrade() << ")";
        crtStr << "   Skill: " << get_skill_string(mTarget->getSkillLevel()/10) << "(" << mTarget->getSkillLevel() << ")\n";
    }

    if(pTarget) {
        // Stop the comma again
        crtStr.imbue(std::locale("C"));

        crtStr << "Bound Room: " << pTarget->bound.str() << "      ";
        crtStr.imbue(std::locale(""));
        crtStr << "Wimpy: " << pTarget->getWimpy() << "\n";
    } else {
        if(mTarget->getLoadAggro())
            crtStr << "  Aggro on load: " << mTarget->getLoadAggro() << "%\n";
        if(mTarget->flagIsSet(M_WILL_BE_AGGRESSIVE))
            crtStr << "  Will aggro: " << mTarget->getUpdateAggro() << "% chance (each update cycle).\n";
    }

    crtStr << "Previous Room: " << previousRoom.str() << "\n";

    crtStr << "HP: " << hp.getCur() << "/" << hp.getMax();
    crtStr << "   MP: " << mp.getCur() << "/" << mp.getMax() << "\n";

    crtStr << "Armor: " << armor;
    if(mTarget) {
        crtStr << " Defense: " << mTarget->getDefenseSkill();
        crtStr << " WeaponSkill: " << mTarget->getWeaponSkill();
    }
    crtStr << " AttackPower: " << attackPower << "\n";
    //crtStr << "   THAC0: " << thaco << "\n";

    crtStr << "Hit: " << damage.str() << " ";
    crtStr << "Damage: low " << damage.low() << ", high " << damage.high() <<
              "  Average: " << damage.average() << "\n";

    crtStr << "Str[" << strength.getCur() <<
            "]  Dex[" << dexterity.getCur() <<
            "]  Con[" << constitution.getCur() <<
            "]  Int[" << intelligence.getCur() <<
            "]  Pty[" << piety.getCur() << "]";

    if(pTarget)
        crtStr <<  "  Lck[" <<  pTarget->getLuck() << "]";
    crtStr << "\n\n";

    crtStr.setf(std::ios::right, std::ios::adjustfield);

    crtStr << "Ea: "<< getRealm(EARTH) <<
            "  Ai: "<< getRealm(WIND) <<
            "  Fi: "<< getRealm(FIRE) <<
            "  Wa: "<< getRealm(WATER) <<
            "  El: "<< getRealm(ELEC) <<
            "  Co: "<< getRealm(COLD) << "\n";

    crtStr << "Ea: " << std::setw(2) << mprofic(getAsCreature(), EARTH) << "%" <<
            "   Ai: " << std::setw(2) << mprofic(getAsCreature(), WIND) << "%" <<
            "   Fi: " << std::setw(2) << mprofic(getAsCreature(), FIRE) << "%" <<
            "   Wa: " << std::setw(2) << mprofic(getAsCreature(), WATER) << "%" <<
            "   El: " << std::setw(2) << mprofic(getAsCreature(), ELEC) << "%" <<
            "   Co: " << std::setw(2) << mprofic(getAsCreature(), COLD) << "%" << "\n\n";

    i = (saves[POI].chance + saves[DEA].chance + saves[BRE].chance +
           saves[MEN].chance + saves[SPL].chance)/5;
    crtStr << "Poi:" << std::setw(2) << saves[POI].chance << "%" <<
            "   Dea:" << std::setw(2) << saves[DEA].chance << "%" <<
            "   Bre:" << std::setw(2) << saves[BRE].chance << "%" <<
            "   Men:" << std::setw(2) << saves[MEN].chance << "%" <<
            "   Spl:" << std::setw(2) << saves[SPL].chance << "%" <<
            "   Lck:" << std::setw(2) << i << "%" << "\n";

    if(poison_dur || poison_dmg) {
        crtStr << "^y";

        if(poison_dur)
            crtStr << "Poison duration: " << poison_dur << " sec\n";

        if(poison_dmg)
            crtStr << "Poison damage: " << poison_dmg << "/tic\n";

        crtStr << "^x";
    }

    if(mTarget) {

        if(mTarget->flagIsSet(M_CAN_CAST) && (!mTarget->flagIsSet(M_CAST_PRECENT)))
            crtStr << "^rCast Percent: You need to set mflag " << (M_CAST_PRECENT+1) << " (cast percent).^x\n";

        if( mTarget->flagIsSet(M_CAN_CAST) &&
            mTarget->flagIsSet(M_CAST_PRECENT)
        ) {
            if(!mTarget->getCastChance())
                crtStr << "^rCast Percent: (cast needs to be set to 1-100%)^x\n";
            else
                crtStr << "Cast Percent: " << mTarget->getCastChance() << "%\n";
        }

        crtStr.imbue(std::locale("C"));

        crtStr << "Carried Items:\n";
        for(i=0; i<10; i++)
            crtStr << "[" << std::setw(4) << mTarget->carry[i].str() << "]";
        crtStr << "\n";

        if(mTarget->getMagicResistance())
            crtStr << "^MMagic Resistance: " << mTarget->getMagicResistance() << "% ^x\n";

        if(mTarget->flagIsSet(M_WILL_YELL_FOR_HELP) || mTarget->flagIsSet(M_YELLED_FOR_HELP)) {
            crtStr << "Rescue mobs:   ";

            n = 0;
            for(i=0; i<NUM_RESCUE; i++) {
                crtStr << "[" << (i+1) << "] ";
                if(!validMobId(mTarget->rescue[i]))
                    crtStr << "None";
                else {
                    n++;
                    crtStr << mTarget->rescue[i].displayStr();
                }
                crtStr << "   ";
            }
            if(!n)
                crtStr << "^r*set c [mob] re[scue]{1-" << NUM_RESCUE << "} {misc.id}^x";
            crtStr << "\n";
        }

        for(i=0; i<NUM_ENEMY_MOB; i++) {
            if(mTarget->enemy_mob[i].id) {
                crtStr << "Enemy Creatures: ";
                for(i=0; i<NUM_ENEMY_MOB; i++)
                    crtStr << "[" << std::setw(4) << mTarget->enemy_mob[i].displayStr() << "]";
                crtStr << "\n";
                break;
            }
        }

        for(i=0; i<NUM_ASSIST_MOB; i++) {
            if(mTarget->assist_mob[i].id) {
                crtStr << "Will Assist: ";
                for(i=0; i<NUM_ASSIST_MOB; i++)
                    crtStr << "[" << std::setw(4) << mTarget->assist_mob[i].displayStr() << "]";
                crtStr << "\n";
                break;
            }
        }
    } else {
        if(!pTarget->getPoisonedBy().empty())
            crtStr << "^r*Poisoned* by " <<  pTarget->getPoisonedBy() << ".^x";

        if( pTarget->inUniqueRoom() &&
            pTarget->getConstUniqueRoomParent()->flagIsSet(R_MOB_JAIL)
        ) {
            crtStr << "^bIn Jailhouse. ";
            crtStr << "Time remaining: " <<
                    timestr( std::max(0L,pTarget->lasttime[LT_MOB_JAILED].ltime+pTarget->lasttime[LT_MOB_JAILED].interval-t) );
            crtStr << ".^x\n";
        }

        if(pTarget->flagIsSet(P_JAILED)) {
            crtStr << "^RIn Dungeon of Despair. ";
            crtStr << "Time remaining: " <<
                    timestr( std::max(0L,pTarget->lasttime[LT_JAILED].ltime+pTarget->lasttime[LT_JAILED].interval-t) );
            crtStr << ".^x\n";
        }
    }

    crtStr << "Flags set: " << getFlagList(", ") << "\n";

    crtStr << effects.getEffectsList();
    crtStr << getSpecialsList();

    str = "";
    if(mTarget) {
        for(i=1; i<static_cast<int>(STAFF); i++) {
            if(mTarget->isClassAggro(i, false)) {
                str += getClassAbbrev(i);
                sprintf(tmp, "(%d), ", i);
                str += tmp;
            }
        }

        for(i=1; i<RACE_COUNT; i++) {
            if(mTarget->isRaceAggro(i, false)) {
                str += gConfig->getRace(i)->getName();
                sprintf(tmp, "(%d), ", i);
                str += tmp;
            }
        }

        for(i=1; i<DEITY_COUNT; i++) {
            if(mTarget->isDeityAggro(i, false)) {
                str += gConfig->getDeity(i)->getName();
                sprintf(tmp, "(%d), ", i);
                str += tmp;
            }
        }


        if(!str.empty()) {
            str = str.substr(0, str.length() - 2);
            crtStr << "Will aggro: " << str << ".\n";
        }


        str = "";

        if(mTarget->flagIsSet(M_TRADES)) {
            crtStr << "Trade items:\n";
            for(i=0; i<10; i++) {
                if(i == 5)
                    crtStr << "\n";
                crtStr << std::setw(15) << mTarget->carry[i].str();
            }
            crtStr << str << "\n";
        }

        for(i=0, n=0; i<get_spell_list_size(); i++)
            if(mTarget->spellIsKnown(i))
                strcpy(spl[n++], get_spell_name(i));

        if(!n)
            str = "None";
        else {
            qsort((void *) spl, n, 20, (PFNCOMPARE) strcmp);
            for(i=0; i<n; i++) {
                if(i)
                    str += ", ";
                str += spl[i];
            }
        }

        crtStr << "Spells Known: " << str << ".\n";

    } else {

        for(i=1, n=0; i<65; i++) {
            if(pTarget->questIsSet(i)) {
                sprintf(tmp, "(%d) - (", i+1);
                str += tmp;
                str += get_quest_name(i);
                str += "), ";
            }
        }

        if(str.empty())
            str = "None";
        else
            str = str.substr(0, str.length() - 2);
        crtStr << "Quests Completed: " << str << ".\n";


//      str = "";
//      for(i=0; i<2; i++) {
//          if(pTarget->lastDeath[i]) {
//              crtStr << "  Death #" << i+1 << ": " << pTarget->killedBy[i]
//                     << " on " << ctime(&pTarget->lastDeath[i]);
//          }
//      }

        crtStr << "Lore: ";
        i=0;
        if(!pTarget->lore.empty()) {
            std::list<CatRef>::const_iterator it;
            for(it = pTarget->lore.begin() ; it != pTarget->lore.end() ; it++) {
                if(i)
                    crtStr << ", ";
                crtStr << "^y" << (*it).displayStr() << "^x";
                i++;
            }
        }
        crtStr << "\n";
    }

    crtStr << hooks.display()
            << "\n";

    return(crtStr.str());
}

//*********************************************************************
//                      dmSetCrt
//*********************************************************************
// This function allows a DM to set a defining characteristic of
// a creature or player

/*          Web Editor
 *           _   _  ____ _______ ______
 *          | \ | |/ __ \__   __|  ____|
 *          |  \| | |  | | | |  | |__
 *          | . ` | |  | | | |  |  __|
 *          | |\  | |__| | | |  | |____
 *          |_| \_|\____/  |_|  |______|
 *
 *      If you change anything here, make sure the changes are reflected in the web
 *      editor! Either edit the PHP yourself or tell Dominus to make the changes.
 */

int dmSetCrt(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> target=nullptr;
    std::shared_ptr<Player> pTarget=nullptr;
    std::shared_ptr<Monster> mTarget=nullptr;
    int     i=0, num=0, test=0, a=0, sp=0,  rnum=0, f=0;
    std::shared_ptr<Object> object=nullptr;
    bool    ctModBuilder=false;
    std::string intText="", objText="";
    std::string rNumString;

    if(player->getClass() == CreatureClass::BUILDER) {
        if(!player->canBuildMonsters())
            return(cmdNoAuth(player));
        if(!player->checkBuilder(player->getUniqueRoomParent())) {
            player->print("Error: Room number not inside any of your alotted ranges.\n");
            return(0);
        }
    }

    if(cmnd->num < 4) {
        player->print("Syntax: *set c <name> <a|con|c|dex|e|f|g|hm|h|int|inv|l|mm|m|\npie|p#|r#|sp|str> [<value>]\n");
        return(0);
    }



    cmnd->str[2][0] = up(cmnd->str[2][0]);
    target = gServer->findPlayer(cmnd->str[2]);
    cmnd->str[2][0] = low(cmnd->str[2][0]);
    if(!target || (!player->isCt() && target->flagIsSet(P_DM_INVIS)))
        target = player->getParent()->findCreature(player, cmnd, 2);

    if(!target) {
        player->print("Creature not found.\n");
        return(0);
    }

    pTarget = target->getAsPlayer();
    mTarget = target->getAsMonster();

    // trying to modify a player?
    if(pTarget && !player->isDm()) {
        if(player->getClass() <= CreatureClass::BUILDER) {
            player->print("You are not allowed to do that.\n");
            return(0);
        }
        if(player != pTarget) {
            if(!pTarget->isStaff()) {
                player->print("You may only modify builder characters.\n");
                return(0);
            }
            ctModBuilder = true;
        }
    }

    if(mTarget) {
        if(mTarget->info.id && !player->checkBuilder(mTarget->info)) {
            player->print("Error: Monster not inside any of your alotted ranges.\n");
            return(0);
        }
        // Don't touch
        mTarget->setFlag(M_WILL_BE_LOGGED);
    }


    switch (low(cmnd->str[3][0])) {
    case 'a':

        if( !strcmp(cmnd->str[3], "ac") ||
            (strlen(cmnd->str[3]) > 1 && !strncmp(cmnd->str[3], "armor", strlen(cmnd->str[3])))
        ) {

            target->setArmor(cmnd->val[3]);

            player->print("Armor Class set.\n");
            log_immort(true, player, "%s set %s %s's armor class to %ld.\n",
                player->getCName(), PLYCRT(target), target->getCName(), cmnd->val[3]);
            break;

        } else if(!strcmp(cmnd->str[3], "ag") ) {

            if(!mTarget) {
                player->print("Error: aggro percent chance cannot be set on players.\n");
                return(PROMPT);
            }

            mTarget->setLoadAggro((short)cmnd->val[3]);
            log_immort(true, player, "%s set %s %s's aggro chance to %ld.\n",
                player->getCName(), PLYCRT(mTarget), mTarget->getCName(), mTarget->getLoadAggro());

            player->print("Aggro percent chance set.\n");
            break;

        } else if(!strcmp(cmnd->str[3], "age")) {

            if(!pTarget) {
                player->print("Error: age cannot be set on mobs.\n");
                return(PROMPT);
            }

            pTarget->lasttime[LT_AGE].ltime = time(nullptr);
            pTarget->lasttime[LT_AGE].interval = cmnd->val[3];

            player->print("Player age set.\n");
            break;

        } else if(!strcmp(cmnd->str[3], "au")) {

            if(!mTarget) {
                player->print("Error: aggro update chance cannot be set on players.\n");
                return(PROMPT);
            }

            mTarget->setUpdateAggro((short)cmnd->val[3]);
            player->print("Aggro update chance set.\n");
            player->print("%M has a %d%% chance to go aggressive each update cycle.\n", mTarget.get(), mTarget->getUpdateAggro());
            log_immort(true, player, "%s set %s %s's chance to go aggro each update cycle to %d.\n",
                player->getCName(), PLYCRT(mTarget), mTarget->getCName(), mTarget->getUpdateAggro());
            break;

        } else if(!strcmp(cmnd->str[3], "alvl")) {

            if(!pTarget) {
                player->print("Error: actual level cannot be set on mobs.\n");
                return(PROMPT);
            }

            pTarget->setActualLevel((short)cmnd->val[3]);
            player->print("Actual Level set.\n");
            log_immort(true, player, "%s set %s %s's actual level to %d.\n",
                player->getCName(), PLYCRT(pTarget), pTarget->getCName(), pTarget->getActualLevel());
            break;

        } else if(!strcmp(cmnd->str[3], "al")) {

            std::string align_txt = cmnd->str[4];

            if (!align_txt.empty()) {
                bool al_match=true;

                if (align_txt == "bloodred")
                    target->setAlignment((short)(target->isPlayer()?((P_TOP_BLOODRED+P_BOTTOM_BLOODRED)/2):((M_TOP_BLOODRED+M_BOTTOM_BLOODRED)/2)));
                else if (align_txt == "red")
                    target->setAlignment((short)(target->isPlayer()?((P_TOP_RED+P_BOTTOM_RED)/2):((M_TOP_RED+M_BOTTOM_RED)/2)));
                else if (align_txt == "reddish")
                    target->setAlignment((short)(target->isPlayer()?((P_TOP_REDDISH+P_BOTTOM_REDDISH)/2):((M_TOP_REDDISH+M_BOTTOM_REDDISH)/2)));
                else if (align_txt == "pinkish")
                     target->setAlignment((short)(target->isPlayer()?((P_TOP_PINKISH+P_BOTTOM_PINKISH)/2):((M_TOP_PINKISH+M_BOTTOM_PINKISH)/2)));
                else if (align_txt == "neutral")
                     target->setAlignment(0);
                else if (align_txt == "lightblue")
                     target->setAlignment((short)(target->isPlayer()?((P_TOP_LIGHTBLUE+P_BOTTOM_LIGHTBLUE)/2):((M_TOP_LIGHTBLUE+M_BOTTOM_LIGHTBLUE)/2)));
                else if (align_txt == "bluish")
                    target->setAlignment((short)(target->isPlayer()?((P_TOP_BLUISH+P_BOTTOM_BLUISH)/2):((M_TOP_BLUISH+M_BOTTOM_BLUISH)/2)));
                else if (align_txt == "blue")
                    target->setAlignment((short)(target->isPlayer()?((P_TOP_BLUE+P_BOTTOM_BLUE)/2):((M_TOP_BLUE+M_BOTTOM_BLUE)/2)));
                else if (align_txt == "royalblue")
                    target->setAlignment((short)(target->isPlayer()?((P_TOP_ROYALBLUE+P_BOTTOM_ROYALBLUE)/2):((M_TOP_ROYALBLUE+M_BOTTOM_ROYALBLUE)/2)));
                else {
                    al_match=false;
                    *player << ColorOn << "^yInvalid alignment: '" << align_txt << "'\nEnter in alignment name or number. i.e. name: bloodred, red, royalblue, etc.. or number (-1000 to 1000)\nNOTE: 'royal blue' or 'blood red' will not work. Use no spaces.\n" << ColorOff;
                    break;
                }
            if (al_match)
                *player << ColorOn << setf(CAP) << target << "'s alignment value is now set to median " << target->alignColor() << target->alignString() << "^x. (" << target->getAlignment() << ") [" << (target->isPlayer()?"Player":"Monster") << "]\n" << ColorOff; 
            break;
            }

            target->setAlignment((short)cmnd->val[3]);

            *player << ColorOn << setf(CAP) << target << "'s alignment value is now set to " << target->getAlignment() << ". " << target->alignColor() << "(" << target->alignString() << ")\n" << ColorOff;
            log_immort(true, player, "%s set %s %s's alignment to %d.\n",
                player->getCName(), PLYCRT(target), target->getCName(), target->getAlignment());
            break;

        } else if(!strcmp(cmnd->str[3], "att")) {

            target->setAttackPower((int)cmnd->val[3]);
            player->print("Attack power set to %d\n", target->getAttackPower());
            break;

        }

        if(cmnd->num >= 4 && cmnd->str[4][0] == 'd') {
            for(i=0; i<NUM_ASSIST_MOB; i++)
                mTarget->assist_mob[i].clear();
            player->print("%M's assist mobs deleted.\n", mTarget.get());
            return(PROMPT);
        }

        num = toNum<int>(&cmnd->str[3][1]);
        if(num < 1 || num > NUM_ASSIST_MOB) {
            player->print("Error: %d is out of range for assist mobs (1-%d).\n", num, NUM_ASSIST_MOB);
            return(PROMPT);
        }

        if(!mTarget) {
            player->print("Error: assist mobs cannot be set on players.\n");
            return(PROMPT);
        }

        getCatRef(getFullstrText(cmnd->fullstr, 4), mTarget->assist_mob[num-1], mTarget);
        player->print("%M's assist mob #%d set to creature %s.\n", mTarget.get(), num,
                      mTarget->assist_mob[num - 1].displayStr().c_str());

        log_immort(true, player, "%s set %s %s's AssistMob#%d to %s.\n",
            player->getCName(), PLYCRT(mTarget), mTarget->getCName(), num, mTarget->assist_mob[num - 1].displayStr().c_str());
        break;

    case 'b':
        if(!strcmp(cmnd->str[3], "bank")) {
            if(!pTarget) {
                player->print("Error: bank gold cannot be set on mobs.\n");
                return(PROMPT);
            }

            pTarget->bank.set(cmnd->val[3], GOLD);

            player->print("%M bank gold is now set at %ld.\n", pTarget.get(), pTarget->bank[GOLD]);
            log_immort(true, player, "%s set %s %s's Bank gold to %d.\n",
                player->getCName(), PLYCRT(pTarget), pTarget->getCName(), pTarget->bank[GOLD]);

        }


        if(!strcmp(cmnd->str[3], "bound")) {
            if(!pTarget) {
                player->print("Error: bound room cannot be set on mobs.\n");
                return(0);
            }
            if(ctModBuilder) {
                player->print("Error: you cannot modify bound room on builders.\n");
                return(0);
            }

            getDestination(getFullstrText(cmnd->fullstr, 4), pTarget->bound, player);
            player->print("Bound room set to %s.\n", pTarget->bound.str().c_str());

            log_immort(true, player, "%s set %s %s's bound room to %s.\n",
                player->getCName(), PLYCRT(pTarget), pTarget->getCName(), pTarget->bound.str().c_str());
        }

        break;
    case 'c':
        if(!strcmp(cmnd->str[3], "cag")) {
            if(!mTarget) {
                player->print("Error: class aggro cannot be set on players.\n");
                return(PROMPT);
            }
            num = cmnd->val[3];

            if(!strcmp(cmnd->str[4], "del")) {
                for(a=1; a<static_cast<int>(STAFF); a++)
                    mTarget->clearClassAggro(a);
                player->print("All %s's class aggros now cleared.\n");
                return(0);
            }

            if(num < 1 || num >= static_cast<int>(STAFF)) {
                player->print("Error: class aggro out of range.\n");
                return(0);
            }

            if(mTarget->isClassAggro(num, false)) {
                player->print("%M will no longer attack %s's.\n", mTarget.get(), getClassAbbrev(num));
                mTarget->clearClassAggro(num);
                log_immort(true, player, "%s set %s %s's to NOT aggro %ss(%d).\n",
                    player->getCName(), PLYCRT(mTarget), mTarget->getCName(), getClassAbbrev(num), num);
            } else {
                mTarget->setClassAggro(num);
                player->print("%M will now attack %s's.\n", mTarget.get(), getClassAbbrev(num));
                log_immort(true, player, "%s set %s %s's to aggro %ss(%d).\n",
                    player->getCName(), PLYCRT(mTarget), mTarget->getCName(), getClassAbbrev(num), num);
            }

            break;
        }


        if(!strcmp(cmnd->str[3], "con")) {
            target->constitution.setMax(std::max(1, std::min<int>(cmnd->val[3], MAX_STAT_NUM)));
            target->constitution.restore();
            player->print("Constitution set.\n");

            log_immort(true, player, "%s set %s %s's constitution to %d.\n",
                player->getCName(), PLYCRT(target), target->getCName(), target->constitution.getCur());
            break;
        }

        if(!strcmp(cmnd->str[3], "cast")) {
            if(!mTarget) {
                player->print("Error: cast chance cannot be set on players.\n");
                return(PROMPT);
            }

            mTarget->setCastChance((short)cmnd->val[3]);
            player->print("Casting %% chance set.\n");
            log_immort(true, player, "%s set %s %s's Cast %% chance to %d.\n",
                player->getCName(), PLYCRT(mTarget), mTarget->getCName(), mTarget->getCastChance());

            break;
        }

        if(!strcmp(cmnd->str[3], "c2")) {
            if(!pTarget) {
                player->print("Error: second class cannot be set on monsters.\n");
                return(0);
            }
            if(ctModBuilder) {
                player->print("Error: you cannot modify second class on builders.\n");
                return(0);
            }

            pTarget->setSecondClass(static_cast<CreatureClass>(cmnd->val[3]));
            player->print("Player's 2nd class set.\n");
            log_immort(true, player, "%s set %s %s's 2nd class to %d.\n",
                player->getCName(), PLYCRT(pTarget), pTarget->getCName(), pTarget->getSecondClass());
            break;
        }

        if(!strcmp(cmnd->str[3], "clan") || !strcmp(cmnd->str[3], "cl")) {
            target->setClan((short)cmnd->val[3]);
            if(pTarget) {
                if(pTarget->getClan() == 0)
                    pTarget->clearFlag(P_PLEDGED);
                else
                    pTarget->setFlag(P_PLEDGED);
            } else
                player->print("Dont forget to %s flag 33 or 34 (Pledge/Rescind).\n", mTarget->getClan() ? "set" : "clear");
            if(target->getClan()) {
                player->print("Clan set to %d(%s).\n", target->getClan(), gConfig->getClan(target->getClan())->getName().c_str());
                log_immort(true, player, "%s set %s %s's clan to %d(%s).\n",
                    player->getCName(), PLYCRT(target), target->getCName(), target->getClan(), gConfig->getClan(target->getClan())->getName().c_str());
            } else {
                player->print("Clan cleared.\n");
                log_immort(true, player, "%s cleared %s %s's clan.\n",
                    player->getCName(), PLYCRT(target), target->getCName());
                }
            break;
        }
        if(ctModBuilder) {
            player->print("Error: you cannot modify class on builders.\n");
            return(0);
        }
        if(!player->isDm() && pTarget)
            return(PROMPT);
        {
        CreatureClass cClass = target->getClass();
        target->setClass(static_cast<CreatureClass>(cmnd->val[3]));


        if(cClass == CreatureClass::PUREBLOOD && target->getClass() !=  CreatureClass::PUREBLOOD)
            player->printColor("Auto-removing ^rVampirism.\n");
        else if(cClass == CreatureClass::WEREWOLF && target->getClass() !=  CreatureClass::WEREWOLF)
            player->printColor("Auto-removing ^oLycanthropy.\n");

        if(cClass != CreatureClass::PUREBLOOD && target->getClass() == CreatureClass::PUREBLOOD)
            player->printColor("Auto-adding ^rVampirism.\n");
        else if(cClass != CreatureClass::WEREWOLF && target->getClass() == CreatureClass::WEREWOLF)
            player->printColor("Auto-adding ^oLycanthropy.\n");


        player->print("Class set.\n");
        log_immort(true, player, "%s set %s %s's class to %d.\n",
            player->getCName(), PLYCRT(target), target->getCName(), target->getClass());

        if(pTarget)
            pTarget->update();
        }
        break;
    case 'd':
        if(!strcmp(cmnd->str[3], "dag")) {
            if(!mTarget) {
                player->print("Error: deity aggro cannot be set on players.\n");
                return(PROMPT);
            }
            num = cmnd->val[3];

            if(!strcmp(cmnd->str[4], "del")) {
                for(a=1; a<DEITY_COUNT; a++)
                    mTarget->clearDeityAggro(a);
                player->print("All %s's class aggros now cleared.\n");
                return(0);
            }

            if(num < 1 || num >= DEITY_COUNT) {
                player->print("Error: deity aggro out of range.\n");
                return(0);
            }

            if(mTarget->isDeityAggro(num, false)) {
                player->print("%M will no longer attack %s's.\n", mTarget.get(), gConfig->getDeity(num)->getName().c_str());
                mTarget->clearDeityAggro(num);
                log_immort(true, player, "%s set %s %s's to NOT aggro %ss(%d).\n",
                    player->getCName(), PLYCRT(mTarget), mTarget->getCName(), gConfig->getDeity(num)->getName().c_str(), num);
            } else {
                mTarget->setDeityAggro(num);
                player->print("%M will now attack %s's.\n", mTarget.get(), gConfig->getDeity(num)->getName().c_str());
                log_immort(true, player, "%s set %s %s's to aggro %ss(%d).\n",
                    player->getCName(), PLYCRT(mTarget), mTarget->getCName(), gConfig->getDeity(num)->getName().c_str(), num);
            }

            break;
        }

        if(!strcmp(cmnd->str[3], "deity")) {
            target->setDeity((short)cmnd->val[3]);
            player->print("Deity set.\n");
            log_immort(true, player, "%s set %s %s's deity to %d(%s).\n",
                player->getCName(), PLYCRT(target), target->getCName(), target->getDeity(),
                gConfig->getDeity(target->getDeity())->getName().c_str());
            break;
        }


        if(!strcmp(cmnd->str[3], "dex")) {
            target->dexterity.setMax(std::max(1, std::min<int>(cmnd->val[3], MAX_STAT_NUM)));
            target->dexterity.restore();
            player->print("Dexterity set.\n");
            log_immort(true, player, "%s set %s %s's dexterity to %d.\n",
                player->getCName(), PLYCRT(target), target->getCName(), target->dexterity.getCur());
            break;
        } else if(!strcmp(cmnd->str[3], "dn") && mTarget) {

            mTarget->damage.setNumber(cmnd->val[3]);
            player->print("Number of dice set.\n");
            log_immort(true, player, "%s set %s %s's Number of Dice to %d.\n",
                player->getCName(), PLYCRT(mTarget), mTarget->getCName(), mTarget->damage.getNumber());
            break;
        } else if(!strcmp(cmnd->str[3], "ds") && mTarget) {

            mTarget->damage.setSides(cmnd->val[3]);
            player->print("Sides of dice set.\n");
            log_immort(true, player, "%s set %s %s's Sides of dice to %d.\n",
                player->getCName(), PLYCRT(mTarget), mTarget->getCName(), mTarget->damage.getSides());
            break;
        } else if(!strcmp(cmnd->str[3], "dp") && mTarget) {

            mTarget->damage.setPlus(cmnd->val[3]);
            player->print("Plus on dice set.\n");
            log_immort(true, player, "%s set %s %s's plus on dice to %d.\n",
                player->getCName(), PLYCRT(mTarget), mTarget->getCName(), mTarget->damage.getPlus());
            break;
        } else if(!strcmp(cmnd->str[3], "def") && mTarget) {
            num = (int)cmnd->val[3];
            num = std::max(0, std::min(num, MAXALVL*10));
            mTarget->setDefenseSkill(num);
            player->print("Target defense set to %d.\n", num);
            break;
        }
        break;
    case 'e':
        if(cmnd->str[3][1] == 'f') {
            if(cmnd->num < 5) {
                player->print("Set what effect to what?\n");
                return(0);
            }

            long duration = -1;
            int strength = 1;

            std::string txt = getFullstrText(cmnd->fullstr, 5);
            if(!txt.empty())
                duration = (long)std::stoi(txt);
            txt = getFullstrText(cmnd->fullstr, 6);
            if(!txt.empty())
                strength = std::stoi(txt);

            if(duration > EFFECT_MAX_DURATION || duration < -1) {
                player->print("Duration must be between -1 and %d.\n", EFFECT_MAX_DURATION);
                return(0);
            }

            if((strength < 0 || strength > EFFECT_MAX_STRENGTH) && strength != -1) {
                player->print("Strength must be between 0 and %d.\n", EFFECT_MAX_STRENGTH);
                return(0);
            }

            std::string effectStr = cmnd->str[4];
            EffectInfo* toSet = nullptr;
            if((toSet = target->getExactEffect(effectStr))) {
                // We have an existing effect we're modifying
                if(duration == 0) {
                    // Duration is 0, so remove it
                    target->removeEffect(toSet, true);
                    player->print("Effect '%s' (creature) removed.\n", effectStr.c_str());
                } else {
                    // Otherwise modify as appropriate
                    toSet->setDuration(duration);
                    if(strength != -1)
                        toSet->setStrength(strength);
                    player->print("Effect '%s' (creature) set to duration %d and strength %d.\n", effectStr.c_str(), toSet->getDuration(), toSet->getStrength());
                }
                break;
            } else {
                // No existing effect, add a new one
                if(strength == -1)
                    strength = 1;
                if(target->addEffect(effectStr, duration, strength, nullptr, true) != nullptr) {
                    player->print("Effect '%s' (creature) added with duration %d and strength %d.\n", effectStr.c_str(), duration, strength);
                } else {
                    player->print("Unable to add effect '%s' (creature)\n", effectStr.c_str());
                }
                break;
            }

        } else if(cmnd->str[3][1] == 'x') {

            target->setExperience(cmnd->val[3]);
            if(target->isMonster() && !player->isDm()) {
                if(target->getExperience() > 10000)
                    target->setExperience(10000);
            }
            player->print("%M has %ld experience.\n", target.get(), target->getExperience());
            log_immort(true, player, "%s set %s %s's experience to %ld.\n",
                player->getCName(), PLYCRT(target), target->getCName(), target->getExperience());
            break;
        }

        if(cmnd->str[4][0] == 'd') {
            for(i=0; i<NUM_ENEMY_MOB; i++)
                mTarget->enemy_mob[i].clear();
            player->print("%M's enemy mobs deleted.\n", mTarget.get());
            break;
        }

        num = toNum<int>(&cmnd->str[3][1]);
        if(num < 1 || num > NUM_ENEMY_MOB) {
            player->print("Error: %d is out of range for enemy mobs (1-%d).\n", num, NUM_ENEMY_MOB);
            return(0);
        }

        if(!mTarget) {
            player->print("Error: enemy mobs cannot be set on players.\n");
            return(0);
        }

        getCatRef(getFullstrText(cmnd->fullstr, 4), mTarget->enemy_mob[num-1], mTarget);
        player->print("%M's enemy mob #%d set to creature %s.\n", mTarget.get(), num, mTarget->enemy_mob[num - 1].displayStr().c_str());
        log_immort(true, player, "%s set %s %s's EnemyMob#%d to %s.\n",
            player->getCName(), PLYCRT(mTarget), mTarget->getCName(), num, mTarget->enemy_mob[num - 1].displayStr().c_str());
        break;


    case 'f':
        switch(cmnd->str[3][1]) {
        case 'a':
        {
            if(cmnd->num < 5) {
                player->print("Set what faction to what?\n");
                return(0);
            }

            const Faction *faction = gConfig->getFaction(cmnd->str[4]);
            if(!faction) {
                player->print("'%s' is an invalid faction.\n", cmnd->str[4]);
                return(0);
            }

            if(pTarget) {
                if(cmnd->val[4] > faction->getUpperLimit(pTarget) || cmnd->val[4] < faction->getLowerLimit(pTarget)) {
                    player->print("Faction rating must be between %d and %d for this player.\n",
                        faction->getUpperLimit(pTarget), faction->getLowerLimit(pTarget));
                    return(0);
                }
            } else {
                if(cmnd->val[4] > Faction::MAX_FACTION || cmnd->val[4] < Faction::MIN_FACTION) {
                    player->print("Faction rating must be between %d and %d.\n", Faction::MIN_FACTION, Faction::MAX_FACTION);
                    return(0);
                }
            }

            target->factions[faction->getName()] = cmnd->val[4];
            //target->faction[num] = (int)cmnd->val[3];
            player->print("%s's faction %s(%s) set to %d.\n", target->getCName(), faction->getName().c_str(), faction->getDisplayName().c_str(), cmnd->val[4]);
            log_immort(true, player, "%s set %s %s's faction %s(%s) to %d.\n",
                player->getCName(), PLYCRT(target), target->getCName(), faction->getName().c_str(), faction->getDisplayName().c_str(), cmnd->val[4]);
            return(0);
        }
        case 'c':
            if(pTarget) {
                pTarget->focus.setCur(std::max(1,std::min(30000,(int)cmnd->val[3])));
                player->print("Current Focus points set.\n");
                log_immort(true, player, "%s set %s %s's Current Focus Points to %d.\n",
                    player->getCName(), PLYCRT(pTarget), pTarget->getCName(), pTarget->focus.getCur());
                return(0);
            }
        case 'm':
            if(pTarget) {
                pTarget->focus.setMax(std::max(1,std::min(30000,(int)cmnd->val[3])));
                player->print("Max Focus Points set.\n");
                log_immort(true, player, "%s set %s %s's Max Focus Points to %d.\n",
                    player->getCName(), PLYCRT(pTarget), pTarget->getCName(), pTarget->focus.getMax());
                return(0);
            }
        }




        if(cmnd->num >= 5 && cmnd->str[4][0] == 'd' && mTarget) {
            for(f=0; f < MAX_MONSTER_FLAGS; f++) {
                mTarget->clearFlag(f);
            }
            player->print("All of %s's flags deleted.\n", target->getCName());
            break;
        }

        num = cmnd->val[3];
        if(num < 1 || num > (target->isPlayer() ? MAX_PLAYER_FLAGS : MAX_MONSTER_FLAGS)) {
            player->print("Error: flag out of range.\n");
            return(0);
        }

        if( (mTarget && num == M_DM_FOLLOW+1) ||
            ( pTarget && !player->isDm() &&
                (   num == P_CAN_OUTLAW+1 ||
                    num == P_CAN_AWARD+1 ||
                    num == P_CT_CAN_DM_BROAD+1 ||
                    num == P_BUGGED+1 ||
                    num == P_CT_CAN_DM_LIST+1 ||
                    num == P_NO_COMPUTE+1
                )
            )
        ) {
           player->print("You cannot set that flag.\n");
           return(0);
        }

        if(num == M_WILL_BE_LOGGED+1 && mTarget) {
            player->print("Error: you cannot set/clear this flag.\n");
            return(PROMPT);
        }



        if(target->flagIsSet(num - 1)) {
            target->clearFlag(num - 1);
            player->print("%M's flag #%d(%s) off.\n", target.get(), num, (mTarget ? gConfig->getMFlag(num-1):gConfig->getPFlag(num-1)).c_str());
            i=0;
        } else {
            target->setFlag(num - 1);
            player->print("%M's flag #%d(%s) on.\n", target.get(), num, (mTarget ? gConfig->getMFlag(num-1):gConfig->getPFlag(num-1)).c_str());
            i=1;

            if(pTarget && num == P_MXP_ENABLED+1) {
                pTarget->getSock()->defineMxp();
            }
        }

        if(pTarget && (num == P_BUILDER_MOBS+1 || num == P_BUILDER_OBJS+1))
            pTarget->save(true);

        log_immort(true, player, "%s set %s %s's flag %d(%s) %s.\n",
            player->getCName(), PLYCRT(target), target->getCName(), num,
            (mTarget ? gConfig->getMFlag(num-1):gConfig->getPFlag(num-1)).c_str(), (i ? "On" : "Off"));
        break;
    case 'g':
        if(!strcmp(cmnd->str[3], "guild") || !strcmp(cmnd->str[3], "gu")) {
            if(!player->isDm()) {
                player->print("Error: you cannot set guilds.\n");
                return(0);
            }
            if(!pTarget) {
                player->print("Error: guilds cannot be set on mobs.\n");
                return(0);
            }
            pTarget->setGuild((int)cmnd->val[3]);
            player->print("Guild set.\n");
            log_immort(true, player, "%s set %s %s's guild to %d.\n",
                    player->getCName(), PLYCRT(pTarget), pTarget->getCName(), pTarget->getGuild());
            break;
        }

        if(!strcmp(cmnd->str[3], "guildrank") || !strcmp(cmnd->str[3], "gr")) {
            if(!player->isDm()) {
                player->print("Error: you cannot set guild rank.\n");
                return(0);
            }
            if(!pTarget) {
                player->print("Error: guild rank cannot be set on mobs.\n");
                return(0);
            }
            pTarget->setGuildRank((int)cmnd->val[3]);
            player->print("Guild Rank set.\n");
            log_immort(true, player, "%s set %s %s's guild rank to %d.\n",
                player->getCName(), PLYCRT(pTarget), pTarget->getCName(), pTarget->getGuildRank());
            break;
        }

        target->coins.set(cmnd->val[3], GOLD);

        player->print("%M has %ld gold.\n", target.get(), target->coins[GOLD]);
        log_immort(true, player, "%s set %s %s's gold to %ld.\n",
            player->getCName(), PLYCRT(target), target->getCName(), target->coins[GOLD]);
        break;
    case 'h':
        if(cmnd->str[3][1] == 'm') {

            target->hp.setMax( std::max(1,std::min(30000,(int)cmnd->val[3])));
            player->print("Max Hit Points set.\n");
            log_immort(true, player, "%s set %s %s's Max Hit Points to %d.\n",
                player->getCName(), PLYCRT(target), target->getCName(), target->hp.getMax());
        } else {

            target->hp.setCur((int)cmnd->val[3]);
            player->print("Current Hitpoints set.\n");
            log_immort(true, player, "%s set %s %s's Current Hit Points to %d.\n",
                player->getCName(), PLYCRT(target), target->getCName(), target->hp.getCur());
        }
        break;
    case 'i':
        if(!strcmp(cmnd->str[3], "int")) {
            target->intelligence.setMax(std::max(1, std::min<int>(cmnd->val[3], MAX_STAT_NUM)));
            target->intelligence.restore();
            player->print("Intelligence set.\n");
            log_immort(true, player, "%s set %s %s's Intelligence to %d.\n",
                player->getCName(), PLYCRT(target), target->getCName(), target->intelligence.getCur());
            break;
        }

        if(!strcmp(cmnd->str[3], "inum") && mTarget) {
            int inv = toNum<int>(getFullstrText(cmnd->fullstr, 4));
            int numTrade = toNum<int>(getFullstrText(cmnd->fullstr, 5));

            if(inv > 10 || inv < 1) {
                player->print("Carry slot number invalid.\n");
                player->print("Must be from 1 to 10.\n");
                return(0);
            }

            if(numTrade < 0)
                numTrade = 0;

            mTarget->carry[inv-1].numTrade = numTrade;
            if(numTrade) {
                player->print("Carry slot %d numTrade set to %d.\n", inv, numTrade);
                log_immort(true, player, "%s set %s %s's carry slot %d numTrade to %d.\n",
                    player->getCName(), PLYCRT(mTarget), mTarget->getCName(), inv, numTrade);
            } else {
                player->print("Carry slot %d numTrade cleared.\n", inv);
                log_immort(true, player, "%s cleared %s %s's carry slot %d numTrade.\n",
                    player->getCName(), PLYCRT(mTarget), mTarget->getCName(), inv);
            }
            break;
        }

        if(!strcmp(cmnd->str[3], "inv") && mTarget) {

            // the parser does a terrible job here, so we have to manually figure it out
            std::string txt = getFullstrText(cmnd->fullstr, 4);
            int inv=0;
            char action=0;

            if(!txt.empty()) {
               //inv = cmnd->val[3];
                boost::trim(txt);
                //inv = toNum<int>(txt);
                intText = std::regex_replace(txt, std::regex(R"([\D])"), "");
                objText = std::regex_replace(txt, std::regex(R"([0-9])"), "");
                objText.erase(remove_if(objText.begin(), objText.end(), isspace), objText.end());
                inv = toNum<int>(intText);


                if(!inv) {
                    action = txt.at(0);
                } else {
                    txt = getFullstrText(cmnd->fullstr, 5);
                    if(!txt.empty())
                        action = txt.at(0);
                }
            }

            // both mean delete
            if(action == '0')
                action = 'd';

            if((inv > 10 || inv <= 0) && action != 'd') {
                player->print("Carry slot number invalid.\n");
                player->print("Must be from 1 to 10.\n");
                return(0);
            }
            if(inv && action == 'd') {
                mTarget->carry[inv-1].info.id = 0;
                mTarget->carry[inv-1].numTrade = 0;

                player->print("Carry slot %d cleared.\n", cmnd->val[3]);

                log_immort(true, player, "%s cleared %s's carry slot %ld.\n",
                    player->getCName(), mTarget->getCName(), cmnd->val[3]);
                break;
            }

            if(action == 'd') {
                for(i=0; i<10; i++) {
                    mTarget->carry[i].info.id = 0;
                    mTarget->carry[i].numTrade = 0;
                }
                mTarget->clearMobInventory();

                player->print("Inventory cleared.\n");
                log_immort(true, player, "%s cleared %s's inventory.\n",
                    player->getCName(), mTarget->getCName());
                break;
            }
            object = player->findObject(player, objText, 1);
            if(!object) {
                player->print("You are not carrying that.\n");
                return(0);
            }

            mTarget->carry[inv-1].info = object->info;
            player->print("Carry slot %d set to object %s.\n", inv, object->info.displayStr().c_str());
            log_immort(true, player, "%s set %s %s's carry slot %d to %s.\n",
                player->getCName(), PLYCRT(mTarget), mTarget->getCName(), inv, object->info.displayStr().c_str());
            break;
        }
    case 'j':
        if(!mTarget) {
            player->print("Error: jail room cannot be set on players.\n");
            return(0);
        }

        getCatRef(getFullstrText(cmnd->fullstr, 4), mTarget->jail, player);

        player->print("Jail room set to %s.\n", mTarget->jail.displayStr().c_str());
        log_immort(true, player, "%s set %s %s's jail room to %s.\n",
            player->getCName(), PLYCRT(mTarget), mTarget->getCName(), mTarget->jail.displayStr().c_str());
        break;
    case 'l':
        //target->getLevel() = std::max(1, std::min(cmnd->val[3], 30));
        num = cmnd->val[3];

        if(!strcmp(cmnd->str[3], "lc")) {
            if(num < 1 || num > LANGUAGE_COUNT) {
                player->print("Error: current language out of range.\n");
                return(0);
            }
            player->print("Current language set.\n");
            target->current_language = num-1;
            log_immort(true, player, "%s set %s %s's current language to %d(%s).\n",
                player->getCName(), PLYCRT(target), target->getCName(), target->current_language+1,
                get_language_adj(target->current_language));

        } else if(!strcmp(cmnd->str[3], "lang")) {

            if(num < 1 || num > LANGUAGE_COUNT) {
                player->print("Error: language out of range.\n");
                return(0);
            }
            player->print("Language set.\n");

            log_immort(true, player, "%s turned %s %s's language #%d(%s).\n",
                player->getCName(), (target->languageIsKnown(LUNKNOWN+(num-1)) ? "off":"on"),
                target->getCName(), num, get_language_adj(num-1));

            if(target->languageIsKnown(LUNKNOWN+(num-1)))
                target->forgetLanguage(LUNKNOWN+num-1);
            else
                target->learnLanguage(LUNKNOWN+num-1);

        } else if(!strcmp(cmnd->str[3], "lore")) {

            if(!pTarget) {
                player->print("Lore may only be plyReset on players.\n");
                return(0);
            }
            if(ctModBuilder) {
                player->print("Error: you cannot plyReset lore on builders.\n");
                return(0);
            }

            player->printColor("Before: ^c%d  ", pTarget->lore.size());
            Lore::reset(pTarget);
            player->printColor("After: ^c%d\n", pTarget->lore.size());
            player->print("%s's lore count has been plyReset.\n", pTarget->getCName());

        } else {

            target->setLevel(num, player->isDm());
            player->print("Level set.\n");
            log_immort(true, player, "%s set %s %s's %s to %d.\n",
                player->getCName(), PLYCRT(target), target->getCName(),
                "Level", target->getLevel());

        }

        break;
    case 'm':
        if(cmnd->str[3][1] == 'm') {

            target->mp.setMax(std::max(0,std::min(30000,(int)cmnd->val[3])), true);
            player->print("Max Magic Points set.\n");
            log_immort(true, player, "%s set %s %s's %s to %d.\n",
                player->getCName(), PLYCRT(target), target->getCName(),
                "Magic Points", target->mp.getMax());

        } else if(cmnd->str[3][1] == 'l' && mTarget) {
            mTarget->setMaxLevel((int)cmnd->val[3]);
            if(!mTarget->getMaxLevel())
                mTarget->clearFlag(M_LEVEL_RESTRICTED);
            else
                mTarget->setFlag(M_LEVEL_RESTRICTED);
            player->print("Max Killable Level set.\n");
            log_immort(true, player, "%s set %s %s's %s to %ld.\n",
                player->getCName(), PLYCRT(mTarget), mTarget->getCName(), "Max Killable Level",
                mTarget->getMaxLevel());

        } else if(cmnd->str[3][1] == 'r') {
            if(!mTarget) {
                player->print("Error: magic resistance cannot be set on player.\n");
                return(0);
            }

            mTarget->setMagicResistance((int)cmnd->val[3]);

            player->print("Magic resistance set.\n");
            log_immort(true, player, "%s set %s %s's %s to %d.\n",
                player->getCName(), PLYCRT(mTarget), mTarget->getCName(),
                "magic resistance", mTarget->getMagicResistance());
        } else {

            target->mp.setCur(std::max(0,std::min(30000,(int)cmnd->val[3])));
            player->print("Current Magic Points set.\n");
            log_immort(true, player, "%s set %s %s's %s to %d.\n",
                player->getCName(), PLYCRT(target), target->getCName(),
                "Current Magic Points", target->mp.getCur());
        }
        break;
    case 'n':
        if(!strcmp(cmnd->str[3], "nl")) {
            if(!pTarget) {
                player->print("Error: negative levels cannot be set on mobs.\n");
                return(0);
            }


            pTarget->setNegativeLevels((short)cmnd->val[3]);
            player->print("%s's negative levels set to %d.\n", pTarget->getCName(), pTarget->getNegativeLevels());
            log_immort(true, player, "%s set %s %s's %s to #%d.\n",
                player->getCName(), "Player", pTarget->getCName(), "Negative Levels",
                pTarget->getNegativeLevels());
            break;
        }

        if(mTarget) {
            mTarget->setNumWander((short)cmnd->val[3]);
            player->print("Num wander set.\n");
            log_immort(true, player, "%s set %s %s's %s to %d.\n",
                player->getCName(), PLYCRT(mTarget), mTarget->getCName(),
                "Num Wander", mTarget->getNumWander());
        }
        break;
    case 'p':
        num = strlen(cmnd->str[3]);
        if(!strcmp(cmnd->str[3], "pf") || (num >= 3 && !strncmp(cmnd->str[3], "primefaction", num))) {
            if(!mTarget) {
                player->print("Error: prime faction cannot be set on players.\n");
                return(0);
            }
            if(cmnd->num < 4) {
                player->print("Set faction to what?\n");
                return(0);
            } else if(cmnd->num == 4) {
                mTarget->setPrimeFaction("");
                player->print("%s's prime faction is cleared.\n", target->getCName());
                log_immort(true, player, "%s cleared %s %s's %s.\n",
                    player->getCName(), "Monster", mTarget->getCName(), "Primary Faction");
                return(0);
            }
            const Faction* faction = gConfig->getFaction(cmnd->str[4]);
            if(!faction) {
                player->print("'%s' is an invalid faction.\n", cmnd->str[4]);
                return(0);
            }
            mTarget->setPrimeFaction(faction->getName());
            player->print("%s is now set to prime faction %s(%s).\n", target->getCName(), faction->getName().c_str(), faction->getDisplayName().c_str());
            log_immort(true, player, "%s set %s %s's %s to %s(%s).\n",
                player->getCName(), "Monster", mTarget->getCName(),
                "Primary Faction", faction->getName().c_str(), faction->getDisplayName().c_str());
            break;
        }

        if(!strcmp(cmnd->str[3], "pdur")) {

            if(!mTarget) {
                player->print("Error: Poison duration cannot be set on players.\n");
                return(0);
            }
            mTarget->setPoisonDamage(std::max(1, std::min<int>(cmnd->val[3], 1200)));
            player->print("Poison duration set to %d seconds.\n", mTarget->getPoisonDuration());
            log_immort(true, player, "%s set %s %s's %s to %d.\n",
                player->getCName(), PLYCRT(target), target->getCName(),
                "Poison duration", mTarget->getPoisonDuration());
            break;
        }
        if(!strcmp(cmnd->str[3], "pdmg")) {

            if(!mTarget) {
                player->print("Error: Poison damage/tick cannot be set on players.\n");
                return(0);
            }
            mTarget->setPoisonDamage(std::max<short>(1, std::min<short>(cmnd->val[3], 500)));
            player->print("Poison damage set to %d per tick.\n", mTarget->getPoisonDamage());
            log_immort(true, player, "%s set %s %s's %s to %d.\n",
                player->getCName(), PLYCRT(target), mTarget->getCName(),
                "Poison damage/tick", mTarget->getPoisonDamage());
            break;
        }
        if(!strcmp(cmnd->str[3], "pty")) {

        target->piety.setMax(std::max(1, std::min<int>(cmnd->val[3], MAX_STAT_NUM)));
        target->piety.restore();
        player->print("Piety set.\n");
        log_immort(true, player, "%s set %s %s's %s to %d.\n",
            player->getCName(), PLYCRT(target), target->getCName(),
            "Peity", target->piety.getCur());
        break;
        }
        
        /*
        if(!strcmp(cmnd->str[3], "pkin") && player->isDm()) {
            if(!pTarget) {
                player->print("Error: PKIN cannot be set on mobs.\n");
                return(0);
            }

            player->print("Pkills In set.\n");
            pTarget->setPkin((short)cmnd->val[3]);

            break;
        }
        if(!strcmp(cmnd->str[3], "pkwon") && player->isDm()) {
            if(!pTarget) {
                player->print("Error: PKWON cannot be set on mobs.\n");
                return(0);
            }

            player->print("Pkills Won set.\n");
            pTarget->setPkwon((short)cmnd->val[3]);
            break;
        }

        num = atoi(&cmnd->str[3][1]);
        if(num < 0 || num > 5) {
            player->print("Error: proficiency out of range.\n");
            return(0);
        }


        target->proficiency[num] = std::max(0, std::min(cmnd->val[3], 10000000));
        player->print("%M given %d shots in prof#%d.\n", target.get(),
            target->proficiency[num], num);
        log_immort(true, player, "%s set %s %s's %s%d to %ld.\n",
            player->getCName(), PLYCRT(target), target->getCName(),
            "Prof#", num, target->proficiency[num]);

        break;
         */
    case 'q':
        if(!pTarget) {
            player->print("Error: quests cannot be set on mobs.\n");
            return(0);
        }

        num = cmnd->val[3];
        if(num < 1 || num > MAX_QUEST) {
            player->print("Error: quest out of range.\n");
            return(0);
        }
        if(pTarget->questIsSet(num - 1)) {
            pTarget->clearQuest(num - 1);
            player->print("%M's quest #%d - (%s) off.\n", pTarget.get(), num, get_quest_name(num-1));
            i=0;
        } else {
            pTarget->setQuest(num - 1);
            player->print("%M's quest #%d - (%s) on.\n", pTarget.get(), num, get_quest_name(num-1));
            i=1;
        }
        log_immort(true, player, "%s turned %s %s's %s %d - (%s) %s.\n",
            player->getCName(), PLYCRT(pTarget), pTarget->getCName(), "Quest",
            num, get_quest_name(num-1), i ? "On" : "Off");

        break;
    case 'r':
        if(!strcmp(cmnd->str[3], "rag")) {
            if(!mTarget) {
                player->print("Error: race aggro cannot be set on players.\n");
                return(0);
            }
            num = cmnd->val[3];

            if(!strcmp(cmnd->str[4], "del")) {
                for(a=1; a<RACE_COUNT-1; a++)
                    mTarget->clearRaceAggro(a);
                player->print("All %s's race aggros now cleared.\n");
                return(0);
            }

            if(num < 1 || num >= RACE_COUNT) {
                player->print("Error: race aggro out of range.\n");
                return(0);
            }

            if(mTarget->isRaceAggro(num, false)) {
                player->print("%M will no longer attack %s's.\n", mTarget.get(), gConfig->getRace(num)->getName().c_str());
                mTarget->clearRaceAggro(num);
                log_immort(true, player, "%s set creature %s to NOT aggro %ss(%d).\n",
                    player->getCName(), mTarget->getCName(), gConfig->getRace(num)->getName().c_str(), num);
            } else {
                mTarget->setRaceAggro(num);
                player->print("%M will now attack %s's.\n", mTarget.get(), gConfig->getRace(num)->getName().c_str());
                log_immort(true, player, "%s set creature %s to aggro %ss(%d).\n",
                    player->getCName(), mTarget->getCName(), gConfig->getRace(num)->getName().c_str(), num);
            }
            break;
        }
        switch(cmnd->str[3][1]) {
        case 'a':

            if(isdigit(cmnd->str[3][2])) {

                if(!pTarget) {
                    player->print("Error: room boundaries cannot be set on mobs.\n");
                    return(0);
                }
                if(!pTarget->isStaff()) {
                    player->print("Error: room boundaries can only be set on staff.\n");
                    return(0);
                }

                rnum = std::max(0, std::min(toNum<int>(&cmnd->str[3][2]), MAX_BUILDER_RANGE));
                pTarget->bRange[rnum-1].low.setArea(cmnd->str[4]);

                player->print("%s's range #%d area set to %s.\n", pTarget->getCName(), rnum,
                    pTarget->bRange[rnum-1].low.area.c_str());
                log_immort(true, player, "%s set %s's range #%d area to %s.\n",
                    player->getCName(), pTarget->getCName(), rnum, pTarget->bRange[rnum-1].low.area.c_str());

            } else {

                target->setRace((short)cmnd->val[3]);
                player->print("Race set to %s(%d).\n", target->getRace() ? gConfig->getRace(target->getRace())->getName().c_str() : "", target->getRace());
                if(target->getRace()) {
                    target->setSize(gConfig->getRace(target->getRace())->getSize());
                    player->print("Size automatically set to %s.\n", getSizeName(target->getSize()).c_str());
                }
                log_immort(true, player, "%s set %s %s's %s to %s(%d).\n",
                    player->getCName(), PLYCRT(target), target->getCName(),
                    "Race", target->getRace() ? gConfig->getRace(target->getRace())->getName().c_str() : "", target->getRace());
            }
            break;

        case 'e':
            if(!mTarget) {
                player->print("Error: rescue monsters cannot be set on players.\n");
                return(0);
            }

            rnum = std::max(1, std::min(toNum<int>(&cmnd->str[3][strlen(cmnd->str[3])-1]), NUM_RESCUE))-1;
            getCatRef(getFullstrText(cmnd->fullstr, 4), mTarget->rescue[rnum], mTarget);

            player->print("%s's rescue mob #%d set to %s.\n", mTarget->getCName(), rnum+1,
                          mTarget->rescue[rnum].displayStr().c_str());
            log_immort(true, player, "%s set %s's rescue mob #%d to %s.\n",
                player->getCName(), mTarget->getCName(), rnum+1, mTarget->rescue[rnum].displayStr().c_str());

            break;
            /*
        case 'e':
            if(!strcmp(cmnd->str[3], "reg") && target->isMonster())
            {

                if(!target->flagIsSet(M_REGENERATES)) {
                    player->print("You must set mflag#%d(regenerates) first.\n", M_REGENERATES+1);
                    return(0);
                }
                target->regenRate = std::max(1,std::min(cmnd->val[3], 30));
                player->print("Regeneration rate set.\n");
                log_immort(true, player, "%s set %s regeneration rate to %d seconds.\n", player->getCName(), target->getCName(), target->regenRate);
            }
            break;
            */
        case 'l':

            if(!pTarget) {
                player->print("Error: room boundaries cannot be set on mobs.\n");
                return(0);
            }
            if(!pTarget->isStaff()) {
                player->print("Error: room boundaries can only be set on staff.\n");
                return(0);
            }

            rnum = std::max(0, std::min(toNum<int>(&cmnd->str[3][2]), MAX_BUILDER_RANGE));
            pTarget->bRange[rnum-1].low.id = std::max(-1, std::min(RMAX, (int)cmnd->val[3]));

            player->print("%s's low range #%d set to %d.\n", pTarget->getCName(), rnum,
                pTarget->bRange[rnum-1].low.id);
            log_immort(true, player, "%s set %s's low range #%d to %d.\n",
                player->getCName(), pTarget->getCName(), rnum, pTarget->bRange[rnum-1].low.id);
            break;
        case 'h':

            if(!pTarget) {
                player->print("Error: room boundaries cannot be set on mobs.\n");
                return(0);
            }
            if(!pTarget->isStaff()) {
                player->print("Error: room boundaries can only be set on staff.\n");
                return(0);
            }

            rnum = std::min(MAX_BUILDER_RANGE, std::max(toNum<int>(&cmnd->str[3][2]),0));
            pTarget->bRange[rnum-1].high = std::max(-1, std::min(RMAX, (int)cmnd->val[3]));

            player->print("%s's high range #%d set to %d.\n", pTarget->getCName(), rnum,
                pTarget->bRange[rnum-1].high);
            log_immort(true, player, "%s set %s's high range #%d to %d.\n",
                player->getCName(), pTarget->getCName(), rnum, pTarget->bRange[rnum-1].high);
            break;
        default:

            rnum = toNum<int>(&cmnd->str[3][1]);

            if (rnum < MIN_REALM || rnum > MAX_REALM-1) {
                *player << ColorOn << "^gRealm # should be a numeric value between " << MIN_REALM << " and " << MAX_REALM-1 << ".\n" << ColorOff;
                return(0);
            }
            Realm r = (Realm)std::max((int)MIN_REALM, std::min((int)MAX_REALM-1, rnum));

            target->setRealm(cmnd->val[3], r);

            *player << ColorOn << "^y" << setf(CAP) << target << "'s Realm #" << rnum << " (" << getRealmSpellName(r) << 
                                ") now set to " << target->getRealm(r) << " [" << mprofic(target, rnum) << "%].\n" << ColorOff;


            log_immort(true, player, "%s set %s %s's %s%d(%s) to %ld[%d%] in room %s\n",
                player->getCName(), PLYCRT(target), target->getCName(),"Realm#",
                rnum, getRealmSpellName(r).c_str(), target->getRealm(r),mprofic(target, rnum), target->getRoomParent()->fullName().c_str());
            break;
        }
        break;
    case 's':
        switch(cmnd->str[3][1]) {
        case 'i':
            target->setSize(getSize(cmnd->str[4]));
            player->print("%s's %s set to %s.\n", target->getCName(), "Size", getSizeName(target->getSize()).c_str());
            log_immort(true, player, "%s set %s %s(%s) %s to %s.\n",
                player->getCName(), PLYCRT(target), target->getCName(), "Size", getSizeName(target->getSize()).c_str());
            return(0);

            break;
        case 'k':
        {
            if(cmnd->num < 5) {
                player->print("Set what skill to what?\n");
                return(0);
            }
            if((int)cmnd->val[4] > MAXALVL*10 || (int)cmnd->val[4] < -1) {
                player->print("Skill range is 0 - %d, -1 to delete.\n", MAXALVL*10);
                return(0);
            }
            //std::string skillStr = cmnd->str[4];
            if(cmnd->val[4] == -1) {
                if(!target->knowsSkill(cmnd->str[4])) {
                    player->print("%s doesn't know that skill.\n", target->getCName());
                    return(0);
                }

                target->remSkill(cmnd->str[4]);

                player->print("%s's skill %s has been removed.\n", target->getCName(), cmnd->str[4]);
                log_immort(true, player, "%s removed %s %s's skill %s.\n",
                        player->getCName(), PLYCRT(target), target->getCName(), cmnd->str[4]);

            } else if(!target->setSkill(cmnd->str[4], cmnd->val[4])) {
                player->print("'%s' is not a valid skill.\n", cmnd->str[4]);
                return(0);
            } else {

                player->print("%s's skill %s set to %d.\n", target->getCName(), cmnd->str[4], cmnd->val[4]);
                log_immort(true, player, "%s set %s %s's skill %s to %d.\n",
                        player->getCName(), PLYCRT(target), target->getCName(), cmnd->str[4], (short)cmnd->val[4]);

            }
            break;
        }
        case 'l':
            if(!mTarget) {
                player->print("Error: skillLevel cannot be set on players.\n");
                return(0);
            }
            mTarget->setSkillLevel((int)cmnd->val[3]);
            player->print("Monster trade skill level set.\n");
            log_immort(true, player, "%s set %s %s's %s to %d(%s).\n",
                player->getCName(), PLYCRT(mTarget), mTarget->getCName(), "trade skill level",
                mTarget->getSkillLevel(), get_skill_string(mTarget->getSkillLevel() / 10));
            break;
        case 'o':
            if(!pTarget) {
                player->print("Error: songs cannot be set on mobs.\n");
                return(0);
            }

            num = cmnd->val[3];
            if(num < 1 || num > gConfig->getMaxSong()) {
                player->print("Error: song out of range.\n");
                return(0);
            }

            if(!pTarget->songIsKnown(num - 1)) {
                pTarget->learnSong(num - 1);
                player->print("Song #%d - (Song of %s) on.\n", num, get_song_name(num-1));
                test=1;
            } else {
                pTarget->forgetSong(num - 1);
                player->print("Song #%d - (Song of %s) off.\n", num, get_song_name(num-1));
                test=0;
            }
            log_immort(true, player, "%s turned %s %s's %s %d - (Song of %s) %s.\n",
                player->getCName(), PLYCRT(pTarget), pTarget->getCName(), "Song",
                num, get_song_name(num-1), test == 1 ? "On" : "Off");
            break;
        case 'p':
            if(cmnd->str[4][0] == 'd') {
                for(sp=0; sp < MAXSPELL; sp++) {
                    target->forgetSpell(sp);
                }
                player->print("%M's spells deleted.\n", target.get());
                break;
            }

            if(cmnd->str[4][0] == 'a') {
                for(sp=0; sp < MAXSPELL; sp++) {
                    target->learnSpell(sp);
                }
                player->print("%M's spells all set.\n", target.get());
                break;
            }

            num = cmnd->val[3];
            if(num < 1 || num > MAXSPELL) {
                player->print("Error: spell out of range.\n");
                return(0);
            }

            if(!target->spellIsKnown(cmnd->val[3] - 1)) {
                target->learnSpell(cmnd->val[3] - 1);
                player->print("Spell #%d - (%s) on.\n", num, get_spell_name(num-1));
                test=1;
            } else {
                target->forgetSpell(cmnd->val[3] - 1);
                player->print("Spell #%d - (%s) off.\n", num, get_spell_name(num-1));
                test=0;
            }
            log_immort(true, player, "%s turned %s %s's %s %d - (%s) %s.\n",
                player->getCName(), PLYCRT(target), target->getCName(), "Spell",
                num, get_spell_name(num-1), test == 1 ? "On" : "Off");


            break;

        case 't':
            target->strength.setMax(std::max(1, std::min((int)cmnd->val[3], MAX_STAT_NUM)));
            target->strength.restore();
            player->print("Strength set.\n");
            log_immort(true, player, "%s set %s %s's %s to %d.\n",
                player->getCName(), PLYCRT(target), target->getCName(),
                "Strength", target->strength.getCur());
            break;
        default:
            if(!strcmp(cmnd->str[4], "d")) {
                target->saves[POI].chance = 0;
                target->saves[POI].gained = 0;
                target->saves[DEA].chance = 0;
                target->saves[DEA].gained = 0;
                target->saves[BRE].chance = 0;
                target->saves[BRE].gained = 0;
                target->saves[MEN].chance = 0;
                target->saves[MEN].gained = 0;
                target->saves[SPL].chance = 0;
                target->saves[SPL].gained = 0;
                player->print("%s's saves plyReset to 0.\n", target->getCName());

                break;
            } else {

                num = toNum<int>(&cmnd->str[3][1]);
                if(num < 0 || num > 5) {
                    player->print("Error: save out of range.\n");
                    return(0);
                }

                target->saves[num].chance = std::max(1, std::min((int)cmnd->val[3], 99));
                player->print("%M now has %d%% chance for save #%d.\n", target.get(),
                    target->saves[num].chance, num);
                log_immort(true, player, "%s set %s %s's %s%d to %d.\n",
                    player->getCName(), PLYCRT(target), target->getCName(),
                    "Save#", num, target->saves[num].chance);

            }
        }
        break;
    case 't':
        switch(low(cmnd->str[3][1])) {
        case 'r':
            if(!mTarget) {
                player->print("Error: trade cannot be set on players.\n");
                return(0);
            }

            mTarget->setMobTrade((int)cmnd->val[3]);
            player->print("Trade set to %s.\n", mTarget->getMobTradeName().c_str());
            log_immort(true, player, "%s set %s %s's %s to %d(%s).\n",
                player->getCName(), PLYCRT(target), target->getCName(),
                "NPC trade", mTarget->getMobTrade(), mTarget->getMobTradeName().c_str());


            break;
            /*case 't':
                if(target->isPlayer())
                    return(PROMPT);
                target->ttype = std::max(0,std::min(MAX_TTYPE,(int)cmnd->val[3]));
                player->print("Treasure type set.\n");
                log_immort(true, player, "%s set %s %s's %s to %d.\n",
                    player->getCName(),
                    PLYCRT(target),
                    target->getCName(),
                    "Treasure type",
                    target->thaco);


            break; */
        case 'y':
            if(!mTarget) {
                player->print("Error: mob type cannot be set on players.\n");
                return(0);
            }

            mTarget->setType((int)cmnd->val[3]);

            if(mTarget->flagIsSet(M_CUSTOM)) {
                player->print("CUSTOM mob flag cleared.\n");
                mTarget->clearFlag(M_CUSTOM);
            }

            player->print("Monster set to type %s.\n", monType::getName(mTarget->getType()));
            log_immort(true, player, "%s set %s %s's %s to %s.\n",
                player->getCName(), PLYCRT(mTarget), mTarget->getCName(),
                "type", monType::getName(mTarget->getType()));

            if(mTarget->getSize() == NO_SIZE) {
                mTarget->setSize(monType::size(mTarget->getType()));
                if(mTarget->getSize() == NO_SIZE)
                    player->print("Size automatically set to %s.\n", getSizeName(mTarget->getSize()).c_str());
            }

            if(!mTarget->flagIsSet(M_CUSTOM))
                mTarget->adjust(0);
            player->print("%s adjusted to standard \"%s\" stats.\n", mTarget->getCName(), monType::getName(mTarget->getType()));
            break;
        }
        break;
    case 'w':
        if(mTarget) {
            num = (int)cmnd->val[3];
            num = std::max(0, std::min(MAXALVL*10, num));

            mTarget->setWeaponSkill(num);
            player->print("Weapon Skill set to %d.\n", mTarget->getWeaponSkill());
            break;
        }
        default:
            player->print("Invalid option.\n");
            return(0);
    }


    // Clear index when updating a monster
    // Set logged flags and save full flags
    if(mTarget) {
        mTarget->info.id = 0;
        mTarget->setFlag(M_WILL_BE_LOGGED);
        mTarget->setFlag(M_SAVE_FULL);

        strcpy(mTarget->last_mod, player->getCName());
        mTarget->escapeText();
    }

    return(0);
}


//*********************************************************************
//                      dmCrtName
//*********************************************************************
// the dmObjName command allows a dm/ caretaker to modify an already
// existing object's name, description, wield description, and key
// words. This command does not save the changes to the object to the
// object data base.  This command is intended for adding personalize
// weapons and objects to the game

int dmCrtName(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Monster> target=nullptr;
    int     i=0, num=0;
    char    modstr[32];
    char    which=0;
    std::string text = "";
    std::string numStr = "";
    strcpy(modstr, "");

    if(cmnd->num < 2) {
        *player << "\nRename what creature to what?\n";
        *player << "*cname <creature> [#] [-A|a|d|t|m|k|tt] <name>\n";
        *player << "-A aggro string\n-a[1-3] attack strings\n-d description\n-t talk string\n-m[1-3] movement strings\n-k[1-3] key strings\n-tt after trade talk string\n";
        return(PROMPT);
    }

    // parse the full command string for the start of the description
    // (pass  command, object, obj #, and possible flag).   The object
    // number has to be interpreted separately, ad with the normal
    // command parse (cmnd->val), due to problems caused having
    // the object number followed by a "-"

    while(isspace(cmnd->fullstr[i]))
        i++;
    while(!isspace(cmnd->fullstr[i]))
        i++;
    while(isspace(cmnd->fullstr[i]))
        i++;
    while(isalpha(cmnd->fullstr[i]))
        i++;
    while(isspace(cmnd->fullstr[i]))
        i++;

    cmnd->val[1]= 1;
    if(isdigit(cmnd->fullstr[i]))
        cmnd->val[1] = toNum<long>(&cmnd->fullstr[i]);

    target = player->getParent()->findMonster(player, cmnd);
    if(!target) {
        *player << "Monster not found in the room.\n";
        return(PROMPT);
    }

    if(target->info.id && !player->checkBuilder(target->info))
        return(0);
    if(!player->builderCanEditRoom("use *cname"))
        return(0);

    while(isdigit(cmnd->fullstr[i]))
        i++;
    while(isspace(cmnd->fullstr[i]))
        i++;

    // parse flag
    if(cmnd->fullstr[i] == '-') {
        if(cmnd->fullstr[i+1] == 'd') {
            which =1;
            i += 2;
        } else if(cmnd->fullstr[i+1] == 't' && cmnd->fullstr[i+2] == 't') {
            which = 7;
            i += 3;
        } else if(cmnd->fullstr[i+1] == 't') {
            which = 2;
            i += 2;
        } else if(cmnd->fullstr[i+1] == 'A') {
            which = 4;
            i += 2;
        } else if(cmnd->fullstr[i+1] == 'a') {
            i += 2;
            which = 5;
            numStr = cmnd->fullstr[i];
            boost::trim(numStr);
            num = toNum<int>(numStr);
            if(num <1 || num > 3)
                num = 0;
            while(isdigit(cmnd->fullstr[i]))
                i++;
        } else if(cmnd->fullstr[i+1] == 'k') {
            i += 2;
            which = 3;
            numStr = cmnd->fullstr[i];
            boost::trim(numStr);
            num = toNum<int>(numStr);
            if(num <1 || num > 3)
                num = 0;
            while(isdigit(cmnd->fullstr[i]))
                i++;
        } else if(cmnd->fullstr[i+1] == 'm') {
            i += 2;
            which = 6;
            numStr = cmnd->fullstr[i];
            boost::trim(numStr);
            num = toNum<int>(numStr);
            if(num <1 || num > 3)
                num = 0;
            while(isdigit(cmnd->fullstr[i]))
                i++;
        }
        while(isspace(cmnd->fullstr[i]))
            i++;
    }

    // no description given
    if(cmnd->fullstr[i] == 0)
        return(PROMPT);

    text = &cmnd->fullstr[i];
    // handle object modification

    switch (which) {
    case 0:
        if(text.length() > 79)
            text = text.substr(0, 79);
        if(Pueblo::is(text)) {
            *player << "That monster name is not allowed.\n";
            return(0);
        }
        target->setName( text.c_str());
        *player << "\nName ";
        strcpy(modstr, "name");
        break;
    case 1:
        strcpy(modstr, "description");
        if(text == "0" && target->getDescription().empty()) {
            target->setDescription("");
            *player << "Description cleared.\n";
            return(0);
        } else {
            target->setDescription(text);
            *player << "\nDescription ";
        }
        break;
    case 2:
        strcpy(modstr, "talk string");
        if(text == "0" && !target->getTalk().empty()) {
            target->setTalk("");
            *player << "Talk string cleared.\n";
            return(0);
        } else {
            target->setTalk(text);
            *player << "\nTalk String ";
        }
        break;
    case 3:
        if(num) {
            if(text == "0" && target->key[num-1][0]) {
                zero(target->key[num-1], sizeof(target->key[num-1]));
                *player << "Key #" << num << " string cleared.\n";
                return(0);
            } else {
                if(text.length() > 19)
                    text = text.substr(0, 19);
                strcpy(target->key[num-1], text.c_str());
                *player << "\nKey ";
                strcpy(modstr, "desc key");
            }
        }
        break;
    case 4:
        strcpy(modstr, "aggro string");
        if(text == "0" && target->aggroString[0]) {
            zero(target->aggroString, sizeof(target->aggroString));
            *player << "Aggro string cleared.\n";
            return(0);
        } else {
            if(text.length() > 79)
                text = text.substr(0, 79);
            strcpy(target->aggroString, text.c_str());
            *player << "\nAggro talk string ";
        }
        break;
    case 5:
        if(num) {
            strcpy(modstr, "attack strings");
            if(text == "0" && target->attack[num-1][0]) {
                zero(target->attack[num-1], sizeof(target->attack[num-1]));
                *player << "Attack string " << num << " cleared.\n";
                return(0);
            } else {
                if(text.length() > 29)
                    text = text.substr(0, 29);
                strcpy(target->attack[num-1], text.c_str());
                *player << "Attack type " << num << " ";
            }
        }
        break;
    case 6:
        if(num) {
            strcpy(modstr, "movement strings");
            if(text == "0" && target->movetype[num-1][0]) {
                zero(target->movetype[num-1], sizeof(target->movetype[num-1]));
                *player << "Movement string " << num << " cleared.\n";
                return(0);
            } else {
                if(text.length() > CRT_MOVETYPE_LENGTH-1)
                    text = text.substr(0, CRT_MOVETYPE_LENGTH-1);
                strcpy(target->movetype[num-1], text.c_str());
                *player << "Movement string " << num << " ";
            }
        }
        break;
    case 7:
        strcpy(modstr, "trade talk string");
        if(text == "0" && target->ttalk[0]) {
            zero(target->ttalk, sizeof(target->ttalk));
            *player << "After trade talk string cleared.\n";
            return(0);
        } else {
            if(text.length() > 79)
                text = text.substr(0, 79);
            strcpy(target->ttalk, text.c_str());
            *player << "\nAfter trade talk string ";
        }
        break;
    default:
        *player << "\nNothing ";
    }
    *player << "done.\n";

    log_immort(true, player, "%s modified %s of creature %s(%s).\n",
        player->getCName(), modstr, target->getCName(), target->info.displayStr().c_str());

    target->escapeText();
    strcpy(target->last_mod, player->getCName());
    return(0);
}

//*********************************************************************
//                      dmAlias
//*********************************************************************
//  This function allows staff to become a monster.

int dmAlias(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Monster>  monster = nullptr;

    if(cmnd->num < 2) {
        player->print("Syntax: *possess <creature>\n");
        return(0);
    }

    monster = player->getParent()->findMonster(player, cmnd);
    if(!monster) {
        player->print("Can't seem to locate that monster here.\n");
        return(0);
    }

    if(monster->isPlayer()) {
        player->print("Their soul refuses to be displaced.\n");
        return(0);
    }

    if(monster->mFlagIsSet(M_PERMANENT_MONSTER)) {
        player->print("Their soul refuses to be displaced.\n");
        return(0);
    }
    if(player->flagIsSet(P_ALIASING) && monster != player->getAlias()) {
        player->print("You may only possess one monster at a time.\n");
        return(0);
    }
    if(monster->flagIsSet(M_DM_FOLLOW)) {
        if(monster != player->getAlias()) {
            player->print("Their soul belongs to another.\n");
            return(0);
        }
        monster->clearFlag(M_DM_FOLLOW);
        player->clearFlag(P_ALIASING);
        player->print("You release %1N's body.\n", monster.get());

        log_immort(false,player, "%s no longer possesses %s.\n", player->getCName(), monster->getCName());
        monster->removeFromGroup(false);
        player->setAlias(nullptr);
        return(0);
    }
    player->addPet(monster, false);
    player->setAlias(monster);
    player->setFlag(P_ALIASING);
    monster->setFlag(M_DM_FOLLOW);


    player->print("You possess %1N.\n", monster.get());

    // make sure we are invis at this point
    if(!player->flagIsSet(P_DM_INVIS)) {
        // dmInvis does not use the second param
        dmInvis(player, nullptr);
    }

    log_immort(false,player, "%s possesses %s in room %s.\n", player->getCName(), monster->getCName(),
        player->getRoomParent()->fullName().c_str());

    return(0);
}

//*********************************************************************
//                      dmFollow
//*********************************************************************
// This function allows staff to force a monster to follow
// them, and has been made to allow for the movement of
// custom monsters (made with the dmCrtName function).

int dmFollow(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Monster>  creature=nullptr;

    if(cmnd->num < 2) {
        player->print("syntax: *cfollow <creature>\n");
        return(0);
    }

    creature = player->getParent()->findMonster(player, cmnd);
    if(!creature) {
        player->print("Can't seem to locate that creature here.\n");
        return(0);
    }

    if(creature->flagIsSet(M_PERMANENT_MONSTER)) {
        player->print("Perms can't follow.\n");
        return(0);
    }
    if(creature->getMaster() == player) {
        player->print("%s stops following you.\n", creature->getCName());
        // TODO: fixme?
        player->delPet(creature);
        return(0);
    }
    player->addPet(creature, false);
    player->print("%s starts following you.\n", creature->getCName());
    return(0);
}

//*********************************************************************
//                      dmAttack
//*********************************************************************
//  This function allows staff to make a monster attack a given player.

int dmAttack(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Monster>  attacker=nullptr;
    std::shared_ptr<Creature> victim=nullptr;
    int inroom=1;

    if(!player->checkBuilder(player->getUniqueRoomParent())) {
        player->print("Error: Room number not inside any of your alotted ranges.\n");
        return(0);
    }
    if(!player->builderCanEditRoom("use *attack"))
        return(0);

    if(cmnd->num < 3) {
        player->print("syntax: *attack <monster> <defender>\n");
        return(0);
    }

    attacker = player->getParent()->findMonster(player, cmnd);

    if(!attacker || !player->canSee(attacker)) {
        player->print("Can't seem to locate that attacker here.\n");
        return(0);
    }

    if(attacker->flagIsSet(M_PERMANENT_MONSTER)) {
        player->print("Perms can't do that.\n");
        return(0);
    }


    victim = player->getParent()->findMonster(player, cmnd, 2);

    if(!victim) {
        lowercize(cmnd->str[2], 1);
        victim = gServer->findPlayer(cmnd->str[2]);
        inroom=0;
    }
    if(!victim || !player->canSee(victim)) {
        player->print("Can't seem to locate that victim here.\nPlease use full names.\n");
        return(0);
    }

    if(victim->flagIsSet(M_PERMANENT_MONSTER) && victim->isMonster()) {
        player->print("Perms can't do that.\n");
        return(0);
    }
    player->print("Adding %N to attack list of %N.\n", victim.get(), attacker.get());


    if(!player->isDm())
        log_immort(true, player, "%s forces %s to attack %s.\n", player->getCName(), attacker->getCName(), victim->getCName());

    attacker->addEnemy(victim);

    if(inroom) {
        broadcast(victim->getSock(), victim->getRoomParent(), "%M attacks %N.", attacker.get(), victim.get());
        victim->print("%M attacked you!\n", attacker.get());
    }
    return(0);
}

//*********************************************************************
//                      dmListEnemy
//*********************************************************************
//  This function lists the enemy list of a given monster.

int dmListEnemy(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Monster>  target=nullptr;

    target = player->getParent()->findMonster(player, cmnd);

    if(!target) {
        player->print("Not here.\n");
        return(0);
    }

    std::ostringstream oStr;

    oStr << "Enemy list for " << target->getName() << ": " << std::endl;
    oStr << target->threatTable;

    return(0);
}

//*********************************************************************
//                      dmListCharm
//*********************************************************************
// This function allows staff to see a given players charm list

int dmListCharm(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;

    if(cmnd->num < 2) {
        *player << "See whose charm list?\n";
        return(PROMPT);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);
    if(!target) {
        *player << cmnd->str[1] << " is not on.\n";
        return(0);
    }

    *player << "Charm list for " << target->getName() << ":\n";

    if(target->charms.empty())
        *player << "Nobody.\n";
    else {
        for(const auto &charm : target->charms) {
            *player << "  " << charm << "\n";
        }
    }
    *player << "Afflicted By: " << target->getAfflictedBy() << "\n";
    if(!target->minions.empty()) {
        *player <<"Minions:\n";
        for(const auto& minion : target->minions) {
            *player <<"  " << minion << "\n";
        }
    }
    return(0);
}

//*********************************************************************
//                      dmSaveMob
//*********************************************************************

void dmSaveMob(const std::shared_ptr<Player>& player, cmd* cmnd, const CatRef& cr) {
    std::shared_ptr<Monster> target=nullptr;
    ttag    *tp=nullptr, *tempt=nullptr;
    int     i=0, x=0;

    if(!player->canBuildMonsters()) {
        cmdNoAuth(player);
        return;
    }

    if(cmnd->num < 3) {
        player->print("Syntax: *save c [name] [number]\n");
        return;
    }

    target = player->getParent()->findMonster(player, cmnd->str[2], 1);

    if(!target) {
        player->print("Monster not found.\n");
        return;
    }

    if(!validMobId(cr)) {
        player->print("Index error: monster number invalid.\n");
        return;
    }
    if(!player->checkBuilder(cr, false)) {
        player->print("Error: %s out of your allowed range.\n", cr.displayStr().c_str());
        return;
    }

    log_immort(true, player, "%s saved %s to %s.\n",
        player->getCName(), target->getCName(), cr.displayStr().c_str());

    target->clearMobInventory();
    player->print("Monster inventory cleaned before saving.\n");

    // No longer need to log this monster because he's been saved
    target->clearFlag(M_WILL_BE_LOGGED);
    // Also no longer need to do a full save to the player's inv
    // because he's been updated on disk
    target->clearFlag(M_SAVE_FULL);

    tp = target->first_tlk;
    while(tp) {
        tempt = tp->next_tag;
        delete tp->key;
        delete[] tp->response;
        delete tp->action;
        delete tp->target;
        delete tp;
        tp = tempt;
    }
    target->first_tlk = nullptr;

    if(!target->flagIsSet(M_TRADES)) {
        for(const auto& obj : target->objects ) {
            x = obj->info.id;
            if(!x) {
                player->print("Unique object in inventory not saved.\n");
                continue;
            }
            target->carry[i].info.id = x;
            i++;
            if(i>9) {
                player->print("Only first 10 objects in inventory saved.\n");
                break;
            }
        }
    }

    // clean up possesed before save
    if(target->flagIsSet(M_DM_FOLLOW)) { // clear relevant follow lists
        if(target->getMaster()) {
            std::shared_ptr<Player> master = target->getMaster()->getAsPlayer();

            master->clearFlag(P_ALIASING);

            master->setAlias(nullptr);
            master->print("%1M's soul was saved.\n", target.get());
            master->delPet(target);
        }
        target->clearFlag(M_DM_FOLLOW);

    }

    target->info = cr;

    if(fs::exists(Path::monsterPath(target->info)))
        player->print( "Monster %s might already exist.\n", cr.displayStr().c_str());

    if(target->saveToFile()!= 0) {
        loge("Error saving monster in dmSaveMob()");
        player->print("Error: monster was not saved\n" );
    } else
        player->print("Monster %s updated.\n", cr.displayStr().c_str());

    if(player->flagIsSet(P_NO_FLUSHCRTOBJ))
        gServer->monsterCache.insert(target->info, *target);
}

//*********************************************************************
//                      dmAddMob
//*********************************************************************
// This function creates a generic creature for staff to work on.

int dmAddMob(const std::shared_ptr<Player>& player, cmd* cmnd) {
    int     n;
    long    t = time(nullptr);

    if(!player->canBuildMonsters())
        return(cmdNoAuth(player));
    if(!player->checkBuilder(player->getUniqueRoomParent())) {
        player->print("Error: this room is out of your range; you cannot create a mob here.\n");
        return(0);
    }
    if(!player->builderCanEditRoom("add monsters"))
        return(0);

    std::shared_ptr<Monster> new_mob = std::make_shared<Monster>();
    log_immort(true, player, "%s made a new mob!\n", player->getCName());

    //zero(new_mob, sizeof(Monster));

    new_mob->setName( "clay form");
    strcpy(new_mob->key[0], "form");
    new_mob->setLevel(1);
    new_mob->setType(MONSTER);
    new_mob->strength.setInitial(100);
    new_mob->dexterity.setInitial(100);
    new_mob->constitution.setInitial(100);
    new_mob->intelligence.setInitial(100);
    new_mob->piety.setInitial(100);
    new_mob->hp.setInitial(12);
    new_mob->hp.restore();
    new_mob->setArmor(25);
    new_mob->setDefenseSkill(5);
    new_mob->setWeaponSkill(5);
    new_mob->setExperience(10);
    new_mob->damage.setNumber(1);
    new_mob->damage.setSides(4);
    new_mob->damage.setPlus(1);
    new_mob->first_tlk = nullptr;
    new_mob->setParent(nullptr);

    for(n=0; n<20; n++)
        new_mob->ready[n] = nullptr;
    new_mob->setFlag(M_SAVE_FULL);
    new_mob->lasttime[LT_MON_SCAVENGE].ltime =
    new_mob->lasttime[LT_MON_WANDER].ltime =
    new_mob->lasttime[LT_MOB_THIEF].ltime =
    new_mob->lasttime[LT_TICK].ltime =
    new_mob->lasttime[LT_TICK_SECONDARY].ltime =
    new_mob->lasttime[LT_TICK_HARMFUL].ltime = t;
    new_mob->addToRoom(player->getRoomParent());


    player->print("Monster created.\n");
    return(0);
}



//*********************************************************************
//                      dmForceWander
//*********************************************************************

int dmForceWander(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Monster>  monster=nullptr;
    char    name[80];

    strcpy(name,"");

    if(!player->checkBuilder(player->getUniqueRoomParent())) {
        player->print("Error: Room number not inside any of your alotted ranges.\n");
        return(0);
    }
    if(!player->builderCanEditRoom("use *wander"))
        return(0);

    if(cmnd->num < 2) {
        player->print("Make what wander away?\n");
        return(0);
    }


    monster = player->getParent()->findMonster(player, cmnd);

    if(!monster) {
        player->print("That is not here.\n");
        return(0);
    }

    if(monster->isPlayer()) {
        player->print("That is not here.\n");
        return(0);
    }
    if(monster->isPet() || monster->flagIsSet(M_DM_FOLLOW)) {
        player->print("You can't make them wander away.\n");
        return(0);
    }

    if(monster->flagIsSet(M_PERMANENT_MONSTER)) {
        player->print("Do not make perms wander away.\n");
        return(0);
    }

    strcpy(name, monster->getCName());
    broadcast((std::shared_ptr<Socket> )nullptr, player->getRoomParent(), "%1M just %s away.", monster.get(), Move::getString(monster).c_str());

    monster->deleteFromRoom();

    log_immort(false,player,"%s forced %s to wander away in room %s.\n",
        player->getCName(), name, player->getRoomParent()->fullName().c_str());


    return(0);
}


//*********************************************************************
//                      dmBalance
//*********************************************************************

int dmBalance(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Monster> target=nullptr;
    int     lvl=0;

    if(cmnd->num < 2) {
        player->print("*balance what and to what level?\n");
        player->print("Syntax: *balance <creature> <level#>\n");
        return(0);
    }

    target = player->getParent()->findMonster(player, cmnd->str[1], 1);

    if(!target) {
        player->print("Monster not found.\n");
        return(0);
    }


    if(target->info.id && !player->checkBuilder(target->info)) {
        player->print("Creature not in any of your alotted ranges.\n");
        return(0);
    }
    if(!player->builderCanEditRoom("use *balance"))
        return(0);


    if(target->getType() == MONSTER) {
        player->print("Set creature type first. *dmh mtypes.\n");
        player->print(":*set c mob ty #\n");
        return(0);
    }

    target->setLevel((short)cmnd->val[1]);

    // Adjusts mob to normal stats.
    target->adjust(0);

    if(lvl >= 7)
        target->setFlag(M_BLOCK_EXIT);

    player->print("%M is now balanced at level %d.\n", target.get(), target->getLevel());

    player->print("%M was balanced with average %s stats.\n", target.get(), monType::getName(target->getType()));
    log_immort(true, player, "%s balanced %s (%s) to level %d.\n", player->getCName(), target->getCName(), monType::getName(target->getType()), target->getLevel());

    return(0);
}

//****************************************************************************
//                      dmAlignment
//****************************************************************************
// This function allows a CT or DM to manipulate a mob or player's alignment.

int dmAlignment(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature=nullptr;
    std::string operation_text="",alignName="";
    short operation=0,alignValue=0,alignShiftValue=0;

    if (cmnd->num < 2) {
        *player << "View of manipulate alignment on which player or mob?\n";
        return(0);
    }

    //Look for monsters in same room first
    creature = player->getParent()->findMonster(player, cmnd);
    //Look for players if no monster found
    if (!creature) {
        lowercize(cmnd->str[1], 1);
        creature = gServer->findPlayer(cmnd->str[1]);
        if(!creature || !player->canSee(creature)) {
                *player << "Monster not found or player not online.\n";
                return(0);
            }
    }

    if (!creature) {
        *player << "ERROR: something went terribly wrong. Aborting.\n";
        return(0);
    }
    

    if (cmnd->num < 3) {
        *player << "Syntax: *align player|mob show\n";
        *player << "        *align player|mob set (alignment_name|#)\n";
        *player << "        *align player|mob shift (-8 to 8) - not 0\n";
        *player << "        *align player|mob opposite\n";

        return(0);
    }


    operation_text = cmnd->str[2];
    if (operation_text == "set")
        operation = 1;
    else if (operation_text == "shift")
        operation = 2;
    else if (operation_text == "show")
        operation = 3;
    else if (operation_text == "opposite")
        operation = 4;
    else {
        *player << "Syntax: *align player|mob show\n";
        *player << "        *align player|mob set (alignment_name|#)\n";
        *player << "        *align player|mob shift (-8 to 8) - not 0\n";
        *player << "        *align player|mob opposite\n";
        return(0);
    }

    switch(operation) {
    case 1: // set alignment
        alignName = cmnd->str[3];
        if (!alignName.empty()) {
                if (alignName == "bloodred")
                    creature->setAlignment((short)(creature->isPlayer()?((P_TOP_BLOODRED+P_BOTTOM_BLOODRED)/2):((M_TOP_BLOODRED+M_BOTTOM_BLOODRED)/2)));
                else if (alignName == "red")
                    creature->setAlignment((short)(creature->isPlayer()?((P_TOP_RED+P_BOTTOM_RED)/2):((M_TOP_RED+M_BOTTOM_RED)/2)));
                else if (alignName == "reddish")
                    creature->setAlignment((short)(creature->isPlayer()?((P_TOP_REDDISH+P_BOTTOM_REDDISH)/2):((M_TOP_REDDISH+M_BOTTOM_REDDISH)/2)));
                else if (alignName == "pinkish")
                     creature->setAlignment((short)(creature->isPlayer()?((P_TOP_PINKISH+P_BOTTOM_PINKISH)/2):((M_TOP_PINKISH+M_BOTTOM_PINKISH)/2)));
                else if (alignName == "neutral")
                     creature->setAlignment(0);
                else if (alignName == "lightblue")
                     creature->setAlignment((short)(creature->isPlayer()?((P_TOP_LIGHTBLUE+P_BOTTOM_LIGHTBLUE)/2):((M_TOP_LIGHTBLUE+M_BOTTOM_LIGHTBLUE)/2)));
                else if (alignName == "bluish")
                    creature->setAlignment((short)(creature->isPlayer()?((P_TOP_BLUISH+P_BOTTOM_BLUISH)/2):((M_TOP_BLUISH+M_BOTTOM_BLUISH)/2)));
                else if (alignName == "blue")
                    creature->setAlignment((short)(creature->isPlayer()?((P_TOP_BLUE+P_BOTTOM_BLUE)/2):((M_TOP_BLUE+M_BOTTOM_BLUE)/2)));
                else if (alignName == "royalblue")
                    creature->setAlignment((short)(creature->isPlayer()?((P_TOP_ROYALBLUE+P_BOTTOM_ROYALBLUE)/2):((M_TOP_ROYALBLUE+M_BOTTOM_ROYALBLUE)/2)));
                else {
                    *player << ColorOn << "^yInvalid alignment: '" << alignName << "'\nEnter in alignment name or a number. i.e. name: bloodred, red, royalblue, etc.. or number (-1000 to 1000)\nNOTE: 'royal blue' or 'blood red' will not work. Use no spaces.\n" << ColorOff;
                    return(0);
                }
            *player << ColorOn << setf(CAP) << creature << "'s alignment value is now set to median " << creature->alignColor() << creature->alignString() << "^x. (" << creature->getAlignment() << ") [" << (creature->isPlayer()?"Player":"Monster") << "]\n" << ColorOff; 
            break;
            }
        alignValue = (short)cmnd->val[2];
        alignValue = std::max<short>(-1000,std::min<short>(1000,alignValue));

        creature->setAlignment((short)alignValue);

        *player << ColorOn << setf(CAP) << creature << "'s alignment value is now set to " << creature->getAlignment() << ". " << creature->alignColor() << "(" << creature->alignString() << ")\n" << ColorOff;
        log_immort(true, player, "%s set %s %s's alignment to %d(%s).\n",
                        player->getCName(), PLYCRT(creature), creature->getCName(), creature->getAlignment(), creature->alignString().c_str());

        break;
    case 2: // shift alignment
        alignShiftValue = cmnd->val[2];
        if (alignShiftValue == 0 || alignShiftValue < -8 || alignShiftValue > 8) {
            *player << setf(CAP) << creature << "'s alignment remains unshifted. Please enter a non-zero value between -8 and 8.\n";
            return(0);
        }
        *player << ColorOn << setf(CAP) << creature << "'s alignment has been shifted " << intToText(abs(alignShiftValue),false) << " tier" << (abs(alignShiftValue)>1?"s":"") << " from " << creature->alignColor() << creature->alignString() << "(" << creature->getAlignment() << ")";
        creature->shiftAlignment(alignShiftValue,true);
        *player << "^x to median " << creature->alignColor() << creature->alignString() << "(" << creature->getAlignment() << ")\n" << ColorOff;
        break;
    case 3: // show alignment
        *player << ColorOn << setf(CAP) << creature << "'s current alignment is " << creature->getAlignment() << ". (" << creature->alignColor() << creature->alignString() << "^x) [" << (creature->isPlayer()?"Player":"Monster") << "]\n" << ColorOff;
        return(0); // so player doesn't save
    case 4: // set opposite alignment
        if (creature->getAdjustedAlignment() == NEUTRAL) {
            *player << setf(CAP) << creature << "'s alignment is currently neutral! There is no opposite.\n";
            return(0);
        }
        *player << ColorOn << setf(CAP) << creature << "'s alignment has been shifted from " << creature->alignColor() << creature->alignString() << "(" << creature->getAlignment() << ")";
        creature->setOppositeAlignment(true);
        *player << "^x to median " << creature->alignColor() << creature->alignString() << "(" << creature->getAlignment() << ")\n" << ColorOff;

    break;
    default:
        break;

    }

    if (creature->isPlayer())
            creature->getAsPlayer()->save(true);

    return(0);
}

