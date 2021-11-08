/*
 * Songs.cpp
 *   Bard Songs
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

#include <toNum.hpp>
#include "cmd.hpp"                                  // for cmd, CMD_NOT_FOUND
#include "commands.hpp"                             // for getFullstrText
#include "config.hpp"                               // for Config, SongMap
#include "container.hpp"                            // for PlayerSet
#include "creatures.hpp"                            // for Creature, Player
#include "pythonHandler.hpp"                        // for addMudObjectToDic...
#include "rooms.hpp"                                // for BaseRoom
#include "server.hpp"                               // for Server, gServer
#include "songs.hpp"                                // for Song


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

int dmSongList(Player* player, cmd* cmnd) {
    std::string command = getFullstrText(cmnd->fullstr, 1);

    bool all = (command == "all");
    int id = toNum<int>(command);

    player->printPaged("^YSongs^x\n");
    int i = 0;
    for(const auto& song : gConfig->songs) {
        i++;
        player->printPaged(fmt::format("{})\tName: ^W{:<30}^x  Priority: {:<3}  Type: {}", i, song.getName(), song.getPriority(), song.getType()));

        if(!all && i != id) continue;
        if(!song.getDescription().empty())
            player->printPaged(fmt::format("\tDescription: {}", song.getDescription()));
        if(!song.getScript().empty())
            player->printPaged(fmt::format("\tScript: {}", song.getScript()));
    }
    player->donePaging();
    return(0);
}

int Song::getDelay() const {
    return(delay);
}

int Song::getDuration() const {
    return(duration);
}

int Song::getPriority() const {
    return(priority);
}

const std::string& Song::getEffect() const {
    return(effect);
}

const std::string& Song::getType() const {
    return(type);
}

const std::string& Song::getTargetType() const {
    return(targetType);
}

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