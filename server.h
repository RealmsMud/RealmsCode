/*
 * server.h
 *   Server class
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

#ifndef SERVER_H_
#define SERVER_H_

#ifdef SQL_LOGGER

namespace odbc {
    class Connection;
}

#endif //SQL_LOGGER

// C Includes
#include "pythonHandler.h"
#include <netinet/in.h> // Needs: htons, htonl, INADDR_ANY, sockaddr_in

class Player;
class Group;
class PythonHandler;
class WebInterface;
class Socket;
class cmd;
class ReportedMsdpVariable;
class MsdpVariable;

// Custom comparison operator to sort by the numeric id instead of standard string comparison
struct idComp : public std::binary_function<const bstring&, const bstring&, bool> {
  bool operator() (const bstring& lhs, const bstring& rhs) const;
};

//// Forward Declaration of PyObject
//struct _object;
//typedef _object PyObject;

enum childType {
	CHILD_START,

	CHILD_DNS_RESOLVER,
	CHILD_LISTER,
	CHILD_DEMOGRAPHICS,
	CHILD_SWAP_FIND,
	CHILD_SWAP_FINISH,
	CHILD_PRINT,

	CHILD_END
};
enum GoldLog {
    GOLD_IN,
    GOLD_OUT
};


#include "asynch.h"

typedef std::map<bstring, MudObject*,idComp> IdMap;
typedef std::list<Group*> GroupList;
typedef std::list<Socket*> SocketList;
typedef std::map<bstring, Player*> PlayerMap;
class Server
{

// **************
// Static Methods
public:
	static Server* getInstance();
	static void destroyInstance();

	~Server();
// Instance
private:
	static Server* myInstance;
protected:
	Server();


// *****************
// Public Structures
public:
	struct controlSock {
		int port;
		int control;
		controlSock(int port, int control) {
			this->port = port;
			this->control = control;
		}
	};
	struct childProcess {
		int pid;
		childType type;
		int fd; // Fd if any we should watch
		bstring extra;
		childProcess() {
			pid = 0;
			fd = -1;
			extra = "";
		}
		childProcess(int p, childType t, int f = -1, bstring e = "") {
			pid = p;
			type = t;
			fd = f;
			extra = e;
		}
	};
	struct dnsCache {
		bstring ip;
		bstring hostName;
		time_t time;
		dnsCache(bstring pIp, bstring pHostName, time_t pTime) {
			ip = pIp;
			hostName = pHostName;
			time = pTime;
		}
		bool operator==(const dnsCache& o) {
			return(ip == o.ip && hostName == o.hostName);
		}
	};

	// Temp public
public:
	PlayerMap players; // Map of all players
	SocketList sockets; // List of all connected sockets
	void handlePythonError();
	
// ******************
// Internal Variables
private:

	std::list<DelayedAction> delayedActionQueue;
	
	PythonHandler* pythonHandler;

	std::list<BaseRoom*> effectsIndex;

	fd_set inSet;
	fd_set outSet;
	fd_set excSet;

	bool running; // True while the game is up and bound to a port
	long pulse; // Current pulse

	bool rebooting;
	bool GDB;
	bool valgrind;

	// List of Ids
	IdMap registeredIds;

	// List of groups
	GroupList groups;

	// Maximum Ids
	long maxPlayerId;
	long maxMonsterId;
	long maxObjectId;
	bool idDirty;

	std::list<childProcess> children; // List of child processes
	std::list<controlSock> controlSocks; // List of control fds
	std::list<dnsCache> cachedDns; // Cache of DNS lookups
	WebInterface* webInterface;

	// Game Updates
	CreatureList activeList; // The new active list
	ctag *first_active; // The active list

	long lastDnsPrune;
	long lastUserUpdate;
	long lastRoomPulseUpdate;
	long lastRandomUpdate;
	long lastActiveUpdate;

	int Deadchildren;


// ****************
// Internal Methods
private:
	// Python
	bool initPython();
	bool cleanUpPython();

	int getNumSockets(void); // Get number of sockets in the sockets list

	// Game & Socket methods
	int handleNewConnection(controlSock& control);
	int poll(void); // Poll all descriptors for input
	int checkNew(void); // Accept new connections
	int processInput(void); // Process input from users
	int processCommands(void); // Process commands from users
	int processChildren(void);
	int processListOutput(childProcess &lister);

	// Child processes
	int reapChildren(void); // Clean up after any dead children

	// Reboot
	bool saveRebootFile(bool resetShips = false);

	// Updates
	void updateGame(void);
	void processMsdp(void);
	void pulseTicks(long t);
	void pulseCreatureEffects(long t);
	void pulseRoomEffects(long t);
	void updateUsers(long t);
	void updateRandom(long t);
	void updateActive(long t);

	// DNS
	void addCache(bstring ip, bstring hostName, time_t t = -1);
	void saveDnsCache();
	void loadDnsCache();
	void pruneDns();

	int installPrintfHandlers();
	void installSignalHandlers();


	// Delayed Actions
protected:
	void parseDelayedActions(long t);


#ifdef SQL_LOGGER

protected:
	odbc::Connection* conn;
	bool connActive;
    void cleanUpSql();
    bool initSql();
    bool logGoldSql(bstring& pName, bstring& pId, bstring& targetStr, bstring& source, bstring& room,
                    bstring& logType, unsigned long amt, bstring& direction);

public:
    bool getConnStatus();
    int getConnTimeout();
#endif // SQL_LOGGER


public:

	void logGold(GoldLog dir, Player* player, Money amt, MudObject* target, bstring logType);

    bool registerGroup(Group* toRegister);
    bool unRegisterGroup(Group* toUnRegister);
    bstring getGroupList();

	bool registerMudObject(MudObject* toRegister);
	bool unRegisterMudObject(MudObject* toUnRegister);
	bstring getRegisteredList();
	Creature* lookupCrtId(const bstring& toLookup);

	void loadIds();
	void saveIds();

	bstring getNextMonsterId();
	bstring getNextPlayerId();
	bstring getNextObjectId();

	long getMaxMonsterId();
	long getMaxPlayerId();
	long getMaxObjectId();

	bool removeDelayedActions(MudObject* target, bool interruptOnly=false);
	void addDelayedAction(void (*callback)(DelayedActionFn), MudObject* target, cmd* cmnd, DelayedActionType type, long howLong, bool canInterrupt=true);
	void addDelayedScript(void (*callback)(DelayedActionFn), MudObject* target, bstring script, long howLong, bool canInterrupt=true);
	bool hasAction(const MudObject* target, DelayedActionType type);
	bstring delayedActionStrings(const MudObject* target);

	void weather(WeatherString w);
	void updateWeather(long t);
// *******************************
// Public methods for server class
public:

	// Child processes
	void addChild(int pid, childType pType, int pFd = -1, bstring pExtra = "");

	// Python
	bool runPython(const bstring& pyScript, object& dictionary);
	bool runPython(const bstring& pyScript, bstring args = "", MudObject *actor = NULL, MudObject *target = NULL);
	bool runPython(const bstring& pyScript, bstring args, Socket *sock, Player *actor, MsdpVariable* msdpVar = NULL);

	// Setup
	bool init();	// Setup the server

	void setGDB();
	void setRebooting();
	void setValgrind();

	int run(void); // Run the server
	int addListenPort(int); // Add a new port to listen to

	// Status
	void sendCrash();

	bool isRebooting();
	bool isGDB();
	bool isValgrind();

	// Reboot
	bool startReboot(bool resetShips = false);
	int finishReboot(void); // Bring the mud back up from a reboot

	// DNS
	bstring getDnsCacheString();
	bool getDnsCache(bstring &ip, bstring &hostName);
	int startDnsLookup(Socket* sock, sockaddr_in pAddr); // Start dns lookup on a socket

	// Players
	bool addPlayer(Player* player);
	bool clearPlayer(Player* player);
	Player* findPlayer(bstring name);
	bool clearPlayer(bstring name);
	int getNumPlayers();

	// Sockets
	int deleteSocket(Socket* sock);
	void disconnectAll(void);
	int cleanUp(void); // Kick out any disconnectors and other general cleanup
	int processOutput(void); // Send any buffered output

	// Web Interface
	bool initWebInterface();
	bool checkWebInterface();
	void recreateFifos();

	// Active list functions
	const ctag* getFirstActive();
	void addActive(Monster* monster);
	void delActive(Monster* monster);
	bool isActive(Monster* monster);

	// Child Processes
	void childDied();
	int getDeadChildren() const;
	int runList(Socket* sock, cmd* cmnd);
	bstring simpleChildRead(Server::childProcess &child);

	// Swap functions - use children
	bstring swapName();
	void finishSwap(Player* player, bool online, CatRef origin, CatRef target);
	bool swap(Swap s);
	void endSwap();
	void swapInfo(const Player* player);

	// Queries
	bool checkDuplicateName(Socket* sock, bool dis);
	bool checkDouble(Socket* sock);

	// Bans
	void checkBans();

	// Broadcasts
	bstring getTimeZone();

	// Effects
	void addEffectsIndex(BaseRoom* room);
	void removeEffectsIndex(BaseRoom* room);
	void removeEffectsOwner(const Creature* owner);
	void showEffectsIndex(const Player* player);
};

#endif /*SERVER_H_*/
