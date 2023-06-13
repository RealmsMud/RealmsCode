/*
 * creatures.cpp
 *   Functions that work on creatures etc
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
// Mud Includes

#include <cstdlib>                     // for abs
#include <cstring>                     // for strlen, strcat, strcpy
#include <ctime>                       // for time
#include <set>                         // for operator==, _Rb_tree_const_ite...
#include <sstream>                     // for operator<<, basic_ostream, ost...
#include <string>                      // for string, operator==, allocator

#include "area.hpp"                    // for MapMarker, Area
#include "calendar.hpp"                // for Calendar
#include "catRef.hpp"                  // for CatRef
#include "cmd.hpp"                     // for cmd
#include "commands.hpp"                // for tollcost
#include "config.hpp"                  // for Config, gConfig
#include "deityData.hpp"               // for DeityData
#include "enums/loadType.hpp"          // for LoadType
#include "flags.hpp"                   // for P_DM_INVIS, O_SMALL_SHIELD
#include "global.hpp"                  // for HELD, WIELD, SHIELD, ISDM, MAX...
#include "lasttime.hpp"                // for lasttime, crlasttime, operator<<
#include "location.hpp"                // for Location
#include "money.hpp"                   // for GOLD, Money
#include "monType.hpp"                 // for mtype stuff
#include "mud.hpp"                     // for LT_AGE
#include "mudObjects/container.hpp"    // for ObjectSet, Container, MonsterSet
#include "mudObjects/creatures.hpp"    // for Creature
#include "mudObjects/exits.hpp"        // for Exit
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/mudObject.hpp"    // for MudObject
#include "mudObjects/objects.hpp"      // for Object, ObjectType, ObjectType...
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "property.hpp"                // for Property
#include "proto.hpp"                   // for isDay, int_to_text, getOrdinal
#include "quests.hpp"                  // for QuestEligibility, QuestTurninS...
#include "raceData.hpp"                // for RaceData
#include "realm.hpp"                   // for WATER
#include "season.hpp"                  // for AUTUMN, SPRING, SUMMER, WINTER
#include "server.hpp"                  // for Server, gServer
#include "size.hpp"                    // for getSizeName, SIZE_COLOSSAL
#include "stats.hpp"                   // for Stat
#include "structs.hpp"                 // for Command, SEX_FEMALE, SEX_MALE


//********************************************************************
//                      canSee
//********************************************************************

bool Creature::canSeeRoom(const std::shared_ptr<BaseRoom>& room, bool p) const {
    if(isStaff())
        return(true);
    const std::shared_ptr<const Player> player = getAsConstPlayer();

    // blind people can't see anything
    if(isBlind()) {
        if(p) {
            printColor("^CYou're blind!^x\n");
            printColor("^yIt's too dark to see.^x\n");
        }
        return(false);
    }

    // magic vision overcomes all darkness
    if(isEffected("infravision"))
        return(true);

    // magic dark can only be overcome by magic vision
    if(room->isMagicDark()) {
        if(p) {
            printColor("^CIt's unnaturally dark here.\n");
            printColor("^yIt's too dark to see.\n");
        }
        return(false);
    }

    // room is affected by normal darkness
    if(room->isNormalDark()) {
        // there are several sources of normal vision
        bool    normal_sight = gConfig->getRace(race)->hasInfravision() ||
            isUndead() || isEffected("lycanthropy") || (player && player->getLight());

        // if they can't see, maybe someone else in the room has light for them
        if(!normal_sight) {
            for(const auto& pIt: room->players) {
                if(auto ply = pIt.lock()) {
                    if (ply->getAsPlayer()->getLight()) {
                        normal_sight = true;
                        break;
                    }
                }
            }
        }

        if(!normal_sight) {
            if(p)
                printColor("^yIt's too dark to see.\n");
            return(false);
        }
    }

    // not any form of dark, then they can see
    return(true);
}

bool Creature::canSee(const std::shared_ptr<const MudObject> target, bool skip) const {
    if(!target)
        return(false);

    if(target->isCreature()) {
        const std::shared_ptr<const Creature> & cTarget = target->getAsConstCreature();
        if(cTarget->isPlayer()) {
            if(cTarget->flagIsSet(P_DM_INVIS) && getClass() < cTarget->getClass()) return(false);
            if(target->isEffected("incognito") && (getClass() < cTarget->getClass()) && getParent() != cTarget->getParent()) return(false);

            if(!skip) {
                if(cTarget->isInvisible() && !isEffected("detect-invisible") && !isStaff()) return(false);
                if(target->isEffected("mist") && !isEffected("true-sight") && !isStaff()) return(false);
            }

        } else {

            if(!skip) {
                if(cTarget->isInvisible() && !isEffected("detect-invisible") && !isStaff()) return(false);
            }

        }

    } // End Creature
    if(target->isExit()) {
        const std::shared_ptr<const Exit> &exit = target->getAsConstExit();
        if(isStaff()) return(true);

        // handle NoSee right away
        if(exit->flagIsSet(X_NO_SEE)) return(false);
        if(exit->isEffected("invisibility") && !isEffected("detect-invisible")) return(false);
    }
    if(target->isObject()) {
        const std::shared_ptr<const Object> &object = target->getAsConstObject();

        if(isStaff()) return(true);
        if(object->isEffected("invisibility") && !isEffected("detect-invisible")) return(false);

    }
    return(true);
}


//********************************************************************
//                      canEnter
//********************************************************************
// Determines if a player/monster can enter the exit provided. If p is not 0,
// it will also display a reason why. This only handles absolutely-unable-to-
// enter. M_BLOCK_EXIT is not handled because players can still enter those
// exits under certain circumstances (ie, flee).
//
// Pets are handled differently. They obey special rules for rooms because
// returning false here would prevent the player from entering the room.

bool Creature::canEnter(const std::shared_ptr<Exit>& exit, bool p, bool blinking) const {
    const Calendar* calendar=nullptr;
    int staff = isStaff();

    if(!exit)
        return(false);

    if(isMonster())
        p = false;

    if(exit->target.mapmarker.getArea()) {
        std::shared_ptr<Area> area = gServer->getArea(exit->target.mapmarker.getArea());
        if(!area)
            return(false);
        // y coords are stored upside down in the array
        // impassable terrain
        if(!area->canPass(Containable::downcasted_shared_from_this<Creature>(), exit->target.mapmarker, true)) {
            if(p) checkStaff("You can't go there!\n");
            if(!staff) return(false);
        }
    }


    // pets can go anywhere
    if(isPet())
        return(true);

    if(!blinking && exit->isWall("wall-of-force")) {
        if(p) checkStaff("^sA wall of force blocks passage in that direction.\n");
        if(!staff) return(false);
    }
    if(exit->flagIsSet(X_CLOSED)) {
        if(p) checkStaff("You have to open it first.\n");
        if(!staff) return(false);
    }
    if(exit->flagIsSet(X_LOCKED)) {
        if(p) checkStaff("It's locked.\n");
        if(!staff) return(false);
    }

    if(exit->flagIsSet(X_STAFF_ONLY) || exit->flagIsSet(X_NO_SEE)) {
        if(p) checkStaff("You cannot go that way.\n");
        if(!staff) return(false);
    }

    if(exit->flagIsSet(X_LOOK_ONLY)) {
        if(p) checkStaff("You may only look through that exit.\n");
        if(!staff) return(false);
    }


    if(!blinking && exit->getSize() && size > exit->getSize()) {
        if(p) checkStaff("Only %s and smaller creatures may go there.\n", getSizeName(exit->getSize()).c_str());
        if(!staff) return(false);
    }


    if( (exit->flagIsSet(X_MALE_ONLY) && getSex() != SEX_MALE) ||
        (exit->flagIsSet(X_FEMALE_ONLY) && getSex() != SEX_FEMALE)
    ) {
        if(p) checkStaff("Your sex prevents you from going that way.\n");
        if(!staff) return(false);
    }

    if( (exit->flagIsSet(X_NIGHT_ONLY) && isDay()) ||
        (exit->flagIsSet(X_DAY_ONLY) && !isDay())
    ) {
        if(p) checkStaff("You may not go that way %s.\n", isDay() ? "during the day" : "at night");
        if(!staff) return(false);
    }

    if( getConstRoomParent()->getTollkeeper() &&
        (exit->flagIsSet(X_TOLL_TO_PASS) || exit->flagIsSet(X_LEVEL_BASED_TOLL))
    ) {
        if(p) checkStaff("You must pay a toll of %lu gold coins to go through the %s^x.\n", tollcost(this->getAsConstPlayer(), exit, nullptr), exit->getCName());
        if(!staff) return(false);
    }

    auto cThis = Containable::downcasted_shared_from_this<Creature>();
    if(exit->clanRestrict(cThis)) {
        if(p) checkStaff("Your allegiance prevents you from going that way.\n");
        if(!staff) return(false);
    }
    if(exit->classRestrict(cThis)) {
        if(p) checkStaff("Your class prevents you from going that way.\n");
        if(!staff) return(false);
    }
    if(exit->raceRestrict(cThis)) {
        if(p) checkStaff("Your race prevents you from going that way.\n");
        if(!staff) return(false);
    }
    if(exit->alignRestrict(cThis)) {
        if(p) checkStaff("Your alignment prevents you from going that way.\n");
        if(!staff) return(false);
    }

    calendar = gConfig->getCalendar();
    if(exit->flagIsSet(X_WINTER) && calendar->whatSeason() == WINTER) {
        if(p) checkStaff("Heavy winter snows prevent you from going that way.\n");
        if(!staff) return(false);
    }
    if(exit->flagIsSet(X_SPRING) && calendar->whatSeason() == SPRING) {
        if(p) checkStaff("Spring floods prevent you from going that way.\n");
        if(!staff) return(false);
    }
    if(exit->flagIsSet(X_SUMMER) && calendar->whatSeason() == SUMMER) {
        if(p) checkStaff("The summer heat prevent you from going that way.\n");
        if(!staff) return(false);
    }
    if(exit->flagIsSet(X_AUTUMN) && calendar->whatSeason() == AUTUMN) {
        if(p) checkStaff("You cannot go that way in autumn.\n");
        if(!staff) return(false);
    }



    // we are a player
    if(isPlayer()) {

        if(exit->flagIsSet(X_NAKED) && getWeight()) {
            if(p) checkStaff("You cannot bring anything through that exit.\n");
            if(!staff) return(false);
        }

        if(!canSee(exit)) {
            if(p) checkStaff("You don't see that exit.\n");
            if(!staff) return(false);
        }

        if(exit->flagIsSet(X_NEEDS_FLY) && !isEffected("fly")) {
            if(p) checkStaff("You must fly to go that way.\n");
            if(!staff) return(false);
        }

        if(exit->flagIsSet(X_NO_MIST) && isEffected("mist")) {
            if(p) checkStaff("You may not go that way in mist form.\n");
            if(!staff) return(false);
        }

        if(exit->flagIsSet(X_MIST_ONLY) && !isEffected("mist")) {
            if(p) checkStaff(getAsConstPlayer()->canMistNow() ? "You must turn to mist before you can go that way.\n" : "You cannot fit through that exit.\n");
            if(!staff) return(false);
        }


        const std::shared_ptr<const Monster>  guard = getConstRoomParent()->getGuardingExit(exit, getAsConstPlayer());
        if(guard) {
            if(p) checkStaff("%M %s.\n", guard.get(), guard->flagIsSet(M_FACTION_NO_GUARD) ? "doesn't like you enough to let you go there" : "blocks your exit");
            if(!staff) return(false);
        }

    // we are a monster
    } else {

        if(exit->flagIsSet(X_MIST_ONLY))
            return(false);
        if(exit->flagIsSet(X_NO_WANDER))
            return(false);
        if(exit->flagIsSet(X_TO_STORAGE_ROOM))
            return(false);
        if(exit->flagIsSet(X_TO_BOUND_ROOM))
            return(false);

    }

    return(true);
}



//********************************************************************
//                      canEnter
//********************************************************************

bool Creature::canEnter(const std::shared_ptr<UniqueRoom>& room, bool p) const {

    // staff may always go anywhere
    bool staff = isStaff();

    if(!room) {
        if(p) print("Off the map in that direction.\n");
        if(staff) printColor("^eThat room does not exist.\n");
        return(false);
    }

    // special rules for pets
    if(isPet()) {
        const std::shared_ptr<const Monster>  mThis = getAsConstMonster();
        return !(room->isUnderwater() &&
                 mThis->getBaseRealm() != WATER &&
                 !isEffected("breathe-water") &&
                 !doesntBreathe());
    }


    if(isMonster())
        p = false;

    if(size && room->getSize() && size > room->getSize()) {
        if(p) checkStaff("Only %s and smaller creatures may go there.\n", getSizeName(room->getSize()).c_str());
        if(!staff) return(false);
    }

    // we are a player
    if(isPlayer()) {

        if(room->flagIsSet(R_SHOP_STORAGE)) {
            if(p) checkStaff("You cannot enter a shop's storage room.\n");
            if(!staff) return(false);
        }
        if(room->getLowLevel() > level) {
            if(p) checkStaff("You must be at least level %d to go there.\n", room->getLowLevel());
            if(!staff) return(false);
        }
        if(level > room->getHighLevel() && room->getHighLevel()) {
            if(p) checkStaff("Only players under level %d may go there.\n", room->getHighLevel()+1);
            if(!staff) return(false);
        }
        if(room->deityRestrict(Containable::downcasted_shared_from_this<Creature>())) {
            if(p) checkStaff("Only members of the proper faith may go there.\n");
            if(!staff) return(false);
        }
        if(room->isFull()) {
            if(p) checkStaff("That room is full.\n");
            if(!staff) return(false);
        }
        if(room->flagIsSet(R_NO_MIST) && isEffected("mist")) {
            if(p) checkStaff("You may not enter there in mist form.\n");
            if(!staff) return(false);
        }
        if(!staff && !flagIsSet(P_PTESTER) && room->isConstruction()) {
            if(p) print("Off the map in that direction.\n");
            return(false);
        }
        if(!staff && room->flagIsSet(R_SHOP_STORAGE)) {
            if(p) print("Off the map in that direction.\n");
            return(false);
        }

        if(!Property::canEnter(getAsConstPlayer(), room, p))
            return(false);

    // we are a monster
    } else {

        // no rules for monsters

    }

    return(true);
}


//********************************************************************
//                      getWeight
//********************************************************************
// This function calculates the total weight that a player (or monster)
// is carrying in their inventory.

int Creature::getWeight() const {
    int     i=0, n=0;
    for(const auto& obj : objects) {
        if(!obj->flagIsSet(O_WEIGHTLESS_CONTAINER))
            n += obj->getActualWeight();
    }

    for(i=0; i<MAXWEAR; i++)
        if(ready[i])
            n += ready[i]->getActualWeight();

    return(n);
}


//********************************************************************
//                      maxWeight
//********************************************************************
// This function returns the maximum weight a player can be allowed to
// hold in their inventory.

int Creature::maxWeight() {
    int n = (3*(20 + strength.getCur())/2);
    
    if(cClass == CreatureClass::BERSERKER || isCt())
        n += level*10;
    return(n);
}

//********************************************************************
//                      tooBulky
//********************************************************************

bool Creature::tooBulky(int n) const {
    if(isCt())
        return(false);

    return(n + getTotalBulk() > getMaxBulk());
}

//********************************************************************
//                      getTotalBulk
//********************************************************************

int Creature::getTotalBulk() const {
    int     n=0, i=0;

    for(const auto& obj : objects) {
        n += obj->getActualBulk();
    }

    for(i=0; i<MAXWEAR; i++)
        if(ready[i])
            n += (ready[i]->getActualBulk()/2);

    return(n);
}

//********************************************************************
//                      getMaxBulk
//********************************************************************

int Creature::getMaxBulk() const {
    switch(size) {
    case SIZE_FINE:
        return(15);
    case SIZE_DIMINUTIVE:
        return(45);
    case SIZE_TINY:
        return(90);
    case SIZE_SMALL:
        return(158);
    case SIZE_LARGE:
        return(278);
    case SIZE_HUGE:
        return(323);
    case SIZE_GARGANTUAN:
        return(368);
    case SIZE_COLOSSAL:
        return(413);
    case SIZE_MEDIUM:
    default:
        return(210);
    }
}

//********************************************************************
//                      willFit
//********************************************************************

bool Creature::willFit(const std::shared_ptr<Object>&  object) const {
    if(isStaff())
        return(true);

    if(size && object->getSize()) {
        if(size == object->getSize())
            return(true);

        // rings must be exact size
        if(object->getWearflag() == FINGER)
            return(false);

        // weapons and shields can be +- 1 category
        if( (   object->getType() == ObjectType::WEAPON ||
                (object->getType() == ObjectType::ARMOR && object->getWearflag() == SHIELD) ) &&
            abs(size - object->getSize()) == 1 )
            return(true);

        // other armor can only be larger
        if(object->getType() == ObjectType::ARMOR && size - object->getSize() == -1)
            return(true);

        // if its wrong size, it can't be used
        return(false);
    }

    return(true);
}

//********************************************************************
//                      canWear
//********************************************************************

bool Player::canWear(const std::shared_ptr<Object>&  object, bool all) const {
    if(!object->getWearflag() || object->getWearflag() == WIELD || object->getWearflag() == HELD) {
        if(!all)
            print("You can't wear that.\n");
        return(false);
    }

    std::string armorType = object->getArmorType();
    if( (   object->getType() == ObjectType::ARMOR &&
            !object->isLightArmor() &&
            (   object->getWearflag() == BODY ||
                object->getWearflag() == ARMS ||
                object->getWearflag() == HANDS
            )
        ) &&
        (   ready[HELD-1] &&
            ready[HELD-1]->getType() == ObjectType ::WEAPON
        )
    ) {
        printColor("You can't wear %P and also use a second weapon.\n", object.get());
        printColor("%O would hinder your movement too much.\n", object.get());
        return(false);
    }

    if( object->getWearflag() != FINGER &&
        object->getWearflag() != SHIELD &&
        object->getType() == ObjectType::ARMOR &&
        !isStaff() &&
        !knowsSkill(armorType)
    ) {
        if(!all)
            printColor("You can't use %P; you lack the ability to wear %s armor.\n", object.get(), armorType.c_str());
        return(false);
    }

    if( object->getWearflag() == FINGER &&
        ready[FINGER1-1] &&
        ready[FINGER2-1] &&
        ready[FINGER3-1] &&
        ready[FINGER4-1] &&
        ready[FINGER5-1] &&
        ready[FINGER6-1] &&
        ready[FINGER7-1] &&
        ready[FINGER8-1]
    ) {
        if(!all)
            print("You don't have any more fingers left.\n");
        return(false);
    }

    if(object->getWearflag() != FINGER && ready[object->getWearflag()-1]) {
        if(!all) {
            printColor("You cannot wear %1P.\nYou're already wearing %1P.\n",
                object.get(), ready[object->getWearflag()-1].get());
        }
        return(false);
    }

    if(object->isHeavyArmor() && inCombat() && checkHeavyRestrict("")) {
        if(!all) {
            printColor("^RYou're too busy fighting to fumble around with heavy armor right now!^x\n");
        }
        return(false);
    }
    return(true);
}

//********************************************************************
//                      canUse
//********************************************************************

bool Player::canUse(const std::shared_ptr<Object>&  object, bool all) {
    if(object->getType() == ObjectType::WEAPON) {
        if(object->getWeaponType() == "none") {
            if(!all)
                printColor("You lack the skills to wield %P.\n", object.get());
            return(false);
        }
        if(isStaff())
            return(true);
//      if(!knowsSkill(object->getWeaponType())) {
        if(!knowsSkill(object->getWeaponCategory())) {
            if(!all)
                print("You are not skilled with %s.\n", gConfig->getSkillDisplayName(object->getWeaponType()).c_str());
            return(false);
        }
    }

    if(!willFit(object)) {
        if(!all)
            printColor("%O isn't the right size for you.\n", object.get());
        return(false);
    }

    if( size &&
        object->getSize() &&
        size != object->getSize() &&
        object->getWearflag() != HELD-1
    ) {
        if(!all)
            printColor("Using %P is awkward due to its size.\n", object.get());
    }

    if(object->getShotsCur() < 1 && object->getType() != ObjectType::WAND) {
        if(!all)
            print("You can't. It's broken.\n");
        return(false);
    }

    if(object->doRestrict(Containable::downcasted_shared_from_this<Creature>(), !all))
        return(false);

    return(true);
}

//********************************************************************
//                      canWield
//********************************************************************

bool Creature::canWield(const std::shared_ptr<Object>&  object, int n) const {
    int     wielding=0, holding=0, shield=0, second=0;

    if(ready[WIELD-1])
        wielding = 1;
    if(ready[HELD-1])
        holding = 1;
    if(ready[HELD-1] && ready[HELD-1]->getType() < ObjectType::ARMOR)
        second = 1;
    if(ready[SHIELD-1])
        shield = 1;

    switch (n) {
    case HOLDOBJ:
        if(second) {
            print("You're wielding a weapon in your off hand. You can't hold that.\n");
            return(false);
        }
        if(holding) {
            print("You're already holding something.\n");
            return(false);
        }
        if(shield && !ready[SHIELD-1]->flagIsSet(O_SMALL_SHIELD)) {
            print("Your shield is being held by your off hand. You can't hold that.\n");
            return(false);
        }
        if(wielding && ready[WIELD-1]->needsTwoHands()) {
            printColor("You're using both hands to wield %P!\n", ready[WIELD-1].get());
            return(false);

        }
        break;
    case WIELDOBJ:
        if(holding && object->needsTwoHands()) {
            printColor("You need both hands to wield %P.\nYou have %P in your off hand.\n", object.get(), ready[HELD-1].get());
            return(false);
        }
        if(shield && !ready[SHIELD-1]->flagIsSet(O_SMALL_SHIELD) && object->needsTwoHands()) {
            printColor("You need both hands to wield %P.\n%O uses your off hand.\n", object.get(), ready[SHIELD-1].get());
            return(false);
        }

        if(wielding) {
            printColor("You're already wielding %P.\n", ready[WIELD-1].get());
            return(false);
        }

        if(cClass == CreatureClass::CLERIC) {
            std::string objCategory = object->getWeaponCategory();
            std::string objType = object->getWeaponType();
            switch(deity) {
            case CERIS:
                if(objCategory != "crushing" && objCategory != "ranged" && objType != "polearm") {
                    print("%s clerics may only use crushing, pole, and ranged weapons.\n", gConfig->getDeity(deity)->getName().c_str());
                    return(false);
                }
                break;
            case KAMIRA:
                if(objCategory == "ranged") {
                    print("%s clerics may not use ranged weapons.\n", gConfig->getDeity(deity)->getName().c_str());
                    return(false);
                }
                break;
            }

        }

        break;
    case SECONDOBJ:
        if(second) {
            printColor("You're already wielding %P in your off hand.\n", ready[HELD-1].get());
            return(false);
        }
        if(holding) {
            printColor("You're already holding %P in your off hand.\n", ready[HELD-1].get());
            return(false);
        }
        if(shield && !ready[SHIELD-1]->flagIsSet(O_SMALL_SHIELD)) {
            printColor("Your shield is being held by your off hand. You can't second %P.\n", object.get());
            return(false);
        }
        if(object->needsTwoHands()) {
            printColor("%O requires two hands and cannot be used as a second weapon.\n", object.get());
            return(false);
        }
        if(wielding && ready[WIELD-1]->needsTwoHands()) {
            printColor("You're using both hands to wield %P!\n", ready[WIELD-1].get());
            return(false);
        }
        if(cClass == CreatureClass::RANGER) {
            if(ready[BODY-1] && !ready[BODY-1]->isLightArmor()) {
                printColor("You must remove %P to wield a second weapon.\nIt is too heavy.\n", ready[BODY-1].get());
                return(false);
            }
            if(ready[ARMS-1] && !ready[ARMS-1]->isLightArmor()) {
                printColor("You must remove %P to wield a second weapon.\nIt is too heavy.\n", ready[ARMS-1].get());
                return(false);
            }
            if(ready[HANDS-1] && !ready[HANDS-1]->isLightArmor()) {
                printColor("You must remove %P to wield a second weapon.\nIt is too heavy.\n", ready[HANDS-1].get());
                return(false);
            }
        }
        break;
    case SHIELDOBJ:
        if(shield) {
            print("You're already wearing a shield.\n");
            return(false);
        }
        if(holding && !object->flagIsSet(O_SMALL_SHIELD)) {
            printColor("You are using your shield arm to hold %P.\n", ready[HELD-1].get());
            return(false);
        }
        if(wielding && !object->flagIsSet(O_SMALL_SHIELD) && ready[WIELD-1]->needsTwoHands()) {
            printColor("You're using both arms to wield %P!\n", ready[WIELD-1].get());
            return(false);
        }
        break;
    }// end switch

    return(true);
}

//********************************************************************
//                      getInventoryValue
//********************************************************************

unsigned long Creature::getInventoryValue() const {
    int     a=0;
    long    total=0;
    std::shared_ptr<Object> object3=nullptr;

    if(isMonster())
        return(0);

    for(const auto& object : objects) {

        if(object->getType() == ObjectType::CONTAINER) {
            for(const auto& insideObject : object->objects) {
                if(insideObject->getType() == ObjectType::SCROLL || insideObject->getType() == ObjectType::POTION || insideObject->getType() == ObjectType::SONGSCROLL) {
                    continue;
                }

                if(insideObject->getShotsCur() < 1 || insideObject->flagIsSet(O_NO_PAWN)) {
                    continue;
                }

                if(insideObject->value[GOLD] < 20) {
                    continue;
                }

                total += std::min<unsigned long>(MAXPAWN,insideObject->value[GOLD]/2);
                total = std::max<long>(0,std::min<long>(2000000000,total));
            }
        }

        if(object->getType() == ObjectType::SCROLL || object->getType() == ObjectType::POTION || object->getType() == ObjectType::SONGSCROLL) {
            continue;
        }

        if( (object->getShotsCur() < 1 && object->getType() != ObjectType::CONTAINER) ||
            object->flagIsSet(O_NO_PAWN) )
        {
            continue;
        }

        if(object->value[GOLD] < 20) {
            continue;
        }

        total += std::min<unsigned long>(MAXPAWN,object->value[GOLD]/2);
        total = std::max<long>(0,std::min<long>(2000000000,total));

    }

    for(a=0;a<MAXWEAR;a++) {
        if(!ready[a])
            continue;
        object3 = ready[a];


        if(object3->getType() == ObjectType::CONTAINER) {
            for(const auto& insideObject : object3->objects) {
                if(insideObject->getType() == ObjectType::SCROLL || insideObject->getType() == ObjectType::POTION || insideObject->getType() == ObjectType::SONGSCROLL) {
                    continue;
                }
                if(insideObject->getShotsCur() < 1 || insideObject->flagIsSet(O_NO_PAWN)) {
                    continue;
                }

                if(insideObject->value[GOLD] < 20) {
                    continue;
                }

                total += std::min<unsigned long>(MAXPAWN,insideObject->value[GOLD]);
                total = std::max<long>(0,std::min<long>(2000000000,total));
            }
        }

        if(object3->getType() == ObjectType::SCROLL || object3->getType()== ObjectType::POTION || object3->getType() == ObjectType::SONGSCROLL)
            continue;
        if( (object3->getShotsCur() < 1 && object3->getType() != ObjectType::CONTAINER) ||
            object3->flagIsSet(O_NO_DROP) ||
            object3->flagIsSet(O_NO_PAWN)
        )
            continue;
        if(object3->value[GOLD] < 20)
            continue;

        total+=std::min<unsigned long>(MAXPAWN,object3->value[GOLD]/2);
        total = std::max<long>(0,std::min<long>(2000000000,total));
    }

    return(total);
}

//********************************************************************
//                      save
//********************************************************************
// This function saves a player when the game decides to. It does
// not issue a message that the player was saved. This should be called
// instead of write_ply becuase it handles saving worn items properly
//
// This function saves a player's char. Since items need to be un-readied
// before a player can be saved to a file, this function makes a duplicate
// of the player, unreadies everything on the duplicate, and then saves
// the duplicate to the file. Afterwards, the duplicate is freed from
// memory.

int Player::save(bool updateTime, LoadType saveType) {
    // having an update time option, which should be false for offline
    // operations, prevents aging of chars and keeps last login accurate
    if(updateTime) {
        lastLogin = time(nullptr);
        lasttime[LT_AGE].interval += (lastLogin - lasttime[LT_AGE].ltime);
        lasttime[LT_AGE].ltime = lastLogin;
    }
    // TODO: Save worn equipment
//    for(i=0; i<MAXWEAR; i++) {
//        if(copy->ready[i]) {
//            obj[n] = copy->unequip(i+1, UNEQUIP_ADD_TO_INVENTORY, false, false);
//            obj[n]->setFlag(O_WORN);
//            n++;
//            copy->ready[i] = nullptr;
//        }
//    }

    if(getName().empty())
        return(1);
    if(saveToFile(saveType) < 0)
        std::clog << "*** ERROR: saveXml!\n";

    return(0);
}


//*********************************************************************
//                      ableToDoCommand
//*********************************************************************

bool Creature::ableToDoCommand( cmd* cmnd) const {

    if(isMonster())
        return(true);

    if(flagIsSet(P_BRAINDEAD)) {
        print("You are brain-dead. You can't do that.\n");
        return(false);
    }

    // unconscious has some special rules
    if(flagIsSet(P_UNCONSCIOUS)) {
        // if they're sleeping, let them do only a few things
        if(cmnd && flagIsSet(P_SLEEPING) && (
            cmnd->myCommand->getName() == "wake" ||
            cmnd->myCommand->getName() == "snore" ||
            cmnd->myCommand->getName() == "murmur" ||
            cmnd->myCommand->getName() == "dream" ||
            cmnd->myCommand->getName() == "rollover"
        ) )
            return(true);

        if(flagIsSet(P_SLEEPING))
            print("You can't do that while sleeping.\n");
        else
            print("How can you do that while unconscious?\n");
        return(false);
    }

    return(true);
}

//*********************************************************************
//                      inCombat
//*********************************************************************
// we don't care if they are fighting people in other rooms -
// we only care if they're fighting someone in THIS room

bool Creature::inCombat(bool countPets) const {
    return(inCombat(nullptr, countPets));
}

// target is used when you want to check for a player being in combat
// with a mob BESIDES the one pointed to by creature.
bool Creature::inCombat(const std::shared_ptr<Creature> & target, bool countPets) const {
    // people just logging in
    if(!getParent())
        return(false);
    const std::shared_ptr<const Monster>  mThis = getAsConstMonster();

    if(mThis) {
        for(const auto& ply: getParent()->players) {
            if(mThis->isEnemy(ply.lock()))
                return(true);
        }
        for(const auto& mons : getParent()->monsters) {
            if(mThis->isEnemy(mons))
                return(true);
        }
    } else {
        for(const auto& mons : getParent()->monsters) {
            if(mons->isEnemy(Containable::downcasted_shared_from_this<Creature>()) && (!target || mons != target) && (countPets || !mons->isPet()))
                return(true);
        }
    }

    return(false);
}

bool Creature::convertFlag(int flag) {
    if(flagIsSet(flag)) {
        clearFlag(flag);
        return(true);
    }
    return(false);
}


//*********************************************************************
//                      getCrtStr
//*********************************************************************

std::string Creature::getCrtStr(const std::shared_ptr<const Creature> & viewer, int ioFlags, int num) const {
    std::ostringstream crtStr;
    std::string toReturn = "";
    char ch;
    int mobNum=0;
    //  char    *str;

    if(viewer)
        ioFlags |= viewer->displayFlags();

    const std::shared_ptr<const Player> pThis = getAsConstPlayer();
    // Player
    if(isPlayer()) {
        // Target is possessing a monster -- Show the monsters name if invis
        if(flagIsSet(P_ALIASING) && flagIsSet(P_DM_INVIS)) {
            if(!pThis->getAlias()->flagIsSet(M_NO_PREFIX)) {
                crtStr << "A " << pThis->getAlias()->getName();
            } else
                crtStr << pThis->getAlias()->getName();
        }
        // Target is a dm, is dm invis, and viewer is not a dm       OR
        // Target is a ct, is dm invis, and viewer is not a dm or ct OR
        // Target is staff less than a ct and is dm invis, viewier is less than a builder
        else if((cClass == CreatureClass::DUNGEONMASTER && flagIsSet(P_DM_INVIS) && !(ioFlags & ISDM) ) ||
                (cClass == CreatureClass::CARETAKER && (flagIsSet(P_DM_INVIS) && !(ioFlags & ISDM) && !(ioFlags & ISCT))) ||
                (flagIsSet(P_DM_INVIS) && !(ioFlags & ISDM) && !(ioFlags & ISCT) && !(ioFlags & ISBD))  )
        {
            crtStr << "Someone";
        }
        // Target is misted, viewer can't detect mist, or isn't staff
        else if(isEffected("mist") && !(ioFlags & MIST) && !(ioFlags & ISDM) && !(ioFlags & ISCT) && !(ioFlags & ISBD)) {
            crtStr << "A light mist";
        }
        // Target is invisible and viewer doesn't have detect-invis or isn't staff
        else if(isInvisible() && !(ioFlags & INV) && !(ioFlags & ISDM) && !(ioFlags & ISCT) && !(ioFlags & ISBD)) {
            crtStr << "Someone";
        }
        // Can be seen
        else {
            crtStr << getName();
            if( flagIsSet(P_DM_INVIS ) )
                crtStr << " (+)";
            // Invis
            else if(isInvisible())
                crtStr << " (*)";
            // Misted
            else if(isEffected("mist"))
                crtStr << " (m)";
        }
        toReturn = crtStr.str();
        return(toReturn);
    }

    // Monster
    // Target is a monster, is invisible, and viewer doesn't have detect-invis or is not staff
    if(isMonster() && isInvisible() && !(ioFlags & INV) && !(ioFlags & ISDM) && !(ioFlags & ISCT) && !(ioFlags & ISBD)) {
        crtStr << "Something";
    } else {
        const std::shared_ptr<const Monster>  mThis = getAsConstMonster();
        if(num == 0) {
            if(!flagIsSet(M_NO_PREFIX)) {
                crtStr << "the ";
                if(!(ioFlags & NONUM)) {
                    mobNum = this->getAsConstMonster()->getNumMobs();
                    if(mobNum>1) {
                        crtStr << getOrdinal(mobNum).c_str();
                        crtStr << " ";
                    }
                }
                crtStr << getName();
            } else
                crtStr << getName();
        } else if(num == 1) {
            if(flagIsSet(M_NO_PREFIX))
                crtStr << "";
            else {
                ch = low(getName()[0]);
                if(ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u')
                    crtStr << "an ";
                else
                    crtStr << "a ";
            }
            crtStr << getName();
        } else if(!plural.empty()) {
            // use new plural code - on monster
            crtStr << int_to_text(num);
            crtStr << " " ;
            crtStr << plural;
        } else {
            auto nameStr = int_to_text(num) + " " + getName();
            auto last = nameStr.at(nameStr.length() - 1);
            if(last == 's' || last == 'x') {
                nameStr += "es";
            } else {
                nameStr += 's';
            }
            crtStr << nameStr;
        }

        if((ioFlags & QUEST)) {
            if(mThis->hasQuests()) {
                QuestEligibility questType = mThis->getEligibleQuestDisplay(viewer);
                if (questType == QuestEligibility::ELIGIBLE) {
                    crtStr << " ^Y(!)^m";
                } else if (questType == QuestEligibility::ELIGIBLE_DAILY ||
                           questType == QuestEligibility::ELIGIBLE_WEEKLY) {
                    crtStr << " ^G(!)^m";
                } else if (questType == QuestEligibility::INELIGIBLE_LEVEL ||
                           questType == QuestEligibility::INELIGIBLE_DAILY_NOT_EXPIRED) {
                    crtStr << " ^D(!)^m";
                }
            }
            QuestTurninStatus turninStatus = mThis->checkQuestTurninStatus(viewer);
            if (turninStatus == QuestTurninStatus::UNCOMPLETED_TURNINS) {
                crtStr << " ^D(?)^m";
            } else if (turninStatus == QuestTurninStatus::COMPLETED_TURNINS) {
                crtStr << " ^Y(?)^m";
            } else if (turninStatus == QuestTurninStatus::COMPLETED_DAILY_TURNINS) {
                crtStr << " ^G(?)^m";
            }
        }

        // Target is magic, and viewer has detect magic on
        if((ioFlags & MAG) && mFlagIsSet(M_CAN_CAST))
            crtStr << " (M)";


    }
    toReturn = crtStr.str();

    if(ioFlags & CAP) {
        int pos = 0;
        // don't capitalize colors
        while(toReturn[pos] == '^') pos += 2;
        toReturn[pos] = up(toReturn[pos]);
    }

    return(toReturn);
}



//*********************************************************************
//                      inSameRoom
//*********************************************************************

bool Creature::inSameRoom(const std::shared_ptr<Creature>& target) {
    return(target && target->getRoomParent() == getRoomParent());
}

//*********************************************************************
//                      getLTLeft
//*********************************************************************
// gets the time left on a LT

long Creature::getLTLeft(int myLT, long t) {
    if(t == -1)
        t = time(nullptr);

    long i = lasttime[myLT].ltime + lasttime[myLT].interval;

    if(i > t)
        return(i-t);
    else
        return(0);
}

//*********************************************************************
//                      setLastTime
//*********************************************************************
// Sets a LT

void Creature::setLastTime(int myLT, long t, long interval) {
    lasttime[myLT].interval = std::max(interval, getLTLeft(myLT, t));
    lasttime[myLT].ltime = t;
 }

//*********************************************************************
//                      unApplyTongues
//*********************************************************************

void Creature::unApplyTongues() {
    std::shared_ptr<Player> pTarget = getAsPlayer();
    if(pTarget) {
        if(!pTarget->languageIsKnown(LUNKNOWN + pTarget->current_language)) {
            std::string selfStr;
            int i;
            selfStr.append("You can no longer speak");
            selfStr.append(get_language_adj(pTarget->current_language));
            selfStr.append ("\n");
            if(pTarget->languageIsKnown(LUNKNOWN+LCOMMON)) {
                pTarget->current_language = LCOMMON;
            } else {
                // oh no, they don't know common; time to find a language they do know
                for(i=0; i<LANGUAGE_COUNT; i++) {
                    if(pTarget->languageIsKnown(LUNKNOWN+i)) {
                        pTarget->current_language = i;
                        break;
                    }
                }
            }
        }
    }
}

std::ostream& operator<<(std::ostream& out, const crlasttime& crl) {
    out << crl.cr.str() << "(" << crl.interval << ")";
    return out;
}

//***********************************************************************************
//                                hatesEnemy
//***********************************************************************************
// This function will return whether the calling creature hates a target enemy.
// It can be used for a variety of things like racial combat bonuses to hit/damage, instant
// aggro if wanted, alignment adjustment, or adjustements to faction calculations, and 
// whatever else. - TC

bool Creature::hatesEnemy(std::shared_ptr<Creature> enemy) const {
    std::shared_ptr<Monster> mEnemy=nullptr;
    std::shared_ptr<Player> pEnemy=nullptr;

    mEnemy = enemy->getAsMonster();
    pEnemy = enemy->getAsPlayer();

    int eDeity = enemy->getDeity();
    int eRace = enemy->getRace();
    CreatureClass eClass = enemy->getClass();
    short eAlign = enemy->getAlignment();
    int eMtype = enemy->getType();

    //check hatreds using caller's race first
    switch(getRace()) {
    case DWARF:
    case HILLDWARF:
        if (eRace==ORC || eRace==GOBLIN || eRace==OGRE ||
            eRace==DUERGAR || eRace==TROLL || eRace==KOBOLD)
            return(true);
        if (eMtype == GIANTKIN || eMtype == GOBLINOID)
            return(true);
    case ELF:
    case GREYELF:
    case WILDELF:
        if (eRace==ORC || eRace==GOBLIN || eRace==DARKELF)
            return(true);
        //Elves hate evil fey
        if (eMtype == FAERIE && eAlign < 0)
            return(true);
    case ORC:
        if(eRace==DWARF || eRace==ELF || eRace==BUGBEAR || 
           eRace==HALFELF || eRace==GNOME || eRace==HALFLING)
            return(true);
        //Orcs hate good fey
        if (eMtype == FAERIE && eAlign > 0)
            return(true);
    case GNOME:
        if(eRace==GOBLIN || eRace==ORC || eRace==KOBOLD)
            return(true);
    case TROLL:
        if(eRace==DWARF || eRace==MINOTAUR || eRace==KATARAN)
            return(true);
    case OGRE:
        if(eRace==DWARF || eRace==ELF)
            return(true);
    case DARKELF:
        //Darkelves in general consider themselves superior to all other races, but they truely despise elves
        if(eRace==ELF || eRace==WILDELF || eRace==GREYELF)
            return(true);
        //Darkelves hate good fey
        if (eMtype == FAERIE && eAlign > 0)
            return(true);
    case GOBLIN: 
        //A lot of the evil humanoid races tend to torture goblins and/or enslave them...
        //Plus, they're generally just hateful little bastards
        if(eRace==ELF || eRace==DWARF || eRace==HOBGOBLIN || eRace==BUGBEAR || 
           eRace==OGRE || eRace==GNOME || eRace==TROLL || eRace==GNOLL || eRace == HALFLING)
            return(true);
        //Goblins hate good fey
        if (eMtype == FAERIE && eAlign > 0)
            return(true);
    case HALFLING:
        if (eRace==ORC || eRace==GOBLIN || eRace==KOBOLD)
            return(true);
    case MINOTAUR:
        //Minotaurs hate trolls because they have ancient grudges about prefering similar territory
        if(eRace==TROLL)
            return(true);
    case KOBOLD:
        if(eRace==OGRE || eRace==GNOME || eRace==HALFLING)
            return(true);
    case KATARAN:
        if(eRace==TROLL)
            return(true);
    case CAMBION:
        if(eRace==SERAPH)
            return(true);
    case DUERGAR:
        if(eRace==DWARF)
            return(true);
    //Gnolls and barbarians hate one another because they often violently compete for the same terrtories
    case GNOLL:
        if(eRace==BARBARIAN)
            return(true);
    case BARBARIAN:
        if(eRace==GNOLL)
            return(true);
    case KENKU:
        //Kenku are bird people and hate katarans because they are cat people and often eat them
        if(eRace==KATARAN)
            return(true);
    default:
        break; 
    }//End race check

    // Next we check by deity to handle all cleric and paladin combo hatred shennanigans
    // along with other things various deities hate
    switch(getDeity()) {
    case ARAMON: 
        //Aramon hates all good gods
        if (enemy->getDeityAlignment() > NEUTRAL)
            return(true);
        //Aramon hates devas (angels) and considers seraphs to be unclean
        if (eMtype == DEVA || eRace==SERAPH)
            return(true);
    case CERIS: 
        //Ceris ia a hippie...She doesn't hate on anything - except for undead!
        if(enemy->isUndead())
            return(true);
    case ENOCH: 
        //Enoch hates Aramon and Arachnus, wants to erradicate Ceris, and considers all cambions and undead unclean
        if (eDeity==ARAMON || eDeity==CERIS || eDeity==ARACHNUS || eRace==CAMBION)
            return(true);
        if (eMtype == DEMON || eMtype == DEVIL)
            return(true);
        if(enemy->isUndead())
            return(true);
    case GRADIUS:
        if (eRace==ORC || eRace==GOBLIN || eRace==OGRE || eRace==TROLL || eRace==KOBOLD || eDeity==ARACHNUS || eDeity==ARAMON)
            return(true);
    case ARES: 
        //Ares considers Ceris to be a cowardly pacifist whore...more importantly, she refuses to have sex with him :P
        if (eDeity==CERIS)
            return(true);
    case KAMIRA: 
        //Kamira despises Jakar..because of his greed, and also because he's a stuck up tightwad that hates theiving
        if (eDeity==JAKAR || eDeity==ARAMON || eDeity==ARACHNUS)
            return(true);
    case MARA:
    case LINOTHAN: 
        // Linothan and Mara hate Arachnus and Aramon, and also drow, orcs, goblins
        if (eDeity==ARACHNUS || eDeity==ARAMON || eRace==DARKELF || eRace==ORC || eRace==GOBLIN)
            return(true);
        //Linothan and Mara hate spiders as unclean because of Arachnus
        if(eMtype == ARACHNID)
            return(true);
        //Both despise undead as abominations
        if(enemy->isUndead())
            return(true);
    case ARACHNUS: 
        //Arachnus hates all good-aligned gods
        //And he of course hates all elves
        if (enemy->getDeityAlignment() > NEUTRAL)
            return(true);
        if (eRace==ELF || eRace==WILDELF || eRace==GREYELF)
            return(true);
    case JAKAR: 
        //Jakar has strong hatred for Kamira's thieving chaotic ways and always schemes to kill her or banish her to celestial jail
        //He hates tieflings because they are wired to be chaotic...and of course he also hates thieves
        if (eDeity==KAMIRA || eClass == CreatureClass::THIEF || eRace==TIEFLING) 
            return(true);
        //Jakar hates dragons because they hoard wealth better than he can and he covets their wealth..straight up jealousy
        if (eMtype == DRAGON) 
            return(true);
        //Jakar cannot stand demons because they are 100% the definition of chaos..no god is more OCD than Jakar
        if (eMtype == DEMON) 
            return(true);
    default:
        break;
    }// End Deity check

    // Now we check various class-related hatreds where they exist (cleric/paladin/dknight all handled above through deity)
    switch(getClass()) {
    case CreatureClass::DRUID: 
        //Druids cannot stand evil fey when they are too evil. They disrupt balance and destroy too much. 
        //Druids find extremely good fey annoying, but they do not hate them
        if (eMtype == FAERIE && enemy->getAdjustedAlignment()==BLOODRED)
            return(true);
        //Druids also consider undead an abomination to nature
        if (enemy->isUndead())
            return(true);
    case CreatureClass::RANGER:
        //Rangers hate undead as much as druids do
        if (enemy->isUndead())
            return(true);
        //TODO: If we ever add in ranger hated enemies...put check here

    //TODO: Will remove the below 2 checks when afflictions are done and working. For now, keeping it simple
    case CreatureClass::PUREBLOOD:
        if (eClass == CreatureClass::WEREWOLF)
            return(true);
    case CreatureClass::WEREWOLF:
        if (eClass == CreatureClass::PUREBLOOD)
            return(true);
    default:
        break;
    }//end class checks

    // Finally we if we're a monster, we check our mtype for specific hatreds in case not caught by the above
    // If we're a player, getType() will return PLAYER(0)..if we're an undefined mtype mob, it'll return MONSTER(1)
    switch(getType()) {
    case GIANTKIN:
        // Giants hate dwarves
        if (eRace == DWARF)
            return(true);
    case GOBLINOID:
        // Pretty much all goblinoid races hate dwarves and elves
        if (eRace==DWARF || eRace==ELF)
            return(true);
    case FAERIE:
        // Evil fey hate elves
        if (getAlignment() < 0 && (eRace==ELF || eRace==WILDELF || eRace==GREYELF))
            return(true);
        // Good fey hate drow, orcs, goblins
        if (getAlignment() > 0 && (eRace==DARKELF || eRace==ORC || eRace==GOBLIN))
            return(true);
        // Extremely evil fey hate druids
        if (getAdjustedAlignment() == BLOODRED && eClass == CreatureClass::DRUID)
            return(true);
        // Good and evil fey hate one another
        if (eMtype == FAERIE && ((getAlignment() > 0 && eAlign < 0) || (getAlignment() < 0 && eAlign > 0)))
            return(true);
    case DEMON: // Demons hate devils and anything good aligned, as well as each another
        if (eMtype == DEVIL || eMtype == DEMON)
            return(true);
        if (eAlign > 0)
            return(true);
    case DEVIL: // Devils hate demons and anything good aligned
        if (eMtype == DEMON)
            return(true);
        if (eAlign > 0)
            return(true);
    case DEVA: // Devas (angels) hate devils and demons
        if (eMtype == DEVIL || eMtype == DEMON)
            return(true);
    case UNDEAD: 
        // Evil intelligent undead (only intelligent undead mobs are supposed to be mtype UNDEAD) hate CERIS, LINOTHAN, and ENOCH clerics/paladins
        if (getAlignment() < 0 && (eDeity==CERIS || eDeity==LINOTHAN || eDeity==ENOCH))
            return(true);
    default:
        break;
    } // End check mtype
    

    //Other checks...for effects..etc...
    //vamps vs werewolves and vice versa
    if ((isEffected("vampirism") && enemy->isEffected("lycanthropy")) ||
        (isEffected("lycanthropy") && enemy->isEffected("vampirism")))
        return(true);

    return(false);

}

