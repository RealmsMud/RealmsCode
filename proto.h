/*
 * proto.h
 *	 Function prototypes.
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
#ifndef PROTO_H
#define PROTO_H

#include <stdarg.h>

class Ship;
class ShipStop;

// Socials
void socialHooks(Creature *creature, MudObject* target, bstring action, bstring result = "");
void socialHooks(Creature *target, bstring action, bstring result = "");
bool actionShow(Player* pTarget, Creature* creature);

// afflictions.cpp
unsigned int standardPoisonDuration(short level, short con);

// area.cpp
void update_track(long t);


// calendar.cpp
int checkBirthdays(Player* player, cmd* cmnd);
int reloadCalendar(Player* player);



// color.cpp
bstring stripColor(bstring color);
bstring escapeColor(bstring color);

// combat.cpp
int check_for_yell(Monster *monster, Creature *target);
int check_absorb(Creature* target, Player* player, int wear, int dmg);
void check_armor(Creature* target, Player* player, int wear);
Creature *findFirstEnemyCrt(Creature *crt, Creature *pet);


// commerce.cpp
CatRef shopStorageRoom(const UniqueRoom *shop);


// config.cpp
int getPkillInCombatDisabled();


// creature.cpp
bool isRace(int subRace, int mainRace);
bstring getSexName(Sex sex);

Creature *find_exact_crt(const Creature* player, ctag *first_ct, char *str, int val);
Creature *getFirstAggro(Creature* creature, Creature* player);
Creature *enm_in_group(Creature *target);

template<class Type, class Compare>
int findCrt(Creature * player, std::set<Type, Compare>& set, int findFlags, char *str, int val, int* match, Creature ** target );

// data.cpp
int cmdRecipes(Player* player, cmd* cmnd);

// dmroom.cpp
bstring wrapText(const bstring& text, int wrap);




// io.cpp
bstring xsc(const bstring& txt);
bstring unxsc(const bstring& txt);
bstring unxsc(const char* txt);

namespace Pueblo {
	static bstring activation = "this world is pueblo ";

	bstring multiline(bstring str);
	bool is(bstring txt);
}

char keyTxtConvert(unsigned char c);
bstring keyTxtConvert(const bstring& txt);
bool keyTxtEqual(const Creature* target, const char* txt);
bool keyTxtEqual(const Object* target, const char* txt);
bool isPrintable(char c);



// ------ everything below this line has not yet been ordered ------ //

bstring md5(bstring text);
int whatTraining(const BaseRoom* room, const AreaZone* zone, const TileInfo* tile, int extra=0);

void showRoomFlags(const Player* player, const BaseRoom* room, const TileInfo *tile, const AreaZone *zone);



void getCatRef(bstring str, CatRef* cr, const Creature* target);
void getDestination(bstring str, Location* l, const Creature* target);
void getDestination(bstring str, MapMarker* mapmarker, CatRef* cr, const Creature* target);

void spawnObjects(const bstring& room, const bstring& objects);

int cmdSing(Creature* creature, cmd* cmnd);




void link_rom(BaseRoom* room, Location l, bstring str);
void link_rom(BaseRoom* room, CatRef cr, bstring str);
void link_rom(BaseRoom* room, MapMarker *mapmarker, bstring str);
int view_log(Socket* sock);
int room_track(Creature* player);
int obj_track(Creature* player, Object* object);
bstring getMaterialName(Material material);

int desc_search(char *desc, char *pattern, int *val);
void txt_parse(char *str, char **pattern, int *val, char **replace);

void gamestat2(Socket* sock, char *instr);



int cmdPrepare(Player* player, cmd* cmnd);




int cmdFlee(Player* player, cmd* cmnd);
int cmdPrepareForTraps(Player* player, cmd* cmnd);
int checkWinFilename(Socket* sock, const bstring str);
//bool Pueblo::is(bstring txt);
// die.cp
//int checkDie(Creature *victim, Creature *killer);
//int checkDieRobJail(Creature *victim, Monster *killer);

// access.cp
bstring intToText(int nNumber, bool cap=false);
char *alignmentString(Creature* player);
char *get_class_string(int nIndex);
int get_lang_color(int nIndex);
char *get_language_adj(int nIndex);
char *get_language_verb(int lang);

char* get_save_string(int nIndex);
char get_save_color(int nIndex);
char* getClassAbbrev( int nIndex );
char* getClassName(Player* player);
const char* get_spell_name(int nIndex);
int get_spell_num(int nIndex);
int get_spell_lvl(int sflag);
int get_spell_list_size(void);
char* int_to_text(int nNumber);
long get_quest_exp(int nQuest);
const char* get_song_name(int nIndex);
const char* get_quest_name(int nIndex);
int get_song_num(int nIndex);
SONGFN get_song_function(int nIndex);
char *obj_type(int nType);

const char* get_rflag(int nIndex);
const char* get_xflag(int nIndex);
const char* get_oflag(int nIndex);
const char* get_mflag(int nIndex);
const char* get_pflag(int nIndex);

char* getShortClassName(const Player* player);
char* getShortClassAbbrev(int nIndex);
bstring getOrdinal(int num);

char* get_trade_string(int nIndex);
char* get_skill_string(int nIndex);

bool isTitle(bstring str);
bool isClass(char str[80]);




int cmdReconnect(Player* player, cmd* cmnd);
void login(Socket* sock, bstring str);
void createPlayer(Socket* sock, bstring str);

void remove_all(Player* player);
void equip_list(const Player* viewer, const Creature* creature);

int peek_bag(Player* player, Player* target, cmd* cmnd, int inv);


int cmdSteal(Player* player, cmd* cmnd);
void steal_gold(Player* player, Creature* creature);


int canGiveTransport(Creature* player, Creature* target, Object* object, bool give);
void talk_action(Player* player, Monster *creature, ttag *tt);






int get_perm_ac(int nIndex);




// creature2.cpp
Creature *getRandomMonster(BaseRoom *inRoom);
Creature *getRandomPlayer(BaseRoom *inRoom);
bool isGuardLoot(BaseRoom *inRoom, Creature* player, const char *fmt);
bool npcPresent(UniqueRoom *inRoom, short trade);

int getAlignDiff(Creature *crt1, Creature *crt2);


// demographics.cpp
void runDemographics();
int cmdDemographics(Player* player, cmd* cmnd);





// duel.cpp
bool induel(const Player* player, const Player* target);

// equipment.cpp
int doGetObject(Object* object, Creature* player, bool doLimited=true, bool noSplit=false, bool noQuest=false, bool noMessage=false, bool saveOnLimited=true);
void wearAll(Player* player, bool login = false);
int cmdWear(Player* player, cmd* cmnd);
int cmdRemoveObj(Player* player, cmd* cmnd);
int cmdEquipment(Player *creature, cmd* cmnd);
int cmdReady(Player* player, cmd* cmnd);
int cmdHold(Player* player, cmd* cmnd);
int cmdSecond(Player* player, cmd* cmnd);

int cmdCast(Creature* creature, cmd* cmnd);
CastResult doCast(Creature* creature, cmd* cmnd);
int cmdTeach(Player* player, cmd* cmnd);
int cmdStudy(Player* player, cmd* cmnd);
int cmdReadScroll(Player* player, cmd* cmnd);
int cmdConsume(Player* player, cmd* cmnd);
int cmdUseWand(Player* player, cmd* cmnd);
int cmdUnlock(Player* player, cmd* cmnd);
void give_money(Player* player, cmd* cmnd);



// faction.cpp
int cmdFactions(Player* player, cmd* cmnd);

// files1.cpp
char* objectPath(const CatRef cr);
char* monsterPath(const CatRef cr);
char* roomPath(const CatRef cr);
char* roomBackupPath(const CatRef cr);

int count_obj(Object* object, char perm_only);
int count_inv(Creature* creature, char perm_only);
int count_bag_inv(Creature* creature);
void free_crt(Creature* creature, bool remove=true);
bstring loadHelpTemplate(const char* filename);

// files2.cpp
void save_all_ply(void);



// files3.cpp
int loadCreature_tlk(Creature* creature);
int talk_crt_act(char *str, ttag *tlk);

// gods.cpp
//int checkGodKill(Player *killer, Player *victim);

// global.cpp

// guilds.cpp
//int guildExists(char *guildName);
void addGuildCreation( GuildCreation* toAdd);
//int addSupporter(GuildCreation* toSupport, char *supporterName);
//void creationToGuild(GuildCreation* toApprove);
//void parseGuildFile(void);
//void loadGuilds(void);
//void parseGuildFile(void);
//void clearGuildList(void);
//void freeGuild(int toFree);
//void saveGuilds(void);
//int findGuild(char *guildName);
//int findGuild(int num);
//GuildCreation* findGuildCreation(char *creationName);
//int avgGuildLevel(int guildId);
const char *getGuildName( int guildNum );
void updateGuild(Player* player, int what);

//void initGuildArray(void);
//void expandGuildArray(void);
//int checkGuildKill(Player *killer, Player *victim);
void rejectGuild(GuildCreation* toReject, char *reason);
//int addMember(int guildId, char *memberName);
//int delMember(int guildId, char *memberName);
void showGuildsNeedingApproval(Player* viewer);

bool canRemovePlyFromGuild(Player* player);


// healers.cpp
bool antiGradius(int race);



// io.cpp
void broadcast(Socket* ignore, Container* container, const char *fmt, ...);
void broadcast(Socket* ignore1, Socket* ignore2, Container* container, const char *fmt, ...);
void broadcast(bool showTo(Socket*), Socket*, Container* container, const char *fmt, ...);

bool yes(Socket* sock);
bool yes(Creature* player);
bool wantsPermDeaths(Socket* sock);
void doBroadCast(bool showTo(Socket*), bool showAlso(Socket*), const char *fmt, va_list ap, Creature* player = NULL);
void broadcast(const char *fmt, ...);
void broadcast(int color, const char *fmt,...);
void broadcast(bool showTo(Socket*), bool showAlso(Socket*), const char *fmt,...);
void broadcast(bool showTo(Socket*), const char *fmt,...);
void broadcast(bool showTo(Socket*), int color, const char *fmt,...);
void broadcast(Creature* player, bool showTo(Socket*), int color, const char *fmt,...);
void announcePermDeath(Creature* player, const char *fmt,...);

//void run_game();
void broadcast_wc(int color,const char *fmt, ...);
void broadcast_login(Player* player, int login);

void broadcast_rom_LangWc(int face, int color, int lang, Socket* ignore, AreaRoom* aRoom, CatRef cr, const char *fmt,...);
void broadcastGroup(bool dropLoot, Creature* player, const char *fmt, ...);
void child_died(int sig);
void quick_shutdown(int sig);
void broadcastGuild(int guildNum, int showName, const char *fmt,...);
void disconnect_all_ply(void);
void shutdown_now(int sig);
int check_flood(Socket* sock);




// logic.cpp
int loadCreature_actions(Creature* creature);



// lottery.cpp
int createLotteryTicket(Object **object, char *name);
int checkPrize(Object *ticket);

// realms.cpp
int getRandomRealm();
int getOffensiveSpell(Realm realm, int level);
//bool realmResistPet(Creature* target, Creature *pet, int (*func) (int));
int getRealmRoomBonusFlag(Realm realm);

bstring getRealmSpellName(Realm realm);
//int getCreatureOf(Realm realm);
Realm getOppositeRealm(Realm realm);


// healmagic.cpp
int getHeal(Creature *healer, Creature* target, int spell);
void niceExp(Creature* creature, int heal, int how);


// magic.cpp
int getSpellMp( int spellNum );
void doStatCancel(Creature* target, Player *pTarget, int stat, bool good);
void doStatChange(Creature* target, Player *pTarget, int stat, bool good);

// magic
int doMpCheck(Creature* target, int splno);
int consume(Player* player, Object* object, cmd* cmnd);
int cmdRecall(Player* player, cmd* cmnd);
int useRecallPotion(Player* player, int show, int log);
int getTurnChance(Creature* player, Creature* target);
bool hinderedByDimensionalAnchor(int splno);

int spell_fail(Creature* player, int how);

void logCast(Creature* caster, Creature* target, bstring spell, bool dmToo=false);


// main.cpp
void startup_mordor(void);
void usage(char *szName);
void handle_args(int argc, char *argv[]);
void mvc_log(void);

// mccp.cpp
//int write_to_socket(int fd, const char *txt, unsigned int nLen, int bSpy );
//int compressStart(Socket* sock);
//int compressEnd(Socket* sock);
//int processCompressed(Socket* sock);

// memory.cpp

// misc.cpp
bool validMobId(const CatRef cr);
bool validObjId(const CatRef cr);
bool validRoomId(const CatRef cr);
bstring timeStr(int seconds);
bstring removeColor(bstring obj);
bool nameEqual(bstring obj, bstring str);
int cmdGo(Player* player, cmd* cmnd);
bstring progressBar(int barLength, float percentFull, bstring text = "", char progressChar = '=', bool enclosed = true);

bool nameIsAllowed(bstring str, Socket* sock);
bool findTarget(Creature * player, int findWhere, int findFlags, char *str, int val, void** target, int* targetType);
int bonus(int num);
int crtWisdom(Creature* creature);
int crtAwareness(Creature* creature);
void new_merror(const char *str, char errtype, const char *file, const int line );
void lowercize(bstring& str, int flag);
void lowercize(char* str, int flag);
int low(char ch);
int up(char ch);
void zero(void *ptr, int size);
#ifdef CYGWIN
char *crt_str(const Creature *crt, int num, int flag);
#endif
void viewFile(Socket* sock, bstring str);
void viewLoginFile(Socket* sock, bstring str, bool showError=true);
void viewFileReverse(Socket* sock, bstring str);
int dice(int n, int s, int p);
int exp_to_lev(unsigned long exp);
int dec_daily(struct daily *dly_ptr);
int update_daily(struct daily *dly_ptr);
void loge(const char *fmt, ...);
void loga(const char *fmt, ...);
bool file_exists(char *filename);
void pleaseWait(Creature* player, int duration);
void logn(const char *name, const char *fmt, ...);
int log_immort(int broad, Player* player, const char *fmt, ...);
bool is_num(char *str);
void _assertlog(const char *strExp, const char *strFile, unsigned int nLine);
bool isdm(bstring name);
//int smashInvis(Creature* creature);
bool parse_name(bstring name);
int dmIson(void);
long exp_split(Creature* creature, long amount);
int strPrefix(const char *haystack, const char *needle);
int strSuffix(const char *haystack, const char *needle);
int pkillPercent(int pkillsWon, int pkillsIn);
int	numEnemyMonInRoom(Creature* player);
int	getLastDigit(int n, int digits);

char *stripLineFeeds(char *str);

char *ltoa(long val, char *buf, int base);


// newMisc.cpp
void stripBadChars(char *str);
void stripBadChars(bstring str);

// missile.cpp

// object.cpp
int findObj(const Creature* player, otag *first_ot, int findFlags, char *str, int val, int* match, Object** target );
int displayObject(Player* player, Object* target);
void del_obj_obj(Object* object, Object* container);
Object* findObject(const Player *player, int id);
Object* findObject(const Creature *player, otag* first_ot, const cmd* cmnd, int val=1);
Object* findObject(const Creature* player, otag *first_ot, const char *str, int val);
bstring listObjects(const Player* player, otag *target, bool showAll, char endColor='x');
//void randomEnchant(Object* object);
//int find_obj_num(Object* object);
//void mageRandomEnchant(Object* object, Creature* player);
int new_scroll(int level, Object **new_obj);
int checkScrollDrop(Creature* creature);



//void loadContainerContents(Object *cnt_obj);
int cmdKeep(Player* player, cmd* cmnd);
int cmdUnkeep(Player* player, cmd* cmnd);
bool cantDropInBag(Object* object);

void getDamageString(char atk[50], Creature* player, Object *weapon, bool critical=false);

// player.cpp
int cmdTitle(Player* player, cmd* cmnd);
void doTitle(Socket* sock, bstring str);
int mprofic(const Creature* player, int index);
Player* lowest_piety(BaseRoom* room, bool invis);
int getMultiClassID(char cls, char cls2);

void renamePlayerFiles(char *old_name, char *new_name);


// player2.cpp
//Creature *getPet(Creature* player);
CatRef getEtherealTravelRoom();
void etherealTravel(Player* player);
int cmdSurname(Player* player, cmd* cmnd);
void doSurname(Socket* sock, bstring str);
int cmdVisible(Player* player, cmd* cmnd);
int cmdDice(Creature* player, cmd* cmnd);
int cmdChooseAlignment(Player* player, cmd* cmnd);
void setPlyAlignment(Socket* sock, char *str);
bool plyHasObj(Creature* player, Object *item);


// post.cpp
int cmdSendMail(Player* player, cmd* cmnd);
void postedit(Socket* sock, bstring str);
int cmdReadMail(Player* player, cmd* cmnd);
int cmdDeleteMail(Player* player, cmd* cmnd);
int notepad(Player* player, cmd* cmnd);
void noteedit(Socket* sock,bstring str);
int cmdEditHistory(Player* player, cmd* cmnd);
void histedit(Socket* sock, bstring str);
int cmdHistory(Player* player, cmd* cmnd);
int cmdDeleteHistory(Player* player, cmd* cmnd);
void sendMail(const bstring& target, const bstring& message);

//void taskedit(Socket* sock, char *str);
//int dmTask(Creature* player, cmd* cmnd);
//void bugEdit(Socket* sock, char *str);
//int bugReport(Creature* player, cmd* cmnd);
//void petitionEdit(Socket* sock, char *str);
//int plyPetition(Creature* player, cmd* cmnd);
//void wlogedit(Socket* sock, char *str);
//int watcher_log(Creature* player, cmd* cmnd);

// quests.cpp
void freeQuest(questPtr toFree);
void fulfillQuest(Player* player, Object* object);

// refuse.cpp
int cmdRefuse(Player* player, cmd* cmnd);
int cmdWatch(Player* player, cmd* cmnd);

// room.cpp
void display_rom(Player* player, Player *looker=0, int magicShowHidden=0);
void display_rom(Player* player, BaseRoom* room);
Exit *findExit(Creature* creature, cmd* cmnd, int val=1, BaseRoom* room = 0);
Exit *findExit(Creature* creature, bstring str, int val, BaseRoom* room = 0);
int createStorage(CatRef cr, const Player* player);
void doRoomHarms(BaseRoom *inRoom, Player* target);
BaseRoom *abortFindRoom(Creature* player, const char from[15]);
bool needUniqueRoom(const Creature* player);
Location getSpecialArea(int (CatRefInfo::*toCheck), const Creature* creature, bstring area, short id);
Location getSpecialArea(int (CatRefInfo::*toCheck), CatRef cr);


// security.cpp
bool isValidPassword(Socket*, bstring pass);
int cmdPassword(Player* player, cmd* cmnd);
void changePassword(Socket*, bstring str);




// screen.cpp
void ANSI(Socket* sock, int color);

// size.cpp
Size getSize(bstring str);
bstring getSizeName(Size size);
int searchMod(Size size);
Size whatSize(int i);

// sing.cpp
int songMultiOffensive(Player* player, cmd* cmnd, char *songname, osong_t *oso);
int songOffensive(Player* player, cmd* cmnd, char *songname, osong_t *oso);
int cmdSongs(Player* player, cmd* cmnd);
int songsKnown(Socket* sock, Player* player, int test);
int songFail(Player* player);
int songHeal(Player* player, cmd* cmnd);
int songMPHeal(Player* player, cmd* cmnd);
int songRestore(Player* player, cmd* cmnd);
int songBless(Player* player, cmd* cmnd);
int songProtection(Player* player, cmd* cmnd);
int songFlight(Player* player, cmd* cmnd);
int songRecall(Player* player, cmd* cmnd);
int songSafety(Player* player, cmd* cmnd);

// spelling.cpp
void init_spelling(void);

// steal.cpp
int get_steal_chance(Player* player, Creature* creature, Object* object);
int steal(Creature* player, cmd* cmnd);



// update.cpp
void update_game(void);
void update_users(long t);
void update_random(long t);
void update_active(long t);
void update_time(long t);
void update_shutdown(long t);
//void update_exit(long t);
void update_allcmd(long t);
int list_act(Player* player, cmd* cmnd);
void log_act(long t);
void update_security(long t);
void update_action(long t);
void update_dust_oldPrint(long t);
void handle_pipe(int sig);
void crash(int sig);
//void load_exits(void);
void bubblesort(short num[], int max);
int nonzero_count(short num[], int max);
void cleanUpMemory(void);
void subtractMobBroadcast(Creature *monster, int num);
int countTotalEnemies(Creature *monster);
int numFightingSameEnemy(Creature* player);
bool isDay();


// weaponless.cpp
int cmdHowl(Creature* player, cmd* cmnd);

// xml.cpp
int toBoolean(char *fromStr);
Color toColor(char *fromStr);
char *iToYesNo(int fromInt);
char *colorToStr(int fromColor);

// staff.cpp
bool isWatcher(Socket* sock);
bool isWatcher(const Creature* player);
bool isStaff(Socket* sock);
bool isStaff(const Creature* player);
bool isCt(Socket* sock);
bool isCt(const Creature* player);
bool isDm(Socket* sock);
bool isDm(const Creature* player);
bool isAdm(Socket* sock);
bool isAdm(const Creature* player);
bool watchingLog(Socket* sock);
bool watchingEaves(Socket* sock);
bool watchingSuperEaves(Socket* sock);

bool isOutdoors(Socket* sock);


AlcoholState getAlcoholState(const EffectInfo* effect);

#endif
