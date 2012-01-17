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
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _DELAYEDACTION_H
#define	_DELAYEDACTION_H


enum DelayedActionType {
	UnknownAction,
	ActionFish,
	ActionSearch,
	ActionTrack,
	ActionStudy,
	ActionScript
};

#define DelayedActionFn	const DelayedAction*
struct DelayedAction {
	// This points to a function that accepts DelayedAction* as a parameter.
	// It will be called when the action is complete.
	void (*callback)(const DelayedAction*);

	MudObject* target;
	DelayedActionType type;
	long whenFinished;
	bool canInterrupt;
	cmd cmnd;
	bstring script;

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

	DelayedAction(void (*callback)(DelayedActionFn), MudObject* target, bstring script, long whenFinished, bool canInterrupt) {
		this->callback = callback;
		this->target = target;
		this->type = ActionScript;
		this->whenFinished = whenFinished;
		this->canInterrupt = canInterrupt;
		this->script = script;
	}
};


#endif	/* _DELAYEDACTION_H */

