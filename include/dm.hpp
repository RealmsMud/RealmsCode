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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
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

int dmAlchemyList(Player* player, cmd* cmnd);

int dmRecipes(Player* player, cmd* cmnd);
int dmCatRefInfo(Player* player, cmd* cmnd);
int dmFishing(Player* player, cmd* cmnd);

// area.c
int dmListArea(Player* player, cmd* cmnd);

// builder.c
int dmRange(Player* player, cmd* cmnd);
int dmMakeBuilder(Player* player, cmd* cmnd);
// The following two are auth functions
bool builderObj(const Creature* player);
bool builderMob(const Creature* player);

// bans.c
int dmListbans(Player* player, cmd* cmnd);
int dmBan(Player* player, cmd* cmnd);
int dmUnban(Player* player, cmd* cmnd);

int dmShowFactions(Player *player, cmd* cmnd);

int dmQueryShips(Player* player, cmd* cmnd);

// craft.cpp
int dmCombine(Player* player, cmd* cmnd);
int dmSetRecipe(Player* player, cmd* cmnd);

// memory.c
int dmMemory(Player* player, cmd* cmnd);

int dmGag(Player* player, cmd* cmnd);

int dmReadmail(Player* player, cmd* cmnd);
int dmDeletemail(Player* player, cmd* cmnd);
int dmDeletehist(Player* player, cmd* cmnd);

int dmLottery(Player* player, cmd* cmnd);
int dmSpelling(Player* player, cmd* cmnd);

int dmApproveGuild(Player* player, cmd* cmnd);
int dmRejectGuild(Player* player, cmd* cmnd);

int dmSkills(Player* player, cmd* cmnd);
//int dmAddSkills(Player* player, cmd* cmnd);

// Players.c
int dmShowClasses(Player* admin, cmd* cmnd);
int dmShowRaces(Player* player, cmd* cmnd);
int dmShowDeities(Player* player, cmd* cmnd);

// spells.cpp
int dmSpellList(Player* player, cmd* cmnd);

// songs.cpp
int dmSongList(Player* player, cmd* cmnd);

// talk.c
int dmTalk(Player* player, cmd* cmnd);

int dmUnique(Player* player, cmd* cmnd);
// dm.c
int dmCache(Player* player, cmd* cmnd);
int dmTxtOnCrash(Player* player, cmd* cmnd);
int dmReboot(Player* player, cmd* cmnd);
int dmMobInventory(Player* player, cmd* cmnd);
int dmSockets(Player* player, cmd* cmnd);
int dmLoad(Player* player, cmd* cmnd);
int dmSave(Player* player, cmd* cmnd);
int dmTeleport(Player* player, cmd* cmnd);
int dmUsers(Player* player, cmd* cmnd);
int dmFlushSave(Player* player, cmd* cmnd);
int dmShutdown(Player* player, cmd* cmnd);
int dmFlushCrtObj(Player* player, cmd* cmnd);
int dmResave(Player* player, cmd* cmnd);
int dmPerm(Player* player, cmd* cmnd);
int dmInvis(Player* player, cmd* cmnd);
int dmIncog(Player* player, cmd *cmd);
int dmAc(Player* player, cmd* cmnd);
int dmWipe(Player* player, cmd* cmnd);
int dmDeleteDb(Player* player, cmd* cmnd);
int dmGameStatus(Player* player, cmd* cmnd);
int dmWeather(Player* player, cmd* cmnd);
int dmQuestList(Player* player, cmd* cmnd);
int dmClanList(Player* player, cmd* cmnd);
int dmBane(Player* player, cmd* cmnd);
int dmHelp(Player* player, cmd* cmnd);
int bhHelp(Player* player, cmd* cmnd);
int dmParam(Player* player, cmd* cmnd);
int dmOutlaw(Player* player, cmd* cmnd);
int dmBroadecho(Player* player, cmd* cmnd);
int dmCast(Player* player, cmd* cmnd);
int dmView(Player* player, cmd* cmnd);
int dmSet(Player* player, cmd* cmnd);
int dmLog(Player* player, cmd* cmnd);
int dmList(Player* player, cmd* cmnd);
int dmInfo(Player* player, cmd* cmnd);
int dmMd5(Player* player, cmd* cmnd);
int dmIds(Player* player, cmd* cmnd);
int dmStat(Player* player, cmd* cmnd);


// dmcrt.c
int dmCreateMob(Player* player, cmd* cmnd);
int dmSetCrt(Player* player, cmd* cmnd);
int dmCrtName(Player* player, cmd* cmnd);
int dmAlias(Player* player, cmd* cmnd);
int dmFollow(Player* player, cmd* cmnd);
int dmAttack(Player* player, cmd* cmnd);
int dmListEnemy(Player* player, cmd* cmnd);
int dmListCharm(Player* player, cmd* cmnd);
void dmSaveMob(Player* player, cmd* cmnd, const CatRef& cr);
int dmAddMob(Player* player, cmd* cmnd);
int dmForceWander(Player* player, cmd* cmnd);
int dmBalance(Player* player, cmd* cmnd);


