/*
 * player.cpp
 *   Player routines.
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

#include <boost/algorithm/string/join.hpp>  // for join
#include <fmt/format.h>                     // for format
#include <math.h>                           // for abs
#include <unistd.h>                         // for unlink
#include <commands.hpp>                     // for cmdSave
#include <cstdio>                           // for sprintf, rename
#include <cstdlib>                          // for free, abs
#include <cstring>                          // for strcpy, strlen, strcat
#include <ctime>                            // for time, ctime
#include <iomanip>                          // for operator<<, setw
#include <list>                             // for list, operator==, list<>::cons...
#include <locale>                           // for locale
#include <map>                              // for operator==, _Rb_tree_const_ite...
#include <ostream>                          // for operator<<, basic_ostream, ost...
#include <set>                              // for set
#include <string>                           // for string, allocator, char_traits
#include <string_view>                      // for string_view
#include <utility>                          // for pair

#include "area.hpp"                         // for Area, MapMarker, MAX_VISION
#include "catRef.hpp"                       // for CatRef
#include "catRefInfo.hpp"                   // for CatRefInfo
#include "clans.hpp"                        // for Clan
#include "config.hpp"                       // for Config, gConfig
#include "deityData.hpp"                    // for DeityData
#include "dice.hpp"                         // for Dice
#include "effects.hpp"                      // for EffectInfo
#include "flags.hpp"                        // for P_DM_INVIS, P_CHAOTIC, O_DARKNESS
#include "free_crt.hpp"                     // for free_crt
#include "global.hpp"                       // for CreatureClass, CreatureClass::...
#include "guilds.hpp"                       // for Guild
#include "lasttime.hpp"                     // for lasttime
#include "levelGain.hpp"                    // for LevelGain
#include "location.hpp"                     // for Location
#include "magic.hpp"                        // for S_ARMOR, S_BLOODFUSION, S_MAGI...
#include "money.hpp"                        // for GOLD, Money
#include "move.hpp"                         // for getRoom
#include "mud.hpp"                          // for LT, LT_PLAYER_SEND, LT_AGE
#include "mudObjects/areaRooms.hpp"         // for AreaRoom
#include "mudObjects/container.hpp"         // for ObjectSet, PlayerSet
#include "mudObjects/creatures.hpp"         // for Creature, PetList
#include "mudObjects/monsters.hpp"          // for Monster
#include "mudObjects/objects.hpp"           // for Object, ObjectType, ObjectType...
#include "mudObjects/players.hpp"           // for Player
#include "mudObjects/rooms.hpp"             // for BaseRoom, ExitList
#include "mudObjects/uniqueRooms.hpp"       // for UniqueRoom
#include "os.hpp"                           // for merror
#include "paths.hpp"                        // for Help, Bank, DMHelp, History
#include "playerClass.hpp"                  // for PlayerClass
#include "property.hpp"                     // for Property
#include "proto.hpp"                        // for bonus, broadcast, abortFindRoom
#include "raceData.hpp"                     // for RaceData
#include "random.hpp"                       // for Random
#include "realm.hpp"                        // for Realm
#include "server.hpp"                       // for Server, gServer, PlayerMap
#include "size.hpp"                         // for NO_SIZE, SIZE_MEDIUM
#include "skillGain.hpp"                    // for SkillGain
#include "socket.hpp"                       // for Socket
#include "startlocs.hpp"                    // for StartLoc
#include "statistics.hpp"                   // for Statistics
#include "stats.hpp"                        // for Stat, MOD_CUR
#include "structs.hpp"                      // for daily
#include "unique.hpp"                       // for remove, deleteOwner
#include "utils.hpp"                        // for MIN, MAX
#include "xml.hpp"                          // for loadRoom


//********************************************************************
//              init
//********************************************************************
// This function initializes a player's stats and loads up the room he
// should be in. This should only be called when the player first
// logs on.

void Player::init() {
    char    file[80], str[50];
    BaseRoom *newRoom=nullptr;
    long    t = time(nullptr);

    statistics.setParent(this);

    // always make sure size matches up with race
    if(size == NO_SIZE) {
        size = gConfig->getRace(race)->getSize();

        EffectInfo *e=nullptr;
        if(isEffected("enlarge"))
            e = getEffect("enlarge");
        else if(isEffected("reduce"))
            e = getEffect("reduce");

        if(e)
            changeSize(0, e->getStrength(), e->getName() == "enlarge");

        if(!size)
            size = SIZE_MEDIUM;
    }


    setFlag(P_KILL_AGGROS);

    clearFlag(P_LINKDEAD);
    clearFlag(P_SPYING);
    clearFlag(P_READING_FILE);
    clearFlag(P_AFK);
    clearFlag(P_DIED_IN_DUEL);
    clearFlag(P_VIEW_ZONES);

    if(cClass == CreatureClass::CLERIC && level >= 7 && flagIsSet(P_CHAOTIC))
        setFlag(P_PLEDGED);

    if(level <= ALIGNMENT_LEVEL && !flagIsSet(P_CHOSEN_ALIGNMENT))
        clearFlag(P_CHAOTIC);

    if(level > ALIGNMENT_LEVEL)
        setFlag(P_CHOSEN_ALIGNMENT);

    if((cClass == CreatureClass::CLERIC && level < 7) || (cClass == CreatureClass::CLERIC && !flagIsSet(P_CHAOTIC)))
        clearFlag(P_PLEDGED);

    setFlag(P_SECURITY_CHECK_OK);

    if(isdm(getName()))
        cClass = CreatureClass::DUNGEONMASTER;
    else if(isDm())
        cClass = CreatureClass::CARETAKER;


    if(!isStaff()) {
        daily[DL_ENCHA].max = 3;
        daily[DL_FHEAL].max = MAX(3, 3 + (level) / 3);
        daily[DL_TRACK].max = MAX(3, 3 + (level) / 3);
        daily[DL_DEFEC].max = 1;    
        
         if(level < 15)
            daily[DL_TELEP].max = 1;
        else
            daily[DL_TELEP].max = 3;

        // Mages and liches get more ports than other classes; dependent on translocation magic skill
        if (getClass() == CreatureClass::MAGE || getClass() == CreatureClass::LICH)
            daily[DL_TELEP].max = MIN(10, (int)getSkillLevel("translocation")/5);

        daily[DL_RCHRG].max = MAX(7, level / 2);
        daily[DL_HANDS].max = 3;

        daily[DL_RESURRECT].max = 1;
        daily[DL_SILENCE].max = 3;
        daily[DL_HARM].max = 2;
        daily[DL_SCARES].max = 3;
    } else {
        daily[DL_DEFEC].max = 100;
    }

    if(!gServer->isRebooting())
        initBuilder();


    if(isEffected("mist") && !canMistNow())
        unmist();

    if(!isStaff()) {
        clearFlag(P_DM_INVIS);
        removeEffect("incognito");
    } else {
        // staff logs on with dmInvis
        setFlag(P_DM_INVIS);
    }

    if(isDm())
        clearFlag(P_BUGGED);

    learnLanguage(LCOMMON); // All races speak common

    //Barbarians get natural warmth. They're from the frozen tundra.
    if(race == BARBARIAN) {
       addPermEffect("warmth"); 
    }
    //Kenku can speak any language, acting like parrots, even though not understanding
    if(race == KENKU) {
       addPermEffect("tongues");
    }

    if(!current_language) {
        initLanguages();
        current_language = LCOMMON;
        setFlag(P_LANGUAGE_COLORS);
    }

    //  Paladins get auto know-aura
    if(cClass==CreatureClass::PALADIN)
        addPermEffect("know-aura");

    //  Mages get auto d-m
    if( (   cClass==CreatureClass::MAGE || cClass2 == CreatureClass::MAGE) &&
       !( cClass == CreatureClass::MAGE && cClass2 == CreatureClass::ASSASSIN)) {
        addPermEffect("detect-magic");
    }

    if(cClass == CreatureClass::DRUID && level >= 7)
        addPermEffect("pass-without-trace");

    if(cClass == CreatureClass::THIEF && cClass2 == CreatureClass::MAGE)
        addPermEffect("detect-magic");

    if(level >=1 && (cClass == CreatureClass::LICH || cClass == CreatureClass::MAGE) ) {
        learnSpell(S_ARMOR);
        learnSpell(S_MAGIC_MISSILE);
    }

    if(level >= 1 && (cClass == CreatureClass::CLERIC || cClass == CreatureClass::DRUID) )
        learnSpell(S_VIGOR);

    if(cClass == CreatureClass::CLERIC && deity == CERIS && level >= 13)
        learnSpell(S_REJUVENATE);

    if(cClass == CreatureClass::CLERIC && deity == CERIS && level >= 19)
        learnSpell(S_RESURRECT);

    if(cClass == CreatureClass::CLERIC && !hasSecondClass() && deity == ARAMON && level >= 22)
        learnSpell(S_BLOODFUSION);

    if((cClass == CreatureClass::RANGER || cClass == CreatureClass::DRUID) && level >= 10)
        learnSpell(S_TRACK);

    //  Werewolves get auto Detect-Invisibility at level 7
    if(isEffected("lycanthropy") && level >= 7)
        addPermEffect("detect-invisible");

    //  Rangers level 13+ can detect misted vampires.
    if(cClass == CreatureClass::RANGER && level >= 13)
        addPermEffect("true-sight");

    // clear ignore flags
    clearFlag(P_IGNORE_CLASS_SEND);
    clearFlag(P_IGNORE_RACE_SEND);
    clearFlag(P_NO_BROADCASTS);
    clearFlag(P_IGNORE_NEWBIE_SEND);
    clearFlag(P_IGNORE_GOSSIP);
    clearFlag(P_IGNORE_CLAN);
    clearFlag(P_NO_TELLS);
    clearFlag(P_PORTAL);

    if(!flagIsSet(P_OUTLAW))
        setFlag(P_NO_SUMMON);

    clearFlag(P_LAG_PROTECTION_OPERATING);
    clearFlag(P_ALIASING);
    if(!isStaff())
        setFlag(P_PROMPT);

    strcpy(str, "");

    lasttime[LT_PLAYER_SAVE].ltime = t;
    lasttime[LT_PLAYER_SAVE].interval = SAVEINTERVAL;

    // SEARCH FOR ME
    lasttime[LT_TICK].ltime = t+30;
    lasttime[LT_TICK_SECONDARY].ltime = t+30;
    lasttime[LT_TICK_HARMFUL].ltime = t+30;

    // spam check
    lasttime[LT_PLAYER_SEND].ltime = t;
    lasttime[LT_PLAYER_SEND].misc = 0;

    if(currentLocation.mapmarker.getArea() != 0) {
        Area *area = gServer->getArea(currentLocation.mapmarker.getArea());
        if(area)
            newRoom = area->loadRoom(nullptr, &currentLocation.mapmarker, false);

    }
    // load up parent_rom for the broadcast below, but don't add the
    // player to the room or the messages will be out of order
    if(!newRoom) {
        Property *p = gConfig->getProperty(currentLocation.room);
        if( p && p->getType() == PROP_STORAGE && !p->isOwner(getName()) && !p->isPartialOwner(getName()) ) {
            // default to bound location
            Location l = bound;
            // but go to previous room, if possible
            if(previousRoom.room.id || previousRoom.mapmarker.getArea())
                l = previousRoom;

            if(l.room.id)
                currentLocation.room = l.room;
            else
                gServer->areaInit(this, l.mapmarker);

            if(this->currentLocation.mapmarker.getArea() != 0) {
                Area *area = gServer->getArea(currentLocation.mapmarker.getArea());
                if(area)
                    newRoom = area->loadRoom(nullptr, &currentLocation.mapmarker, false);
            }
        }
    }


    // area_room might get set by areaInit, so check again
    if(!newRoom) {
        UniqueRoom  *uRoom=nullptr;
        if(!loadRoom(currentLocation.room, &uRoom)) {
            loge(fmt::format("{}: {} ({}) Attempted logon to bad or missing room!\n", getName(),
                getSock()->getHostname(), currentLocation.room.displayStr()).c_str());
            // NOTE: Using ::isCt to use the global function, not the local function
            broadcast(::isCt, fmt::format("^y{}: {} ({}) Attempted logon to bad or missing room (normal)!", getName(),
                getSock()->getHostname(), currentLocation.room.displayStr()).c_str());
            newRoom = abortFindRoom(this, "init_ply");
            uRoom = newRoom->getAsUniqueRoom();
        }


        if(uRoom && !isStaff() && !gServer->isRebooting()) {
            if( (   uRoom->flagIsSet(R_LOG_INTO_TRAP_ROOM) || uRoom->flagIsSet(R_SHOP_STORAGE) || uRoom->hasTraining()) &&
                uRoom->getTrapExit().id && !loadRoom(uRoom->getTrapExit(), &uRoom)) {
                broadcast(::isCt, fmt::format("^y{}: {} ({}) Attempted logon to bad or missing room!", getName(), getSock()->getHostname(),
                                              uRoom->getTrapExit().displayStr()).c_str());
                newRoom = abortFindRoom(this, "init_ply");
                uRoom = newRoom->getAsUniqueRoom();
            }

            if( uRoom && (   uRoom->isFull() || uRoom->flagIsSet(R_NO_LOGIN) || (!isStaff() && !flagIsSet(P_PTESTER) && uRoom->isConstruction()) || (!isStaff() && uRoom->flagIsSet(R_SHOP_STORAGE)))) {
                newRoom = getRecallRoom().loadRoom(this);
                if(!newRoom) {
                    broadcast(::isCt, fmt::format("^y{}: {} ({}) Attempted logon to bad or missing room!", getName(), getSock()->getHostname(), getRecallRoom().str()).c_str());
                    newRoom = abortFindRoom(this, "init_ply");
                }
                uRoom = newRoom->getAsUniqueRoom();
            }
        }

        if(uRoom) {
            uRoom->killMortalObjects();
            newRoom = uRoom;
        }
    }

    //  str[0] = 0;
    if(!isDm()) {
        loge(fmt::format("{}(L:{}) ({}) {}. Room - {} (Port-{})\n", getCName(), level, getSock()->getHostname(), gServer->isRebooting() ? "reloaded" : "logged on", newRoom->fullName().c_str(), Port).c_str());
    }
    if(isStaff())
        logn("log.imm", fmt::format("{}  ({}) {}.\n", getCName(), getSock()->getHostname(), gServer->isRebooting() ? "reloaded" : "logged on").c_str());


    // broadcast
    if(!gServer->isRebooting()) {
        setSockColors();
        broadcastLogin(this, newRoom, 1);
    }

    // don't do the actual adding until after broadcast
    addToRoom(newRoom);

    checkDarkness();

    for(Monster* pet : pets) {
        pet->setMaster(this);
        pet->fixLts();

        pet->updateAttackTimer();
        pet->lasttime[LT_SPELL].interval = 3;
        pet->lasttime[LT_SPELL].ltime =
        pet->lasttime[LT_MOB_THIEF].ltime = t;

        pet->addToRoom(getRoomParent());
        gServer->addActive(pet);
    }

    fixLts();

    if(cClass == CreatureClass::FIGHTER && !hasSecondClass() && flagIsSet(P_PTESTER)) {
        mp.setInitial(0);
        focus.setInitial(100);
        focus.clearModifiers();
        focus.addModifier("UnFocused", -100, MOD_CUR);
    }


    wearCursed();
    if(!flagIsSet(P_NO_AUTO_WEAR))
        wearAll(this, true);

    computeLuck();
    update();

    Socket* sock = getSock();

    if(!gServer->isRebooting()) {
        sprintf(file, "%s/news.txt",Path::Help);
        sock->viewFile(file);

        sprintf(file, "%s/newbie_news.txt",Path::Help);
        sock->viewFile(file);

        if(isCt()) {
            sprintf(file, "%s/news.txt", Path::DMHelp);
            sock->viewFile(file);
        }
        if(isStaff() && getName() != "Bane") {
            sprintf(file, "%s/news.txt", Path::BuilderHelp);
            sock->viewFile(file);
        }
        if(isCt() || flagIsSet(P_WATCHER)) {
            sprintf(file, "%s/watcher_news.txt", Path::DMHelp);
            sock->viewFile(file);
        }

        sprintf(file, "%s/latest_post.txt", Path::Help);
        sock->viewFile(file, false);

        hasNewMudmail();

        printColor("^yWatchers currently online: ");
        std::list<std::string> watchers;
        for(const auto& [pName, ply] : gServer->players) {
            if(!ply->isConnected()) continue;
            if(!ply->isPublicWatcher()) continue;
            if(!canSee(ply)) continue;

            watchers.emplace_back(ply->getName());
        }

        if(!watchers.empty()) {
            printColor(fmt::format("{}.\n", boost::algorithm::join(watchers, ", ")).c_str());
        } else
            printColor("None.\n");
    }

    if(isCt())
        showGuildsNeedingApproval(this);

    if(hp.getCur() < 0)
        hp.setCur(1);


    // only players and builders are effected
    if(!isCt()) {
        // players can't set eavesdropper flag
        clearFlag(P_EAVESDROPPER);
        clearFlag(P_SUPER_EAVESDROPPER);
        // DM/CT only flag
        clearFlag(P_NO_COMPUTE);

        setMonkDice();

        if(lasttime[LT_AGE].ltime > time(nullptr) ) {
            lasttime[LT_AGE].ltime = time(nullptr);
            lasttime[LT_AGE].interval = 0;
            broadcast(::isCt, "^yPlayer %s had negative age and is now validated.\n", getCName());
            logn("log.validate", "Player %s had negative age and is now validated.\n", getCName());
        }

    }

    // Make sure everything is in order with their guild.
    if(guild) {
        if(!gConfig->getGuild(guild)) {
            if(guildRank >= GUILD_PEON)
                print("You are now guildless because your guild has been disbanded.\n");
            guild = guildRank = 0;
        } else if(!gConfig->getGuild(guild)->isMember(getName())) {
            guild = guildRank = 0;
        }
    }


    if(actual_level < level)
        actual_level = MAX(level, actual_level);

    killDarkmetal();

    if(weaponTrains != 0)
        print("You have %d weapon train(s) available! (^YHELP WEAPONS^x)\n", weaponTrains);


    computeInterest(t, true);
}

//*********************************************************************
//                      uninit
//*********************************************************************
// This function de-initializes a player who has left the game. This
// is called whenever a player quits or disconnects, right before save
// is called.

void Player::uninit() {
    int     i=0;
    long    t=0;
    char    str[50];

    // Save storage rooms
    if(inUniqueRoom() && getUniqueRoomParent()->info.isArea("stor"))
        gServer->resaveRoom(getUniqueRoomParent()->info);

    courageous();
    clearMaybeDueling();
    removeFromGroup(!gServer->isRebooting());

    for(Monster* pet : pets) {
        if(pet->isPet()) {
            gServer->delActive(pet);
            pet->deleteFromRoom();
            free_crt(pet);
        } else {
            pet->setMaster(nullptr);
        }
    }
    pets.clear();

    for(i=0; i<MAXWEAR; i++) {
        if(ready[i]) {
            // i is wearloc-1, so add 1
            unequip(i+1);
        }
    }

    if(!gServer->isRebooting() && Crash == 0)
        broadcastLogin(this, this->getRoomParent(), 0);

    if(this->inRoom())
        deleteFromRoom();

    // TODO: Handle deleting from non rooms

    t = time(nullptr);
    strcpy(str, (char *)ctime(&t));
    str[strlen(str)-1] = 0;
    if(!isDm() && !gServer->isRebooting())
        loge("%s logged off.\n", getCName());


    charms.clear();

    if(alias_crt) {
        alias_crt->clearFlag(M_DM_FOLLOW);
        alias_crt = nullptr;
    }
}

//*********************************************************************
//                      courageous
//*********************************************************************

void Player::courageous() {
    int **scared;

    scared = &(scared_of);
    if(*scared) {
        free(*scared);
        *scared = nullptr;
    }
}

//*********************************************************************
//                      checkTempEnchant
//*********************************************************************

void Player::checkTempEnchant( Object* object) {
    long i=0, t=0;
    if(object) {
        if( object->flagIsSet(O_TEMP_ENCHANT)) {
            t = time(nullptr);
            i = LT(object, LT_ENCHA);
            if(i < t) {
                object->setArmor(MAX(0, object->getArmor() - object->getAdjustment()));
                object->setAdjustment(0);
                object->clearFlag(O_TEMP_ENCHANT);
                object->clearFlag(O_RANDOM_ENCHANT);
                if(isEffected("detect-magic"))
                    printColor("The enchantment on your %s fades.\n", object->getCName());
            }
        }
        if(object->getType() == ObjectType::CONTAINER) {
            auto it = object->objects.begin();
            auto end = object->objects.end();
            while(it != end) {
                Object* subObj = *it;
                ++it;
                checkTempEnchant(subObj);
            }
        }
    }
}

//*********************************************************************
//                      checkEnvenom
//*********************************************************************

void Player::checkEnvenom( Object* object) {
    long i=0, t=0;
    if(object && object->flagIsSet(O_ENVENOMED)) {
        t = time(nullptr);
        i = LT(object, LT_ENVEN);
        if(i < t) {
            object->clearFlag(O_ENVENOMED);
            object->clearEffect();
            printColor("The poison on your %s deludes.\n", object->getCName());
        }
    }
}

//*********************************************************************
//                      checkInventory
//*********************************************************************
// Check inventory for temp enchant or envenom

void Player::checkInventory( ) {
    int i=0;

    // Check for temp enchant items carried/inventory/in containers
    auto it = objects.begin();
    auto end = objects.end();
    while(it != end) {
        Object* obj = *it;
        ++it;
        checkTempEnchant(obj);
    }
    for(i=0; i<MAXWEAR; i++) {
        checkTempEnchant(ready[i]);
    }
}



//*********************************************************************
//                      update
//*********************************************************************
// This function checks up on all a player's time-expiring flags to see
// if some of them have expired.  If so, flags are set accordingly.

void Player::update() {
    BaseRoom* room=nullptr;
    long    t = time(nullptr);
    int     item=0;
    bool    fighting = inCombat();

    lasttime[LT_AGE].interval += (t - lasttime[LT_AGE].ltime);
    lasttime[LT_AGE].ltime = t;

    checkInventory();

    if(flagIsSet(P_LAG_PROTECTION_SET) && flagIsSet(P_LAG_PROTECTION_ACTIVE) && level > 1) {
        // Suspends lag protect if this not in battle.
        if(!fighting)
            clearFlag(P_LAG_PROTECTION_ACTIVE);
    }

    // All mobs will not attack a petrified opponent.
    if(isEffected("petrification"))
        clearAsEnemy();

    if(flagIsSet(P_UNIQUE_TO_DECAY) && !fighting) {
        gConfig->uniqueDecay(this);
        clearFlag(P_UNIQUE_TO_DECAY);
    }


    checkEffectsWearingOff();

    if(isDiseased() && immuneToDisease())
        cureDisease();
    if(isPoisoned() && immuneToPoison())
        curePoison();

    //pulseEffects(t);

    if(mp.getCur() < 0)
        mp.setCur(0);

    room = getRoomParent();
    if(room && !flagIsSet(P_LINKDEAD))
        doRoomHarms(room, this);

    if(t > LT(this, LT_PLAYER_SAVE)) {
        lasttime[LT_PLAYER_SAVE].ltime = t;
        cmdSave(this, nullptr);
    }

    item = getLight();
    if(item && item != MAXWEAR+1) {
        if(ready[item-1]->getType() == ObjectType::LIGHTSOURCE) {
            ready[item-1]->decShotsCur();
            if(ready[item-1]->getShotsCur() < 1) {
                print("Your %s died out.\n", ready[item-1]->getCName());
                broadcast(getSock(), room, "%M's %s died out.", this, ready[item-1]->getCName());
            }
        }
    }

    if(isStaff() && flagIsSet(P_AUTO_INVIS) && !flagIsSet(P_DM_INVIS) && getSock() && (t - getSock()->ltime) > 6000) {
        printColor("^g*** Automatically enabling DM invisibility ***\n");
        setFlag(P_DM_INVIS);
    }

    // Fix funky stuff
    setMonkDice();
    computeAC();
    computeAttackPower();
}

//*********************************************************************
//                      computeAC
//*********************************************************************
// This function computes a player's armor class by
// examining their stats and the items they are holding.

void Player::computeAC() {
    int     ac=0, i;

    // Monks are a little more impervious to damage than other classes due to
    // their practice in meditation
    if(cClass == CreatureClass::MONK)
        ac += level * 10;

    // Wolves have a tough skin that grows tougher as they level up
    if(isEffected("lycanthropy"))
        ac += level * 20;

    // Vamps have a higher armor at night
    if(isEffected("vampirism") && !isDay())
        ac += level * 5 / 2;


    for(i=0; i<MAXWEAR; i++) {
        if(ready[i] && ready[i]->getType() == ObjectType::ARMOR) {
            ac += ready[i]->getArmor();
            // penalty for wearing armor of the wrong size
            if(size && ready[i]->getSize() && size != ready[i]->getSize())
                ac -= ready[i]->getArmor()/2;
            if(ready[i]->getWearflag() < FINGER || ready[i]->getWearflag() > HELD)
                ac += (int)(ready[i]->getAdjustment()*4.4);
        }
    }

    if((cClass == CreatureClass::DRUID || isCt()) && isEffected("barkskin")) {
        EffectInfo* barkskin = getEffect("barkskin");
        ac += (int)(((level+bonus(constitution.getCur())) * barkskin->getStrength())*4.4);
    }

    if(isEffected("armor"))
        ac += 200 + 200 * level /30;

// TODO: Possibly add in +armor for higher constitution?

    if(isEffected("berserk"))
        ac -= 100;
    if(isEffected("fortitude"))
        ac += 100;
    if(isEffected("weakness"))
        ac -= 100;

    armor = MAX(0, ac);
}


//*********************************************************************
//                      getArmorWeight
//*********************************************************************

int Player::getArmorWeight() const {
    int weight=0;

    for(auto i : ready) {
        if( i && i->getType() == ObjectType::ARMOR &&
            (   (i->getWearflag() < FINGER) ||
                (i->getWearflag() > HELD)
            )
        )
            weight += i->getActualWeight();
    }

    return(weight);
}


//*********************************************************************
//                      getFallBonus
//*********************************************************************
// This function computes a player's bonus (or susceptibility) to falling
// while climbing.

int Player::getFallBonus()  {
    int fall = bonus(dexterity.getCur()) * 5;

    for(auto & j : ready)
        if(j)
            if(j->flagIsSet(O_CLIMBING_GEAR))
                fall += j->damage.getPlus()*3;
    return(fall);
}



//*********************************************************************
//                      lowest_piety
//*********************************************************************
// This function finds the player with the lowest piety in a given room.
// The pointer to that player is returned. In the case of a tie, one of
// them is randomly chosen.

Player* lowest_piety(BaseRoom* room, bool invis) {
    Creature* player=nullptr;
    int     totalpiety=0, pick=0;

    if(room->players.empty())
        return(nullptr);

    for(Player* ply : room->players) {
        if( ply->flagIsSet(P_HIDDEN) ||
            (   ply->isInvisible() &&
                !invis
            ) ||
            ply->flagIsSet(P_DM_INVIS) )
        {
            continue;
        }
        totalpiety += MAX<int>(1, (25 - ply->piety.getCur()));
    }

    if(!totalpiety)
        return(nullptr);
    pick = Random::get(1, totalpiety);

    totalpiety = 0;

    for(Player* ply : room->players) {
        if( ply->flagIsSet(P_HIDDEN) ||
            (   ply->isInvisible() &&
                !invis
            ) ||
            ply->flagIsSet(P_DM_INVIS) )
        {
            continue;
        }
        totalpiety += MAX<int>(1, (25 - ply->piety.getCur()));
        if(totalpiety >= pick) {
            player = ply;
            break;
        }
    }

    return(player->getAsPlayer());
}

//*********************************************************************
//                      getLight
//*********************************************************************
// This function returns true if the player in the first parameter is
// holding or wearing anything that generates light.

int Player::getLight() const {
    int i=0, light=0;

    for(i = 0; i < MAXWEAR; i++) {
        if (!ready[i])
            continue;
        if (ready[i]->flagIsSet(O_LIGHT_SOURCE)) {
            if ((ready[i]->getType() == ObjectType::LIGHTSOURCE && ready[i]->getShotsCur() > 0) ||
                ready[i]->getType() != ObjectType::LIGHTSOURCE) {
                light = 1;
                break;
            }
        }
    }

    if(light)
        return (i + 1);
    return(0);
}

//*********************************************************************
//                      computeLuck
//*********************************************************************
// This sets the luck value for a given player

int Player::computeLuck() {
    int     num=0, alg=0, con=0, smrt=0;

    alg = abs(alignment);
    alg = alg + 1;

    alg /= 10;


    // alignment only matters for these classes
    if(cClass != CreatureClass::PALADIN && cClass != CreatureClass::CLERIC && cClass != CreatureClass::DEATHKNIGHT && cClass != CreatureClass::LICH)
        alg = 0;


    if( !alg ||
        (cClass == CreatureClass::PALADIN && deity != GRADIUS && getAdjustedAlignment() > NEUTRAL) ||
        (cClass == CreatureClass::DEATHKNIGHT && getAdjustedAlignment() < NEUTRAL) ||
        (cClass == CreatureClass::LICH && alignment <= -500) ||
        (cClass == CreatureClass::CLERIC && (deity == ENOCH || deity == LINOTHAN || deity == KAMIRA) && getAdjustedAlignment() >= LIGHTBLUE) ||
        (cClass == CreatureClass::CLERIC && (deity == ARAMON || deity == ARACHNUS) && getAdjustedAlignment() <= PINKISH)
    )
        alg = 1;

    if(cClass != CreatureClass::LICH)  // Balances mages with liches for luck.
        con = constitution.getCur()/10;
    else
        con = piety.getCur()/10;

    smrt = intelligence.getCur()/10;

    num = 100*(smrt+con);
    num /= alg;

    if(ready[HELD-1] && ready[HELD-1]->flagIsSet(O_LUCKY))
        num += ready[HELD-1]->damage.getPlus();

    // Carrying around alot of gold isn't very lucky!
    if(!isStaff())
        num -= (coins[GOLD] / 20000);

    num = MAX(1, MIN(99, num));

    luck = num;
    return(num);
}

//*********************************************************************
//                      checkForSpam
//*********************************************************************

bool Player::checkForSpam() {
    int     t = time(nullptr);

    if(!isCt()) {
        if(lasttime[LT_PLAYER_SEND].ltime == t) {
            // 4 in one second is spam
            if(++lasttime[LT_PLAYER_SEND].misc > 3) {
                silenceSpammer();
                return(true);
            }
        } else if(lasttime[LT_PLAYER_SEND].ltime + 1 == t) {
            // 6 in 2 seconds is spam
            if(++lasttime[LT_PLAYER_SEND].misc > 5) {
                silenceSpammer();
                return(true);
            }
        } else if(lasttime[LT_PLAYER_SEND].ltime + 2 == t) {
            // 7 in 3 seconds is spam
            if(++lasttime[LT_PLAYER_SEND].misc > 6 ) {
                silenceSpammer();
                return(true);
            }
        } else {
            // reset spam counter
            lasttime[LT_PLAYER_SEND].ltime = t;
            lasttime[LT_PLAYER_SEND].misc = 1;
        }
    }


    return(false);
}


//*********************************************************************
//                      silenceSpammer
//*********************************************************************

void Player::silenceSpammer() {
    if(isEffected("silence")) {
        // already spamming
        EffectInfo* silenceEffect = getEffect("silence");
        silenceEffect->setDuration(silenceEffect->getDuration() + 300);
        printColor("^rYou have been given an additional 5 minutes of silence for spamming again!\n");
    } else {
        // first spam
        addEffect("silence", 120, 1);

        printColor("^rYou have been silenced for 2 minutes for spamming!\n");
        broadcast(getSock(), getRoomParent(), "%s has been silenced for spamming!\n",getCName());
    }
}

//*********************************************************************
//                      setMonkDice
//*********************************************************************

// Wolf leveling code
Dice wolf_dice[41] =
{                           // Old dice
    Dice(2, 2, 0),   /* 0  */  // Dice(1, 4, 0)
    Dice(2, 2, 0),   /* 1  */  // Dice(1, 4, 0)
    Dice(2, 2, 1),   /* 2  */  // Dice(1, 5, 1)
    Dice(2, 3, 1),   /* 3  */  // Dice(1, 7, 1)
    Dice(2, 4, 0),   /* 4  */  // Dice(1, 7, 2)
    Dice(3, 3, 0),   /* 5  */  // Dice(2, 3, 2)
    Dice(3, 3, 2),   /* 6  */  // Dice(2, 4, 1)
    Dice(4, 3, 0),   /* 7  */  // Dice(2, 4, 2)
    Dice(4, 3, 1),   /* 8  */  // Dice(2, 5, 1)
    Dice(5, 3, 0),   /* 9  */  // Dice(2, 5, 2)
    Dice(5, 3, 1),   /* 10 */  // Dice(2, 6, 1)
    Dice(5, 3, 2),   /* 11 */  // Dice(2, 6, 2)
    Dice(6, 3, 1),   /* 12 */  // Dice(3, 4, 1)
    Dice(6, 3, 2),   /* 13 */  // Dice(3, 4, 2)
    Dice(7, 3, 0),   /* 14 */  // Dice(3, 5, 1)
    Dice(7, 3, 1),   /* 15 */  // Dice(3, 5, 2)
    Dice(7, 3, 2),   /* 16 */  // Dice(3, 7, 1)
    Dice(7, 3, 3),   /* 17 */  // Dice(4, 5, 0)
    Dice(7, 4, 0),   /* 18 */  // Dice(5, 6, 1)
    Dice(7, 4, 2),   /* 19 */  // Dice(5, 6, 2)
    Dice(7, 4, 3),   /* 20 */  // Dice(6, 5, 3)
    Dice(7, 4, 5),   /* 21 */  // Dice(6, 6, 0)
    Dice(7, 5, 0),   /* 22 */  // Dice(6, 6, 2)
    Dice(7, 5, 2),   /* 23 */  // Dice(6, 6, 3)
    Dice(7, 5, 1),   /* 24 */  // Dice(6, 7, 1)
    Dice(7, 5, 3),   /* 25 */  // Dice(6, 8, 0)
    Dice(8, 5, 0),   /* 26 */  // Dice(6, 8, 2)
    Dice(8, 5, 2),   /* 27 */  // Dice(6, 8, 4)
    Dice(8, 5, 4),   /* 28 */  // Dice(7, 7, 2)
    Dice(9, 5, 2),   /* 29 */  // Dice(7, 7, 4)
    Dice(9, 5, 3),  /* 30 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 31 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 32 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 33 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 34 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 35 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 36 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 37 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 38 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 39 */  // Dice(7, 7, 6)
    Dice(9, 5, 3)   /* 40 */  // Dice(7, 7, 6)
};


