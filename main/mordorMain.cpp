/*
 * mordorMain.cpp
 *   Mordor startup functions.
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
#include <unistd.h>

#include "config.hpp"
#include "mud.hpp"
#include "version.hpp"
#include "server.hpp"

void handle_args(int argc, char *argv[]);
void startup_mordor(void);

unsigned short  Port;

extern long     last_weather_update;
extern int      Numplayers;

void startup_mordor(void) {
    char    buf[BUFSIZ];
    FILE    *out;

    LIBXML_TEST_VERSION

    StartTime = time(0);



    std::clog << "Starting RoH Server " << VERSION " compiled on " << __DATE__ << " at " << __TIME__ << " " << "(LINUX)\n";

#ifdef NODEMOGRAPHICS
    std::clog << "Demographics disabled\n";
#endif
        
    if(gServer->isValgrind())
        std::clog << "Running under valgrind\n";


    gServer->init();

    std::clog << "--- Game Up: " << Port << " --- [" << VERSION << "]\n";
    loge("--- Game Up: %d --- [%s]\n", Port, VERSION);
    // record the process ID
    sprintf(buf, "%s/mordor%d.pid", Path::Log, Port);
    out = fopen(buf, "w");
    if(out != nullptr) {
        fprintf(out, "%d", getpid());
        fclose(out);
    } else {
        loge("couldn't create pid file %s: %s\n", buf, strerror(errno));
    }

    std::clog << "Starting Sock Loop\n";
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
    
    gConfig->bHavePort = false;
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
                std::clog << "Unknown option.\n";
                usage(argv[0]);
                break;
            }
            break;
        default:
            if(is_num(argv[i])) {
                gConfig->portNum = (unsigned short)atoi(argv[i]);
                gConfig->bHavePort = true;
            } else {
                std::clog << "Unknown option\n";
                usage(argv[0]);
            }
        }
    }

}
