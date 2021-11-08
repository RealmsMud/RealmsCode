/*
 * songBuilder.h
 *   Fluent API style builder for songs
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
#include "songs.hpp"

#include "fluentBuilder.hpp"
#define BUILDER(name, type) GENERIC_BUILDER(SongBuilder, song, name, type)

class SongBuilder {
public:
    STRING_BUILDER(name);
    STRING_BUILDER(description);
    STRING_BUILDER(effect);
    STRING_BUILDER(script);
    STRING_BUILDER(type);
    STRING_BUILDER(targetType);
    INT_BUILDER(priority);
    INT_BUILDER(delay);

    // NOLINTNEXTLINE - We want implicit conversion
    operator Song&&() {
        if(song.name.empty()) {
            throw std::runtime_error("Song with empty name");
        }
        if(!Song::validTargetTypes.contains(song.targetType)) {
            throw std::runtime_error(fmt::format("Invalid TargetType for {} - '{}'", song.getName(), song.targetType));
        }
        if(!Song::validTypes.contains(song.type)) {
            throw std::runtime_error(fmt::format("Invalid Type for {} - '{}'", song.getName(), song.type));
        }
        return std::move(song);
    }

private:
    Song song;
};

#include "fluentBuilderEnd.hpp"
