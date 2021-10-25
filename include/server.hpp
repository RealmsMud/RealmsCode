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

namespace pybind11 {
    class object;
}
namespace py = pybind11;

class cmd;
class Area;
class BaseRoom;
class UniqueRoom;
class MapMarker;
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

namespace dpp {
    class cluster;
    class commandhandler;
}

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

// Custom comparison operator to sort by the numeric id instead of standard string comparison
struct idComp : public std::binary_function<const std::string&, const std::string&, bool> {
    bool operator() (const std::string& lhs, const std::string& rhs) const;
};


#include "async.hpp"

typedef std::map<std::string, MudObject*,idComp> IdMap;
using MonsterList = std::list<Monster*>;
using GroupList = std::list<Group*>;
using SocketList = std::list<Socket>;
using SocketVector= std::vector<Socket*>;
using PlayerMap = std::map<std::string, Player*>;

using RoomCache = LRU::lru_cache<CatRef, UniqueRoom, CleanupRoomFn, CanCleanupRoomFn>;
using MonsterCache = LRU::lru_cache<CatRef, Monster, FreeCrt>;
using ObjectCache = LRU::lru_cache<CatRef, Object>;

class Server {
    friend class PythonHandler;
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
        std::string ip;
        std::string hostName;
        time_t time;
        dnsCache(std::string_view pIp, std::string_view pHostName, time_t pTime) {
            ip = pIp;
            hostName = pHostName;
            time = pTime;
        }
        bool operator==(const dnsCache& o) const {
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
    dpp::cluster *discordBot{};
    dpp::commandhandler *commandHandler{};

    // Game Updates
    MonsterList activeList; // The new active list

    long lastDnsPrune;
    long lastUserUpdate;
    long lastRoomPulseUpdate;
    long lastRandomUpdate;
    long lastActiveUpdate;

public:
    std::list<Area*> areas;

// ****************
// Internal Methods
private:
    bool initDiscordBot();

    size_t getNumSockets() const; // Get number of sockets in the sockets list

    // Game & Socket methods
    int handleNewConnection(controlSock& control);
    int poll(); // Poll all descriptors for input
    int checkNew(); // Accept new connections
    int processInput(); // Process input from users
    int processCommands(); // Process commands from users
    int updatePlayerCombat(); // Handle player auto attacks etc
    int processChildren();
    int processListOutput(const childProcess &lister);

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
    void updateTrack(long t);
    void updateShips(long n=0);

    // TODO: Get rid of this and switch all old "logic" creatures over to python scripts
    void updateAction(long t);

    // DNS
    void addCache(std::string_view ip, std::string_view hostName, time_t t = -1);
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
    bool logGoldSql(std::string& pName, std::string& pId, std::string& targetStr, std::string& source, std::string& room,
                    std::string& logType, unsigned long amt, std::string& direction);

public:
    bool getConnStatus();
    int getConnTimeout();
#endif // SQL_LOGGER


public:

    void clearAsEnemy(Player* player);
    std::string showActiveList();

    static void logGold(GoldLog dir, Player* player, Money amt, MudObject* target, std::string_view logType);

    bool registerGroup(Group* toRegister);
    bool unRegisterGroup(Group* toUnRegister);
    std::string getGroupList();

    bool registerMudObject(MudObject* toRegister, bool reassignId = false);
    bool unRegisterMudObject(MudObject* toUnRegister);
    std::string getRegisteredList();
    Creature* lookupCrtId(const std::string &toLookup);
    Object* lookupObjId(const std::string &toLookup);
    Player* lookupPlyId(const std::string &toLookup);

    void loadIds();
    void saveIds();

    std::string getNextMonsterId();
    std::string getNextPlayerId();
    std::string getNextObjectId();

    long getMaxMonsterId();
    long getMaxPlayerId();
    long getMaxObjectId();

    bool removeDelayedActions(MudObject* target, bool interruptOnly=false);
    void addDelayedAction(void (*callback)(DelayedActionFn), MudObject* target, cmd* cmnd, DelayedActionType type, long howLong, bool canInterrupt=true);
    void addDelayedScript(void (*callback)(DelayedActionFn), MudObject* target, std::string_view script, long howLong, bool canInterrupt=true);
    bool hasAction(const MudObject* target, DelayedActionType type);
    std::string delayedActionStrings(const MudObject* target);

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
    void addChild(int pid, ChildType pType, int pFd = -1, std::string_view pExtra = "");


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
    std::string getDnsCacheString();
    bool getDnsCache(std::string &ip, std::string &hostName);
    int startDnsLookup(Socket* sock, sockaddr_in pAddr); // Start dns lookup on a socket

    // Players
    bool addPlayer(Player* player);
    bool clearPlayer(Player* player);
    Player* findPlayer(const std::string &name);
    bool clearPlayer(const std::string &name);
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
    int runList(Socket* sock, cmd* cmnd);
    std::string simpleChildRead(childProcess &child);

    // Swap functions - use children
    std::string swapName();
    void finishSwap(Player* player, bool online, const CatRef& origin, const CatRef& target);
    bool swap(const Swap& s);
    void endSwap();
    void swapInfo(const Player* player);

    // Queries
    bool checkDuplicateName(Socket &sock, bool dis);
    bool checkDouble(Socket &sock);

    // Bans
    void checkBans();

    // Broadcasts
    static std::string getTimeZone();
    static std::string getServerTime();

    // Effects
    void addEffectsIndex(BaseRoom* room);
    void removeEffectsIndex(BaseRoom* room);
    void removeEffectsOwner(const Creature* owner);
    void showEffectsIndex(const Player* player);

    // Python
    bool runPython(const std::string& pyScript, py::object& dictionary);
    bool runPythonWithReturn(const std::string& pyScript, py::object& dictionary);
    bool runPython(const std::string& pyScript, const std::string &args = "", MudObject *actor = nullptr, MudObject *target = nullptr);
    bool runPythonWithReturn(const std::string& pyScript, const std::string &args = "", MudObject *actor = nullptr, MudObject *target = nullptr);

protected:
    int cleanUp(); // Kick out any disconnectors and other general cleanup


public:
    bool sendDiscordWebhook(long webhookID, int type, const std::string &author, const std::string &msg);
};

extern Server *gServer;

#endif /*SERVER_H_*/
