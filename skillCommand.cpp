/*
 * SkillCommand.cpp
 *	 Skills that can be performed as a command
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
	if(!skill->checkResources(creature)) {
		// TODO: Clarify which resource
		*creature << "You lack the required resources to perform that skill\n";
		return(0);
	}

	return(0);
}




//********************************************************************
//* Check Resources
//********************************************************************
// Returns: True  - Sufficient resources
//          False - Insufficient resources

bool SkillCommand::checkResources(Creature* creature) {
	for(SkillCost& res : resources) {
		if(!creature->checkResource(res.resource, res.cost))
			return(false);
	}
	return(true);
}

int SkillCommand::execute(Creature* player, cmd* cmnd) {
    return((fn)(player, cmnd));
}
