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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#pragma once

#include <list>
#include <map>
#include <vector>

// C Includes
#include <netinet/in.h> // Needs: htons, htonl, INADDR_ANY, sockaddr_in
#include "config.hpp"
#include "catRef.hpp"
#include "delayedAction.hpp"
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
class HttpServer;

namespace dpp {
    class cluster;
    class commandhandler;
}

struct CanCleanupRoomFn {
    bool operator()(const std::shared_ptr<UniqueRoom>& r );
};

struct CleanupRoomFn {
    void operator()(const std::shared_ptr<UniqueRoom>& r );
};

enum GoldLog {
    GOLD_IN,
    GOLD_OUT
};

// Custom comparison operator to sort by the numeric id instead of standard string comparison
auto idComp = [](const std::string& lhs, const std::string& rhs) {
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
        else {
            return (longL < longR);
        }
    } else {
        return(charL < charR);
    }
    return(true);
};

#include "async.hpp"

typedef std::map<std::string, std::weak_ptr<MudObject>, decltype(idComp)> IdMap;
using WeakMonsterList = std::list<std::weak_ptr<Monster> >;
using MonsterList = std::list<std::shared_ptr<Monster> >;
using GroupList = std::list<Group*>;
using SocketList = std::list<std::shared_ptr<Socket>>;
using SocketVector= std::vector<std::weak_ptr<Socket>>;
using PlayerMap = std::map<std::string, std::shared_ptr<Player>>;

using RoomCache = LRU::lru_cache<CatRef, std::shared_ptr<UniqueRoom>, CleanupRoomFn, CanCleanupRoomFn>;
using MonsterCache = LRU::lru_cache<CatRef, Monster >;
using ObjectCache = LRU::lru_cache<CatRef, Object >;

class Server {
    friend class PythonHandler;
// **************
// Static Methods
public:
    static Server* getInstance();
    static void destroyInstance();


// Instance
private:
    static Server* myInstance;
    Server();
    Server(const Server&) = delete;
    Server(Server&&) = delete;
    ~Server();

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
    HttpServer* httpServer;

    std::list<std::weak_ptr<BaseRoom>> effectsIndex;

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
    WeakMonsterList activeList; // The new active list

    long lastDnsPrune;
    long lastUserUpdate;
    long lastRoomPulseUpdate;
    long lastRandomUpdate;
    long lastActiveUpdate;

public:
    std::list<std::shared_ptr<Area> > areas;

// ****************
// Internal Methods
private:
    bool initDiscordBot();
    bool initHttpServer();

    void cleanupDiscordBot();
    void cleanupHttpServer();

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

public:

    void clearAsEnemy(const std::shared_ptr<Player>& player);
    std::string showActiveList();

    static void logGold(GoldLog dir, const std::shared_ptr<Player>& player, Money amt, std::shared_ptr<MudObject> target, std::string_view logType);

    bool registerGroup(Group* toRegister);
    bool unRegisterGroup(Group* toUnRegister);
    std::string getGroupList();

    bool registerMudObject(const std::shared_ptr<MudObject>& toRegister, bool reassignId = false);
    bool unRegisterMudObject(MudObject* toUnRegister);
    std::string getRegisteredList();
    std::shared_ptr<Creature> lookupCrtId(const std::string &toLookup);
    std::shared_ptr<Object>  lookupObjId(const std::string &toLookup);
    std::shared_ptr<Player> lookupPlyId(const std::string &toLookup);

    void loadIds();
    void saveIds();

    std::string getNextMonsterId();
    std::string getNextPlayerId();
    std::string getNextObjectId();

    long getMaxMonsterId();
    long getMaxPlayerId();
    long getMaxObjectId();

