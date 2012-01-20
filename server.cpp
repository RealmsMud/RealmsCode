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
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

// Mud Includes
#include "mud.h"
#include "serverTimer.h"
#include "login.h"
#include "version.h"
#include "factions.h"
#include "web.h"
#include "calendar.h"
#include "dm.h"

// C Includes
#include <netinet/in.h> // Needs: htons, htonl, INADDR_ANY, sockaddr_in
#include <sys/socket.h> // Needs: bind, std::listen, socket, AF_INET
#include <fcntl.h>		// Needs: fnctl
#include <netdb.h>		// Needs: gethostbyaddr
#include <sys/wait.h>	// Needs: WNOHANG, wait3
#include <errno.h>
#include <stdlib.h>
#include <sys/resource.h>

// C++ Includes
#include <iostream>
#include <sstream>
#include <iomanip>
#include <locale>


// External declarations
extern int Numplayers;
extern long last_time_update;
extern long last_weather_update;

// Function prototypes
char *inetname(struct in_addr in );
void initSpellList(); // TODO: Move spelling stuff into server

// Global server pointer
Server* gServer = NULL;

// Static initialization
Server* Server::myInstance = NULL;

// Custom comparison operator to sort by the numeric id instead of standard string comparison
bool idComp::operator() (const bstring& lhs, const bstring& rhs) const {
	std::stringstream strL(lhs);
	std::stringstream strR(rhs);

	char charL, charR;
	long longL, longR;

	strL >> charL >> longL;
	strR >> charR >> longR;

	if(charL == charR) {
		return(longL < longR);
	} else {
		return(charL < charR);
	}
	return(true);
}
//--------------------------------------------------------------------
// Constructors, Destructors, etc

//********************************************************************
//						Server
//********************************************************************

Server::Server() {
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
//						~Server
//********************************************************************

Server::~Server() {
	std::cout << "Server destructor called!\n";
	if(running) {
		// Do shutdown here
	}
	effectsIndex.clear();
	cleanUpPython();

#ifdef SQL_LOGGER
    cleanUpSql();
#endif // SQL_LOGGER

}

//********************************************************************
//						init
//********************************************************************

bool Server::init() {

	std::cout << "Initializing Server.\n";

	std::cout << "Setting RLIMIT...";
	struct rlimit lim;
	lim.rlim_cur = RLIM_INFINITY;
	lim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &lim);
	std::cout << "done\n";

	std::cout << "Installing signal handlers.";
	installSignalHandlers();
	std::cout << "done." << std::endl;

	std::cout << "Installing unique IDs...";
	loadIds();
	std::cout << "done." << std::endl;

#ifndef __CYGWIN__
	std::cout << "Installing custom printf handlers...";
	if(installPrintfHandlers() == 0)
	    std::cout << "done." << std::endl;
	else
	    std::cout << "failed." << std::endl;
#endif

	gConfig->load();
	gConfig->startFlashPolicy();
	gConfig->setLotteryRunTime();

	Port = gConfig->getPortNum();
	Tablesize = getdtablesize();


	std::cout << "Initializing Spelling...";
	init_spelling();
	std::cout << "done." << std::endl;

	initWebInterface();

	std::cout <<  "Initializing Spelling List...";
	initSpellList();
	std::cout << "done." << std::endl;

	// Python
	std::cout <<  "Initializing Python...";
	initPython();

#ifdef SQL_LOGGER
	std::cout <<  "Initializing SQL Logger...";
    if(initSql())
        std::cout << "done." << std::endl;
    else
        std::cout << "failed." << std::endl;
#endif // SQL_LOGGER

	umask(000);
	srand(getpid() + time(0));
	if(rebooting) {
		printf("Doing a reboot.\n");
		finishReboot();
	} else {
		addListenPort(Port);
		char filename[80];
		snprintf(filename, 80, "%s/reboot.xml", Path::Config);
		unlink(filename);
	}
	return(true);
}

//********************************************************************
//						installSignalHandlers
//********************************************************************

