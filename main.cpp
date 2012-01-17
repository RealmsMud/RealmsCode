/*
 * main.cpp
 *	 This files contains the main() function which initiates the game.
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

#include <sys/stat.h>
#include <sys/signal.h>
#include <unistd.h>
#include <time.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "commands.h"

bool listing = false;

void handle_args(int argc, char *argv[]);
void startup_mordor(void);


int main(int argc, char *argv[]) {
	// Get our instance variables
	gConfig = Config::getInstance();
	gServer = Server::getInstance();
	
	handle_args(argc, argv);
	startup_mordor();

	return(0);
}



