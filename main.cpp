/*
 * main.cpp
 *   This files contains the main() function which initiates the game.
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


//#include "mud.h"
#include "config.h"
#include "server.h"

void handle_args(int argc, char *argv[]);
void startup_mordor(void);

extern Config *gConfig;
extern Server *gServer;

int main(int argc, char *argv[]) {
    // Get our instance variables
    gConfig = Config::getInstance();
    gServer = Server::getInstance();
    
    handle_args(argc, argv);
    startup_mordor();

    return(0);
}



