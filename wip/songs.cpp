/*
 * Songs.cpp
 *   Additional song-casting routines.
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
#include <boost/python/errors.hpp>                  // for error_already_set
#include <boost/python/extract.hpp>                 // for extract
#include <boost/python/handle.hpp>                  // for handle
#include <boost/python/object_core.hpp>             // for object, object_op...

#include "bstring.hpp"                              // for bstring
#include "cmd.hpp"                                  // for cmd, CMD_NOT_FOUND
#include "commands.hpp"                             // for getFullstrText
#include "config.hpp"                               // for Config, SongMap
#include "container.hpp"                            // for PlayerSet
#include "creatureStreams.hpp"                      // for Streamable
#include "creatures.hpp"                            // for Creature, Player
#include "dictobject.h"                             // for PyDict_New
#include "import.h"                                 // for PyImport_ImportMo...
#include "mud.hpp"                                  // for LT_SONG_PLAYED
#include "pythonHandler.hpp"                        // for addMudObjectToDic...
#include "rooms.hpp"                                // for BaseRoom
#include "server.hpp"                               // for Server, gServer
#include "songs.hpp"                                // for Song

class MudObject;

//*********************************************************************
//                      getEffect
//*********************************************************************

const bstring& Song::getEffect() const {
    return(effect);
}

//*********************************************************************
//                      getType
//*********************************************************************

const bstring& Song::getType() const {
    return(type);
}

//*********************************************************************
//                      getTargetType
//*********************************************************************

const bstring& Song::getTargetType() const {
    return(targetType);
}

//*********************************************************************
//                      runScript
//*********************************************************************

bool Song::runScript(MudObject* singer, MudObject* target) const {

    if(script.empty())
        return(false);

    try {
        bp::object localNamespace( (bp::handle<>(PyDict_New())));

        bp::object effectModule( (bp::handle<>(PyImport_ImportModule("songLib"))) );
        localNamespace["songLib"] = effectModule;

        localNamespace["song"] = bp::ptr(this);

        // Default retVal is true
        localNamespace["retVal"] = true;
        addMudObjectToDictionary(localNamespace, "actor", singer);
        addMudObjectToDictionary(localNamespace, "target", target);

        gServer->runPython(script, localNamespace);

        bool retVal = bp::extract<bool>(localNamespace["retVal"]);
        return(retVal);
    }
    catch( bp::error_already_set &e) {
        gServer->handlePythonError();
    }

    return(false);
}

//*********************************************************************
//                      clearSongs
//*********************************************************************

void Config::clearSongs() {
    for(const auto& [pId, p] : gServer->players) {
        p->stopPlaying(true);
    }

    songs.clear();
}

//*********************************************************************
//                      isPlaying
//*********************************************************************

bool Creature::isPlaying() {
    return(playing != nullptr);
}

//*********************************************************************
//                      getPlaying
//*********************************************************************

const Song* Creature::getPlaying() {
    return(playing);
}

//*********************************************************************
//                      setPlaying
//*********************************************************************

bool Creature::setPlaying(const Song* newSong, bool echo) {
    bool wasPlaying = false;
    if(isPlaying()) {
        stopPlaying(echo);
        wasPlaying = true;
    }

    playing = newSong;

    return(wasPlaying);
}

//*********************************************************************
//                      stopPlaying
//*********************************************************************

bool Creature::stopPlaying(bool echo) {
    if(!isPlaying())
        return(false);

    if(echo) {
        print("You stop playing \"%s\"\n", getPlaying()->getName().c_str());
        getRoomParent()->print(getSock(), "%M stops playing %s\n", this, getPlaying()->getName().c_str());
    }
    playing = nullptr;
    return(true);
}

//*********************************************************************
//                      pulseSong
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
        if(targetType.equals("group", false) && (getGroup() != nullptr)) {
            Group* group = getGroup();
            for(Creature* crt : group->members) {
                if(inSameRoom(crt))
                    crt->addEffect(playing->getEffect(), -2, -2, this)->setDuration(playing->getDuration());
            }
        }
        if(targetType.equals("room", false)) {
            for(Player* ply : getRoomParent()->players) {
                if(getAsPlayer() && ply->getAsPlayer() && getAsPlayer()->isDueling(ply->getName()))
                    continue;
                ply->addEffect(playing->getEffect(), -2, -2, this)->setDuration(playing->getDuration());
            }
        }
    } else if(playing->getType() == "script") {
        print("Running script for song \"%s\"\n", playing->getName().c_str());
        MudObject *target = nullptr;

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
//                      dmSongList
//*********************************************************************

int dmSongList(Player* player, cmd* cmnd) {
    player->printColor("^YSongs\n");
    for(const auto& song : gConfig->songs) {
        player->printColor("  %s   %d - %s\n    Script: ^y%s^x\n", song.name.c_str(),
            song.priority, song.description.c_str(), song.script.c_str());
    }

    return(0);
}

//*********************************************************************
//                      cmdPlay
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
    bstring songStr = getFullstrText(cmnd->fullstr, 1);
    auto* song = gConfig->getSong(songStr, retVal);

    if(retVal == CMD_NOT_FOUND) {
        *player << "Alas, there exists no song by that name (" << songStr << ")\n";
        return(0);
    } else if(retVal == CMD_NOT_UNIQUE ) {
        *player << "Alas, there exists many songs by that name, please be more specific!\n";
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
    player->getRoomParent()->print(player->getSock(), "%M starts playing %s\n", player, song->getName().c_str());

    return(0);
}

//*********************************************************************
//                      getDelay
//*********************************************************************

int Song::getDelay() const {
    return(delay);
}

//*********************************************************************
//                      getDuration
//*********************************************************************

int Song::getDuration() const {
    return(duration);
}