void Server::installSignalHandlers() {
	signal(SIGABRT, crash);	// abnormal termination triggered by abort call
	printf(".");
	signal(SIGFPE, crash);	// floating point exception
	printf(".");
	signal(SIGSEGV, crash);	// segment violation
	printf(".");
	signal(SIGPIPE, SIG_IGN);
	printf(".");
	signal(SIGTERM, SIG_IGN);
	printf(".");
	signal(SIGCHLD, child_died);
	printf(".");
	signal(SIGHUP, quick_shutdown);
	printf(".");
	signal(SIGINT, shutdown_now);
	printf(".");
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
	if(myInstance == NULL)
		myInstance = new Server;
	return(myInstance);
}
//********************************************************************
// Destroy Instance - Destroy the static instance
//********************************************************************
void Server::destroyInstance() {
	if(myInstance != NULL)
		delete myInstance;
	myInstance = NULL;
}

// End - Instance Functions
//--------------------------------------------------------------------


//********************************************************************
//						run
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

		poll();

		checkNew();

		processInput();

		processCommands();
		// Update game here
		updateGame();

		processMsdp();

		processOutput();

		cleanUp();
		// Temp
		pulse++;

		checkWebInterface();

		timer.end(); // End the timer
		timer.sleep();
	}

	return(0);
}

//********************************************************************
//						addListenPort
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
		printf("Error with socket\n");
		return(-1);
	}

	if(setsockopt(control, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
		printf("Error with setSockOpt\n");
		return(-1);
	}

	if(bind(control, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		close(control);
		printf("Unable to bind to port(%d)\n", port);
		return(-1);
	}

	if(nonBlock(control) != 0) {
		printf("Error with nonBlock\n");
		return(-1);
	}

	if(listen(control, 100) != 0) {
		printf("Error with listen\n");
		return(-1);
	}

	std::cout << "Mud is now listening on port " << port << std::endl;

	// TODO: Leaky, make sure to erase these when the server shuts down
	controlSocks.push_back(controlSock(port, control));
	running = true;

	return(0);
}

//********************************************************************
//						poll
//********************************************************************

int Server::poll() {
	if(!this || controlSocks.empty()) {
		printf("Not bound to any ports, nothing to poll.\n");
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
//						checkNew
//********************************************************************

int Server::checkNew() {
	for(controlSock & cs : controlSocks) {
		if(FD_ISSET(cs.control, &inSet)) { // We have a new connection waiting
			std::cout << "Got a new connection on port " << cs.port << ", control sock " << cs.control << std::endl;
			handleNewConnection(cs);
		}
	}
	return(0);
}

//********************************************************************
//						handleNewConnection
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
//						processInput
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
			printf("Exception found\n");
			continue;
		}
		// Try to read something
		if(FD_ISSET(sock->getFd(), &inSet)) {
			if(sock->processInput() != 0) {
				FD_CLR(sock->getFd(), &outSet);
				std::cout << "Error reading from socket " << sock->getFd() << std::endl;
				sock->setState(CON_DISCONNECTING);
				continue;
			}
		}
	}
	return(0);
}

//********************************************************************
//						processCommands
//********************************************************************