// dmobj.c
int dmCreateObj(Player* player, cmd* cmnd);
int stat_obj(Player* player, Object* object);
int dmSetObj(Player* player, cmd* cmnd);
int dmObjName(Player* player, cmd* cmnd);
int dmAddObj(Player* player, cmd* cmnd);
void dmSaveObj(Player* player, cmd* cmnd, const CatRef& cr);
void dmResaveObject(const Player* player, Object* object, bool flush=false);
int dmSize(Player* player, cmd* cmnd);
int dmClone(Player* player, cmd* cmnd);


// dmply.c
bstring dmLastCommand(const Player* player);
int dmForce(Player* player, cmd* cmnd);
int dmSpy(Player* player, cmd* cmnd);
int dmSilence(Player* player, cmd* cmnd);
int dmTitle(Player* player, cmd* cmnd);
int dmSurname(Player* player, cmd* cmnd);
int dmGroup(Player* player, cmd* cmnd);
int dmDust(Player* player, cmd* cmnd);
int dmFlash(Player* player, cmd* cmnd);
int dmAward(Player* player, cmd* cmnd);
int dmBeep(Player* player, cmd* cmnd);
int dmAdvance(Player* player, cmd* cmnd);
int dmFinger(Player* player, cmd* cmnd);
int dmDisconnect(Player* player, cmd* cmnd);
int dmTake(Player* player, cmd* cmnd);
int dmRemove(Player* player, cmd* cmnd);
int dmPut(Player* player, cmd* cmnd);
int dmMove(Player* player, cmd* cmnd);
int dmWordAll(Player* player, cmd* cmnd);
int dmRename(Player* player, cmd* cmnd);
int dmPassword(Player* player, cmd* cmnd);
int dmRestorePlayer(Player* player, cmd* cmnd);
int dmBank(Player* player, cmd* cmnd);
int dmInventoryValue(Player* player, cmd* cmnd);
int dmProxy(Player* player, cmd* cmnd);
int dmWarn(Player* player, cmd* cmnd);
int dmBugPlayer(Player* player, cmd* cmnd);
int dmKillSwitch(Player* player, cmd* cmnd);
int dmRepair(Player* player, cmd* cmnd);
int dmMax(Player* player, cmd* cmnd);
int dmBackupPlayer(Player* player, cmd* cmnd);
int dmChangeStats(Player *admin, cmd* cmnd);
int dmJailPlayer(Player *admin, cmd* cmnd);
int dmLts(Player* player, cmd* cmnd);
int dmLtClear(Player* player, cmd* cmnd);
int dm2x(Player* player, cmd* cmnd);


// dmroom.cpp
bool isCardinal(const bstring& xname);
bstring opposite_exit_name(const bstring& name);
int dmPurge(Player* player, cmd* cmnd);
int dmEcho(Player* player, cmd* cmnd);
int dmReloadRoom(Player* player, cmd* cmnd);
int dmResetPerms(Player *admin, cmd* cmnd);
int stat_rom(Player* player, AreaRoom* room);
int stat_rom(Player* player, UniqueRoom* room);
int dmAddRoom(Player* player, cmd* cmnd);
int dmSetRoom(Player* player, cmd* cmnd);
int dmSetExit(Player* player, cmd* cmnd);
int dmReplace(Player* player, cmd* cmnd);
int dmDelete(Player* player, cmd* cmnd);
int dmNameRoom(Player* player, cmd* cmnd);
int dmAppend(Player* player, cmd* cmnd);
int dmPrepend(Player* player, cmd* cmnd);
int dmMobList(Player *admin, cmd* cmnd);
int dmWrap(Player* player, cmd* cmnd);
int dmDeleteAllExits(Player* player, cmd* cmnd);
int dmArrangeExits(Player* player, cmd* cmnd);
int dmFixExit(Player* player, cmd* cmnd);
int dmUnfixExit(Player* player, cmd* cmnd);
int dmRenameExit(Player *admin, cmd* cmnd);
int dmDestroyRoom(Player* player, cmd* cmnd);
void findRoomsWithFlag(const Player* player, const Range& range, int flag);
void findRoomsWithFlag(const Player* player, CatRef area, int flag);
CatRef findNextEmpty(const bstring& type, const bstring& area);
int dmFind(Player* player, cmd* cmnd);

// watchers.c
int dmCheckStats(Player* player, cmd* cmnd);
int dmLocatePlayer(Player* player, cmd* cmnd);
int dmWatcherBroad(Player *admin, cmd* cmnd);
int reportTypo(Player* player, cmd* cmnd);


// swap.cpp
int dmSwap(Player* player, cmd* cmnd);
int dmRoomSwap(Player* player, cmd* cmnd);
int dmObjSwap(Player* player, cmd* cmnd);
int dmMobSwap(Player* player, cmd* cmnd);



#endif
