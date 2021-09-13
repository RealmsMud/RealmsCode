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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

// C includes
#include <arpa/telnet.h>          // for IAC, EOR, GA
#include <cstdio>                // for sprintf, rename
#include <cstdlib>               // for free, abs
#include <cstring>               // for strcpy, strlen, strcat
#include <ctime>                 // for time, ctime
#include <unistd.h>               // for unlink
#include <iomanip>                // for operator<<, setw

#include "area.hpp"               // for Area, MapMarker, MAX_VISION
#include "bstring.hpp"            // for bstring
#include "catRef.hpp"             // for CatRef
#include "catRefInfo.hpp"         // for CatRefInfo
#include "clans.hpp"              // for Clan
#include "commands.hpp"           // for cmdSave
#include "config.hpp"             // for Config, gConfig
#include "creatures.hpp"          // for Player, Creature, Monster, PetList
#include "deityData.hpp"          // for DeityData
#include "dice.hpp"               // for Dice
#include "effects.hpp"            // for EffectInfo
#include "flags.hpp"              // for P_DM_INVIS, P_CHAOTIC, O_DARKNESS
#include "global.hpp"             // for CreatureClass, CreatureClass::CLERIC
#include "guilds.hpp"             // for Guild
#include "hooks.hpp"              // for Hooks
#include "location.hpp"           // for Location
#include "magic.hpp"              // for S_ARMOR, S_REJUVENATE
#include "money.hpp"              // for GOLD, Money
#include "move.hpp"               // for getRoom
#include "mud.hpp"                // for LT, LT_PLAYER_SEND, LT_AGE, LT_PLAY...
#include "objects.hpp"            // for Object, ObjectType, ObjectType::CON...
#include "paths.hpp"              // for Help, Bank, DMHelp, History, Player
#include "property.hpp"           // for Property
#include "proto.hpp"              // for bonus, broadcast, abortFindRoom
#include "raceData.hpp"           // for RaceData
#include "random.hpp"             // for Random
#include "realm.hpp"              // for Realm
#include "rooms.hpp"              // for UniqueRoom, BaseRoom, AreaRoom, Exi...
#include "server.hpp"             // for Server, gServer, PlayerMap
#include "size.hpp"               // for NO_SIZE, SIZE_MEDIUM
#include "skillGain.hpp"          // for SkillGain
#include "socket.hpp"             // for Socket
#include "startlocs.hpp"          // for StartLoc
#include "stats.hpp"              // for Stat, MOD_CUR
#include "unique.hpp"             // for remove, deleteOwner
#include "utils.hpp"              // for MIN, MAX
#include "xml.hpp"                // for loadRoom


//********************************************************************
//              fixLts
//********************************************************************

void Creature::fixLts() {
    long tdiff=0, t = time(nullptr);
    int i=0;
    if(isPet())  {
        tdiff = t - getMaster()->lasttime[LT_AGE].ltime;
    }
    else
        tdiff = t - lasttime[LT_AGE].ltime;
    for(i=0; i<MAX_LT; i++) {
        if(i == LT_JAILED)
            continue;
        // Fix pet fade on login here
        if(lasttime[i].ltime == 0 && lasttime[i].interval == 0)
            continue;
        lasttime[i].ltime += tdiff;
        lasttime[i].ltime = MIN(t, lasttime[i].ltime);
    }
}

//********************************************************************
//              init
//********************************************************************
// This function initializes a player's stats and loads up the room he
// should be in. This should only be called when the player first
// logs on.

