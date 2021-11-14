/*
 * SongLoader.cpp
 *   Bard Song Loader
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

#include <map>                       // for allocator, operator==, _Rb_tree_...
#include <utility>                   // for move
#include <utility>

#include "builders/songBuilder.hpp"
#include "config.hpp"                // for Config, SongSet
#include "mudObjects/players.hpp"    // for Player
#include "server.hpp"                // for gServer, PlayerMap, Server
#include "songs.hpp"                 // for Song


void addToSet(Song &&song, SongSet &songSet) {
    songSet.emplace(std::move(song));
}

bool Config::loadSongs() {
    addToSet(SongBuilder()
        .name("Banshee Wail")
        .type("effect")
        .targetType("target")
        .delay(8)
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Hailey's Hasting Hymn")
        .type("effect")
        .effect("haileyshastinghymn")
        .targetType("group")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Hymn of Revitalization")
        .type("effect")
        .effect("hymnofrevitalization")
        .targetType("group")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Warsong")
        .type("effect")
        .effect("warsong")
        .targetType("group")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Aaron's Lugubrious Lament")
        .type("effect")
        .targetType("target")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Magical Monologue")
        .type("effect")
        .targetType("self")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Charismatic Carillon")
        .type("effect")
        .targetType("target")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Song of Purification")
        .type("effect")
        .effect("songofpurification")
        .targetType("group")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Lucid Lullaby")
        .type("effect")
        .targetType("target")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Aquatic Ayre")
        .type("effect")
        .effect("aquaticayre")
        .targetType("group")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Sonorus Serenade")
        .type("effect")
        .effect("sonorusserenade")
        .targetType("self")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Chains of Dissonance")
        .type("effect")
        .effect("chainsofdissonance")
        .targetType("target")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Disenchanting Melody")
        .type("effect")
        .targetType("target")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Song of Sight")
        .type("effect")
        .effect("songofsight")
        .targetType("group")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Heartwarming Rhapsody")
        .type("effect")
        .effect("heartwarmingrapsody")
        .targetType("group")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Dreadful Screech")
        .type("effect")
        .targetType("target")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Song of Vitality")
        .type("effect")
        .effect("songofvitality")
        .targetType("group")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Avian Aria")
        .type("effect")
        .effect("avianaria")
        .targetType("group")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Chorus of Clarity")
        .type("effect")
        .effect("chorusofclarity")
        .targetType("group")
        .priority(100), songs
    );
    addToSet(SongBuilder()
        .name("Guardian Rhythms")
        .type("effect")
        .effect("guardianrhythms")
        .targetType("group")
        .priority(100), songs
    );


    return true;
}

//*********************************************************************
//                      clearSongs
//*********************************************************************

void Config::clearSongs() {
    if(gServer != nullptr) {
        for (const auto&[pId, p]: gServer->players) {
            p->stopPlaying(true);
        }
    }
    songs.clear();
}
