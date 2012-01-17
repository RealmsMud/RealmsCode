/*
 * mordorMain.cpp
 *	 Mordor startup functions.
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
#include <errno.h>

#include "version.h"
#include "vprint.h"
#include "commands.h"

void handle_args(int argc, char *argv[]);
void startup_mordor(void);

unsigned short  Port;

extern long     last_weather_update;
extern int      Numplayers;

void startup_mordor(void) {
	char	buf[BUFSIZ];
	FILE	*out;

	LIBXML_TEST_VERSION

	StartTime = time(0);

	

	printf("Starting RoH Server" VERSION " compiled on " __DATE__ " at " __TIME__ " ");
	#ifdef __CYGWIN__
		printf("(CYGWIN)\n");
	#else
		printf("(LINUX)\n");
	#endif
	
#ifdef NODEMOGRAPHICS
		printf("Demographics disabled\n");
#endif
		
	if(gServer->isValgrind())
		printf("Running under valgrind\n");


	gServer->init();
	
	printf("--- Game Up: %d --- [%s]\n", Port, VERSION);
	loge("--- Game Up: %d --- [%s]\n", Port, VERSION);
	// record the process ID
	sprintf(buf, "%s/mordor%d.pid", Path::Log, Port);
	out = fopen(buf, "w");
	if(out != NULL) {
		fprintf(out, "%d", getpid());
		fclose(out);
	} else {
		loge("couldn't create pid file %s: %s\n", buf, strerror(errno));
	}

	std::cout << "Starting Sock Loop\n";
	gServer->run();

	return;

}

void usage(char *szName) {
	printf(" %s [port number] [-r]\n", szName);
	return;
}

void handle_args(int argc, char *argv[]) {
	int i=0;

	strncpy(gConfig->cmdline, argv[0], 255);
	gConfig->cmdline[255] = 0;
	
	bHavePort = 0;
	for(i = 1; i < argc; i++) {
		switch (argv[i][0]) {
		case '-':
		case '/':
			switch (argv[i][1]) {
			case 'g':
			case 'G':
				gServer->setGDB();
				break;
			case 'r':
			case 'R':
				gServer->setRebooting();
				break;
			case 'v':
			case 'V':
				gServer->setValgrind();
				break;
			default:
				printf("Unknown option.\n");
				usage(argv[0]);
				break;
			}
			break;
		default:
			if(is_num(argv[i])) {
				gConfig->portNum = (unsigned short)atoi(argv[i]);
				bHavePort = 1;
			} else {
				printf("Unknown option\n");
				usage(argv[0]);
			}
		}
	}

}