// monk leveling code
Dice monk_dice[41] =
{
    Dice(1, 3, 0),  /* 0  */  // Old dice
    Dice(1, 3, 0),  /* 1  */  // Dice(1, 3, 0)
    Dice(1, 4, 0),  /* 2  */  // Dice(1, 5, 0)
    Dice(1, 5, 0),  /* 3  */  // Dice(1, 5, 1)
    Dice(1, 6, 0),  /* 4  */  // Dice(1, 6, 0)
    Dice(1, 6, 1),  /* 5  */  // Dice(1, 6, 1)
    Dice(2, 4, 1),  /* 6  */  // Dice(1, 6, 2)
    Dice(2, 5, 0),  /* 7  */  // Dice(2, 3, 1)
    Dice(2, 5, 1),  /* 8  */  // Dice(2, 4, 0)
    Dice(2, 6, 0),  /* 9  */  // Dice(2, 4, 1)
    Dice(2, 6, 2),  /* 10 */  // Dice(2, 5, 0)
    Dice(3, 5, 2),  /* 11 */  // Dice(2, 5, 2)
    Dice(3, 6, 0),  /* 12 */  // Dice(2, 6, 1)
    Dice(3, 6, 2),  /* 13 */  // Dice(2, 6, 2)
    Dice(3, 7, 0),  /* 14 */  // Dice(3, 6, 1)
    Dice(3, 7, 2),  /* 15 */  // Dice(3, 7, 1)
    Dice(4, 6, 2),  /* 16 */  // Dice(4, 7, 1)
    Dice(4, 7, 0),  /* 17 */  // Dice(5, 7, 0)
    Dice(4, 7, 2),  /* 18 */  // Dice(5, 8, 1)
    Dice(4, 8, 0),  /* 19 */  // Dice(6, 7, 0)
    Dice(4, 8, 2),  /* 20 */  // Dice(6, 7, 2)
    Dice(5, 7, 2),  /* 21 */  // Dice(6, 8, 0)
    Dice(5, 8, 0),  /* 22 */  // Dice(6, 8, 2)
    Dice(5, 8, 2),  /* 23 */  // Dice(6, 9, 0)
    Dice(5, 9, 0),  /* 24 */  // Dice(6, 9, 2)
    Dice(5, 9, 2),  /* 25 */  // Dice(6, 10, 0 )
    Dice(6, 8, 2),  /* 26 */  // Dice(6, 10, 2 )
    Dice(6, 9, 0),  /* 27 */  // Dice(7, 9, 4)
    Dice(6, 9, 2),  /* 28 */  // Dice(7, 9, 6)
    Dice(6, 10, 0), /* 29 */  // Dice(7, 8, 8)
    Dice(6, 10, 2), /* 30 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 31 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 32 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 33 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 34 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 35 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 36 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 37 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 38 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 39 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2)  /* 40 */  // Dice(8, 8, 10 )

};

