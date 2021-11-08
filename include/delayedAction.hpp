/*
 * delayedAction.h
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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _DELAYEDACTION_H
#define _DELAYEDACTION_H

#include "cmd.hpp"

class MudObject;

enum DelayedActionType {
    UnknownAction,
    ActionFish,
    ActionSearch,
    ActionTrack,
    ActionStudy,
    ActionScript
};

#define DelayedActionFn const DelayedAction*
struct DelayedAction {
    // This points to a function that accepts DelayedAction* as a parameter.
    // It will be called when the action is complete.
    void (*callback)(const DelayedAction*);

    MudObject* target;
    DelayedActionType type;
    long whenFinished;
    bool canInterrupt;
    cmd cmnd;
    std::string script;

    DelayedAction(void (*callback)(DelayedActionFn), MudObject* target, cmd* cmnd, DelayedActionType type, long whenFinished, bool canInterrupt) {
        this->callback = callback;
        this->target = target;
        this->type = type;
        this->whenFinished = whenFinished;
        this->canInterrupt = canInterrupt;
        if(cmnd)
            this->cmnd = *(cmnd);
        this->script = "";
    }

    DelayedAction(void (*callback)(DelayedActionFn), MudObject* target, std::string_view script, long whenFinished, bool canInterrupt) {
        this->callback = callback;
        this->target = target;
        this->type = ActionScript;
        this->whenFinished = whenFinished;
        this->canInterrupt = canInterrupt;
        this->script = script;
    }
};


#endif  /* _DELAYEDACTION_H */

