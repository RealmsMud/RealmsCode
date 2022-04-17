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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <unistd.h>             // for getpid
#include <cerrno>               // for errno
#include <cstdio>               // for fclose, fopen, fprintf, printf, sprintf
#include <cstdlib>              // for atoi
#include <cstring>              // for strerror, strncpy
#include <ctime>                // for time
#include <iostream>             // for std::clog
#include <ostream>              // for operator<<, basic_ostream, char_traits

#include "config.hpp"           // for Config, gConfig
#include "libxml/xmlversion.h"  // for LIBXML_TEST_VERSION
#include "mud.hpp"              // for StartTime
#include "paths.hpp"            // for Log
#include "proto.hpp"            // for loge, is_num, handle_args, startup_mo...
#include "server.hpp"           // for Server, gServer
#include "version.hpp"          // for VERSION




unsigned short  Port;

extern long     last_weather_update;
extern int      Numplayers;

void startup_mordor() {
    char    buf[BUFSIZ];
    FILE    *out;

    LIBXML_TEST_VERSION

    StartTime = time(nullptr);

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

    gServer->run();

}

void usage(char *szName) {
    printf(" %s [port number] [-r]\n", szName);
}

void handle_args(int argc, char *argv[]) {
    int i=0;

    strncpy(gConfig->cmdline, argv[0], 255);
    gConfig->cmdline[255] = 0;

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
                gConfig->setPortNum((unsigned short)atoi(argv[i]));
            } else {
                std::clog << "Unknown option\n";
                usage(argv[0]);
            }
        }
    }

}