void Player::setMonkDice() {
    int nLevel = MAX<int>(0, MIN<int>(level, MAXALVL));

    // reset monk dice?
    if(cClass == CreatureClass::MONK) {
        damage = monk_dice[nLevel];
    } else if(cClass == CreatureClass::WEREWOLF) {
        damage = wolf_dice[nLevel];
    }
}

//*********************************************************************
//                      initLanguages
//*********************************************************************

void Player::initLanguages() {

    switch(race) {
        case DWARF:
            learnLanguage(LDWARVEN);
            learnLanguage(LGNOMISH);
            learnLanguage(LKOBOLD);
            learnLanguage(LGOBLINOID);
            learnLanguage(LORCISH);
            break;
        case ELF:
            learnLanguage(LELVEN);
            learnLanguage(LGNOMISH);
            learnLanguage(LORCISH);
            break;
        case HALFELF:
            learnLanguage(LELVEN);
            learnLanguage(LHALFLING);
            break;
        case HALFLING:
            learnLanguage(LHALFLING);
            learnLanguage(LGNOMISH);
            break;
        case ORC:
            learnLanguage(LORCISH);
            learnLanguage(LGIANTKIN);
            break;
        case HALFGIANT:
            learnLanguage(LGIANTKIN);
            learnLanguage(LOGRISH);
            learnLanguage(LTROLL);
            break;
        case GNOME:
            learnLanguage(LGNOMISH);
            learnLanguage(LDWARVEN);
            learnLanguage(LGOBLINOID);
            learnLanguage(LKOBOLD);
            break;
        case TROLL:
            learnLanguage(LTROLL);
            break;
        case HALFORC:
            learnLanguage(LORCISH);
            break;
        case OGRE:
            learnLanguage(LOGRISH);
            learnLanguage(LGIANTKIN);
            break;
        case DARKELF:
            learnLanguage(LDARKELVEN);
            learnLanguage(LORCISH);
            learnLanguage(LELVEN);
            learnLanguage(LDWARVEN);
            learnLanguage(LGNOMISH);
            learnLanguage(LKOBOLD);
            learnLanguage(LGOBLINOID);
            break;
        case GOBLIN:
            learnLanguage(LGOBLINOID);
            learnLanguage(LORCISH);
            break;
        case MINOTAUR:
            learnLanguage(LMINOTAUR);
            break;
        case SERAPH:
            learnLanguage(LELVEN);
            learnLanguage(LCELESTIAL);
            learnLanguage(LINFERNAL);
            learnLanguage(LGNOMISH);
            learnLanguage(LHALFLING);
            learnLanguage(LABYSSAL);
            break;
        case KENKU:
            learnLanguage(LKENKU);
            break;
        case KOBOLD:
            learnLanguage(LKOBOLD);
            learnLanguage(LGNOMISH);
            learnLanguage(LDARKELVEN);
            learnLanguage(LOGRISH);
            learnLanguage(LGIANTKIN);
            break;
        case CAMBION:
            learnLanguage(LINFERNAL);
            learnLanguage(LDARKELVEN);
            learnLanguage(LELVEN);
            learnLanguage(LCELESTIAL);
            learnLanguage(LORCISH);
            learnLanguage(LGIANTKIN);
            learnLanguage(LGOBLINOID);
            break;
        case BARBARIAN:
            learnLanguage(LBARBARIAN);
            break;
        case KATARAN:
            learnLanguage(LKATARAN);
            break;
        case TIEFLING:
            learnLanguage(LHALFLING);
            learnLanguage(LINFERNAL);
            learnLanguage(LABYSSAL);
            learnLanguage(LORCISH);
            learnLanguage(LGOBLINOID);
            learnLanguage(LTIEFLING);
            break;
    } // End switch.

    switch(cClass) {
        case CreatureClass::THIEF:
        case CreatureClass::ASSASSIN:
        case CreatureClass::BARD:
        case CreatureClass::ROGUE:
            learnLanguage(LTHIEFCANT);
            break;
        case CreatureClass::DRUID:
        case CreatureClass::RANGER:
            learnLanguage(LDRUIDIC);
            break;
        case CreatureClass::WEREWOLF:
            learnLanguage(LWOLFEN);
            break;
        case CreatureClass::MAGE:
        case CreatureClass::LICH:
            learnLanguage(LARCANIC);
            break;
        case CreatureClass::CLERIC:
            switch(deity) {
            case ARAMON:
                learnLanguage(LINFERNAL);
                break;
            case ENOCH:
                learnLanguage(LCELESTIAL);
                break;
            case ARES:
                learnLanguage(LBARBARIAN);
                break;
            case KAMIRA:
                learnLanguage(LTHIEFCANT);
                break;
            }
            break;
        default:
            break;
    }
    }

