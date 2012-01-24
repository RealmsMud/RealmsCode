/*
 * dmroom.cpp
 *	 Staff functions related to rooms.
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
#include "mud.h"
#include "commands.h"
#include "dm.h"
#include "factions.h"
#include "property.h"
#include "ships.h"
#include "tokenizer.h"
#include "effects.h"
#include "traps.h"

#include <signal.h>
#include <dirent.h>
#include <iomanip>


//*********************************************************************
//							checkTeleportRange
//*********************************************************************

void checkTeleportRange(const Player* player, CatRef cr) {
	// No warning for the test range
	if(cr.isArea("test"))
		return;

	const CatRefInfo* cri = gConfig->getCatRefInfo(cr.area);
	if(!cri) {
		player->printColor("^yNo CatRefInfo zone found for this room's area. Contact a dungeonmaster to fix this.\n");
		return;
	}

	if(cr.id > cri->getTeleportWeight()) {
		player->printColor("^yThis room is outside the CatRefInfo zone's teleport range.\n");
		return;
	}
}


//*********************************************************************
//							isCardinal
//*********************************************************************

bool isCardinal(bstring xname) {
	return(	xname == "north" ||
			xname == "east" ||
			xname == "south" ||
			xname == "west" ||
			xname == "northeast" ||
			xname == "northwest" ||
			xname == "southeast" ||
			xname == "southwest"
	);
}


//*********************************************************************
//							wrapText
//*********************************************************************

bstring wrapText(const bstring& text, int wrap) {
	if(text == "")
		return("");
	
	bstring wrapped = "";
	int		len = text.length(), i=0, sp=0, spLast=0, spLen=0;
	char	ch, chLast;

	// find our starting position
	while(text.at(i) == ' ' || text.at(i) == '\n' || text.at(i) == '\r')
		i++;

	for(; i < len; i++) {

		ch = text.at(i);

		// convert linebreaks to spaces
		if(ch == '\r')
			ch = ' ';
		if(ch == '\n')
			ch = ' ';

		// skiping 2x spacing (or greater)
		if(ch == ' ' && chLast == ' ') {
			do {
				i++;
			} while(i+1 < len && (text.at(i+1) == ' ' || text.at(i+1) == '\n' || text.at(i+1) == '\r'));
			if(i < len)
				ch = text.at(i);
		}


		// don't add trailing spaces
		if(ch != ' ' || i+1 < len) {

			// If there is color in the room description, the color characters
			// shouldn't count toward string length.
			if(ch == '^')
				spLen += 2;

			// wrap
			if(ch == ' ') {
				// We went over! spLast points to the last non-overboard space.
				if(wrap <= (sp - spLen)) {
					wrapped.replace(spLast, 1, "\n");
					spLen = spLast;
				}

				spLast = sp;
			}

			wrapped += ch;
			sp++;
			chLast = ch;
		}
	}
	return(wrapped);
}

//*********************************************************************
//							expand_exit_name
//*********************************************************************

bstring expand_exit_name(const bstring& name) {
	if(name == "n")
		return("north");
	if(name == "s")
		return("south");
	if(name == "e")
		return("east");
	if(name == "w")
		return("west");
	if(name == "sw")
		return("southwest");
	if(name == "nw")
		return("northwest");
	if(name == "se")
		return("southeast");
	if(name == "ne")
		return("northeast");
	if(name == "d")
		return("door");
	if(name == "o")
		return("out");
	if(name == "p")
		return("passage");
	if(name == "t")
		return("trap door");
	if(name == "a")
		return("arch");
	if(name == "g")
		return("gate");
	if(name == "st")
		return("stairs");
	return(name);
}


//*********************************************************************
//							opposite_exit_name
//*********************************************************************

bstring opposite_exit_name(const bstring& name) {
	if(name == "south")
		return("north");
	if(name == "north")
		return("south");
	if(name == "west")
		return("east");
	if(name == "east")
		return("west");
	if(name == "northeast")
		return("southwest");
	if(name == "southeast")
		return("northwest");
	if(name == "northwest")
		return("southeast");
	if(name == "southwest")
		return("northeast");
	if(name == "up")
		return("down");
	if(name == "down")
		return("up");

	return(name);
}




//*********************************************************************
//						dmPurge
//*********************************************************************
// This function allows staff to purge a room of all its objects and
// monsters.

int dmPurge(Player* player, cmd* cmnd) {
	ctag	*cp=0, *ctemp=0, *pet=0;
	otag	*op=0, *otemp=0;
	BaseRoom* room = player->getRoom();

	if(!player->canBuildMonsters() && !player->canBuildObjects())
		return(cmdNoAuth(player));
	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Error: this room is out of your range; you cannot *purge here.\n");
		return(0);
	}

	cp = room->first_mon;
	room->first_mon = 0;
	while(cp) {
		ctemp = cp->next_tag;
		if(cp->crt->isPet()) {
			player->print("Skipping %N.\n", cp->crt);
			// don't kill pets
			if(!room->first_mon) {
				room->first_mon = cp;
				pet = room->first_mon;
			} else {
				pet->next_tag = cp;
				pet = pet->next_tag;
			}
			pet->next_tag = 0;
			cp = ctemp;
			continue;
		}

		if(cp->crt->flagIsSet(M_DM_FOLLOW)) {
		    Player* master = cp->crt->getPlayerMaster();
			if(master) {
				master->clearFlag(P_ALIASING);

				master->setAlias(0);
				master->print("%1M's soul was purged.\n", cp->crt);
				master->delPet(cp->crt->getMonster());
			}
		}
		free_crt(cp->crt);
		delete cp;
		cp = ctemp;
	}

	op = room->first_obj;
	room->first_obj = 0;
	while(op) {
		otemp = op->next_tag;
		if(!op->obj->flagIsSet(O_TEMP_PERM)) {
			delete op->obj;
			delete op;
		}
		op = otemp;
	}

	player->print("Purged.\n");

	if(!player->isDm())
		log_immort(false,player, "%s purged room %s.\n", player->name, player->getRoom()->fullName().c_str());

	return(0);
}


//*********************************************************************
//						dmEcho
//*********************************************************************
// This function allows a staff specified by the socket descriptor in
// the first parameter to echo the rest of their command line to all
// the other people in the room.

int dmEcho(Player* player, cmd* cmnd) {
	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Error: room number not in any of your alotted ranges.\n");
		return(0);
	}

	bstring text = getFullstrText(cmnd->fullstr, 1);
	if(text == "" || Pueblo::is(text)) {
		player->print("Echo what?\n");
		return(0);
	}

	if(!player->isCt())
		broadcast(isStaff, "^G*** %s (%s) echoed: %s",
			player->name, player->getRoom()->fullName().c_str(), text.c_str());

	broadcast(NULL, player->getRoom(), "%s", text.c_str());
	return(0);
}

//*********************************************************************
//						dmReloadRoom
//*********************************************************************
// This function allows a staff to reload a room from disk.

int dmReloadRoom(Player* player, cmd* cmnd) {

	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Error: this room is out of your range; you cannot reload this room.\n");
		return(0);
	}

	if(gConfig->reloadRoom(player->getRoom()))
		player->print("Ok.\n");
	else
		player->print("Reload failed.\n");

	return(0);
}

//*********************************************************************
//						resetPerms
//*********************************************************************
// This function allows a staff to reset perm timeouts in the room

int dmResetPerms(Player* player, cmd* cmnd) {
	std::map<int, crlasttime>::iterator it;
	crlasttime* crtm=0;
	std::map<int, long> tempMonsters;
	std::map<int, long> tempObjects;
	UniqueRoom	*room = player->parent_rom;
	//long	temp_obj[10], temp_mon[10];

	if(!needUniqueRoom(player))
		return(0);

	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Error: this room is out of your range; you cannot reload this room.\n");
		return(0);
	}


	for(it = room->permMonsters.begin(); it != room->permMonsters.end() ; it++) {
		crtm = &(*it).second;
		tempMonsters[(*it).first] = crtm->interval;
		crtm->ltime = time(0);
		crtm->interval = 0;
	}
	for(it = room->permObjects.begin(); it != room->permObjects.end() ; it++) {
		crtm = &(*it).second;
		tempObjects[(*it).first] = crtm->interval;
		crtm->ltime = time(0);
		crtm->interval = 0;
	}

	player->print("Permanent object and creature timeouts reset.\n");
	room->addPermCrt();

	for(it = room->permMonsters.begin(); it != room->permMonsters.end() ; it++) {
		crtm = &(*it).second;
		crtm->interval = tempMonsters[(*it).first];
		crtm->ltime = time(0);
	}
	for(it = room->permObjects.begin(); it != room->permObjects.end() ; it++) {
		crtm = &(*it).second;
		crtm->interval = tempObjects[(*it).first];
		crtm->ltime = time(0);
	}

	log_immort(true, player, "%s reset perm timeouts in room %s\n", player->name, player->getRoom()->fullName().c_str());

	if(gConfig->resaveRoom(room->info) < 0)
		player->print("Room fail saved.\n");
	else
		player->print("Room saved.\n");

	return(0);
}

//*********************************************************************
//						stat_rom_exits
//*********************************************************************
// Display information on room given to staff.

void stat_rom_exits(Creature* player, BaseRoom* room) {
	xtag	*xp=0;
	Exit	*exit=0;
	char	str[1024], temp[25], tempstr[32];
	int		i=0, flagcount=0;
	UniqueRoom*	uRoom = room->getUniqueRoom();

	if(!room->first_ext)
		return;

	player->print("Exits:\n");

	xp = room->first_ext;
	while(xp) {
		exit = xp->ext;
		xp = xp->next_tag;

		if(!exit->getLevel())
			player->print("  %s: ", exit->name);
		else
			player->print("  %s(L%d): ", exit->name, exit->getLevel());

		if(!exit->target.mapmarker.getArea())
			player->printColor("%s ", exit->target.room.str(uRoom ? uRoom->info.area : "", 'y').c_str());
		else
			player->print(" A:%d X:%d Y:%d Z:%d  ",
				exit->target.mapmarker.getArea(), exit->target.mapmarker.getX(),
				exit->target.mapmarker.getY(), exit->target.mapmarker.getZ());

		*str = 0;
		strcpy(str, "Flags: ");


		for(i=0; i<MAX_EXIT_FLAGS; i++) {
			if(exit->flagIsSet(i)) {
				sprintf(tempstr, "%s(%d), ", get_xflag(i), i+1);
				strcat(str, tempstr);
				flagcount++;
			}
		}

		if(flagcount) {
			str[strlen(str) - 2] = '.';
			str[strlen(str) - 1] = 0;
		}


		if(flagcount)
			player->print("%s", str);


		if(exit->flagIsSet(X_LOCKABLE)) {
			player->print(" Key#: %d ", exit->getKey());
			if(exit->getKeyArea() != "")
				player->printColor(" Area: ^y%s^x ", exit->getKeyArea().c_str());
		}

		if(exit->flagIsSet(X_TOLL_TO_PASS))
			player->print(" Toll: %d ", exit->getToll());

		player->print("\n");

		if(exit->getDescription() != "")
			player->print("    Description: \"%s\"\n", exit->getDescription().c_str());

		if(	(exit->flagIsSet(X_CAN_LOOK) || exit->flagIsSet(X_LOOK_ONLY)) &&
			exit->flagIsSet(X_NO_SCOUT)
		)
			player->printColor("^rExit is flagged as no-scout, but it flagged as lookable.\n");


		if(exit->flagIsSet(X_PLEDGE_ONLY)) {
			for(i=1; i<15; i++)
				if(exit->flagIsSet(i+40)) {
					sprintf(temp, "Clan: %d, ",i);
					strcat(str, temp);
				}
			player->print("    Clan: %s\n", temp);
		}
		if(exit->flagIsSet(X_PORTAL)) {
			player->printColor("    Owner: ^c%s^x  Uses: ^c%d^x\n", exit->getPassPhrase().c_str(), exit->getKey());
		} else if(exit->getPassPhrase() != "") {
			player->print("    Passphrase: \"%s\"\n", exit->getPassPhrase().c_str());
			if(exit->getPassLanguage())
				player->print("    Passlang: %s\n", get_language_adj(exit->getPassLanguage()));
		}
		if(exit->getEnter() != "")
			player->print("    OnEnter: \"%s\"\n", exit->getEnter().c_str());
		if(exit->getOpen() != "")
			player->print("    OnOpen: \"%s\"\n", exit->getOpen().c_str());

		
		if(exit->getSize() || exit->getDirection()) {
			if(exit->getSize())
				player->print("    Size: %s", getSizeName(exit->getSize()).c_str());
			if(exit->getDirection()) {
				player->print("    Direction: %s", getDirName(exit->getDirection()).c_str());
				if(getDir(exit->name) != NoDirection)
					player->printColor("\n^rThis exit has a direction set, but the exit is a cardinal exit.");
			}
			player->print("\n");
		}

		if(exit->effects.list.size())
			player->printColor("    Effects:\n%s", exit->effects.getEffectsString(player).c_str());
		player->printColor("%s", exit->hooks.display().c_str());
	}
}


//*********************************************************************
//						trainingFlagSet
//*********************************************************************

bool trainingFlagSet(const BaseRoom* room, const TileInfo *tile, const AreaZone *zone, int flag) {
	return(	(room && room->flagIsSet(flag)) ||
			(tile && tile->flagIsSet(flag)) ||
			(zone && zone->flagIsSet(flag))
	);
}

//*********************************************************************
//						whatTraining
//*********************************************************************
// determines what class can train here

int whatTraining(const BaseRoom* room, const TileInfo *tile, const AreaZone *zone, int extra) {
	int i = 0;

	if(R_TRAINING_ROOM - 1 == extra || trainingFlagSet(room, tile, zone, R_TRAINING_ROOM - 1))
		i += 16;
	if(R_TRAINING_ROOM == extra || trainingFlagSet(room, tile, zone, R_TRAINING_ROOM))
		i += 8;
	if(R_TRAINING_ROOM + 1 == extra || trainingFlagSet(room, tile, zone, R_TRAINING_ROOM + 1))
		i += 4;
	if(R_TRAINING_ROOM + 2 == extra || trainingFlagSet(room, tile, zone, R_TRAINING_ROOM + 2))
		i += 2;
	if(R_TRAINING_ROOM + 3 == extra || trainingFlagSet(room, tile, zone, R_TRAINING_ROOM + 3))
		i += 1;

	return(i > CLASS_COUNT - 1 ? 0 : i);
}

int BaseRoom::whatTraining(int extra) const {
	return(::whatTraining(this, (const TileInfo*)0, (const AreaZone*)0, extra));
}


//*********************************************************************
//						showRoomFlags
//*********************************************************************

void showRoomFlags(const Player* player, const BaseRoom* room, const TileInfo *tile, const AreaZone *zone) {
	bool	flags=false;
	int		i=0;
	std::ostringstream oStr;
	oStr << "^@Flags set: ";

	for(; i<MAX_ROOM_FLAGS; i++) {
		if(i >=2 && i <= 6)	// skips training flags
			continue;
		if(	(room && room->flagIsSet(i)) ||
			(tile && tile->flagIsSet(i)) ||
			(zone && zone->flagIsSet(i))
		) {
			if(flags)
				oStr << ", ";
			flags = true;
			oStr << get_rflag(i) << "(" << (int)(i+1) << ")";
		}
	}

	if(!flags)
		oStr << "None";
	oStr << ".^x\n";

	i = whatTraining(room, tile, zone, 0);

	if(i)
		oStr << "^@Training: " << get_class_string(i) << "^x\n";
	player->printColor("%s", oStr.str().c_str());

	// inform user of redundant flags
	if(room && room->getConstUniqueRoom()) {
		int whatTraining = room->whatTraining();
		bool limboOrCoven = room->flagIsSet(R_LIMBO) || room->flagIsSet(R_VAMPIRE_COVEN);

		if(room->flagIsSet(R_NO_TELEPORT)) {
			if(	limboOrCoven ||
				room->flagIsSet(R_JAIL) ||
				room->flagIsSet(R_ETHEREAL_PLANE) ||
				whatTraining
			)
				player->printColor("^rThis room does not need flag 13-No Teleport set.\n");
		}

		if(room->flagIsSet(R_NO_SUMMON_OUT)) {
			if(	room->flagIsSet(R_IS_STORAGE_ROOM) ||
				room->flagIsSet(R_LIMBO)
			)
				player->printColor("^rThis room does not need flag 29-No Summon Out set.\n");
		}

		if(room->flagIsSet(R_NO_LOGIN)) {
			if(	room->flagIsSet(R_LOG_INTO_TRAP_ROOM) ||
				whatTraining
			)
				player->printColor("^rThis room does not need flag 34-No Log set.\n");
		}

		if(room->flagIsSet(R_NO_CLAIR_ROOM)) {
			if(limboOrCoven)
				player->printColor("^rThis room does not need flag 43-No Clair set.\n");
		}

		if(room->flagIsSet(R_NO_TRACK_TO)) {
			if(	limboOrCoven ||
				room->flagIsSet(R_IS_STORAGE_ROOM) ||
				room->flagIsSet(R_NO_TELEPORT) ||
				room->whatTraining()
			)
				player->printColor("^rThis room does not need flag 52-No Track To set.\n");
		}

		if(room->flagIsSet(R_NO_SUMMON_TO)) {
			if(	limboOrCoven ||
				room->flagIsSet(R_NO_TELEPORT) ||
				room->flagIsSet(R_ONE_PERSON_ONLY) ||
				whatTraining
			)
				player->printColor("^rThis room does not need flag 54-No Summon To set.\n");
		}

		if(room->flagIsSet(R_NO_TRACK_OUT)) {
			if(	room->flagIsSet(R_LIMBO) ||
				room->flagIsSet(R_ETHEREAL_PLANE)
			)
				player->printColor("^rThis room does not need flag 54-No Summon To set.\n");
		}

		if(room->flagIsSet(R_OUTLAW_SAFE)) {
			if(	limboOrCoven ||
				whatTraining
			)
				player->printColor("^rThis room does not need flag 57-Outlaw Safe set.\n");
		}

		if(room->flagIsSet(R_LOG_INTO_TRAP_ROOM)) {
			if(whatTraining)
				player->printColor("^rThis room does not need flag 63-Log To Trap Exit set.\n");
		}

		if(room->flagIsSet(R_LIMBO)) {
			if(room->flagIsSet(R_POST_OFFICE))
				player->printColor("^rThis room does not need flag 11-Post Office set.\n");
			if(room->flagIsSet(R_FAST_HEAL))
				player->printColor("^rThis room does not need flag 14-Fast Heal set.\n");
			if(room->flagIsSet(R_NO_POTION))
				player->printColor("^rThis room does not need flag 32-No Potion set.\n");
			if(room->flagIsSet(R_NO_CAST_TELEPORT))
				player->printColor("^rThis room does not need flag 38-No Cast Teleport set.\n");
			if(room->flagIsSet(R_NO_FLEE))
				player->printColor("^rThis room does not need flag 44-No Flee set.\n");
			if(room->flagIsSet(R_BANK))
				player->printColor("^rThis room does not need flag 58-Bank set.\n");
			if(room->flagIsSet(R_MAGIC_MONEY_MACHINE))
				player->printColor("^rThis room does not need flag 59-Magic Money Machine set.\n");
		}

	}
}


//*********************************************************************
//						stat_rom
//*********************************************************************

int stat_rom(Player* player, AreaRoom* room) {
	std::list<AreaZone*>::iterator it;
	AreaZone* zone=0;
	TileInfo* tile=0;

	if(!player->checkBuilder(0))
		return(0);

	if(player->getClass() == CARETAKER)
		log_immort(false,player, "%s statted room %s.\n", player->name, player->getRoom()->fullName().c_str());

	player->print("Room: %s %s\n\n",
		room->area->name.c_str(), room->fullName().c_str());
	tile = room->area->getTile(room->area->getTerrain(0, &room->mapmarker, 0, 0, 0, true), false);

	for(it = room->area->zones.begin() ; it != room->area->zones.end() ; it++) {
		zone = (*it);
		if(zone->inside(room->area, &room->mapmarker)) {
			player->printColor("^yZone:^x %s\n", zone->name.c_str());
			if(zone->wander.getTraffic()) {
				zone->wander.show(player);
			} else {
				player->print("    No monsters come in this zone.\n");
			}
			player->print("\n");
		}
	}

	if(room->getSize())
		player->printColor("Size: ^y%s\n", getSizeName(room->getSize()).c_str());
	
	if(tile->wander.getTraffic())
		tile->wander.show(player);

	player->print("Terrain: %c\n", tile->getDisplay());
	if(room->isWater())
		player->printColor("Water: ^gyes\n");
	if(room->isRoad())
		player->printColor("Road: ^gyes\n");

	player->printColor("Generic Room: %s\n", room->canSave() ? "^rNo" : "^gYes");
	if(room->unique.id) {
		player->printColor("Links to unique room ^y%s^x.\n", room->unique.str().c_str());
		player->printColor("needsCompass: %s^x    decCompass: %s",
			room->getNeedsCompass() ? "^gYes" : "^rNo", room->getDecCompass() ? "^gYes" : "^rNo");
	}
	player->print("\n");

	showRoomFlags(player, room, 0, 0);
	if(room->effects.list.size())
		player->printColor("Effects:\n%s", room->effects.getEffectsString(player).c_str());
	player->printColor("%s", room->hooks.display().c_str());
	stat_rom_exits(player, room);
	return(0);
}

//*********************************************************************
//						validateShop
//*********************************************************************

void validateShop(const Player* player, const UniqueRoom* shop, const UniqueRoom* storage) {
	// basic checks
	if(!shop) {
		player->printColor("^rThe shop associated with this storage room does not exist.\n");
		return;
	}
	if(!storage) {
		player->printColor("^rThe storage room associated with this shop does not exist.\n");
		return;
	}

	if(shop->info == storage->info) {
		player->printColor("^rThe shop and the storage room cannot be the same room. Set the shop's trap exit appropriately.\n");
		return;
	}

	CatRef cr = shopStorageRoom(shop);

	if(shop->info == cr) {
		player->printColor("^rThe shop and the storage room cannot be the same room. Set the shop's trap exit appropriately.\n");
		return;
	}

	bstring name = "Storage: ";
	name += shop->getName();

	if(cr != storage->info)
		player->printColor("^rThe shop's storage room of %s does not match the storage room %s.\n", cr.str().c_str(), storage->info.str().c_str());
	if(storage->getTrapExit() != shop->info)
		player->printColor("^yThe storage room's trap exit of %s does not match the shop room %s.\n", storage->info.str().c_str(), shop->info.str().c_str());

	if(!shop->flagIsSet(R_SHOP))
		player->printColor("^rThe shop's flag 1-Shoppe is not set.\n");
	if(!storage->flagIsSet(R_SHOP_STORAGE))
		player->printColor("^rThe storage room's flag 97-Shop Storage is not set.\n");

	// what DOESN'T the storage room need?
	if(storage->flagIsSet(R_NO_LOGIN))
		player->printColor("^rThe storage room does not need flag 34-No Log set.\n");
	if(storage->flagIsSet(R_LOG_INTO_TRAP_ROOM))
		player->printColor("^rThe storage room does not need flag 63-Log To Trap Exit set.\n");
	if(storage->flagIsSet(R_NO_TELEPORT))
		player->printColor("^rThe storage room does not need flag 13-No Teleport set.\n");
	if(storage->flagIsSet(R_NO_SUMMON_TO))
		player->printColor("^rThe storage room does not need flag 54-No Summon To set.\n");
	if(storage->flagIsSet(R_NO_TRACK_TO))
		player->printColor("^rThe storage room does not need flag 52-No Track To set.\n");
	if(storage->flagIsSet(R_NO_CLAIR_ROOM))
		player->printColor("^rThe storage room does not need flag 43-No Clair set.\n");

	if(!storage->first_ext) {
		player->printColor("^yThe storage room does not have an out exit pointing to the shop.\n");
	} else {
		const Exit* exit = storage->first_ext->ext;
		
		if(	exit->target.room != shop->info ||
			strcmp(exit->getName(), "out")
		)
			player->printColor("^yThe storage room does not have an out exit pointing to the shop.\n");
		else if(storage->first_ext->next_tag)
			player->printColor("^yThe storage room has more than one exit - it only needs one out exit pointing to the shop.\n");
	}
}

//*********************************************************************
//						stat_rom
//*********************************************************************

int stat_rom(Player* player, UniqueRoom* room) {
	std::map<int, crlasttime>::iterator it;
	crlasttime* crtm=0;
	CatRef	cr;
	Monster* monster=0;
	Object* object=0;
	UniqueRoom* shop=0;

	if(!player->checkBuilder(room))
		return(0);

	if(player->getClass() == CARETAKER)
		log_immort(false,player, "%s statted room %s.\n", player->name, player->getRoom()->fullName().c_str());

	player->printColor("Room: %s", room->info.str("", 'y').c_str());
	if(gConfig->inSwapQueue(room->info, SwapRoom, true))
		player->printColor("        ^eThis room is being swapped.");

	player->print("\nTimes People have entered this room: %d\n", room->getBeenHere());
	player->print("Name: %s\n", room->name);

	Property *p = gConfig->getProperty(room->info);
	if(p) {
		player->printColor("Property Belongs To: ^y%s^x\nProperty Type: ^y%s\n",
			p->getOwner().c_str(), p->getTypeStr().c_str());
	}

	if(player->isCt()) {
		if(room->last_mod[0])
			player->printColor("^cLast modified by: %s on %s\n", room->last_mod, stripLineFeeds(room->lastModTime));
		if(room->lastPly[0])
			player->printColor("^cLast player here: %s on %s\n", room->lastPly, stripLineFeeds(room->lastPlyTime));
	} else
		player->print("\n");

	if(room->getSize())
		player->printColor("Size: ^y%s\n", getSizeName(room->getSize()).c_str());
	if(room->getRoomExperience())
		player->print("Experience for entering this room: %d\n", room->getRoomExperience());

	if(room->getFaction() != "")
		player->printColor("Faction: ^g%s^x\n", room->getFaction().c_str());
	if(room->getFishingStr() != "")
		player->printColor("Fishing: ^g%s^x\n", room->getFishingStr().c_str());

	if(room->getMaxMobs() > 0)
		player->print("Max mob allowance: %d\n", room->getMaxMobs());

	room->wander.show(player, room->info.area);
	player->print("\n");

	player->print("Perm Objects:\n");
	for(it = room->permObjects.begin(); it != room->permObjects.end() ; it++) {
		crtm = &(*it).second;
		loadObject((*it).second.cr, &object);

		player->printColor("^y%2d) ^x%14s ^y::^x %-30s ^yInterval:^x %-5d", (*it).first+1,
			crtm->cr.str("", 'y').c_str(), object ? object->name : "", crtm->interval);

		if(room->flagIsSet(R_SHOP_STORAGE) && object)
			player->printColor(" ^yCost:^x %s", object->value.str().c_str());

		player->print("\n");

		// warning about deeds in improper areas
		if(object && object->deed.low.id && !object->deed.isArea(room->info.area))
			player->printColor("      ^YCaution:^x this object's deed area does not match the room's area.\n");

		if(object) {
			delete object;
			object = 0;
		}
	}
	player->print("\n");

	player->print("Perm Monsters:\n");
	for(it = room->permMonsters.begin(); it != room->permMonsters.end() ; it++) {
		crtm = &(*it).second;
		loadMonster((*it).second.cr, &monster);

		player->printColor("^m%2d) ^x%14s ^m::^x %-30s ^mInterval:^x %d\n", (*it).first+1,
			crtm->cr.str("", 'm').c_str(), monster ? monster->name : "", crtm->interval);

		if(monster) {
			free_crt(monster);
			monster = 0;
		}
	}
	player->print("\n");

	if(room->track.getDirection() != "" && room->flagIsSet(R_PERMENANT_TRACKS))
		player->print("Perm Tracks: %s.\n", room->track.getDirection().c_str());

	if(room->getLowLevel() || room->getHighLevel()) {
		player->print("Level Boundary: ");
		if(room->getLowLevel())
			player->print("%d+ level  ", room->getLowLevel());
		if(room->getHighLevel())
			player->print("%d- level  ", room->getHighLevel());
		player->print("\n");
	}

	if(	room->flagIsSet(R_LOG_INTO_TRAP_ROOM) ||
		room->flagIsSet(R_SHOP_STORAGE) ||
		room->whatTraining()
	) {
		if(room->getTrapExit().id)
			player->print("Players will relog into room %s from here.\n", room->getTrapExit().str(room->info.area).c_str());
		else
			player->printColor("^rTrap exit needs to be set to %s room number.\n", room->flagIsSet(R_SHOP_STORAGE) ? "shop" : "relog");
	}

	if(	room->getSize() == NO_SIZE && (
			room->flagIsSet(R_INDOORS) ||
			room->flagIsSet(R_UNDERGROUND)
		)
	)
		player->printColor("^yThis room does not have a size set.\n");

	checkTeleportRange(player, room->info);

	// isShopValid
	if(room->flagIsSet(R_SHOP)) {
		cr = shopStorageRoom(room);
		player->print("Shop storage room: %s (%s)\n", cr.str().c_str(),
			cr.id == room->info.id+1 && cr.isArea(room->info.area) ? "default" : "trapexit");

		if(room->getFaction() == "" && room->info.area != "shop")
			player->printColor("^yThis shop does not have a faction set.\n");

		loadRoom(cr, &shop);
		validateShop(player, room, shop);
	} else if(room->flagIsSet(R_PAWN_SHOP)) {
		if(room->getFaction() == "")
			player->printColor("^yThis pawn shop does not have a faction set.\n");
	}

	if(room->flagIsSet(R_SHOP_STORAGE)) {
		loadRoom(room->getTrapExit(), &shop);
		validateShop(player, shop, room);
	}

	if(room->getTrap()) {
		if(room->getTrapWeight())
			player->print("Trap weight: %d/%d lbs\n", room->getWeight(), room->getTrapWeight());
		player->print("Trap type: ");
		switch(room->getTrap()) {
		case TRAP_PIT:
			player->print("Pit Trap (exit rm %s)\n", room->getTrapExit().str(room->info.area).c_str());
			break;
		case TRAP_DART:
			player->print("Poison Dart Trap\n");
			break;
		case TRAP_BLOCK:
			player->print("Falling Block Trap\n");
			break;
		case TRAP_MPDAM:
			player->print("MP Damage Trap\n");
			break;
		case TRAP_RMSPL:
			player->print("Negate Spell Trap\n");
			break;
		case TRAP_NAKED:
			player->print("Naked Trap\n");
			break;
		case TRAP_TPORT:
			player->print("Teleport Trap\n");
			break;
		case TRAP_ARROW:
			player->print("Arrow Trap\n");
			break;
		case TRAP_SPIKED_PIT:
			player->print("Spiked Pit Trap (exit rm %s)\n", room->getTrapExit().str(room->info.area).c_str());
			break;
		case TRAP_WORD:
			player->print("Word of Recall Trap\n");
			break;
		case TRAP_FIRE:
			player->print("Fire Trap\n");
			break;
		case TRAP_FROST:
			player->print("Frost Trap\n");
			break;
		case TRAP_ELEC:
			player->print("Electricity Trap\n");
			break;
		case TRAP_ACID:
			player->print("Acid Trap\n");
			break;
		case TRAP_ROCKS:
			player->print("Rockslide Trap\n");
			break;
		case TRAP_ICE:
			player->print("Icicle Trap\n");
			break;
		case TRAP_SPEAR:
			player->print("Spear Trap\n");
			break;
		case TRAP_CROSSBOW:
			player->print("Crossbow Trap\n");
			break;
		case TRAP_GASP:
			player->print("Poison Gas Trap\n");
			break;
		case TRAP_GASB:
			player->print("Blinding Gas Trap\n");
			break;
		case TRAP_GASS:
			player->print("Stun Gas Trap\n");
			break;
		case TRAP_MUD:
			player->print("Mud Trap\n");
			break;
		case TRAP_DISP:
			player->print("Room Displacement Trap (exit rm %s)\n", room->getTrapExit().str(room->info.area).c_str());
			break;
		case TRAP_FALL:
			player->print("Deadly Fall Trap (exit rm %s)\n", room->getTrapExit().str(room->info.area).c_str());
			break;
		case TRAP_CHUTE:
			player->print("Chute Trap (exit rm %s)\n", room->getTrapExit().str(room->info.area).c_str());
			break;
		case TRAP_ALARM:
			player->print("Alarm Trap (guard rm %s)\n", room->getTrapExit().str(room->info.area).c_str());
			break;
		case TRAP_BONEAV:
			player->print("Bone Avalanche Trap (exit rm %s)\n", room->getTrapExit().str(room->info.area).c_str());
			break;
		case TRAP_PIERCER:
			player->print("Piercer trap (%d piercers)\n", room->getTrapStrength());
			break;
		case TRAP_ETHEREAL_TRAVEL:
			player->print("Ethereal travel trap.\n");
			break;
		case TRAP_WEB:
			player->print("Sticky spider web trap.\n");
			break;
		default:
			player->print("Invalid trap #\n");
			break;
		}
	}

	if(room->flagIsSet(R_CAN_SHOPLIFT))
		player->print("Store guardroom: rm %s\n", cr.str(room->info.area).c_str());

	showRoomFlags(player, room, 0, 0);


	if(room->effects.list.size())
		player->printColor("Effects:\n%s", room->effects.getEffectsString(player).c_str());
	player->printColor("%s", room->hooks.display().c_str());
	stat_rom_exits(player, room);
	return(0);
}

//*********************************************************************
//						dmAddRoom
//*********************************************************************
// This function allows staff to add a new, empty room to the current
// database of rooms.

int dmAddRoom(Player* player, cmd* cmnd) {
	UniqueRoom	*newRoom=0;
	char	file[80];
	int		i=1;

	if(!strcmp(cmnd->str[1], "c") && (cmnd->num > 1)) {
		dmAddMob(player, cmnd);
		return(0);
	}
	if(!strcmp(cmnd->str[1], "o") && (cmnd->num > 1)) {
		dmAddObj(player, cmnd);
		return(0);
	}

	CatRef	cr;
	bool extra = !strcmp(cmnd->str[1], "r");
	getCatRef(getFullstrText(cmnd->fullstr, extra ? 2 : 1), &cr, player);

	if(cr.id < 1) {
		player->print("Index error: please specify room number.\n");
		return(0);
	}
	if(!player->checkBuilder(cr, false)) {
		player->print("Error: Room number not inside any of your alotted ranges.\n");
		return(0);
	}
	if(gConfig->moveRoomRestrictedArea(cr.area)) {
		player->print("Error: ""%s"" is a restricted range. You cannot create unique rooms in that area.\n");
		return(0);
	}
	Path::checkDirExists(cr.area, roomPath);

	if(!strcmp(cmnd->str[extra ? 3 : 2], "loop"))
		i = MAX(1, MIN(100, cmnd->val[extra ? 3 : 2]));

	for(; i; i--) {
		if(!player->checkBuilder(cr, false)) {
			player->print("Error: Room number not inside any of your alotted ranges.\n");
			return(0);
		}

		sprintf(file, "%s", roomPath(cr));
		if(file_exists(file)) {
			player->print("Room already exists.\n");
			return(0);
		}

		newRoom = new UniqueRoom;
		if(!newRoom)
			merror("dmAddRoom", FATAL);
		newRoom->info = cr;

		newRoom->setFlag(R_CONSTRUCTION);
		sprintf(newRoom->name, "New Room");

		if(newRoom->saveToFile(0) < 0) {
			player->print("Write failed.\n");
			return(0);
		}

		delete newRoom;

		log_immort(true, player, "%s created room %s.\n", player->name, cr.str().c_str());
		player->print("Room %s created.\n", cr.str().c_str());
		checkTeleportRange(player, cr);
		cr.id++;
	}
	return(0);
}

//*********************************************************************
//						dmSetRoom
//*********************************************************************
// This function allows staff to set a characteristic of a room.

int dmSetRoom(Player* player, cmd* cmnd) {
	BaseRoom *room = player->getRoom();
	int		a=0, num=0;
	CatRef	cr;

	if(cmnd->num < 3) {
		player->print("Syntax: *set r [option] [<value>]\n");
		return(0);
	}

	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Error: Room number not inside any of your alotted ranges.\n");
		return(0);
	}

	switch(low(cmnd->str[2][0])) {
	case 'b':
		if(!player->parent_rom) {
			player->print("Error: You need to be in a unique room to do that.\n");
			return(0);
		}
		if(low(cmnd->str[2][1]) == 'l') {
			player->parent_rom->setLowLevel(cmnd->val[2]);
			player->print("Low level boundary %d\n", player->parent_rom->getLowLevel());
		} else if(low(cmnd->str[2][1]) == 'h') {
			player->parent_rom->setHighLevel(cmnd->val[2]);
			player->print("Upper level boundary %d\n", player->parent_rom->getHighLevel());
		}

		break;
	case 'd':
		if(!player->area_room) {
			player->print("Error: You need to be in an area room to do that.\n");
			return(0);
		}
		if(!player->area_room->unique.id) {
			player->print("Error: The area room must have the unique field set [*set r unique #].\n");
			return(0);
		}
		player->area_room->setDecCompass(!player->area_room->getDecCompass());
		player->printColor("DecCompass toggled, set to %s^x.\n",
			player->area_room->getDecCompass() ? "^gYes" : "^rNo");

		log_immort(true, player, "%s set decCompass to %s in room %s.\n", player->name,
			player->area_room->getDecCompass() ? "true" : "false", room->fullName().c_str());

		break;
	case 'e':
		if(cmnd->str[2][1] == 'f') {
			if(cmnd->num < 4) {
				player->print("Set what effect to what?\n");
				return(0);
			}

			long duration = -1;
			int strength = 1;

			bstring txt = getFullstrText(cmnd->fullstr, 4);
			if(txt != "")
				duration = atoi(txt.c_str());
			txt = getFullstrText(cmnd->fullstr, 5);
			if(txt != "")
				strength = atoi(txt.c_str());

			if(duration > EFFECT_MAX_DURATION || duration < -1) {
				player->print("Duration must be between -1 and %d.\n", EFFECT_MAX_DURATION);
				return(0);
			}

			if(strength < 0 || strength > EFFECT_MAX_STRENGTH) {
				player->print("Strength must be between 0 and %d.\n", EFFECT_MAX_STRENGTH);
				return(0);
			}

			bstring effectStr = cmnd->str[3];
			EffectInfo* toSet = 0;
			if((toSet = room->getExactEffect(effectStr))) {
				// We have an existing effect we're modifying
				if(duration == 0) {
					// Duration is 0, so remove it
					room->removeEffect(toSet, true);
					player->print("Effect '%s' (room) removed.\n", effectStr.c_str());
				} else {
					// Otherwise modify as appropriate
					toSet->setDuration(duration);
					if(strength != -1)
						toSet->setStrength(strength);
					player->print("Effect '%s' (room) set to duration %d and strength %d.\n", effectStr.c_str(), toSet->getDuration(), toSet->getStrength());
				}
				break;
			} else {
				// No existing effect, add a new one
				if(strength == -1)
					strength = 1;
				if(room->addEffect(effectStr, duration, strength, 0, true) != NULL){
					player->print("Effect '%s' (room) added with duration %d and strength %d.\n", effectStr.c_str(), duration, strength);
				} else {
					player->print("Unable to add effect '%s' (room)\n", effectStr.c_str());
				}
				break;
			}
		} else if(cmnd->str[2][1] == 'x') {
			if(!player->parent_rom) {
				player->print("Error: You need to be in a unique room to do that.\n");
				return(0);
			}
			player->parent_rom->setRoomExperience(cmnd->val[2]);

			player->print("Room experience set to %d.\n", player->parent_rom->getRoomExperience());
			log_immort(true, player, "%s set roomExp to %d in room %s.\n", player->name,
				player->parent_rom->getRoomExperience(), room->fullName().c_str());
		} else {
			player->print("Invalid option.\n");
			return(0);
		}
		break;
	case 'f':
		if(low(cmnd->str[2][1]) == 'i') {
			if(!strcmp(cmnd->str[3], "")) {
				player->parent_rom->setFishing("");

				player->print("Fishing list cleared.\n");
				log_immort(true, player, "%s cleared fishing list in room %s.\n", player->name,
					room->fullName().c_str());
			} else {
				const Fishing* list = gConfig->getFishing(cmnd->str[3]);

				if(!list) {
					player->print("Fishing list \"%s\" does not exist!\n", cmnd->str[3]);
					return(0);
				}

				player->parent_rom->setFishing(cmnd->str[3]);

				player->print("Fishing list set to %s.\n", player->parent_rom->getFishingStr().c_str());
				log_immort(true, player, "%s set fishing list to %s in room %s.\n", player->name,
					player->parent_rom->getFishingStr().c_str(), room->fullName().c_str());
			}
		} else if(low(cmnd->str[2][1]) == 'a') {
			if(!player->parent_rom) {
				player->print("Error: You need to be in a unique room to do that.\n");
				return(0);
			}

			if(cmnd->num < 3) {
				player->print("Set faction to what?\n");
				return(0);
			} else if(cmnd->num == 3) {
				player->parent_rom->setFaction("");
				player->print("Faction cleared.\n");
				log_immort(true, player, "%s cleared faction in room %s.\n", player->name,
					room->fullName().c_str());
				return(0);
			}

			Property* p = gConfig->getProperty(player->parent_rom->info);

			if(p && p->getType() == PROP_SHOP) {
				player->print("You can't set room faction on player shops!\n");
				return(0);
			}

			const Faction* faction = gConfig->getFaction(cmnd->str[3]);
			if(!faction) {
				player->print("'%s' is an invalid faction.\n", cmnd->str[3]);
				return(0);
			}
			player->parent_rom->setFaction(faction->getName());
			player->print("Faction set to %s.\n", player->parent_rom->getFaction().c_str());
			log_immort(true, player, "%s set faction to %s in room %s.\n", player->name,
				player->parent_rom->getFaction().c_str(), room->fullName().c_str());
			break;
		} else {
			if(!player->parent_rom) {
				player->print("Error: You need to be in a unique room to do that.\n");
				return(0);
			}
			num = cmnd->val[2];
			if(num < 1 || num > MAX_ROOM_FLAGS) {
				player->print("Error: outside of range.\n");
				return(0);
			}

			if(!player->isCt() && num == R_CONSTRUCTION+1) {
				player->print("Error: you cannot set/clear that flag.\n");
				return(0);
			}

			if(!strcmp(cmnd->str[3], "del")) {
				for(a=0;a<MAX_ROOM_FLAGS;a++)
					player->parent_rom->clearFlag(a);

				player->print("All room flags cleared.\n");
				log_immort(true, player, "%s cleared all flags in room %s.\n",
					player->name, room->fullName().c_str());
				break;
			}

			if(player->parent_rom->flagIsSet(num - 1)) {
				player->parent_rom->clearFlag(num - 1);
				player->print("Room flag #%d(%s) off.\n", num, get_rflag(num-1));

				log_immort(true, player, "%s cleared flag #%d(%s) in room %s.\n", player->name, num, get_rflag(num-1),
					room->fullName().c_str());
			} else {
	 			if(num >= R_TRAINING_ROOM && num - 4 <= R_TRAINING_ROOM) {
	 				// setting a training flag - do we let them?
	 				if(!player->parent_rom->whatTraining(num-1)) {
	 					player->print("You are setting training for a class that does not exist.\n");
	 					return(0);
	 				}
	 			}

	 			player->parent_rom->setFlag(num - 1);
				player->print("Room flag #%d(%s) on.\n", num, get_rflag(num-1));
				log_immort(true, player, "%s set flag #%d(%s) in room %s.\n", player->name, num, get_rflag(num-1),
					room->fullName().c_str());

				if(num-1 == R_SHOP)
					player->printColor("^YNote:^x you must set the Shop Storage (97) to use this room as a shop.\n");
				if((num-1 == R_INDOORS || num-1 == R_VAMPIRE_COVEN || num-1 == R_UNDERGROUND) && player->parent_rom->getSize() == NO_SIZE)
					player->printColor("^YNote:^x don't forget to set the size for this room.\n");
			}

			// try and be smart
 			if(	num-1 == R_SHOP_STORAGE &&
 				!strcmp(player->parent_rom->name, "New Room") &&
 				!player->parent_rom->first_ext
 			) {
 				cr = player->parent_rom->info;
 				UniqueRoom* shop=0;
 				bstring storageName = "Storage: ";

 				cr.id--;
 				if(loadRoom(cr, &shop)) {
 					if(	shop->flagIsSet(R_SHOP) &&
 						(!shop->getTrapExit().id || shop->getTrapExit() == cr)
 					) {
 						player->printColor("^ySetting up this storage room for you...\n");
 						player->printColor("^y * ^xSetting the trap exit to %s...\n", cr.str().c_str());
 						player->parent_rom->setTrapExit(cr);
 						player->printColor("^y * ^xCreating exit ""out"" to %s...\n", cr.str().c_str());
 						link_rom(player->parent_rom, cr, "out");
 						player->parent_rom->setTrapExit(cr);
 						player->printColor("^y * ^xNaming this room...\n");
 						storageName += shop->name;
 						strcpy(player->parent_rom->name, storageName.c_str());
 						player->print("Done!\n");
 					}
 				}
 			}
		}
		break;
	case 'l':
		if(!player->parent_rom) {
			player->print("Error: You need to be in a unique room to do that.\n");
			return(0);
		}
		if(strcmp(cmnd->str[3], "clear")) {
			player->print("Are you sure?\nType \"*set r last clear\" to clear last-arrived info.\n");
			return(0);
		}
		strcpy(player->parent_rom->lastPly, "");
		strcpy(player->parent_rom->lastPlyTime, "");
		player->print("Last-arrived info cleared.\n");
		break;
	case 'm':
		if(!player->parent_rom) {
			player->print("Error: You need to be in a unique room to do that.\n");
			return(0);
		}
		player->parent_rom->setMaxMobs(cmnd->val[2]);

		if(!player->parent_rom->getMaxMobs())
			player->print("The limit on the number of creatures that can be here has been removed.\n");
		else
			player->print("Only %d creature%s can now be in here at a time.\n", player->parent_rom->getMaxMobs(), player->parent_rom->getMaxMobs() != 1 ? "s" : "");

		log_immort(true, player, "%s set max %d mobs in room %s.\n", player->name, player->parent_rom->getMaxMobs(),
			room->fullName().c_str());

		break;
	case 'n':
		if(!player->area_room) {
			player->print("Error: You need to be in an area room to do that.\n");
			return(0);
		}
		if(!player->area_room->unique.id) {
			player->print("Error: The area room must have the unique field set [*set r unique #].\n");
			return(0);
		}
		player->area_room->setNeedsCompass(!player->area_room->getNeedsCompass());
		player->printColor("NeedsCompass toggled, set to %s^x.\n",
			player->area_room->getNeedsCompass() ? "^gYes" : "^rNo");

		log_immort(true, player, "%s set needsCompass to %s in room %s.\n", player->name,
			player->area_room->getNeedsCompass() ? "true" : "false", room->fullName().c_str());

		break;
	case 'r':
		if(!player->parent_rom) {
			player->print("Error: You need to be in a unique room to do that.\n");
			return(0);
		}
		num = atoi(&cmnd->str[2][1]);
		if(num < 1 || num > NUM_RANDOM_SLOTS) {
			player->print("Error: outside of range.\n");
			return(PROMPT);
		}

		getCatRef(getFullstrText(cmnd->fullstr, 3), &cr, player);

		if(!cr.id) {
			player->parent_rom->wander.random.erase(num-1);
			player->print("Random #%d has been cleared.\n", num);
		} else {
			player->parent_rom->wander.random[num-1] = cr;
			player->print("Random #%d is now %s.\n", num, cr.str().c_str());
		}

		log_immort(false,player, "%s set mob slot %d to mob %s in room %s.\n",
			player->name, num, cr.str().c_str(),
			room->fullName().c_str());

		break;
	case 's':
		if(!player->parent_rom) {
			player->print("Error: You need to be in a unique room to do that.\n");
			return(0);
		}
		player->parent_rom->setSize(getSize(cmnd->str[3]));

		player->print("Size set to %s.\n", getSizeName(player->parent_rom->getSize()).c_str());
		log_immort(true, player, "%s set room %s's %s to %s.\n",
			player->name, room->fullName().c_str(), "Size", getSizeName(player->parent_rom->getSize()).c_str());
		break;
	case 't':
		if(!player->parent_rom) {
			player->print("Error: You need to be in a unique room to do that.\n");
			return(0);
		}
		player->parent_rom->wander.setTraffic(cmnd->val[2]);
		log_immort(true, player, "%s set room %s's traffic to %ld.\n", player->name,
			player->parent_rom->info.str().c_str(), player->parent_rom->wander.getTraffic());
		player->print("Traffic is now %d%%.\n", player->parent_rom->wander.getTraffic());

		break;
	case 'x':
		if(!player->parent_rom) {
			player->print("Error: You need to be in a unique room to do that.\n");
			return(0);
		}
		if(low(cmnd->str[2][1]) == 'x') {
			getCatRef(getFullstrText(cmnd->fullstr, 3), &cr, player);

			if(!player->checkBuilder(cr)) {
				player->print("Trap's exit must be within an assigned range.\n");
				return(0);
			}
			player->parent_rom->setTrapExit(cr);
			player->print("Room's trap exit is now %s.\n", player->parent_rom->getTrapExit().str().c_str());
			log_immort(true, player, "%s set trapexit to %s in room %s.\n", player->name,
				player->parent_rom->getTrapExit().str().c_str(), room->fullName().c_str());
		} else if(low(cmnd->str[2][1]) == 'w') {
			num = (int)cmnd->val[2];
			if(num < 0 || num > 5000) {
				player->print("Trap weight cannot be less than 0 or greater than 5000.\n");
				return(0);
			}
			player->parent_rom->setTrapWeight(num);
			player->print("Room's trap weight is now %d.\n", player->parent_rom->getTrapWeight());
			log_immort(true, player, "%s set trapweight to %d in room %s.\n", player->name, player->parent_rom->getTrapWeight(),
				room->fullName().c_str());
		} else if(low(cmnd->str[2][1]) == 's') {
			num = (int)cmnd->val[2];
			if(num < 0 || num > 5000) {
				player->print("Trap strength cannot be less than 0 or greater than 5000.\n");
				return(0);
			}
			player->parent_rom->setTrapStrength(num);
			player->print("Room's trap strength is now %d.\n", player->parent_rom->getTrapStrength());
			log_immort(true, player, "%s set trapstrength to %d in room %s.\n", player->name, player->parent_rom->getTrapStrength(),
				room->fullName().c_str());
		} else {
			player->parent_rom->setTrap(cmnd->val[2]);
			player->print("Room has trap #%d set.\n", player->parent_rom->getTrap());
			log_immort(true, player, "%s set trap #%d in room %s.\n", player->name, player->parent_rom->getTrap(),
				room->fullName().c_str());
		}

		break;
	case 'u':
		if(!player->area_room) {
			player->print("Error: You need to be in an area room to do that.\n");
			return(0);
		}

		getCatRef(getFullstrText(cmnd->fullstr, 3), &cr, player);

		player->area_room->unique = cr;
		player->print("Unique room set to %s.\n", player->area_room->unique.str().c_str());
		if(player->area_room->unique.id)
			player->print("You'll need to use *teleport to get to this room in the future.\n");

		log_immort(true, player, "%s set unique room to %s in room %s.\n",
			player->name, player->area_room->unique.str().c_str(),
			room->fullName().c_str());

		break;
	default:
		player->print("Invalid option.\n");
		return(0);
	}

	if(player->parent_rom)
		player->parent_rom->escapeText();
	room_track(player);
	return(0);
}


//*********************************************************************
//						dmSetExit
//*********************************************************************
// This function allows staff to set a characteristic of an exit.

int dmSetExit(Player* player, cmd* cmnd) {
	BaseRoom* room = player->getRoom();
	int		num=0;
	//char	orig_exit[30];
	short	n=0;

	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Error: Room number not inside any of your alotted ranges.\n");
		return(0);
	}

	// setting something on the exit
	if(cmnd->str[1][1]) {

		if(cmnd->num < 3) {
			player->print("Invalid syntax.\n");
			return(0);
		}

		Exit* exit = findExit(player, cmnd->str[2], 1);

		if(!exit) {
			player->print("Exit not found.\n");
			return(0);
		}

		switch(cmnd->str[1][1]) {
		case 'd':
		{
			if(cmnd->str[1][2] == 'i') {
				Direction dir = getDir(cmnd->str[3]);
				if(getDir(exit->name) != NoDirection && dir != NoDirection) {
					player->print("This exit does not need a direction set on it.\n");
					return(0);
				}

				exit->setDirection(dir);

				player->printColor("%s^x's %s set to %s.\n", exit->name, "Direction", getDirName(exit->getDirection()).c_str());
				log_immort(true, player, "%s set %s %s^g's %s to %s.\n",
					player->name, "exit", exit->name, "Direction", getDirName(exit->getDirection()).c_str());
			} else if(cmnd->str[1][2] == 'e') {

				bstring desc = getFullstrText(cmnd->fullstr, 3);
				desc.Replace("*CR*", "\n");
				exit->setDescription(desc);

				if(exit->getDescription() == "") {
					player->print("Description cleared.\n");
					log_immort(true, player, "%s cleared %s^g's %s.\n", player->name, exit->name, "Description");
				} else {
					player->print("Description set to \"%s\".\n", exit->getDescription().c_str());
					log_immort(true, player, "%s set %s^g's %s to \"%s\".\n", player->name, exit->name, "Description", exit->getDescription().c_str());
				}

			} else {
				player->print("Description or direction?\n");
				return(0);
			}
			break;
		}
		case 'e':
			if(cmnd->str[1][2] == 'f') {
				if(cmnd->num < 4) {
					player->print("Set what effect to what?\n");
					return(0);
				}

				long duration = -1;
				int strength = 1;

				bstring txt = getFullstrText(cmnd->fullstr, 4);
				if(txt != "")
					duration = atoi(txt.c_str());
				txt = getFullstrText(cmnd->fullstr, 5);
				if(txt != "")
					strength = atoi(txt.c_str());

				if(duration > EFFECT_MAX_DURATION || duration < -1) {
					player->print("Duration must be between -1 and %d.\n", EFFECT_MAX_DURATION);
					return(0);
				}

				if(strength < 0 || strength > EFFECT_MAX_STRENGTH) {
					player->print("Strength must be between 0 and %d.\n", EFFECT_MAX_STRENGTH);
					return(0);
				}

				bstring effectStr = cmnd->str[3];
				EffectInfo* toSet = 0;
				if((toSet = exit->getExactEffect(effectStr))) {
					// We have an existing effect we're modifying
					if(duration == 0) {
						// Duration is 0, so remove it
						exit->removeEffect(toSet, true);
						player->print("Effect '%s' (exit) removed.\n", effectStr.c_str());
					} else {
						// Otherwise modify as appropriate
						toSet->setDuration(duration);
						if(strength != -1)
							toSet->setStrength(strength);
						player->print("Effect '%s' (exit) set to duration %d and strength %d.\n", effectStr.c_str(), toSet->getDuration(), toSet->getStrength());
					}
				} else {
					// No existing effect, add a new one
					if(strength == -1)
						strength = 1;
					if(exit->addEffect(effectStr, duration, strength, 0, true) != NULL) {
						player->print("Effect '%s' (exit) added with duration %d and strength %d.\n", effectStr.c_str(), duration, strength);
					} else {
						player->print("Unable to add effect '%s' (exit)\n", effectStr.c_str());
					}
				}
				break;
			} else {
				exit->setEnter(getFullstrText(cmnd->fullstr, 3));

				if(exit->getEnter() == "" || Pueblo::is(exit->getEnter())) {
					exit->setEnter("");
					player->print("OnEnter cleared.\n");
					log_immort(true, player, "%s cleared %s^g's %s.\n", player->name, exit->name, "OnEnter");
				} else {
					player->print("OnEnter set to \"%s\".\n", exit->getEnter().c_str());
					log_immort(true, player, "%s set %s^g's %s to \"%s\".\n", player->name, exit->name, "OnEnter", exit->getEnter().c_str());
				}
			}

			break;
		case 'f':
			num = cmnd->val[2];
			if(num < 1 || num > MAX_EXIT_FLAGS) {
				player->print("Error: flag out of range.\n");
				return(PROMPT);
			}

			if(exit->flagIsSet(num - 1)) {
			exit->clearFlag(num - 1);
			player->printColor("%s^x exit flag #%d off.\n", exit->name, num);

			log_immort(true, player, "%s cleared %s^g exit flag #%d(%s) in room %s.\n", player->name, exit->name, num, get_xflag(num-1),
				room->fullName().c_str());
			} else {
				exit->setFlag(num - 1);
				player->printColor("%s^x exit flag #%d on.\n", exit->name, num);
				log_immort(true, player, "%s turned on %s^g exit flag #%d(%s) in room %s.\n", player->name, exit->name, num, get_xflag(num-1),
					room->fullName().c_str());
			}
			break;
		case 'k':

			if(cmnd->str[1][2] == 'a') {
				exit->setKeyArea(cmnd->str[3]);
				if(exit->getKeyArea() == "") {
					player->print("Key Area cleared.\n");
					log_immort(true, player, "%s cleared %s^g's %s.\n", player->name, exit->name, "Key Area");
				} else {
					player->print("Key Area set to \"%s\".\n", exit->getKeyArea().c_str());
					log_immort(true, player, "%s set %s^g's %s to \"%s\".\n", player->name, exit->name, "Key Area", exit->getKeyArea().c_str());
				}
			} else {

				if(cmnd->val[2] > 255 || cmnd->val[2] < 0) {
					player->print("Error: key out of range.\n");
					return(0);
				}

				exit->setKey(cmnd->val[2]);
				player->printColor("Exit %s^x key set to %d.\n", exit->name, exit->getKey());
				log_immort(true, player, "%s set %s^g's %s to %ld.\n", player->name, exit->name, "Key", exit->getKey());
			}
			break;
		case 'l':
			if(cmnd->val[2] > MAXALVL || cmnd->val[2] < 0) {
				player->print("Level must be from 0 to %d.\n", MAXALVL);
				return(0);
			}

			exit->setLevel(cmnd->val[2]);
			player->printColor("Exit %s^x's level is now set to %d.\n", exit->name, exit->getLevel());
			log_immort(true, player, "%s set %s^g's %s to %ld.\n", player->name, exit->name, "Pick Level", exit->getLevel());
			break;
		case 'o':

			exit->setOpen(getFullstrText(cmnd->fullstr, 3));

			if(exit->getOpen() == "" || Pueblo::is(exit->getOpen())) {
				exit->setOpen("");
				player->print("OnOpen cleared.\n");
				log_immort(true, player, "%s cleared %s^g's %s.\n", player->name, exit->name, "OnOpen");
			} else {
				player->print("OnOpen set to \"%s\".\n", exit->getOpen().c_str());
				log_immort(true, player, "%s set %s^g's %s to \"%s\".\n", player->name, exit->name, "OnOpen", exit->getOpen().c_str());
			}

			break;
		case 'p':
			if(low(cmnd->str[1][2]) == 'p') {

				exit->setPassPhrase(getFullstrText(cmnd->fullstr, 3));

				if(exit->getPassPhrase() == "") {
					player->print("Passphrase cleared.\n");
					log_immort(true, player, "%s cleared %s^g's %s.\n", player->name, exit->name, "Passphrase");
				} else {
					player->print("Passphrase set to \"%s\".\n", exit->getPassPhrase().c_str());
					log_immort(true, player, "%s set %s^g's %s to \"%s\".\n", player->name, exit->name, "Passphrase", exit->getPassPhrase().c_str());
				}

			} else if(low(cmnd->str[1][2]) == 'l') {

				n = cmnd->val[2];

				if(n < 0 || n > LANGUAGE_COUNT) {
					player->print("Error: pass language out of range.\n");
					return(0);
				}
				n--;
				if(n < 0)
					n = 0;
				exit->setPassLanguage(n);

				player->print("Pass language %s.\n", n ? "set" : "cleared");
				log_immort(true, player, "%s set %s^g's %s to %s(%ld).\n", player->name, exit->name, "Passlang", n ? get_language_adj(n) : "Nothing", n+1);

			} else {
				player->print("Passphrase (xpp) or passlang (xpl)?\n");
				return(0);
			}
			break;
		case 's':
			exit->setSize(getSize(cmnd->str[3]));

			player->printColor("%s^x's %s set to %s.\n", exit->name, "Size", getSizeName(exit->getSize()).c_str());
			log_immort(true, player, "%s set %s %s^g's %s to %s.\n",
				player->name, "exit", exit->name, "Size", getSizeName(exit->getSize()).c_str());
			break;
		case 't':
			n = (short)cmnd->val[2];
			if(n > 30000 || n < 0) {
				player->print("Must be between 0-30000.\n");
				return(0);
			}
			exit->setToll(n);
			player->printColor("Exit %s^x's toll is now set to %d.\n", exit->name, exit->getToll());
			log_immort(true, player, "%s set %s^g's %s to %ld.\n", player->name, exit->name, "Toll", exit->getToll());
			break;
		default:
			player->print("Invalid syntax.\n");
			return(0);
		}

		if(player->parent_rom)
			player->parent_rom->escapeText();
		room_track(player);
		return(0);
	}



	// otherwise, we have other plans for this function
	if(cmnd->num < 3) {
		player->print("Syntax: *set x <name> <#> [. or name]\n");
		return(0);
	}


	// we need more variables to continue our work
	MapMarker mapmarker;
	BaseRoom* room2=0;
	AreaRoom* aRoom=0;
	UniqueRoom	*uRoom=0;
	Area	*area=0;
	CatRef	cr;
	bstring	returnExit = getFullstrText(cmnd->fullstr, 4);


	getDestination(getFullstrText(cmnd->fullstr, 3).c_str(), &mapmarker, &cr, player);


	if(!mapmarker.getArea() && !cr.id) {
		// if the expanded exit wasnt found
		// and the exit was expanded, check to delete the original
		if(del_exit(room, cmnd->str[2]))
			player->print("Exit %s deleted.\n", cmnd->str[2]);
		//} else if(
		//	strcmp(orig_exit, cmnd->str[2]) &&
		//	del_exit(room, orig_exit)
		//)
		//	player->print("Exit %s not found.\nExit %s deleted.\n", cmnd->str[2], orig_exit);
		else
			player->print("Exit %s not found.\n", cmnd->str[2]);
		return(0);
	}


	if(cr.id) {
		if(!player->checkBuilder(cr))
			return(0);

		if(!loadRoom(cr, &uRoom)) {
			player->print("Room %s does not exist.\n", cr.str().c_str());
			return(0);
		}

		room2 = uRoom;
	} else {
		if(player->getClass() == BUILDER) {
			player->print("Sorry, builders cannot link exits to the overland.\n");
			return(0);
		}
		area = gConfig->getArea(mapmarker.getArea());
		if(!area) {
			player->print("Area does not exist.\n");
			return(0);
		}
		aRoom = area->loadRoom(0, &mapmarker, false);
		room2 = aRoom;
	}


	bstring newName = getFullstrText(cmnd->fullstr, 2, ' ', false, true);


	if(newName.getLength() > 20) {
		player->print("Exit names must be 20 characters or less in length.\n");
		return(0);
	}

	newName = expand_exit_name(newName);


	if(returnExit != "") {

		if(returnExit == ".")
			returnExit = opposite_exit_name(newName);

		if(cr.id) {
			link_rom(room, cr, newName);

			if(player->parent_rom)
				link_rom(uRoom, player->parent_rom->info, returnExit);
			else
				link_rom(uRoom, &player->area_room->mapmarker, returnExit);

			gConfig->resaveRoom(cr);

		} else {
			link_rom(room, &mapmarker, newName);

			if(player->parent_rom)
				link_rom(aRoom, player->parent_rom->info, returnExit);
			else
				link_rom(aRoom, &player->area_room->mapmarker, returnExit);

			aRoom->save();
		}


		log_immort(true, player, "%s linked room %s to room %s in %s^g direction, both ways.\n",
			player->name, room->fullName().c_str(), room2->fullName().c_str(), newName.c_str());
		player->printColor("Room %s linked to room %s in %s^x direction, both ways.\n",
			room->fullName().c_str(), room2->fullName().c_str(), newName.c_str());

	} else {

		if(cr.id)
			link_rom(room, cr, newName);
		else
			link_rom(room, &mapmarker, newName);

		player->printColor("Room %s linked to room %s in %s^x direction.\n",
		      room->fullName().c_str(), room2->fullName().c_str(), newName.c_str());

		log_immort(true, player, "%s linked room %s to room %s in %s^g direction.\n",
			player->name, room->fullName().c_str(), room2->fullName().c_str(), newName.c_str());

	}

	if(player->parent_rom)
		gConfig->resaveRoom(player->parent_rom->info);

	if(aRoom && aRoom->canDelete())
		area->remove(aRoom);

	room_track(player);
	return(0);
}

//*********************************************************************
//						room_track
//*********************************************************************

int room_track(Creature* player) {
	long	t = time(0);

	if(player->isMonster() || !player->parent_rom)
		return(0);

	strcpy(player->parent_rom->last_mod, player->name);
	strcpy(player->parent_rom->lastModTime, ctime(&t));
	return(0);
}


//*********************************************************************
//						dmReplace
//*********************************************************************
// this command lets staff replace words or phrases in a room description

int dmReplace(Player* player, cmd* cmnd) {
	UniqueRoom	*room = player->parent_rom;
	int		n=0, skip=0, skPos=0;
	bstring::size_type i=0, pos=0;
	char	delim = ' ';
	bool	sdesc=false, ldesc=false;
	bstring search = "", temp = "";

	if(!needUniqueRoom(player))
		return(0);

	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Room is not in any of your alotted room ranges.\n");
		return(0);
	}

	if(cmnd->num < 3) {
		player->print("syntax: *replace [-SL#<num>] <search> <replace>\n");
		return(0);
	}

	// we have flags!
	// let's find out what they are
	if(cmnd->str[1][0] == '-') {

		i=1;
		while(i < strlen(cmnd->str[1])) {
			switch(cmnd->str[1][i]) {
			case 'l':
				ldesc = true;
				break;
			case 's':
				sdesc = true;
				break;
			case '#':
				skip = atoi(&cmnd->str[1][++i]);
				break;
			default:
				break;
			}
			i++;
		}

	}

	// we need to get fullstr into a nicer format
	i = strlen(cmnd->str[0]);
	if(ldesc || sdesc || skip)
		i += strlen(cmnd->str[1]) + 1;

	// which deliminator should we use?
	if(cmnd->fullstr[i+1] == '\'' || cmnd->fullstr[i+1] == '"' || cmnd->fullstr[i+1] == '*') {
		delim = cmnd->fullstr[i+1];
		i++;
	}

	// fullstr is now our search text and replace text seperated by a space
	cmnd->fullstr = cmnd->fullstr.substr(i+1);
	//strcpy(cmnd->fullstr, &cmnd->fullstr[i+1]);


	// we search until we find the deliminator we're looking for
	pos=0;
	i = cmnd->fullstr.length();
	while(pos < i) {
		if(cmnd->fullstr[pos] == delim)
			break;
		pos++;
	}

	if(pos == i) {
		if(delim != ' ')
			player->print("Deliminator not found.\n");
		else
			player->print("No replace text found.\n");
		return(0);
	}

	// cut the string apart
	cmnd->fullstr[pos] = 0;
	search = cmnd->fullstr;

	// if it's not a space, we need to add 2 to get rid of the space and
	// next deliminator
	if(delim != ' ') {
		pos += 2;
		// although we don't have to, we're enforcing that the deliminators
		// equal each other so people are consistent with the usage of this function
		if(cmnd->fullstr[pos] != delim) {
			player->print("Deliminators do not match up.\n");
			return(0);
		}
	}

	// fullstr now has our replace text
	//strcpy(cmnd->fullstr, &cmnd->fullstr[pos+1]);
	cmnd->fullstr = cmnd->fullstr.substr(pos+1);
	if(delim != ' ') {
		if(cmnd->fullstr[cmnd->fullstr.length()-1] != delim) {
			player->print("Deliminators do not match up.\n");
			return(0);
		}
		cmnd->fullstr[cmnd->fullstr.length()-1] = 0;
	}


	// the text we are searching for is in "search"
	// the text we are replacing it with is in "fullstr"


	// we will use i to help us reuse code
	// 0 = sdesc, 1 = ldesc
	i = 0;
	// if only long desc, skip short desc
	if(ldesc && !sdesc)
		i++;

	// loop for the short and long desc
	do {
		// loop for skip
		do {
			if(!i)
				n = room->getShortDescription().find(search.c_str(), skPos);
			else
				n = room->getLongDescription().find(search.c_str(), skPos);

			if(n >= 0) {
				if(--skip > 0)
					skPos = n + 1;
				else {
					if(!i) {
						temp = room->getShortDescription().substr(0, n);
						temp += cmnd->fullstr;
						temp += room->getShortDescription().substr(n + search.length(), room->getShortDescription().length());
						room->setShortDescription(temp);
					} else {
						temp = room->getLongDescription().substr(0, n);
						temp += cmnd->fullstr;
						temp += room->getLongDescription().substr(n + search.length(), room->getLongDescription().length());
						room->setLongDescription(temp);
					}
					player->print("Replaced.\n");
					room->escapeText();
					return(0);
				}
			}
		} while(n >= 0 && skip);

		// if we're on long desc (i=1), we'll stop after this, so no worries
		// if we're on short desc and not doing long desc, we need to stop
		if(sdesc && !ldesc)
			i++;
		i++;
	} while(i<2);

	player->print("Pattern not found.\n");
	return(0);
}


//*********************************************************************
//						dmDelete
//*********************************************************************
// Allows staff to delete some/all of the room description

int dmDelete(Player* player, cmd* cmnd) {
	UniqueRoom	*room = player->parent_rom;
	int		pos=0;
	int unsigned i=0;
	bool	sdesc=false, ldesc=false, phrase=false, after=false;

	if(!needUniqueRoom(player))
		return(0);

	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Room is not in any of your alotted room ranges.\n");
		return(0);
	}

	if(cmnd->num < 2) {
		player->print("syntax: *delete [-ASLPE] <delete_word>\n");
		return(0);
	}

	// take care of the easy one
	if(!strcmp(cmnd->str[1], "-a")) {
		room->setShortDescription("");
		room->setLongDescription("");
	} else {

		// determine our flags
		i=1;
		while(i < strlen(cmnd->str[1])) {
			switch(cmnd->str[1][i]) {
			case 'l':
				ldesc = true;
				break;
			case 's':
				sdesc = true;
				break;
			case 'p':
				phrase = true;
				break;
			case 'e':
				after = true;
				break;
			default:
				break;
			}
			i++;
		}

		// a simple delete operation
		if(!phrase && !after) {
			if(sdesc)
				room->setShortDescription("");
			if(ldesc)
				room->setLongDescription("");
			if(!sdesc && !ldesc) {
				player->print("Invalid syntax.\n");
				return(0);
			}
		} else {

			// we need to figure out what our phrase is
			// turn fullstr into what we want to delete
			i = strlen(cmnd->str[0]) + strlen(cmnd->str[1]) + 1;

			// fullstr is now our phrase
			cmnd->fullstr = cmnd->fullstr.substr(i+1);
			//strcpy(cmnd->fullstr, &cmnd->fullstr[i+1]);


			// we will use i to help us reuse code
			// 0 = sdesc, 1 = ldesc
			i = 0;
			// if only long desc, skip short desc
			if(ldesc && !sdesc)
				i++;

			// loop!
			do {
				if(!i)
					pos = room->getShortDescription().find(cmnd->fullstr);
				else
					pos = room->getLongDescription().find(cmnd->fullstr);

				if(pos >= 0)
					break;

				// if we're on long desc (i=1), we'll stop after this, so no worries
				// if we're on short desc and not doing long desc, we need to stop
				if(sdesc && !ldesc)
					i++;
				i++;
			} while(i<2);


			// did we find it?
			if(pos < 0) {
				player->print("Pattern not found.\n");
				return(0);
			}

			// we delete everything after the phrase
			if(after) {

				if(!phrase)
					pos += cmnd->fullstr.length();

				// if it's in the short desc, and they wanted to delete
				// from both short and long, then delete all of long
				if(!i && !(sdesc ^ ldesc))
					room->setLongDescription("");

				if(!i)
					room->setShortDescription(room->getShortDescription().substr(0, pos));
				else
					room->setLongDescription(room->getLongDescription().substr(0, pos));

			// only delete the phrase
			} else {

				if(!i)
					room->setShortDescription(room->getShortDescription().substr(0, pos) + room->getShortDescription().substr(pos + cmnd->fullstr.length(), room->getShortDescription().length()));
				else
					room->setLongDescription(room->getLongDescription().substr(0, pos) + room->getLongDescription().substr(pos + cmnd->fullstr.length(), room->getLongDescription().length()));

			}

		} // phrase && after

	} // *del -A

	log_immort(true, player, "%s deleted description in room %s.\n", player->name,
		player->parent_rom->info.str().c_str());

	player->print("Deleted.\n");
	return(0);
}

//*********************************************************************
//						dmNameRoom
//*********************************************************************
int dmNameRoom(Player* player, cmd* cmnd) {

	if(!needUniqueRoom(player))
		return(0);
	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Room is not in any of your alotted room ranges.\n");
		return(0);
	}

	bstring	name = getFullstrText(cmnd->fullstr, 1);

	if(name == "" || Pueblo::is(name)) {
		player->print("Rename room to what?\n");
		return(0);
	}

	if(name.getLength() > 79)
		name = name.left(79);

	strcpy(player->parent_rom->name, name.c_str());
	log_immort(true, player, "%s renamed room %s.\n", player->name, player->getRoom()->fullName().c_str());
	player->print("Done.\n");

	return(0);
}

//*********************************************************************
//						dmDescription
//*********************************************************************
// Allows a staff to add the given text to the room description.

int dmDescription(Player* player, cmd* cmnd, bool append) {
	UniqueRoom	*room = player->parent_rom;
	int unsigned i=0;
	bool	sdesc=false, newline=false;

	if(!needUniqueRoom(player))
		return(0);
	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Room is not in any of your alotted room ranges.\n");
		return(0);
	}

	if(cmnd->num < 2) {
		player->print("syntax: *%s [-sn] <text>\n", append ? "append" : "prepend");
		return(0);
	}

	// turn fullstr into what we want to append
	i = strlen(cmnd->str[0]);

	sdesc = cmnd->str[1][0] == '-' && (cmnd->str[1][1] == 's' || cmnd->str[1][2] == 's');
	newline = !(cmnd->str[1][0] == '-' && (cmnd->str[1][1] == 'n' || cmnd->str[1][2] == 'n'));

	// keep chopping
	if(sdesc || !newline)
		i += strlen(cmnd->str[1]) + 1;

	cmnd->fullstr = cmnd->fullstr.substr(i+1);
//	strcpy(cmnd->fullstr, &cmnd->fullstr[i+1]);

	if(cmnd->fullstr.find("  ") != cmnd->fullstr.npos)
		player->printColor("Do not use double spaces in room descriptions! Use ^W*wrap^x to fix this.\n");

	if(sdesc) {
		// short descriptions
		newline = newline && room->getShortDescription() != "";

		if(append) {
			room->appendShortDescription(newline ? "\n" : "");
			room->appendShortDescription(cmnd->fullstr);
		} else {
			if(newline)
			    cmnd->fullstr += "\n";
			room->setShortDescription(cmnd->fullstr + room->getShortDescription());
		}

		player->print("Short description %s.\n", append ? "appended" : "prepended");
	} else {
		// long descriptions
		newline = newline && room->getLongDescription() != "";

		if(append) {
			room->appendLongDescription(newline ? "\n" : "");
			room->appendLongDescription(cmnd->fullstr);
		} else {
			if(newline)
			    cmnd->fullstr += "\n";
			room->setLongDescription(cmnd->fullstr + room->getLongDescription());
		}

		player->print("Long description %s.\n", append ? "appended" : "prepended");
	}

	player->parent_rom->escapeText();
	log_immort(true, player, "%s descripted in room %s.\n", player->name,
		player->parent_rom->info.str().c_str());
	return(0);
}

//*********************************************************************
//						dmAppend
//*********************************************************************
int dmAppend(Player* player, cmd* cmnd) {
	return(dmDescription(player, cmnd, true));
}
//*********************************************************************
//						dmPrepend
//*********************************************************************
int dmPrepend(Player* player, cmd* cmnd) {
	return(dmDescription(player, cmnd, false));
}



//*********************************************************************
//						dmMobList
//*********************************************************************
// Display information about what mobs will randomly spawn.

void showMobList(Player* player, WanderInfo *wander, bstring type) {
	std::map<int, CatRef>::iterator it;
	Monster *monster=0;
	bool	found=false, maybeAggro=false;
	std::ostringstream oStr;

	for(it = wander->random.begin(); it != wander->random.end() ; it++) {
		if(!found)
			oStr << "^cTraffic = " << wander->getTraffic() << "\n";
		found=true;

		if(!(*it).second.id)
			continue;
		if(!loadMonster((*it).second, &monster))
			continue;

		if(monster->flagIsSet(M_AGGRESSIVE))
			oStr << "^r";
		else {
			maybeAggro = monster->flagIsSet(M_AGGRESSIVE_EVIL) ||
				monster->flagIsSet(M_AGGRESSIVE_GOOD) ||
				monster->flagIsSet(M_AGGRESSIVE_AFTER_TALK) ||
				monster->flagIsSet(M_CLASS_AGGRO_INVERT) ||
				monster->flagIsSet(M_RACE_AGGRO_INVERT) ||
				monster->flagIsSet(M_DEITY_AGGRO_INVERT);

			if(!maybeAggro) {
				std::map<int, RaceData*>::iterator rIt;
				for(rIt = gConfig->races.begin() ; rIt != gConfig->races.end() ; rIt++) {
					if(monster->isRaceAggro((*rIt).second->getId(), false)) {
						maybeAggro = true;
						break;
					}
				}
			}

			if(!maybeAggro) {
				for(int n=1; n<STAFF; n++) {
					if(monster->isClassAggro(n, false)) {
						maybeAggro = true;
						break;
					}
				}
			}

			if(!maybeAggro) {
				std::map<int, DeityData*>::iterator dIt;
				for(dIt = gConfig->deities.begin() ; dIt != gConfig->deities.end() ; dIt++) {
					if(monster->isDeityAggro((*dIt).second->getId(), false)) {
						maybeAggro = true;
						break;
					}
				}
			}

			if(maybeAggro)
				oStr << "^y";
			else
				oStr << "^g";
		}

		oStr << "Slot " << std::setw(2) << (*it).first+1 << ": " << monster->name << " "
			 << "[" << monType::getName(monster->getType()) << ":" << monType::getHitdice(monster->getType()) << "HD]\n"
			 << "         ^x[I:" << monster->info.str() << " L:" << monster->getLevel()
			 << " X:" << monster->getExperience() << " G:" << monster->coins[GOLD]
			 << " H:" << monster->hp.getMax() << " M:" << monster->mp.getMax()
			 << " N:" << (monster->getNumWander() ? monster->getNumWander() : 1)
			 << (monster->getAlignment() > 0 ? "^g" : "") << (monster->getAlignment() < 0 ? "^r" : "")
			 << " A:" << monster->getAlignment()
			 << "^x D:" << monster->damage.average() << "]\n";

		free_crt(monster);
	}

	if(!found)
		oStr << "    No random monsters currently come in this " << type << ".";

	player->printColor("%s\n", oStr.str().c_str());
}

int dmMobList(Player* player, cmd* cmnd) {

	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Error: Room number not inside any of your alotted ranges.\n");
		return(0);
	}

	player->print("Random monsters which come in this room:\n");

	if(player->parent_rom)
		showMobList(player, &player->parent_rom->wander, "room");
	else if(player->area_room) {

		Area* area = player->area_room->area;
		std::list<AreaZone*>::iterator it;
		AreaZone *zone=0;

		for(it = area->zones.begin() ; it != area->zones.end() ; it++) {
			zone = (*it);
			if(zone->inside(area, &player->area_room->mapmarker)) {
				player->print("Zone: %s\n", zone->name.c_str());
				showMobList(player, &zone->wander, "zone");
			}
		}

		TileInfo* tile = area->getTile(area->getTerrain(0, &player->area_room->mapmarker, 0, 0, 0, true), area->getSeasonFlags(&player->area_room->mapmarker));
		if(tile && tile->wander.getTraffic()) {
			player->print("Tile: %s\n", tile->getName().c_str());
			showMobList(player, &tile->wander, "tile");
		}
	}

	return(0);
}

//*********************************************************************
//						dmWrap
//*********************************************************************
// dmWrap will  either wrap the short or long desc of a room to the
// specified length, for sanity, a range of  60 - 78 chars is the limit.

int dmWrap(Player* player, cmd* cmnd) {
	UniqueRoom	*room = player->parent_rom;
	int		wrap=0;
	bool err=false, which=false;

	bstring text = "";

	if(!needUniqueRoom(player))
		return(0);
	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Room is not in any of your alotted room ranges.\n");
		return(0);
	}


	// parse the input command for syntax/range problems
	if(cmnd->num < 2)
		err = true;
	else {
		switch(cmnd->str[1][0]) {
		case 'l':
			which = true;
			text = room->getLongDescription();
			break;
		case 's':
			which = false;
			text = room->getShortDescription();
			break;
		default:
			err = true;
			break;
		}
		if(!err) {
			wrap = cmnd->val[1];
			if(wrap < 60 || wrap > 78)
				err = true;
		}
	}
	if(err) {
		player->print("*wrap <s | l> <len> where len is between 60 and 78 >\n");
		return(0);
	}

	if((!which && room->getShortDescription() == "") || (which && room->getLongDescription() == "")) {
		player->print("No text to wrap!\n");
		return(0);
	}

	// adjust!
	wrap++;

	// replace!
	if(!which)
		room->setShortDescription(wrapText(room->getShortDescription(), wrap));
	else
		room->setLongDescription(wrapText(room->getLongDescription(), wrap));

	player->print("Text wrapped.\n");
	player->parent_rom->escapeText();
	log_immort(false, player, "%s wrapped the description in room %s.\n", player->name, player->getRoom()->fullName().c_str());
	return(0);
}



//*********************************************************************
//						dmDeleteAllExits
//*********************************************************************

int dmDeleteAllExits(Player* player, cmd* cmnd) {
	xtag	*xp=0, *prev = 0;

	xp = player->getRoom()->first_ext;
	if(!xp) {
		player->print("No exits to delete.\n");
		return(0);
	}

	player->getRoom()->first_ext = 0;
	while(xp) {
		prev = xp;
		xp = xp->next_tag;
		delete prev;
	}

	// sorry, can't delete exits in overland
	if(player->area_room)
		player->area_room->updateExits();

	player->print("All exits deleted.\n");

	log_immort(true, player, "%s deleted all exits in room %s.\n", player->name, player->getRoom()->fullName().c_str());
	room_track(player);
	return(0);
}

//*********************************************************************
//							exit_ordering
//*********************************************************************
// 1 mean exit2 goes in front of exit1
// 0 means keep looking

int exit_ordering(char *exit1, char *exit2) {

	// always skip if they're the same name
	if(!strcmp(exit1, exit2)) return(0);

	// north east south west
	if(!strcmp(exit1, "north")) return(0);
	if(!strcmp(exit2, "north")) return(1);

	if(!strcmp(exit1, "east")) return(0);
	if(!strcmp(exit2, "east")) return(1);

	if(!strcmp(exit1, "south")) return(0);
	if(!strcmp(exit2, "south")) return(1);

	if(!strcmp(exit1, "west")) return(0);
	if(!strcmp(exit2, "west")) return(1);

	// northeast northwest southeast southwest
	if(!strcmp(exit1, "northeast")) return(0);
	if(!strcmp(exit2, "northeast")) return(1);

	if(!strcmp(exit1, "northwest")) return(0);
	if(!strcmp(exit2, "northwest")) return(1);

	if(!strcmp(exit1, "southeast")) return(0);
	if(!strcmp(exit2, "southeast")) return(1);

	if(!strcmp(exit1, "southwest")) return(0);
	if(!strcmp(exit2, "southwest")) return(1);

	if(!strcmp(exit1, "up")) return(0);
	if(!strcmp(exit2, "up")) return(1);

	if(!strcmp(exit1, "down")) return(0);
	if(!strcmp(exit2, "down")) return(1);

	// alphabetic
	return strcmp(exit1, exit2) > 0 ? 1 : 0;
}


//*********************************************************************
//						dmArrangeExits
//*********************************************************************

void BaseRoom::arrangeExits(Player* player) {
	xtag	*xp=0, *prev=0, *sxp=0;

	xp = first_ext;

	if(!xp || !xp->next_tag) {
		if(player)
			player->print("No exits to rearrange!\n");
		return;
	}

	// start at the 2nd one
	prev = xp;
	xp = xp->next_tag;

	while(xp) {
		if(player)
			player->print("Checking exit %s vs %s...%s\n",
				xp->ext->name, prev->ext->name,
				exit_ordering(prev->ext->name, xp->ext->name) ? "" : "ok.");

		// we're looking at an exit, we've got a pointer to the previous exit
		if(exit_ordering(prev->ext->name, xp->ext->name)) {
			// if xp goes before prev, start rearranging!
			if(	prev->ext == first_ext->ext ||
				exit_ordering(first_ext->ext->name, xp->ext->name)
			) {
				// if it should go before the FIRST exit..
				if(player)
					player->print("  Moving %s to the front...\n", xp->ext->name);
				prev->next_tag = xp->next_tag;
				xp->next_tag = first_ext;
				first_ext = xp;
				xp = prev->next_tag;
			} else {
				// otherwise, start over from the top:
				// find where xp belongs!
				if(player)
					player->print("  Finding a spot for %s...\n", xp->ext->name);
				sxp = first_ext;
				while(!exit_ordering(sxp->next_tag->ext->name, xp->ext->name))
					sxp = sxp->next_tag;
				if(player)
					player->print("  Spot found after: %s.\n", sxp->ext->name);

				// ok we found it, time to do some magic
				prev->next_tag = xp->next_tag;
				xp->next_tag = sxp->next_tag;
				sxp->next_tag = xp;
				xp = prev->next_tag;
			}
		} else {
			prev = xp;
			xp = xp->next_tag;
		}
	}

	if(player)
		player->print("Exits rearranged!\n");
}

int dmArrangeExits(Player* player, cmd* cmnd) {
	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Error: Room number not inside any of your alotted ranges.\n");
		return(0);
	}

	player->getRoom()->arrangeExits(player);
	return(0);
}


//*********************************************************************
//							link_rom
//*********************************************************************
// from this room to unique room
void link_rom(BaseRoom* room, Location l, bstring str) {
	Exit	*exit=0;
	xtag	*xp=0, *prev=0, *temp=0;

	const char* dir = str.c_str();
	xp = room->first_ext;

	while(xp) {
		exit = xp->ext;
		prev = xp;
		xp = xp->next_tag;
		if(!strcmp(exit->name, dir)) {
			strcpy(exit->name, dir);
			exit->target = l;
			return;
		}
	}

	temp = new xtag;
	exit = new Exit;

    exit->setRoom(room);

	strcpy(exit->name, dir);
	exit->target = l;

	temp->next_tag = 0;
	temp->ext = exit;

	if(prev)
		prev->next_tag = temp;
	else
		room->first_ext = temp;
}

void link_rom(BaseRoom* room, short tonum, bstring str) {
	Location l;
	l.room.id = tonum;
	link_rom(room, l, str);
}
void link_rom(BaseRoom* room, CatRef cr, bstring str) {
	Location l;
	l.room = cr;
	link_rom(room, l, str);
}
void link_rom(BaseRoom* room, MapMarker *mapmarker, bstring str) {
	Location l;
	l.mapmarker = *mapmarker;
	link_rom(room, l, str);
}


//*********************************************************************
//						del_exit
//*********************************************************************

int del_exit(BaseRoom* room, Exit *exit) {
	xtag	*xp = room->first_ext, *prev=0;

	while(xp) {
		if(xp->ext == exit) {
			if(prev)
				prev->next_tag = xp->next_tag;
			else
				room->first_ext = xp->next_tag;

			delete xp->ext;
			delete xp;
			return(1);
		}
		prev = xp;
		xp = xp->next_tag;
	}

	return(0);
}

int del_exit(BaseRoom* room, const char *dir) {
	xtag	*xp = room->first_ext, *prev=0;

	while(xp) {
		if(!strcmp(xp->ext->name, dir)) {
			if(prev)
				prev->next_tag = xp->next_tag;
			else
				room->first_ext = xp->next_tag;

			delete xp->ext;
			delete xp;
			return(1);
		}
		prev = xp;
		xp = xp->next_tag;
	}

	return(0);
}


//*********************************************************************
//						dmFix
//*********************************************************************

int dmFix(Player* player, cmd* cmnd, bstring name, char find, char replace) {
	Exit	*exit=0;
	int		i=0;
	bool	fixed=false;

	if(cmnd->num < 2) {
		player->print("Syntax: *%sup <exit>\n", name.c_str());
		return(0);
	}

	exit = findExit(player, cmnd);
	if(!exit) {
		player->print("You don't see that exit.\n");
		return(0);
	}

	for(i=strlen(exit->name); i>0; i--) {
		if(exit->name[i] == find) {
			exit->name[i] = replace;
			fixed = true;
		}
	}

	if(fixed) {
		log_immort(true, player, "%s %sed the exit '%s' in room %s.\n",
			player->name, name.c_str(), exit->name, player->getRoom()->fullName().c_str());
		player->print("Done.\n");
	} else
		player->print("Couldn't find any underscores.\n");

	return(0);
}

//*********************************************************************
//						dmUnfixExit
//*********************************************************************
int dmUnfixExit(Player* player, cmd* cmnd) {
	return(dmFix(player, cmnd, "unfix", ' ', '_'));
}

//*********************************************************************
//						dmFixExit
//*********************************************************************
int dmFixExit(Player* player, cmd* cmnd) {
	return(dmFix(player, cmnd, "fix", '_', ' '));
}


//*********************************************************************
//						dmRenameExit
//*********************************************************************

int dmRenameExit(Player* player, cmd* cmnd) {
	Exit	*exit=0;

	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Room is not in any of your alotted room ranges.\n");
		return(0);
	}

	if(cmnd->num < 3) {
		player->print("Syntax: *xrename <old exit>[#] <new exit>\n");
		return(0);
	}


	exit = findExit(player, cmnd);
	if(!exit) {
		player->print("There is no exit here by that name.\n");
		return(0);
	}

	bstring newName = getFullstrText(cmnd->fullstr, 2);

	if(newName.getLength() > 20) {
		player->print("New exit name must be 20 characters or less in length.\n");
		return(0);
	}


	player->printColor("Exit \"%s^x\" renamed to \"%s^x\".\n", exit->name, newName.c_str());
	log_immort(false, player, "%s renamed exit %s^g to %s^g in room %s.\n",
		player->name, exit->name, newName.c_str(), player->getRoom()->fullName().c_str());
	room_track(player);

	if(getDir(newName) != NoDirection)
		exit->setDirection(NoDirection);

	strcpy(exit->name, newName.c_str());
	return(0);
}


//*********************************************************************
//						dmDestroyRoom
//*********************************************************************

int dmDestroyRoom(Player* player, cmd* cmnd) {
	if(!player->parent_rom) {
		player->print("Error: You need to be in a unique room to do that.\n");
		return(0);
	}
	if(!player->checkBuilder(player->parent_rom)) {
		player->print("Room is not in any of your alotted room ranges.\n");
		return(0);
	}
	if(	player->parent_rom->info.isArea("test") &&
		player->parent_rom->info.id == 1
	) {
		player->print("Sorry, you cannot destroy this room.\n");
		player->print("It is the Builder Waiting Room.\n");
		return(0);
	}
	if(player->bound.room == player->room) {
		player->print("Sorry, you cannot destroy this room.\n");
		player->print("It is your bound room.\n");
		return(0);
	}

	std::map<bstring, StartLoc*>::iterator sIt;
	for(sIt = gConfig->start.begin() ; sIt != gConfig->start.end() ; sIt++) {
		if(	player->parent_rom->info == (*sIt).second->getBind().room ||
			player->parent_rom->info == (*sIt).second->getRequired().room
		) {
			player->print("Sorry, you cannot destroy this room.\n");
			player->print("It is important to starting locations.\n");
			return(0);
		}
	}

	std::list<CatRefInfo*>::const_iterator crIt;
	for(crIt = gConfig->catRefInfo.begin() ; crIt != gConfig->catRefInfo.end() ; crIt++) {
		if(player->parent_rom->info.isArea((*crIt)->getArea())) {
			if((*crIt)->getLimbo() == player->parent_rom->info.id) {
				player->print("Sorry, you cannot destroy this room.\n");
				player->print("It is a Limbo room.\n");
				return(0);
			}
			if((*crIt)->getRecall() == player->parent_rom->info.id) {
				player->print("Sorry, you cannot destroy this room.\n");
				player->print("It is a recall room.\n");
				return(0);
			}
		}
	}

	log_immort(true, player, "%s destroyed room %s.\n",
		player->name, player->getRoom()->fullName().c_str());
	player->parent_rom->destroy();
	return(0);
}


//*********************************************************************
//							findRoomsWithFlag
//*********************************************************************

void findRoomsWithFlag(const Player* player, Range range, int flag) {
	Async async;
	if(async.branch(player, CHILD_PRINT) == AsyncExternal) {
		std::ostringstream oStr;
		bool found = false;

		if(range.low.id <= range.high) {
			UniqueRoom* room=0;
			CatRef cr;
			int high = range.high;

			cr.setArea(range.low.area);
			cr.id = range.low.id;
			if(range.low.id == -1 && range.high == -1) {
				cr.id = 1;
				high = RMAX;
			}

			for(; cr.id < high; cr.id++) {
				if(!loadRoom(cr, &room))
					continue;

				if(room->flagIsSet(flag)) {
					if(player->isStaff())
						oStr << room->info.rstr() << " - ";
					oStr << room->getName() << "^x\n";
					found = true;
				}
			}
		}

		printf("^YLocations found:^x\n");
		if(!found) {
			printf("No available locations were found.");
		} else {
			bstring output = oStr.str();
			printf("%s", output.c_str());
		}
		exit(0);
	}
}

void findRoomsWithFlag(const Player* player, CatRef area, int flag) {
	Async async;
	if(async.branch(player, CHILD_PRINT) == AsyncExternal) {
		struct	dirent *dirp=0;
		DIR		*dir=0;
		std::ostringstream oStr;
		bool	found = false;
		char	filename[250];
		UniqueRoom	*room=0;

		// This tells us just to get the path, not the file,
		// and tells loadRoomFromFile to ignore the CatRef
		area.id = -1;
		bstring path = roomPath(area);

		if((dir = opendir(path.c_str())) != NULL) {
			while((dirp = readdir(dir)) != NULL) {
				// is this a room file?
				if(dirp->d_name[0] == '.')
					continue;

				sprintf(filename, "%s/%s", path.c_str(), dirp->d_name);
				if(!loadRoomFromFile(area, &room, filename))
					continue;

				if(room->flagIsSet(flag)) {
					if(player->isStaff())
						oStr << room->info.rstr() << " - ";
					oStr << room->getName() << "^x\n";
					found = true;
				}
			}
		}

		printf("^YLocations found:^x\n");
		if(!found) {
			printf("No available locations were found.");
		} else {
			bstring output = oStr.str();
			printf("%s", output.c_str());
		}
		exit(0);
	}
}


//*********************************************************************
//						dmFind
//*********************************************************************

int dmFind(Player* player, cmd* cmnd) {
	bstring type = getFullstrText(cmnd->fullstr, 1);
	CatRef cr;

	if(player->parent_rom)
		cr = player->parent_rom->info;
	else
		cr.setArea("area");

	if(type == "r")
		type = "room";
	else if(type == "o")
		type = "object";
	else if(type == "m")
		type = "monster";

	if(type == "" || (type != "room" && type != "object" && type != "monster")) {
		if(type != "")
			player->print("\"%s\" is not a valid type.\n", type.c_str());
		player->print("Search for next available of the following: room, object, monster.\n");
		return(0);
	}

	if(!player->checkBuilder(cr)) {
		player->print("Error: this area is out of your range; you cannot *find here.\n");
		return(0);
	}

	if(type == "monster" && !player->canBuildMonsters()) {
		player->print("Error: you cannot work with monsters.\n");
		return(0);
	}
	if(type == "object" && !player->canBuildObjects()) {
		player->print("Error: you cannot work with objects.\n");
		return(0);
	}


	Async async;
	if(async.branch(player, CHILD_PRINT) == AsyncExternal) {
		cr = findNextEmpty(type, cr.area);

		printf("^YNext available %s in area %s:^x\n", type.c_str(), cr.area.c_str());
		if(cr.id == -1)
			printf("No empty %ss found.", type.c_str());
		else {
			bstring output = cr.rstr();
			printf("%s", output.c_str());
		}
		exit(0);
	} else {
		player->printColor("Searching for next available %s in ^W%s^x.\n", type.c_str(), cr.area.c_str());
	}

	return(0);
}


//*********************************************************************
//							findNextEmpty
//*********************************************************************
// searches for the next empty room/object/monster in the area

CatRef findNextEmpty(bstring type, bstring area) {
	CatRef cr;
	cr.setArea(area);

	if(type == "room") {
		UniqueRoom* room=0;
		
		for(cr.id = 1; cr.id < RMAX; cr.id++)
			if(!loadRoom(cr, &room))
				return(cr);
	} else if(type == "object") {
		Object *object=0;

		for(cr.id = 1; cr.id < OMAX; cr.id++) {
			if(!loadObject(cr, &object))
				return(cr);
			else
				delete object;
		}
	} else if(type == "monster") {
		Monster *monster=0;

		for(cr.id = 1; cr.id < MMAX; cr.id++) {
			if(!loadMonster(cr, &monster))
				return(cr);
			else
				free_crt(monster);
		}
	}

	// -1 indicates failure
	cr.id = -1;
	return(cr);
}
