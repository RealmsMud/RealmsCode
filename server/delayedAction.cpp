/*
 * delayedAction.cpp
 *   Delayed action queue
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
#include "mud.hpp"
#include "commands.hpp"
#include "creatures.hpp"
#include "server.hpp"


//*********************************************************************
//                      Delayed Action Queue
//*********************************************************************

// TODO: optomize this by making the queue a sorted list based on completion.
// That way you only have to check the front of the list to see if anything needs to be
// done - no more looping!
// std::map<long, action>


//*********************************************************************
//                      removeDelayedActions
//*********************************************************************
// If the creature's delayedActionQueue is not cleared first, it will have invalid pointers.
// If the creature is immediately going out of memory (like in free_crt), that's fine.

bool Server::removeDelayedActions(MudObject* target, bool interruptOnly) {
    std::list<DelayedAction>::iterator it;
    bool found = false;

    for(it = delayedActionQueue.begin(); it != delayedActionQueue.end(); ) {
        if(!(*it).target) {
            // this must be a bug
            found = true;
            it = delayedActionQueue.erase(it);
            broadcast(isCt, "^rA delayed action without a target was discovered and removed from the queue.");
        } else if((*it).target == target) {
            // should we remove this action?
            if(!interruptOnly || (*it).canInterrupt) {
                // if we're just interrupting, we won't remove-all at the end
                if(interruptOnly)
                    target->removeDelayedAction(&(*it));

                found = true;
                it = delayedActionQueue.erase(it);
            } else {
                it++;
            }
        } else {
            it++;
        }
    }

    // if we aren't just interrupting, remove all
    if(!interruptOnly)
        target->clearDelayedActions();
    return(found);
}

//*********************************************************************
//                      parseDelayedActions
//*********************************************************************
// this function is called once a second from Server::updateGame

void Server::parseDelayedActions(long t) {
    std::list<DelayedAction>::iterator it;

    for(it = delayedActionQueue.begin(); it != delayedActionQueue.end(); ) {
        if((*it).whenFinished <= t) {
            if((*it).target) {
                // exectue the callback function
                ((*it).callback) (&(*it));
                (*it).target->removeDelayedAction(&(*it));
            }
            it = delayedActionQueue.erase(it);
        } else
            it++;
    }
}


//*********************************************************************
//                      addDelayedAction
//*********************************************************************
// This is called by anywhere in the game that wants to provide a delayed action to the player.
// If adding the action interrupts what the player is currently doing, that's up to the
// calling code to take care of.

void Server::addDelayedAction(void (*callback)(DelayedActionFn), MudObject* target, cmd* cmnd, DelayedActionType type, long howLong, bool canInterrupt) {
    switch(type) {
        case ActionFish:
        case ActionSearch:
        case ActionTrack:
        case ActionStudy:
            // these actions are player-only
            if(!target->getAsConstPlayer())
                return;
            break;
        default:
            break;
    }

    DelayedAction action = DelayedAction(callback, target, cmnd, type, time(nullptr) + howLong, canInterrupt);

    delayedActionQueue.push_back(action);
    target->addDelayedAction(&action);
}

//*********************************************************************
//                      addDelayedScript
//*********************************************************************

void Server::addDelayedScript(void (*callback)(DelayedActionFn), MudObject* target, const bstring& script, long howLong, bool canInterrupt) {
    DelayedAction action = DelayedAction(callback, target, script, time(nullptr) + howLong, canInterrupt);

    delayedActionQueue.push_back(action);
    target->addDelayedAction(&action);
}


//*********************************************************************
//                      hasAction
//*********************************************************************
// this will inform the calling function that a particular delayed action is in the queue

bool Server::hasAction(const MudObject* target, DelayedActionType type) {
    std::list<DelayedAction>::const_iterator it;

    for(it = delayedActionQueue.begin(); it != delayedActionQueue.end(); it++) {
        if((*it).target == target && (*it).type == type)
            return(true);
    }

    return(false);
}

//*********************************************************************
//                      delayedActionStrings
//*********************************************************************
// called from Player::score

bstring Server::delayedActionStrings(const MudObject* target) {
    std::ostringstream oStr;
    std::list<DelayedAction>::const_iterator it;

    for(it = delayedActionQueue.begin(); it != delayedActionQueue.end(); it++) {
        if((*it).target == target) {
            switch((*it).type) {
                case ActionFish:
                    oStr << " ^C*Fishing*";
                    break;
                case ActionSearch:
                    oStr << " ^C*Searching*";
                    break;
                case ActionTrack:
                    oStr << " ^C*Tracking*";
                    break;
                case ActionStudy:
                    oStr << " ^Y*Studying*";
                    break;
                default:
                    break;
            }
        }
    }

    return(oStr.str());
}


//*********************************************************************
//                      interruptDelayedActions
//*********************************************************************
// Called whenever the player does something to make them cancel their delayed action queue.

void MudObject::interruptDelayedActions() {
    if(!delayedActionQueue.empty()) {
        // true means we are only removing interrupt-able actions
        if(gServer->removeDelayedActions(this, true)) {
            const Creature* creature = getAsConstCreature();
            if(creature)
                creature->print("You stop what you are doing.\n");
        }
    }
}


//*********************************************************************
//                      addDelayedAction
//*********************************************************************
// ONLY to be called from Server::addDelayedAction

void MudObject::addDelayedAction(DelayedAction* action) {
    delayedActionQueue.push_back(action);
}


//*********************************************************************
//                      removeDelayedAction
//*********************************************************************
// ONLY to be called from Server::parseDelayedActions and Server::removeDelayedActions

void MudObject::removeDelayedAction(DelayedAction* action) {
    std::list<DelayedAction*>::iterator it;

    for(it = delayedActionQueue.begin(); it != delayedActionQueue.end(); it++) {
        if((*it) == action) {
            delayedActionQueue.erase(it);
            return;
        }
    }
}


//*********************************************************************
//                      clearDelayedActions
//*********************************************************************
// ONLY to be called from Server::removeDelayedActions

void MudObject::clearDelayedActions() {
    delayedActionQueue.clear();
}


//*********************************************************************
//                      doDelayedAction
//*********************************************************************

void doDelayedAction(const DelayedAction* action) {
    Creature* creature = action->target->getAsCreature();
    if(!creature)
        return;
    // we need a non-const command
    cmd cmnd;
    cmnd = action->cmnd;
    cmdProcess(creature, &cmnd);
}

//*********************************************************************
//                      delayedAction
//*********************************************************************


void Creature::delayedAction(const bstring& action, int delay, MudObject* target) {
    cmd cmnd;

    cmnd.fullstr = action;
    if(target) {
        cmnd.fullstr += bstring(" ") + target->getName();
    }

    stripBadChars(cmnd.fullstr); // removes '.' and '/'
    lowercize(cmnd.fullstr, 0);
    parse(cmnd.fullstr, &cmnd);

    gServer->addDelayedAction(doDelayedAction, this, &cmnd, ActionScript, delay);
}

//*********************************************************************
//                      doDelayedScript
//*********************************************************************

void doDelayedScript(const DelayedAction* action) {
    Creature* creature = action->target->getAsCreature();
    if(!creature)
        return;
    gServer->runPython(action->script, "", creature);
}

//*********************************************************************
//                      delayedScript
//*********************************************************************

void Creature::delayedScript(const bstring& script, int delay) {
    gServer->addDelayedScript(doDelayedScript, this, script, delay);
}
