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

#include <boost/algorithm/string/predicate.hpp>
#include "cmd.hpp"                                  // for cmd, CMD_NOT_FOUND
#include "commands.hpp"                             // for getFullstrText
#include "config.hpp"                               // for Config, SongMap
#include "container.hpp"                            // for PlayerSet
#include "creatureStreams.hpp"                      // for Streamable
#include "creatures.hpp"                            // for Creature, Player
#include "mud.hpp"                                  // for LT_SONG_PLAYED
#include "pythonHandler.hpp"                        // for addMudObjectToDic...
#include "rooms.hpp"                                // for BaseRoom
#include "server.hpp"                               // for Server, gServer
#include "songs.hpp"                                // for Song

class MudObject;

//*********************************************************************
//                      getEffect
//*********************************************************************

const std::string& Song::getEffect() const {
    return(effect);
}

//*********************************************************************
//                      getType
//*********************************************************************

const std::string& Song::getType() const {
    return(type);
}

//*********************************************************************
//                      getTargetType
//*********************************************************************

const std::string& Song::getTargetType() const {
    return(targetType);
}

//*********************************************************************
//                      runScript
//*********************************************************************

bool Song::runScript(MudObject* singer, MudObject* target) const {

    if(script.empty())
        return(false);

    try {
        auto locals = py::dict();
        auto songModule = py::module::import("songLib");
        locals["songLib"] = songModule;
        locals["song"] = this;

        PythonHandler::addMudObjectToDictionary(locals, "actor", singer);
        PythonHandler::addMudObjectToDictionary(locals, "target", target);

        return (gServer->runPythonWithReturn(script, locals));
    }
    catch( pybind11::error_already_set& e) {
        PythonHandler::handlePythonError(e);
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

    const auto& targetType = playing->getTargetType();

    if(boost::iequals(playing->getType(), "effect")) {
        // Group effects affect the singer as well
        if(boost::iequals(targetType, "self") || boost::iequals(targetType, "group")) {
            addEffect(playing->getEffect(), -2, -2, this)->setDuration(playing->getDuration());
        }
        if(boost::iequals(targetType, "group") && (getGroup() != nullptr)) {
            Group* myGroup = getGroup();
            for(Creature* crt : myGroup->members) {
                if(inSameRoom(crt))
                    crt->addEffect(playing->getEffect(), -2, -2, this)->setDuration(playing->getDuration());
            }
        }
        if(boost::iequals(targetType, "room")) {
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
        if(boost::iequals(targetType, "target")) {
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
    std::string songStr = getFullstrText(cmnd->fullstr, 1);
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

