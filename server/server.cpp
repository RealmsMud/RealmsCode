/*
 * server.cpp
 *   Server code, Handles sockets and input/output
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

#include <cassert>                                  // for assert
#include <bits/types/struct_tm.h>                   // for tm
#include <libxml/parser.h>                          // for xmlFreeDoc, xmlDo...
#include <netdb.h>                                  // for getnameinfo, EAI_...
#include <netinet/in.h>                             // for sockaddr_in, htons
#include <poll.h>                                   // for pollfd, poll, POL...
#include <csignal>                                  // for sigaction, signal
#include <sys/resource.h>                           // for rlimit, setrlimit
#include <sys/select.h>                             // for FD_ZERO, FD_ISSET
#include <sys/socket.h>                             // for AF_INET, accept
#include <sys/stat.h>                               // for umask
#include <sys/time.h>                               // for timeval
#include <sys/wait.h>                               // for wait3, waitpid
#include <unistd.h>                                 // for close, unlink, read
#include <algorithm>                                // for find
#include <boost/algorithm/string/replace.hpp>       // for replace_all
#include <boost/iterator/iterator_traits.hpp>       // for iterator_value<>:...
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <cerrno>                                   // for EWOULDBLOCK, errno
#include <cstdio>                                   // for snprintf, sprintf
#include <cstdlib>                                  // for exit, abort, srand
#include <cstring>                                  // for memset, strcpy
#include <ctime>                                    // for time, time_t, ctime
#include <deque>                                    // for _Deque_iterator
#include <iomanip>                                  // for operator<<, setw
#include <iostream>                                 // for operator<<, basic...
#include <list>                                     // for operator==, list
#include <libxml/xmlstring.h>                       // for BAD_CAST
#include <map>                                      // for operator==, map
#include <set>                                      // for set
#include <string>                                   // for string, allocator
#include <string_view>                              // for operator<<, strin...
#include <utility>                                  // for pair
#include <vector>                                   // for vector

#include "area.hpp"                                 // for MapMarker, Area
#include "calendar.hpp"                             // for Calendar
#include "catRef.hpp"                               // for CatRef
#include "color.hpp"                                // for stripColor
#include "config.hpp"                               // for Config, gConfig
#include "delayedAction.hpp"                        // for DelayedAction
#include "factions.hpp"                             // for Faction
#include "flags.hpp"                                // for M_PERMENANT_MONSTER
#include "global.hpp"                               // for FATAL, ALLITEMS
#include "httpServer.hpp"                           // for HttpServer
#include "lasttime.hpp"                             // for lasttime
#include "login.hpp"                                // for CON_DISCONNECTING
#include "magic.hpp"                                // for S_CURE_POISON
#include "money.hpp"                                // for Money, GOLD
#include "mud.hpp"                                  // for Port, LT, StartTime
#include "mudObjects/areaRooms.hpp"                 // for AreaRoom
#include "mudObjects/container.hpp"                 // for MonsterSet, Playe...
#include "mudObjects/creatures.hpp"                 // for Creature, ATTACK_...
#include "mudObjects/monsters.hpp"                  // for Monster
#include "mudObjects/mudObject.hpp"                 // for MudObject
#include "mudObjects/objects.hpp"                   // for Object, DroppedBy
#include "mudObjects/players.hpp"                   // for Player
#include "mudObjects/rooms.hpp"                     // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"               // for UniqueRoom
#include "paths.hpp"                                // for Config, Game, Are...
#include "proc.hpp"                                 // for childProcess, Chi...
#include "proto.hpp"                                // for broadcast, isDay
#include "pythonHandler.hpp"                        // for PythonHandler
#include "random.hpp"                               // for Random
#include "ships.hpp"                                // for Ship
#include "server.hpp"                               // for Server, Server::c...
#include "serverTimer.hpp"                          // for ServerTimer
#include "socket.hpp"                               // for Socket, xmlNode
#include "stats.hpp"                                // for Stat
#include "structs.hpp"                              // for daily
#include "version.hpp"                              // for VERSION
#include "wanderInfo.hpp"                           // for WanderInfo
#include "xml.hpp"                                  // for copyToNum, newNum...


// External declarations
extern int Numplayers;
extern long last_time_update;
extern long last_weather_update;

// Function prototypes
bool init_spelling();  // TODO: Move spelling stuff into server
void initSpellList();

// Global server pointer
Server* gServer = nullptr;

// Static initialization
Server* Server::myInstance = nullptr;

bool CanCleanupRoomFn::operator()( const std::shared_ptr<UniqueRoom>& r ) { return r->players.empty(); }

void CleanupRoomFn::operator()(const std::shared_ptr<UniqueRoom>& r ) {
	r->saveToFile(PERMONLY);
}

// Custom comparison operator to sort by the numeric id instead of standard string comparison
bool idComp::operator() (const std::string& lhs, const std::string& rhs) const {
    std::stringstream strL(lhs);
    std::stringstream strR(rhs);

    char charL, charR;
    long longL, longR;

    strL >> charL >> longL;
    strR >> charR >> longR;

    if(charL == charR) {
        if(charL == 'R') {
            // Rooms work a bit differently
            return(lhs < rhs);
        }
        else
            return(longL < longR);
    } else {
        return(charL < charR);
    }
    return(true);
}
//--------------------------------------------------------------------
// Constructors, Destructors, etc

//********************************************************************
//                      Server
//********************************************************************

Server::Server(): roomCache(RQMAX, true), monsterCache(MQMAX, false), objectCache(OQMAX, false) {
	std::clog << "Constructing the Server." << std::endl;
    FD_ZERO(&inSet);
    FD_ZERO(&outSet);
    FD_ZERO(&excSet);
    rebooting = GDB = valgrind = false;

    running = false;
    pulse = 0;
    webInterface = nullptr;
    lastDnsPrune = lastUserUpdate = lastRoomPulseUpdate = lastRandomUpdate = lastActiveUpdate = 0;
    maxPlayerId = maxObjectId = maxMonsterId = 0;
    loadDnsCache();
    pythonHandler = nullptr;
    httpServer = nullptr;
    idDirty = false;

#ifdef SQL_LOGGER
    connActive = false;
#endif // SQL_LOGGER

}

//********************************************************************
//                      ~Server
//********************************************************************

Server::~Server() {
    std::clog << "Destroying the server!" << std::endl;
    if(running) {
        // Do shutdown here
    }
    sockets.clear();
    players.clear();
    areas.clear();
    activeList.clear();
    flushRoom();
    flushObject();
    flushMonster();
    effectsIndex.clear();
    PythonHandler::cleanUpPython();
    cleanupDiscordBot();
    cleanupHttpServer();


    delete vSockets;
#ifdef SQL_LOGGER
    cleanUpSql();
#endif // SQL_LOGGER

}

//********************************************************************
//                      init
//********************************************************************

bool Server::init() {

    std::clog << "Initializing Server." << std::endl;

    std::clog << "Setting RLIMIT...";
    struct rlimit lim{};
    lim.rlim_cur = RLIM_INFINITY;
    lim.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &lim);
    std::clog << "done\n";

    std::clog << "Installing signal handlers.";
    installSignalHandlers();
    std::clog << "done." << std::endl;

    std::clog << "Installing unique IDs...";
    loadIds();
    std::clog << "done." << std::endl;

    std::clog << "Installing custom printf handlers...";
    if(installPrintfHandlers() == 0)
        std::clog << "done." << std::endl;
    else
        std::clog << "failed." << std::endl;

    gConfig->loadBeforePython();
    gConfig->setLotteryRunTime();

    Port = gConfig->getPortNum();
    Tablesize = getdtablesize();


    std::clog << "Initializing Spelling...";
    if(init_spelling())
        std::clog << "done." << std::endl;
    else
        std::clog << "failed." << std::endl;

    initWebInterface();

    std::clog <<  "Initializing Spell List...";
    initSpellList();
    std::clog << "done." << std::endl;

    // Python
    std::clog <<  "Initializing Python...";
    if(!PythonHandler::initPython()) {
        std::clog << "failed!" << std::endl;
        exit(-1);
    }

    std::clog << "Loading Areas..." << (loadAreas() ? "done" : "*** FAILED ***") << std::endl;
    gConfig->loadAfterPython();

    initHttpServer();
    initDiscordBot();



#ifdef SQL_LOGGER
    std::clog <<  "Initializing SQL Logger...";
    if(initSql())
        std::clog << "done." << std::endl;
    else
        std::clog << "failed." << std::endl;
#endif // SQL_LOGGER

    umask(000);
    srand(getpid() + time(nullptr));
    if(rebooting) {
        std::clog << "Doing a reboot." << std::endl;
        finishReboot();
    } else if (!gConfig->isListing()) {
        addListenPort(Port);
        std::error_code ec;
        fs::remove(Path::Config / "reboot.xml", ec);
    }
    return(true);
}

//********************************************************************
//                      installSignalHandlers
//********************************************************************

void Server::installSignalHandlers() {
    if (!gConfig->isListing()) {
        // TODO(SPT)
//        struct sigaction crash_sa{};
//        crash_sa.sa_handler = crash;
//        sigaction(SIGABRT, &crash_sa, nullptr); // abnormal termination triggered by abort call
//        std::clog << ".";
//        sigaction(SIGFPE, &crash_sa, nullptr);  // floating point exception
//        std::clog << ".";
//        sigaction(SIGSEGV, &crash_sa, nullptr); // segment violation
//        std::clog << ".";
    } else {
        std::clog << "Ignoring crash handlers";
    }
    signal(SIGPIPE, SIG_IGN);
    std::clog << ".";
    signal(SIGCHLD, SIG_IGN);
    std::clog << ".";

    struct sigaction shutdown_sa{};
    shutdown_sa.sa_handler = shutdown_now;
    sigaction(SIGTERM, &shutdown_sa, nullptr);
    std::clog << ".";
    sigaction(SIGHUP, &shutdown_sa, nullptr);
    std::clog << ".";
    sigaction(SIGINT, &shutdown_sa, nullptr);
    std::clog << ".";
}


void Server::setGDB() { GDB = true; }
void Server::setRebooting() { rebooting = true; }
void Server::setValgrind() { valgrind = true; }
bool Server::isRebooting() { return(rebooting); }
bool Server::isValgrind() { return(valgrind); }
size_t Server::getNumSockets() const { return(sockets.size()); }

// End - Constructors, Destructors, etc
//--------------------------------------------------------------------


//--------------------------------------------------------------------
// Instance Functions

//********************************************************************
// Get Instance - Return the static instance of config
//********************************************************************
Server* Server::getInstance() {
    if(myInstance == nullptr)
        myInstance = new Server;
    return(myInstance);
}
//********************************************************************
// Destroy Instance - Destroy the static instance
//********************************************************************
void Server::destroyInstance() {
    delete myInstance;
    myInstance = nullptr;
}

// End - Instance Functions
//--------------------------------------------------------------------



void Server::populateVSockets() {
    if(vSockets)
        return;
    vSockets = new SocketVector();
    for(auto &sock : sockets) vSockets->push_back(sock);

    Random::shuffle(vSockets->begin(), vSockets->end());
}


//********************************************************************
//                      run
//********************************************************************

void Server::run() {
    ServerTimer timer{};
    if(!running) {
        std::cerr << "Not bound to any ports, exiting." << std::endl;
        exit(-1);
    }

    if(httpServer)
        httpServer->run();

    std::clog << "Starting Sock Loop\n";
    while(running) {
        if(!children.empty()) reapChildren();

        processChildren();
        timer.start(); // Start the timer

        populateVSockets();

        poll();

        checkNew();

        processInput();

        processCommands();

        updatePlayerCombat();

        // Update game here
        updateGame();

        processMsdp();

        processOutput();

        cleanUp();
        // Temp
        pulse++;

        checkWebInterface();

        delete vSockets;
        vSockets = nullptr;

        timer.end(); // End the timer
        timer.sleep();
    }

}

//********************************************************************
//                      addListenPort
//********************************************************************

int Server::addListenPort(int port) {
    struct sockaddr_in sa{};
    int optval = 1;
    int control;

    memset((char *)&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if((control = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::clog << "Error with socket\n";
        return(-1);
    }

    if(setsockopt(control, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
        std::clog << "Error with setSockOpt\n";
        return(-1);
    }

    if(bind(control, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        close(control);
        std::clog << "Unable to bind to port " << port << std::endl;
        return(-1);
    }

    if(nonBlock(control) != 0) {
        std::clog << "Error with nonBlock\n";
        return(-1);
    }

    if(listen(control, 100) != 0) {
        std::clog << "Error with listen\n";
        return(-1);
    }

    std::clog << "Mud is now listening on port " << port << std::endl;

    // TODO: Leaky, make sure to erase these when the server shuts down
    controlSocks.emplace_back(port, control);
    running = true;

    return(0);
}

//********************************************************************
//                      poll
//********************************************************************

int Server::poll() {
    if(controlSocks.empty()) {
        std::clog << "Not bound to any ports, nothing to poll.\n";
        exit(0);
    }

    int maxFd = 0;

    struct timeval noTime{};
    noTime.tv_sec = 0;
    noTime.tv_usec = 0;

    FD_ZERO(&inSet);
    FD_ZERO(&outSet);
    FD_ZERO(&excSet);

    for(controlSock & cs : controlSocks) {
        if(cs.control > maxFd)
            maxFd = cs.control;
        FD_SET(cs.control, &inSet);
    }

    for(const auto &sock : sockets) {
        if(sock->getFd() > maxFd)
            maxFd = sock->getFd();
        FD_SET(sock->getFd(), &inSet);
        FD_SET(sock->getFd(), &outSet);
        FD_SET(sock->getFd(), &excSet);
    }

    if(select(maxFd+1, &inSet, &outSet, &excSet, &noTime) < 0)
        return(-1);

    return(0);
}

//********************************************************************
//                      checkNew
//********************************************************************

int Server::checkNew() {
    for(controlSock & cs : controlSocks) {
        if(FD_ISSET(cs.control, &inSet)) { // We have a new connection waiting
            std::clog << "Got a new connection on port " << cs.port << ", control sock " << cs.control << std::endl;
            handleNewConnection(cs);
        }
    }
    return(0);
}

//********************************************************************
//                      handleNewConnection
//********************************************************************

int Server::handleNewConnection(controlSock& cs) {
    int fd;
    int len;

    struct sockaddr_in addr{};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(Port);
    addr.sin_addr.s_addr = INADDR_ANY;
    len = sizeof(struct sockaddr_in);

    if(( fd = accept(cs.control, (struct sockaddr *) &addr, (socklen_t *) &len)) < 0) {
        return(-1);
    }

    // Game's full, drop the connection
    if(getNumSockets() > Tablesize-10) {
        close(fd);
        return -1;
    }
    sockets.emplace_back(std::make_shared<Socket>(fd, addr, false));
    return(0);
}

//********************************************************************
//                      processInput
//********************************************************************

int Server::processInput() {

    for(const auto &sock : sockets) {
        if(sock->getState() == CON_DISCONNECTING)
            continue;

        // Clear out the descriptor of we have an exception
        if(FD_ISSET(sock->getFd(), &excSet)) {
            FD_CLR(sock->getFd(), &inSet);
            FD_CLR(sock->getFd(), &outSet);
            sock->setState(CON_DISCONNECTING);
            std::clog << "Exception found\n";
            continue;
        }
        // Try to read something
        if(FD_ISSET(sock->getFd(), &inSet)) {
            if(sock->processInput() != 0) {
                FD_CLR(sock->getFd(), &outSet);
                std::clog << "Error reading from socket " << sock->getFd() << std::endl;
                sock->setState(CON_DISCONNECTING);
                continue;
            }
        }
    }
    return(0);
}

//********************************************************************
//                      processCommands
//********************************************************************

int Server::processCommands() {
    for(const auto &unlockedSock : *vSockets) {
        if(auto sock = unlockedSock.lock()) {
            if(sock->hasCommand() && sock->getState() != CON_DISCONNECTING && sock->processOneCommand() == -1) {
                sock->setState(CON_DISCONNECTING);
                continue;
            }
        }
    }
    return(0);
}

//********************************************************************
//                      updatePlayerCombat
//********************************************************************

int Server::updatePlayerCombat() {
    for(const auto &unlockedSock : *vSockets) {
        if(auto sock = unlockedSock.lock()) {
            if(auto player = sock->getPlayer()) {
                if (player->isFleeing() && player->canFlee(false)) {
                    player->doFlee();
                } else if (player->autoAttackEnabled() && !player->isFleeing() && player->hasAttackableTarget() && player->isAttackingTarget()) {
                    if (!player->checkAttackTimer(false))
                        continue;

                    player->attackCreature(player->getTarget(), ATTACK_NORMAL);
                }
            }
        } else {
            std::clog << "Error locking socket" << std::endl;
        }
    }
    return(0);
}


//********************************************************************
//                      processOutput
//********************************************************************

int Server::processOutput() {
    // This can be called outside of the normal server loop so verify VSockets is populated
    populateVSockets();
    for(const auto& unlockedSock : *vSockets) {
        if(auto sock = unlockedSock.lock()) {
            if (FD_ISSET(sock->getFd(), &outSet) && sock->hasOutput()) {
                sock->flush();
            }
        }
    }
    return(0);
}

//********************************************************************
//                      disconnectAll
//********************************************************************

void Server::disconnectAll() {
    for(const auto &sock : sockets) {
        // Set everyone to disconnecting
        sock->setState(CON_DISCONNECTING);
    }
    // And now clean them up
    cleanUp();
}

//********************************************************************
//                      cleanUp
//********************************************************************

bool isDisconnecting(const std::shared_ptr<Socket>& sock) {
    if(sock->getState() == CON_DISCONNECTING) {
        // Flush any residual data
        sock->flush();
        return true;
    }
    return false;
}

int Server::cleanUp() {
    sockets.remove_if(isDisconnecting);
    return(0);
}

//********************************************************************
//                      getDnsCacheString
//********************************************************************

std::string Server::getDnsCacheString() {
    std::ostringstream dnsStr;
    int num = 0;

    dnsStr << "^cCached DNS Information\r\n";
    dnsStr << "IP               | Address\r\n";
    dnsStr << "---------------------------------------------------------------\n";

    dnsStr.setf(std::ios::left, std::ios::adjustfield);
    for(dnsCache & dns : cachedDns) {
        dnsStr << "^c" << std::setw(16) << dns.ip << " | ^C" << dns.hostName << "\n";
        num++;
    }

    dnsStr << "\n\n^x Found " << num << " cached item(s).\n";
    return(dnsStr.str());
}

//********************************************************************
//                      getDnsCache
//********************************************************************

bool Server::getDnsCache(std::string &ip, std::string &hostName) {
    for(dnsCache & dns : cachedDns) {
        if(dns.ip == ip) {
            // Got a match
            hostName = dns.hostName;
            std::clog << "DNS: Found " << ip << " in dns cache\n";
            return(true);
        }
    }
    return(false);
}

//********************************************************************
//                      pruneDns
//********************************************************************

void Server::pruneDns() {
    long fifteenDays = 60*60*24*15;
    long currentTime = time(nullptr);
    std::list<dnsCache>::iterator it;
    std::list<dnsCache>::iterator oldIt;

    std::clog << "Pruning DNS\n";
    for( it = cachedDns.begin(); it != cachedDns.end() ; ) {
        oldIt = it++;
        if(currentTime - (*oldIt).time >= fifteenDays) {
            cachedDns.erase(oldIt);
        }
    }
    saveDnsCache();
    lastDnsPrune = currentTime;
}

//********************************************************************
//                      pulseTicks
//********************************************************************

void Server::pulseTicks(long t) {

    for(const auto& unlockedSock : *vSockets) {
        if(auto sock = unlockedSock.lock()) {
            std::shared_ptr<Player> player = sock->getPlayer();
            if (player) {
                player->pulseTick(t);
                if (player->isPlaying())
                    player->pulseSong(t);
            }
        }
    }
}



//*********************************************************************
//                      updateUsers
//*********************************************************************

void Server::updateUsers(long t) {
    int tout;
    lastUserUpdate = t;

    for(const auto& unlockedSock : *vSockets) {
        if(auto sock = unlockedSock.lock()) {
            std::shared_ptr<Player> player = sock->getPlayer();

            if (player) {
                if (player->isDm()) tout = INT_MAX;
                else if (player->isStaff()) tout = 1200;
                else tout = 600;
            } else {
                tout = 300;
            }
            if (t - sock->ltime > tout) {
                sock->write("\n\rTimed out.\n\r");
                sock->setState(CON_DISCONNECTING);
            }
            if (player) {
                player->checkOutlawAggro();
                player->update();
            }
        }
    }
}

//********************************************************************
//                      update_random
//********************************************************************
// This function checks all player-occupied rooms to see if random monsters
// have entered them.  If it is determined that random monster should enter
// a room, it is loaded and items it is carrying will be loaded with it.

void Server::updateRandom(long t) {
    std::shared_ptr<Monster>  monster=nullptr;
    std::shared_ptr<BaseRoom> room=nullptr;
    std::shared_ptr<UniqueRoom> uRoom=nullptr;
    std::shared_ptr<AreaRoom> aRoom=nullptr;
    WanderInfo* wander=nullptr;
    CatRef  cr;
    int     num=0, l=0;
    std::map<std::string, bool> check;

    lastRandomUpdate = t;

    std::shared_ptr<Player> player;
    for(const auto &unlockedSock : *vSockets) {
        if(auto sock = unlockedSock.lock()) {
            player = sock->getPlayer();

            if (!player || !player->getRoomParent())
                continue;
            uRoom = player->getUniqueRoomParent();
            aRoom = player->getAreaRoomParent();
            room = player->getRoomParent();

            if (uRoom) {
                // handle monsters arriving in unique rooms
                if (!uRoom->info.id)
                    continue;

                if (check.find(uRoom->info.displayStr()) != check.end())
                    continue;

                check[uRoom->info.displayStr()] = true;
                wander = &uRoom->wander;
            } else {
                // handle monsters arriving in area rooms
                if (check.find(aRoom->mapmarker.str()) != check.end())
                    continue;
                if (aRoom->unique.id)
                    continue;

                check[aRoom->mapmarker.str()] = true;
                wander = aRoom->getRandomWanderInfo();
            }
            if (!wander)
                continue;

            if (Random::get(1, 100) > wander->getTraffic())
                continue;

            cr = wander->getRandom();
            if (!cr.id)
                continue;
            if (room->countCrt() >= room->getMaxMobs())
                continue;

            // Will make mobs not spawn if a DM is invis in the room. -TC
            if (!room->countVisPly())
                continue;

            if (!loadMonster(cr, monster))
                continue;

            // if the monster can't go there, they won't wander there
            if (aRoom) {
                auto aArea = aRoom->area.lock();
                if (!aArea || !aArea->canPass(monster, aRoom->mapmarker, true)) {
                    continue;
                }
            }

            if (!monster->flagIsSet(M_CUSTOM))
                monster->validateAc();

            if (((monster->flagIsSet(M_NIGHT_ONLY) && isDay()) || (monster->flagIsSet(M_DAY_ONLY) && !isDay())) && !monster->inCombat()) {
                continue;
            }

            if (room->flagIsSet(R_PLAYER_DEPENDENT_WANDER))
                num = Random::get(1, room->countVisPly());
            else if (monster->getNumWander() > 1)
                num = Random::get<unsigned short>(1, monster->getNumWander());
            else
                num = 1;

            for (l = 0; l < num; l++) {
                monster->initMonster();

                if (monster->flagIsSet(M_PERMENANT_MONSTER))
                    monster->clearFlag(M_PERMENANT_MONSTER);

                if (!l)
                    monster->addToRoom(room, num);
                else
                    monster->addToRoom(room, 0);

                if (!monster->flagIsSet(M_PERMENANT_MONSTER) || monster->flagIsSet(M_NO_ADJUST))
                    monster->adjust(-1);

                gServer->addActive(monster);
                if (l != num - 1)
                    loadMonster(cr, monster);
            }
        }
    }
}

//*********************************************************************
//                      update_active
//*********************************************************************
// This function updates the activities of all monsters who are currently
// active (ie. monsters on the active list). Usually this is reserved
// for monsters in rooms that are occupied by players.

void Server::updateActive(long t) {
    std::shared_ptr<Creature> target = nullptr;
    std::shared_ptr<BaseRoom> room = nullptr;

    long    tt = gConfig->currentHour();
    int     timetowander=0, immort=0;
    bool    shouldoPrint=false;

    lastActiveUpdate = t;

    if(activeList.empty())
        return;

    auto it = activeList.begin();
    while(it != activeList.end()) {
        std::shared_ptr<Monster>  monster;
        // Increment the iterator in case this monster dies during the update and is removed from the active list
        if(!(monster = it->lock())) {
            std::clog << "UpdateActive: Can't lock monster" << std::endl;
            it = activeList.erase(it);
            continue;
        }


        // Better be a monster to be on the active list
        if(!monster->inRoom()) {
            broadcast(isStaff, "^y%s without a parent/area room on the active list. Info: %s. Deleting.", monster->getCName(), monster->info.displayStr().c_str());
            monster->deleteFromRoom();
            it = activeList.erase(it);
            continue;
        }


        room = monster->getRoomParent();

        // Reset's mob's smack-talking broadcasts at 7am every day
        if(tt == 7 && ((t - last_time_update) / 2) == 0)
            monster->daily[DL_BROAD].cur = 20;

        if( (monster->flagIsSet(M_NIGHT_ONLY) && isDay()) || (monster->flagIsSet(M_DAY_ONLY) && !isDay())) {
            for(const auto& ply: room->players) {
                if(ply->isStaff()) {
                    immort = 1;
                    break;
                }
            }
            if(!immort) {
                timetowander=1;
                broadcast(nullptr, monster->getRoomParent(), "%M wanders slowly away.", monster.get());
                monster->deleteFromRoom();
                it = activeList.erase(it);
                continue;
            }

        }

        // fast wanderers and pets always stay active
        if( room->players.empty() &&
            !monster->flagIsSet(M_ALWAYS_ACTIVE) &&
            !monster->flagIsSet(M_FAST_WANDER) &&
            !monster->flagIsSet(M_FAST_TICK) &&
            !monster->flagIsSet(M_REGENERATES) &&
            !monster->isPet() &&
            !monster->isPoisoned() &&
            !monster->isEffected("slow") &&
            !monster->flagIsSet(M_PERMENANT_MONSTER) &&
            !monster->flagIsSet(M_AGGRESSIVE))
        {
            std::clog << "Removing " << monster->getName() << " from active list" << std::endl;
            it = activeList.erase(it);
            continue;
        }

        // Lets see if we'll attack any other monsters in this room
        if(monster->checkEnemyMobs()) {
            it++;
            continue;
        }


        if(monster->flagIsSet(M_KILL_PERMS)) {
            for(const auto& mons : room->monsters) {
                if(mons == monster)
                    continue;
                if( mons->flagIsSet(M_PERMENANT_MONSTER) && !monster->willAssist(mons->getAsMonster()) && !monster->isEnemy(mons))
                    monster->addEnemy(mons);
            }
        }

        if(monster->flagIsSet(M_KILL_NON_ASSIST_MOBS)) {
            for(const auto& mons : room->monsters) {
                if(mons == monster)
                    continue;
                if( !monster->willAssist(mons->getAsMonster()) &&
                    !mons->flagIsSet(M_PERMENANT_MONSTER) &&
                    !monster->isEnemy(mons) && 
                    !mons->isPet()
                )
                    monster->addEnemy(mons);
            }
        }

        monster->checkAssist();
        monster->pulseTick(t);
        monster->checkSpellWearoff();

        if(monster->isPoisoned() && !monster->immuneToPoison()) {
            if( monster->spellIsKnown(S_CURE_POISON) &&
                monster->mp.getCur() >=6 &&
                (Random::get(1,100) < (30+monster->intelligence.getCur()/10)))
            {
                broadcast(nullptr, monster->getRoomParent(), "%M casts a curepoison spell on %sself.", monster.get(), monster->himHer());
                monster->mp.decrease(6);
                monster->curePoison();
                it++;
                continue;
            }
        }

        if(!monster->checkAttackTimer(false)) {
            it++;
            continue;
        }


        if(t > LT(monster, LT_CHARMED) && monster->flagIsSet(M_CHARMED))
            monster->clearFlag(M_CHARMED);


        if(monster->doHarmfulAuras()) {
            it = activeList.begin();
            continue;
        }

        // Calls beneficial casting routines for mobs.
        if(monster->flagIsSet(M_BENEVOLENT_SPELLCASTER))
            monster->beneficialCaster();

        if(monster->petCaster()) {
            it++;
            continue;
        }


        if(!monster->getPrimeFaction().empty())
            Faction::worshipSocial(monster);


        // summoned monsters expire here
        if( monster->isPet() && (t > LT(monster, LT_INVOKE) || t > LT(monster, LT_ANIMATE))) {
            if(monster->isUndead())
                broadcast(nullptr, room, "%1M wanders away.", monster.get());
            else
                broadcast(nullptr, room, "%1M fades away.", monster.get());

            it = activeList.erase(it);
            monster->die(monster->getMaster());
            continue;
        }

        monster->updateAttackTimer();
        if(monster->dexterity.getCur() > 200 || monster->isEffected("haste"))
            monster->modifyAttackDelay(-10);
        if(monster->isEffected("slow"))
            monster->modifyAttackDelay(10);


        if(monster->flagIsSet(M_WILL_WIELD))
            monster->mobWield();

        // Grab some stuff from the room
        monster->checkScavange(t);

        // See if we can wander around or away
        int mobileResult = monster->checkWander(t);
        if(mobileResult == 1) {
            it++;
            continue;
        } if(mobileResult == 2) {
            it = activeList.erase(it);
            monster->deleteFromRoom();
            continue;
        }


        // Steal from people
        if(monster->flagIsSet(M_STEAL_ALWAYS) && (t - monster->lasttime[LT_STEAL].ltime) > 60 && Random::get<bool>(0.05)) {
            std::shared_ptr<Player> ply = lowest_piety(room, monster->isEffected("detect-invisible"));
            if(ply)
                monster->steal(ply);
        }


        // Try to death scream
        if( monster->flagIsSet(M_DEATH_SCREAM) &&
            monster->hasEnemy() &&
            monster->nearEnemy() &&
            !monster->flagIsSet(M_CHARMED) &&
            monster->canSpeak() &&
            monster->mobDeathScream()
        ) {
            it = activeList.begin();
            continue;
        }

        // Update combat here
        if( monster->hasEnemy() && !timetowander && monster->updateCombat()) {
            it = activeList.begin();
            continue;
        }

        if( !monster->flagIsSet(M_AGGRESSIVE) &&
            monster->flagIsSet(M_WILL_BE_AGGRESSIVE) &&
            !monster->hasEnemy() &&
            Random::get(1,100) <= monster->getUpdateAggro() &&
            room->countVisPly()
        ) {
            //broadcast(isDm, "^g%M(L%d,R:%s) just became aggressive!", monster, monster->getLevel(), room->fullName().c_str());
            monster->setFlag(M_AGGRESSIVE);
        }


        target = monster->whoToAggro();
        if(target) {

            shouldoPrint = (!monster->flagIsSet(M_HIDDEN) && !(monster->isInvisible() && target->isEffected("detect-invisible")));

            monster->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
            monster->addEnemy(target, shouldoPrint);

            if(target->flagIsSet(P_LAG_PROTECTION_SET))
                target->setFlag(P_LAG_PROTECTION_ACTIVE);

        }
        it++;
    }
}






//--------------------------------------------------------------------
// Active list manipulation

//*********************************************************************
//                      add_active
//*********************************************************************
// This function adds a monster to the active-monster list. A pointer
// to the monster is passed in the first parameter.

void Server::addActive(const std::shared_ptr<Monster>& monster) {

    if(!monster) {
        loga("add_active: tried to activate a null crt!\n");
        return;
    }
    if(isActive(monster.get()))
        return;

    monster->validateId();

    activeList.push_back(monster);
}

//*********************************************************************
//                      del_active
//*********************************************************************
// This function removes a monster from the active-monster list. The
// parameter contains a pointer to the monster which is to be removed

void Server::delActive(Monster* monster) {
    if(activeList.empty()) {
        return;
    }
    const auto it = std::find_if(activeList.begin(), activeList.end(), [&monster](const std::weak_ptr<Creature>& crt) {
        auto locked = crt.lock();
        return locked && locked.get() == monster;
    });


    if(it == activeList.end()) {
        // No longer an error condition with weak ptrs
        return;
    }


    activeList.erase(it);
}


//*********************************************************************
//                      isActive
//*********************************************************************
// This function returns 1 if the parameter passed is in the
// active list.

bool Server::isActive(Monster* monster) {
    if(activeList.empty())
        return(false);

    const auto it = std::find_if(activeList.begin(), activeList.end(), [&monster](const std::weak_ptr<Creature>& crt) {
        auto locked = crt.lock();
        return locked && locked.get() == monster;
    });

    if(it == activeList.end())
        return(false);

    return true;

}

// End - Active List Manipulation
//--------------------------------------------------------------------


//--------------------------------------------------------------------
// Children Control


//********************************************************************
//                      reapChildren
//********************************************************************

int Server::reapChildren() {
    int status;
    bool dnsChild = false;
    childProcess myChild;
    const childProcess* cp;
    bool found=false;
    std::cout << "Reaping Children (maybe)\n";
    pollfd *fds = nullptr;

    while(!children.empty()) {
        delete[] fds;
        fds = new pollfd[children.size()];
        int i = 0;
        for(const childProcess& c : children) {
            fds[i].fd = c.fd;
            fds[i++].events = POLLHUP;
        }
        int ret = ::poll(fds, i, -1);
        if (ret <= 0) break;

        std::list<childProcess>::const_iterator it, oldIt;
        for( it = children.begin(), i=0; it != children.end() ; i++) {
            cp = &*it;
            oldIt = it++;

            if(fds[i].revents == 0) {
                continue;
            } else if (fds[i].revents != POLLHUP) {
                std::cout << "Unexpected revent " << fds[i].revents << std::endl;
                continue;
            }

            std::cout << "waitpid " << cp->pid << std::endl;
            waitpid(cp->pid, &status, WNOHANG);

            if(cp->type == ChildType::DNS_RESOLVER) {
                char tmpBuf[1024];
                memset(tmpBuf, '\0', sizeof(tmpBuf));
                // Read in the results from the resolver
                size_t n = read(cp->fd, tmpBuf, 1023);

                // Close the read fd in the pipe now, won't need it anymore
                close(cp->fd);
                // If we have an error reading, just use the ip address then
                if( n <= 0 ) {
                    if(errno == EWOULDBLOCK)
                        std::clog << "DNS ReapChildren: Error would block\n";
                    else
                        std::clog << "DNS ReapChildren: Error\n";
                    strcpy(tmpBuf, cp->extra.c_str());
                }

                // Add dns to cache
                addCache(cp->extra, tmpBuf);
                dnsChild = true;

                // Now we want to look through all connected sockets and update dns where appropriate
                for(const auto &sock : sockets) {
                    if(sock->getState() == LOGIN_DNS_LOOKUP && sock->getIp() == cp->extra) {
                        // Be sure to set the hostname first, then check for lockout
                        sock->setHostname(tmpBuf);
                        sock->checkLockOut();
                    }
                }
                std::clog << "Reaped DNS child (" << cp->pid << "-" << tmpBuf << ")\n";
            } else if(cp->type == ChildType::LISTER) {
                std::clog << "Reaping LISTER child (" << cp->pid << "-" << cp->extra << ")" << std::endl;
                processListOutput(*cp);
                // Don't forget to close the pipe!
                close(cp->fd);
            } else if(cp->type == ChildType::SWAP_FINISH) {
                std::clog << "Reaping MoveRoom Finish child (" << cp->pid << "-" << cp->extra << ")" << std::endl;
                myChild = *it;
                found = true;
            } else if(cp->type == ChildType::PRINT) {
                //broadcast(isDm, "Reaping Print Child (%d-%s)",pid, cp->extra.c_str());
                std::clog << "Reaping Print child (" << cp->pid << "-" << cp->extra << ")" << std::endl;
                myChild = *it;
                found = true;
            } else {
                std::clog << "ReapChildren: Unknown child type " << (int)cp->type << std::endl;
            }
            children.erase(oldIt);

            // finish swap after they've been deleted from the list
            if(found) {
                found = false;
                if(myChild.type == ChildType::SWAP_FIND) {
                    gConfig->findNextEmpty(myChild, true);
                } else if(myChild.type == ChildType::SWAP_FINISH) {
                    gConfig->offlineSwap(myChild, true);
                } else if(myChild.type == ChildType::PRINT) {
                    const std::shared_ptr<Player> player = gServer->findPlayer(myChild.extra);
                    std::string output = gServer->simpleChildRead(myChild);
                    if(player && !output.empty())
                        player->printColor("%s\n", output.c_str());
                }
                // Don't forget to close the pipe!
                close(myChild.fd);
            }
        }
    }
    if(dnsChild)
        saveDnsCache();
    if(fds != nullptr) {
        delete[] fds;
        fds = nullptr;
    }
    // just in case, kill off any zombies
    wait3(&status, WNOHANG, (struct rusage *)nullptr);
    return(0);
}

//********************************************************************
//                      processListOutput
//********************************************************************

int Server::processListOutput(const childProcess &lister) {
    bool found = false;
    std::shared_ptr<Socket> foundSock;
    for(auto& sock : sockets) {
        if(sock->getPlayer() && lister.extra == sock->getPlayer()->getName()) {
            found = true;
            foundSock = sock;
            break;
        }
    }

    char tmpBuf[4096];
    std::string toWrite;
    size_t n;
    for(;;) {
        // Even if no socket is found, read in all the data
        memset(tmpBuf, '\0', sizeof(tmpBuf));
        n = read(lister.fd, tmpBuf, sizeof(tmpBuf)-1);
        if(n <= 0)
            break;

        if(found) {
            toWrite = tmpBuf;
            boost::replace_all(toWrite, "\n", "\nList> ");
            foundSock->write(toWrite, false);
        }
    }
    return(1);
}

//********************************************************************
//                      processChildren
//********************************************************************

int Server::processChildren() {
    for(childProcess & child : children) {
        if(child.type == ChildType::DNS_RESOLVER) {
            // Ignore, will be handled by reapChildren
        } else if(child.type == ChildType::LISTER) {
            processListOutput(child);
        } else if(child.type == ChildType::SWAP_FIND) {
            gConfig->findNextEmpty(child, false);
        } else if(child.type == ChildType::SWAP_FINISH) {
            gConfig->offlineSwap(child, false);
        } else if(child.type == ChildType::PRINT) {
            const std::shared_ptr<Player> player = gServer->findPlayer(child.extra);
            std::string output = gServer->simpleChildRead(child);
            if(player && !output.empty())
                player->printColor("%s\n", output.c_str());
        } else {
            std::clog << "processChildren: Unknown child type " << (int)child.type << std::endl;
        }
    }
    return(1);
}


//********************************************************************
//                      startDnsLookup
//********************************************************************

int Server::startDnsLookup(Socket* sock, struct sockaddr_in addr) {
    int fds[2];
    int pid;

    // We're going to make a pipe here.
    // The child process will write the
    if(pipe(fds) == -1) {
        std::clog << "DNS: Error with pipe!\n";
        abort();
    }
    pid = fork();
    if(!pid) {
        // Child Process: Close the reading end, we'll only be writing
        close(fds[0]);
        int tries = 0, res = 0;
        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

        while(tries < 5 && tries >= 0) {
            res = getnameinfo((struct sockaddr*) &addr, sizeof(addr), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NAMEREQD);

            if (res != 0) {
                switch(res) {
                    case EAI_FAIL:
                    case EAI_BADFLAGS:
                    case EAI_MEMORY:
                    case EAI_OVERFLOW:
                    case EAI_SYSTEM:
                        std::clog << "DNS Error: Unrecoverable error for " << sock->getIp() << std::endl;
                        tries = -1;
                        break;
                    case EAI_NONAME:
                        std::clog << "DNS Error: Host not found for " << sock->getIp() << std::endl;
                        tries = -1;
                        break;
                    case EAI_AGAIN:
                    default:
                        std::clog << "DNS Error: Try again for " << sock->getIp() << std::endl;
                        tries++;
                        break;
                }
            } else {
                break;
            }
        }
        std::clog << "DNS: Resolver finished for " << sock->getIp() << "(" << (!res ? hbuf : sock->getIp()) << ")" << std::endl;
        if(res == 0) {
            // Found a hostname so print it
            write(fds[1], hbuf, strlen(hbuf));
        } else {
            // Didn't find a hostname, print the ip
            write(fds[1], sock->getIp().data(), sock->getIp().length());
        }
        exit(0);
    } else { // pid != 0
        // Parent Process: Close the writing end, we'll only be reading
        close(fds[1]);
        std::clog << "Watching Child DNS Resolver for(" << sock->getIp() << ") running with pid " << pid << " reading from fd " << fds[0] << std::endl;
        // Let the server know we're monitoring this child process
        addChild(pid, ChildType::DNS_RESOLVER, fds[0], sock->getIp());
    }
    return(0);
}

//********************************************************************
//                      addCache
//********************************************************************

void Server::addCache(std::string_view ip, std::string_view hostName, time_t t) {
    if(t == -1)
        t = time(nullptr);
    cachedDns.emplace_back(ip, hostName, t);
}

//********************************************************************
//                      addChild
//********************************************************************

void Server::addChild(int pid, ChildType pType, int pFd, std::string_view pExtra) {
    std::clog << "Adding pid " << pid << " as child type " << (int)pType << ", watching " << pFd << "\n";
    children.emplace_back(pid, pType, pFd, pExtra);
}

// End - Children Control
//--------------------------------------------------------------------

//********************************************************************
//                      saveDnsCache
//********************************************************************

void Server::saveDnsCache() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    xmlNodePtr      curNode;
    char            filename[80];

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "DnsCache", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    std::list<dnsCache>::iterator it;
    for( it = cachedDns.begin(); it != cachedDns.end() ; it++) {
        curNode = xmlNewChild(rootNode, nullptr, BAD_CAST"Dns", nullptr);
        xml::newStringChild(curNode, "Ip", (*it).ip);
        xml::newStringChild(curNode, "HostName", (*it).hostName);
        xml::newNumChild(curNode, "Time", (long)(*it).time);
    }

    sprintf(filename, "%s/dns.xml", Path::Config.c_str());
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
}

//********************************************************************
//                      loadDnsCache
//********************************************************************

void Server::loadDnsCache() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode, childNode;

    char filename[80];
    snprintf(filename, 80, "%s/dns.xml", Path::Config.c_str());
    xmlDoc = xml::loadFile(filename, "DnsCache");

    if(xmlDoc == nullptr)
        return;

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode)) {
        curNode = curNode->next;
    }
    if(curNode == nullptr) {
        xmlFreeDoc(xmlDoc);
        return;
    }
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Dns")) {
            std::string ip, hostname;
            long time=0;
            childNode = curNode->children;
            while(childNode != nullptr) {
                if(NODE_NAME(childNode, "Ip")) {
                    xml::copyToString(ip, childNode);
                } else if(NODE_NAME(childNode, "HostName")) {
                    xml::copyToString(hostname, childNode);
                } else if(NODE_NAME(childNode, "Time")) {
                    xml::copyToNum(time, childNode);
                    addCache(ip, hostname, time);
                }
                childNode = childNode->next;
            }
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
}


//--------------------------------------------------------------------
// Reboot Functions

//********************************************************************
//                      startReboot
//********************************************************************

bool Server::startReboot(bool resetShips) {
    gConfig->save();

    gServer->setRebooting();
    // First give all players a free restore
    for(const auto& sock : sockets) {
        std::shared_ptr<Player> player = sock->getPlayer();
        if(player && player->fd > -1) {
            player->hp.restore();
            player->mp.restore();
        }
    }

    // Then run through and save the reboot file
    saveRebootFile(resetShips);

    // Now disconnect people that won't make it through the reboot
    for(const auto& sock : sockets) {
        std::shared_ptr<Player> player = sock->getPlayer();
        if(player && player->fd > -1 ) {
            // End the compression, we'll try to restart it after the reboot
            if(sock->mccpEnabled()) {
                sock->endCompress();
            }
            player->save(true);
            player->uninit();
            player = nullptr;
            players[player->getName()] = nullptr;
            sock->clearPlayer();
        } else {
            sock->write("\n\r\n\r\n\rSorry, we are rebooting. You may reconnect in a few seconds.\n\r");
            sock->disconnect();
        }
    }

    processOutput();
    cleanUp();

    if(resetShips)
        Config::resetShipsFile();

    char port[10], path[80];

    sprintf(port, "%d", Port);
    strcpy(path, gConfig->cmdline);

    execl(path, path, "-r", port, (char *)nullptr);

    std::error_code ec;
    fs::remove(Path::Config / "config.xml", ec);

    throw std::runtime_error("dmReboot failed!!!");
    return(false);
}

//********************************************************************
//                      saveRebootFile
//********************************************************************

bool Server::saveRebootFile(bool resetShips) {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    xmlNodePtr      serverNode;
    xmlNodePtr      curNode;

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Reboot", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    // Save server information
    serverNode = xmlNewChild(rootNode, nullptr, BAD_CAST"Server", nullptr);

    // Dump control socket information
    std::list<controlSock>::const_iterator it1;
    for( it1 = controlSocks.begin(); it1 != controlSocks.end(); it1++ ) {
        curNode = xmlNewChild(serverNode, nullptr, BAD_CAST"ControlSock", nullptr);
        xml::newNumProp(curNode, "Port", (*it1).port);
        xml::newNumProp(curNode, "Control", (*it1).control);
    }

    xml::newNumChild(serverNode, "StartTime", StartTime);
    xml::newNumChild(serverNode, "LastWeatherUpdate", last_weather_update);
    xml::newNumChild(serverNode, "LastRandomUpdate", lastRandomUpdate);
    xml::newNumChild(serverNode, "LastTimeUpdate", last_time_update);
    xml::newNumChild(serverNode, "InBytes", InBytes);
    xml::newNumChild(serverNode, "OutBytes", OutBytes);
    xml::newNumChild(serverNode, "UnCompressedBytes", UnCompressedBytes);
    if(resetShips)
        xml::newStringChild(serverNode, "ResetShips", "true");
    for(const auto &sock : sockets) {
        std::shared_ptr<Player>player = sock->getPlayer();
        if(player && player->fd > -1) {
            curNode = xmlNewChild(rootNode, nullptr, BAD_CAST"Player", nullptr);
            xml::newStringChild(curNode, "Name", player->getCName());
            xml::newNumChild(curNode, "Fd", sock->getFd());
            xml::newStringChild(curNode, "Ip", std::string(sock->getIp()));
            xml::newStringChild(curNode, "HostName", std::string(sock->getHostname()));
            xml::newStringChild(curNode, "ProxyName", player->getProxyName());
            xml::newStringChild(curNode, "ProxyId", player->getProxyId());
            sock->saveTelopts(curNode);
        }
    }

    char filename[80];
    snprintf(filename, 80, "%s/reboot.xml", Path::Config.c_str());
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
    return(true);
}

//********************************************************************
//                      finishReboot
//********************************************************************

int Server::finishReboot() {
    xmlDocPtr doc;
    xmlNodePtr curNode;
    xmlNodePtr childNode;
    bool resetShips = false;
    std::clog << "Running finishReboot()" << std::endl;

    // We are rebooting
    rebooting = true;

    char filename[80];
    snprintf(filename, 80, "%s/reboot.xml", Path::Config.c_str());
    // build an XML tree from a the file
    doc = xml::loadFile(filename, "Reboot");
    unlink(filename);

    if(doc == nullptr) {
        std::clog << "Unable to loadBeforePython reboot file\n";
        throw std::runtime_error("Loading reboot file");
    }

    curNode = xmlDocGetRootElement(doc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode)) {
        curNode = curNode->next;
    }
    if(curNode == nullptr) {
        xmlFreeDoc(doc);
        throw std::runtime_error("Parsing reboot file");
    }

    Numplayers = 0;
    StartTime = time(nullptr);

    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Server")) {
            childNode = curNode->children;
            while(childNode != nullptr) {
                if(NODE_NAME(childNode, "ControlSock")) {
                    int port = xml::getIntProp(childNode, "Port");
                    int control = xml::getIntProp(childNode, "Control");
                    controlSocks.emplace_back(port, control);
                    running = true;
                }
                else if(NODE_NAME(childNode, "StartTime"))
                    xml::copyToNum(StartTime, childNode);
                else if(NODE_NAME(childNode, "LastWeatherUpdate"))
                    xml::copyToNum(last_weather_update, childNode);
                else if(NODE_NAME(childNode, "LastRandomUpdate"))
                    xml::copyToNum(lastRandomUpdate, childNode);
                else if(NODE_NAME(childNode, "LastTimeUpdate"))
                    xml::copyToNum(last_time_update, childNode);
                else if(NODE_NAME(childNode, "InBytes"))
                    xml::copyToNum(InBytes, childNode);
                else if(NODE_NAME(childNode, "OutBytes"))
                    xml::copyToNum(OutBytes, childNode);
                else if(NODE_NAME(childNode, "UnCompressedBytes"))
                    xml::copyToNum(UnCompressedBytes, childNode);
                else if(NODE_NAME(childNode, "ResetShips"))
                    resetShips = true;
                    //xml::copyToBool(resetShips, childNode);
                childNode = childNode->next;
            }
        } else if(NODE_NAME(curNode, "Player")) {
            childNode = curNode->children;
            std::shared_ptr<Player> player=nullptr;
            std::shared_ptr<Socket> sock=nullptr;
            while(childNode != nullptr) {
                if(NODE_NAME(childNode, "Name")) {
                    std::string name;
                    xml::copyToString(name, childNode);
                    if(!loadPlayer(name.c_str(), player)) {
                        throw std::runtime_error("finishReboot: loadPlayer");
                    }
                }
                else if(NODE_NAME(childNode, "Fd")) {
                    short fd;
                    xml::copyToNum(fd, childNode);
                    sock = sockets.emplace_back(std::make_shared<Socket>(fd));
                    if(!player || !sock)
                        throw std::runtime_error("finishReboot: No Sock/Player");

                    player->fd = fd;

                    sock->setPlayer(player);
                    player->setSock(sock.get());
                    addPlayer(player);
                }
                else if(NODE_NAME(childNode, "Ip")) {
                    std::string ip;
                    xml::copyToString(ip, childNode);
                    sock->setIp(ip);
                }
                else if(NODE_NAME(childNode, "HostName")) {
                    std::string host;
                    xml::copyToString(host, childNode);
                    sock->setHostname(host);
                } else if(NODE_NAME(childNode, "Telopts")) {
                    sock->loadTelopts(childNode);
                } else if(NODE_NAME(childNode, "ProxyName")) {
                    std::string proxyName = xml::getString(childNode);
                    player->setProxyName(proxyName);
                } else if(NODE_NAME(childNode, "ProxyId")) {
                    std::string proxyId = xml::getString(childNode);
                    player->setProxyId(proxyId);
                }

                childNode = childNode->next;
            }

            if(!player || !sock)
                throw std::runtime_error("finishReboot: Finished, still no Sock/Player");

            sock->ltime = time(nullptr);
            sock->print("The world comes back into focus!\n");
            player->init();

            if(player->isDm()) {
                sock->getPlayer()->print("Now running on version %s.\n", VERSION);
                if(resetShips)
                    sock->getPlayer()->print("Time has been moved back %d hour%s.\n",
                        gConfig->currentHour(), gConfig->currentHour() != 1 ? "s" : "");
            }
            sock->intrpt = 1;
            sock->setState(CON_PLAYING);
            Numplayers++;
        }

        curNode = curNode->next;
    }
    xmlFreeDoc(doc);
    xmlCleanupParser();

    if(resetShips)
        gConfig->calendar->resetToMidnight();
    else
        gConfig->calendar->shipUpdates = gConfig->calendar->shipUpdates % 60;
    gConfig->resetMinutes();

    // Done rebooting
    rebooting = false;
    return(true);
}

// End - Reboot Functions
//--------------------------------------------------------------------



// -------------------------------------------------------------------
// Player Functions

//********************************************************************
//                      findPlayer
//********************************************************************

std::shared_ptr<Player> Server::findPlayer(const std::string &name) {
    auto it = players.find(name);

    if(it != players.end())
        return((*it).second);
    return(nullptr);

}

//*********************************************************************
//                      saveAllPly
//*********************************************************************
// This function saves all players currently in memory.

void Server::saveAllPly() {
    for(std::pair<std::string, std::shared_ptr<Player>> p : players) {
        if(!p.second->isConnected())
            continue;
        p.second->save(true);
    }
}


//*********************************************************************
//                      clearPlayer
//*********************************************************************
// This will NOT free up the player, it will just remove them from the list

bool Server::clearPlayer(const std::string &name) {
    auto pIt = players.find(name);
    if (pIt != players.end())
        return clearPlayer(pIt->second);
    return false;
}

bool Server::clearPlayer(const std::shared_ptr<Player>& player) {
    player->unRegisterMo();
    players.erase(player->getName());
    return(true);
}

//*********************************************************************
//                      addPlayer
//*********************************************************************

bool Server::addPlayer(const std::shared_ptr<Player>& player) {
    player->validateId();
    players[player->getName()] = player;
    player->registerMo(player);
    return(true);
}

//*********************************************************************
//                      checkDuplicateName
//*********************************************************************

bool Server::checkDuplicateName(Socket *sock, bool dis) {
    for(const auto &s : sockets) {
        if(sock != s.get() && s->hasPlayer() && s->getPlayer()->getName() ==  sock->getPlayer()->getName()) {
            if(!dis) {
                sock->printColor("\n\n^ySorry, that character is already logged in.^x\n\n\n");
                sock->reconnect();
            } else {
                s->disconnect();
            }
            return(true);
        }
    }
    return(false);
}

//*********************************************************************
//                      checkDouble
//*********************************************************************
// returning true will disconnect the connecting socket (sock)

bool Server::checkDouble(Socket *sock) {
    if(!gConfig->getCheckDouble())
        return(false);

    if(sock->getPlayer() && sock->getPlayer()->isStaff())
        return(false);

//    if(sock.getHostname().find("localhost") != std::string_view::npos)
//        return(false);

    std::shared_ptr<Player> player=nullptr;
    int cnt = 0;
    for(const auto &s : sockets) {
        player = s->getPlayer();
        if(!player || s.get() == sock)
            continue;

        if(player->isCt())
            continue;
        if(player->flagIsSet(P_LINKDEAD))
            continue;
        if(sock->getIp() != s->getIp())
            continue;

        cnt++;

        if(cnt >= gConfig->getMaxDouble()) {
            sock->write("\nMaximum number of connections has been exceeded!\n\n");
            sock->disconnect();
            return true;
        }
    }
    return(false);
}

//*********************************************************************
//                      sendCrash
//*********************************************************************

void Server::sendCrash() {
    auto filename = Path::Config / "crash.txt";

    for(const auto &sock : sockets) {
        sock->viewFile(filename);
    }
}


//*********************************************************************
//                      getTimeZone
//*********************************************************************
// not accurate for the fractional hour timezones

std::string Server::getTimeZone() {
    // current local time
    time_t curr = time(nullptr);
    // convert curr to GMT, store as tm
    tm local = *gmtime(&curr);
    // convert GMT tm to GMT time_t
    time_t utc = mktime(&local);
    // difference in hours
    int tz = (int)(difftime(utc,curr) / -3600);

    switch(tz) {
    case -12:
        return("International Date Line West");
    case -11:
        return("Midway Island, Samoa");
    case -10:
        return("Hawaii");
    case -9:
        return("Alaska");
    case -8:
        return("Pacific");
    case -7:
        return("Mountain");
    case -6:
        return("Central");
    case -5:
        return("Eastern");
    case -4:
        return("Atlantic");
    case -3:
        return("Brasilia, Buenos Aires, Georgetown, Greenland");
    case -2:
        return("Mid-Atlantic");
    case -1:
        return("Azores, Cape verde Is.");
    case 0:
        return("Greenwich Mean Time");
    case 1:
        return("Berlin, Rome, Prague, Warsaw, , West Central Africa");
    case 2:
        return("Athens, Minsk, Cairo, Jerusalem");
    case 3:
        return("Baghdad, Moscow, Nairobi");
    case 4:
        return("Abu Dhabi, Tbilsi");
    case 5:
        return("Islamabad, Karachi, Tashkent");
    case 6:
        return("Almaty, Dhaka, Sri Jayawardenepura");
    case 7:
        return("Bangkok, Jakarta, Krasnoyarsk");
    case 8:
        return("Beijing, Hong Kong, Singapore, Taipei");
    case 9:
        return("Osaka, Tokyo, Seoul");
    case 10:
        return("Melbourne, Sydney, Guam, Vladivostok");
    case 11:
        return("Magadan, Solomon Is., New Caledonia");
    case 12:
        return("Auckland, , Fiji, Marshall Is.");
    case 13:
        return("Nuku'alofa");
    default:
        return("Unknown");
    }
}

std::string Server::getServerTime() {
    time_t t = time(nullptr);
    char* str = ctime(&t);
    str[strlen(str) - 1] = 0;
    std::ostringstream oStr;
    oStr << str << "(" << getTimeZone() << ")";

    return (oStr.str());
}

//*********************************************************************
//                      getNumPlayers
//*********************************************************************

int Server::getNumPlayers() {
    int numPlayers=0;
    std::shared_ptr<Player> target=nullptr;
    for(const auto& p : players) {
        target = p.second;

        if(!target->isConnected())
                continue;

        if(!target->isStaff())
            numPlayers++;
    }
    return(numPlayers);
}

// *************************************
// Functions that deal with unique IDs
// *************************************

bool Server::registerMudObject(const std::shared_ptr<MudObject>& toRegister, bool reassignId) {
    assert(toRegister != nullptr);

    if(toRegister->getId() =="-1")
        return(false);

    auto it =registeredIds.find(toRegister->getId());
    if(it != registeredIds.end()) {
        std::ostringstream oStr;
        oStr << "ERROR: ID: " << toRegister->getId() << " is already registered!";
        if(toRegister->isMonster() || toRegister->isObject()) {
            // Give them a new id and continue
            oStr << " Assigning them a new ID";
            reassignId = true;
            toRegister->setRegistered();
            toRegister->setId("-1");
            toRegister->validateId();
        }
        //broadcast(isDm, "%s", oStr.str().c_str());
        std::clog << oStr.str() << std::endl;
        if(!reassignId)
            return(false);
        else
            return(registerMudObject(toRegister, true));
    }
    if(!reassignId)
        toRegister->setRegistered();

    registeredIds.insert(IdMap::value_type(toRegister->getId(), toRegister));
    //std::clog << "Registered: " << toRegister->getId() << " - " << toRegister->getName() << std::endl;
    return(true);
}

bool Server::unRegisterMudObject(MudObject* toUnRegister) {
    assert(toUnRegister != nullptr);

    if(toUnRegister->getId() == "-1")
        return(false);

    auto it = registeredIds.find(toUnRegister->getId());
    bool registered = toUnRegister->isRegistered();
    if(!registered) {
        std::ostringstream oStr;
        oStr << "ERROR: ID: " << toUnRegister->getId() << " thinks it is not registered, but is being told to unregister, trying anyway.";
        broadcast(isDm, "%s", oStr.str().c_str());
        std::clog << oStr.str() << std::endl;

    }
    if(it == registeredIds.end()) {
        if(registered) {
            std::ostringstream oStr;
            oStr << "ERROR: ID: " << toUnRegister->getId() << " is not registered!";
            broadcast(isDm, "%s", oStr.str().c_str());
            std::clog << oStr.str() << std::endl;
        }
        return(false);
    }
    if(!registered) {
        std::ostringstream oStr;

        if(!it->second.expired() && it->second.lock().get() == toUnRegister) {
            oStr << "ERROR: ID: " << toUnRegister->getId() << " thought it wasn't registered, but the server thought it was.";
            broadcast(isDm, "%s", oStr.str().c_str());
            std::clog << oStr.str() << std::endl;
            // Continue on with the unregistering since this object really was registered
        } else {
            oStr << "ERROR: ID: " << toUnRegister->getId() << " Server does not have this instance registered.";
            broadcast(isDm, "%s", oStr.str().c_str());
            std::clog << oStr.str() << std::endl;
            // Stop here, don't unregister this ID since the mudObject registered it not the one we're after
            return(false);
        }
    }
    toUnRegister->setUnRegistered();
    registeredIds.erase(it);
    //std::clog << "Unregistered: " << toUnRegister->getId() << " - " << toUnRegister->getName() << std::endl;
    return(true);
}

std::shared_ptr<Object>  Server::lookupObjId(const std::string &toLookup) {
    if(toLookup[0] != 'O')
        return(nullptr);

    auto it = registeredIds.find(toLookup);

    if(it == registeredIds.end())
        return(nullptr);
    else {
        auto res = it->second.lock();
        return res ? res->getAsObject() : nullptr;
    }
}

std::shared_ptr<Creature> Server::lookupCrtId(const std::string &toLookup) {
    if(toLookup[0] != 'M' && toLookup[0] != 'P')
        return(nullptr);

    auto it = registeredIds.find(toLookup);

    if(it == registeredIds.end())
        return(nullptr);
    else {
        auto res = it->second.lock();
        return res ? res->getAsCreature() : nullptr;
    }
}
std::shared_ptr<Player> Server::lookupPlyId(const std::string &toLookup) {
    if(toLookup[0] != 'P')
        return(nullptr);
    auto it = registeredIds.find(toLookup);

    if(it == registeredIds.end())
        return(nullptr);
    else{
        auto res = it->second.lock();
        return res ? res->getAsPlayer() : nullptr;
    }
}
std::string Server::getRegisteredList() {
    std::ostringstream oStr;
    for(const auto& [id, mo] : registeredIds) {
        if(auto locked = mo.lock())
            oStr << id << " - " << locked->getName() << std::endl;
    }
    return(oStr.str());
}

long Server::getMaxMonsterId() {
    return(maxMonsterId);
}

long Server::getMaxPlayerId() {
    return(maxPlayerId);
}

long Server::getMaxObjectId() {
    return(maxObjectId);
}


std::string Server::getNextMonsterId() {
    long id = ++maxMonsterId;
    std::string toReturn = std::string("M") + std::to_string(id);
    idDirty = true;
    return(toReturn);
}

std::string Server::getNextObjectId() {
    long id = ++maxObjectId;
    std::string toReturn = std::string("O") + std::to_string(id);
    idDirty = true;
    return(toReturn);
}

std::string Server::getNextPlayerId() {
    long id = ++maxPlayerId;
    std::string toReturn = std::string("P") + std::to_string(id);
    idDirty = true;
    return(toReturn);
}

void Server::loadIds() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;

    char filename[80];
    snprintf(filename, 80, "%s/ids.xml", Path::Game.c_str());
    xmlDoc = xml::loadFile(filename, "Ids");

    if(xmlDoc == nullptr)
        return;

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode)) {
        curNode = curNode->next;
    }
    if(curNode == nullptr) {
        xmlFreeDoc(xmlDoc);
        return;
    }
    while(curNode != nullptr) {
             if(NODE_NAME(curNode, "MaxMonsterId")) xml::copyToNum(maxMonsterId, curNode);
        else if(NODE_NAME(curNode, "MaxPlayerId")) xml::copyToNum(maxPlayerId, curNode);
        else if(NODE_NAME(curNode, "MaxObjectId")) xml::copyToNum(maxObjectId, curNode);

        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    idDirty = false;
}
void Server::saveIds() {
    xmlDocPtr       xmlDoc;
    xmlNodePtr      rootNode;
    char            filename[80];

    if(!idDirty)
        return;

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Ids", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    xml::newNumChild(rootNode, "MaxMonsterId", maxMonsterId);
    xml::newNumChild(rootNode, "MaxPlayerId",  maxPlayerId);
    xml::newNumChild(rootNode, "MaxObjectId",  maxObjectId);

    sprintf(filename, "%s/ids.xml", Path::Game.c_str());
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);

    idDirty = false;
}

void Server::logGold(GoldLog dir, const std::shared_ptr<Player>& player, Money amt, std::shared_ptr<MudObject> target, std::string_view logType) {
    std::string pName = player->getName();
    std::string pId = player->getId();
    // long amt
    std::string targetStr;
    std::string source;

    if(target) {
        targetStr = stripColor(target->getName());
        std::shared_ptr<Object>  oTarget = target->getAsObject();
        if(oTarget) {
            targetStr += "(" + oTarget->info.displayStr() + ")";
            if(dir == GOLD_IN) {
                source = oTarget->droppedBy.str();
            }
        }
    }
    std::string room;
    if(auto parentRoom = player->getRoomParent()) {
        if(auto uniqueRoom = parentRoom->getAsUniqueRoom()) {
            room = std::string(parentRoom->getName()) + "(" + uniqueRoom->info.displayStr() + ")";
        } else if (auto areaRoom = parentRoom->getAsAreaRoom()) {
            auto area = areaRoom->area.lock();
            room = (area ? area->name : "<invalid>") + "(" + areaRoom->mapmarker.str() + ")";
        }
    }
    // logType
    std::string direction = (dir == GOLD_IN ? "In" : "Out");
    std::clog << direction << ": P:" << pName << " I:" << pId << " T: " << targetStr << " S:" << source << " R: " << room << " Type:" << logType << " G:" << amt.get(GOLD) << std::endl;

#ifdef SQL_LOGGER
    logGoldSql(pName, pId, targetStr, source, room, logType, amt.get(GOLD), direction);
#endif // SQL_LOGGER
}

//*********************************************************************
//                      reloadRoom
//*********************************************************************
// This function reloads a room from disk, if it's already loaded. This
// allows you to make changes to a room, and then reload it, even if it's
// already in the memory room queue.

bool Server::reloadRoom(const std::shared_ptr<BaseRoom>& room) {
    auto uRoom = room->getAsUniqueRoom();

    if(uRoom) {
    	CatRef cr = uRoom->info;
        if(reloadRoom(cr)) {
            roomCache.fetch(cr, false)->addPermCrt();
            return(true);
        }
    } else  {

        auto aRoom = room->getAsAreaRoom();
        auto area = aRoom->area.lock();
        char    filename[256];
        sprintf(filename, "%s/%d/%s", Path::AreaRoom.c_str(), area->id, aRoom->mapmarker.filename().c_str());

        if(fs::exists(filename)) {
            xmlDocPtr   xmlDoc;
            xmlNodePtr  rootNode;

            if((xmlDoc = xml::loadFile(filename, "AreaRoom")) == nullptr)
                throw std::runtime_error("Unable to read arearoom file");

            rootNode = xmlDocGetRootElement(xmlDoc);

            aRoom->reset();
            aRoom->area = area;
            aRoom->load(rootNode);
            return(true);
        }
    }
    return(false);
}

std::shared_ptr<UniqueRoom> Server::reloadRoom(const CatRef& cr) {
    std::shared_ptr<UniqueRoom> room=nullptr, oldRoom=nullptr;

    std::string str = cr.displayStr();
    oldRoom = roomCache.fetch(cr);
    if(!oldRoom)
    	return nullptr;

    if(!loadRoomFromFile(cr, room))
        return(nullptr);

    // Move any current players & monsters into the new room
    for(const auto& ply: oldRoom->players) {
        room->players.insert(ply);
        ply->setParent(room);
    }
    oldRoom->players.clear();

    if(room->monsters.empty()) {
        for(const auto& mons : oldRoom->monsters) {
            room->monsters.insert(mons);
            mons->setParent(room);
        }
        oldRoom->monsters.clear();
    }
    if(room->objects.empty()) {
        for(const auto& obj : oldRoom->objects) {
            room->objects.insert(obj);
            obj->setParent(room);
        }
        oldRoom->objects.clear();
    }

    roomCache.insert(cr, room);

    room->registerMo(room);

    return(room);
}

//*********************************************************************
//                      resaveRoom
//*********************************************************************
// This function saves an already-loaded room back to memory without
// altering its position on the queue.

int Server::resaveRoom(const CatRef& cr) {
    std::shared_ptr<UniqueRoom> room = roomCache.fetch(cr);
    if(room)
        room->saveToFile(ALLITEMS);
    return(0);
}


int Server::saveStorage(const std::shared_ptr<UniqueRoom>& uRoom) {
    if(uRoom->flagIsSet(R_SHOP))
        saveStorage(shopStorageRoom(uRoom));
    return(saveStorage(uRoom->info));
}
int Server::saveStorage(const CatRef& cr) {

    std::shared_ptr<UniqueRoom> room = roomCache.fetch(cr);
    if(room) {
        if(room->saveToFile(ALLITEMS) < 0) {
            return(-1);
        }
    }

    return(0);
}

void Server::stop() {
    if(httpServer) httpServer->stop();

}