    bool removeDelayedActions(const std::shared_ptr<MudObject>& target, bool interruptOnly=false);
    bool removeDelayedActions(MudObject* target, bool interruptOnly=false);
    void addDelayedAction(void (*callback)(DelayedActionFn), const std::shared_ptr<MudObject>& target, cmd* cmnd, DelayedActionType type, long howLong, bool canInterrupt=true);
    void addDelayedScript(void (*callback)(DelayedActionFn), const std::shared_ptr<MudObject>& target, std::string_view script, long howLong, bool canInterrupt=true);
    bool hasAction(const std::shared_ptr<MudObject>& target, DelayedActionType type);
    std::string delayedActionStrings(const std::shared_ptr<MudObject>& target);

    void weather(WeatherString w);
    void updateWeather(long t);

    // queue related functions
    void flushRoom();
    void flushMonster();
    void flushObject();

    bool reloadRoom(const std::shared_ptr<BaseRoom>& room);
    std::shared_ptr<UniqueRoom> reloadRoom(const CatRef& cr);
    int resaveRoom(const CatRef& cr);
    int saveStorage(const std::shared_ptr<UniqueRoom>& uRoom);
    int saveStorage(const CatRef& cr);
    void resaveAllRooms(char permonly);

    // Areas
    std::shared_ptr<Area> getArea(int id);
    bool loadAreas();
    void areaInit(const std::shared_ptr<Creature>& player, xmlNodePtr curNode);
    void areaInit(const std::shared_ptr<Creature>& player, MapMarker mapmarker);
    void saveAreas(bool saveRooms) const;
    void cleanUpAreas();

    void killMortalObjects();


// *******************************
// Public methods for server class
public:
    void showMemory(std::shared_ptr<Socket> sock, bool extended=false);

    // Child processes
    void addChild(int pid, ChildType pType, int pFd = -1, std::string_view pExtra = "");


    // Setup
    bool init();    // Setup the server

    void setGDB();
    void setRebooting();
    void setValgrind();

    void run(); // Run the server
    void stop(); // Run the server
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
    int startDnsLookup(Socket *sock, sockaddr_in addr); // Start dns lookup on a socket

    // Players
    bool addPlayer(const std::shared_ptr<Player>& player);
    bool clearPlayer(const std::shared_ptr<Player>& player);
    std::shared_ptr<Player> findPlayer(const std::string &name);
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
    void addActive(const std::shared_ptr<Monster>&  monster);
    void delActive(Monster* monster);
    bool isActive(Monster* monster);

    // Child Processes
    int runList(std::shared_ptr<Socket> sock, cmd* cmnd);
    std::string simpleChildRead(childProcess &child);

    // Swap functions - use children
    std::string swapName();
    void finishSwap(const std::shared_ptr<Player>& player, bool online, const CatRef& origin, const CatRef& target);
    bool swap(const Swap& s);
    void endSwap();
    void swapInfo(const std::shared_ptr<Player>& player);

    // Queries
    bool checkDuplicateName(std::shared_ptr<Socket> sock, bool dis);
    bool checkDouble(std::shared_ptr<Socket> sock);

    // Bans
    void checkBans();

    // Broadcasts
    static std::string getTimeZone();
    static std::string getServerTime();

    // Effects
    void addEffectsIndex(const std::shared_ptr<BaseRoom>& room);
    void removeEffectsIndex(const BaseRoom* room);
    void removeEffectsOwner(const std::shared_ptr<Creature> & owner);
    void showEffectsIndex(const std::shared_ptr<Player> &player);

    // Python
    bool runPython(const std::string& pyScript, py::object& dictionary);
    bool runPythonWithReturn(const std::string& pyScript, py::object& dictionary);
    bool runPython(const std::string& pyScript, const std::string &args = "", std::shared_ptr<MudObject>actor = nullptr, std::shared_ptr<MudObject>target = nullptr);
    bool runPythonWithReturn(const std::string& pyScript, const std::string &args = "", std::shared_ptr<MudObject>actor = nullptr, std::shared_ptr<MudObject>target = nullptr);

protected:
    int cleanUp(); // Kick out any disconnectors and other general cleanup


public:
    bool sendDiscordWebhook(long webhookID, int type, const std::string &author, const std::string &msg);
};

extern Server *gServer;
