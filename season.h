/*
 * season.h.h
 *   Season
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef REALMSCODE_SEASON_H
#define REALMSCODE_SEASON_H

// make sure these stay updated with calendar.xml!
enum Season {
    NO_SEASON = 0,
    SPRING =    1,
    SUMMER =    2,
    AUTUMN =    3,
    WINTER =    4
};

// for bit flags, take (season-1)^2
//   spring flag (1) = overflowing river
//   summer flag (2) = ?
//   autumn flag (4) = ?
//   winter flag (8) = cold damage, no herbs



#endif //REALMSCODE_SEASON_H