void Player::init() {
    char    file[80], str[50], watchers[128];
    BaseRoom *newRoom=nullptr;
    long    t = time(nullptr);
    int     watch=0;

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



    /* change this later
       strength.getCur() = strength.max;
    dexterity.getCur() = dexterity.max;
    constitution.getCur() = constitution.max;
    intelligence.getCur() = intelligence.max;
    piety.getCur() = piety.max;
    */
    if(!isStaff()) {
        daily[DL_ENCHA].max = 3;
        daily[DL_FHEAL].max = MAX(3, 3 + (level) / 3);
        daily[DL_TRACK].max = MAX(3, 3 + (level) / 3);
        daily[DL_DEFEC].max = 1;
        //  daily[DL_TELEP].max = MAX(3, MAX(2, level/4));
        daily[DL_TELEP].max = 3;
        if(level < 13)
            daily[DL_TELEP].max = 1;
        //  daily[DL_ETRVL].max = MAX(3, MAX(2, level/4));
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



    //if((race != KATARAN && race != TROLL))
    learnLanguage(LCOMMON); // All races speak common but troll and kataran.

    //Barbarians get natural warmth. They're from the frozen tundra.
    if(race == BARBARIAN) {
       addPermEffect("warmth"); 
    }

    if(!current_language) {
        initLanguages();
        current_language = LCOMMON;
        setFlag(P_LANGUAGE_COLORS);
    }



    //  Paladins get auto know-aur
    if(cClass==CreatureClass::PALADIN)
        addPermEffect("know-aura");
    //  Mages get auto d-m
    if( (   cClass==CreatureClass::MAGE ||
            cClass2 == CreatureClass::MAGE
        ) &&
        !(  cClass == CreatureClass::MAGE &&
            cClass2 == CreatureClass::ASSASSIN
        ))
    {
        addPermEffect("detect-magic");
    }

    if(cClass == CreatureClass::DRUID && level >= 7)
        addPermEffect("pass-without-trace");

    if(cClass == CreatureClass::THIEF && cClass2 == CreatureClass::MAGE)
        addPermEffect("detect-magic");

    if((cClass == CreatureClass::MAGE || cClass2 == CreatureClass::MAGE) && level >= 7)
        learnSpell(S_ARMOR);

    if(cClass == CreatureClass::CLERIC && deity == CERIS && level >= 13)
        learnSpell(S_REJUVENATE);


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
        if( p &&
            p->getType() == PROP_STORAGE &&
            !p->isOwner(getName()) &&
            !p->isPartialOwner(getName()) )
        {
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
            loge("%s: %s (%s) Attempted logon to bad or missing room!\n", getCName(),
                getSock()->getHostname().c_str(), currentLocation.room.str().c_str());
            // NOTE: Using ::isCt to use the global function, not the local function
            broadcast(::isCt, "^y%s: %s (%s) Attempted logon to bad or missing room (normal)!", getCName(),
                getSock()->getHostname().c_str(), currentLocation.room.str().c_str());
            newRoom = abortFindRoom(this, "init_ply");
            uRoom = newRoom->getAsUniqueRoom();
        }


        if(uRoom && !isStaff() && !gServer->isRebooting()) {
            if( (   uRoom->flagIsSet(R_LOG_INTO_TRAP_ROOM) ||
                    uRoom->flagIsSet(R_SHOP_STORAGE) ||
                    uRoom->hasTraining()
                ) &&
                uRoom->getTrapExit().id &&
                !loadRoom(uRoom->getTrapExit(), &uRoom)
            ) {
                broadcast(::isCt, "^y%s: %s (%s) Attempted logon to bad or missing room!", getCName(),
                    getSock()->getHostname().c_str(), uRoom->getTrapExit().str().c_str());
                newRoom = abortFindRoom(this, "init_ply");
                uRoom = newRoom->getAsUniqueRoom();
            }

            if( uRoom &&
                (   uRoom->isFull() ||
                    uRoom->flagIsSet(R_NO_LOGIN) ||
                    (!isStaff() && !flagIsSet(P_PTESTER) && uRoom->isConstruction()) ||
                    (!isStaff() && uRoom->flagIsSet(R_SHOP_STORAGE))
                )
            ) {
                newRoom = getRecallRoom().loadRoom(this);
                if(!newRoom) {
                    broadcast(::isCt, "^y%s: %s (%s) Attempted logon to bad or missing room!", getCName(),
                        getSock()->getHostname().c_str(), getRecallRoom().str().c_str());
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
        loge("%s(L:%d) (%s) %s. Room - %s (Port-%d)\n", getCName(), level,
             getSock()->getHostname().c_str(), gServer->isRebooting() ? "reloaded" : "logged on",
                     newRoom->fullName().c_str(), Port);
    }
    if(isStaff())
        logn("log.imm", "%s  (%s) %s.\n",
                getCName(), getSock()->getHostname().c_str(),
             gServer->isRebooting() ? "reloaded" : "logged on");


    // broadcast
    if(!gServer->isRebooting()) {
        setSockColors();
        broadcast_login(this, newRoom, 1);
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

    if(!gServer->isRebooting())
        bug("%s logged into room %s.\n", getCName(), getRoomParent()->fullName().c_str());


    wearCursed();
    if(!flagIsSet(P_NO_AUTO_WEAR))
        wearAll(this, true);

    computeLuck();
    update();

    Socket* sock = getSock();

    if(!gServer->isRebooting()) {
        sprintf(file, "%s/news.txt",Path::Help);
        sock->viewLoginFile(file);

        sprintf(file, "%s/newbie_news.txt",Path::Help);
        sock->viewLoginFile(file);

        if(isCt()) {
            sprintf(file, "%s/news.txt", Path::DMHelp);
            sock->viewLoginFile(file);
        }
        if(isStaff() && getName() != "Bane") {
            sprintf(file, "%s/news.txt", Path::BuilderHelp);
            sock->viewLoginFile(file);
        }
        if(isCt() || flagIsSet(P_WATCHER)) {
            sprintf(file, "%s/watcher_news.txt", Path::DMHelp);
            sock->viewLoginFile(file);
        }

        sprintf(file, "%s/latest_post.txt", Path::Help);
        sock->viewLoginFile(file, false);

        hasNewMudmail();
    }

    if(!gServer->isRebooting()) {
        strcpy(watchers, "");
        printColor("^yWatchers currently online: ");
        Player* ply;
        for(const auto& p : gServer->players) {
            ply = p.second;

            if(!ply->isConnected())
                continue;
            if(!ply->isPublicWatcher())
                continue;
            if(!canSee(ply))
                continue;

            strcat(watchers, ply->getCName());
            strcat(watchers, ", ");
            watch++;
        }

        if(watch) {
            watchers[strlen(watchers) - 2] = '.';
            watchers[strlen(watchers) - 1] = 0;
            printColor("%s\n", watchers);
        } else
            printColor("None.\n");
    }

    if(isCt())
        showGuildsNeedingApproval(this);

    if(hp.getCur() < 0)
        hp.setCur(1);


    // only players and builders are effected
    if(!isCt()) {
        // players can't set eavesdropper flag anymore
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
        print("You have %d weapon train(s) available!\n", weaponTrains);


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

    if(!gServer->isRebooting())
        broadcast_login(this, this->getRoomParent(), 0);

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
//                      checkEffectsWearingOff
//*********************************************************************

void Player::checkEffectsWearingOff() {
    long t = time(nullptr);
    int staff = isStaff();

    // Added P_STUNNED and LT_PLAYER_STUNNED stun for dodge code.
    if(flagIsSet(P_STUNNED)) {
        if(t > LT(this, LT_PLAYER_STUNNED)) {
            clearFlag(P_STUNNED);
        }
    }

    if(flagIsSet(P_FOCUSED) && (cClass == CreatureClass::MONK || staff)) {
        if(t > LT(this, LT_FOCUS)) {
            printColor("^cYou lose your concentration.\n");
            clearFlag(P_FOCUSED);
            //computeAC();
            computeAttackPower();
        }
    }
    if(flagIsSet(P_MISTBANE)) {
        if(t > LT(this, LT_FOCUS)) {
            printColor("^bYour mistbane is ended.\n");
            clearFlag(P_MISTBANE);
        }
    }

    if(flagIsSet(P_OUTLAW)) {
        if(t > LT(this, LT_OUTLAW)) {
            printColor("^yYou are no longer an outlaw.\n");
            clearFlag(P_OUTLAW);
            clearFlag(P_OUTLAW_WILL_BE_ATTACKED);
            setFlag(P_NO_SUMMON);
            clearFlag(P_OUTLAW_WILL_LOSE_XP);
            clearFlag(P_NO_GET_ALL);
        }
    }

    // only force them to wake from sleeping unconsciousness when they're
    // completely healed.
    if(flagIsSet(P_UNCONSCIOUS)) {
        if( (   !flagIsSet(P_SLEEPING) &&
                t > LT(this, LT_UNCONSCIOUS)
            ) ||
            (   flagIsSet(P_SLEEPING) && (
                (hp.getCur() >= hp.getMax() && mp.getCur() >= mp.getMax()) ||
                (cClass == CreatureClass::PUREBLOOD && !getRoomParent()->vampCanSleep(getSock()))
            ) )
        ) {
            printColor("^cYou wake up.\n");
            clearFlag(P_UNCONSCIOUS);
            wake();

            clearFlag(P_DIED_IN_DUEL);
            broadcast(getSock(), getRoomParent(), "%M wakes up.", this);
        }
    }

    if(flagIsSet(P_NO_SUICIDE)) {
        if(t > LT(this, LT_MOBDEATH)) {
            printColor("^yYour cooling-off period has ended.\n");
            clearFlag(P_NO_SUICIDE);
        }
    }
    if(flagIsSet(P_HIDDEN) && !staff) {
        if(t - lasttime[LT_HIDE].ltime > 300L) {
            printColor("^cShifting shadows expose you.\n");
            unhide(false);
        }
    }
    if(flagIsSet(P_FREE_ACTION)) {
        if(t > LT(this, LT_FREE_ACTION)) {
            printColor("^c^#You no longer magically move freely.\n");
            clearFlag(P_FREE_ACTION);
            computeAC();
            computeAttackPower();
        }
    }

    if(flagIsSet(P_NO_PKILL)) {
        if(t > LT(this, LT_NO_PKILL)) {
            printColor("^c^#You can now be pkilled again.\n");
            clearFlag(P_NO_PKILL);
        }
    }



    if(flagIsSet(P_DOCTOR_KILLER)) {
        if(t > LT(this, LT_KILL_DOCTOR) || staff) {
            printColor("^y^#The doctors have forgiven you.\n");
            clearFlag(P_DOCTOR_KILLER);
        }
    }


    if(flagIsSet(P_NO_TICK_MP)) {
        if(t > LT(this, LT_NOMPTICK) || staff) {
            printColor("^cYour magical vitality has returned.\n");
            clearFlag(P_NO_TICK_MP);
        }

    }

    if(flagIsSet(P_NO_TICK_HP)) {
        if(t > LT(this, LT_NOHPTICK) || staff) {
            printColor("^gYou now heal normally again.\n");
            clearFlag(P_NO_TICK_HP);
        }
    }

    if(flagIsSet(P_CANT_BROADCAST)) {
        if(t > LT(this, LT_NO_BROADCAST)) {
            printColor("^rYou can broadcast again, now don't abuse it this time.\n");
            clearFlag(P_CANT_BROADCAST);
        }
    }

    if(isEffected("mist")) {
        if(isDay() && !staff)
            unmist();
    }
    if(flagIsSet(P_CHARMED)) {
        if(t > LT(this, LT_CHARMED) || staff) {
            printColor("^yYou are again in control of your actions.\n");
            clearFlag(P_CHARMED);
        }
    }

    if(negativeLevels) {
        if(t > LT(this, LT_LEVEL_DRAIN) || staff) {
            long expTemp=0;
            negativeLevels--;
            if(negativeLevels) {
                printColor("^WYou have regained a lost level.\n");
                expTemp = experience;
                upLevel();
                experience = expTemp;

                lasttime[LT_LEVEL_DRAIN].ltime = t;
                lasttime[LT_LEVEL_DRAIN].interval = 60L + 5*bonus(constitution.getCur());
            } else {
                printColor("^WYou have recovered all your lost levels.\n");
                expTemp = experience;
                upLevel();
                experience = expTemp;
            }
        }
    }

    if(t > LT(this, LT_JAILED) && flagIsSet(P_JAILED)) {
        printColor("^rA demonic jailer just arrived.\n");
        printColor("The demonic jailer says, \"You have been released from your torment.\"\n");
        printColor("The demonic jailer casts word of recall on you.\n");

        broadcast(getSock(), getRoomParent(), "A demonic jailer just arrived.\nThe demonic jailer casts word of recall on %s.", getCName());
        broadcast(getSock(), getRoomParent(), "The demonic jailer sneers evilly and spits on you.\nThe demonic jailer vanishes.");
        broadcast("^R### Cackling demons shove %s from the Dungeon of Despair.", getCName());
        doRecall();

        clearFlag(P_JAILED);
    }


    if( t > LT(this, LT_MOB_JAILED) &&
        inUniqueRoom() && getUniqueRoomParent()->flagIsSet(R_MOB_JAIL) &&
        !staff
    ) {
        printColor("A jailer just arrived.\n");
        printColor("The jailer says, \"You're free to go...get out!\"\n");
        printColor("The jailer opens the cell door and shoves you out.\n");
        printColor("The jailer goes back to napping.\n");

        doRecall();
    }
}

//*********************************************************************
//                      doPetrificationDmg
//*********************************************************************

bool Creature::doPetrificationDmg() {
    if(!isEffected("petrification"))
        return(false);

    wake("Terrible nightmares disturb your sleep!");
    printColor("^c^#Petrification spreads toward your heart.\n");
    hp.decrease(MAX<int>(1,(hp.getMax()/15 - bonus(constitution.getCur()))));

    if(hp.getCur() < 1) {
        Player* pThis = getAsPlayer();
        if(pThis)
            pThis->die(PETRIFIED);
        else
            die(this);
        return(true);
    }
    return(false);
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
//                      addObj
//*********************************************************************
// This function adds the object pointer to by the first parameter to
// the inventory of the player pointed to by the second parameter.

void Creature::addObj(Object* object) {
    Player* pPlayer = getAsPlayer();

    object->validateId();

    Hooks::run(this, "beforeAddObject", object, "beforeAddToCreature");
    object->clearFlag(O_JUST_LOADED);

    // players have big inventories; to keep the mud from searching them when it
    // doesnt need to, record a flag on the player
    if(pPlayer && object->flagIsSet(O_DARKMETAL))
        setFlag(P_DARKMETAL);
    if(object->flagIsSet(O_DARKNESS))
        setFlag(pPlayer ? P_DARKNESS : M_DARKNESS);

    object->addTo(this);
    //add(object);

    if(pPlayer)
        pPlayer->updateItems(object);

    Hooks::run(this, "afterAddObject", object, "afterAddToCreature");

    killDarkmetal();
}

//*********************************************************************
//                      finishDelObj
//*********************************************************************
// This function removes the object pointer to by the first parameter
// from the player pointed to by the second. This does NOT DELETE THE
// OBJECT. You will have to do that yourself, if desired.

// we put in a choice to do darkmetal or not so we won't have
// recursion problems
// if you pass false to darkness, you MUST run checkDarkness() unless
// you are certain the item you are deleted isn't flagged O_DARKNESS

void Creature::finishDelObj(Object* object, bool breakUnique, bool removeUnique, bool darkmetal, bool darkness, bool keep) {
    if(darkmetal)
        killDarkmetal();
    if(breakUnique || removeUnique) {
        Player* player = getPlayerMaster();
        if(player) {
            if(breakUnique)
                Limited::remove(player, object);
            else if(removeUnique)
                Limited::deleteOwner(player, object);
        }
    }
    if(darkness)
        checkDarkness();
    if(!keep)
        object->clearFlag(O_KEEP);

    Hooks::run(this, "afterRemoveObject", object, "afterRemoveFromCreature");
}

//*********************************************************************
//                      delObj
//*********************************************************************

void Creature::delObj(Object* object, bool breakUnique, bool removeUnique, bool darkmetal, bool darkness, bool keep) {
    Hooks::run(this, "beforeRemoveObject", object, "beforeRemoveFromCreature");

    // don't run checkDarkness if this isnt a dark item
    if(!object->flagIsSet(O_DARKNESS))
        darkness = false;
    object->clearFlag(O_BEING_PREPARED);
    object->clearFlag(O_HIDDEN);
    object->clearFlag(O_JUST_LOADED);

    // if it doesnt have a parent_crt, it's either being worn or is in a bag
    if(!object->inCreature()) {
        // the object is being worn
        if(object->getWearflag() && ready[object->getWearflag()-1] == object) {
            unequip(object->getWearflag(), UNEQUIP_NOTHING, false);
            finishDelObj(object, breakUnique, removeUnique, darkmetal, darkness, keep);
        } else {
            // the object is in a bag somewhere
            // problem is, we don't know which bag
            for(Object* obj : objects) {
                if(obj->getType() == ObjectType::CONTAINER) {
                    for(Object* subObj : obj->objects ) {
                        if(subObj == object) {
                            obj->delObj(object);
                            finishDelObj(object, breakUnique, removeUnique, darkmetal, darkness, keep);
                            return;
                        }
                    }
                }
            }

            // not in their inventory? they must be wearing a bag
            for(auto & i : ready) {
                if(!i)
                    continue;
                if(i->getType() == ObjectType::CONTAINER) {
                    for(Object* obj : i->objects) {
                        if(obj == object) {
                            i->delObj(object);
                            finishDelObj(object, breakUnique, removeUnique, darkmetal, darkness, keep);
                            return;
                        }
                    }
                }
            }
        }
        return;
    }
    object->removeFrom();
    finishDelObj(object, breakUnique, removeUnique, darkmetal, darkness, keep);
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
//                      mprofic
//*********************************************************************
// This function returns the magical realm proficiency as a percentage

int mprofic(const Creature* player, int index) {
    const Player *pPlayer = player->getAsConstPlayer();
    long    prof_array[12];
    int i=0, n=0, prof=0;

    switch(player->getClass()) {
    case CreatureClass::MAGE:
        if(pPlayer && (pPlayer->getSecondClass() == CreatureClass::ASSASSIN || pPlayer->getSecondClass() == CreatureClass::THIEF)) {
            prof_array[0] = 0L;
            prof_array[1] = 1024L;
            prof_array[2] = 4092L;
            prof_array[3] = 8192L;
            prof_array[4] = 16384L;
            prof_array[5] = 32768L;
            prof_array[6] = 70536L;
            prof_array[7] = 119000L;
            prof_array[8] = 226410L;
            prof_array[9] = 709410L;
            prof_array[10] = 2973307L;
            prof_array[11] = 500000000L;
        } else {
            prof_array[0] = 0L;
            prof_array[1] = 1024L;
            prof_array[2] = 2048L;
            prof_array[3] = 4096L;
            prof_array[4] = 8192L;
            prof_array[5] = 16384L;
            prof_array[6] = 35768L;
            prof_array[7] = 85536L;
            prof_array[8] = 140000L;
            prof_array[9] = 459410L;
            prof_array[10] = 2073306L;
            prof_array[11] = 500000000L;

        }
        break;
    case CreatureClass::LICH:
        prof_array[0] = 0L;
        prof_array[1] = 1024L;
        prof_array[2] = 2048L;
        prof_array[3] = 4096L;
        prof_array[4] = 8192L;
        prof_array[5] = 16384L;
        prof_array[6] = 35768L;
        prof_array[7] = 85536L;
        prof_array[8] = 140000L;
        prof_array[9] = 459410L;
        prof_array[10] = 2073306L;
        prof_array[11] = 500000000L;
        break;
    case CreatureClass::CLERIC:
        if(pPlayer && (pPlayer->getSecondClass() == CreatureClass::ASSASSIN || pPlayer->getSecondClass() == CreatureClass::FIGHTER)) {
            prof_array[0] = 0L;
            prof_array[1] = 1024L;
            prof_array[2] = 8192L;
            prof_array[3] = 16384L;
            prof_array[4] = 32768L;
            prof_array[5] = 65536L;
            prof_array[6] = 105000L;
            prof_array[7] = 165410L;
            prof_array[8] = 287306L;
            prof_array[9] = 809410L;
            prof_array[10] = 3538232L;
            prof_array[11] = 500000000L;
        } else {
            prof_array[0] = 0L;
            prof_array[1] = 1024L;
            prof_array[2] = 4092L;
            prof_array[3] = 8192L;
            prof_array[4] = 16384L;
            prof_array[5] = 32768L;
            prof_array[6] = 70536L;
            prof_array[7] = 119000L;
            prof_array[8] = 226410L;
            prof_array[9] = 709410L;
            prof_array[10] = 2973307L;
            prof_array[11] = 500000000L;
        }
        break;
    case CreatureClass::THIEF:
        if(pPlayer && pPlayer->getSecondClass() == CreatureClass::MAGE) {
            prof_array[0] = 0L;
            prof_array[1] = 1024L;
            prof_array[2] = 8192L;
            prof_array[3] = 16384L;
            prof_array[4] = 32768L;
            prof_array[5] = 65536L;
            prof_array[6] = 105000L;
            prof_array[7] = 165410L;
            prof_array[8] = 287306L;
            prof_array[9] = 809410L;
            prof_array[10] = 3538232L;
            prof_array[11] = 500000000L;
        } else {
            prof_array[0] = 0L;
            prof_array[1] = 1024L;
            prof_array[2] = 40000L;
            prof_array[3] = 80000L;
            prof_array[4] = 120000L;
            prof_array[5] = 160000L;
            prof_array[6] = 205000L;
            prof_array[7] = 222000L;
            prof_array[8] = 380000L;
            prof_array[9] = 965410L;
            prof_array[10] = 5495000;
            prof_array[11] = 500000000L;
        }
        break;

    case CreatureClass::FIGHTER:
        if(pPlayer && pPlayer->getSecondClass() == CreatureClass::MAGE) {
            prof_array[0] = 0L;
            prof_array[1] = 1024L;
            prof_array[2] = 8192L;
            prof_array[3] = 16384L;
            prof_array[4] = 32768L;
            prof_array[5] = 65536L;
            prof_array[6] = 105000L;
            prof_array[7] = 165410L;
            prof_array[8] = 287306L;
            prof_array[9] = 809410L;
            prof_array[10] = 3538232L;
            prof_array[11] = 500000000L;
        } else {
            prof_array[0] = 0L;
            prof_array[1] = 1024L;
            prof_array[2] = 40000L;
            prof_array[3] = 80000L;
            prof_array[4] = 120000L;
            prof_array[5] = 160000L;
            prof_array[6] = 205000L;
            prof_array[7] = 222000L;
            prof_array[8] = 380000L;
            prof_array[9] = 965410L;
            prof_array[10] = 5495000;
            prof_array[11] = 500000000L;
        }
        break;
    case CreatureClass::PALADIN:
    case CreatureClass::BARD:
    case CreatureClass::PUREBLOOD:
    case CreatureClass::DRUID:
        prof_array[0] = 0L;
        prof_array[1] = 1024L;
        prof_array[2] = 4092L;
        prof_array[3] = 8192L;
        prof_array[4] = 16384L;
        prof_array[5] = 32768L;
        prof_array[6] = 70536L;
        prof_array[7] = 119000L;
        prof_array[8] = 226410L;
        prof_array[9] = 709410L;
        prof_array[10] = 2973307L;
        prof_array[11] = 500000000L;
        break;
    case CreatureClass::DEATHKNIGHT:
    case CreatureClass::MONK:
    case CreatureClass::RANGER:
        prof_array[0] = 0L;
        prof_array[1] = 1024L;
        prof_array[2] = 8192L;
        prof_array[3] = 16384L;
        prof_array[4] = 32768L;
        prof_array[5] = 65536L;
        prof_array[6] = 105000L;
        prof_array[7] = 165410L;
        prof_array[8] = 287306L;
        prof_array[9] = 809410L;
        prof_array[10] = 3538232L;
        prof_array[11] = 500000000L;
        break;
    default:
        prof_array[0] = 0L;
        prof_array[1] = 1024L;
        prof_array[2] = 40000L;
        prof_array[3] = 80000L;
        prof_array[4] = 120000L;
        prof_array[5] = 160000L;
        prof_array[6] = 205000L;
        prof_array[7] = 222000L;
        prof_array[8] = 380000L;
        prof_array[9] = 965410L;
        prof_array[10] = 5495000;
        prof_array[11] = 500000000L;
        break;
    }

    n = player->getRealm((Realm)index);
    for(i=0; i<11; i++)
        if(n < prof_array[i+1]) {
            prof = 10*i;
            break;
        }

    prof += ((n - prof_array[i])*10) / (prof_array[i+1] - prof_array[i]);

    return(prof);
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
            if ((ready[i]->getType() == ObjectType::LIGHTSOURCE &&
                ready[i]->getShotsCur() > 0) ||
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

//***********************************************************************
//                      sendPrompt
//***********************************************************************
// This function returns the prompt that the player should be seeing

void Player::sendPrompt() {
    bstring toPrint;

    if(fd < 0)
        return;

    // no prompt in this situation
    if(flagIsSet(P_SPYING) || flagIsSet(P_READING_FILE))
        return;

    if(flagIsSet(P_PROMPT) || flagIsSet(P_ALIASING)) {
        std::ostringstream promptStr;

        if(!flagIsSet(P_NO_EXTRA_COLOR))
            promptStr << alignColor();

        if(flagIsSet(P_ALIASING) && alias_crt) {
            promptStr << "(" << alias_crt->hp.getCur() << " H " << alias_crt->mp.getCur() << " M): ";
        } else {
            promptStr << "(" << hp.getCur() << " H";
            if(cClass != CreatureClass::LICH && cClass != CreatureClass::BERSERKER
                    && (cClass != CreatureClass::FIGHTER || !flagIsSet(P_PTESTER)))
                promptStr << " " << mp.getCur() << " M";
            else if(cClass == CreatureClass::FIGHTER && flagIsSet(P_PTESTER))
                promptStr << " " << focus.getCur() << " F";

            if(flagIsSet(P_SHOW_XP_IN_PROMPT))
                promptStr << " " << expToLevel(true);
            promptStr << "):^x ";

        }
        toPrint = promptStr.str();
    } else
        toPrint = ": ";

    if(flagIsSet(P_AFK))
        toPrint += "^r[AFK]^x ";
    if(flagIsSet(P_NEWLINE_AFTER_PROMPT))
        toPrint += "\n";

    // Send EOR if they want it, otherwise send GA
    if(getSock()->getEor() == 1) {
        unsigned char eor_str[] = {IAC, EOR, '\0' };
        toPrint += eor_str;
    } else if(!getSock()->isDumbClient()){
        unsigned char ga_str[] = {IAC, GA, '\0' };
        toPrint += ga_str;
    }

    mySock->write(toPrint);
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
//                      getStatusStr
//*********************************************************************
// returns a status string that describes the hp condition of the creature

const char* Creature::getStatusStr(int dmg) {
    int health = hp.getCur() - dmg;

    if(health < 1)
        return "'s dead!";

    switch(MIN<int>(health * 10 / (hp.getMax() ? hp.getMax() : 1), 10)) {
    case 10:
        return("'s unharmed.");
    case 9:
        return("'s relatively unscathed.");
    case 8:
        return("'s a little battered.");
    case 7:
        return("'s getting bruised.");
    case 6:
        return("'s noticeably bleeding.");
    case 5:
        return("'s having some trouble.");
    case 4:
        return(" doesn't look too good.");
    case 3:
        return("'s beginning to stagger.");
    case 2:
        return(" has some nasty wounds.");
    case 1:
        return(" isn't going to last much longer.");
    case 0:
        return(" is about to die!");
    }
    return("");
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
//                      recallWhere
//*********************************************************************
// Because of ethereal plane, we don't always know where we're going to
// recall to. We need a function to figure out where we are going.

BaseRoom* Creature::recallWhere() {
    // A builder should never get this far, but let's not chance it.
    // Only continue if they can't load the perm_low_room.
    if(cClass == CreatureClass::BUILDER) {
        UniqueRoom* uRoom=nullptr;
        CatRef cr;
        cr.setArea("test");
        cr.id = 1;
        if(loadRoom(cr, &uRoom))
            return(uRoom);
    }

    if( getRoomParent()->flagIsSet(R_ETHEREAL_PLANE) &&
        (Random::get(1,100) <= 50)
    ) {
        return(teleportWhere());
    }

    BaseRoom* room = getRecallRoom().loadRoom(getAsPlayer());
    // uh oh!
    if(!room)
        return(abortFindRoom(this, "recallWhere"));
    return(room);

}

//*********************************************************************
//                      teleportWhere
//*********************************************************************
// Loops through rooms and finds us a place we can teleport to.
// This function will always return a room or it will crash trying to.

BaseRoom* Creature::teleportWhere() {
    BaseRoom *newRoom=nullptr;
    const CatRefInfo* cri = gConfig->getCatRefInfo(getRoomParent());
    int     i=0, zone = cri ? cri->getTeleportZone() : 0;
    Area    *area=nullptr;
    Location l;
    bool    found = false;



    // A builder should never get this far, but let's not chance it.
    // Only continue if they can't load the perm_low_room.
    if(cClass == CreatureClass::BUILDER) {
        CatRef cr;
        cr.setArea("test");
        cr.id = 1;
        UniqueRoom* uRoom =nullptr;
        if(loadRoom(cr, &uRoom))
            return(uRoom);
    }

    do {
        if(i>250)
            return(abortFindRoom(this, "teleportWhere"));
        cri = gConfig->getRandomCatRefInfo(zone);

        // if this fails, we have nowhere to teleport to
        if(!cri)
            return(getRoomParent());

        // special area used to signify overland map
        if(cri->getArea() == "area") {
            area = gServer->getArea(cri->getId());
            l.mapmarker.set(area->id, Random::get<short>(0, area->width), Random::get<short>(0, area->height), Random::get<short>(0, area->depth));
            if(area->canPass(nullptr, &l.mapmarker, true)) {
                //area->adjustCoords(&mapmarker.x, &mapmarker.y, &mapmarker.z);

                // don't bother sending a creature because we've already done
                // canPass check here
                //aRoom = area->loadRoom(0, &mapmarker, false);
                if(Move::getRoom(this, nullptr, &newRoom, false, &l.mapmarker)) {
                    if(newRoom->isUniqueRoom()) {
                        // recheck, just to be safe
                        found = newRoom->getAsUniqueRoom()->canPortHere(this);
                        if(!found)
                            newRoom = nullptr;
                    } else {
                        found = true;
                    }
                }
            }
        } else {
            l.room.setArea(cri->getArea());
            // if misc, first 1000 rooms are off-limits
            l.room.id = Random::get(l.room.isArea("misc") ? 1000 : 1, cri->getTeleportWeight());
            UniqueRoom* uRoom = nullptr;

            if(loadRoom(l.room, &uRoom))
                found = uRoom->canPortHere(this);
            if(found)
                newRoom = uRoom;
        }

        i++;
    } while(!found);

    if(!newRoom)
        return(abortFindRoom(this, "teleportWhere"));
    return(newRoom);
}


//*********************************************************************
//                      canPortHere
//*********************************************************************

bool UniqueRoom::canPortHere(const Creature* creature) const {
    // check creature-specific settings
    if(creature) {
        if(size && creature->getSize() && creature->getSize() > size)
            return(false);
        if(deityRestrict(creature))
            return(false);
        if(getLowLevel() > creature->getLevel())
            return(false);
        if(getHighLevel() && creature->getLevel() > getHighLevel())
            return(false);
    }

    // check room-specific settings
    if( flagIsSet(R_NO_TELEPORT) ||
        flagIsSet(R_LIMBO) ||
        flagIsSet(R_VAMPIRE_COVEN) ||
        flagIsSet(R_SHOP_STORAGE) ||
        flagIsSet(R_JAIL) ||
        flagIsSet(R_IS_STORAGE_ROOM) ||
        flagIsSet(R_ETHEREAL_PLANE) ||
        isConstruction() ||
        hasTraining()
    )
        return(false);
    if(isFull())
        return(false);
    // artificial limits for the misc area
    if(info.isArea("misc") && info.id <= 1000)
        return(false);
    if(exits.empty())
        return(false);

    return(true);
}

//*********************************************************************
//                      checkSkillsGain
//*********************************************************************
// setToLevel: Set skill level to player level - 1, otherwise set to whatever the skill gain tells us to

void Creature::checkSkillsGain(const std::list<SkillGain*>::const_iterator& begin, const std::list<SkillGain*>::const_iterator& end, bool setToLevel) {
    SkillGain *sGain=nullptr;
    std::list<SkillGain*>::const_iterator sgIt;
    for(sgIt = begin ; sgIt != end ; sgIt++) {
        sGain = (*sgIt);
        if(sGain->getName().empty())
            continue;
        if(!sGain->hasDeities() || sGain->deityIsAllowed(deity)) {
            if(!knowsSkill(sGain->getName())) {
                if(setToLevel)
                    addSkill(sGain->getName(), (level-1)*10);
                else
                    addSkill(sGain->getName(), sGain->getGained());
                print("You have learned the fine art of '%s'.\n", gConfig->getSkillDisplayName(sGain->getName()).c_str());
            }
        }
    }
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

bstring Player::getWhoString(bool whois, bool color, bool ignoreIllusion) const {
    std::ostringstream whoStr;

    if(whois) {
        whoStr << "^bWhois for [" << getName() << "]\n";
        whoStr << "----------------------------------------------------------------------------------\n";
    }

    whoStr << (color ? "^x[^c" : "[") << std::setw(2) << level
           << ":" << std::setw(4) << bstring(getShortClassName(this)).left(4)
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


    if( flagIsSet(P_DM_INVIS) ||
        isEffected("incognito") ||
        isInvisible() ||
        isEffected("mist") ||
        (flagIsSet(P_LINKDEAD) && !isPublicWatcher())
    ) {
        if(color)
            whoStr << " ^w";
        if(flagIsSet(P_DM_INVIS))
            whoStr << "[+]";
        if(isEffected("incognito") )
            whoStr << "[g]";
        if(isInvisible() )
            whoStr << "[*]";
        if(isEffected("mist") )
            whoStr << "[m]";
        if(flagIsSet(P_LINKDEAD) && !isPublicWatcher() )
            whoStr << "[l]";
    }


    if(flagIsSet(P_AFK))
        whoStr << (color ? "^R" : "") << " [AFK]";

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

bstring Player::expToLevel(bool addX) const {
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

bstring Player::expInLevel() const {
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
bstring Player::expForLevel() const {
    int displayLevel = level;
    if(level > MAXALVL) {
        displayLevel = MAXALVL;
    }

    std::ostringstream oStr;
    oStr << (Config::expNeeded(displayLevel) - Config::expNeeded(displayLevel-1));
    return(oStr.str());
}

bstring Player::expNeededDisplay() const {
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

bool Player::exists(const bstring& name) {
    char    file[80];
    sprintf(file, "%s/%s.xml", Path::Player, name.c_str());
    return(file_exists(file));
}

//*********************************************************************
//                      inList functions
//*********************************************************************

bool Player::inList(const std::list<bstring>* list, const bstring& name) const {
    std::list<bstring>::const_iterator it;

    for(it = list->begin(); it != list->end() ; it++) {
        if((*it) == name)
            return(true);
    }
    return(false);
}


bool Player::isIgnoring(const bstring& name) const {
    return(inList(&ignoring, name));
}
bool Player::isGagging(const bstring& name) const {
    return(inList(&gagging, name));
}
bool Player::isRefusing(const bstring& name) const {
    return(inList(&refusing, name));
}
bool Player::isDueling(const bstring& name) const {
    return(inList(&dueling, name));
}
bool Player::isWatching(const bstring& name) const {
    return(inList(&watching, name));
}

//*********************************************************************
//                      showList
//*********************************************************************

bstring Player::showList(const std::list<bstring>* list) const {
    std::ostringstream oStr;
    std::list<bstring>::const_iterator it;
    bool initial=false;

    for(it = list->begin(); it != list->end() ; it++) {
        if(initial)
            oStr << ", ";
        initial = true;

        oStr << (*it);
    }

    if(!initial)
        oStr << "No one";
    oStr << ".";

    return(oStr.str());
}


bstring Player::showIgnoring() const {
    return(showList(&ignoring));
}
bstring Player::showGagging() const {
    return(showList(&gagging));
}
bstring Player::showRefusing() const {
    return(showList(&refusing));
}
bstring Player::showDueling() const {
    return(showList(&dueling));
}
bstring Player::showWatching() const {
    return(showList(&watching));
}

//*********************************************************************
//                      addList functions
//*********************************************************************

void Player::addList(std::list<bstring>* list, const bstring& name) {
    list->push_back(name);
}


void Player::addIgnoring(const bstring& name) {
    addList(&ignoring, name);
}
void Player::addGagging(const bstring& name) {
    addList(&gagging, name);
}
void Player::addRefusing(const bstring& name) {
    addList(&refusing, name);
}
void Player::addDueling(const bstring& name) {
    delList(&maybeDueling, name);

    // if they aren't dueling us, add us to their maybe dueling list
    Player* player = gServer->findPlayer(name);
    if(player && !player->isDueling(name))
        player->addMaybeDueling(getName());

    addList(&dueling, name);
}
void Player::addMaybeDueling(const bstring& name) {
    addList(&maybeDueling, name);
}
void Player::addWatching(const bstring& name) {
    addList(&watching, name);
}

//*********************************************************************
//                      delList functions
//*********************************************************************

void Player::delList(std::list<bstring>* list, const bstring& name) {
    std::list<bstring>::iterator it;

    for(it = list->begin(); it != list->end() ; it++) {
        if((*it) == name) {
            list->erase(it);
            return;
        }
    }
}


void Player::delIgnoring(const bstring& name) {
    delList(&ignoring, name);
}
void Player::delGagging(const bstring& name) {
    delList(&gagging, name);
}
void Player::delRefusing(const bstring& name) {
    delList(&refusing, name);
}
void Player::delDueling(const bstring& name) {
    delList(&dueling, name);
}
void Player::delWatching(const bstring& name) {
    delList(&watching, name);
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
    std::list<bstring>::iterator it;

    Player* player=nullptr;
    for(it = maybeDueling.begin(); it != maybeDueling.end() ; it++) {
        player = gServer->findPlayer(*it);
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

bool Player::checkHeavyRestrict(const bstring& skill) const {
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
        if( i &&
            (   i->isHeavyArmor() ||
                (   !mediumOK &&
                    i->isMediumArmor()
                )
            )
        ) {
            printColor("You can't ^W%s^x while wearing heavy armor!\n", skill.c_str());
            printColor("^W%O^x would hinder your movement too much!\n", i);
            return(true);
        }
    }
    return(false);
}


void Player::validateId() {
    if(id.empty() || id.equals("-1")) {
        setId(gServer->getNextPlayerId());
    }
}


