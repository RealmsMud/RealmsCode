/*
 * Songs.cpp
 *	 Additional song-casting routines.
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
#include "effects.h"
#include "songs.h"

//*********************************************************************
//						Song
//*********************************************************************

Song::Song(xmlNodePtr rootNode) {
	xmlNodePtr curNode = rootNode->children;
	priority = 100;
	delay = 5;
	duration = 15;

	while(curNode) {
			 if(NODE_NAME(curNode, "Name")) { 
                 xml::copyToBString(name, curNode);
                 parseName();
             }
		else if(NODE_NAME(curNode, "Script")) xml::copyToBString(script, curNode);
		else if(NODE_NAME(curNode, "Effect")) xml::copyToBString(effect, curNode);
		else if(NODE_NAME(curNode, "Type")) xml::copyToBString(type, curNode);
		else if(NODE_NAME(curNode, "TargetType")) xml::copyToBString(targetType, curNode);
		else if(NODE_NAME(curNode, "Priority")) xml::copyToNum(priority, curNode);
		else if(NODE_NAME(curNode, "Description")) xml::copyToBString(desc, curNode);
		else if(NODE_NAME(curNode, "Delay")) xml::copyToNum(delay, curNode);
		else if(NODE_NAME(curNode, "Duration")) xml::copyToNum(duration, curNode);

		curNode = curNode->next;
	}
}


//*********************************************************************
//						save
//*********************************************************************

void Song::save(xmlNodePtr rootNode) const {
	xml::saveNonNullString(rootNode, "Name", name);
	xml::saveNonNullString(rootNode, "Script", script);
	xml::saveNonNullString(rootNode, "Effect", effect);
	xml::saveNonNullString(rootNode, "Type", type);
	xml::saveNonNullString(rootNode, "TargetType", targetType);
	xml::saveNonZeroNum<int>(rootNode, "Priority", priority);
	xml::saveNonNullString(rootNode,"Description", desc);
	xml::saveNonZeroNum<int>(rootNode, "Delay", priority);
	xml::saveNonZeroNum<int>(rootNode, "Duration", priority);

}

//*********************************************************************
//						getEffect
//*********************************************************************

const bstring& Song::getEffect() const {
	return(effect);
}

//*********************************************************************
//						getType
//*********************************************************************

const bstring& Song::getType() const {
	return(type);
}

//*********************************************************************
//						getTargetType
//*********************************************************************

const bstring& Song::getTargetType() const {
	return(targetType);
}

//*********************************************************************
//						runScript
//*********************************************************************

bool Song::runScript(MudObject* singer, MudObject* target) {

	if(script.empty())
		return(false);

	try {
		object localNamespace( (handle<>(PyDict_New())));

		object effectModule( (handle<>(PyImport_ImportModule("songLib"))) );
		localNamespace["songLib"] = effectModule;

		localNamespace["song"] = ptr(this);

		// Default retVal is true
		localNamespace["retVal"] = true;
		addMudObjectToDictionary(localNamespace, "actor", singer);
		addMudObjectToDictionary(localNamespace, "target", target);

		gServer->runPython(script, localNamespace);

		bool retVal = extract<bool>(localNamespace["retVal"]);
		return(retVal);
	}
	catch( error_already_set) {
		gServer->handlePythonError();
	}

	return(false);
}

//*********************************************************************
//						clearSongs
//*********************************************************************

void Config::clearSongs() {

	// TODO: Loop through all players and stop any songs being played
    for(PlayerMap::value_type p : gServer->players) {
        p.second->stopPlaying(true);
    }
	for(std::pair<bstring, Song*> sp : songs) {
		delete sp.second;
	}
	songs.clear();
}

//*********************************************************************
//						isPlaying
//*********************************************************************

bool Creature::isPlaying() {
	return(playing != NULL);
}

//*********************************************************************
//						getPlaying
//*********************************************************************

Song* Creature::getPlaying() {
	return(playing);
}

//*********************************************************************
//						setPlaying
//*********************************************************************

bool Creature::setPlaying(Song* newSong, bool echo) {
	bool wasPlaying = false;
	if(isPlaying()) {
 		stopPlaying(echo);
		wasPlaying = true;
	}

	playing = newSong;

	return(wasPlaying);
}

//*********************************************************************
//						stopPlaying
//*********************************************************************

bool Creature::stopPlaying(bool echo) {
	if(!isPlaying())
		return(false);

	if(echo) {
		print("You stop playing \"%s\"\n", getPlaying()->getName().c_str());
		getRoom()->print(getSock(), "%M stops playing %s\n", this, getPlaying()->getName().c_str());
	}
	playing = NULL;
	return(true);
}

//*********************************************************************
//						pulseSong
//*********************************************************************

bool Creature::pulseSong(long t) {
	if(!isPlaying())
		return(false);

	if(getLTLeft(LT_SONG_PLAYED, t))
		return(false);

	if(!playing)
		return(false);

	print("Pulsing song: %s\n", playing->getName().c_str());

	bstring targetType = playing->getTargetType();

	if(playing->getType().equals("effect", false)) {
		// Group effects affect the singer as well
		if(targetType.equals("self",false) || targetType.equals("group", false)) {
			addEffect(playing->getEffect(), -2, -2, this)->setDuration(playing->getDuration());
		}
		if(targetType.equals("group", false) && (getGroup() != NULL)) {
			Group* group = getGroup();
			for(Creature* crt : group->members) {
				if(inSameRoom(crt))
					crt->addEffect(playing->getEffect(), -2, -2, this)->setDuration(playing->getDuration());
			}
		}
		if(targetType.equals("room", false)) {
			ctag* cp = 0;
			Creature* crt = 0;
			cp = getRoom()->first_ply;
			while(cp) {
				crt = cp->crt;
				cp = cp->next_tag;
				if(getPlayer() && crt->getPlayer() && getPlayer()->isDueling(crt->getName()))
					continue;
				crt->addEffect(playing->getEffect(), -2, -2, this)->setDuration(playing->getDuration());
			}
		}
	} else if(playing->getType() == "script") {
		print("Running script for song \"%s\"\n", playing->getName().c_str());
		MudObject *target = NULL;

		// Find the target here, if any
		if(targetType.equals("target", false)) {
			if(hasAttackableTarget())
				target = getTarget();
		}

		playing->runScript(this, target);
	}


	setLastTime(LT_SONG_PLAYED, t, playing->getDelay());
	return(true);
}


//*********************************************************************
//						dmSongList
//*********************************************************************

int dmSongList(Player* player, cmd* cmnd) {
	const Song* song=0;

	player->printColor("^YSongs\n");
	for(std::pair<bstring, Song*> sp : gConfig->songs) {
		song = sp.second;
		player->printColor("  %s   %d - %s\n	Script: ^y%s^x\n", song->name.c_str(),
			song->priority, song->desc.c_str(), song->script.c_str());
	}

	return(0);
}

//*********************************************************************
//						cmdPlay
//*********************************************************************

int cmdPlay(Player* player, cmd* cmnd) {

	if(cmnd->num == 1) {
		if(player->isPlaying()) {
			player->stopPlaying();
			return(0);
		} else {
			player->print("What song would you like to play?\n");
			return(0);
		}
	}

	int retVal = 0;
	Song* song = gConfig->getSong(cmnd->str[1], retVal);

	if(retVal == CMD_NOT_FOUND) {
		player->print("Alas, there exists no song by that name.\n");
		return(0);
	} else if(retVal == CMD_NOT_UNIQUE ) {
		player->print("Alas, there exists many songs by that name, please be more specific!\n");
		return(0);
	}


	if(!song) {
		player->print("Error loading song \"%s\"\n", cmnd->str[1]);
		return(0);
	}


	player->print("Found song: \"%s\"\n", song->getName().c_str());

	// Is there a required instrument? Do we have it?

	// Do certain instruments give a bonus or penalty to this song? Modify it here
	if(!player->isPlaying() || player->getPlaying() != song)
		player->setPlaying(song);

	player->print("You start playing \"%s\"\n", song->getName().c_str());
	player->getRoom()->print(player->getSock(), "%M starts playing %s\n", player, song->getName().c_str());

	return(0);
}

//*********************************************************************
//						getDelay
//*********************************************************************

int Song::getDelay() {
	return(delay);
}

//*********************************************************************
//						getDuration
//*********************************************************************

int Song::getDuration() {
	return(duration);
}

