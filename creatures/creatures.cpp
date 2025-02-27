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
#include "lasttime.hpp"                // for lasttime, CRLastTime, operator<<
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
#include "random.hpp"                  // For Random
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
            if(cTarget->flagIsSet(P_DM_INVIS) && getClass() < cTarget->getClass()) 
                return(false);
            if(target->isEffected("incognito") && (getClass() < cTarget->getClass()) && getParent() != cTarget->getParent()) 
                return(false);

            if(!skip) {
                if(cTarget->isInvisible() && !isEffected("detect-invisible") && !isStaff()) 
                    return(false);
                if(target->isEffected("mist") && !isEffected("true-sight") && !isStaff()) 
                    return(false);
            }

        } else {

            if(!skip) {
                if(cTarget->isInvisible() && !isEffected("detect-invisible") && !isStaff()) 
                    return(false);
            }

        }

    } // End Creature
    if(target->isExit()) {
        const std::shared_ptr<const Exit> &exit = target->getAsConstExit();
        if(isStaff()) 
            return(true);

        // handle NoSee right away
        if(exit->flagIsSet(X_NO_SEE)) 
            return(false);
        if(exit->isEffected("invisibility") && !isEffected("detect-invisible")) 
            return(false);
    }
    if(target->isObject()) {
        const std::shared_ptr<const Object> &object = target->getAsConstObject();

        if(isStaff()) 
            return(true);
        if(object->isEffected("invisibility") && !isEffected("detect-invisible")) 
            return(false);

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

bool Creature::inPawn() const {
    return(getConstRoomParent()->flagIsSet(R_PAWN_SHOP) || getConstRoomParent()->flagIsSet(R_SHOP));
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
//                      canUseWands
//********************************************************************
bool Player::canUseWands(bool print) {
    bool useable=false;

    if(isStaff())
        useable = true;

    switch (getClass()) {
    case CreatureClass::LICH:
    case CreatureClass::DRUID:
    case CreatureClass::CLERIC:        
    case CreatureClass::MAGE:
    case CreatureClass::BARD:
        useable = true;
        break;
    case CreatureClass::THIEF:
        if(getSecondClass() == CreatureClass::MAGE)
            useable = true;
        else if(getLevel() >= 10)
            useable = true;
        break;
    case CreatureClass::FIGHTER:
        if(getSecondClass() == CreatureClass::THIEF && getLevel()>=10)
            useable = true;
        if(getSecondClass() == CreatureClass::MAGE)
            useable = true;
        break;
    default:
        useable = false;

        break;
    }

    // Temporarily put this here, ignoring above checks, until we eventually use this function, 
    // since everybody can use wands right now.
    useable = true;
    
    if(!useable && print) {
        *this << ColorOn << "^cYou do not comprehend the complex idiosyncracies of wand use.\n" << ColorOff;
    }
    return(useable);

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
        object->getWearflag() != HELD
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

bool Creature::isVampire() {
    return(getClass() == CreatureClass::PUREBLOOD || isEffected("vampirism"));
}

bool Creature::isUndeadImmuneEnchantments() {

    //Vampire class or vampirism during day not immune to enchantments
    if (isDay() && (isVampire()))
        return(false);
    //Lich class set always resists enchantments
    else if (getClass() == CreatureClass::LICH)
        return(true);
    //Vampire class or vampirism at night always resists
    else if (isVampire() && !isDay())
        return(true);
    //If is a monster with undead flag, or monType is UNDEAD, always resist enchantments
    else if (isMonster() && (getAsMonster()->flagIsSet(M_UNDEAD) || getAsMonster()->getType() == UNDEAD))
        return(true);
    else
        return(false);

}

// ********************************************************************************************************
//                                  Monster::getEnchantmentImmunity
// ********************************************************************************************************
// This function checks a monster to determine if the monster is immune or not eligible for the enchantment
// spell sent to it
bool Monster::getEnchantmentImmunity(const std::shared_ptr<Creature>& caster, const std::string spell, bool print) {
    bool wrongMonType=false, monsterImmune=false;

    if (!isResistableEnchantment(spell))
        return(false);


    // ********************************************************************************************************
    if (flagIsSet(M_DM_FOLLOW) || (isPet() && getMaster()->isStaff()) ) {
        if(print) {
            *caster << ColorOn << "Strangely, your spell dissipated.\n" << ColorOff;
            broadcast(caster->getSock(), caster->getParent(), "^yStrangely, %M'spell dissipated.^x", caster.get());
        }
        return(true);
    }
    
    
    if (spell == "hold-person" && !monType::isHumanoidLike(getType()) && getAsCreature()->isUndeadImmuneEnchantments())
        wrongMonType=true;
    else if (spell == "hold-monster" && getAsCreature()->isUndeadImmuneEnchantments()) // Undead are immune to hold-monster
        wrongMonType=true;
    else if (spell == "hold-undead" && !isUndead()) // Only undead are affected by hold-undead, even vampires during daytime
       wrongMonType=true;
    else if (spell == "hold-plant" && !monType::isPlant(getType()))
        wrongMonType=true;
    else if (spell == "hold-animal" && !monType::isAnimal(getType()))
        wrongMonType=true;
    else if (spell == "hold-elemental" && !monType::isElemental(getType()))
        wrongMonType=true;
    else if (spell == "hold-fey" && !monType::isFey(getType()))
        wrongMonType=true;
    
    if(monType::isImmuneEnchantments(getType())) 
        monsterImmune=true;

    if (spell != "hold-undead" && getAsCreature()->isUndeadImmuneEnchantments())
        monsterImmune=true;

    if (wrongMonType || monsterImmune) {
        if(print) {
            if (isUndead())
                *caster << ColorOn << "^y" << ((getAsCreature()->isVampire() && !isDay())?"At night, vampiric creatures":"Undead creatures") << " are immune to the " << spell << " spell. Your spell fizzled out.\n" << ColorOff;
            else
                *caster << ColorOn << "^y" << "Creatures of type^D " << monType::getName(getType()) << "^y " 
                        << (wrongMonType?"are unaffected by the ":"are immune to ") << (wrongMonType?spell:"") 
                                            << (wrongMonType?" spell":" enchantment spells") << ". Your spell fizzled out.\n" << ColorOff; 
            broadcast(caster->getSock(), caster->getParent(), "^yThe spell fizzled out.^x");
        }

        doCheckFailedCastAggro(caster, (Random::get(1,100)<=5?true:false), print);
    }

    return(wrongMonType || monsterImmune);

}

//************************************************************************************************************
//                   Monster::getEnchantmentVsMontype
//************************************************************************************************************
// This checks against a monster's mtype for resistance to whatever eligible enchantment spell is cast on it

bool Monster::getEnchantmentVsMontype(const std::shared_ptr<Creature>& caster, const std::string spell, bool print) {
    bool resist = false;
    bool willAggro = true; // default is mobs will go aggro on resist

    if (!isResistableEnchantment(spell))
        return(false);
    
    switch (getType()) {
        case DEMON:
            if (Random::get(1,100) <= 50) {
                if (print) {
                    *caster << ColorOn << "^y" << setf(CAP) << this << " was unaffected by your clownish " << spell << " spell. " << upHisHer() << " demonic origins shrugged it off.\n" << ColorOff;
                    broadcast(caster->getSock(), caster->getParent(), "^y%M is unaffected by %N's clownish mortal magic.^x", this, caster.get());
                }
                resist=true;
            }
            break;
        case DEVIL:
            if (Random::get(1,100) <= 50) {
                if (print) {
                    *caster << ColorOn << "^y" << setf(CAP) << this << " was unaffected by your weakling mortal " << spell << " spell. " << upHisHer() << " diabolical origins shrugged it off.\n" << ColorOff;
                    broadcast(caster->getSock(), caster->getParent(),"^y%M is unaffected by %N's weakling mortal magic.^x", this, caster.get());
                }
                resist=true;
            }
            break;
        case DAEMON:
            if (Random::get(1,100) <= 50) {
                if (print) {
                    *caster << ColorOn << "^y" << setf(CAP) << this << " was unaffected by your worthless mortal " << spell << " spell. "<< upHisHer() << " fiendish origins shrugged it off.\n" << ColorOff;
                    broadcast(caster->getSock(), caster->getParent(),"^y%M is unaffected by %N's worthless mortal magic.^x", this, caster.get());
                }
                resist=true;
            }
            break;
        case MODRON:
            if (Random::get(1,100) <= 50) {
                if (print) {
                    *caster << ColorOn << "^y" << setf(CAP) << this << " was unaffected by your discordant mortal " << spell << " spell. " << upHisHer() << "  orderly nature shrugged it off.\n" << ColorOff;
                    broadcast(caster->getSock(), caster->getParent(),"^y%M is unaffected by %N's discordant mortal magic.^x", this, caster.get());
                }
                resist=true;
            }
            break;
        case DEVA:
            if (Random::get(1,100) <= 98) {
                if (print) {
                    *caster << ColorOn << "^y" << setf(CAP) << this << " was unaffected by your adorable mortal " << spell << " spell. " << upHisHer() << " celestial origins shrugged it off.\n" << ColorOff;
                    broadcast(caster->getSock(), caster->getParent(),"^y%M is unaffected by %N's adorable mortal magic.^x", this, caster.get());
                }
                resist=true;
                if(caster->getAdjustedAlignment() >= REDDISH)    // Devas will not attack on failed attempts..Unless caster is very evil.
                    willAggro = false;
            }
            break;
        case DRAGON:
            if (print) {
                *caster << ColorOn << "^y" << setf(CAP) << this << " is a dragon! " << upHeShe() << " is unaffected by your puny mortal " << spell << " spell.\n" << ColorOff;
                broadcast(caster->getSock(), caster->getParent(),"^y%M is unaffected by such puny mortal enchantment magic.^x", this);
            }
            resist=true;
            break;
        default:
            resist=false;
            break;
        }

    if (resist)
        doCheckFailedCastAggro(caster, willAggro, print);
    
    return(resist);

}

//************************************************************************************************************
//                   Creature::getClassEnchantmentResist
//************************************************************************************************************
// This checks against a creature's class for resistance to whatever eligible enchantment spell is cast on it
bool Creature::getClassEnchantmentResist(const std::shared_ptr<Creature>& caster, const std::string spell, bool print) {
    bool willAggro=true; // default is mobs will go aggro on resist
    bool resist=false;

    if (!isResistableEnchantment(spell))
        return(false);
    
    // Check of class of player or mob for specific resistances to various spells
    switch (getClass()) {
    case CreatureClass::LICH:
        if (spell != "hold-undead") {
            if (print) {
                *this << ColorOn << "^yYou feel a tingle from " << caster << "'s spell. Liches are immune to such worthless magic.\n" << ColorOff;
                *caster << ColorOn << "^yLiches are immune to " << spell << " spells.\nYour spell dissipated.\n" << ColorOff;
                broadcast(caster->getSock(), getSock(), caster->getParent(), "^y%N is immune to %s spells. %M's spell dissipated.^x", this, spell.c_str(), caster.get());
            }
            resist=true;
        }
        break;
    case CreatureClass::PUREBLOOD:
        if (!isDay() && spell != "hold-undead") {
            if (print) {
                *this << ColorOn << "^y" << setf(CAP) << caster << "'s spell dissipated. You are immune to " << spell << " spells at night.\n" << ColorOff;
                *caster << ColorOn << "^yVampires are immune to " << spell << " spells during the night.\nYour spell dissipated.\n" << ColorOff;
                broadcast(caster->getSock(), caster->getParent(), "^yVampires are immune to %s spells at night. %M's spell dissipated.^x", spell.c_str(), caster.get());
            }
            resist=true;
        }    
        break;
    case CreatureClass::PALADIN:
        if (spell == "fear" || spell == "scare") {
            if (print) {
                *this << ColorOn << "^y" << setf(CAP) << caster << "'s spell dissipated. Paladins are immune to such ridiculous fearmongering enchantments!\n" << ColorOff;
                *caster << ColorOn << "^yPaladins are immune to " << spell << " spells. Your spell dissipated.\n" << ColorOff;
                broadcast(caster->getSock(), caster->getParent(), "^yPaladins are immune to %s spells. %M's spell dissipated.^x", spell.c_str(), caster.get());
            }
            resist=true;
            if (isMonster() && caster->getAdjustedAlignment() >= PINKISH) // Good paladin mobs will only attack failed casters if they are more evil than PINKISH
                willAggro=false;
        }
        break;
    default:
        resist=false;
        break;
    }

    // Check for just vampirism affliction rather than PUREBLOOD class
    if (getClass() != CreatureClass::PUREBLOOD && isEffected("vampirism") && !isDay() && spell != "hold-undead") {
        if (print) {
            *this << ColorOn << "^y" << setf(CAP) << caster << "'s spell dissipated. You are immune to " << spell << " spells at night.\n" << ColorOff;
            *caster << ColorOn << "^yVampires are immune to " << spell << " spells during the night.\nYour spell dissipated.\n" << ColorOff;
            broadcast(caster->getSock(), getSock(), caster->getParent(), "^yVampires are immune to %s spells at night. %M's spell dissipated.^x", spell.c_str(), caster.get());
        }
        resist=true;
    }

    if (isMonster() && resist)
        getAsMonster()->doCheckFailedCastAggro(caster, willAggro, print);

    return(resist);

}

//************************************************************************************************************
//                   Creature::getRaceEnchantmentResist
//************************************************************************************************************
// This checks against a creature's race for resistance to whatever eligible enchantment spell is cast on it
bool Creature::getRaceEnchantmentResist(const std::shared_ptr<Creature>& caster, const std::string spell, bool print) {
    bool willAggro=true; // default is will go aggro on casters for ineligible, immune, or failed enchantment spellcast attempts
    bool resist=false;

    if (!isResistableEnchantment(spell))
        return(false);
    
    // Check natural resistances due to player race, or race set on mob
    switch (getRace()) {
    case ELF:
    case GREYELF:
    case WILDELF:
    case DARKELF:
    case HALFELF:
        if (Random::get(1,100) <= (getRace()==HALFELF?30:90)) {
            if (print) {
                *this << ColorOn << "^yYour " << (getRace()==HALFELF?"Elven half":"Fey ancestry") << " protected you from " << caster << "'s spell.\n" << ColorOff;
                *caster << ColorOn << "^y" << setf(CAP) << this << " resisted your spell due to " << hisHer() << (getRace()==HALFELF?" Elven half":" Fey origins") << ".\n" << ColorOff;
                if(caster->isStaff())
                    *caster << "Race Number = " << getRace() << "\n";
                broadcast(caster->getSock(), getSock(), caster->getParent(), "^y%M resisted %N's spell due to %s %s!^x", 
                                                                        this, caster.get(), hisHer(), (getRace()==HALFELF?"Elven half":"Fey ancestry"));
            }
            resist=true;
        }
        break;
    case TIEFLING:
        if (Random::get(1,100) <= 25) {
            if (print) {
                *this << ColorOn << "^yYour daibolical origins caused you to resist " << caster << "'s spell.\n" << ColorOff;
                *caster << ColorOn << "^y" << this << " resisted the spell due to " << hisHer() << " diabolical ancestry.\n" << ColorOff;
                broadcast(caster->getSock(), getSock(), caster->getParent(), 
                                "^y%M resisted %N's spell due to %s diabolical ancestry!^x", this, caster.get(), hisHer());
            }
            resist=true;
        }
        break;
    case CAMBION:
        if (Random::get(1,100) <= 25) {
            if (print) {
                *this << ColorOn << "^yYour demonic origins shrugged off " << caster << "'s spell.\n" << ColorOff;
                *caster << ColorOn << "^y" << this << " resisted the spell due to " << hisHer() << " demonic ancestry.\n" << ColorOff;
                broadcast(caster->getSock(), getSock(), caster->getParent(), 
                                "^y%M resisted %N's spell due to %s demonic ancestry!^x", this, caster.get(), hisHer());
            }
            resist=true;
        }
        break;
    case SERAPH:
        if (Random::get(1,100) <= 25) {
            if (print) {
                *this << ColorOn << "^yYour angelic ancestry protected you from " << caster << "'s spell.\n" << ColorOff;
                *caster << ColorOn << "^y" << this << " resisted the spell due to " << hisHer() << " angelic ancestry.\n" << ColorOff;
                broadcast(caster->getSock(), getSock(), caster->getParent(), 
                                "^y%M resisted %N's spell due to %s angelic ancestry!^x", this, caster.get(), hisHer());
            }
            resist=true;
            if (isMonster() && getAdjustedAlignment() >= NEUTRAL && caster->getAdjustedAlignment() >= PINKISH) // Non-evil Seraph mobs will only attack failed casters if they are more evil than PINKISH
                willAggro=false;
        }
        break;
    default:
        resist=false;
        break;
    }

    if (isMonster() && resist)
        getAsMonster()->doCheckFailedCastAggro(caster, willAggro, print);

    return(resist);

}

//*******************************************************************************************************************
//                   Monster::doCheckFailedCastAggro
//*******************************************************************************************************************
// This function handles whether a mob decides or not to go aggro on somebody if they fail a spellcast on them
void Monster::doCheckFailedCastAggro(const std::shared_ptr<Creature>& caster, bool willAggro, bool print) {

    // Unless they are a guardian, good-aligned mobs will not be provoked by an ineligible, immune, or failed spellcast attempt
    if (willAggro && !isEnemy(caster) && getAdjustedAlignment() > NEUTRAL && !flagIsSet(M_PASSIVE_EXIT_GUARD)) {
        if (print && monType::isIntelligent(getType())) {
            *caster << ColorOn << setf(CAP) << this << " eyes you suspiciously.\n" << ColorOff;
            broadcast (caster->getSock(), caster->getParent(), "%M eyes %N suspiciously.", this, caster.get());
        }

        //Mob also checks against its wisdom...successful means aggro turns false and mob won't attack the caster
        int checkroll = Random::get(1,30), wisdom = getWisdom()/10;
        if (caster->isCt() || (caster->isPlayer() && caster->flagIsSet(P_PTESTER))) 
            *caster << ColorOn << "^D*PTEST*\n" << setf(CAP) << this << "'s wisdom: " << wisdom << "\n" << setf(CAP) << this << "'s wisdom check roll (d30): " << checkroll << "\n" << ColorOff;
        if (checkroll < wisdom)
            willAggro = false;
    }

    if (willAggro && !isEnemy(caster) && 
                    (monType::isIntelligent(getType()) || 
                     intelligence.getCur() >= 130 ||
                     flagIsSet(M_PASSIVE_EXIT_GUARD)) 
    ) {
        if(print) {
            *caster << ColorOn << "^rYour bumbled casting has greatly upset " << this << "!\n" << ColorOff;
            broadcast(caster->getSock(), caster->getParent(), "^r%M's bumbled casting has greatly upset %N!", caster.get(), this);
        }
        addEnemy(caster, true);
    }

    return;

}

//********************************************************************************************************
//                                       Creature::checkResistEnchantments
//********************************************************************************************************
// This function can be called to check against any racial/class/other resistances to various enchantment
// and hold spells and return true if the creature resists. Send false for output if do not want to print 
// anything and are just checking resistances in any various logic

bool Creature::checkResistEnchantments(const std::shared_ptr<Creature>& caster, const std::string spell, bool print) {
    bool monsterImmune = false, montypeResist = false, classResist = false, raceResist = false, miscResist = false;
    bool willAggro = true;
    bool output = print;

    if (!caster)
        return(false);

    //Staff always resists
    if(isStaff())
        return(true);

    //TODO: After hold spell testing is finished, put a check right here for caster->isStaff() so staff-casted enchantments never are resisted

    if (!isResistableEnchantment(spell))
        return(false);
    
    // Shouldn't happen, but sanity check to prevent weird output
    if (getCName() == caster->getCName())
        output = false;

    if(isPlayer() && (spell == "hold-animal" || spell == "hold-plant" || 
                      spell == "hold-elemental" || spell == "hold-fey") ) { 
        if (output) {
            *this << ColorOn << "^yThe spell didn't do anything.\n" << ColorOff;
            *caster << ColorOn << "^y" << "The " << spell << " spell does not work against players. Your magic dissipated.\n" << ColorOff;
            broadcast(caster->getSock(), getSock(), caster->getParent(), "^y%M's spell has no effect.\n^x", caster.get());
        }
        return(true);
    }

    if (isMonster()) {
        monsterImmune = getAsMonster()->getEnchantmentImmunity(caster, spell, output);
        if (!monsterImmune)
            montypeResist = getAsMonster()->getEnchantmentVsMontype(caster, spell, output);
    }
    if (!monsterImmune && !montypeResist)
        classResist = getClassEnchantmentResist(caster, spell, output);
    if (!monsterImmune && !montypeResist && !classResist && !isUndeadImmuneEnchantments()) // vampires at night, or liches, bypass race check
        raceResist = getRaceEnchantmentResist(caster, spell, output);
        
    //Now we check for various effects or other misc things that may cause resistance
    if (!monsterImmune && !montypeResist && !classResist && !raceResist) {
        // Being berserk blocks pretty much all enchantments
        if (isEffected("berserk")) {
            if (output) {
                *this << ColorOn << "^y" << setf(CAP) << caster << "'s spell had no effect. You brushed it off with your rage.\n" << ColorOff;
                *caster << ColorOn << "^y" << setf(CAP) << this << " is currently berserk! Your spell had no effect.\n" << ColorOff;
                broadcast(caster->getSock(), getSock(), caster->getParent(), "^y%M's spell has no effect. %N's rage makes %s immune.\n^x", caster.get(), this, himHer());
            }
            miscResist=true;
        }

        // Being unconcious or petrified causes enchantments to fail
        if (isPlayer() && (flagIsSet(P_UNCONSCIOUS) || isEffected("petrification"))) {
            if (output) {
                *this << ColorOn << "^y" << setf(CAP) << caster << "'s spell dissipated.\n" << ColorOff;
                *caster << ColorOn << "^yYour spell dissipated.\n" << ColorOff;
                broadcast(caster->getSock(), getSock(), caster->getParent(), "^y%M's spell dissipated.^x", caster.get());
            }
            miscResist=true;
        }

        // Resist-magic effect and level >= than caster always gives target an 85% chance to avoid..Otherwise only 25% chance.
        if (Random::get(1,100) <= ((caster->getLevel() < getLevel())?85:25) && isEffected("resist-magic")) {
            if (output) {
                *this << ColorOn << "^y" << "Your resist-magic dissipated " << caster << "'s spell.\n" << ColorOff;
                *caster << ColorOn << "^yYour spell dissipated due to " << this << "'s resist-magic.\n" << ColorOff;
                broadcast(caster->getSock(), getSock(), caster->getParent(), "^y%M's spell dissipated.^x", caster.get());
            }
            miscResist=true;
        }
    }

    if(isMonster() && miscResist)
        getAsMonster()->doCheckFailedCastAggro(caster, willAggro, output);

    return(monsterImmune || montypeResist || classResist || raceResist || miscResist);
}

//***********************************************************************************
//                             isMagicallyHeld
//***********************************************************************************
// This function will return true if a player is effected by hold magic of any kind,
// preventing their actions. If just need to use it for logic around checking for 
// hold magic, set print to false in order to not have any output
bool Creature::isMagicallyHeld(bool print) const {

    if (isEffected("hold-person") || isEffected("hold-monster") || 
        isEffected("hold-undead") || isEffected("hold-animal") ||
        isEffected("hold-plant") || isEffected("hold-elemental") || isEffected("hold-fey")) {
        if (isPlayer() && print)
            printColor("^yYou can't do that! You're magically held!^x\n");
        return(true);
        }
    return(false);
}

//*********************************************************************
//                      doCheckBreakMagicalHolds
//*********************************************************************
// This is called when a player or mob is attacked to determine whether
// that action is enough to break any magical hold spells they are under

void Creature::doCheckBreakMagicalHolds(std::shared_ptr<Creature>& attacker, int dmg) {
    int dmgToBreak=0;
    EffectInfo* holdEffect = nullptr;

    //Sanity checks
    if (!attacker || dmg <= 0)
        return;

    if (!isMagicallyHeld())
        return;

    //This works because creatures can only be under one type of hold effect at a time
    if (isEffected("hold-person"))
        holdEffect = getEffect("hold-person");
    else if (isEffected("hold-monster"))
        holdEffect = getEffect("hold-monster");
    else if (isEffected("hold-undead"))
        holdEffect = getEffect("hold-undead");
    else if (isEffected("hold-animal"))
        holdEffect = getEffect("hold-animal");
    else if (isEffected("hold-plant"))
        holdEffect = getEffect("hold-plant");
    else if (isEffected("hold-elemental"))
        holdEffect = getEffect("hold-elemental");
    else if (isEffected("hold-fey"))
        holdEffect = getEffect("hold-fey");

    if(!holdEffect)
        return;

    dmgToBreak = holdEffect->getExtra();

    if (hatesEnemy(attacker))
        dmgToBreak-=(dmg*2);
    else
        dmgToBreak-=dmg;

    holdEffect->setExtra(dmgToBreak);
    int roll = Random::get(1,100);
    if (dmgToBreak > 0 && (attacker->isCt() || (attacker->isPlayer() && attacker->flagIsSet(P_PTESTER)))) {
        *attacker << ColorOn << "\n^DChance hold randomly breaks: ^R" << (std::max(1,getWillpower()/60)) << "%^D\n" ;
        *attacker << "Dmg remaining before forced breakout: ^R: " << dmgToBreak << "\n";
        *attacker << "^DRoll % (d100): ^W" << roll << "\n" << ColorOff;
    }

    if (dmgToBreak <= 0 || (roll <= (std::max(1,getWillpower()/80))) ) {
        if (isPlayer()) {
            *this << ColorOn << "^YThe hold magic on you has been broken!\n" << ColorOff;
            *attacker << ColorOn << "^YThe hold magic on " << this << " has been broken!\n" << ColorOff;
            broadcast(getSock(), attacker->getSock(), attacker->getParent(),"^GThe hold magic on %M has been broken!^x",this);
            getAsPlayer()->computeAC();
            getAsPlayer()->computeAttackPower();
    
        }

        if (isMonster()) {
            if (attacker->isPlayer())
                *attacker << ColorOn << "^YThe hold magic on " << this << " has been broken!\n" << ColorOff;
            broadcast(attacker->getSock(), attacker->getParent(), "^YThe hold magic on %N has been broken!^x",this);
        }

        removeEffect(holdEffect->getName());
        lasttime[LT_SPELL].ltime = time(nullptr);
        setAttackDelay(0);
    }

    return;
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

//********************************************************************
//                    getWisdom()
//********************************************************************
//Wisdom combo stat
int Creature::getWisdom() {
    return((intelligence.getCur() + piety.getCur())/2);
}

//********************************************************************
//                    getAgility()
//********************************************************************
//Agility combo stat
int Creature::getAgility() {

    return((constitution.getCur() + dexterity.getCur())/2);
}

//********************************************************************
//                    getElusivness()
//********************************************************************
//Elusiveness combo stat
int Creature::getElusiveness() {
    return((intelligence.getCur() + dexterity.getCur())/2);
}

//********************************************************************
//                    getWillpower()
//********************************************************************
//Willpower stat
int Creature::getWillpower() {

    int willpower=0, race=0;

    //Base Willpower stat is avg of int, str, and pie
    willpower = (intelligence.getCur() + strength.getCur() + piety.getCur()) / 3;
    race = getRace();

    if (isEffected("berserk"))
        willpower += ((willpower*3)/2);

    // Certain races are very natually stubborn!
    if (race == DWARF || race == HILLDWARF || race == DUERGAR || 
            race == BARBARIAN || race == MINOTAUR || race == SERAPH || 
                race == OGRE || race == LIZARDMAN || race == CENTAUR || race == ORC)    
        willpower += (willpower/10);
    // As are Paladins, Deathknights, and Clerics of Ares
    if  (getClass() == CreatureClass::PALADIN || 
             getClass() == CreatureClass::DEATHKNIGHT ||
                (getClass() == CreatureClass::CLERIC && getDeity() == ARES)) 
        if (!isPlayer() || (isPlayer() && getAsPlayer()->alignInOrder()))
            willpower += (willpower/10);

    if (isMonster()) {

        if (isPet()) 
            willpower += (willpower/5); 

        switch(getType()) {
        case DRAGON:
        case DEMON:
        case DEVIL:
        case FAERIE:
            willpower += ((willpower*3)/2);
            break;
        default:
            break;
        }
    }

    if (isEffected("courage"))
        willpower += (willpower/5);

    if (isEffected("fear"))
        willpower -= (willpower/5);

    return(willpower);
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

std::ostream& operator<<(std::ostream& out, const CRLastTime& crl) {
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
        break;
    case ELF:
    case GREYELF:
    case WILDELF:
        if (eRace==ORC || eRace==GOBLIN || eRace==DARKELF)
            return(true);
        //Elves hate evil fey
        if (eMtype == FAERIE && eAlign < 0)
            return(true);
        break;
    case ORC:
        if(eRace==DWARF || eRace==ELF || eRace==BUGBEAR || 
           eRace==HALFELF || eRace==GNOME || eRace==HALFLING)
            return(true);
        //Orcs hate good fey
        if (eMtype == FAERIE && eAlign > 0)
            return(true);
        break;
    case GNOME:
        if(eRace==GOBLIN || eRace==ORC || eRace==KOBOLD)
            return(true);
    case TROLL:
        if(eRace==DWARF || eRace==MINOTAUR || eRace==KATARAN)
            return(true);
        break;
    case OGRE:
        if(eRace==DWARF || eRace==ELF)
            return(true);
        break;
    case DARKELF:
        //Darkelves in general consider themselves superior to all other races, but they truely despise elves
        if(eRace==ELF || eRace==WILDELF || eRace==GREYELF)
            return(true);
        //Darkelves hate good fey
        if (eMtype == FAERIE && eAlign > 0)
            return(true);
        break;
    case GOBLIN: 
        //A lot of the evil humanoid races tend to torture goblins and/or enslave them...
        //Plus, they're generally just hateful little bastards
        if(eRace==ELF || eRace==DWARF || eRace==HOBGOBLIN || eRace==BUGBEAR || 
           eRace==OGRE || eRace==GNOME || eRace==TROLL || eRace==GNOLL || eRace == HALFLING)
            return(true);
        //Goblins hate good fey
        if (eMtype == FAERIE && eAlign > 0)
            return(true);
        break;
    case HALFLING:
        if (eRace==ORC || eRace==GOBLIN || eRace==KOBOLD)
            return(true);
        break;
    case MINOTAUR:
        //Minotaurs hate trolls because they have ancient grudges about prefering similar territory
        if(eRace==TROLL)
            return(true);
        break;
    case KOBOLD:
        if(eRace==OGRE || eRace==GNOME || eRace==HALFLING)
            return(true);
        break;
    case KATARAN:
        if(eRace==TROLL)
            return(true);
        break;
    case CAMBION:
        if(eRace==SERAPH)
            return(true);
        break;
    case DUERGAR:
        if(eRace==DWARF)
            return(true);
        break;
    //Gnolls and barbarians hate one another because they often violently compete for the same terrtories
    case GNOLL:
        if(eRace==BARBARIAN)
            return(true);
        break;
    case BARBARIAN:
        if(eRace==GNOLL)
            return(true);
        break;
    case KENKU:
        //Kenku are bird people and hate katarans because they are cat people and often eat them
        if(eRace==KATARAN)
            return(true);
        break;
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
        break;
    case CERIS: 
        //Ceris ia a hippie...She doesn't hate on anything - except for undead!
        if(enemy->isUndead())
            return(true);
        break;
    case ENOCH: 
        //Enoch hates Aramon and Arachnus, wants to erradicate Ceris, and considers all cambions and undead unclean
        if (eDeity==ARAMON || eDeity==CERIS || eDeity==ARACHNUS || eRace==CAMBION)
            return(true);
        if (eMtype == DEMON || eMtype == DEVIL)
            return(true);
        if(enemy->isUndead())
            return(true);
        break;
    case GRADIUS:
        if (eRace==ORC || eRace==GOBLIN || eRace==OGRE || eRace==TROLL || eRace==KOBOLD || eDeity==ARACHNUS || eDeity==ARAMON)
            return(true);
        break;
    case ARES: 
        //Ares considers Ceris to be a cowardly pacifist whore...more importantly, she refuses to have sex with him :P
        if (eDeity==CERIS)
            return(true);
        break;
    case KAMIRA: 
        //Kamira despises Jakar..because of his greed, and also because he's a stuck up tightwad that hates theiving
        if (eDeity==JAKAR || eDeity==ARAMON || eDeity==ARACHNUS)
            return(true);
        break;
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
        break;
    case ARACHNUS: 
        //Arachnus hates all good-aligned gods
        //And he of course hates all elves
        if (enemy->getDeityAlignment() > NEUTRAL)
            return(true);
        if (eRace==ELF || eRace==WILDELF || eRace==GREYELF)
            return(true);
        break;
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
        break;
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
        break;
    case CreatureClass::RANGER:
        //Rangers hate undead as much as druids do, but they have a little moral wiggle room for recently turned undead
        if (enemy->isUndead() && enemy->getAdjustedAlignment()<BLUISH)
            return(true);
        //TODO: If we ever add in ranger hated enemies...put check here
        break;
    //TODO: Will remove the below 2 checks when afflictions are done and working. For now, keeping it simple
    case CreatureClass::PUREBLOOD:
        if (eClass == CreatureClass::WEREWOLF)
            return(true);
        break;
    case CreatureClass::WEREWOLF:
        if (eClass == CreatureClass::PUREBLOOD)
            return(true);
        break;
    default:
        break;
    }//end class checks

    // Finally we if we're a monster, we check our mtype for specific hatreds in case not caught by the above
    if (isMonster()) {
        switch(getType()) {
        case GIANTKIN:
            // Giants hate dwarves
            if (eRace == DWARF)
                return(true);
            break;
        case GOBLINOID:
            // Pretty much all goblinoid races hate dwarves and elves
            if (eRace==DWARF || eRace==ELF)
                return(true);
            break;
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
            break;
        case DEMON: // Demons hate devils and anything good aligned, as well as each another
            if (eMtype == DEVIL || eMtype == DEMON)
                return(true);
            if (eAlign > 0)
                return(true);
            break;
        case DEVIL: // Devils hate demons and anything good aligned
            if (eMtype == DEMON)
                return(true);
            if (eAlign > 0)
                return(true);
            break;
        case DEVA: // Devas (angels) hate devils and demons
            if (eMtype == DEVIL || eMtype == DEMON)
                return(true);
            break;
        case UNDEAD: 
            // Evil intelligent undead (only intelligent undead mobs are supposed to be mtype UNDEAD) hate CERIS, LINOTHAN, and ENOCH clerics/paladins
            if (getAlignment() < 0 && (eDeity==CERIS || eDeity==LINOTHAN || eDeity==ENOCH))
                return(true);
            break;
        default:
            break;
        } // End check mtype

    }
    

    //Other checks...for effects..etc...
    //vamps vs werewolves and vice versa
    if ((isEffected("vampirism") && enemy->isEffected("lycanthropy")) ||
        (isEffected("lycanthropy") && enemy->isEffected("vampirism")))
        return(true);

    return(false);

}

std::string Creature::getHpDurabilityStr() {
    std::ostringstream dStr;
    float percent = 1.0;
    int currentHp = hp.getCur(), maximumHp = hp.getMax();

    if (maximumHp <= 0)
        percent = 1.0;
    else if (currentHp <= 0)
        percent = 0.01;
    else
        percent = getPercentRemaining(currentHp, maximumHp);

    if(percent >= 1.0){
        dStr << "^W" << upHeShe() << " is in excellent condition.";
    } else if(percent >= 0.90) {
        dStr << "^W" << upHeShe() << " has a few scratches.";
    } else if(percent >= 0.75) {
        dStr << "^W" << upHeShe() << " has some small wounds.";
    } else if(percent >= 0.60) {
        dStr << "^Y" << upHeShe() << " is in obvious pain.";
    } else if(percent >= 0.50) {
        dStr << "^Y" << upHeShe() << " is moving very tentatively.";
    } else if(percent >= 0.35) {
        dStr << "^Y" << upHeShe() << " looks beaten up pretty badly.";
    } else if(percent >= 0.20) {
        dStr << "^R" << upHeShe() << " has many big nasty wounds and scratches.";
    } else if(percent >= 0.10) {
        dStr << "^R" << upHeShe() << " is in pain and moving erratically.";
    } else if(percent >= 0.05) {
        dStr << "^r" << upHeShe() << " is barely clinging to life.";
    } else if(currentHp) {
        dStr << "^r" << upHeShe() << " is nearly dead.";
    } 
    else
        dStr << "^D" << upHeShe() << " is dead.^x"; // should never get here
        

    return (dStr.str());
}
std::string Creature::getHpProgressBar() {
    return (progressBar(50, getPercentRemaining(hp.getCur(), hp.getMax())));
}

bool Creature::isIndoors() {

    std::shared_ptr<BaseRoom> room = this->getRoomParent();

    if(!room)
        return(false);

    return (room->flagIsSet(R_INDOORS));
}


