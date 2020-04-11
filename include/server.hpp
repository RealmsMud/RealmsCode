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
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
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

#include <list>
#include <map>
#include <vector>

// C Includes
#include <netinet/in.h> // Needs: htons, htonl, INADDR_ANY, sockaddr_in

#include "catRef.hpp"
#include "delayedAction.hpp"
#include "free_crt.hpp"
#include "money.hpp"
#include "proc.hpp"
#include "swap.hpp"
#include "weather.hpp"
#include "lru/lru.hpp"


class cmd;
class Area;
class BaseRoom;
class Creature;
class Group;
class Monster;
class MsdpVariable;
class Object;
class Player;
class PythonHandler;
class ReportedMsdpVariable;
class Socket;
class WebInterface;

struct CanCleanupRoomFn {
	bool operator()( UniqueRoom* r );
};

struct CleanupRoomFn {
		void operator()( UniqueRoom* r );
};

struct FreeCrt {
		void operator()( Monster* mon ) { free_crt((Creature*)mon); }
};

enum GoldLog {
    GOLD_IN,
    GOLD_OUT
};

namespace boost::python {
    namespace api
    {
        class object;
    }
    using api::object;
} // namespace boost::python


#include "asynch.hpp"

#ifndef PYTHON_CODE_GEN
typedef std::map<bstring, MudObject*,idComp> IdMap;
#endif
using MonsterList= std::list<Monster*>;
using GroupList = std::list<Group*>;
using SocketList = std::list<Socket*>;
using SocketVector= std::vector<Socket*>;
using PlayerMap = std::map<bstring, Player*>;

using RoomCache = LRU::lru_cache<CatRef, UniqueRoom, CleanupRoomFn, CanCleanupRoomFn>;
using MonsterCache = LRU::lru_cache<CatRef, Monster, FreeCrt>;
using ObjectCache = LRU::lru_cache<CatRef, Object>;

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

    struct dnsCache {
        bstring ip;
        bstring hostName;
        time_t time;
        dnsCache(const bstring& pIp, const bstring& pHostName, time_t pTime) {
            ip = pIp;
            hostName = pHostName;
            time = pTime;
        }
        bool operator==(const dnsCache& o) {
            return(ip == o.ip && hostName == o.hostName);
        }
    };

public:
    PlayerMap players; // Map of all players
    SocketList sockets; // List of all connected sockets
    SocketVector* vSockets = nullptr;

    RoomCache roomCache;
    MonsterCache monsterCache;
    ObjectCache objectCache;

    void handlePythonError();

// ******************
// Internal Variables
private:

    std::list<DelayedAction> delayedActionQueue;
    
    PythonHandler* pythonHandler;

    std::list<BaseRoom*> effectsIndex;

    fd_set inSet{};
    fd_set outSet{};
    fd_set excSet{};

    bool running; // True while the game is up and bound to a port
    long pulse; // Current pulse

    bool rebooting;
    bool GDB;
    bool valgrind;

    // List of Ids
#ifndef PYTHON_CODE_GEN
    IdMap registeredIds;
#endif
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
    MonsterList activeList; // The new active list

    long lastDnsPrune;
    long lastUserUpdate;
    long lastRoomPulseUpdate;
    long lastRandomUpdate;
    long lastActiveUpdate;

    int Deadchildren;

public:
    std::list<Area*> areas;

