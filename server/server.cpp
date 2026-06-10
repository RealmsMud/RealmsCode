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
#include <libxml/parser.h>                          // for xmlFreeDoc, xmlDo...
#include <netdb.h>                                  // for getnameinfo, EAI_...
#include <netinet/in.h>                             // for sockaddr_in, htons
#include <poll.h>                                   // for pollfd, poll, POL...
#include <csignal>                                  // for sigaction, signal
#include <sys/resource.h>                           // for rlimit, setrlimit
#include <sys/socket.h>                             // for AF_INET, accept
#include <sys/stat.h>                               // for umask
#include <sys/time.h>                               // for timeval
#include <sys/wait.h>                               // for wait3, waitpid
#include <unistd.h>                                 // for close, unlink, read
#include <algorithm>                                // for find
#include <array>                                    // for getTimeZone lookup table
#include <ranges>                                   // for views::filter/transform, ranges::any_of
#include <chrono>                                   // for the index-build time budget
#include <asio.hpp>                                 // io_context, acceptor, steady_timer, async_accept
#include <fmt/chrono.h>                             // for fmt::localtime
#include <fmt/format.h>                             // for fmt::format
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
#include "account.hpp"                              // for Account
#include "delayedAction.hpp"                        // for DelayedAction
#include "factions.hpp"                             // for Faction
#include "flags.hpp"                                // for M_PERMANENT_MONSTER
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

// Forward declaration
void showAccountMenu(const std::shared_ptr<Socket>& sock, std::shared_ptr<Account> account);

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


//--------------------------------------------------------------------
// Constructors, Destructors, etc

//********************************************************************
//                      Server
//********************************************************************

Server::Server(): roomCache(RQMAX, true), monsterCache(MQMAX, false), objectCache(OQMAX, false) {
	std::clog << "Constructing the Server." << std::endl;
    GDB = valgrind = false;

    running = false;
    pulse = 0;
    webInterface = nullptr;
    lastDnsPrune = lastUserUpdate = lastRoomPulseUpdate = lastRandomUpdate = lastActiveUpdate = lastAccountSave = 0;
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
    // Tear down each socket while `sockets` still owns it, so the upcoming clear() (and each
    // ~Socket) is a guarded no-op. Running teardown from a destructor mid-list-mutation let a
    // broadcast inside cleanUp re-enter the deleter. Players/areas are still alive (cleared
    // below), so uninit is valid.
    for(const auto& sock : sockets)
        sock->cleanUp();
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
    rlimit lim{};
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

    if(!gConfig->isListing()) {
        initWebInterface();

        std::clog << "Initializing Spell List...";
        initSpellList();
        std::clog << "done." << std::endl;
    }
    // Python
    std::clog << "Initializing Python...";
    if (!PythonHandler::initPython()) {
        std::clog << "failed!" << std::endl;
        exit(-1);
    }

    std::clog << "Loading Areas..." << (loadAreas() ? "done" : "*** FAILED ***") << std::endl;
    gConfig->loadAfterPython();

    if(!gConfig->isListing()) {
        initHttpServer();
        initDiscordBot();
    }



#ifdef SQL_LOGGER
    std::clog <<  "Initializing SQL Logger...";
    if(initSql())
        std::clog << "done." << std::endl;
    else
        std::clog << "failed." << std::endl;
#endif // SQL_LOGGER

    umask(000);
    srand(getpid() + time(nullptr));
    if(!gConfig->isListing())
        addListenPort(Port);
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
void Server::setValgrind() { valgrind = true; }
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
    vSockets = std::make_unique<SocketVector>();
    for(auto &sock : sockets) vSockets->push_back(sock);

    Random::shuffle(vSockets->begin(), vSockets->end());
}


//********************************************************************
//                      run
//********************************************************************

