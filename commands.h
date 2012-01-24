/*
 * commands.h
 *	 Various command prototypes
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

#ifndef COMMANDS_H_
#define COMMANDS_H_

// Effects.cpp
int dmEffectList(Player* player, cmd* cmnd);
int dmShowEffectsIndex(Player* player, cmd* cmnd);

// songs.cpp
int cmdPlay(Player* player, cmd* cmnd);

// alchemy.cpp
int cmdBrew(Player* player, cmd* cmnd);


int cmdWeapons(Player* player, cmd* cmnd);

// action.c
int plyAction(Player* player, cmd* cmnd);
int cmdAction(Creature* player, cmd* cmnd);
bool isBadSocial(const bstring& str);
bool isSemiBadSocial(const bstring& str);
bool isGoodSocial(const bstring& str);

// attack.c
int cmdAttack(Creature* player, cmd* cmnd);

// bank.c
int cmdBalance(Player* player, cmd* cmnd);
int cmdDeposit(Player* player, cmd* cmnd);
int cmdWithdraw(Player* player, cmd* cmnd);
int cmdTransfer(Player* player, cmd* cmnd);
int cmdStatement(Player* player, cmd* cmnd);
int cmdDeleteStatement(Player* player, cmd* cmnd);


// color.c
int cmdColors(Player* player, cmd* cmnd);

// command2.c
int cmdLook(Player* player, cmd* cmnd);
int cmdKnock(Creature* player, cmd* cmnd);
int cmdThrow(Creature* creature, cmd* cmnd);

// command1.c
int cmdNoExist(Player* player, cmd* cmnd);
int cmdNoAuth(Player* player);
void command(Socket* sock, bstring str);
void parse(bstring str, cmd* cmnd);

int cmdPush(Player* player, cmd* cmnd);
int cmdPull(Player* player, cmd* cmnd);
int cmdPress(Player* player, cmd* cmnd);


// command4.c
int cmdScore(Player* player, cmd* cmnd);
int cmdDaily(Player* player, cmd* cmnd);
int cmdHelp(Player* player, cmd* cmnd);
int cmdWiki(Player* player, cmd* cmnd);
int cmdWelcome(Player* player, cmd* cmnd);
int cmdAge(Player* player, cmd* cmnd);
int cmdVersion(Player* player, cmd* cmnd);
int cmdStatistics(Player* player, cmd* cmnd);
int cmdInfo(Player* player, cmd* cmnd);
int cmdSpells(Creature* player, cmd* cmnd);
void spellsUnder(const Player* viewer, const Creature* target, bool notSelf);


// command5.c
int cmdWho(Player* player, cmd* cmnd);
int cmdClasswho(Player* player, cmd* cmnd);
int cmdWhois(Player* player, cmd* cmnd);
int cmdSuicide(Player* player, cmd* cmnd);
void deletePlayer(Player* player);
int cmdConvert(Player* player, cmd* cmnd);
int flag_list(Creature* player, cmd* cmnd);
int cmdPrefs(Player* player, cmd* cmnd);
int cmdTelOpts(Player* player, cmd* cmnd);
int cmdQuit(Player* player, cmd* cmnd);
int cmdChangeStats(Player* player, cmd* cmnd);
void changingStats(Socket* sock, bstring str );


// command7.c
int cmdTrain(Player* player, cmd* cmnd);


// command8.c
char *timestr(long t);
int cmdTime(Player* player, cmd* cmnd);
int cmdSave(Player* player, cmd* cmnd);


// command10.c
void lose_all(Player* player, bool destroyAll, const char* lostTo);
int cmdBreak(Player* player, cmd* cmnd);


// command11.c
bstring doFinger(const Player* player, bstring name, unsigned short cls);
int cmdFinger(Player* player, cmd* cmnd);
int cmdPayToll(Player* player, cmd* cmnd);
unsigned long tollcost(const Player* player, const Exit *exit, Monster* keeper);
int infoGamestat(Player* player, cmd* cmnd);
int cmdDescription(Player* player, cmd* cmnd);
int hire(Player* player, cmd* cmnd);


// communication.c
bstring getFullstrTextTrun(bstring str, int skip, char toSkip = ' ', bool colorEscape=false);
bstring getFullstrText(bstring str, int skip, char toSkip = ' ', bool colorEscape=false, bool truncate=false);
int communicateWith(Player* player, cmd* cmnd);
int pCommunicate(Player* player, cmd* cmnd);
int communicate(Creature* player, cmd* cmnd);
int channel(Player* player, cmd* cmnd );
int cmdIgnore(Player* player, cmd* cmnd);
int cmdSpeak(Player* player, cmd* cmnd);
int cmdLanguages(Player* player, cmd* cmnd);
bool canCommunicate(Player* player);

// commerce.c
int cmdShop(Player* player, cmd* cmnd);
int cmdList(Player* player, cmd* cmnd);
int cmdPurchase(Player* player, cmd* cmnd);
int cmdSelection(Player* player, cmd* cmnd);
int cmdBuy(Player* player, cmd* cmnd);
int cmdSell(Player* player, cmd* cmnd);
int cmdValue(Player* player, cmd* cmnd);
int cmdRefund(Player* player, cmd* cmnd);
int cmdTrade(Player* player, cmd* cmnd);
int cmdAuction(Player* player, cmd* cmnd);
int cmdReclaim(Player* player, cmd* cmnd);




// effects.cpp
int cmdEffects(Creature* player, cmd* cmnd);


// ------ everything below this line has not yet been ordered ------ //




// property.cpp
int dmProperties(Player* player, cmd* cmnd);
int cmdProperties(Player* player, cmd* cmnd);
int cmdHouse(Player* player, cmd* cmnd);





// skills.c
int dmSetSkills(Player *admin, cmd* cmnd);
int cmdSkills(Player* player, cmd* cmnd);
int cmdPrepareObject(Player* player, cmd* cmnd);
int cmdUnprepareObject(Player* player, cmd* cmnd);
int cmdCraft(Player* player, cmd* cmnd);
int cmdFish(Player* player, cmd* cmnd);
int cmdReligion(Player* player, cmd* cmnd);


bool isPtester(const Creature* player);
bool isPtester(Socket* sock);



// Combine.c

int sneak(Creature* player, cmd* cmnd);
int cmdMove(Player* player, cmd* cmnd);


int cmdInventory(Player* player, cmd* cmnd);
int cmdDrop(Creature* player, cmd* cmnd);


int cmdTrack(Player* player, cmd* cmnd);
int cmdPeek(Player* player, cmd* cmnd);

int cmdHide(Player* player, cmd* cmnd);
int cmdSearch(Player* player, cmd* cmnd);

int cmdOpen(Player* player, cmd* cmnd);
int cmdClose(Player* player, cmd* cmnd);
int cmdLock(Player* player, cmd* cmnd);
int cmdPickLock(Player* player, cmd* cmnd);

int cmdShoplift(Player* player, cmd* cmnd);
int cmdBackstab(Player* player, cmd* cmnd);
int cmdAmbush(Player* player, cmd* cmnd);


int cmdGive(Creature* player, cmd* cmnd);
int cmdRepair(Player* player, cmd* cmnd);

int cmdCircle(Player* player, cmd* cmnd);
int cmdBash(Player* player, cmd* cmnd);
int cmdKick(Player* player, cmd* cmnd);
int cmdMaul(Player* player, cmd* cmnd);
int cmdTalk(Player* player, cmd* cmnd);


int cmdBribe(Player* player, cmd* cmnd);
int cmdFrenzy(Player* player, cmd* cmnd);
int cmdPray(Player* player, cmd* cmnd);
int cmdBerserk(Player* player, cmd* cmnd);
int cmdBloodsacrifice(Player* player, cmd* cmnd);
int cmdUse(Player* player, cmd* cmnd);
int cmdCommune(Player* player, cmd* cmnd);
int cmdBandage(Player* player, cmd* cmnd);

int ply_bounty(Creature* player, cmd* cmnd);



int cmdHypnotize(Player* player, cmd* cmnd);
int cmdMeditate(Player* player, cmd* cmnd);
int cmdTouchOfDeath(Player* player, cmd* cmnd);
int cmdScout(Player* player, cmd* cmnd);





int cmdBite(Player* player, cmd* cmnd);
int cmdDisarm(Player* player, cmd* cmnd);
int cmdEndurance(Player* player, cmd* cmnd);
int cmdCharm(Player* player, cmd* cmnd);
int cmdEnthrall(Player* player, cmd* cmnd);
int cmdIdentify(Player* player, cmd* cmnd);
int cmdMist(Player* player, cmd* cmnd);
int cmdUnmist(Player* player, cmd* cmnd);
int cmdRegenerate(Player* player, cmd* cmnd);
int cmdCreepingDoom(Player* player, cmd* cmnd);
int cmdPoison(Player* player, cmd* cmnd);
int cmdEarthSmother(Player* player, cmd* cmnd);
int cmdDrainLife(Player* player, cmd* cmnd);
int cmdFocus(Player* player, cmd* cmnd);
int cmdBarkskin(Player* player, cmd* cmnd);

int cmdEnvenom(Player* player, cmd* cmnd);
int cmdLayHands(Player* player, cmd* cmnd);
int cmdHarmTouch(Player* player, cmd* cmnd);
//int watcher_send(Creature* player, cmd* cmnd);
int cmdGamble(Player* player, cmd* cmnd);

int cmdMistbane(Player* player, cmd* cmnd);




// Duel.c
int cmdDuel(Player* player, cmd* cmnd);

// Equipment.c
void finishDropObject(Object* object, BaseRoom* room, Creature* player, bool cash=false, bool printPlayer=true, bool printRoom=true);
int cmdGet(Creature* player, cmd* cmnd);
int cmdCost(Player* player, cmd* cmnd);


// Gag.c
int cmdGag(Player* player, cmd* cmnd);

// Group.c
int cmdFollow(Player* player, cmd* cmnd);
int cmdLose(Player* player, cmd* cmnd);
int cmdGroup(Player* player, cmd* cmnd);

// Guilds.c
int cmdGuild(Player* player, cmd* cmnd);
int cmdGuildSend(Player* player, cmd* cmnd);
int cmdGuildHall(Player* player, cmd* cmnd);
int dmListGuilds(Player* player, cmd* cmnd);
void doGuildSend(const Guild* guild, Player* player, bstring txt);

// Lottery.c
int cmdClaim(Player* player, cmd* cmnd);
int cmdLottery(Player* player, cmd* cmnd);

// Magic1.c

// Magic3.c
int cmdTurn(Player* player, cmd* cmnd);
int cmdRenounce(Player* player, cmd* cmnd);
int cmdHolyword(Player* player, cmd* cmnd);

// Magic6.c
int conjureCmd(Player* player, cmd* cmnd);
int animateDeadCmd(Player* player, cmd* cmnd);

// Magic9.c
int cmdEnchant(Player* player, cmd* cmnd);
int cmdTransmute(Player* player, cmd* cmnd);

// Mccp.c
int mccp(Player* player, cmd* cmnd);


// Missile.c
int shoot(Creature* player, cmd* cmnd);

// Threat.cpp
int cmdTarget(Player* player, cmd* cmnd);
int cmdAssist(Player* player, cmd* cmnd);

// Clans.c
int cmdPledge(Player* player, cmd* cmnd);
int cmdRescind(Player* player, cmd* cmnd);

// cmd.c
int cmdProcess(Creature *user, cmd* cmnd, Creature* pet=NULL);

// specials.c
int dmSpecials(Player* player, cmd* cmnd);

// startlocs.cpp
int dmStartLocs(Player* player, cmd* cmnd);

// quests.c
int cmdTalkNew(Player* player, cmd* cmnd);
int cmdQuests(Player* player, cmd* cmnd);

// web.cpp
int dmFifo(Player* player, cmd* cmnd);
int cmdForum(Player* player, cmd* cmnd);


#endif /*COMMANDS_H_*/