//*********************************************************************
//                      doRecall
//*********************************************************************

void Player::doRecall(int roomNum) {
    UniqueRoom  *new_rom=nullptr;
    BaseRoom *newRoom=nullptr;

    if(roomNum == -1) {
        newRoom = recallWhere();
    } else {
        if(!loadRoom(roomNum, &new_rom))
            newRoom = abortFindRoom(this, "doRecall");
        else
            newRoom = new_rom;
    }

    courageous();
    deleteFromRoom();
    addToRoom(newRoom);
    doPetFollow();
}


//*********************************************************************
//                      loseRage
//*********************************************************************

void Player::loseRage() {
    if(!flagIsSet(P_BERSERKED_OLD))
        return;
    printColor("^rYour rage diminishes.^x\n");
    clearFlag(P_BERSERKED_OLD);

    if(cClass == CreatureClass::CLERIC && deity == ARES)
        strength.upgradeSetCur(strength.getCur(false) - 30);
    else
        strength.upgradeSetCur(strength.getCur(false) - 50);
    computeAC();
    computeAttackPower();
}

//*********************************************************************
//                      losePray
//*********************************************************************

void Player::losePray() {
    if(!flagIsSet(P_PRAYED_OLD))
        return;
    if(cClass != CreatureClass::DEATHKNIGHT) {
        printColor("^yYou feel less pious.\n");
        piety.upgradeSetCur(piety.getCur(false) - 50);
    } else {
        printColor("^rYour demonic strength leaves you.\n");
        strength.upgradeSetCur(strength.getCur(false) - 30);
        computeAC();
        computeAttackPower();
    }
    clearFlag(P_PRAYED_OLD);
}