void Server::run() {
    if(!running) {
        std::cerr << "Not bound to any ports, exiting." << std::endl;
        exit(-1);
    }

    if(httpServer)
        httpServer->run();

    std::clog << "Starting Sock Loop\n";
    tick();           // run the first iteration; it re-arms tickTimer for the next
    ioContext.run();  // drive accept/read/write/timer handlers until stop()
}

//********************************************************************
//                      tick
//********************************************************************
// One game-loop iteration, fired by tickTimer on the (single) io_context thread.
// Async accept/read/write completion handlers run in the gaps between ticks.

void Server::tick() {
    if(!running)
        return;

    ServerTimer timer{};
    timer.start();

    if(!children.empty()) reapChildren();
    processChildren();

    populateVSockets();

    processCommands();
    updatePlayerCombat();
    updateGame();
    processReporting();
    processOutput();      // flush each socket's accumulated output into its async send queue

    cleanUp();
    pulse++;
    checkWebInterface();

    if(httpServer)
        httpServer->processApiQueue();

    if(pulse % 100 == 0)
        zoneIndex.flushDirty();

    vSockets.reset();

    timer.end(); // measure this tick's work
    if(zoneIndexBuilder.hasWork()) {
        long spareUs = 100000 - timer.passedMicros();
        long budgetUs = std::min<long>(std::max<long>(spareUs, 0) / 2, 30000);
        zoneIndexBuilder.pump(std::chrono::microseconds(budgetUs), 10);
        timer.end();
    }

    // Re-arm 100ms after this tick's work (fixed cadence; immediate if we overran).
    // Skip re-arming once stop() cleared `running`, so ioContext.run() drains and returns.
    if(running) {
        long remainingUs = 100000 - timer.passedMicros();
        if(remainingUs < 0) remainingUs = 0;
        tickTimer.expires_after(std::chrono::microseconds(remainingUs));
        tickTimer.async_wait([this](const asio::error_code& ec) {
            if(!ec && running)
                tick();
        });
    }
}

//********************************************************************
//                      addListenPort
//********************************************************************

int Server::addListenPort(int port) {
    asio::error_code ec;
    asio::ip::tcp::endpoint ep(asio::ip::tcp::v4(), static_cast<unsigned short>(port));
    asio::ip::tcp::acceptor acc(ioContext);

    acc.open(ep.protocol(), ec);
    if(ec) { std::clog << "Error opening acceptor: " << ec.message() << "\n"; return(-1); }
    acc.set_option(asio::ip::tcp::acceptor::reuse_address(true), ec);
    acc.bind(ep, ec);
    if(ec) { std::clog << "Unable to bind to port " << port << ": " << ec.message() << std::endl; return(-1); }
    acc.listen(asio::socket_base::max_listen_connections, ec);
    if(ec) { std::clog << "Error with listen: " << ec.message() << "\n"; return(-1); }

    std::clog << "Mud is now listening on port " << port << std::endl;
    acceptors.emplace_back(std::move(acc));
    doAccept(acceptors.back());
    running = true;

    return(0);
}

//********************************************************************
//                      doAccept
//********************************************************************
// Async accept loop: build a Socket from each accepted connection, then re-arm.

void Server::doAccept(asio::ip::tcp::acceptor& acc) {
    acc.async_accept([this, &acc](const asio::error_code& ec, asio::ip::tcp::socket peer) {
        if(!ec) {
            // Game's full, drop the connection
            if(getNumSockets() > static_cast<size_t>(Tablesize - 10)) {
                asio::error_code cec;
                peer.close(cec);
            } else {
                std::clog << "Got a new connection on port " << acc.local_endpoint().port() << std::endl;
                auto sock = std::make_shared<Socket>(std::move(peer));
                // Must run after the shared_ptr exists (these use shared_from_this()).
                sock->startTelnetNeg();
                sock->showLoginScreen();
                sockets.emplace_back(sock);
                sock->startRead();
                if(sock->dnsDone) sock->checkLockOut();
            }
        }
        if(acc.is_open())
            doAccept(acc);
    });
}

//********************************************************************
//                      processCommands
//********************************************************************

