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
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "mud.h"

int getFindWhere(TargetType targetType) {
    switch(targetType) {
        case TARGET_MONSTER:
            return(FIND_MON_ROOM);
        case TARGET_PLAYER:
            return(FIND_PLY_ROOM);
        case TARGET_CREATURE:
            return(FIND_MON_ROOM | FIND_PLY_ROOM);
        case TARGET_OBJECT:
            return(FIND_OBJ_EQUIPMENT | FIND_OBJ_INVENTORY | FIND_OBJ_ROOM);
        case TARGET_OBJECT_CREATURE:
            return(FIND_OBJ_EQUIPMENT | FIND_OBJ_INVENTORY | FIND_OBJ_ROOM | FIND_MON_ROOM | FIND_PLY_ROOM);
        case TARGET_EXIT:
            return(FIND_EXIT);
        case TARGET_MUDOBJECT:
            return(FIND_MON_ROOM | FIND_PLY_ROOM | FIND_OBJ_EQUIPMENT | FIND_OBJ_INVENTORY | FIND_OBJ_ROOM | FIND_EXIT);
        default:
            break;
    }
    return(0);
}
int cmdSkill(Creature* creature, cmd* cmnd) {
    *creature << "Attempting to use the " << cmnd->myCommand->getName() << " skill\n";

    if(!creature->ableToDoCommand(cmnd))
        return(0);

    bstring str = cmnd->myCommand->getName();
    SkillCommand* skillCmd = dynamic_cast<SkillCommand*>(cmnd->myCommand);
    if(!skillCmd) {
        *creature << "Invalid skill!\n";
        return(0);
    }
    Skill* skill = creature->getSkill(cmnd->myCommand->getName());
    if(!skill) {
        *creature << "You don't know how to " << cmnd->myCommand->getName() << "\n";
        return(0);
    }
    if(!skillCmd->checkResources(creature))
        return(0);

    MudObject* target = nullptr;
    if(skillCmd->getTargetType() != TARGET_NONE) {
        bstring toFind = cmnd->str[1];
        int num = cmnd->val[1];
        target = creature->findTarget(getFindWhere(skillCmd->getTargetType()), 0, toFind, num);

        // Assumption: If it's offensive, it's usable on creatures
        if(!target && skillCmd->isOffensive() && creature->hasAttackableTarget())
            target = creature->getTarget();

        if(!target) {
            *creature << "You can't find '" << toFind << "'.\n";
            return(0);
        }
    } else {
        target = creature;
    }

    if(skillCmd->isOffensive()) {
        if(target->isCreature()) {
            if(!creature->canAttack(target->getAsCreature(), false)) {
                return(0);
            }
        }
    }

    if(skillCmd->hasCooldown()) {
        if(!skill->checkTimer(creature, true))
            return(0);
        skill->updateTimer();
    }

    if(skillCmd->getUsesAttackTimer()) {
        if(!creature->checkAttackTimer())
            return(0);

        creature->updateAttackTimer();
    }
    *creature << "You attempt to " << skillCmd->getName() << " " << target << "\n";

    bool result = skillCmd->runScript(creature, target, skill);

    if(skillCmd->hasCooldown()) {
        if(result)
            skill->modifyDelay(skillCmd->getCooldown());
        else
            skill->modifyDelay(skillCmd->getFailCooldown());
    }
    // TODO: Track last skill for future combos

    return(0);
}
//*********************************************************************
//                      checkAttackTimer
//*********************************************************************
// Return: true if the attack timer has expired, false if not

bool Skill::checkTimer(Creature* creature, bool displayFail) {
    long i;
    if(!((i = timer.getTimeLeft()) == 0)) {
        if(displayFail)
            creature->pleaseWait(i/10.0);
//        if(!creature->isDm())
            return(false);
    }
    return(true);
}
//*********************************************************************
//                      UpdateTimer
//*********************************************************************

void Skill::updateTimer(bool setDelay, int delay) {
    if(!setDelay) {
        timer.update();
    } else {
        timer.update(delay);
    }
}

//*********************************************************************
//                      modifyDelay
//*********************************************************************

void Skill::modifyDelay(int amt) {
    timer.modifyDelay(amt);
}

//*********************************************************************
//                      setAttackDelay
//*********************************************************************

void Skill::setDelay(int newDelay) {
    timer.setDelay(newDelay);
}

bool SkillCommand::getUsesAttackTimer() const {
    return(usesAttackTimer);
}
bool SkillCommand::hasCooldown() const {
    return(cooldown != 0);
}
int SkillCommand::getCooldown() const {
    return(cooldown);
}
int SkillCommand::getFailCooldown() const {
    return(failCooldown);
}


bool SkillCommand::runScript(Creature* actor, MudObject* target, Skill* skill) {
    try {
        object localNamespace( (handle<>(PyDict_New())));

        object skillModule( (handle<>(PyImport_ImportModule("skillLib"))) );

        localNamespace["skillLib"] = skillModule;

        localNamespace["skill"] = ptr(skill);
        localNamespace["skillCmd"] = ptr(this);

        // Default retVal is true
        localNamespace["retVal"] = true;
        addMudObjectToDictionary(localNamespace, "actor", actor);
        addMudObjectToDictionary(localNamespace, "target", target);


        gServer->runPython(pyScript, localNamespace);

        bool retVal = extract<bool>(localNamespace["retVal"]);
        //std::cout << "runScript returning: " << retVal << std::endl;
        return(retVal);
    }
    catch( error_already_set) {
        gServer->handlePythonError();
    }
    return(false);
}
TargetType SkillCommand::getTargetType() const {
    return(targetType);
}

bool SkillCommand::isOffensive() const { return(offensive); }

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
void SkillCommand::subResources(Creature* creature) {
    for(SkillCost& res : resources) {
        creature->subResource(res.resource, res.cost);
    }
}

int SkillCommand::execute(Creature* player, cmd* cmnd) {
    return((fn)(player, cmnd));
}