//*********************************************************************
//                      loseFrenzy
//*********************************************************************

void Player::loseFrenzy() {
    if(!flagIsSet(P_FRENZY_OLD))
        return;
    printColor("^gYou feel slower.\n");
    clearFlag(P_FRENZY_OLD);
    dexterity.upgradeSetCur(dexterity.getCur(false) - 50);
}

//*********************************************************************
//                      halftolevel
//*********************************************************************
// The following function will return true if a player is more then halfway in experience
// to their next level, causing them to not gain exp until they go level. If they are
// not yet half way, it will return false. Staff and players under level 5 are exempt.

bool Player::halftolevel() const {
    if(isStaff())
        return(false);

    // can save up xp until level 5.
    if(experience <= Config::expNeeded(5))
        return(false);

    if(experience >= Config::expNeeded(level) + (((unsigned)(Config::expNeeded(level+1) - Config::expNeeded(level)))/2)) {
        print("You have enough experience to train.\nYou are half way to the following level.\n");
        print("You must go train in order to gain further experience.\n");
        return(true);
    }

    return(false);
}

//*********************************************************************
//                      getVision
//*********************************************************************

int Player::getVision() const {

    if(isStaff())
        return(MAX_VISION);

    int vision = 8;
    if(race == ELF)
        vision++;

    if(race == DARKELF) {
        if(isDay())
            vision--;
        else
            vision++;
    }

    if(isEffected("farsight"))
        vision *= 2;

    return(MIN(MAX_VISION, vision));
}


