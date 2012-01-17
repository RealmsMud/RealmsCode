/*
 * files1.cpp
 *   File I/O Routines.
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

#include <sstream>
#include <iomanip>
#include <locale>
#include <iostream>
#include <fstream>

namespace Path {
	const char* Bin = "/home/realms/realms/bin/";
	const char* Log = "/home/realms/realms/log/";
	const char* BugLog = "/home/realms/realms/log/bug/";
	const char* StaffLog = "/home/realms/realms/log/staff/";
	const char* BankLog = "/home/realms/realms/log/bank/";
	const char* GuildBankLog = "/home/realms/realms/log/guildbank/";

	const char* UniqueRoom = "/home/realms/realms/rooms/";
	const char* AreaRoom = "/home/realms/realms/rooms/area/";
	const char* Monster = "/home/realms/realms/monsters/";
	const char* Object = "/home/realms/realms/objects/";
	const char* Player = "/home/realms/realms/player/";
	const char* PlayerBackup = "/home/realms/realms/player/backup/";

	const char* Config = "/home/realms/realms/config/";

	const char* Code = "/home/realms/realms/config/code/";
	const char* Python = "/home/realms/realms/config/code/python/";
	const char* Game = "/home/realms/realms/config/game/";
	const char* AreaData = "/home/realms/realms/config/game/area/";
	const char* Talk = "/home/realms/realms/config/game/talk/";
	const char* Desc = "/home/realms/realms/config/game/ddesc/";
	const char* Sign = "/home/realms/realms/config/game/signs/";

	const char* PlayerData = "/home/realms/realms/config/player/";
	const char* Bank = "/home/realms/realms/config/player/bank/";
	const char* GuildBank = "/home/realms/realms/config/player/guildbank/";
	const char* History = "/home/realms/realms/config/player/history/";
	const char* Post = "/home/realms/realms/config/player/post/";

	const char* BaseHelp = "/home/realms/realms/help/";
	const char* Help = "/home/realms/realms/help/help/";
	const char* Wiki = "/home/realms/realms/help/wiki/";
	const char* DMHelp = "/home/realms/realms/help/dmhelp/";
	const char* BuilderHelp = "/home/realms/realms/help/bhelp/";
	const char* HelpTemplate = "/home/realms/realms/help/template/";
}
//*********************************************************************
//						path functions
//*********************************************************************

char* objectPath(const CatRef cr) {
	static char filename[256];
	if(cr.id < 0)
		sprintf(filename, "%s/%s/", Path::Object, cr.area.c_str());
	else
		sprintf(filename, "%s/%s/o%05d.xml", Path::Object, cr.area.c_str(), cr.id);
	return(filename);
}
char* monsterPath(const CatRef cr) {
	static char filename[256];
	if(cr.id < 0)
		sprintf(filename, "%s/%s/", Path::Monster, cr.area.c_str());
	else
		sprintf(filename, "%s/%s/m%05d.xml", Path::Monster, cr.area.c_str(), cr.id);
	return(filename);
}
char* roomPath(const CatRef cr) {
	static char filename[256];
	if(cr.id < 0)
		sprintf(filename, "%s/%s/", Path::UniqueRoom, cr.area.c_str());
	else
		sprintf(filename, "%s/%s/r%05d.xml", Path::UniqueRoom, cr.area.c_str(), cr.id);
	return(filename);
}
char* roomBackupPath(const CatRef cr) {
	static char filename[256];
	if(cr.id < 0)
		sprintf(filename, "%s/%s/backup/", Path::UniqueRoom, cr.area.c_str());
	else
		sprintf(filename, "%s/%s/backup/r%05d.xml", Path::UniqueRoom, cr.area.c_str(), cr.id);
	return(filename);
}

//*********************************************************************
//						checkPaths
//*********************************************************************

bool Path::checkPaths() {
	bool ok = true;
	
	ok = Path::checkDirExists(Path::Bin) && ok;
	ok = Path::checkDirExists(Path::Log) && ok;
	ok = Path::checkDirExists(Path::BugLog) && ok;
	ok = Path::checkDirExists(Path::StaffLog) && ok;
	ok = Path::checkDirExists(Path::BankLog) && ok;
	ok = Path::checkDirExists(Path::GuildBankLog) && ok;

	ok = Path::checkDirExists(Path::UniqueRoom) && ok;
	ok = Path::checkDirExists(Path::AreaRoom) && ok;
	ok = Path::checkDirExists(Path::Monster) && ok;
	ok = Path::checkDirExists(Path::Object) && ok;
	ok = Path::checkDirExists(Path::Player) && ok;
	ok = Path::checkDirExists(Path::PlayerBackup) && ok;

	ok = Path::checkDirExists(Path::Config) && ok;

	ok = Path::checkDirExists(Path::Code) && ok;
	ok = Path::checkDirExists(Path::Python) && ok;
	ok = Path::checkDirExists(Path::Game) && ok;
	ok = Path::checkDirExists(Path::AreaData) && ok;
	ok = Path::checkDirExists(Path::Talk) && ok;
	ok = Path::checkDirExists(Path::Desc) && ok;
	ok = Path::checkDirExists(Path::Sign) && ok;

	ok = Path::checkDirExists(Path::PlayerData) && ok;
	ok = Path::checkDirExists(Path::Bank) && ok;
	ok = Path::checkDirExists(Path::GuildBank) && ok;
	ok = Path::checkDirExists(Path::History) && ok;
	ok = Path::checkDirExists(Path::Post) && ok;

	ok = Path::checkDirExists(Path::BaseHelp) && ok;
	ok = Path::checkDirExists(Path::Help) && ok;
	ok = Path::checkDirExists(Path::Wiki) && ok;
	ok = Path::checkDirExists(Path::DMHelp) && ok;
	ok = Path::checkDirExists(Path::BuilderHelp) && ok;
	ok = Path::checkDirExists(Path::HelpTemplate) && ok;

	return(ok);
}

//*********************************************************************
//						checkDirExists
//*********************************************************************

bool Path::checkDirExists(const char* filename) {
	struct stat     f_stat;
	if(stat(filename, &f_stat)) {
		return(!mkdir(filename, 0755));
	}
	return(true);
}

bool Path::checkDirExists(bstring area, char* (*fn)(const CatRef cr)) {
	char	filename[256];
	CatRef	cr;

	// this will trigger the dir-only mode
	cr.setArea(area.c_str());
	cr.id = -1;
	strcpy(filename, (*fn)(cr));

	return(checkDirExists(filename));
}


//*********************************************************************
//						count_obj
//*********************************************************************
// Return the total number of objects contained within an object.
// If perm_only != 0, then only the permanent objects are counted.

int count_obj(Object* object, char perm_only) {
	otag	*op;
	int	total=0;

	op = object->first_obj;
	while(op) {
		if(!perm_only || (perm_only && (op->obj->flagIsSet(O_PERM_ITEM))))
			total++;
		op = op->next_tag;
	}
	return(total);
}

//*********************************************************************
//						count_inv
//*********************************************************************
// Returns the number of objects a creature has in their inventory.
// If perm_only != 0 then only those objects which are permanent
// will be counted.

int count_inv(Creature* creature, char perm_only) {
	otag	*op;
	int	total=0;

	op = creature->first_obj;
	while(op) {
		if(!perm_only || (perm_only && (op->obj->flagIsSet(O_PERM_ITEM))))
			total++;
		op = op->next_tag;
	}
	return(MIN(100,total));
}

//*********************************************************************
//						count_bag_inv
//*********************************************************************
int count_bag_inv(Creature* creature) {
	otag	*op;
	int	total=0;
	op = creature->first_obj;
	while(op) {
		total++;
		if(op->obj && op->obj->getType() == CONTAINER) {
			total += count_obj(op->obj, 0);
		}
		op = op->next_tag;
	}
	return(total);
}

//*********************************************************************
//						free_crt
//*********************************************************************
// This function releases the creature pointed to by the first parameter
// from memory. All items that creature has readied or carries will
// also be released. *ASSUMPTION*: This function will only be called
// from delete room. If it is called from somewhere else, unresolved
// links may remain and cause a game crash. *EXCEPTION*: This function
// can be called independently to free a player's information from
// memory (but not a monster).

void free_crt(Creature* creature, bool remove) {
	otag	*op=0, *tempo=0;
	ctag	*cp=0, *tempc=0;
	ttag	*tp=0, *tempt=0;
	int	i;
	for(i=0; i<MAXWEAR; i++) {
		if(creature->ready[i])
			creature->unequip(i+1, UNEQUIP_DELETE, false);
	}

	op = creature->first_obj;
	while(op) {
		tempo = op->next_tag;
		delete op->obj;
		delete op;
		op = tempo;
	}

	cp = creature->first_fol;
	while(cp) {
		tempc = cp->next_tag;
		if(cp->crt)
			free_crt(cp->crt);
		delete cp;
		cp = tempc;
	}

	tp = creature->first_tlk;
	creature->first_tlk = 0;
	while(tp) {
		tempt = tp->next_tag;
		if(tp->key)
			delete[] tp->key;
		if(tp->response)
			delete[] tp->response;
		if(tp->action)
			delete[] tp->action;
		if(tp->target)
			delete[] tp->target;
		delete tp;
		tp = tempt;
	}

	
	gServer->removeDelayedActions(creature);
	
	if(creature->isMonster())
		gServer->delActive(creature->getMonster());
	else if(remove)
		gServer->clearPlayer(creature->getPlayer());
	delete creature;
}

//**********************************************************************
//						writeHelpFiles
//**********************************************************************
// commands are written in initCommands
// socials are written in initCommands
// flags are written in loadFlags

bool Config::writeHelpFiles() const {
	bool success = false;

	success = writeSpellFiles() || success;

	return(success);
}

//**********************************************************************
//						loadHelpTemplate
//**********************************************************************

bstring loadHelpTemplate(const char* filename) {
	char	file[80], line[200];
	bstring str;
	
	sprintf(file, "%s%s.txt", Path::HelpTemplate, filename);
	std::ifstream in(file);

	// if there isn't a template
	if(!in)
		return("");

	// read all the information from the template in
	while(!in.eof()) {
		in.getline(line, 200, '\n');
		str += line;
		str += "\n";
	}
	in.close();

	return(str);
}