int Server::processCommands() {
	for(Socket * sock : sockets) {
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
//						processOutput
//********************************************************************

int Server::processOutput() {
	for(Socket * sock : sockets) {
		if(FD_ISSET(sock->getFd(), &outSet) && sock->hasOutput()) {
			sock->flush();
		}
	}
	return(0);
}

//********************************************************************
//						deleteSocket
//********************************************************************
// This should force a socket off right away

int Server::deleteSocket(Socket* sock) {
	sock->setState(CON_DISCONNECTING);
	cleanUp();
	return(0);
}

//********************************************************************
//						disconnectAll
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
//						cleanUp
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
//						getDnsCacheString
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
//						getDnsCache
//********************************************************************

bool Server::getDnsCache(bstring &ip, bstring &hostName) {
	for(dnsCache & dns : cachedDns) {
		if(dns.ip == ip) {
			// Got a match
			hostName = dns.hostName;
			printf("DNS: Found %s in dns cache\n", ip.c_str());
			return(true);
		}
	}
	return(false);
}

//********************************************************************
//						pruneDns
//********************************************************************

void Server::pruneDns() {
	long fifteenDays = 60*60*24*15;
	long currentTime = time(0);
	std::list<dnsCache>::iterator it;
	std::list<dnsCache>::iterator oldIt;

	std::cout << "Pruning DNS\n";
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
//						pulseTicks
//********************************************************************

void Server::pulseTicks(long t) {
	std::list<Socket*>::iterator it;
	Player* player=0;

	for(it = sockets.begin() ; it != sockets.end() ; ) {
		player = (*it)->getPlayer();
		if(player) {
			player->pulseTick(t);
			if(!player) {
				it = sockets.erase(it);
				continue;
			}
			if(player->isPlaying()) {
				player->pulseSong(t);
			}
		}
		it++;
	}
}



//*********************************************************************
//						updateUsers
//*********************************************************************

void Server::updateUsers(long t) {
	int tout = 300;
	lastUserUpdate = t;

	Player* player=0;
	for(Socket * sock : sockets) {
		player = sock->getPlayer();

		if(player) {
			if(player->isDm()) tout = t;
			else if(player->isStaff()) tout = 1200;
			else tout = 600;
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
//						update_random
//********************************************************************
// This function checks all player-occupied rooms to see if random monsters
// have entered them.  If it is determined that random monster should enter
// a room, it is loaded and items it is carrying will be loaded with it.

void Server::updateRandom(long t) {
	Monster* monster=0;
	BaseRoom* room=0;
	UniqueRoom*	uRoom=0;
	AreaRoom* aRoom=0;
	WanderInfo* wander=0;
	CatRef	cr;
	int 	num=0, l=0;
	std::map<bstring, bool> check;
	//Object      *object;
	//int		k, numout=0, alnum=0, x=0;

	lastRandomUpdate = t;

	Player* player;
	for(Socket * sock : sockets) {
		player = sock->getPlayer();

		if(!player || !player->getRoom())
			continue;
		uRoom = player->parent_rom;
		aRoom = player->area_room;
		room = player->getRoom();

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
//						update_active
//*********************************************************************
// This function updates the activities of all monsters who are currently
// active (ie. monsters on the active list). Usually this is reserved
// for monsters in rooms that are occupied by players.

void Server::updateActive(long t) {
	Player	*ply=0;
	Creature *target=0;
	Monster	*monster=0;
	BaseRoom* room=0;
	ctag	*imp=0, *ap=0;

	long	tt = gConfig->currentHour();
	int		timetowander=0, immort=0;
	bool	print=false;

	lastActiveUpdate = t;

	if(activeList.empty())
		return;
//	if(!(cp = first_active))
//		return;

	MonsterList::iterator it = activeList.begin();
	while(it != activeList.end()) {
		// Increment the iterator in case this monster dies during the update and is removed from the active list
		monster = (*it++);

		// Better be a monster to be on the active list
		ASSERTLOG(monster);

		if(!monster->parent_rom && !monster->area_room) {
			broadcast(isStaff, "^y%s without a parent/area room on the active list. Info: %s. Deleting.",
				monster->name, monster->info.str().c_str());
			monster->deleteFromRoom();
			gServer->delActive(monster);
			free_crt(monster);
			it = activeList.begin();
			continue;
		}


		room = monster->getRoom();

		// Reset's mob's smack-talking broadcasts at 7am every day
		if(tt == 7 && ((t - last_time_update) / 2) == 0)
			monster->daily[DL_BROAD].cur = 20;

		if(	(monster->flagIsSet(M_NIGHT_ONLY) && isDay()) ||
			(monster->flagIsSet(M_DAY_ONLY) && !isDay()))
		{

			imp = room->first_ply;
			while(imp) {
				if(imp->crt->isStaff()) {
					immort = 1;
					break;
				}
				imp = imp->next_tag;
			}
			if(!immort) {
				timetowander=1;
				broadcast(NULL, monster->getRoom(), "%M wanders slowly away.", monster);
				monster->deleteFromRoom();
				gServer->delActive(monster);
				free_crt(monster);
				it = activeList.begin();
				continue;
			}

		}

		// fast wanderers and pets always stay active
		if(	!room->first_ply &&
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
//			cp = cp->next_tag;
			continue;
		}


		if(monster->flagIsSet(M_KILL_PERMS)) {
			ap = room->first_mon;
			while(ap) {
				if(ap->crt == monster) {
					ap = ap->next_tag;
					continue;
				}
				if(	ap->crt->flagIsSet(M_PERMENANT_MONSTER) &&
					!monster->willAssist(ap->crt->getMonster()) &&
					!monster->isEnemy(ap->crt)
				)
					monster->addEnemy(ap->crt);
				ap = ap->next_tag;
			}
		}

		if(monster->flagIsSet(M_KILL_NON_ASSIST_MOBS)) {
			ap = room->first_mon;
			while(ap) {
				if(ap->crt == monster) {
					ap = ap->next_tag;
					continue;
				}
				if(	!monster->willAssist(ap->crt->getMonster()) &&
					!ap->crt->flagIsSet(M_PERMENANT_MONSTER) &&
					!monster->isEnemy(ap->crt)
				)
					monster->addEnemy(ap->crt);
				ap = ap->next_tag;
			}
		}

		monster->checkAssist();
		monster->pulseTick(t);
		monster->checkSpellWearoff();

		if(monster->isPoisoned() && !monster->immuneToPoison()) {
			if(	monster->spellIsKnown(S_CURE_POISON) &&
				monster->mp.getCur() >=6 &&
				(mrand(1,100) < (30+monster->intelligence.getCur()/10)))
			{
				broadcast(NULL, monster->getRoom(), "%M casts a curepoison spell on %sself.", monster, monster->himHer());
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
		if(	monster->isPet() &&
			(t > LT(monster, LT_INVOKE) || t > LT(monster, LT_ANIMATE)))
		{
			if(monster->isUndead())
				broadcast(NULL, room, "%1M wanders away.", monster);
			else
				broadcast(NULL, room, "%1M fades away.", monster);

			monster->die(monster->following);
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
		if(	monster->flagIsSet(M_DEATH_SCREAM) &&
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
		if(	monster->hasEnemy() && !timetowander && monster->updateCombat()) {
    		it = activeList.begin();
    		continue;
		}

		if(	!monster->flagIsSet(M_AGGRESSIVE) &&
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

			print = (	!monster->flagIsSet(M_HIDDEN) &&
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
//						add_active
//*********************************************************************
// This function adds a monster to the active-monster list. A pointer
// to the monster is passed in the first parameter.

void Server::addActive(Monster* monster) {

	if(!monster) {
		loga("add_active: tried to activate a NULL crt!\n");
		return;
	}
	if(isActive(monster))
		return;
	// try and guess if the creature is valid
	{
		char *p = monster->name;
		while(*p) {
			ASSERTLOG(isPrintable((unsigned char)*p));
			p++;
		}
		ASSERTLOG(p - monster->name);
	}

	monster->validateId();

	activeList.push_back(monster);
}

//*********************************************************************
//						del_active
//*********************************************************************
// This function removes a monster from the active-monster list. The
// parameter contains a pointer to the monster which is to be removed

void Server::delActive(Monster* monster) {
	if(!this)
		return;

	if(activeList.empty()) {
		std::cerr << "Attempting to delete '" << monster->getName() << "' from active list with an empty active list." << std::endl;
		broadcast(isStaff, "^yAttempting to delete %s from active list with an empty active list.", monster->name);
		return;
	}

	MonsterList::iterator it = std::find(activeList.begin(), activeList.end(), monster);
	if(it == activeList.end()) {
		std::cerr << "Attempting to delete '" << monster->getName() << "' from active list but could not find them on the list." << std::endl;
		broadcast(isStaff, "^yAttempting to delete %s from active list but could not find them on the list.", monster->name);
		return;
	}


	activeList.remove(monster);
}


//*********************************************************************
//						isActive
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
//						childDied
//********************************************************************

void Server::childDied() {
	Deadchildren++;
}

//********************************************************************
//						getDeadChildren
//********************************************************************

int Server::getDeadChildren() const {
	return(Deadchildren);
}

//********************************************************************
//						reapChildren
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
							printf("DNS ReapChildren: Error would block\n");
						else
							printf("DNS ReapChildren: Error\n");
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
					printf("Reaped DNS child (%d-%s).\n", pid, tmpBuf);
				} else if((*it).type == CHILD_LISTER) {
					printf("Reaping LISTER child (%d-%s)\n", pid, (*it).extra.c_str());
					processListOutput(*it);
					// Don't forget to close the pipe!
					close((*it).fd);
				} else if((*it).type == CHILD_SWAP_FINISH) {
					printf("Reaping MoveRoom Finish child (%d-%s)\n", pid, (*it).extra.c_str());
					c = *it;
					found = true;
				} else if((*it).type == CHILD_PRINT) {
					//broadcast(isDm, "Reaping Print Child (%d-%s)",pid, (*it).extra.c_str());
					printf("Reaping Print child (%d-%s)\n", pid, (*it).extra.c_str());
					c = *it;
					found = true;
				} else {
					printf("ReapChildren: Unknown child type %d\n", (*it).type);
				}
//				// Child was processed, reduce number of children
//				Deadchildren--;
//				printf("Reaped Child: %d dead children left.\n", Deadchildren);
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
//						processListOutput
//********************************************************************

int Server::processListOutput(childProcess &lister) {
	bool found = false;
	std::list<Socket*>::iterator sIt;
	Socket *sock;
	for(sIt = sockets.begin() ; sIt != sockets.end() ; ++sIt ) {
		sock = *sIt;
		if(sock->getPlayer() && lister.extra == sock->getPlayer()->name) {
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
//						processChildren
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
			printf("processChildren: Unknown child type %d\n", child.type);
		}
	}
	return(1);
}


//********************************************************************
//						startDnsLookup
//********************************************************************

int Server::startDnsLookup(Socket* sock, struct sockaddr_in addr) {
	int fds[2];
	int pid;

	// We're going to make a pipe here.
	// The child process will write the
	if(pipe(fds) == -1) {
		printf("DNS: Error with pipe!\n");
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

			if(he == NULL) {
				switch(h_errno) {
				case HOST_NOT_FOUND:
					printf("DNS Error: Host not found for %s.\n", sock->getIp().c_str());
					tries = -1;
					break;
				case NO_RECOVERY:
					printf("DNS Error: Unrecoverable error for %s.\n", sock->getIp().c_str());
					tries = -1;
					break;
				case TRY_AGAIN:
					printf("DNS Error: Try again for %s.\n", sock->getIp().c_str());
					tries++;
					break;
				}
			} else
				break;
		}
		printf("DNS: Resolver finished for %s(%s).\n", sock->getIp().c_str(), he ? he->h_name : sock->getIp().c_str());
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
		std::cout << "Watching Child DNS Resolver for(" << sock->getIp() << ") running with pid " << pid
				  << " reading from fd " << fds[0] << std::endl;
		// Let the server know we're monitoring this child process
		addChild(pid, CHILD_DNS_RESOLVER, fds[0], sock->getIp());
	}
	return(0);
}

//********************************************************************
//						addCache
//********************************************************************

void Server::addCache(bstring ip, bstring hostName, time_t t) {
	if(t == -1)
		t = time(0);
	cachedDns.push_back(dnsCache(ip, hostName, t));
}

//********************************************************************
//						addChild
//********************************************************************

void Server::addChild(int pid, childType pType, int pFd, bstring pExtra) {
	printf("Adding pid %d as child type %d, watching fd %d\n", pid, pType, pFd);
	children.push_back(childProcess(pid, pType, pFd, pExtra));
}

// End - Children Control
//--------------------------------------------------------------------

//********************************************************************
//						saveDnsCache
//********************************************************************

void Server::saveDnsCache() {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	xmlNodePtr		curNode;
	char			filename[80];

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "DnsCache", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	std::list<dnsCache>::iterator it;
	for( it = cachedDns.begin(); it != cachedDns.end() ; it++) {
		curNode = xmlNewChild(rootNode, NULL, BAD_CAST"Dns", NULL);
		xml::newStringChild(curNode, "Ip", (*it).ip.c_str());
		xml::newStringChild(curNode, "HostName", (*it).hostName.c_str());
		xml::newNumChild(curNode, "Time", (long)(*it).time);
	}

	sprintf(filename, "%s/dns.xml", Path::Config);
	xml::saveFile(filename, xmlDoc);
	xmlFreeDoc(xmlDoc);
}

//********************************************************************
//						loadDnsCache
//********************************************************************

void Server::loadDnsCache() {
	xmlDocPtr xmlDoc;
	xmlNodePtr curNode, childNode;

	char filename[80];
	snprintf(filename, 80, "%s/dns.xml", Path::Config);
	xmlDoc = xml::loadFile(filename, "DnsCache");

	if(xmlDoc == NULL)
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
	while(curNode != NULL) {
		if(NODE_NAME(curNode, "Dns")) {
			bstring ip, hostname;
			long time=0;
			childNode = curNode->children;
			while(childNode != NULL) {
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
//						resetShipsFile
//********************************************************************

void Config::resetShipsFile() {
	// copying ships.midnight.xml to ships.xml
	char	sfile1[80], sfile2[80], command[255];
	sprintf(sfile1, "%s/ships.midnight.xml", Path::Game);
	sprintf(sfile2, "%s/ships.xml", Path::PlayerData);

	sprintf(command, "cp %s %s", sfile1, sfile2);
	system(command);
}

//********************************************************************
//						startReboot
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
			players[player->name] = 0;
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

	execl(path, path, "-r", port, (char *)NULL);

	char filename[80];
	snprintf(filename, 80, "%s/config.xml", Path::Config);
	unlink(filename);

	merror("dmReboot failed!!!", FATAL);
	return(0);
}

//********************************************************************
//						saveRebootFile
//********************************************************************

bool Server::saveRebootFile(bool resetShips) {
	xmlDocPtr	xmlDoc;
	xmlNodePtr	rootNode;
	xmlNodePtr		serverNode;
	xmlNodePtr		curNode;

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Reboot", NULL);
	xmlDocSetRootElement(xmlDoc, rootNode);

	// Save server information
	serverNode = xmlNewChild(rootNode, NULL, BAD_CAST"Server", NULL);

	// Dump control socket information
	std::list<controlSock>::const_iterator it1;
	for( it1 = controlSocks.begin(); it1 != controlSocks.end(); it1++ ) {
		curNode = xmlNewChild(serverNode, NULL, BAD_CAST"ControlSock", NULL);
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
			curNode = xmlNewChild(rootNode, NULL, BAD_CAST"Player", NULL);
			xml::newStringChild(curNode, "Name", player->name);
			xml::newNumChild(curNode, "Fd", sock->getFd());
			xml::newStringChild(curNode, "Ip", sock->getIp().c_str());
			xml::newStringChild(curNode, "HostName", sock->getHostname().c_str());
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
//						finishReboot
//********************************************************************

int Server::finishReboot() {
	xmlDocPtr doc;
	xmlNodePtr curNode;
	xmlNodePtr childNode;
	bool resetShips = false;
	std::cout << "Running finishReboot()" << std::endl;

	// We are rebooting
	rebooting = true;

	char filename[80];
	snprintf(filename, 80, "%s/reboot.xml", Path::Config);
	// build an XML tree from a the file
	doc = xml::loadFile(filename, "Reboot");
	unlink(filename);

	if(doc == NULL) {
		printf("Unable to load reboot file\n");
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

	while(curNode != NULL) {
		if(NODE_NAME(curNode, "Server")) {
			childNode = curNode->children;
			while(childNode != NULL) {
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
			while(childNode != NULL) {
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

	StartTime = time(0);
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
//						findPlayer
//********************************************************************

Player* Server::findPlayer(bstring name) {
	std::map<bstring, Player*>::const_iterator it = players.find(name);

	if(it != players.end())
		return((*it).second);
	return(0);

}

//*********************************************************************
//						clearPlayer
//*********************************************************************
// This will NOT free up the player, it will just remove them from the list

bool Server::clearPlayer(bstring name) {
	players.erase(name);
	return(true);
}
bool Server::clearPlayer(Player* player) {
	ASSERTLOG(player);
	players.erase(player->getName());
	unRegisterMudObject(player);
	return(true);
}

//*********************************************************************
//						addPlayer
//*********************************************************************

bool Server::addPlayer(Player* player) {
	ASSERTLOG(player);
	player->validateId();
	players[player->name] = player;
	player->getSock()->addToPlayerList();
	registerMudObject(player);
	return(true);
}

//*********************************************************************
//						checkDuplicateName
//*********************************************************************

bool Server::checkDuplicateName(Socket* sock, bool dis) {
	for(Socket *s : sockets) {
		if(sock != s && s->getPlayer() && !strcmp(s->getPlayer()->name, sock->getPlayer()->name)) {
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
//						checkDouble
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
			sock->getPlayer()->name);
		s->disconnect();
		return(false);
	}
	return(false);
}

//*********************************************************************
//						sendCrash
//*********************************************************************

void Server::sendCrash() {
	char filename[80];
	snprintf(filename, 80, "%s/crash.txt", Path::Config);

	for(Socket *sock : sockets) {
		viewLoginFile(sock, filename);
	}
}


//*********************************************************************
//						getTimeZone
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

//*********************************************************************
//						getNumPlayers
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

bool Server::registerMudObject(MudObject* toRegister) {
	ASSERT(toRegister != NULL);

	if(toRegister->getId().equals("-1"))
		return(false);

	if(registeredIds.find(toRegister->getId()) != registeredIds.end()) {
		std::ostringstream oStr;
		oStr << "ERROR: ID: " << toRegister->getId() << " is already registered!";
		broadcast(isDm, "%s", oStr.str().c_str());
		std::cout << oStr.str() << std::endl;
		return(false);
	}
	registeredIds.insert(IdMap::value_type(toRegister->getId(), toRegister));
	//registeredIds[toRegister->getId()] = toRegister;
//	std::cout << "Registered: " << toRegister->getId() << " - " << toRegister->getName() << std::endl;
	return(true);
}

bool Server::unRegisterMudObject(MudObject* toUnRegister) {
	ASSERT(toUnRegister != NULL);

	if(toUnRegister->getId().equals("-1"))
		return(false);

	IdMap::iterator it = registeredIds.find(toUnRegister->getId());

	if(it == registeredIds.end()) {
		std::ostringstream oStr;
		oStr << "ERROR: ID: " << toUnRegister->getId() << " is not registered!";
		broadcast(isDm, "%s", oStr.str().c_str());
		std::cout << oStr.str() << std::endl;
		return(false);
	}
	registeredIds.erase(it);
//	std::cout << "Unregistered: " << toUnRegister->getId() << " - " << toUnRegister->getName() << std::endl;
	return(true);
}

Creature* Server::lookupCrtId(const bstring& toLookup) {
    IdMap::iterator it = registeredIds.find(toLookup);

    if(it == registeredIds.end())
        return(NULL);
    else
        return(((*it).second)->getCreature());

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

	if(xmlDoc == NULL)
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
	while(curNode != NULL) {
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
	xmlDocPtr		xmlDoc;
	xmlNodePtr		rootNode;
	char			filename[80];

	if(idDirty == false)
		return;

	xmlDoc = xmlNewDoc(BAD_CAST "1.0");
	rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "Ids", NULL);
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
	    targetStr = colorize(target->getName(), false, player);
	    Object* oTarget = target->getObject();
	    if(oTarget) {
	        targetStr += "(" + oTarget->info.str() + ")";
	        if(dir == GOLD_IN) {
	            source = oTarget->droppedBy.str();
	        }
	    }
	}
	bstring room = "";
	if(player->getRoom()) {
	    if(player->getRoom()->getUniqueRoom()) {
	        room = bstring(player->getRoom()->getName()) + "(" + player->getRoom()->getUniqueRoom()->info.str() + ")";
	    } else if (player->getRoom()->getAreaRoom()) {
	        room = bstring(player->getRoom()->getName()) + "(" + player->getRoom()->getAreaRoom()->mapmarker.str() + ")";
	    }
	}
	// logType
	bstring direction = (dir == GOLD_IN ? "In" : "Out");
	std::cout << direction << ": P:" << pName << " I:" << pId << " T: " << targetStr << " S:" << source << " R: " << room << " Type:" << logType << " G:" << amt.get(GOLD) << std::endl;

#ifdef SQL_LOGGER
	logGoldSql(pName, pId, targetStr, source, room, logType, amt.get(GOLD), direction);
#endif // SQL_LOGGER
}