//*********************************************************************
//                      getSneakChance
//*********************************************************************
// determines out percentage chance of being able to sneak

int Player::getSneakChance()  {
    int sLvl = (int)getSkillLevel("sneak");

    if(isStaff())
        return(101);

    // this is the base chance for most classes
    int chance = MIN(70, 5 + 2 * sLvl + 3 * bonus(dexterity.getCur()));

    switch(cClass) {
    case CreatureClass::THIEF:
        if(cClass2 == CreatureClass::MAGE)
            chance = MIN(90, 5 + 8 * MAX(1,sLvl-2) + 3 * bonus(dexterity.getCur()));
        else
            chance = MIN(90, 5 + 8 * sLvl + 3 * bonus(dexterity.getCur()));

        break;
    case CreatureClass::ASSASSIN:
        chance = MIN(90, 5 + 8 * sLvl + 3 * bonus(dexterity.getCur()));
        break;
    case CreatureClass::CLERIC:
        if(cClass2 == CreatureClass::ASSASSIN)
            chance = MIN(90, 5 + 8 * MAX(1,sLvl-2) + 3 * bonus(dexterity.getCur()));
        else if(deity == KAMIRA || deity == ARACHNUS)
            chance = MIN(90, 5 + 8 * MAX(1,sLvl-2) + 3 * bonus(piety.getCur()));

        break;
    case CreatureClass::FIGHTER:
        if(cClass2 == CreatureClass::THIEF)
            chance = MIN(90, 5 + 8 * MAX(1,sLvl-2) + 3 * bonus(dexterity.getCur()));

        break;
    case CreatureClass::MAGE:
        if(cClass2 == CreatureClass::THIEF || cClass2 == CreatureClass::ASSASSIN)
            chance = MIN(90, 5 + 8 * MAX(1,sLvl-3) + 3 * bonus(dexterity.getCur()));

        break;
    case CreatureClass::DRUID:
        if(getConstRoomParent()->isForest())
            chance = MIN(95 , 5 + 10 * sLvl + 3 * bonus(dexterity.getCur()));

        break;
    case CreatureClass::RANGER:
        if(getConstRoomParent()->isForest())
            chance = MIN(95 , 5 + 10 * sLvl + 3 * bonus(dexterity.getCur()));
        else
            chance = MIN(83, 5 + 8 * sLvl + 3 * bonus(dexterity.getCur()));
        break;
    case CreatureClass::ROGUE:
        chance = MIN(85, 5 + 7 * sLvl + 3 * bonus(dexterity.getCur()));
        break;
    default:
        break;
    }

    if(isBlind())
        chance = MIN(20, chance);

    if(isEffected("camouflage")) {
        if(getConstRoomParent()->isOutdoors())
            chance += 15;
        if(cClass == CreatureClass::DRUID && getConstRoomParent()->isForest())
            chance += 5;
    }

    return(MIN(99, chance));
}



//*********************************************************************
//                      breakObject
//*********************************************************************
// breaks a worn object and readds it to the player's inventory

bool Player::breakObject(Object* object, int loc) {
    bool darkness = false;

    if(!object)
        return(false);

    if(object->getShotsCur() < 1) {
        printColor("Your %s is broken.\n", object->getCName());
        broadcast(getSock(), getRoomParent(), "%M broke %s %s.", this, hisHer(), object->getCName());

        if(object->compass) {
            delete object->compass;
            object->compass = nullptr;
        }

        object->clearFlag(O_WORN);
        Limited::remove(this, object);

        if(object->flagIsSet(O_DARKNESS)) {
            darkness = true;
            object->clearFlag(O_DARKNESS);
        }

        if(object->flagIsSet(O_EQUIPPING_BESTOWS_EFFECT)) {
            object->clearFlag(O_EQUIPPING_BESTOWS_EFFECT);
            object->setEffect("");
            object->setEffectDuration(0);
            object->setEffectStrength(0);
        }

        if(object->getAdjustment())
            object->setAdjustment(0);
        if(object->flagIsSet(O_TEMP_ENCHANT)) {
            object->damage.setPlus(0);
            object->clearFlag(O_TEMP_ENCHANT);
            object->clearFlag(O_RANDOM_ENCHANT);
            if(isEffected("detect-magic"))
                print("The enchantment on your %s fades.\n", object->getCName());
        }
        if(object->flagIsSet(O_ENVENOMED)) {
            object->clearFlag(O_ENVENOMED);
            object->clearEffect();
            print("The poison on your %s becomes useless.\n", object->getCName());
        }

        if(loc != -1) {
            unequip(loc);
        } else {
            broadcast(::isDm, "^g>>> BreakObject: BadLoc (Loc:%d) %'s %s", loc, getCName(), object->getCName());
            if(ready[WIELD-1] == object) {
                unequip(WIELD);
            } else if(ready[HELD-1] == object) {
                unequip(HELD);
            }
        }
        if(darkness)
            checkDarkness();
        return(true);
    }
    return(false);
}


//********************************************************************
//              getWhoString
//********************************************************************

