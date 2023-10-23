/*
 * commands.h
 *   Various command prototypes
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

#include <string>
#include <memory>
#include "global.hpp"

class BaseRoom;
class Creature;
class cmd;
class Exit;
class Guild;
class Monster;
class MudObject;
class Object;
class Player;
class Socket;

int orderPet(const std::shared_ptr<Player>& player, cmd* cmnd);


// Effects.cpp
int dmEffectList(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmShowEffectsIndex(const std::shared_ptr<Player>& player, cmd* cmnd);

// songs.cpp
int cmdPlay(const std::shared_ptr<Player>& player, cmd* cmnd);

// alchemy.cpp
int cmdBrew(const std::shared_ptr<Player>& player, cmd* cmnd);


int cmdWeapons(const std::shared_ptr<Player>& player, cmd* cmnd);

// proxy.c
int cmdProxy(const std::shared_ptr<Player>& player, cmd* cmnd);

// action.c
int plyAction(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdAction(const std::shared_ptr<Creature>& player, cmd* cmnd);
bool isBadSocial(const std::string& str);
bool isSemiBadSocial(const std::string& str);
bool isGoodSocial(const std::string& str);

// attack.c
int cmdAttack(const std::shared_ptr<Creature>& player, cmd* cmnd);

// bank.c
int cmdBalance(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdDeposit(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdWithdraw(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdTransfer(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdStatement(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdDeleteStatement(const std::shared_ptr<Player>& player, cmd* cmnd);


// color.c
int cmdColors(const std::shared_ptr<Player>& player, cmd* cmnd);

// command2.c
int cmdTraffic(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdRoominfo(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdCondition(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdLook(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdKnock(const std::shared_ptr<Creature>& player, cmd* cmnd);
int cmdThrow(const std::shared_ptr<Creature>& creature, cmd* cmnd);

// command1.c
int cmdNoExist(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdNoAuth(const std::shared_ptr<Player>& player);
void command(std::shared_ptr<Socket> sock, const std::string& str);
void parse(std::string_view str, cmd* cmnd);

int cmdPush(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdPull(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdPress(const std::shared_ptr<Player>& player, cmd* cmnd);


// command4.c
int cmdScore(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdDaily(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdChecksaves(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdHelp(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdWiki(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdWelcome(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdAge(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdVersion(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdLevelHistory(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdStatistics(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdInfo(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdSpells(const std::shared_ptr<Creature>& player, cmd* cmnd);
void spellsUnder(const std::shared_ptr<Player>& viewer, const std::shared_ptr<Creature> & target, bool notSelf);


// command5.c
int cmdWho(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdClasswho(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdWhois(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdSuicide(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdConvert(const std::shared_ptr<Player>& player, cmd* cmnd);
int flag_list(const std::shared_ptr<Creature>& player, cmd* cmnd);
int cmdPrefs(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdTelOpts(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdQuit(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdChangeStats(const std::shared_ptr<Player>& player, cmd* cmnd);
void changingStats(std::shared_ptr<Socket> sock, const std::string& str );


// command7.c
int cmdTrain(const std::shared_ptr<Player>& player, cmd* cmnd);


// command8.c
char *timestr(long t);
int cmdTime(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdSave(const std::shared_ptr<Player>& player, cmd* cmnd);


// command10.c
int cmdBreak(const std::shared_ptr<Player>& player, cmd* cmnd);


// command11.c
std::string doFinger(const std::shared_ptr<Player>& player, std::string name, CreatureClass cls);
int cmdFinger(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdPayToll(const std::shared_ptr<Player>& player, cmd* cmnd);
unsigned long tollcost(const std::shared_ptr<const Player>& player, const std::shared_ptr<Exit> exit, std::shared_ptr<Monster>  keeper);
int infoGamestat(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdDescription(const std::shared_ptr<Player>& player, cmd* cmnd);
int hire(const std::shared_ptr<Player>& player, cmd* cmnd);


// communication.c
std::string getFullstrTextTrun(std::string str, int skip, char toSkip = ' ', bool colorEscape=false);
std::string getFullstrText(std::string str, int skip, char toSkip = ' ', bool colorEscape=false, bool truncate=false);
int communicateWith(const std::shared_ptr<Player>& player, cmd* cmnd);
int pCommunicate(const std::shared_ptr<Player>& player, cmd* cmnd);
int communicate(const std::shared_ptr<Creature>& player, cmd* cmnd);
int channel(const std::shared_ptr<Player>& player, cmd* cmnd );
int cmdIgnore(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdSpeak(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdLanguages(const std::shared_ptr<Player>& player, cmd* cmnd);

// commerce.c
int cmdShop(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdList(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdPurchase(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdSelection(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdBuy(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdSell(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdValue(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdRefund(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdTrade(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdAuction(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdReclaim(const std::shared_ptr<Player>& player, cmd* cmnd);


// demographics.cpp
int cmdDemographics(const std::shared_ptr<Player>& player, cmd* cmnd);

// effects.cpp
int cmdEffects(const std::shared_ptr<Creature>& player, cmd* cmnd);


// ------ everything below this line has not yet been ordered ------ //




// property.cpp
int dmProperties(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdProperties(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdHouse(const std::shared_ptr<Player>& player, cmd* cmnd);





// skills.c
int dmSetSkills(const std::shared_ptr<Player>& admin, cmd* cmnd);
int cmdSkills(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdPrepareObject(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdUnprepareObject(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdCraft(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdFish(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdReligion(const std::shared_ptr<Player>& player, cmd* cmnd);


bool isPtester(const std::shared_ptr<Creature> & player);
bool isPtester(std::shared_ptr<Socket> sock);



// Combine.c

int sneak(const std::shared_ptr<Creature>& player, cmd* cmnd);
int cmdMove(const std::shared_ptr<Player>& player, cmd* cmnd);


int cmdInventory(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdDrop(const std::shared_ptr<Creature>& player, cmd* cmnd);


int cmdTrack(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdPeek(const std::shared_ptr<Player>& player, cmd* cmnd);

int cmdHide(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdSearch(const std::shared_ptr<Player>& player, cmd* cmnd);

int cmdOpen(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdClose(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdLock(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdPickLock(const std::shared_ptr<Player>& player, cmd* cmnd);

int cmdShoplift(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdBackstab(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdAmbush(const std::shared_ptr<Player>& player, cmd* cmnd);


int cmdGive(const std::shared_ptr<Creature>& player, cmd* cmnd);
int cmdRepair(const std::shared_ptr<Player>& player, cmd* cmnd);

int cmdCircle(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdBash(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdKick(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdMaul(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdTalk(const std::shared_ptr<Player>& player, cmd* cmnd);


int cmdBribe(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdFrenzy(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdPray(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdBerserk(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdBloodsacrifice(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdUse(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdCommune(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdBandage(const std::shared_ptr<Player>& player, cmd* cmnd);

int ply_bounty(const std::shared_ptr<Creature>& player, cmd* cmnd);



int cmdHypnotize(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdMeditate(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdTouchOfDeath(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdScout(const std::shared_ptr<Player>& player, cmd* cmnd);






int cmdBite(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdDisarm(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdCharm(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdEnthrall(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdIdentify(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdMist(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdUnmist(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdRegenerate(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdCreepingDoom(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdPoison(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdEarthSmother(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdStarstrike(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdDrainLife(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdFocus(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdBarkskin(const std::shared_ptr<Player>& player, cmd* cmnd);

int cmdEnvenom(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdLayHands(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdHarmTouch(const std::shared_ptr<Player>& player, cmd* cmnd);
//int watcher_send(const std::shared_ptr<Creature>& player, cmd* cmnd);
int cmdGamble(const std::shared_ptr<Player>& player, cmd* cmnd);

int cmdMistbane(const std::shared_ptr<Player>& player, cmd* cmnd);




// Duel.c
int cmdDuel(const std::shared_ptr<Player>& player, cmd* cmnd);

// Equipment.c
int cmdCompare(const std::shared_ptr<Player>& player, cmd* cmnd);
void finishDropObject(const std::shared_ptr<Object>&  object, const std::shared_ptr<BaseRoom>& room, const std::shared_ptr<Creature>& player, bool cash=false, bool printPlayer=true, bool printRoom=true);
int cmdGet(const std::shared_ptr<Creature>& player, cmd* cmnd);
int cmdCost(const std::shared_ptr<Player>& player, cmd* cmnd);


// Gag.c
int cmdGag(const std::shared_ptr<Player>& player, cmd* cmnd);

// Group.c
int cmdFollow(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdLose(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdGroup(const std::shared_ptr<Player>& player, cmd* cmnd);

// Guilds.c
int cmdGuild(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdGuildSend(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdGuildHall(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmListGuilds(const std::shared_ptr<Player>& player, cmd* cmnd);
void doGuildSend(const Guild* guild, std::shared_ptr<Player> player, std::string txt);

// Lottery.c
int cmdClaim(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdLottery(const std::shared_ptr<Player>& player, cmd* cmnd);

// Magic1.c

// Magic3.c
int cmdTurn(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdRenounce(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdHolyword(const std::shared_ptr<Player>& player, cmd* cmnd);

// Magic6.c
int conjureCmd(const std::shared_ptr<Player>& player, cmd* cmnd);
int animateDeadCmd(const std::shared_ptr<Player>& player, cmd* cmnd);

// Magic9.c
int cmdEnchant(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdTransmute(const std::shared_ptr<Player>& player, cmd* cmnd);

// Mccp.c
int mccp(const std::shared_ptr<Player>& player, cmd* cmnd);


// Missile.c
int shoot(const std::shared_ptr<Creature>& player, cmd* cmnd);

// Threat.cpp
int cmdTarget(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdAssist(const std::shared_ptr<Player>& player, cmd* cmnd);

// Clans.c
int cmdPledge(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdRescind(const std::shared_ptr<Player>& player, cmd* cmnd);

// cmd.c
int cmdProcess(std::shared_ptr<Creature>user, cmd* cmnd, const std::shared_ptr<Creature>& pet=nullptr);

// socials.cpp
int cmdSocial(const std::shared_ptr<Creature>& creature, cmd* cmnd);

// specials.c
int dmSpecials(const std::shared_ptr<Player>& player, cmd* cmnd);

// startlocs.cpp
int dmStartLocs(const std::shared_ptr<Player>& player, cmd* cmnd);

// quests.c
int cmdTalkNew(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdQuests(const std::shared_ptr<Player>& player, cmd* cmnd);

// web.cpp
int dmFifo(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdForum(const std::shared_ptr<Player>& player, cmd* cmnd);

// update.cpp
int list_act(const std::shared_ptr<Player>& player, cmd* cmnd);


// Somewhere
int cmdPrepare(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdTitle(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdReconnect(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdWear(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdRemoveObj(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdEquipment(const std::shared_ptr<Player>& creature, cmd* cmnd);
int cmdReady(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdHold(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdSecond(const std::shared_ptr<Player>& player, cmd* cmnd);

int cmdCast(const std::shared_ptr<Creature>& creature, cmd* cmnd);
int cmdTeach(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdStudy(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdReadScroll(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdConsume(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdUseWand(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdUnlock(const std::shared_ptr<Player>& player, cmd* cmnd);

int checkBirthdays(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdFlee(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdPrepareForTraps(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdSteal(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdRecall(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdPassword(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdSongs(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdSurname(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdVisible(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdDice(const std::shared_ptr<Creature>& player, cmd* cmnd);
int cmdChooseAlignment(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdKeep(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdUnkeep(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdLabel(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdGo(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdSing(const std::shared_ptr<Creature>& creature, cmd* cmnd);



// refuse.cpp
int cmdRefuse(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdWatch(const std::shared_ptr<Player>& player, cmd* cmnd);

// data.cpp
int cmdRecipes(const std::shared_ptr<Player>& player, cmd* cmnd);

// faction.cpp
int cmdFactions(const std::shared_ptr<Player>& player, cmd* cmnd);


// post.cpp
int cmdSendMail(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdReadMail(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdDeleteMail(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdEditHistory(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdHistory(const std::shared_ptr<Player>& player, cmd* cmnd);
int cmdDeleteHistory(const std::shared_ptr<Player>& player, cmd* cmnd);

// weaponless.cpp
int cmdHowl(const std::shared_ptr<Creature>& player, cmd* cmnd);



int dmStatDetail(const std::shared_ptr<Player>& player, cmd* cmnd);

void doCastPython(std::shared_ptr<MudObject> caster, const std::shared_ptr<Creature>& target, std::string_view spell, int strength = 130);
