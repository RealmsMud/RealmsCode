/*
 * exits.cpp
 *   Exit routines.
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

#include <boost/algorithm/string/case_conv.hpp>  // for to_lower
#include <cstring>                               // for strncmp, size_t
#include <list>                                  // for operator==, _List_it...
#include <sstream>                               // for operator<<, char_traits
#include <string>                                // for string, allocator
#include <string_view>                           // for string_view, operator<<

#include "area.hpp"                              // for MapMarker
#include "catRef.hpp"                            // for CatRef
#include "cmd.hpp"                               // for cmd
#include "color.hpp"                             // for stripColor
#include "effects.hpp"                           // for EffectInfo, Effects
#include "flags.hpp"                             // for X_CLAN_1, X_CLAN_10
#include "global.hpp"                            // for CreatureClass, NEUTRAL
#include "hooks.hpp"                             // for Hooks
#include "location.hpp"                          // for Location
#include "mudObjects/areaRooms.hpp"              // for AreaRoom
#include "mudObjects/creatures.hpp"              // for Creature
#include "mudObjects/exits.hpp"                  // for Exit, Direction, NoD...
#include "mudObjects/mudObject.hpp"              // for MudObject
#include "mudObjects/players.hpp"                // for Player
#include "mudObjects/rooms.hpp"                  // for BaseRoom, ExitList
#include "mudObjects/uniqueRooms.hpp"            // for UniqueRoom
#include "proto.hpp"                             // for zero, broadcast, fin...
#include "size.hpp"                              // for NO_SIZE, Size
#include "mud.hpp"                               // for LT_SPELL

//*********************************************************************
//                      Exit
//*********************************************************************

Exit::Exit() {

    level = trap = 0;
    zero(desc_key, sizeof(desc_key));
    clanFlags.reset();
    classFlags.reset();
    raceFlags.reset();
    key = toll = 0;
    size = NO_SIZE;
    flags.reset();
    keyArea = passphrase = open = enter = description = "";
    passlang = 0;
    size = NO_SIZE;
    parentRoom.reset();
    direction = NoDirection;
    usedBy = {};
}

Exit::~Exit() {
    if(!effects.effectList.empty()) {
        effects.removeAll();
    }
}

int exit_ordering(const char *exit1, const char *exit2);

bool Exit::operator< (const MudObject& t) const {
    return(exit_ordering(this->getCName(), t.getCName()));
}

short Exit::getLevel() const { return(level); }
std::string Exit::getOpen() const { return(open); }
short Exit::getTrap() const { return(trap); }
short Exit::getKey() const { return(key); }
std::string Exit::getKeyArea() const { return(keyArea); }
short Exit::getToll() const { return(toll); }
std::string Exit::getPassPhrase() const { return(passphrase); }
short Exit::getPassLanguage() const { return(passlang); }
std::string Exit::getDescription() const { return(description); }
Size Exit::getSize() const { return(size); }
Direction Exit::getDirection() const { return(direction); }
std::string Exit::getEnter() const { return(enter); }
std::shared_ptr<BaseRoom> Exit::getRoom() const { return(parentRoom.lock()); }

void Exit::setLevel(short lvl) { level = lvl; }
void Exit::setOpen(std::string_view o) { open = o; }
void Exit::setTrap(short t) { trap = t; }
void Exit::setKey(short k) { key = k; }
void Exit::setKeyArea(std::string_view k) { keyArea = k; }
void Exit::setToll(short t) { toll = t; }
void Exit::setPassPhrase(std::string_view phrase) { passphrase = phrase; }
void Exit::setPassLanguage(short lang) { passlang = lang; }
void Exit::setDescription(std::string_view d) { description = d; }
void Exit::setSize(Size s) { size = s; }
void Exit::setDirection(Direction d) { direction = d; }
void Exit::setEnter(std::string_view e) { enter = e; }
void Exit::setRoom(const std::shared_ptr<BaseRoom>& room) { parentRoom = room; }

// Checks if the exit is flagged to relock after being used, and do as such

void Exit::checkReLock(const std::shared_ptr<Creature>& creature, bool sneaking) {
    if(flagIsSet(X_LOCK_AFTER_USAGE) && !creature->isCt()) {
        bool nowLocked = false;
        bool nowClosed = false;
        if(flagIsSet(X_CLOSABLE) && !flagIsSet(X_CLOSED)) {
            setFlag(X_CLOSED);
            nowClosed = true;
        }

        if(flagIsSet(X_LOCKABLE) && !flagIsSet(X_LOCKED)) {
            setFlag(X_LOCKED);
            nowLocked = true;
        }
        std::string exitAction;
        if(nowClosed && nowLocked) {
            exitAction = "closes and locks itself";
        } else if (nowClosed) {
            exitAction = "closes itself";
        } else if(nowLocked) {
            exitAction = "locks";
        }
        if(nowClosed || nowLocked) {
            if(auto tRoom = parentRoom.lock()) {
                if (!sneaking) {
                    broadcast(creature->getSock(), tRoom, "The %s %s.^x", getCName(), exitAction.c_str());
                } else {
                    broadcast(isCt, creature->getSock(), tRoom, "*STAFF* The %s %s^x.", getCName(), exitAction.c_str());
                }
            }
        }
    }
}

bool Exit::flagIsSet(int flag) const {
    return flags.test(flag);
}
void Exit::setFlag(int flag) {
    flags.set(flag);
}
void Exit::clearFlag(int flag) {
    flags.reset(flag);
}
bool Exit::toggleFlag(int flag) {
    flags.flip(flag);
    return(flagIsSet(flag));
}

//*********************************************************************
//                      findExit
//*********************************************************************
// This function attempts to find the exit specified by the given string
// and value by looking through the exit list headed by the second para-
// meter.  If found, a pointer to the exit is returned.

std::shared_ptr<Exit> findExit(const std::shared_ptr<Creature>& creature, cmd* cmnd, int val, std::shared_ptr<BaseRoom> room) {
    return(findExit(creature, cmnd->str[val], cmnd->val[val], room));
}

std::shared_ptr<Exit> findExit(const std::shared_ptr<Creature>& creature, const std::string &inStr, int val, std::shared_ptr<BaseRoom> room) {
    int  match=0;
    auto str = stripColor(inStr);
    bool minThree = (creature->getAsPlayer() && !creature->isStaff() && str.length() < 3);

    if(!room && ((room = creature->getRoomParent()) == nullptr))
        return(nullptr);

    for(const auto& exit : room->exits) {
        auto name = stripColor(exit->getName());
        boost::algorithm::to_lower(name);

        if(!creature->isStaff()) {
            if(!creature->canSee(exit))
                continue;
            if( minThree && (exit->flagIsSet(X_CONCEALED) || exit->flagIsSet(X_SECRET)) && name.length() > 2 && !exit->hasBeenUsedBy(creature))
                continue;
        }

        if(!exit->flagIsSet(X_DESCRIPTION_ONLY) || creature->isStaff()) {
            if(name.starts_with(str))
                match++;
        } else {
            if(name == str)
                match++;
        }

        if(match == val)
            return(exit);
    }

    return(nullptr);
}

//*********************************************************************
//                          raceRestrict
//*********************************************************************

bool Exit::raceRestrict(const std::shared_ptr<const Creature> & creature) const {
    bool pass = false;

    // if no flags are set
    if( !flagIsSet(X_SEL_DWARF) &&
        !flagIsSet(X_SEL_ELF) &&
        !flagIsSet(X_SEL_HALFELF) &&
        !flagIsSet(X_SEL_HALFLING) &&
        !flagIsSet(X_SEL_HUMAN) &&
        !flagIsSet(X_SEL_ORC) &&
        !flagIsSet(X_SEL_HALFGIANT) &&
        !flagIsSet(X_SEL_GNOME) &&
        !flagIsSet(X_SEL_TROLL) &&
        !flagIsSet(X_SEL_HALFORC) &&
        !flagIsSet(X_SEL_OGRE) &&
        !flagIsSet(X_SEL_DARKELF) &&
        !flagIsSet(X_SEL_GOBLIN) &&
        !flagIsSet(X_SEL_MINOTAUR) &&
        !flagIsSet(X_SEL_SERAPH) &&
        !flagIsSet(X_SEL_KOBOLD) &&
        !flagIsSet(X_SEL_CAMBION) &&
        !flagIsSet(X_SEL_BARBARIAN) &&
        !flagIsSet(X_SEL_KATARAN) &&
        !flagIsSet(X_SEL_TIEFLING) &&
        !flagIsSet(X_SEL_KENKU) &&
        !flagIsSet(X_RSEL_INVERT) )
        return(false);

    // if the race flag is set and they match, they pass
    pass = (
        (flagIsSet(X_SEL_DWARF) && creature->isRace(DWARF)) ||
        (flagIsSet(X_SEL_ELF) && creature->isRace(ELF)) ||
        (flagIsSet(X_SEL_HALFELF) && creature->isRace(HALFELF)) ||
        (flagIsSet(X_SEL_HALFLING) && creature->isRace(HALFLING)) ||
        (flagIsSet(X_SEL_HUMAN) && creature->isRace(HUMAN)) ||
        (flagIsSet(X_SEL_ORC) && creature->isRace(ORC)) ||
        (flagIsSet(X_SEL_HALFGIANT) && creature->isRace(HALFGIANT)) ||
        (flagIsSet(X_SEL_GNOME) && creature->isRace(GNOME)) ||
        (flagIsSet(X_SEL_TROLL) && creature->isRace(TROLL)) ||
        (flagIsSet(X_SEL_HALFORC) && creature->isRace(HALFORC)) ||
        (flagIsSet(X_SEL_OGRE) && creature->isRace(OGRE)) ||
        (flagIsSet(X_SEL_DARKELF) && creature->isRace(DARKELF)) ||
        (flagIsSet(X_SEL_GOBLIN) && creature->isRace(GOBLIN)) ||
        (flagIsSet(X_SEL_MINOTAUR) && creature->isRace(MINOTAUR)) ||
        (flagIsSet(X_SEL_SERAPH) && creature->isRace(SERAPH)) ||
        (flagIsSet(X_SEL_KOBOLD) && creature->isRace(KOBOLD)) ||
        (flagIsSet(X_SEL_CAMBION) && creature->isRace(CAMBION)) ||
        (flagIsSet(X_SEL_BARBARIAN) && creature->isRace(BARBARIAN)) ||
        (flagIsSet(X_SEL_KATARAN) && creature->isRace(KATARAN)) ||
        (flagIsSet(X_SEL_TIEFLING) && creature->isRace(TIEFLING)) ||
        (flagIsSet(X_SEL_KENKU) && creature->isRace(KENKU))
    );

    if(flagIsSet(X_RSEL_INVERT)) pass = !pass;

    return(!pass);
}

//*********************************************************************
//                          classRestrict
//*********************************************************************

bool Exit::classRestrict(const std::shared_ptr<const Creature> & creature) const {
    bool pass = false;

    // if no flags are set
    if( !flagIsSet(X_SEL_ASSASSIN) &&
        !flagIsSet(X_SEL_BERSERKER) &&
        !flagIsSet(X_SEL_CLERIC) &&
        !flagIsSet(X_SEL_FIGHTER) &&
        !flagIsSet(X_SEL_MAGE) &&
        !flagIsSet(X_SEL_PALADIN) &&
        !flagIsSet(X_SEL_RANGER) &&
        !flagIsSet(X_SEL_THIEF) &&
        !flagIsSet(X_SEL_VAMPIRE) &&
        !flagIsSet(X_SEL_MONK) &&
        !flagIsSet(X_SEL_DEATHKNIGHT) &&
        !flagIsSet(X_SEL_DRUID) &&
        !flagIsSet(X_SEL_LICH) &&
        !flagIsSet(X_SEL_WEREWOLF) &&
        !flagIsSet(X_SEL_BARD) &&
        !flagIsSet(X_SEL_ROGUE) &&
        !flagIsSet(X_CSEL_INVERT) )
        return(false);

    // if the class flag is set and they match, they pass
    pass = (
        (flagIsSet(X_SEL_ASSASSIN) && creature->getClass() == CreatureClass::ASSASSIN) ||
        (flagIsSet(X_SEL_BERSERKER) && creature->getClass() == CreatureClass::BERSERKER) ||
        (flagIsSet(X_SEL_CLERIC) && creature->getClass() == CreatureClass::CLERIC) ||
        (flagIsSet(X_SEL_FIGHTER) && creature->getClass() == CreatureClass::FIGHTER) ||
        (flagIsSet(X_SEL_MAGE) && creature->getClass() == CreatureClass::MAGE) ||
        (flagIsSet(X_SEL_PALADIN) && creature->getClass() == CreatureClass::PALADIN) ||
        (flagIsSet(X_SEL_RANGER) && creature->getClass() == CreatureClass::RANGER) ||
        (flagIsSet(X_SEL_THIEF) && creature->getClass() == CreatureClass::THIEF) ||
        (flagIsSet(X_SEL_VAMPIRE) && creature->isEffected("vampirism")) ||
        (flagIsSet(X_SEL_MONK) && creature->getClass() == CreatureClass::MONK) ||
        (flagIsSet(X_SEL_DEATHKNIGHT) && creature->getClass() == CreatureClass::DEATHKNIGHT) ||
        (flagIsSet(X_SEL_DRUID) && creature->getClass() == CreatureClass::DRUID) ||
        (flagIsSet(X_SEL_LICH) && creature->getClass() == CreatureClass::LICH) ||
        (flagIsSet(X_SEL_WEREWOLF) && creature->isEffected("lycanthropy")) ||
        (flagIsSet(X_SEL_BARD) && creature->getClass() == CreatureClass::BARD) ||
        (flagIsSet(X_SEL_ROGUE) && creature->getClass() == CreatureClass::ROGUE)
    );

    if(flagIsSet(X_CSEL_INVERT)) pass = !pass;

    return(!pass);
}

//*********************************************************************
//                      clanRestrict
//*********************************************************************

bool Exit::clanRestrict(const std::shared_ptr<const Creature> & creature) const {

    // if the exit requires pledging, let any clan pass
    if(flagIsSet(X_PLEDGE_ONLY))
        return(creature->getClan()!=0);

    // if no flags are set
    if( !flagIsSet(X_CLAN_1) &&
        !flagIsSet(X_CLAN_2) &&
        !flagIsSet(X_CLAN_3) &&
        !flagIsSet(X_CLAN_4) &&
        !flagIsSet(X_CLAN_5) &&
        !flagIsSet(X_CLAN_6) &&
        !flagIsSet(X_CLAN_7) &&
        !flagIsSet(X_CLAN_8) &&
        !flagIsSet(X_CLAN_9) &&
        !flagIsSet(X_CLAN_10) &&
        !flagIsSet(X_CLAN_11) &&
        !flagIsSet(X_CLAN_12) )
        return(false);

    // if the clan flag is set and they match, they pass
    if( (flagIsSet(X_CLAN_1) && creature->getClan() == 1) ||
        (flagIsSet(X_CLAN_2) && creature->getClan() == 2) ||
        (flagIsSet(X_CLAN_3) && creature->getClan() == 3) ||
        (flagIsSet(X_CLAN_4) && creature->getClan() == 4) ||
        (flagIsSet(X_CLAN_5) && creature->getClan() == 5) ||
        (flagIsSet(X_CLAN_6) && creature->getClan() == 6) ||
        (flagIsSet(X_CLAN_7) && creature->getClan() == 7) ||
        (flagIsSet(X_CLAN_8) && creature->getClan() == 8) ||
        (flagIsSet(X_CLAN_9) && creature->getClan() == 9) ||
        (flagIsSet(X_CLAN_10) && creature->getClan() == 10) ||
        (flagIsSet(X_CLAN_11) && creature->getClan() == 11) ||
        (flagIsSet(X_CLAN_12) && creature->getClan() == 12) )
        return(false);

    return(true);
}

//*********************************************************************
//                      alignRestrict
//*********************************************************************

bool Exit::alignRestrict(const std::shared_ptr<const Creature> & creature) const {
    return(
        (flagIsSet(X_NO_GOOD_ALIGN) && creature->getAdjustedAlignment() > NEUTRAL) ||
        (flagIsSet(X_NO_EVIL_ALIGN) && creature->getAdjustedAlignment() < NEUTRAL) ||
        (flagIsSet(X_NO_NEUTRAL_ALIGN) && creature->getAdjustedAlignment() == NEUTRAL)
    );
}

//*********************************************************************
//                      getReturnExit
//*********************************************************************
// Tries to find a return exit. Will only return an exit if there is
// only one unique exit back to the parent room.

std::shared_ptr<Exit> Exit::getReturnExit(const std::shared_ptr<const BaseRoom>& parent, std::shared_ptr<BaseRoom> &targetRoom) const {
    (*targetRoom) = *target.loadRoom();
    if(!targetRoom)
        return(nullptr);

    std::shared_ptr<Exit> exit=nullptr;
    bool found = false;

    const std::shared_ptr<const AreaRoom> aRoom = parent->getAsConstAreaRoom();
    const std::shared_ptr<const UniqueRoom> uRoom = parent->getAsConstUniqueRoom();

    for(const auto& ext : (targetRoom)->exits) {
        if(ext->target.mapmarker.getArea()) {
            if(aRoom && ext->target.mapmarker == aRoom->mapmarker) {
                if(found)
                    return(nullptr);
                exit = ext;
                found = true;
            }
        } else {
            if(uRoom && ext-> target.room == uRoom->info) {
                if(found)
                    return(nullptr);
                exit = ext;
                found = true;
            }
        }
    }

    return(exit);
}

//*********************************************************************
//                      blockedByStr
//*********************************************************************

std::string Exit::blockedByStr(char color, std::string_view spell, std::string_view effectName, bool detectMagic, bool canSee) const {
    EffectInfo* effect = nullptr;
    std::ostringstream oStr;

    if(canSee)
        oStr << "^" << color << "The " << getName() << "^" << color << " is blocked by a " << spell;
    else
        oStr << "^" << color << "A " << spell << " stands in the room";

    if(detectMagic) {
        effect = getEffect(effectName);
        if(!effect->getOwner().empty())
            oStr << " (cast by " << effect->getOwner() << ")";
    }

    oStr << ".\n";
    return(oStr.str());
}

//*********************************************************************
//                      showExit
//*********************************************************************

bool Player::showExit(const std::shared_ptr<Exit>& exit, int magicShowHidden) const {
    if(isStaff())
        return(true);
    if(exit->isDiscoverable() && exit->hasBeenUsedBy(Containable::downcasted_shared_from_this<Player>())){
        return(true);
    }
    return( canSee(exit) &&
        (!exit->flagIsSet(X_SECRET) || magicShowHidden) &&
        (!exit->isConcealed(Containable::downcasted_shared_from_this<Creature>()) || magicShowHidden) &&
        !exit->flagIsSet(X_DESCRIPTION_ONLY) &&
        (isEffected("fly") ? 1:!exit->flagIsSet(X_NEEDS_FLY))
    );
}

//*********************************************************************
//                      addEffectReturnExit
//*********************************************************************
// Add an effect to the given exit and the return exit (ie, an exit in
// the room it points to that points back to this room)

void Exit::addEffectReturnExit(const std::string &effect, long duration, int strength, const std::shared_ptr<Creature> & owner) {
    std::shared_ptr<BaseRoom> targetRoom=nullptr;

    addEffect(effect, duration, strength, nullptr, true, owner);

    targetRoom = target.loadRoom();
    if (!targetRoom) 
        return;

    // switch the meaning of exit
    std::shared_ptr<Exit> exit = getReturnExit(owner->getConstRoomParent(), targetRoom);
    if(exit)
        exit->addEffect(effect, duration, strength, nullptr, true, owner);
}

//*********************************************************************
//                      removeEffectReturnExit
//*********************************************************************
// Add an effect to the given exit and the return exit (ie, an exit in
// the room it points to that points back to this room)

void Exit::removeEffectReturnExit(const std::string &effect, const std::shared_ptr<BaseRoom>& rParent) {
    std::shared_ptr<BaseRoom> targetRoom=nullptr;

    removeEffect(effect, true, false);
    targetRoom = target.loadRoom();
    if (!targetRoom)
        return;

    // switch the meaning of exit
    std::shared_ptr<Exit> exit = getReturnExit(rParent, targetRoom);
    if(exit)
        exit->removeEffect(effect, true, false);
}

//*********************************************************************
//                      isWall
//*********************************************************************

bool Exit::isWall(std::string_view name) const {
    EffectInfo* effect = getEffect(name);
    if(!effect)
        return(false);
    // a wall of force with "extra" set is temporarily disabled
    if(effect->getExtra())
        return(false);
    return(true);
}

//*********************************************************************
//                      isConcealed
//*********************************************************************

bool Exit::isConcealed(const std::shared_ptr<const Creature> & viewer) const {
    if(flagIsSet(X_CONCEALED))
        return(true);
    if(isEffected("concealed") && (!viewer || !viewer->willIgnoreIllusion()))
        return(true);
    return(false);
}

//*********************************************************************
//                      isDiscoverable
//*********************************************************************

bool Exit::isDiscoverable() const {
    return(
        !flagIsSet(X_NO_REMEMBER) &&
        (
            flagIsSet(X_SECRET) ||
            flagIsSet(X_DESCRIPTION_ONLY) ||
            flagIsSet(X_CONCEALED) ||
            flagIsSet(X_NEEDS_FLY) ||
            isEffected("invisibility")
        )
    );
}

//**********************************************************************
//                      hasBeenUsedBy
//**********************************************************************

bool Exit::hasBeenUsedBy(const std::string& id) const {
    return std::find(usedBy.begin(), usedBy.end(), id) != usedBy.end();
}
bool Exit::hasBeenUsedBy(const std::shared_ptr<const Creature> & creature) const {
    return(hasBeenUsedBy(creature->getId()));
}

//*********************************************************************
//                      getDir
//*********************************************************************

Direction getDir(std::string str) {
    size_t n = str.length();
    if(!n)
        return(NoDirection);
    str = stripColor(str);

    if(!strncmp(str.c_str(), "north", n))
        return(North);
    else if(!strncmp(str.c_str(), "east", n))
        return(East);
    else if(!strncmp(str.c_str(), "south", n))
        return(South);
    else if(!strncmp(str.c_str(), "west", n))
        return(West);
    else if(str == "ne" || !strncmp(str.c_str(), "northeast", n))
        return(Northeast);
    else if(str == "nw" || !strncmp(str.c_str(), "northwest", n))
        return(Northwest);
    else if(str == "se" || !strncmp(str.c_str(), "southeast", n))
        return(Southeast);
    else if(str == "sw" || !strncmp(str.c_str(), "southwest", n))
        return(Southwest);
    return(NoDirection);
}

//*********************************************************************
//                      getDirName
//*********************************************************************

std::string getDirName(Direction dir) {
    switch(dir) {
    case North:
        return("north");
    case East:
        return("east");
    case South:
        return("south");
    case West:
        return("west");
    case Northeast:
        return("northeast");
    case Northwest:
        return("northwest");
    case Southeast:
        return("southeast");
    case Southwest:
        return("southwest");
    default:
        return("none");
    }
}

//*********************************************************************
//                      hisHer
//*********************************************************************

const char *Exit::hisHer() const {
    return("it");
}

//*********************************************************************
//                      himHer
//*********************************************************************

const char *Exit::himHer() const {
    return("it");
}

//*********************************************************************
//                      heShe
//*********************************************************************

const char *Exit::heShe() const {
    return("it");
}

//*********************************************************************
//                      upHisHer
//*********************************************************************

const char *Exit::upHisHer() const {
    return("It");
}

//*********************************************************************
//                      upHeShe
//*********************************************************************

const char *Exit::upHeShe() const {
    return("It");
}