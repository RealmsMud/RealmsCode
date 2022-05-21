/*
 * proto.h
 *   Function prototypes.
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
#ifndef PROTO_H
#define PROTO_H

#include <cstdarg>
#include <filesystem>

#include "structs.hpp"

namespace fs = std::filesystem;

class cmd;

class AreaRoom;
class AreaZone;
class BaseRoom;
class CatRefInfo;
class Creature;
class Container;
class GuildCreation;
class Location;
class MapMarker;
class MudObject;
class Object;
class Player;
class Ship;
class ShipStop;
class Socket;
class TileInfo;


// Stats
double getConBonusPercentage(unsigned int pCon);
double getIntBonusPercentage(unsigned int pInt);

// Container
bool isMatch(const std::shared_ptr<const Creature>& searcher, const std::shared_ptr<MudObject>& target, const std::string& name, bool exactMatch, bool checkVisibility = false);


// Socials
void socialHooks(const std::shared_ptr<Creature>&creature, const std::shared_ptr<MudObject> &target, const std::string &action, const std::string &result = "");
void socialHooks(const std::shared_ptr<Creature>&target, const std::string &action, const std::string &result = "");
bool actionShow(const std::shared_ptr<Player>& pTarget, const std::shared_ptr<Creature>& creature);

// afflictions.cpp
unsigned int standardPoisonDuration(short level, short con);

// calendar.cpp
int reloadCalendar(std::shared_ptr<Player> player);


// commerce.cpp
CatRef shopStorageRoom(const std::shared_ptr<const UniqueRoom>& shop);


// creature.cpp
bool isRace(int subRace, int mainRace);
std::string getSexName(Sex sex);

std::shared_ptr<Monster> getFirstAggro(std::shared_ptr<Monster>  creature, const std::shared_ptr<const Creature> & player);
std::shared_ptr<Creature>enm_in_group(std::shared_ptr<Creature>target);



// dmroom.cpp
std::string wrapText(std::string_view text, int wrap);




// io.cpp
std::string xsc(std::string_view txt);
std::string unxsc(std::string_view txt);

namespace Pueblo {
    static std::string activation = "this world is pueblo ";

    std::string multiline(std::string str);
    bool is(const std::string& txt);
}

char keyTxtConvert(unsigned char c);
std::string keyTxtConvert(std::string_view txt);
bool keyTxtEqual(const std::shared_ptr<Creature> & target, const char* txt);
bool keyTxtEqual(const std::shared_ptr<Object>  target, const char* txt);
bool isPrintable(char c);



// ------ everything below this line has not yet been ordered ------ //

std::string md5(std::string text);
void showRoomFlags(const std::shared_ptr<const Player>& player, const std::shared_ptr<const BaseRoom>& room, const std::shared_ptr<const TileInfo>& tile, const std::shared_ptr<const AreaZone>& zone);



void getCatRef(std::string str, CatRef& cr, const std::shared_ptr<Creature> & target);
void getDestination(const std::string &str, Location& l, const std::shared_ptr<Creature> & target);
void getDestination(std::string str, MapMarker& mapmarker, CatRef& cr, const std::shared_ptr<Creature> & target);

void spawnObjects(const std::string &room, const std::string &objects);




void link_rom(const std::shared_ptr<BaseRoom>& room, const Location& l, std::string_view str);
void link_rom(const std::shared_ptr<BaseRoom> &room, const CatRef& cr, std::string_view str);
void link_rom(const std::shared_ptr<BaseRoom> &room, const MapMarker& mapmarker, std::string_view str);

int room_track(const std::shared_ptr<Creature>& player);







// access.cp
std::string intToText(int nNumber, bool cap=false);
char* getSaveName(int save);
char *get_class_string(int nIndex);
char* get_lang_color(int nIndex);
char *get_language_adj(int nIndex);
char *get_language_verb(int lang);

char* get_save_string(int nIndex);
char get_save_color(int nIndex);
char* getClassAbbrev( int nIndex );
std::string getClassName(const std::shared_ptr<Player>& player);
std::string getClassName(CreatureClass cClass, CreatureClass secondClass);
const char* get_spell_name(int nIndex);
int get_spell_num(int nIndex);
int get_spell_lvl(int sflag);
int get_spell_list_size(void);
std::string int_to_text(int nNumber);
long get_quest_exp(int nQuest);
const char* get_song_name(int nIndex);
const char* get_quest_name(int nIndex);
int get_song_num(int nIndex);
SONGFN get_song_function(int nIndex);

std::string getShortClassName(const std::shared_ptr<const Player> &player);
char* getShortClassAbbrev(int nIndex);
std::string getOrdinal(int num);

char* get_skill_string(int nIndex);


void remove_all(const std::shared_ptr<Player>& player);

int peek_bag(std::shared_ptr<Player> player, std::shared_ptr<Player> target, cmd* cmnd, int inv);


void steal_gold(std::shared_ptr<Player> player, std::shared_ptr<Creature> creature);


int canGiveTransport(const std::shared_ptr<Creature>& player, const std::shared_ptr<Creature>& target, const std::shared_ptr<Object>&  object, bool give);

int get_perm_ac(int nIndex);


// creature2.cpp
std::shared_ptr<Creature>getRandomMonster(const std::shared_ptr<BaseRoom>& inRoom);
std::shared_ptr<Creature>getRandomPlayer(const std::shared_ptr<BaseRoom>& inRoom);
bool isGuardLoot(const std::shared_ptr<BaseRoom>& inRoom, const std::shared_ptr<Creature>& player, const char *fmt);

int getAlignDiff(std::shared_ptr<Creature>crt1, std::shared_ptr<Creature>crt2);


// demographics.cpp
void runDemographics();


// duel.cpp
bool induel(const std::shared_ptr<const Player>& player, const std::shared_ptr<const Player>& target);

// equipment.cpp
int doGetObject(const std::shared_ptr<Object>&  object, const std::shared_ptr<Creature>& player, bool doLimited=true, bool noSplit=false, bool noQuest=false, bool noMessage=false, bool saveOnLimited=true);
void wearAll(const std::shared_ptr<Player>& player, bool login = false);
CastResult doCast(const std::shared_ptr<Creature>& creature, cmd* cmnd);

void give_money(const std::shared_ptr<Player>& player, cmd* cmnd);




// files3.cpp
int loadCreature_tlk(std::shared_ptr<Creature> creature);
int talk_crt_act(char *str, ttag *tlk);


// guilds.cpp
std::string getGuildName( int guildNum );
void updateGuild(std::shared_ptr<Player> player, int what);

void rejectGuild(GuildCreation* toReject, char *reason);
void showGuildsNeedingApproval(std::shared_ptr<Player> viewer);

bool canRemovePlyFromGuild(std::shared_ptr<Player> player);


// healers.cpp
bool antiGradius(int race);



// io.cpp
void broadcast(Socket* ignore, const std::shared_ptr<const Container>& container, const char *fmt, ...);
void broadcast(Socket* ignore1, Socket* ignore2, const std::shared_ptr<const Container>& container, const char *fmt, ...);
void broadcast(bool showTo(Socket*), Socket*, const std::shared_ptr<const Container>& container, const char *fmt, ...);

bool yes(Socket* sock);
bool yes(std::shared_ptr<Creature> player);
bool wantsPermDeaths(Socket* sock);
void doBroadCast(bool showTo(Socket*), bool showAlso(Socket*), const char *fmt, va_list ap, const std::shared_ptr<Creature>& player = nullptr);
void broadcast(const char *fmt, ...);
void broadcast(int color, const char *fmt,...);
void broadcast(bool showTo(Socket*), bool showAlso(Socket*), const char *fmt,...);
void broadcast(bool showTo(Socket*), const char *fmt,...);
void broadcast(bool showTo(Socket*), int color, const char *fmt,...);
void broadcast(std::shared_ptr<Creature> player, bool showTo(Socket*), int color, const char *fmt,...);

void broadcast_wc(int color,const char *fmt, ...);
void broadcastLogin(std::shared_ptr<Player> player, const std::shared_ptr<BaseRoom>& inRoom, int login);

void broadcast_rom_LangWc(int lang, Socket* ignore, const Location& currentLocation, const char *fmt,...);
void broadcastGroup(bool dropLoot, const std::shared_ptr<Creature>& player, const char *fmt, ...);

void broadcastGuild(int guildNum, int showName, const char *fmt,...);
void shutdown_now(int sig);


// logic.cpp
int loadCreature_actions(std::shared_ptr<Creature> creature);



// lottery.cpp
int createLotteryTicket(std::shared_ptr<Object>& object, const char *name);
int checkPrize(std::shared_ptr<Object>ticket);

// realms.cpp
int getRandomRealm();
int getOffensiveSpell(Realm realm, int level);
int getRealmRoomBonusFlag(Realm realm);

std::string getRealmSpellName(Realm realm);
Realm getOppositeRealm(Realm realm);


// healmagic.cpp
int getHeal(const std::shared_ptr<Creature>&healer, std::shared_ptr<Creature> target, int spell);
void niceExp(std::shared_ptr<Creature> creature, int heal, CastType how);


// magic.cpp
int getSpellMp( int spellNum );
void doStatCancel(std::shared_ptr<Creature> target, std::shared_ptr<Player>pTarget, int stat, bool good);
void doStatChange(std::shared_ptr<Creature> target, std::shared_ptr<Player>pTarget, int stat, bool good);

// magic
bool hinderedByDimensionalAnchor(int splno);


void logCast(const std::shared_ptr<Creature>& caster, std::shared_ptr<Creature> target, std::string_view spell, bool dmToo=false);


// main.cpp
void startup_mordor(void);
void usage(char *szName);
void handle_args(int argc, char *argv[]);

// misc.cpp
bool validMobId(const CatRef& cr);
bool validObjId(const CatRef& cr);
bool validRoomId(const CatRef& cr);
std::string timeStr(int seconds);

std::string progressBar(int barLength, float percentFull, const std::string &text = "", char progressChar = '=', bool enclosed = true);

std::vector<std::string> splitString(std::string s, std::string delimiter = "");
std::string joinVector(std::vector<std::string> v, std::string delimiter = "");

bool nameIsAllowed(std::string str, Socket* sock);
int bonus(unsigned int num);
int crtWisdom(std::shared_ptr<Creature> creature);
int crtAwareness(std::shared_ptr<Creature> creature);
void lowercize(std::string& str, int flag);
void lowercize(char* str, int flag);
char low(char ch);
char up(char ch);
void zero(void *ptr, int size);
int dice(int n, int s, int p);
int exp_to_lev(unsigned long exp);
int dec_daily(struct daily *dly_ptr);
int update_daily(struct daily *dly_ptr);
void loge(const char *fmt, ...);
void loga(const char *fmt, ...);
void pleaseWait(std::shared_ptr<Creature> player, int duration);
void logn(const char *name, const char *fmt, ...);
int log_immort(int broad, std::shared_ptr<Player> player, const char *fmt, ...);
bool is_num(char *str);
bool isdm(std::string_view name);
bool parse_name(std::string_view name);
int dmIson(void);
int strPrefix(const char *haystack, const char *needle);
int strSuffix(const char *haystack, const char *needle);
int pkillPercent(int pkillsWon, int pkillsIn);
int getLastDigit(int n, int digits);

char *stripLineFeeds(char *str);

char *ltoa(long val, char *buf, int base);


// newMisc.cpp
void stripBadChars(char *str);
void stripBadChars(std::string str);

// object.cpp
int displayObject(const std::shared_ptr<const Player> &player, const std::shared_ptr<const Object> &target);
int new_scroll(int level, std::shared_ptr<Object> &new_obj);


bool cantDropInBag(std::shared_ptr<Object>  object);

std::string getDamageString(const std::shared_ptr<Creature>& player, std::shared_ptr<Object> weapon, bool critical);

// player.cpp
int mprofic(const std::shared_ptr<Creature> & player, int index);
std::shared_ptr<Player> lowest_piety(const std::shared_ptr<BaseRoom>& room, bool invis);

void renamePlayerFiles(const char *old_name, const char *new_name);

CatRef getEtherealTravelRoom();
void etherealTravel(std::shared_ptr<Player> player);


void sendMail(const std::string &target, const std::string &message);


// room.cpp
void display_rom(const std::shared_ptr<Player>& player, std::shared_ptr<Player> looker=nullptr, int magicShowHidden=0);
void display_rom(const std::shared_ptr<Player> &player, std::shared_ptr<BaseRoom> &room);
std::shared_ptr<Exit> findExit(const std::shared_ptr<Creature>& creature, cmd* cmnd, int val=1, std::shared_ptr<BaseRoom> room = nullptr);
std::shared_ptr<Exit> findExit(const std::shared_ptr<Creature>& creature, const std::string &inStr, int val, std::shared_ptr<BaseRoom> room = nullptr);
int createStorage(CatRef cr, const std::shared_ptr<Player>& player);
void doRoomHarms(const std::shared_ptr<BaseRoom>& inRoom, const std::shared_ptr<Player>& target);
std::shared_ptr<BaseRoom> abortFindRoom(const std::shared_ptr<Creature>& player, const char from[15]);
bool needUniqueRoom(const std::shared_ptr<Creature> & player);
Location getSpecialArea(int (CatRefInfo::*toCheck), const std::shared_ptr<const Creature> & creature, std::string_view area, short id);
Location getSpecialArea(int (CatRefInfo::*toCheck), const CatRef& cr);


// security.cpp
bool isValidPassword(Socket *sock, const std::string &pass);



// sing.cpp
int songMultiOffensive(const std::shared_ptr<Player>& player, cmd* cmnd, char *songname, osong_t *oso);
int songOffensive(const std::shared_ptr<Player>& player, cmd* cmnd, char *songname, osong_t *oso);
int songsKnown(Socket* sock, const std::shared_ptr<Player>& player, int test);
int songFail(const std::shared_ptr<Player>& player);
int songHeal(const std::shared_ptr<Player>& player, cmd* cmnd);
int songMPHeal(const std::shared_ptr<Player>& player, cmd* cmnd);
int songRestore(const std::shared_ptr<Player>& player, cmd* cmnd);
int songBless(const std::shared_ptr<Player>& player, cmd* cmnd);
int songProtection(const std::shared_ptr<Player>& player, cmd* cmnd);
int songFlight(const std::shared_ptr<Player>& player, cmd* cmnd);
int songRecall(const std::shared_ptr<Player>& player, cmd* cmnd);
int songSafety(const std::shared_ptr<Player>& player, cmd* cmnd);

// steal.cpp
int get_steal_chance(std::shared_ptr<Player> player, std::shared_ptr<Creature> creature, std::shared_ptr<Object>  object);
int steal(const std::shared_ptr<Creature>& player, cmd* cmnd);



// update.cpp
void update_time(long t);
void update_shutdown(long t);
void update_dust_oldPrint(long t);
void crash(int sig);
void cleanUpMemory();
void subtractMobBroadcast(const std::shared_ptr<Creature>&monster, int num);
int countTotalEnemies(const std::shared_ptr<Creature>&monster);
bool isDay();



// staff.cpp
bool isWatcher(Socket* sock);
bool isWatcher(const std::shared_ptr<Creature> & player);
bool isStaff(Socket* sock);
bool isStaff(const std::shared_ptr<Creature> & player);
bool isCt(Socket* sock);
bool isCt(const std::shared_ptr<Creature> & player);
bool isDm(Socket* sock);
bool isDm(const std::shared_ptr<Creature> & player);
bool isAdm(Socket* sock);
bool isAdm(const std::shared_ptr<Creature> & player);
bool watchingLog(Socket* sock);
bool watchingEaves(Socket* sock);
bool watchingSuperEaves(Socket* sock);


AlcoholState getAlcoholState(const EffectInfo* effect);

#endif
