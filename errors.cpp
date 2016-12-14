/*
 * error.cpp
 *   Code to handle errors.
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

//*********************************************************************
// _assertlog()
// called when an ASSERTLOG fails and NDEBUG is not defined
// prints a message to the log file and shutsdown the game
//
// JPF March 98
//*********************************************************************

void _assertlog(const char *strExp, const char *strFile, unsigned int nLine ) {
    char buffer[2048];
    
    sprintf(buffer, "--- Assertion (%s) failed at file \'%s\' line %u ---\nAborting Process\n",
        strExp, strFile, nLine );
    broadcast(isDm, "^g%s", buffer);
    gServer->processOutput();
    
    logn("assert.log", buffer); 

    abort();
    
    return;
}
