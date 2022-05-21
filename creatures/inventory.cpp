/*
 * inventory.cpp
 *   Creature Inventory
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

#include "flags.hpp"                        // for P_DM_INVIS, P_CHAOTIC, O_DARKNESS
#include "hooks.hpp"                        // for Hooks
#include "mudObjects/creatures.hpp"         // for Creature, PetList
#include "mudObjects/objects.hpp"           // for Object, ObjectType, ObjectType...
#include "mudObjects/rooms.hpp"             // for BaseRoom, ExitList
#include "proto.hpp"                        // for bonus, broadcast, abortFindRoom
#include "unique.hpp"                       // for remove, deleteOwner
#include "utils.hpp"                        // for MAX, MIN

//*********************************************************************
//                      addObj
//*********************************************************************
// This function adds the object pointer to by the first parameter to
// the inventory of the player pointed to by the second parameter.

void Creature::addObj(const std::shared_ptr<Object>&  object) {
    std::shared_ptr<Player> pPlayer = getAsPlayer();

    object->validateId();
    auto cThis = Containable::downcasted_shared_from_this<Creature>();
    Hooks::run(cThis, "beforeAddObject", object, "beforeAddToCreature");
    object->clearFlag(O_JUST_LOADED);

    // players have big inventories; to keep the mud from searching them when it
    // doesn't need to, record a flag on the player
    if(pPlayer && object->flagIsSet(O_DARKMETAL))
        setFlag(P_DARKMETAL);
    if(object->flagIsSet(O_DARKNESS))
        setFlag(pPlayer ? P_DARKNESS : M_DARKNESS);

    object->addTo(cThis);
    //add(object);

    if(pPlayer)
        pPlayer->updateItems(object);

    Hooks::run(cThis, "afterAddObject", object, "afterAddToCreature");

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

void Creature::finishDelObj(const std::shared_ptr<Object>&  object, bool breakUnique, bool removeUnique, bool darkmetal, bool darkness, bool keep) {
    if(darkmetal)
        killDarkmetal();
    if(breakUnique || removeUnique) {
        std::shared_ptr<Player> player = getPlayerMaster();
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

    Hooks::run(Containable::downcasted_shared_from_this<Creature>(), "afterRemoveObject", object, "afterRemoveFromCreature");
}

//*********************************************************************
//                      delObj
//*********************************************************************

void Creature::delObj(std::shared_ptr<Object>  object, bool breakUnique, bool removeUnique, bool darkmetal, bool darkness, bool keep) {
    Hooks::run(Containable::downcasted_shared_from_this<Creature>(), "beforeRemoveObject", object, "beforeRemoveFromCreature");

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
            for(const auto& obj : objects) {
                if(obj->getType() == ObjectType::CONTAINER) {
                    for(const auto& subObj : obj->objects ) {
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
                    for(const auto& obj : i->objects) {
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
//                      count_inv
//*********************************************************************
// Returns the number of objects a creature has in their inventory.
// If perm_only != false then only those objects which are permanent
// will be counted.

int Creature::countInv(bool permOnly) {
    int total=0;
    for(const auto& obj : objects ) {
        if(!permOnly || (permOnly && (obj->flagIsSet(O_PERM_ITEM))))
            total++;
    }
    return(MIN(100,total));
}


//*********************************************************************
//                      countBagInv
//*********************************************************************
int Creature::countBagInv() {
    int total=0;
    for(const auto& obj : objects ) {
        total++;
        if(obj && obj->getType() == ObjectType::CONTAINER) {
            total += obj->countObj();
        }
    }
    return(total);
}


//*********************************************************************
//                  equip
//*********************************************************************

bool Creature::equip(const std::shared_ptr<Object>&  object, bool showMessage) {
    bool isWeapon = false;

    switch(object->getWearflag()) {
        case BODY:
        case ARMS:
        case LEGS:
        case HANDS:
        case HEAD:
        case FEET:
        case FACE:
        case HELD:
        case SHIELD:
        case NECK:
        case BELT:
            // handle armor
            ready[object->getWearflag()-1] = object;
            break;
        case FINGER:
            // handle rings
            for(int i=FINGER1; i<FINGER8+1; i++) {
                if(!ready[i-1]) {
                    ready[i-1] = object;
                    break;
                }
            }
            break;
        case WIELD:
            // handle weapons
            if(!ready[WIELD-1]) {
                // weapons going in the first hand
                if(showMessage) {
                    printColor("You wield %1P.\n", object.get());
                    broadcast(getSock(), getRoomParent(), "%M wields %1P.", this, object.get());
                }
                ready[WIELD-1] = object;
            } else if(!ready[HELD-1]) {
                // weapons going in the off hand
                if(showMessage) {
                    printColor("You wield %1P in your off hand.\n", object.get());
                    broadcast(getSock(), getRoomParent(), "%M wields %1P in %s off hand.",
                              this, object.get(), hisHer());
                }
                ready[HELD-1] = object;
            } else
                return(false);

            isWeapon = true;
            break;
        default:
            return(false);
    }

    // message for armor/rings
    if(!isWeapon && showMessage) {
        printColor("You wear %1P.\n", object.get());
        broadcast(getSock(), getRoomParent(), "%M wore %1P.", this, object.get());
    }


    if(showMessage && object->getType() != ObjectType::CONTAINER && object->use_output[0])
        printColor("%s\n", object->use_output);

    object->setFlag(O_WORN);

    delObj(object, false, false, true, false, true);

    if(object->flagIsSet(O_EQUIPPING_BESTOWS_EFFECT)) {
        if(Effect::objectCanBestowEffect(object->getEffect())) {
            if(!isEffected(object->getEffect())) {
                // passing keepApplier = true, which means this effect will continue to point to this object
                addEffect(object->getEffect(), object->getEffectDuration(), object->getEffectStrength(), object, true, Containable::downcasted_shared_from_this<Creature>(), true);
            }
        } else {
            object->clearFlag(O_EQUIPPING_BESTOWS_EFFECT);
            if(isStaff())
                printColor("^MClearing flag EquipEffect flag from %s^M.\n", object->getCName());
        }
    }

    return(true);
}

//*********************************************************************
//                      unequip
//*********************************************************************

std::shared_ptr<Object>  Creature::unequip(int wearloc, UnequipAction action, bool darkness, bool showEffect) {
    wearloc--;
    std::shared_ptr<Object> object = ready[wearloc];
    if(object) {
        object->clearFlag(O_WORN);

        if(object->flagIsSet(O_EQUIPPING_BESTOWS_EFFECT))
            removeEffect(object->getEffect(), showEffect, true, object);

        // don't run checkDarkness if this isnt a dark item
        if(!object->flagIsSet(O_DARKNESS))
            darkness = false;
        if(action == UNEQUIP_DELETE) {
            Limited::remove(getAsPlayer(), object);
            darkness = object->flagIsSet(O_DARKNESS);
            object = nullptr;
        } else if(action == UNEQUIP_ADD_TO_INVENTORY) {
            darkness = false;
            addObj(object);
        }
    }
    ready[wearloc] = nullptr;
    if(darkness)
        checkDarkness();
    return(object);
}


//*********************************************************************
//                      printEquipList
//*********************************************************************

void Creature::printEquipList(const std::shared_ptr<Player>& viewer) {
    bool showIndicator = viewer->flagIsSet(P_SHOW_DURABILITY_INDICATOR);

    if(ready[BODY-1]){
        viewer->printColor("On body:   %1P  %s\n", ready[BODY-1].get(), showIndicator ? ready[BODY-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[ARMS-1]){
        viewer->printColor("On arms:   %1P  %s\n", ready[ARMS-1].get(), showIndicator ? ready[ARMS-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[LEGS-1]){
        viewer->printColor("On legs:   %1P  %s\n", ready[LEGS-1].get(), showIndicator ? ready[LEGS-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[NECK-1]){
        viewer->printColor("On neck:   %1P  %s\n", ready[NECK-1].get(), showIndicator ? ready[NECK-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[HANDS-1]){
        viewer->printColor("On hands:  %1P  %s\n", ready[HANDS-1].get(), showIndicator ? ready[HANDS-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[HEAD-1]){
        viewer->printColor("On head:   %1P  %s\n", ready[HEAD-1].get(), showIndicator ? ready[HEAD-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[BELT-1]){
        viewer->printColor("On waist:  %1P  %s\n", ready[BELT -1].get(), showIndicator ? ready[BELT-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[FEET-1]){
        viewer->printColor("On feet:   %1P  %s\n", ready[FEET-1].get(), showIndicator ? ready[FEET-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[FACE-1]){
        viewer->printColor("On face:   %1P  %s\n", ready[FACE-1].get(), showIndicator ? ready[FACE-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[FINGER1-1]){
        viewer->printColor("On finger: %1P  %s\n", ready[FINGER1-1].get(), showIndicator ? ready[FINGER1-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[FINGER2-1]){
        viewer->printColor("On finger: %1P  %s\n", ready[FINGER2-1].get(), showIndicator ? ready[FINGER2-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[FINGER3-1]){
        viewer->printColor("On finger: %1P  %s\n", ready[FINGER3-1].get(), showIndicator ? ready[FINGER3-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[FINGER4-1]){
        viewer->printColor("On finger: %1P  %s\n", ready[FINGER4-1].get(), showIndicator ? ready[FINGER4-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[FINGER5-1]){
        viewer->printColor("On finger: %1P  %s\n", ready[FINGER5-1].get(), showIndicator ? ready[FINGER5-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[FINGER6-1]){
        viewer->printColor("On finger: %1P  %s\n", ready[FINGER6-1].get(), showIndicator ? ready[FINGER6-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[FINGER7-1]){
        viewer->printColor("On finger: %1P  %s\n", ready[FINGER7-1].get(), showIndicator ? ready[FINGER7-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[FINGER8-1]){
        viewer->printColor("On finger: %1P  %s\n", ready[FINGER8-1].get(), showIndicator ? ready[FINGER8-1]->getDurabilityIndicator().c_str() : "");
    }
    // dual wield
    if(ready[HELD-1]) {
        if(ready[HELD-1]->getWearflag() != WIELD)
            viewer->printColor("Holding:   %1P  %s\n", ready[HELD-1].get(), showIndicator ? ready[HELD-1]->getDurabilityIndicator().c_str() : "");
        else if(ready[HELD-1]->getWearflag() == WIELD)
            viewer->printColor("In Hand:   %1P  %s\n", ready[HELD-1].get(), showIndicator ? ready[HELD-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[SHIELD-1]){
        viewer->printColor("Shield:    %1P  %s\n", ready[SHIELD-1].get(), showIndicator ? ready[SHIELD-1]->getDurabilityIndicator().c_str() : "");
    }
    if(ready[WIELD-1]){
        viewer->printColor("Wielded:   %1P  %s\n", ready[WIELD-1].get(), showIndicator ? ready[WIELD-1]->getDurabilityIndicator().c_str() : "");
    }
}


//*********************************************************************
//                      checkDarkness
//*********************************************************************
// checks the creature for a darkness item. if they have one, flag them

void Creature::checkDarkness() {
    clearFlag(isPlayer() ? P_DARKNESS : M_DARKNESS);
    for(auto & i : ready) {
        if(i && i->flagIsSet(O_DARKNESS)) {
            setFlag(isPlayer() ? P_DARKNESS : M_DARKNESS);
            return;
        }
    }
    for(const auto& obj : objects) {
        if(obj->flagIsSet(O_DARKNESS)) {
            setFlag(isPlayer() ? P_DARKNESS : M_DARKNESS);
            return;
        }
    }
}