std::string Player::getWhoString(bool whois, bool color, bool ignoreIllusion) const {
    std::ostringstream whoStr;

    if(whois) {
        whoStr << "^bWhois for [" << getName() << "]\n";
        whoStr << "----------------------------------------------------------------------------------\n";
    }

    whoStr << (color ? "^x[^c" : "[") << std::setw(2) << level
           << ":" << std::setw(4) << std::string(getShortClassName(this)).substr(0, 4)
           << (color ? "^x] " : "] ");

    if(isHardcore())
        whoStr << (color ? "^y" : "") << "H ";
    else if(flagIsSet(P_OUTLAW))
        whoStr << (color ? "^r" : "") << "O ";
    else if( (flagIsSet(P_NO_PKILL) || flagIsSet(P_DIED_IN_DUEL) ||
            getConstRoomParent()->isPkSafe()) &&
            (flagIsSet(P_CHAOTIC) || clan || cClass == CreatureClass::CLERIC) )
        whoStr << (color ? "^y" : "") << "N ";
    else if(flagIsSet(P_CHAOTIC)) // Chaotic
        whoStr << (color ? "^y" : "") << "C ";
    else // Lawful
        whoStr << (color ? "^y" : "") << "L ";

    if(color)
        whoStr << (isPublicWatcher() ? "^w" : "^g");

    whoStr << fullName() << " the ";
    if(whois)
        whoStr << getSexName(getSex()) << " ";

    whoStr << gConfig->getRace(getDisplayRace())->getAdjective();
    // will they see through the illusion?
    if(ignoreIllusion && getDisplayRace() != getRace())
        whoStr << " (" << gConfig->getRace(getRace())->getAdjective() << ")";
    whoStr << " " << getTitle();
    if(whois)
        whoStr << " (Age:" << getAge() << ")";

    if(color) whoStr << " ^w";
    if(flagIsSet(P_DM_INVIS)) whoStr << "[+]";
    if(isEffected("incognito") ) whoStr << "[g]";
    if(isInvisible() ) whoStr << "[*]";
    if(isEffected("mist") ) whoStr << "[m]";
    if(flagIsSet(P_LINKDEAD) && !isPublicWatcher() ) whoStr << "[l]";

    if(flagIsSet(P_AFK)) whoStr << (color ? "^R" : "") << " [AFK]";

    if(deity) {
        whoStr << (color ? "^r" : "") << " (" << gConfig->getDeity(deity)->getName() << ")";
    } else if(clan) {
        whoStr << (color ? "^r" : "") << " (" << gConfig->getClan(clan)->getName() << ")";
    }

    if(guild && guildRank >= GUILD_PEON) {
        whoStr << (color ? "^y" : "") << " [" << getGuildName(guild) << "]";
    }

    whoStr << (color ? "^x" : "") << "\n";
    return(whoStr.str());
}


//*********************************************************************
//                      getBound
//*********************************************************************

Location Player::getBound() {
    if(bound != getLimboRoom() && bound != getRecallRoom())
        return(bound);
    else {
        // we hardcode highport in case of failure
        const StartLoc* location = gConfig->getStartLoc("highport");
        bind(location);
        if(location)
            return(location->getBind());
    }
    // too many errors to handle!
    Location cr;
    cr.room.id = ROOM_BOUND_FAILURE;
    return(cr);
}


//*********************************************************************
//                      expToLevel
//*********************************************************************

unsigned long Player::expToLevel() const {
    return(experience > Config::expNeeded(level) ? 0 : Config::expNeeded(level) - experience);
}

std::string Player::expToLevel(bool addX) const {
    if(level < MAXALVL) {
        std::ostringstream oStr;
        oStr.imbue(std::locale(isStaff() ? "C" : ""));
        oStr << expToLevel();
        if(addX)
            oStr << "X";
        return(oStr.str());
    }
    return("infinity");
}

std::string Player::expInLevel() const {
    int displayLevel = level;
    if(level > MAXALVL) {
        displayLevel = MAXALVL;
    }

    std::ostringstream oStr;
    oStr << MIN<long>((experience - Config::expNeeded(displayLevel-1)),
            (Config::expNeeded(displayLevel) - Config::expNeeded(displayLevel-1)));
    return(oStr.str());
}

// TNL Max
std::string Player::expForLevel() const {
    int displayLevel = level;
    if(level > MAXALVL) {
        displayLevel = MAXALVL;
    }

    std::ostringstream oStr;
    oStr << (Config::expNeeded(displayLevel) - Config::expNeeded(displayLevel-1));
    return(oStr.str());
}

std::string Player::expNeededDisplay() const {
    if(level < MAXALVL) {
        std::ostringstream oStr;
        oStr.imbue(std::locale(isStaff() ? "C" : ""));
        oStr << Config::expNeeded(level);
        return(oStr.str());
    }
    return("infinity");
}
//*********************************************************************
//                      exists
//*********************************************************************

bool Player::exists(std::string_view name) {
    return(file_exists(fmt::format("{}/{}.xml", Path::Player, name).c_str()));
}

//*********************************************************************
//                      inList functions
//*********************************************************************

bool Player::inList(const std::list<std::string> &list, const std::string &name) const {
    return std::find(list.begin(), list.end(), name) != list.end();
}


bool Player::isIgnoring(const std::string &name) const {
    return(inList(ignoring, name));
}
bool Player::isGagging(const std::string &name) const {
    return(inList(gagging, name));
}
bool Player::isRefusing(const std::string &name) const {
    return(inList(refusing, name));
}
bool Player::isDueling(const std::string &name) const {
    return(inList(dueling, name));
}
bool Player::isWatching(const std::string &name) const {
    return(inList(watching, name));
}

//*********************************************************************
//                      showList
//*********************************************************************

std::string Player::showList(const std::list<std::string> &list) const {
    std::ostringstream oStr;

    if(list.empty())
        return("No one.");

    return fmt::format("{}.", boost::algorithm::join(list, ", "));
}


std::string Player::showIgnoring() const {
    return(showList(ignoring));
}
std::string Player::showGagging() const {
    return(showList(gagging));
}
std::string Player::showRefusing() const {
    return(showList(refusing));
}
std::string Player::showDueling() const {
    return(showList(dueling));
}
std::string Player::showWatching() const {
    return(showList(watching));
}

//*********************************************************************
//                      addList functions
//*********************************************************************

void Player::addList(std::list<std::string> &list, const std::string &name) {
    list.push_back(name);
}


void Player::addIgnoring(const std::string &name) {
    addList(ignoring, name);
}
void Player::addGagging(const std::string &name) {
    addList(gagging, name);
}
void Player::addRefusing(const std::string &name) {
    addList(refusing, name);
}
void Player::addDueling(const std::string &name) {
    delList(maybeDueling, name);

    // if they aren't dueling us, add us to their maybe dueling list
    Player* player = gServer->findPlayer(name);
    if(player && !player->isDueling(name))
        player->addMaybeDueling(getName());

    addList(dueling, name);
}
void Player::addMaybeDueling(const std::string &name) {
    addList(maybeDueling, name);
}
void Player::addWatching(const std::string &name) {
    addList(watching, name);
}

//*********************************************************************
//                      delList functions
//*********************************************************************

void Player::delList(std::list<std::string> &list, const std::string &name) {
    list.remove(name);
}

void Player::delIgnoring(const std::string &name) {
    delList(ignoring, name);
}
void Player::delGagging(const std::string &name) {
    delList(gagging, name);
}
void Player::delRefusing(const std::string &name) {
    delList(refusing, name);
}
void Player::delDueling(const std::string &name) {
    delList(dueling, name);
}
void Player::delWatching(const std::string &name) {
    delList(watching, name);
}

//*********************************************************************
//                      clear list functions
//*********************************************************************

void Player::clearIgnoring() {
    ignoring.clear();
}
void Player::clearGagging() {
    gagging.clear();
}
void Player::clearRefusing() {
    refusing.clear();
}
void Player::clearDueling() {
    dueling.clear();
}
void Player::clearMaybeDueling() {
    Player* player=nullptr;
    for(const auto& pName : maybeDueling) {
        player = gServer->findPlayer(pName);
        if(!player)
            continue;
        player->delDueling(getName());
    }

    maybeDueling.clear();
}
void Player::clearWatching() {
    watching.clear();
}

//*********************************************************************
//                      renamePlayerFiles
//*********************************************************************

void renamePlayerFiles(const char *old_name, const char *new_name) {
    char    file[80], file2[80];

    sprintf(file, "%s/%s.xml", Path::Player, old_name);
    unlink(file);

    sprintf(file, "%s/%s.txt", Path::Post, old_name);
    sprintf(file2, "%s/%s.txt", Path::Post, new_name);
    rename(file, file2);

    sprintf(file, "%s/%s.txt", Path::History, old_name);
    sprintf(file2, "%s/%s.txt", Path::History, new_name);
    rename(file, file2);

    sprintf(file, "%s/%s.txt", Path::Bank, old_name);
    sprintf(file2, "%s/%s.txt", Path::Bank, new_name);
    rename(file, file2);
}

//*********************************************************************
//                      checkHeavyRestrict
//*********************************************************************