// ****************
// Internal Methods
private:
    // Python
    bool initPython();
    bool cleanUpPython();

    int getNumSockets(); // Get number of sockets in the sockets list

    // Game & Socket methods
    int handleNewConnection(controlSock& control);
    int poll(); // Poll all descriptors for input
    int checkNew(); // Accept new connections
    int processInput(); // Process input from users
    int processCommands(); // Process commands from users
    int updatePlayerCombat(); // Handle player auto attacks etc
    int processChildren();
    int processListOutput(childProcess &lister);

    // Child processes
    int reapChildren(); // Clean up after any dead children

    // Reboot
    bool saveRebootFile(bool resetShips = false);

    // Updates
    void updateGame();
    void processMsdp();
    void pulseTicks(long t);
    void pulseCreatureEffects(long t);
    void pulseRoomEffects(long t);
    void updateUsers(long t);
    void updateRandom(long t);
    void updateActive(long t);

    // TODO: Get rid of this and switch all old "logic" creatures over to python scripts
    void updateAction(long t);

    // DNS
    void addCache(const bstring& ip, const bstring& hostName, time_t t = -1);
    void saveDnsCache();
    void loadDnsCache();
    void pruneDns();

    int installPrintfHandlers();
    static void installSignalHandlers();

    void populateVSockets();

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

    void clearAsEnemy(Player* player);
    bstring showActiveList();

    static void logGold(GoldLog dir, Player* player, Money amt, MudObject* target, const bstring& logType);

    bool registerGroup(Group* toRegister);
    bool unRegisterGroup(Group* toUnRegister);
    bstring getGroupList();

    bool registerMudObject(MudObject* toRegister, bool reassignId = false);
    bool unRegisterMudObject(MudObject* toUnRegister);
    bstring getRegisteredList();
    Creature* lookupCrtId(const bstring& toLookup);
    Object* lookupObjId(const bstring& toLookup);
    Player* lookupPlyId(const bstring& toLookup);

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
    void addDelayedScript(void (*callback)(DelayedActionFn), MudObject* target, const bstring& script, long howLong, bool canInterrupt=true);
    bool hasAction(const MudObject* target, DelayedActionType type);
    bstring delayedActionStrings(const MudObject* target);

    void weather(WeatherString w);
    void updateWeather(long t);

    // queue related functions
    void flushRoom();
    void flushMonster();
    void flushObject();

    bool reloadRoom(BaseRoom* room);
    UniqueRoom* reloadRoom(const CatRef& cr);
    int resaveRoom(const CatRef& cr);
    int saveStorage(UniqueRoom* uRoom);
    int saveStorage(const CatRef& cr);
    void resaveAllRooms(char permonly);
//    void replaceMonsterInQueue(CatRef cr, Monster *creature);
//    void replaceObjectInQueue(CatRef cr, Object* object);

    // Areas
    Area *getArea(int id);
    bool loadAreas();
    void clearAreas();
    void areaInit(Creature* player, xmlNodePtr curNode);
    void areaInit(Creature* player, MapMarker mapmarker);
    void saveAreas(bool saveRooms) const;
    void cleanUpAreas();

    void killMortalObjects();


// *******************************
// Public methods for server class
public:
    void showMemory(Socket* sock, bool extended=false);

    // Child processes
    void addChild(int pid, ChildType pType, int pFd = -1, const bstring& pExtra = "");

    // Python
    bool runPython(const bstring& pyScript, boost::python::object& dictionary);
    bool runPython(const bstring& pyScript, bstring args = "", MudObject *actor = nullptr, MudObject *target = nullptr);
    bool runPythonWithReturn(const bstring& pyScript, bstring args = "", MudObject *actor = nullptr, MudObject *target = nullptr);

    // Setup
    bool init();    // Setup the server

    void setGDB();
    void setRebooting();
    void setValgrind();

    int run(); // Run the server
    int addListenPort(int); // Add a new port to listen to

    // Status
    void sendCrash();

    bool isRebooting();

    bool isValgrind();

    // Reboot
    bool startReboot(bool resetShips = false);
    int finishReboot(); // Bring the mud back up from a reboot

    // DNS
    bstring getDnsCacheString();
    bool getDnsCache(bstring &ip, bstring &hostName);
    int startDnsLookup(Socket* sock, sockaddr_in pAddr); // Start dns lookup on a socket

    // Players
    bool addPlayer(Player* player);
    bool clearPlayer(Player* player);
    Player* findPlayer(const bstring& name);
    bool clearPlayer(const bstring& name);
    void saveAllPly();
    int getNumPlayers();

    void disconnectAll();
    int processOutput(); // Send any buffered output

    // Web Interface
    bool initWebInterface();
    bool checkWebInterface();
    void recreateFifos();

    // Active list functions
    void addActive(Monster* monster);
    void delActive(Monster* monster);
    bool isActive(Monster* monster);

    // Child Processes
    void childDied();
    int getDeadChildren() const;
    int runList(Socket* sock, cmd* cmnd);
    bstring simpleChildRead(childProcess &child);

    // Swap functions - use children
    bstring swapName();
    void finishSwap(Player* player, bool online, const CatRef& origin, const CatRef& target);
    bool swap(const Swap& s);
    void endSwap();
    void swapInfo(const Player* player);

    // Queries
    bool checkDuplicateName(Socket* sock, bool dis);
    bool checkDouble(Socket* sock);

    // Bans
    void checkBans();

    // Broadcasts
    static bstring getTimeZone();
    static bstring getServerTime();

    // Effects
    void addEffectsIndex(BaseRoom* room);
    void removeEffectsIndex(BaseRoom* room);
    void removeEffectsOwner(const Creature* owner);
    void showEffectsIndex(const Player* player);

protected:
    int cleanUp(); // Kick out any disconnectors and other general cleanup



};

extern Server *gServer;

#endif /*SERVER_H_*/
