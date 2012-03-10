/*
 * SkillCommand.cpp
 *   Skills that can be performed as a command
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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "mud.h"

int cmdSkill(Creature* creature, cmd* cmnd) {
    Player *player = creature->getAsPlayer();

    *creature << "Attempting to use the " << cmnd->myCommand->getName() << " skill\n";

    if(!creature->ableToDoCommand(cmnd))
        return(0);

    bstring str = cmnd->myCommand->getName();
    SkillCommand* skill = dynamic_cast<SkillCommand*>(cmnd->myCommand);
    if(!skill) {
        *creature << "Invalid skill!\n";
        return(0);
    }
    if(!creature->knowsSkill(cmnd->myCommand->getName())) {
        *creature << "You don't know how to " << cmnd->myCommand->getName() << "\n";
        return(0);
    }
    if(!skill->checkResources(creature))
        return(0);

    MudObject* target = NULL;
    Container* parent = creature->getParent();
    if(!skill->getTargetType() != TARGET_NONE) {

    }
    return(0);
}



//********************************************************************************
// CheckResource
//********************************************************************************
// Checks that the given resource type has sufficient resources left

bool Creature::checkResource(ResourceType resType, int resCost) {
    bool retVal = true;
    switch(resType) {
    case RES_NONE:
        retVal = true;
        break;
    case RES_GOLD:
        retVal = coins[GOLD] >= resCost;
        break;
    case RES_MANA:
        retVal = mp.getCur() >= resCost;
        break;
    case RES_HIT_POINTS:
        retVal = hp.getCur() >= resCost;
        break;
    case RES_FOCUS:
        if(!getAsPlayer())
            retVal = false;
        else
            retVal = getAsPlayer()->focus.getCur() >= resCost;
        break;
    case RES_ENERGY:
        // no energy for now
        retVal = false;
        break;
    default:
        // Unknown resource, we don't have it
        retVal = false;
        break;
    }
    return(retVal);
}
//********************************************************************************
// SubResource
//********************************************************************************
// Removes resCost of given resource; does not check for sufficient resources,
// as it assumes that has been checked elsewhere
void Creature::subResource(ResourceType resType, int resCost) {
    switch(resType) {
        case RES_NONE:
            return;
        case RES_GOLD:
            //return(coins[GOLD] >= resCost);
            coins.sub(resCost, GOLD);
            return;
        case RES_MANA:
            mp.decrease(resCost);
            return;
        case RES_HIT_POINTS:
            hp.decrease(resCost);
            return;
        case RES_FOCUS:
            if(!getAsPlayer())
                return;
            getAsPlayer()->focus.decrease(resCost);
            return;
        case RES_ENERGY:
            // no energy for now
            return;
        default:
            // Unknown resource, we don't have it
            return;
        }
}

//********************************************************************************
// GetResourceName
//********************************************************************************
bstring getResourceName(ResourceType resType) {
    switch(resType) {
        case RES_NONE:
        default:
            return("(Unknown)");
        case RES_GOLD:
            return("gold coins");
        case RES_MANA:
            return("magic points");
        case RES_HIT_POINTS:
            return("hit points");
        case RES_FOCUS:
            return("focus");
        case RES_ENERGY:
            return("energy");
        }
}
//********************************************************************
//* Check Resources
//********************************************************************
// Returns: True  - Sufficient resources
//          False - Insufficient resources

bool SkillCommand::checkResources(Creature* creature) {
    for(SkillCost& res : resources) {
        if(!creature->checkResource(res.resource, res.cost)) {
            bstring failMsg = bstring("You need to have at least ") + res.cost + " " + getResourceName(res.resource) + ".\n";
            return(creature->checkStaff(failMsg.c_str()));
        }
    }
    return(true);
}

int SkillCommand::execute(Creature* player, cmd* cmnd) {
    return((fn)(player, cmnd));
}