bool Player::checkHeavyRestrict(std::string_view skill) const {
    // If we aren't one of the classes that can use heavy armor, but with restrictions
    // immediately return false
    if(! ((getClass() == CreatureClass::FIGHTER && getSecondClass() == CreatureClass::THIEF) ||
          (getClass() == CreatureClass::RANGER)) )
          return(false);

    // Allows us to do a blank check and see if the player's class is one of the heavy armor
    // restricted classes.
    if(skill.empty())
        return(true);


    bool mediumOK = (getClass() == CreatureClass::RANGER);

    for(auto i : ready) {
        if( i && (   i->isHeavyArmor() || (   !mediumOK && i->isMediumArmor()))) {
            printColor(fmt::format("You can't ^W{}^x while wearing heavy armor!\n", skill).c_str());
            printColor("^W%O^x would hinder your movement too much!\n", i);
            return(true);
        }
    }
    return(false);
}

void Player::validateId() {
    if(id.empty() || id == "-1") {
        setId(gServer->getNextPlayerId());
    }
}

//********************************************************************
//                      checkConfusion
//********************************************************************

bool Player::checkConfusion() {
    int     action=0, dmg=0;
    mType   targetType = PLAYER;
    char    atk[50];
    Creature* target=nullptr;
    Exit    *newExit=nullptr;
    BaseRoom* room = getRoomParent(), *bRoom=nullptr;
    CatRef  cr;


    if(!isEffected("confusion"))
        return(false);

    action = Random::get(1,4);
    switch(action) {
        case 1: // Stand confused

            broadcast(getSock(), room, "%M stands with a confused look on %s face.", this, hisHer());
            printColor("^BYou are confused and dizzy. You stand and look around cluelessly.\n");
            stun(Random::get(5,10));
            return(true);
            break;
        case 2: // Wander to random exit

            newExit = getFleeableExit();
            if(!newExit)
                return(false);
            bRoom = getFleeableRoom(newExit);
            if(!bRoom)
                return(false);

            printColor("^BWanderlust overtakes you.\n");
            printColor("^BYou wander aimlessly to the %s exit.\n", newExit->getCName());
            broadcast(getSock(), room, "%M wanders aimlessly to the %s exit.", this, newExit->getCName());
            deleteFromRoom();
            addToRoom(bRoom);
            doPetFollow();
            return(true);
            break;
        case 3: // Attack something randomly
            if(!checkAttackTimer(false))
                return(false);

            switch(Random::get(1,2)) {
                case 1:
                    target = getRandomMonster(room);
                    if(!target)
                        target = getRandomPlayer(room);
                    if(target) {
                        targetType = target->getType();
                        break;
                    }
                    break;
                case 2:
                    target = getRandomPlayer(room);
                    if(!target)
                        target = getRandomMonster(room);
                    if(target) {
                        targetType = target->getType();
                        break;
                    }
                    break;
                default:
                    break;
            }

            if(!target || target == this)
                targetType = INVALID;


            switch(targetType) {
                case PLAYER: // Random player in room
                    printColor("^BYou are convinced %s is trying to kill you!\n", target->getCName());
                    attackCreature(target);
                    return(true);
                    break;
                case INVALID: // Self

                    if(ready[WIELD-1]) {
                        dmg = ready[WIELD-1]->damage.roll() +
                              bonus(strength.getCur()) + ready[WIELD - 1]->getAdjustment();
                        printColor("^BYou frantically swing your weapon at imaginary enemies.\n");
                    } else {
                        printColor("^BYou madly flail around at imaginary enemies.\n");
                        if(cClass == CreatureClass::MONK) {
                            dmg = Random::get(1,2) + level/3 + Random::get(1,(1+level)/2);
                            if(strength.getCur() < 90) {
                                dmg -= (90-strength.getCur())/10;
                                dmg = MAX(1,dmg);
                            }
                        } else
                            dmg = damage.roll();
                    }

                    getDamageString(atk, this, ready[WIELD - 1]);

                    printColor("^BYou %s yourself for %d damage!\n", atk, dmg);

                    broadcast(getSock(), room, "%M %s %sself!", this, atk, himHer());
                    hp.decrease(dmg);
                    if(hp.getCur() < 1) {

                        hp.setCur(1);

                        printColor("^BYou accidentally killed yourself!\n");
                        broadcast("### Sadly, %s accidentally killed %sself.", getCName(), himHer());

                        mp.setCur(1);

                        if(!inJail()) {
                            bRoom = getLimboRoom().loadRoom(this);
                            if(bRoom) {
                                deleteFromRoom();
                                addToRoom(bRoom);
                                doPetFollow();
                            }
                        }
                    }
                    return(true);
                    break;
                default: // Random monster in room.
                    if(target->flagIsSet(M_UNKILLABLE))
                        return(false);

                    printColor("^BYou think %s is attacking you!\n", target);
                    broadcast(getSock(), room, "%M yells, \"DIE %s!!!\"\n", this, target->getCName());
                    attackCreature(target);
                    return(true);
                    break;
            }
            break;
        case 4: // flee
            printColor("^BPhantom dangers in your head beckon you to flee!\n");
            broadcast(getSock(), room, "%M screams in terror, pointing around at everything.", this);
            flee();
            return(true);
            break;
        default:
            break;
    }
    return(false);
}

//********************************************************************
//                      getEtherealTravelRoom
//********************************************************************

CatRef getEtherealTravelRoom() {
    const CatRefInfo* eth = gConfig->getCatRefInfo("et");
    CatRef cr;
    cr.setArea("et");
    cr.id = Random::get(1, eth->getTeleportWeight());
    return(cr);
}

//********************************************************************
//                      etherealTravel
//********************************************************************

void etherealTravel(Player* player) {
    UniqueRoom  *newRoom=nullptr;
    CatRef  cr = getEtherealTravelRoom();

    if(!loadRoom(cr, &newRoom))
        return;

    player->deleteFromRoom();
    player->addToRoom(newRoom);
    player->doPetFollow();
}

//********************************************************************
//                      cmdVisible
//********************************************************************

int cmdVisible(Player* player, cmd* cmnd) {
    if(!player->isInvisible()) {
        player->print("You are not invisible.\n");
        return(0);
    } else {
        player->removeEffect("invisibility");
        player->removeEffect("greater-invisibility");
    }
    return(0);
}

//********************************************************************
//                      cmdDice
//********************************************************************

int cmdDice(Creature* player, cmd* cmnd) {
    char    *str=nullptr, *tok=nullptr, diceOutput[256], add[256];
    int     strLen=0, i=0;
    int     diceSides=0,diceNum=0,diceAdd=0;
    int     rolls=0, total=0;

    const char *Syntax =    "\nSyntax: dice 1d2\n"
                            "        dice 1d2+3\n";

    strcpy(diceOutput, "");
    strcpy(add ,"");

    strLen = cmnd->fullstr.length();

    // This kills all leading whitespace
    while(i<strLen && isspace(cmnd->fullstr[i]))
        i++;
    // This kills the command itself
    while(i<strLen && !isspace(cmnd->fullstr[i]))
        i++;

    str = strstr(&cmnd->fullstr[i], "d");
    if(!str)
        return(cmdAction(player, cmnd));

    str = strdup(&cmnd->fullstr[i]);
    if(!str) {
        player->print(Syntax);
        return(0);
    }

    tok = strtok(str, "d");
    if(!tok) {
        player->print(Syntax);
        return(0);
    }
    diceNum = atoi(tok);

    tok = strtok(nullptr, "+");
    if(!tok) {
        player->print(Syntax);
        return(0);
    }
    diceSides = atoi(tok);

    tok = strtok(nullptr, "+");

    if(tok)
        diceAdd = atoi(tok);

    if(diceNum < 0) {
        player->print("How can you roll a negative number of dice?\n");
        return(0);
    }

    diceNum = MAX(1, diceNum);

    if(diceSides<2) {
        player->print("A die has a minimum of 2 sides.\n");
        return(0);
    }

    diceNum = MIN(100, diceNum);
    diceSides = MIN(100, diceSides);
    diceAdd = MAX(-100,MIN(100, diceAdd));


    sprintf(diceOutput, "%dd%d", diceNum, diceSides);
    if(diceAdd) {
        if(diceAdd > 0)
            sprintf(add, "+%d", diceAdd);
        else
            sprintf(add, "%d", diceAdd);
        strcat(diceOutput, add);
    }


    for(rolls=0;rolls<diceNum;rolls++)
        total += Random::get(1, diceSides);

    total += diceAdd;


    player->print("You roll %s\n: %d\n", diceOutput, total);
    broadcast(player->getSock(), player->getParent(), "(Dice %s): %M got %d.", diceOutput, player, total );

    return(0);
}

