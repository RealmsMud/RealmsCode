/*
 * dm.h
 *   Staff unction prototypes.
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
#ifndef DMH_H
#define DMH_H

#include "catRef.hpp"
#include "range.hpp"

class cmd;
class AreaRoom;
class Creature;
class Object;
class Player;
class UniqueRoom;

int dmAlchemyList(const std::shared_ptr<Player>& player, cmd* cmnd);

int dmRecipes(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmCatRefInfo(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmFishing(const std::shared_ptr<Player>& player, cmd* cmnd);

// area.c
int dmListArea(const std::shared_ptr<Player>& player, cmd* cmnd);

// builder.c
int dmRange(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmMakeBuilder(const std::shared_ptr<Player>& player, cmd* cmnd);
// The following two are auth functions
bool builderObj(const std::shared_ptr<Creature> & player);
bool builderMob(const std::shared_ptr<Creature> & player);

// bans.c
int dmListbans(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmBan(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmUnban(const std::shared_ptr<Player>& player, cmd* cmnd);

int dmShowFactions(const std::shared_ptr<Player>& player, cmd* cmnd);

int dmQueryShips(const std::shared_ptr<Player>& player, cmd* cmnd);

// craft.cpp
int dmCombine(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmSetRecipe(const std::shared_ptr<Player>& player, cmd* cmnd);

// memory.c
int dmMemory(const std::shared_ptr<Player>& player, cmd* cmnd);

int dmGag(const std::shared_ptr<Player>& player, cmd* cmnd);

int dmReadmail(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmDeletemail(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmDeletehist(const std::shared_ptr<Player>& player, cmd* cmnd);

int dmLottery(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmSpelling(const std::shared_ptr<Player>& player, cmd* cmnd);

int dmApproveGuild(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmRejectGuild(const std::shared_ptr<Player>& player, cmd* cmnd);

int dmSkills(const std::shared_ptr<Player>& player, cmd* cmnd);
//int dmAddSkills(const std::shared_ptr<Player>& player, cmd* cmnd);

// Players.c
int dmShowClasses(const std::shared_ptr<Player>& admin, cmd* cmnd);
int dmShowRaces(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmShowDeities(const std::shared_ptr<Player>& player, cmd* cmnd);

// spells.cpp
int dmSpellList(const std::shared_ptr<Player>& player, cmd* cmnd);

// songs.cpp
int dmSongList(const std::shared_ptr<Player>& player, cmd* cmnd);

// talk.c
int dmTalk(const std::shared_ptr<Player>& player, cmd* cmnd);

int dmUnique(const std::shared_ptr<Player>& player, cmd* cmnd);
// dm.c
int dmCache(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmTxtOnCrash(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmReboot(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmMobInventory(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmSockets(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmLoad(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmSave(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmTeleport(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmUsers(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmFlushSave(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmShutdown(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmFlushCrtObj(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmResave(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmPerm(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmInvis(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmIncog(const std::shared_ptr<Player>& player, cmd *cmd);
int dmAc(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmWipe(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmDeleteDb(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmGameStatus(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmWeather(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmQuestList(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmClanList(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmBane(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmHelp(const std::shared_ptr<Player>& player, cmd* cmnd);
int bhHelp(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmParam(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmOutlaw(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmBroadecho(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmCast(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmSet(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmLog(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmList(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmInfo(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmMd5(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmIds(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmStat(const std::shared_ptr<Player>& player, cmd* cmnd);


// dmcrt.c
int dmCreateMob(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmSetCrt(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmCrtName(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmAlias(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmFollow(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmAttack(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmListEnemy(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmListCharm(const std::shared_ptr<Player>& player, cmd* cmnd);
void dmSaveMob(const std::shared_ptr<Player>& player, cmd* cmnd, const CatRef& cr);
int dmAddMob(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmForceWander(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmBalance(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmAlignment(const std::shared_ptr<Player>& player, cmd* cmnd);


// dmobj.c
int dmCreateObj(const std::shared_ptr<Player>& player, cmd* cmnd);
int stat_obj(const std::shared_ptr<Player>& player, const std::shared_ptr<Object>&  object);
int dmSetObj(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmObjName(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmAddObj(const std::shared_ptr<Player>& player, cmd* cmnd);
void dmSaveObj(const std::shared_ptr<Player>& player, cmd* cmnd, const CatRef& cr);
void dmResaveObject(const std::shared_ptr<Player>& player, const std::shared_ptr<Object>&  object, bool flush=false);
int dmSize(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmClone(const std::shared_ptr<Player>& player, cmd* cmnd);


// dmply.c
std::string dmLastCommand(const std::shared_ptr<const Player> &player);
int dmForce(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmSpy(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmSilence(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmTitle(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmSurname(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmGroup(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmDust(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmFlash(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmAward(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmBeep(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmAdvance(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmFinger(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmDisconnect(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmTake(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmRemove(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmComputeLuck(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmPut(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmMove(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmWordAll(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmRename(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmPassword(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmRestorePlayer(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmBank(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmInventoryValue(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmWarn(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmBugPlayer(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmKillSwitch(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmRepair(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmMax(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmBackupPlayer(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmChangeStats(const std::shared_ptr<Player>& admin, cmd* cmnd);
int dmJailPlayer(const std::shared_ptr<Player>& admin, cmd* cmnd);
int dmLts(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmLtClear(const std::shared_ptr<Player>& player, cmd* cmnd);


// dmroom.cpp
bool isCardinal(std::string_view xname);
std::string opposite_exit_name(const std::string &name);
int dmPurge(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmEcho(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmReloadRoom(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmResetPerms(const std::shared_ptr<Player>& admin, cmd* cmnd);
int stat_rom(const std::shared_ptr<Player>& player, const std::shared_ptr<AreaRoom>& room);
int stat_rom(const std::shared_ptr<Player>& player, const std::shared_ptr<UniqueRoom>& room);
int dmAddRoom(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmSetRoom(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmSetExit(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmReplace(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmDelete(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmNameRoom(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmAppend(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmPrepend(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmMobList(const std::shared_ptr<Player>& admin, cmd* cmnd);
int dmWrap(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmDeleteAllExits(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmArrangeExits(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmFixExit(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmUnfixExit(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmRenameExit(const std::shared_ptr<Player>& admin, cmd* cmnd);
int dmDestroyRoom(const std::shared_ptr<Player>& player, cmd* cmnd);
void findRoomsWithFlag(const std::shared_ptr<Player>& player, const Range& range, int flag);
void findRoomsWithFlag(const std::shared_ptr<Player>& player, CatRef area, int flag);
CatRef findNextEmpty(const std::string &type, const std::string &area);
int dmFind(const std::shared_ptr<Player>& player, cmd* cmnd);

// watchers.c
int dmCheckStats(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmLocatePlayer(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmWatcherBroad(const std::shared_ptr<Player>& admin, cmd* cmnd);
int reportTypo(const std::shared_ptr<Player>& player, cmd* cmnd);
int reportBug(const std::shared_ptr<Player>& player, cmd* cmnd);


// swap.cpp
int dmSwap(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmRoomSwap(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmObjSwap(const std::shared_ptr<Player>& player, cmd* cmnd);
int dmMobSwap(const std::shared_ptr<Player>& player, cmd* cmnd);



#endif
