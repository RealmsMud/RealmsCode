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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

// C Includes
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h> // Needs: htons, htonl, INADDR_ANY, sockaddr_in
#include <sys/socket.h> // Needs: bind, std::listen, socket, AF_INET
#include <fcntl.h>      // Needs: fnctl
#include <netdb.h>      // Needs: gethostbyaddr
#include <sys/wait.h>   // Needs: WNOHANG, wait3
#include <errno.h>
#include <stdlib.h>
#include <sys/resource.h>

// C++ Includes
#include <iostream>
#include <sstream>
#include <iomanip>
#include <locale>

// Mud Includes
#include "calendar.hpp"
#include "config.hpp"
#include "creatures.hpp"
#include "dm.hpp"
#include "login.hpp"
#include "factions.hpp"
#include "mud.hpp"
#include "rooms.hpp"
#include "server.hpp"
#include "serverTimer.hpp"
#include "socket.hpp"
#include "version.hpp"
#include "web.hpp"
#include "xml.hpp"


// External declarations
extern int Numplayers;
extern long last_time_update;
extern long last_weather_update;

// Function prototypes
char *inetname(struct in_addr in );
void initSpellList(); // TODO: Move spelling stuff into server

// Global server pointer
Server* gServer = nullptr;

// Static initialization
Server* Server::myInstance = nullptr;

bool CanCleanupRoomFn::operator()( UniqueRoom* r ) { return r->players.empty(); }

void CleanupRoomFn::operator()( UniqueRoom* r ) {
	r->saveToFile(PERMONLY);
	delete r;
}

// Custom comparison operator to sort by the numeric id instead of standard string comparison
bool idComp::operator() (const bstring& lhs, const bstring& rhs) const {
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
    Deadchildren = 0;
    pulse = 0;
    webInterface = 0;
    lastDnsPrune = lastUserUpdate = lastRoomPulseUpdate = lastRandomUpdate = lastActiveUpdate = 0;
    maxPlayerId = maxObjectId = maxMonsterId = 0;
    loadDnsCache();
    pythonHandler = 0;
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
    effectsIndex.clear();
    cleanUpPython();
    clearAreas();
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
    struct rlimit lim;
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
#if !defined(__CYGWIN__) && !defined(__MACOS__)
    std::clog << "Installing custom printf handlers...";
    if(installPrintfHandlers() == 0)
        std::clog << "done." << std::endl;
    else
        std::clog << "failed." << std::endl;
#endif

    gConfig->loadBeforePython();
    gConfig->startFlashPolicy();
    gConfig->setLotteryRunTime();

    Port = gConfig->getPortNum();
    Tablesize = getdtablesize();


    std::clog << "Initializing Spelling...";
    init_spelling();
    std::clog << "done." << std::endl;

    initWebInterface();

    std::clog <<  "Initializing Spelling List...";
    initSpellList();
    std::clog << "done." << std::endl;

    // Python
    std::clog <<  "Initializing Python...";
    initPython();

    std::clog << "Loading Areas..." << (loadAreas() ? "done" : "*** FAILED ***") << std::endl;
    gConfig->loadAfterPython();



#ifdef SQL_LOGGER
    std::clog <<  "Initializing SQL Logger...";
    if(initSql())
        std::clog << "done." << std::endl;
    else
        std::clog << "failed." << std::endl;
#endif // SQL_LOGGER

    umask(000);
    srand(getpid() + time(0));
    if(rebooting) {
        std::clog << "Doing a reboot." << std::endl;
        finishReboot();
    } else if (!gConfig->isListing()) {
        addListenPort(Port);
        char filename[80];
        snprintf(filename, 80, "%s/reboot.xml", Path::Config);
        unlink(filename);
    }
    return(true);
}

//********************************************************************
//                      installSignalHandlers
//********************************************************************

void Server::installSignalHandlers() {
    if (!gConfig->isListing()) {
        signal(SIGABRT, crash); // abnormal termination triggered by abort call
        std::clog << ".";
        signal(SIGFPE, crash);  // floating point exception
        std::clog << ".";
        signal(SIGSEGV, crash); // segment violation
        std::clog << ".";
    } else {
        std::clog << "Ignoring crash handlers";
    }
    signal(SIGPIPE, SIG_IGN);
    std::clog << ".";
    signal(SIGTERM, SIG_IGN);
    std::clog << ".";
    signal(SIGCHLD, child_died);
    std::clog << ".";
    signal(SIGHUP, quick_shutdown);
    std::clog << ".";
    signal(SIGINT, shutdown_now);
    std::clog << ".";
}


void Server::setGDB() { GDB = true; }
void Server::setRebooting() { rebooting = true; }
void Server::setValgrind() { valgrind = true; }
bool Server::isRebooting() { return(rebooting); }
bool Server::isGDB() { return(GDB); }
bool Server::isValgrind() { return(valgrind); }
int Server::getNumSockets() { return(sockets.size()); }

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
    if(myInstance != nullptr)
        delete myInstance;
    myInstance = nullptr;
}

// End - Instance Functions
//--------------------------------------------------------------------



void Server::populateVSockets() {
    if(vSockets)
        return;
    vSockets = new SocketVector(sockets.size());
    std::copy(sockets.begin(), sockets.end(), vSockets->begin());
    random_shuffle(vSockets->begin(), vSockets->end());
}


//********************************************************************
//                      run
//********************************************************************