int Server::processCommands() {
    for(auto&& sock : liveSockets()) {
        if(sock->hasCommand() && sock->getState() != CON_DISCONNECTING && sock->processOneCommand() == -1) {
            sock->setState(CON_DISCONNECTING);
            continue;
        }
        // Re-arm reads paused by input backpressure once this tick drained the queue.
        sock->resumeRead();
    }
    return(0);
}

//********************************************************************
//                      updatePlayerCombat
//********************************************************************

int Server::updatePlayerCombat() {
    for(auto&& sock : liveSockets()) {
        if(auto player = sock->getPlayer()) {
            if (player->isFleeing() && player->canFlee(false)) {
                player->doFlee();
            } else if (player->autoAttackEnabled() && !player->isFleeing() && player->hasAttackableTarget() && player->isAttackingTarget()) {
                if (!player->checkAttackTimer(false))
                    continue;

                player->attackCreature(player->getTarget(), ATTACK_NORMAL);
            }
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
    for(auto&& sock : liveSockets()) {
        if (sock->getFd() != -1 && sock->hasOutput()) {
            sock->flush();
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

static bool isDisconnecting(const std::shared_ptr<Socket>& sock) {
    return sock->getState() == CON_DISCONNECTING;
}

int Server::cleanUp() {
    for(const auto& sock : sockets)
        if(isDisconnecting(sock))
            sock->cleanUp();
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

size_t Server::expireDns(long now) {
    constexpr long fifteenDays = 60*60*24*15;
    return std::erase_if(cachedDns, [now](const dnsCache& dns) {
        return now - dns.time >= fifteenDays;
    });
}

void Server::pruneDns() {
    long currentTime = time(nullptr);
    std::clog << "Pruning DNS\n";
    expireDns(currentTime);
    saveDnsCache();
    lastDnsPrune = currentTime;
}

//********************************************************************
//                      pulseTicks
//********************************************************************

void Server::pulseTicks(long t) {

    for(auto&& sock : liveSockets()) {
        if(auto player = sock->getPlayer()) {
            player->pulseTick(t);
            if (player->isPlaying())
                player->pulseSong(t);
        }
    }
}



//*********************************************************************
//                      updateUsers
//*********************************************************************

void Server::updateUsers(long t) {
    lastUserUpdate = t;

    for(auto&& sock : liveSockets()) {
        auto player = sock->getPlayer();

        int tout;
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

//********************************************************************
//                      update_random
//********************************************************************
// This function checks all player-occupied rooms to see if random monsters
// have entered them.  If it is determined that random monster should enter
// a room, it is loaded and items it is carrying will be loaded with it.

void Server::updateRandom(long t) {
    std::set<std::string> check;

    lastRandomUpdate = t;

    for(auto&& sock : liveSockets()) {
        auto player = sock->getPlayer();

        if (!player || !player->getRoomParent())
            continue;
        auto uRoom = player->getUniqueRoomParent();
        auto aRoom = player->getAreaRoomParent();
        auto room = player->getRoomParent();

        WanderInfo* wander;
        if (uRoom) {
            // handle monsters arriving in unique rooms
            if (!uRoom->info.id)
                continue;

            if (check.contains(uRoom->info.displayStr()))
                continue;

            check.insert(uRoom->info.displayStr());
            wander = &uRoom->wander;
        } else {
            // handle monsters arriving in area rooms
            if (check.contains(aRoom->mapmarker.str()))
                continue;
            if (aRoom->unique.id)
                continue;

            check.insert(aRoom->mapmarker.str());
            wander = aRoom->getRandomWanderInfo();
        }
        if (!wander)
            continue;

        if (Random::get(1, 100) > wander->getTraffic())
            continue;

        CatRef cr = wander->getRandom();
        if (!cr.id)
            continue;
        if (room->countCrt() >= room->getMaxMobs())
            continue;

        // Will make mobs not spawn if a DM is invis in the room. -TC
        if (!room->countVisPly())
            continue;

        std::shared_ptr<Monster> monster;
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

        int num;
        if (room->flagIsSet(R_PLAYER_DEPENDENT_WANDER))
            num = Random::get(1, room->countVisPly());
        else if (monster->getNumWander() > 1)
            num = Random::get<unsigned short>(1, monster->getNumWander());
        else
            num = 1;

        for (int l = 0; l < num; l++) {
            monster->initMonster();

            if (monster->flagIsSet(M_PERMANENT_MONSTER))
                monster->clearFlag(M_PERMANENT_MONSTER);

            if (!l)
                monster->addToRoom(room, num);
            else
                monster->addToRoom(room, 0);

            if (!monster->flagIsSet(M_PERMANENT_MONSTER) || monster->flagIsSet(M_NO_ADJUST))
                monster->adjust(-1);

            gServer->addActive(monster);
            if (l != num - 1)
                loadMonster(cr, monster);
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
    long    tt = gConfig->currentHour();

    lastActiveUpdate = t;

    if(activeList.empty())
        return;

    // reset mob smack-talk broadcasts once when the clock reaches 7am
    bool doSmackReset = false;
    if(tt == 7) {
        if(!smackTalkReset) { doSmackReset = true; smackTalkReset = true; }
    } else
        smackTalkReset = false;

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


        auto room = monster->getRoomParent();

        if(doSmackReset)
            monster->daily[DL_BROAD].cur = 20;

        bool timetowander = false;
        if( (monster->flagIsSet(M_NIGHT_ONLY) && isDay()) || (monster->flagIsSet(M_DAY_ONLY) && !isDay())) {
            bool immort = std::ranges::any_of(room->players, [](const auto& pIt){
                auto ply = pIt.lock();
                return ply && ply->isStaff();
            });
            if(!immort) {
                timetowander=true;
                broadcast(std::shared_ptr<Socket>(), monster->getRoomParent(), "%M wanders slowly away.", monster.get());
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
            !monster->flagIsSet(M_PERMANENT_MONSTER) &&
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
                if( mons->flagIsSet(M_PERMANENT_MONSTER) && !monster->willAssist(mons->getAsMonster()) && !monster->isEnemy(mons))
                    monster->addEnemy(mons);
            }
        }

        if(monster->flagIsSet(M_KILL_NON_ASSIST_MOBS)) {
            for(const auto& mons : room->monsters) {
                if(mons == monster)
                    continue;
                if( !monster->willAssist(mons->getAsMonster()) &&
                    !mons->flagIsSet(M_PERMANENT_MONSTER) &&
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
                broadcast(std::shared_ptr<Socket>(), monster->getRoomParent(), "%M casts a curepoison spell on %sself.", monster.get(), monster->himHer());
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
                broadcast(std::shared_ptr<Socket>(), room, "%1M wanders away.", monster.get());
            else
                broadcast(std::shared_ptr<Socket>(), room, "%1M fades away.", monster.get());

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


        auto target = monster->whoToAggro();
        if(target) {

            bool shouldoPrint = (!monster->flagIsSet(M_HIDDEN) && !(monster->isInvisible() && target->isEffected("detect-invisible")));

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

WeakMonsterList::iterator Server::findActive(Monster* monster) {
    return std::ranges::find_if(activeList, [&monster](const std::weak_ptr<Monster>& crt) {
        auto locked = crt.lock();
        return locked && locked.get() == monster;
    });
}

void Server::delActive(Monster* monster) {
    const auto it = findActive(monster);
    if(it != activeList.end())
        activeList.erase(it);
}


//*********************************************************************
//                      isActive
//*********************************************************************
// This function returns 1 if the parameter passed is in the
// active list.

bool Server::isActive(Monster* monster) {
    return findActive(monster) != activeList.end();
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
    std::cout << "Reaping Children (maybe)\n";

    while(!children.empty()) {
        std::vector<pollfd> fds(children.size());
        int i = 0;
        for(const childProcess& c : children) {
            fds[i].fd = c.fd;
            fds[i++].events = POLLHUP;
        }
        int ret = ::poll(fds.data(), i, 0);
        if (ret <= 0) break;

        std::list<childProcess>::const_iterator it, oldIt;
        for( it = children.begin(), i=0; it != children.end() ; i++) {
            const childProcess* cp = &*it;
            oldIt = it++;

            if(fds[i].revents == 0) {
                continue;
            } else if (fds[i].revents != POLLHUP) {
                std::cout << "Unexpected revent " << fds[i].revents << std::endl;
                continue;
            }

            std::cout << "waitpid " << cp->pid << std::endl;
            waitpid(cp->pid, &status, WNOHANG);

            childProcess myChild;
            bool found = false;
            if(cp->type == ChildType::DNS_RESOLVER) {
                // Read in the results from the resolver
                char readBuf[1024];
                ssize_t n = read(cp->fd, readBuf, sizeof(readBuf));

                // Close the read fd in the pipe now, won't need it anymore
                close(cp->fd);

                std::string hostName;
                // If we have an error reading, just use the ip address then
                if(n <= 0) {
                    if(errno == EWOULDBLOCK)
                        std::clog << "DNS ReapChildren: Error would block\n";
                    else
                        std::clog << "DNS ReapChildren: Error\n";
                    hostName = cp->extra;
                } else {
                    hostName.assign(readBuf, n);
                }

                // Add dns to cache
                addCache(cp->extra, hostName);
                dnsChild = true;

                // Now we want to look through all connected sockets and update dns where appropriate
                for(const auto &sock : sockets) {
                    if(sock->getState() == LOGIN_DNS_LOOKUP && sock->getIp() == cp->extra) {
                        // Be sure to set the hostname first, then check for lockout
                        sock->dnsDone = true;
                        sock->setHostname(hostName);
                        sock->checkLockOut();
                    }
                }
                std::clog << "Reaped DNS child (" << cp->pid << "-" << hostName << ")\n";
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
                std::clog << "ReapChildren: Unknown child type " << static_cast<int>(cp->type) << std::endl;
            }
            children.erase(oldIt);

            // finish swap after they've been deleted from the list
            if(found) {
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
    // just in case, kill off any zombies
    wait3(&status, WNOHANG, nullptr);
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

    char readBuf[4096];
    for(;;) {
        // Even if no socket is found, read in all the data
        ssize_t n = read(lister.fd, readBuf, sizeof(readBuf));
        if(n <= 0)
            break;

        if(found) {
            std::string toWrite(readBuf, n);
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
            std::clog << "processChildren: Unknown child type " << static_cast<int>(child.type) << std::endl;
        }
    }
    return(1);
}


//********************************************************************
//                      startDnsLookup
//********************************************************************

int Server::startDnsLookup(Socket *sock, sockaddr_in addr) {
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
            res = getnameinfo(reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NAMEREQD);

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
    std::clog << "Adding pid " << pid << " as child type " << static_cast<int>(pType) << ", watching " << pFd << "\n";
    children.emplace_back(pid, pType, pFd, pExtra);
}

// End - Children Control
//--------------------------------------------------------------------

//********************************************************************
//                      saveDnsCache
//********************************************************************

void Server::saveDnsCache() {
    xml::DocPtr xmlDoc(xmlNewDoc(BAD_CAST "1.0"));
    xmlNodePtr rootNode = xmlNewDocNode(xmlDoc.get(), nullptr, BAD_CAST "DnsCache", nullptr);
    xmlDocSetRootElement(xmlDoc.get(), rootNode);

    for(const dnsCache& dns : cachedDns) {
        xmlNodePtr curNode = xmlNewChild(rootNode, nullptr, BAD_CAST "Dns", nullptr);
        xml::newStringChild(curNode, "Ip", dns.ip);
        xml::newStringChild(curNode, "HostName", dns.hostName);
        xml::newNumChild(curNode, "Time", static_cast<long>(dns.time));
    }

    xml::saveFile(Path::Config / "dns.xml", xmlDoc.get());
}

//********************************************************************
//                      loadDnsCache
//********************************************************************

void Server::loadDnsCache() {
    xml::DocPtr xmlDoc(xml::loadFile(Path::Config / "dns.xml", "DnsCache"));
    if(!xmlDoc)
        return;

    xmlNodePtr curNode = xmlDocGetRootElement(xmlDoc.get())->children;
    while(curNode && xmlIsBlankNode(curNode))
        curNode = curNode->next;

    for(; curNode != nullptr; curNode = curNode->next) {
        if(!NODE_NAME(curNode, "Dns"))
            continue;
        std::string ip, hostname;
        long time=0;
        for(xmlNodePtr childNode = curNode->children; childNode != nullptr; childNode = childNode->next) {
            if(NODE_NAME(childNode, "Ip")) {
                xml::copyToString(ip, childNode);
            } else if(NODE_NAME(childNode, "HostName")) {
                xml::copyToString(hostname, childNode);
            } else if(NODE_NAME(childNode, "Time")) {
                xml::copyToNum(time, childNode);
                addCache(ip, hostname, time);
            }
        }
    }
    xmlCleanupParser();
}


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
    for(const auto& [name, player] : players) {
        if(!player->isConnected())
            continue;
        player->save(true);
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

bool Server::checkDuplicateName(const std::shared_ptr<Socket>& sock, bool dis) {
    for(const auto &s : sockets) {
        if(sock != s && s->hasPlayer() && s->getPlayer()->getName() ==  sock->getPlayer()->getName()) {
            if(!dis) {
                sock->printColor("\n\n^ySorry, that character is already logged in.^x\n\n\n");
                // Return to account menu instead of reconnecting
                auto account = sock->getAccount();
                if(account) {
                    sock->clearPlayer();
                    showAccountMenu(sock, account);
                } else {
                    sock->reconnect();
                }
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
// returning true indicates the limit has been exceeded
// if disconnectOnLimit is true, will disconnect the connecting socket

bool Server::checkDouble(const std::shared_ptr<Socket>& sock, bool disconnectOnLimit) {
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
        if(!player || s == sock)
            continue;

        if(player->isCt())
            continue;
        if(player->flagIsSet(P_LINKDEAD))
            continue;
        if(sock->getIp() != s->getIp())
            continue;

        cnt++;

        if(cnt >= gConfig->getMaxDouble()) {
            if(disconnectOnLimit) {
                sock->write("\nMaximum number of connections has been exceeded!\n\n");
                sock->disconnect();
            }
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
    // Indexed by (tz hours + 12), covering UTC-12 .. UTC+13.
    static constexpr std::array<std::string_view, 26> zones = {
        "International Date Line West",
        "Midway Island, Samoa",
        "Hawaii",
        "Alaska",
        "Pacific",
        "Mountain",
        "Central",
        "Eastern",
        "Atlantic",
        "Brasilia, Buenos Aires, Georgetown, Greenland",
        "Mid-Atlantic",
        "Azores, Cape verde Is.",
        "Greenwich Mean Time",
        "Berlin, Rome, Prague, Warsaw, , West Central Africa",
        "Athens, Minsk, Cairo, Jerusalem",
        "Baghdad, Moscow, Nairobi",
        "Abu Dhabi, Tbilsi",
        "Islamabad, Karachi, Tashkent",
        "Almaty, Dhaka, Sri Jayawardenepura",
        "Bangkok, Jakarta, Krasnoyarsk",
        "Beijing, Hong Kong, Singapore, Taipei",
        "Osaka, Tokyo, Seoul",
        "Melbourne, Sydney, Guam, Vladivostok",
        "Magadan, Solomon Is., New Caledonia",
        "Auckland, , Fiji, Marshall Is.",
        "Nuku'alofa",
    };

    time_t curr = time(nullptr);
    tm local{};
    gmtime_r(&curr, &local);
    time_t utc = mktime(&local);
    int idx = static_cast<int>(difftime(utc, curr) / -3600) + 12;

    if(idx < 0 || idx >= static_cast<int>(zones.size()))
        return("Unknown");
    return std::string(zones[idx]);
}

std::string Server::getServerTime() {
    time_t t = time(nullptr);
    tm local{};
    localtime_r(&t, &local);
    return fmt::format("{:%a %b %e %H:%M:%S %Y}", local) + "(" + getTimeZone() + ")";
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

    registeredIds.emplace(toRegister->getId(), toRegister);
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

template<typename Map>
static std::shared_ptr<MudObject> lockRegistered(const Map& ids, const std::string& key) {
    auto it = ids.find(key);
    return it == ids.end() ? nullptr : it->second.lock();
}

std::shared_ptr<Object> Server::lookupObjId(const std::string &toLookup) {
    if(toLookup.empty() || toLookup[0] != 'O')
        return(nullptr);
    auto res = lockRegistered(registeredIds, toLookup);
    return res ? res->getAsObject() : nullptr;
}

std::shared_ptr<Creature> Server::lookupCrtId(const std::string &toLookup) {
    if(toLookup.empty() || (toLookup[0] != 'M' && toLookup[0] != 'P'))
        return(nullptr);
    auto res = lockRegistered(registeredIds, toLookup);
    return res ? res->getAsCreature() : nullptr;
}

std::shared_ptr<Player> Server::lookupPlyId(const std::string &toLookup) {
    if(toLookup.empty() || toLookup[0] != 'P')
        return(nullptr);
    auto res = lockRegistered(registeredIds, toLookup);
    return res ? res->getAsPlayer() : nullptr;
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


static std::string makeNextId(char prefix, long& counter, bool& dirty) {
    dirty = true;
    return fmt::format("{}{}", prefix, ++counter);
}

std::string Server::getNextMonsterId() { return makeNextId('M', maxMonsterId, idDirty); }
std::string Server::getNextObjectId()  { return makeNextId('O', maxObjectId,  idDirty); }
std::string Server::getNextPlayerId()  { return makeNextId('P', maxPlayerId,  idDirty); }

void Server::loadIds() {
    xml::DocPtr xmlDoc(xml::loadFile(Path::Game / "ids.xml", "Ids"));
    if(!xmlDoc)
        return;

    xmlNodePtr curNode = xmlDocGetRootElement(xmlDoc.get())->children;
    while(curNode && xmlIsBlankNode(curNode))
        curNode = curNode->next;

    for(; curNode != nullptr; curNode = curNode->next) {
             if(NODE_NAME(curNode, "MaxMonsterId")) xml::copyToNum(maxMonsterId, curNode);
        else if(NODE_NAME(curNode, "MaxPlayerId")) xml::copyToNum(maxPlayerId, curNode);
        else if(NODE_NAME(curNode, "MaxObjectId")) xml::copyToNum(maxObjectId, curNode);
    }
    xmlCleanupParser();
    idDirty = false;
}
void Server::saveIds() {
    if(!idDirty)
        return;

    xml::DocPtr xmlDoc(xmlNewDoc(BAD_CAST "1.0"));
    xmlNodePtr rootNode = xmlNewDocNode(xmlDoc.get(), nullptr, BAD_CAST "Ids", nullptr);
    xmlDocSetRootElement(xmlDoc.get(), rootNode);

    xml::newNumChild(rootNode, "MaxMonsterId", maxMonsterId);
    xml::newNumChild(rootNode, "MaxPlayerId",  maxPlayerId);
    xml::newNumChild(rootNode, "MaxObjectId",  maxObjectId);

    xml::saveFile(Path::Game / "ids.xml", xmlDoc.get());

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
            targetStr += fmt::format("({})", oTarget->info.displayStr());
            if(dir == GOLD_IN) {
                source = oTarget->droppedBy.str();
            }
        }
    }
    std::string room;
    if(auto parentRoom = player->getRoomParent()) {
        if(auto uniqueRoom = parentRoom->getAsUniqueRoom()) {
            room = fmt::format("{}({})", parentRoom->getName(), uniqueRoom->info.displayStr());
        } else if (auto areaRoom = parentRoom->getAsAreaRoom()) {
            auto area = areaRoom->area.lock();
            room = fmt::format("{}({})", area ? area->name : "<invalid>", areaRoom->mapmarker.str());
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
        if(!area)
            return(false);

        fs::path filename = Path::AreaRoom / std::to_string(area->id) / aRoom->mapmarker.filename();

        if(fs::exists(filename)) {
            xml::DocPtr xmlDoc(xml::loadFile(filename, "AreaRoom"));
            if(!xmlDoc)
                throw std::runtime_error("Unable to read arearoom file");

            aRoom->reset();
            aRoom->area = area;
            aRoom->load(xmlDocGetRootElement(xmlDoc.get()));
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
    for(const auto& pIt: oldRoom->players) {
        if(auto ply = pIt.lock()) {
            room->players.insert(ply);
            ply->setParent(room);
        }
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
    running = false;
    if(httpServer) httpServer->stop();
    asio::error_code ec;
    tickTimer.cancel();
    for(auto& acc : acceptors) acc.close(ec);
    for(const auto& s : sockets)
        s->drainAndClose();
}

//*********************************************************************
//                      Account Management Methods
//*********************************************************************

std::shared_ptr<Account> Server::getOrLoadAccount(const std::string& accountName) {
    auto it = accountCache.find(accountName);
    if(it != accountCache.end()) {
        return it->second;  // Return existing shared instance
    }
    
    // Load from disk
    std::shared_ptr<Account> account;
    if(Account::load(accountName, account)) {
        accountCache[accountName] = account;
        return account;
    }
    
    return nullptr;
}

void Server::trackAccountConnection(const std::string& accountName, const std::string& characterName) {
    accountConnections[accountName].insert(characterName);
}

void Server::untrackAccountConnection(const std::string& accountName, const std::string& characterName) {
    auto it = accountConnections.find(accountName);
    if(it != accountConnections.end()) {
        it->second.erase(characterName);
        if(it->second.empty()) {
            // No more connections, can remove from cache
            accountCache.erase(accountName);
            accountConnections.erase(it);
        }
    }
}

std::vector<std::string> Server::getAccountCharacters(const std::string& accountName) const {
    auto it = accountConnections.find(accountName);
    if(it != accountConnections.end()) {
        return std::vector<std::string>(it->second.begin(), it->second.end());
    }
    return {};
}

void Server::releaseAccount(const std::string& accountName, const std::string& characterName) {
	// If a specific character is provided, untrack it first
	if(!characterName.empty()) {
		untrackAccountConnection(accountName, characterName);
		return;
	}
	// No character provided: if there are no active character connections
	// for this account, evict the account from cache.
	auto it = accountConnections.find(accountName);
	if(it == accountConnections.end() || it->second.empty()) {
		accountCache.erase(accountName);
		if(it != accountConnections.end()) {
			accountConnections.erase(it);
		}
	}
}

void Server::saveAllCachedAccounts() {
	// Only save accounts that have at least one active character connection.
	// Also prune any accounts that linger in cache without connections.
	std::vector<std::string> toErase;
	for(const auto& [accountName, account] : accountCache) {
		auto it = accountConnections.find(accountName);
		bool hasConnections = (it != accountConnections.end() && !it->second.empty());
		if(!hasConnections) {
			toErase.push_back(accountName);
			continue;
		}
		if(account) {
			account->save();
		}
	}
	// Erase after iterating to avoid invalidating iterators
	for(const auto& name : toErase) {
		accountCache.erase(name);
		accountConnections.erase(name);
	}
}