int Server::run(void) {
    ServerTimer timer;
    if(running == false)
    {
        std::cerr << "Not bound to any ports, exiting." << std::endl;
        exit(-1);
    }

    while(running) {
        if(Deadchildren) reapChildren();

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
        vSockets = 0;

        timer.end(); // End the timer
        timer.sleep();
    }

    return(0);
}

//********************************************************************
//                      addListenPort
//********************************************************************

int Server::addListenPort(int port) {
    struct sockaddr_in sa;
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
    controlSocks.push_back(controlSock(port, control));
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

    struct timeval noTime;
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

    for(Socket * sock : sockets) {
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

    struct sockaddr_in addr;

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

    Socket *newSock;
    bool dnsDone = false;
    newSock = new Socket(fd, addr, dnsDone);
    sockets.push_back(newSock);

    newSock->showLoginScreen(dnsDone);
    return(0);
}

//********************************************************************
//                      processInput
//********************************************************************

int Server::processInput() {

    for(Socket * sock : sockets) {
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
    for(Socket * sock : *vSockets) {
        if(sock->hasCommand() && sock->getState() != CON_DISCONNECTING) {
            if(sock->processOneCommand() == -1) {
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
    for(Socket * sock : *vSockets) {
        Player* player=sock->getPlayer();
        if(player) {
            if(player->isFleeing() && player->canFlee(false)) {
                player->doFlee();
            } else if(player->autoAttackEnabled() && !player->isFleeing() && player->hasAttackableTarget() && player->isAttackingTarget()) {
                if(!player->checkAttackTimer(false))
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
    for(Socket * sock : *vSockets) {
        if(FD_ISSET(sock->getFd(), &outSet) && sock->hasOutput()) {
            sock->flush();
        }
    }
    return(0);
}

//********************************************************************
//                      deleteSocket
//********************************************************************
// This should force a socket off right away

int Server::deleteSocket(Socket* sock) {
    sock->setState(CON_DISCONNECTING);
    cleanUp();
    return(0);
}

//********************************************************************
//                      disconnectAll
//********************************************************************

void Server::disconnectAll() {
    for(Socket * sock : sockets) {
    // Set everyone to disconnecting
        sock->setState(CON_DISCONNECTING);
    }
    // And now clean them up
    cleanUp();
}

//********************************************************************
//                      cleanUp
//********************************************************************

int Server::cleanUp() {
    std::list<Socket*>::iterator it;
    Socket *sock=0;

    for(it = sockets.begin() ; it != sockets.end() ;) {
        sock = *it;
        if(sock->getState() == CON_DISCONNECTING) {
            // Flush any residual data
            sock->flush();
            delete sock;
            it = sockets.erase(it);
        } else
            it++;
    }

    return(0);
}

//********************************************************************
//                      getDnsCacheString
//********************************************************************

bstring Server::getDnsCacheString() {
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

bool Server::getDnsCache(bstring &ip, bstring &hostName) {
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
    long currentTime = time(0);
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

    for(Socket* sock : *vSockets) {
        Player* player=sock->getPlayer();
        if(player) {
            player->pulseTick(t);
            if(player->isPlaying())
                player->pulseSong(t);
        }
    }
}



//*********************************************************************
//                      updateUsers
//*********************************************************************

void Server::updateUsers(long t) {
    int tout = 300;
    lastUserUpdate = t;

    for(Socket* sock : *vSockets) {

        Player* player= sock->getPlayer();

        if(player) {
            if(player->isDm()) tout = t;
            else if(player->isStaff()) tout = 1200;
            else tout = 600;
        } else {
            tout = 300;
        }
        if(t - sock->ltime > tout) {
            sock->write("\n\rTimed out.\n\r");
            sock->setState(CON_DISCONNECTING);
        }
        if(player) {
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
    Monster* monster=0;
    BaseRoom* room=0;
    UniqueRoom* uRoom=0;
    AreaRoom* aRoom=0;
    WanderInfo* wander=0;
    CatRef  cr;
    int     num=0, l=0;
    std::map<bstring, bool> check;
    //Object      *object;
    //int       k, numout=0, alnum=0, x=0;

    lastRandomUpdate = t;

    Player* player;
    for(Socket * sock : *vSockets) {
        player = sock->getPlayer();

        if(!player || !player->getRoomParent())
            continue;
        uRoom = player->getUniqueRoomParent();
        aRoom = player->getAreaRoomParent();
        room = player->getRoomParent();

        wander = 0;
        if(uRoom) {
            // handle monsters arriving in unique rooms
            if(!uRoom->info.id)
                continue;

            if(check.find(uRoom->info.str()) != check.end())
                continue;

            check[uRoom->info.str()] = true;
            wander = &uRoom->wander;
        } else {
            // handle monsters arriving in area rooms
            if(check.find(aRoom->mapmarker.str()) != check.end())
                continue;
            if(aRoom->unique.id)
                continue;

            check[aRoom->mapmarker.str()] = true;
            wander = aRoom->getRandomWanderInfo();
        }
        if(!wander)
            continue;

        if(mrand(1,100) > wander->getTraffic())
            continue;

        cr = wander->getRandom();
        if(!cr.id)
            continue;
        if(room->countCrt() >= room->getMaxMobs())
            continue;

        // Will make mobs not spawn if a DM is invis in the room. -TC
        if(!room->countVisPly())
            continue;

        if(!loadMonster(cr, &monster))
            continue;

        // if the monster can't go there, they won't wander there
        if(aRoom && !aRoom->area->canPass(monster, &aRoom->mapmarker, true)) {
            free_crt(monster);
            continue;
        }

        if(!monster->flagIsSet(M_CUSTOM))
            monster->validateAc();

        if((monster->flagIsSet(M_NIGHT_ONLY) && isDay()) ||(monster->flagIsSet(M_DAY_ONLY) && !isDay())) {
            free_crt(monster);
            continue;
        }

        if(room->flagIsSet(R_PLAYER_DEPENDENT_WANDER))
            num = mrand(1, room->countVisPly());
        else if(monster->getNumWander() > 1)
            num = mrand(1, monster->getNumWander());
        else
            num = 1;

        for(l=0; l<num; l++) {
            monster->initMonster();

            if(monster->flagIsSet(M_PERMENANT_MONSTER))
                monster->clearFlag(M_PERMENANT_MONSTER);

            if(!l)
                monster->addToRoom(room, num);
            else
                monster->addToRoom(room, 0);

            if(!monster->flagIsSet(M_PERMENANT_MONSTER) || monster->flagIsSet(M_NO_ADJUST))
                monster->adjust(-1);

            gServer->addActive(monster);
            if(l != num-1)
                loadMonster(cr, &monster);
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
    Player  *ply=0;
    Creature *target=0;
    Monster *monster=0;
    BaseRoom* room=0;

    long    tt = gConfig->currentHour();
    int     timetowander=0, immort=0;
    bool    print=false;

    lastActiveUpdate = t;

    if(activeList.empty())
        return;
//  if(!(cp = first_active))
//      return;

    MonsterList::iterator it = activeList.begin();
    while(it != activeList.end()) {
        // Increment the iterator in case this monster dies during the update and is removed from the active list
        monster = (*it++);

        // Better be a monster to be on the active list
        ASSERTLOG(monster);

        if(!monster->inRoom()) {
            broadcast(isStaff, "^y%s without a parent/area room on the active list. Info: %s. Deleting.",
                monster->getCName(), monster->info.str().c_str());
            monster->deleteFromRoom();
            gServer->delActive(monster);
            free_crt(monster);
            it = activeList.begin();
            continue;
        }


        room = monster->getRoomParent();

        // Reset's mob's smack-talking broadcasts at 7am every day
        if(tt == 7 && ((t - last_time_update) / 2) == 0)
            monster->daily[DL_BROAD].cur = 20;

        if( (monster->flagIsSet(M_NIGHT_ONLY) && isDay()) ||
            (monster->flagIsSet(M_DAY_ONLY) && !isDay()))
        {

            for(Player* ply : room->players) {
                if(ply->isStaff()) {
                    immort = 1;
                    break;
                }
            }
            if(!immort) {
                timetowander=1;
                broadcast(nullptr, monster->getRoomParent(), "%M wanders slowly away.", monster);
                monster->deleteFromRoom();
                gServer->delActive(monster);
                free_crt(monster);
                it = activeList.begin();
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
            gServer->delActive(monster);
            it = activeList.begin();
            continue;
        }

        // Lets see if we'll attack any other monsters in this room
        if(monster->checkEnemyMobs()) {
            continue;
        }


        if(monster->flagIsSet(M_KILL_PERMS)) {
            for(Monster* mons : room->monsters) {
                if(mons == monster)
                    continue;
                if( mons->flagIsSet(M_PERMENANT_MONSTER) &&
                    !monster->willAssist(mons->getAsMonster()) &&
                    !monster->isEnemy(mons)
                )
                    monster->addEnemy(mons);
            }
        }

        if(monster->flagIsSet(M_KILL_NON_ASSIST_MOBS)) {
            for(Monster* mons : room->monsters) {
                if(mons == monster)
                    continue;
                if( !monster->willAssist(mons->getAsMonster()) &&
                    !mons->flagIsSet(M_PERMENANT_MONSTER) &&
                    !monster->isEnemy(mons) && 
                    !monster->isPet()
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
                (mrand(1,100) < (30+monster->intelligence.getCur()/10)))
            {
                broadcast(nullptr, monster->getRoomParent(), "%M casts a curepoison spell on %sself.", monster, monster->himHer());
                monster->mp.decrease(6);
                monster->curePoison();
                continue;
            }
        }

        if(!monster->checkAttackTimer(false)) {
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
            continue;
        }


        if(monster->getPrimeFaction() != "")
            Faction::worshipSocial(monster);


        // summoned monsters expire here
        if( monster->isPet() &&
            (t > LT(monster, LT_INVOKE) || t > LT(monster, LT_ANIMATE)))
        {
            if(monster->isUndead())
                broadcast(nullptr, room, "%1M wanders away.", monster);
            else
                broadcast(nullptr, room, "%1M fades away.", monster);

            monster->die(monster->getMaster());
            it = activeList.begin();
            continue;
        }

        monster->updateAttackTimer();
        if(monster->dexterity.getCur() > 200 || monster->isEffected("haste"))
            monster->modifyAttackDelay(-10);
        if(monster->isEffected("slow"))
            monster->modifyAttackDelay(10);


        if(monster->flagIsSet(M_WILL_WIELD))
            monster->mobWield();
        else
            monster->clearFlag(M_WILL_WIELD);

        // Grab some stuff from the room
        monster->checkScavange(t);

        // See if we can wander around or away
        int mobileResult = monster->checkWander(t);
        if(mobileResult == 1) {
            continue;
        } if(mobileResult == 2) {
            monster->deleteFromRoom();
            gServer->delActive(monster);
            free_crt(monster);
            it = activeList.begin();
            continue;
        }


        // Steal from people
        if(monster->flagIsSet(M_STEAL_ALWAYS) && (t - monster->lasttime[LT_STEAL].ltime) > 60 && mrand(1, 100) <= 5) {
            ply = lowest_piety(room, monster->isEffected("detect-invisible"));
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
            mrand(1,100) <= monster->getUpdateAggro() &&
            room->countVisPly()
        ) {
            //broadcast(isDm, "^g%M(L%d,R:%s) just became aggressive!", monster, monster->getLevel(), room->fullName().c_str());
            monster->setFlag(M_AGGRESSIVE);
        }


        target = monster->whoToAggro();
        if(target) {

            print = (   !monster->flagIsSet(M_HIDDEN) &&
                        !(monster->isInvisible() &&
                        target->isEffected("detect-invisible"))
                    );

            monster->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
            monster->addEnemy(target, print);

            if(target->flagIsSet(P_LAG_PROTECTION_SET))
                target->setFlag(P_LAG_PROTECTION_ACTIVE);

        }
    }
}






//--------------------------------------------------------------------
// Active list manipulation

//*********************************************************************
//                      add_active
//*********************************************************************
// This function adds a monster to the active-monster list. A pointer
// to the monster is passed in the first parameter.

void Server::addActive(Monster* monster) {

    if(!monster) {
        loga("add_active: tried to activate a null crt!\n");
        return;
    }
    if(isActive(monster))
        return;
    // try and guess if the creature is valid
    {
        const char *p = monster->getCName();
        while(*p) {
            ASSERTLOG(isPrintable((unsigned char)*p));
            p++;
        }
        ASSERTLOG(p - monster->getCName());
    }

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
        std::cerr << "Attempting to delete '" << monster->getName() << "' from active list with an empty active list." << std::endl;
        broadcast(isStaff, "^yAttempting to delete %s from active list with an empty active list.", monster->getCName());
        return;
    }

    MonsterList::iterator it = std::find(activeList.begin(), activeList.end(), monster);
    if(it == activeList.end()) {
        std::cerr << "Attempting to delete '" << monster->getName() << "' from active list but could not find them on the list." << std::endl;
        broadcast(isStaff, "^yAttempting to delete %s from active list but could not find them on the list.", monster->getCName());
        return;
    }


    activeList.remove(monster);
}


//*********************************************************************
//                      isActive
//*********************************************************************
// This function returns 1 if the parameter passed is in the
// active list.

bool Server::isActive(Monster* monster) {
    if(activeList.empty())
        return(false);

    MonsterList::iterator it = std::find(activeList.begin(), activeList.end(), monster);
    if(it == activeList.end())
        return(false);

    if(*it == monster)
        return(true);

    return(false);
}

// End - Active List Manipulation
//--------------------------------------------------------------------


//--------------------------------------------------------------------
// Children Control

//********************************************************************
//                      childDied
//********************************************************************

void Server::childDied() {
    Deadchildren++;
}

//********************************************************************
//                      getDeadChildren
//********************************************************************

int Server::getDeadChildren() const {
    return(Deadchildren);
}

//********************************************************************
//                      reapChildren
//********************************************************************

int Server::reapChildren() {
    int pid, status;
    bool dnsChild = false;
    childProcess c;
    bool found=false;

    while(Deadchildren > 0) {
        pid = waitpid(-1, &status, WNOHANG);
        if(pid <= 0) {
            Deadchildren--;
            return(-1);
        }
        std::list<childProcess>::iterator it;
        for( it = children.begin(); it != children.end();) {
            if((*it).pid == pid) {
                if((*it).type == CHILD_DNS_RESOLVER) {
                    char tmpBuf[1024];
                    memset(tmpBuf, '\0', sizeof(tmpBuf));
                    // Read in the results from the resolver
                    int n = read((*it).fd, tmpBuf, 1023);

                    // Close the read fd in the pipe now, won't need it anymore
                    close((*it).fd);
                    // If we have an error reading, just use the ip address then
                    if( n <= 0 ) {
                        if(errno == EWOULDBLOCK)
                            std::clog << "DNS ReapChildren: Error would block\n";
                        else
                            std::clog << "DNS ReapChildren: Error\n";
                        strcpy(tmpBuf, (*it).extra.c_str());
                    }

                    // Add dns to cache
                    addCache((*it).extra, tmpBuf);
                    dnsChild = true;

                    // Now we want to look through all connected sockets and update dns where appropriate
                    for(Socket * sock : sockets) {
                        if(sock->getState() == LOGIN_DNS_LOOKUP && sock->getIp() == (*it).extra) {
                            // Be sure to set the hostname first, then check for lockout
                            sock->setHostname(tmpBuf);
                            sock->checkLockOut();
                        }
                    }
                    std::clog << "Reaped DNS child (" << pid << "-" << tmpBuf << ")\n";
                } else if((*it).type == CHILD_LISTER) {
                    std::clog << "Reaping LISTER child (" << pid << "-" << (*it).extra << ")" << std::endl;
                    processListOutput(*it);
                    // Don't forget to close the pipe!
                    close((*it).fd);
                } else if((*it).type == CHILD_SWAP_FINISH) {
                    std::clog << "Reaping MoveRoom Finish child (" << pid << "-" << (*it).extra << ")" << std::endl;
                    c = *it;
                    found = true;
                } else if((*it).type == CHILD_PRINT) {
                    //broadcast(isDm, "Reaping Print Child (%d-%s)",pid, (*it).extra.c_str());
                    std::clog << "Reaping Print child (" << pid << "-" << (*it).extra << ")" << std::endl;
                    c = *it;
                    found = true;
                } else {
                    std::clog << "ReapChildren: Unknown child type " << (*it).type << std::endl;
                }
//              // Child was processed, reduce number of children
//              Deadchildren--;
//              printf("Reaped Child: %d dead children left.\n", Deadchildren);
                it = children.erase(it);

                // finish swap after they've been deleted from the list
                if(found) {
                    found = false;
                    if(c.type == CHILD_SWAP_FIND) {
                        gConfig->findNextEmpty(c, true);
                    } else if(c.type == CHILD_SWAP_FINISH) {
                        gConfig->offlineSwap(c, true);
                    } else if(c.type == CHILD_PRINT) {
                        const Player* player = gServer->findPlayer(c.extra);
                        bstring output = gServer->simpleChildRead(c);
                        if(player && output != "")
                            player->printColor("%s\n", output.c_str());
                    }
                    // Don't forget to close the pipe!
                    close(c.fd);
                }
                continue;
            } else {
                it++;
                continue;
            }
        }
        Deadchildren--;
    }
    if(dnsChild)
        saveDnsCache();

// just in case, kill off any zombies
    wait3(&status, WNOHANG, (struct rusage *)0);
    return(0);
}

//********************************************************************
//                      processListOutput
//********************************************************************

int Server::processListOutput(childProcess &lister) {
    bool found = false;
    std::list<Socket*>::iterator sIt;
    Socket *sock;
    for(sIt = sockets.begin() ; sIt != sockets.end() ; ++sIt ) {
        sock = *sIt;
        if(sock->getPlayer() && lister.extra == sock->getPlayer()->getName()) {
            found = true;
            break;
        }
    }

    char tmpBuf[4096];
    bstring toWrite;
    int n;
    for(;;) {
        // Even if no socket is found, read in all of the data
        memset(tmpBuf, '\0', sizeof(tmpBuf));
        n = read(lister.fd, tmpBuf, sizeof(tmpBuf)-1);
        if(n <= 0)
            break;

        if(found) {
            toWrite = tmpBuf;
            toWrite.Replace("\n", "\nList> ");
            sock->write(toWrite, false);
        }
    }
    return(1);
}

//********************************************************************
//                      processChildren
//********************************************************************

int Server::processChildren(void) {
    for(childProcess & child : children) {
        if(child.type == CHILD_DNS_RESOLVER) {
            // Ignore, will be handled by reapChildren
        } else if(child.type == CHILD_LISTER) {
            processListOutput(child);
        } else if(child.type == CHILD_SWAP_FIND) {
            gConfig->findNextEmpty(child, false);
        } else if(child.type == CHILD_SWAP_FINISH) {
            gConfig->offlineSwap(child, false);
        } else if(child.type == CHILD_PRINT) {
            const Player* player = gServer->findPlayer(child.extra);
            bstring output = gServer->simpleChildRead(child);
            if(player && output != "")
                player->printColor("%s\n", output.c_str());
        } else {
            std::clog << "processChildren: Unknown child type " << child.type << std::endl;
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
        // Child Process
        // Close the reading end, we'll only be writing
        close(fds[0]);
        struct hostent *he = 0;
        int tries = 0;
        while(tries < 5 && tries >= 0) {
            he = gethostbyaddr((char *)&addr.sin_addr.s_addr, sizeof(addr.sin_addr.s_addr), AF_INET);

            if(he == nullptr) {
                switch(h_errno) {
                case HOST_NOT_FOUND:
                    std::clog << "DNS Error: Host not found for " << sock->getIp() << std::endl;
                    tries = -1;
                    break;
                case NO_RECOVERY:
                    std::clog << "DNS Error: Unrecoverable error for " << sock->getIp() << std::endl;
                    tries = -1;
                    break;
                case TRY_AGAIN:
                    std::clog << "DNS Error: Try again for " << sock->getIp() << std::endl;
                    tries++;
                    break;
                }
            } else
                break;
        }
        std::clog << "DNS: Resolver finished for " << sock->getIp() << "(" << (he ? he->h_name : sock->getIp()) << ")" << std::endl;
        if(he) {
            // Found a hostname so print it
            write(fds[1], he->h_name, strlen(he->h_name));
        } else {
            // Didn't find a hostname, print the ip
            write(fds[1], sock->getIp().c_str(), sock->getIp().length());
        }
        exit(0);
    } else { // pid != 0
        // Parent Process
        // Close the writing end, we'll only be reading
        close(fds[1]);
        std::clog << "Watching Child DNS Resolver for(" << sock->getIp() << ") running with pid " << pid
                  << " reading from fd " << fds[0] << std::endl;
        // Let the server know we're monitoring this child process
        addChild(pid, CHILD_DNS_RESOLVER, fds[0], sock->getIp());
    }
    return(0);
}

//********************************************************************
//                      addCache
//********************************************************************

void Server::addCache(bstring ip, bstring hostName, time_t t) {
    if(t == -1)
        t = time(0);
    cachedDns.push_back(dnsCache(ip, hostName, t));
}

//********************************************************************
//                      addChild
//********************************************************************

void Server::addChild(int pid, childType pType, int pFd, bstring pExtra) {
    std::clog << "Adding pid " << pid << " as child type " << pType << ", watching " << pFd << "\n";
    children.push_back(childProcess(pid, pType, pFd, pExtra));
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
        xml::newStringChild(curNode, "Ip", (*it).ip.c_str());
        xml::newStringChild(curNode, "HostName", (*it).hostName.c_str());
        xml::newNumChild(curNode, "Time", (long)(*it).time);
    }

    sprintf(filename, "%s/dns.xml", Path::Config);
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
    snprintf(filename, 80, "%s/dns.xml", Path::Config);
    xmlDoc = xml::loadFile(filename, "DnsCache");

    if(xmlDoc == nullptr)
        return;

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode)) {
        curNode = curNode->next;
    }
    if(curNode == 0) {
        xmlFreeDoc(xmlDoc);
        return;
    }
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Dns")) {
            bstring ip, hostname;
            long time=0;
            childNode = curNode->children;
            while(childNode != nullptr) {
                if(NODE_NAME(childNode, "Ip")) {
                    xml::copyToBString(ip, childNode);
                } else if(NODE_NAME(childNode, "HostName")) {
                    xml::copyToBString(hostname, childNode);
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
//                      resetShipsFile
//********************************************************************

void Config::resetShipsFile() {
    // copying ships.midnight.xml to ships.xml
    char    sfile1[80], sfile2[80], command[255];
    sprintf(sfile1, "%s/ships.midnight.xml", Path::Game);
    sprintf(sfile2, "%s/ships.xml", Path::Game);

    sprintf(command, "cp %s %s", sfile1, sfile2);
    system(command);
}

//********************************************************************
//                      startReboot
//********************************************************************

bool Server::startReboot(bool resetShips) {
    gConfig->save();

    gServer->setRebooting();
    // First give all players a free restore
    for(Socket *sock : sockets) {
        Player* player = sock->getPlayer();
        if(player && player->fd > -1) {
            player->hp.restore();
            player->mp.restore();
        }
    }

    // Then run through and save the reboot file
    saveRebootFile(resetShips);

    // Now disconnect people that won't make it through the reboot
    for(Socket *sock : sockets) {
        Player* player = sock->getPlayer();
        if(player && player->fd > -1 ) {
            // End the compression, we'll try to restart it after the reboot
            if(sock->getMccp()) {
                sock->endCompress();
            }
            player->save(true);
            players[player->getName()] = 0;
            player->uninit();
            free_crt(player);
            player = 0;
            sock->setPlayer(0);
        } else {
            sock->write("\n\r\n\r\n\rSorry, we are rebooting. You may reconnect in a few seconds.\n\r");
            sock->disconnect();
        }
    }

    processOutput();
    cleanUp();

    if(resetShips)
        gConfig->resetShipsFile();

    char port[10], path[80];

    sprintf(port, "%d", Port);
    strcpy(path, gConfig->cmdline);

    execl(path, path, "-r", port, (char *)nullptr);

    char filename[80];
    snprintf(filename, 80, "%s/config.xml", Path::Config);
    unlink(filename);

    merror("dmReboot failed!!!", FATAL);
    return(0);
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
    for(Socket *sock : sockets) {
        Player*player = sock->getPlayer();
        if(player && player->fd > -1) {
            curNode = xmlNewChild(rootNode, nullptr, BAD_CAST"Player", nullptr);
            xml::newStringChild(curNode, "Name", player->getCName());
            xml::newNumChild(curNode, "Fd", sock->getFd());
            xml::newStringChild(curNode, "Ip", sock->getIp().c_str());
            xml::newStringChild(curNode, "HostName", sock->getHostname().c_str());
            xml::newStringChild(curNode, "ProxyName", player->getProxyName());
            xml::newStringChild(curNode, "ProxyId", player->getProxyId());
            sock->saveTelopts(curNode);
        }
    }

    char filename[80];
    snprintf(filename, 80, "%s/reboot.xml", Path::Config);
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
    snprintf(filename, 80, "%s/reboot.xml", Path::Config);
    // build an XML tree from a the file
    doc = xml::loadFile(filename, "Reboot");
    unlink(filename);

    if(doc == nullptr) {
        std::clog << "Unable to loadBeforePython reboot file\n";
        merror("Loading reboot file", FATAL);
    }

    curNode = xmlDocGetRootElement(doc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode)) {
        curNode = curNode->next;
    }
    if(curNode == 0) {
        xmlFreeDoc(doc);
        merror("Parsing reboot file", FATAL);
    }

    Numplayers = 0;
    StartTime = time(0);

    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Server")) {
            childNode = curNode->children;
            while(childNode != nullptr) {
                if(NODE_NAME(childNode, "ControlSock")) {
                    int port = xml::getIntProp(childNode, "Port");
                    int control = xml::getIntProp(childNode, "Control");
                    controlSocks.push_back(controlSock(port, control));
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
            Player* player=0;
            Socket* sock=0;
            while(childNode != nullptr) {
                if(NODE_NAME(childNode, "Name")) {
                    bstring name;
                    xml::copyToBString(name, childNode);
                    if(!loadPlayer(name.c_str(), &player)) {
                        merror("finishReboot: loadPlayer", FATAL);
                    }
                }
                else if(NODE_NAME(childNode, "Fd")) {
                    int fd;
                    xml::copyToNum(fd, childNode);
                    sock = new Socket(fd);
                    if(!player || !sock)
                        merror("finishReboot: No Sock/Player", FATAL);

                    player->fd = fd;

                    sock->setPlayer(player);
                    player->setSock(sock);
                    addPlayer(player);
                }
                else if(NODE_NAME(childNode, "Ip")) {
                    bstring ip;
                    xml::copyToBString(ip, childNode);
                    sock->setIp(ip);
                }
                else if(NODE_NAME(childNode, "HostName")) {
                    bstring host;
                    xml::copyToBString(host, childNode);
                    sock->setHostname(host);
                } else if(NODE_NAME(childNode, "Telopts")) {
                    sock->loadTelopts(childNode);
                } else if(NODE_NAME(childNode, "ProxyName")) {
                    bstring proxyName = xml::getBString(childNode);
                    player->setProxyName(proxyName);
                } else if(NODE_NAME(childNode, "ProxyId")) {
                    bstring proxyId = xml::getBString(childNode);
                    player->setProxyId(proxyId);
                }

                childNode = childNode->next;
            }

            if(!player || !sock)
                merror("finishReboot: Finished, still no Sock/Player", FATAL);

            sock->ltime = time(0);
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
            sockets.push_back(sock);
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

Player* Server::findPlayer(bstring name) {
    std::map<bstring, Player*>::const_iterator it = players.find(name);

    if(it != players.end())
        return((*it).second);
    return(0);

}

//*********************************************************************
//                      saveAllPly
//*********************************************************************
// This function saves all players currently in memory.

void Server::saveAllPly() {
    for(std::pair<bstring, Player*> p : players) {
        if(!p.second->isConnected())
            continue;
        p.second->save(true);
    }
}


//*********************************************************************
//                      clearPlayer
//*********************************************************************
// This will NOT free up the player, it will just remove them from the list

bool Server::clearPlayer(bstring name) {
    players.erase(name);
    return(true);
}
bool Server::clearPlayer(Player* player) {
    ASSERTLOG(player);
    players.erase(player->getName());
    player->unRegisterMo();
    return(true);
}

//*********************************************************************
//                      addPlayer
//*********************************************************************

bool Server::addPlayer(Player* player) {
    ASSERTLOG(player);
    player->validateId();
    players[player->getName()] = player;
    player->getSock()->addToPlayerList();
    player->registerMo();
    return(true);
}

//*********************************************************************
//                      checkDuplicateName
//*********************************************************************

bool Server::checkDuplicateName(Socket* sock, bool dis) {
    for(Socket *s : sockets) {
        if(sock != s && s->getPlayer() && s->getPlayer()->getName() ==  sock->getPlayer()->getName()) {
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

bool Server::checkDouble(Socket* sock) {
    if(!gConfig->checkDouble)
        return(false);
    if(sock->getPlayer()->flagIsSet(P_ON_PROXY) || sock->getPlayer()->isCt())
        return(false);
    if(strstr(sock->getHostname().c_str(), "localhost"))
        return(false);

    Player* player=0;
    for(Socket *s : sockets) {
        player = s->getPlayer();
        if(!player || s == sock)
            continue;

        if(player->isCt())
            continue;
        if(player->flagIsSet(P_LINKDEAD) || player->flagIsSet(P_ON_PROXY))
            continue;
        if(sock->getIp() != s->getIp())
            continue;
        if(gConfig->canDoubleLog(sock->getPlayer()->getForum(), s->getPlayer()->getForum()))
            continue;

        s->printColor("^Y\n\nAnother character (%s) from your IP address has just logged in.\n\n",
            sock->getPlayer()->getCName());
        s->disconnect();
        return(false);
    }
    return(false);
}

//*********************************************************************
//                      sendCrash
//*********************************************************************

void Server::sendCrash() {
    char filename[80];
    snprintf(filename, 80, "%s/crash.txt", Path::Config);

    for(Socket *sock : sockets) {
        viewLoginFile(sock, filename);
    }
}


//*********************************************************************
//                      getTimeZone
//*********************************************************************
// not accurate for the fractional hour timezones

bstring Server::getTimeZone() {
    // current local time
    time_t curr = time(0);
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

bstring Server::getServerTime() {
    time_t t = time(0);
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
    Player* target=0;
    for(std::pair<bstring, Player*> p : players) {
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

bool Server::registerMudObject(MudObject* toRegister, bool reassignId) {
    ASSERT(toRegister != nullptr);

    if(toRegister->getId().equals("-1"))
        return(false);

    IdMap::iterator it =registeredIds.find(toRegister->getId());
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
    ASSERT(toUnRegister != nullptr);

    if(toUnRegister->getId().equals("-1"))
        return(false);

    IdMap::iterator it = registeredIds.find(toUnRegister->getId());
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
        if((*it).second == toUnRegister) {
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

Object* Server::lookupObjId(const bstring& toLookup) {
    if(toLookup[0] != 'O')
        return(nullptr);

    IdMap::iterator it = registeredIds.find(toLookup);

    if(it == registeredIds.end())
        return(nullptr);
    else
        return(((*it).second)->getAsObject());

}

Creature* Server::lookupCrtId(const bstring& toLookup) {
    if(toLookup[0] != 'M' && toLookup[0] != 'P')
        return(nullptr);

    IdMap::iterator it = registeredIds.find(toLookup);

    if(it == registeredIds.end())
        return(nullptr);
    else
        return(((*it).second)->getAsCreature());

}
Player* Server::lookupPlyId(const bstring& toLookup) {
    if(toLookup[0] != 'P')
        return(nullptr);
    IdMap::iterator it = registeredIds.find(toLookup);

    if(it == registeredIds.end())
        return(nullptr);
    else
        return(((*it).second)->getAsPlayer());

}
bstring Server::getRegisteredList() {
    std::ostringstream oStr;
    for(std::pair<bstring, MudObject*> p : registeredIds) {
        oStr << p.first << " - " << p.second->getName() << std::endl;
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


bstring Server::getNextMonsterId() {
    long id = ++maxMonsterId;
    bstring toReturn = bstring("M") + bstring(id);
    idDirty = true;
    return(toReturn);
}

bstring Server::getNextObjectId() {
    long id = ++maxObjectId;
    bstring toReturn = bstring("O") + bstring(id);
    idDirty = true;
    return(toReturn);
}

bstring Server::getNextPlayerId() {
    long id = ++maxPlayerId;
    bstring toReturn = bstring("P") + bstring(id);
    idDirty = true;
    return(toReturn);
}

void Server::loadIds() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;

    char filename[80];
    snprintf(filename, 80, "%s/ids.xml", Path::Game);
    xmlDoc = xml::loadFile(filename, "Ids");

    if(xmlDoc == nullptr)
        return;

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode)) {
        curNode = curNode->next;
    }
    if(curNode == 0) {
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

    if(idDirty == false)
        return;

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Ids", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    xml::newNumChild(rootNode, "MaxMonsterId", maxMonsterId);
    xml::newNumChild(rootNode, "MaxPlayerId",  maxPlayerId);
    xml::newNumChild(rootNode, "MaxObjectId",  maxObjectId);

    sprintf(filename, "%s/ids.xml", Path::Game);
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);

    idDirty = false;
}

void Server::logGold(GoldLog dir, Player* player, Money amt, MudObject* target, bstring logType) {
    bstring pName = player->getName();
    bstring pId = player->getId();
    // long amt
    bstring targetStr = "";
    bstring source = "";

    if(target) {
        targetStr = stripColor(target->getName());
        Object* oTarget = target->getAsObject();
        if(oTarget) {
            targetStr += "(" + oTarget->info.str() + ")";
            if(dir == GOLD_IN) {
                source = oTarget->droppedBy.str();
            }
        }
    }
    bstring room = "";
    if(player->getRoomParent()) {
        if(player->getRoomParent()->getAsUniqueRoom()) {
            room = bstring(player->getRoomParent()->getName()) + "(" + player->getRoomParent()->getAsUniqueRoom()->info.str() + ")";
        } else if (player->getRoomParent()->getAsAreaRoom()) {
            room = player->getRoomParent()->getAsAreaRoom()->area->name + "(" + player->getRoomParent()->getAsAreaRoom()->mapmarker.str() + ")";
        }
    }
    // logType
    bstring direction = (dir == GOLD_IN ? "In" : "Out");
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

bool Server::reloadRoom(BaseRoom* room) {
    UniqueRoom* uRoom = room->getAsUniqueRoom();

    if(uRoom) {
    	CatRef cr = uRoom->info;
        if(reloadRoom(cr))
        {
            roomCache.fetch(cr, false)->addPermCrt();
            return(true);
        }
    } else {

        AreaRoom* aRoom = room->getAsAreaRoom();
        char    filename[256];
        sprintf(filename, "%s/%d/%s", Path::AreaRoom, aRoom->area->id,
            aRoom->mapmarker.filename().c_str());

        if(file_exists(filename)) {
            xmlDocPtr   xmlDoc;
            xmlNodePtr  rootNode;

            if((xmlDoc = xml::loadFile(filename, "AreaRoom")) == nullptr)
                merror("Unable to read arearoom file", FATAL);

            rootNode = xmlDocGetRootElement(xmlDoc);

            Area *area = aRoom->area;
            aRoom->reset();
            aRoom->area = area;
            aRoom->load(rootNode);
            return(true);
        }
    }
    return(false);
}

UniqueRoom* Server::reloadRoom(CatRef cr) {
    UniqueRoom  *room=nullptr, *oldRoom=nullptr;

    bstring str = cr.str();
    oldRoom = roomCache.fetch(cr);
    if(!oldRoom)
    	return nullptr;

    if(!loadRoomFromFile(cr, &room))
        return(nullptr);

    // Move any current players & monsters into the new room
    for(Player* ply : oldRoom->players) {
        room->players.insert(ply);
        ply->setParent(room);
    }
    oldRoom->players.clear();

    if(room->monsters.empty()) {
        for(Monster* mons : oldRoom->monsters) {
            room->monsters.insert(mons);
            mons->setParent(room);
        }
        oldRoom->monsters.clear();
    }
    if(room->objects.empty()) {
        for(Object* obj : oldRoom->objects) {
            room->objects.insert(obj);
            obj->setParent(room);
        }
        oldRoom->objects.clear();
    }

    roomCache.insert(cr, &room);

    room->registerMo();

    return(room);
}

//*********************************************************************
//                      resaveRoom
//*********************************************************************
// This function saves an already-loaded room back to memory without
// altering its position on the queue.

int saveRoomToFile(UniqueRoom* pRoom, int permOnly);

int Server::resaveRoom(CatRef cr) {
    UniqueRoom *room = roomCache.fetch(cr);
    if(room)
        room->saveToFile(ALLITEMS);
    return(0);
}


int Server::saveStorage(UniqueRoom* uRoom) {
    if(uRoom->flagIsSet(R_SHOP))
        saveStorage(shopStorageRoom(uRoom));
    return(saveStorage(uRoom->info));
}
int Server::saveStorage(CatRef cr) {

    UniqueRoom *room = roomCache.fetch(cr);
    if(room) {
        if(room->saveToFile(ALLITEMS) < 0) {
            return(-1);
        }
    }

    return(0);
}
