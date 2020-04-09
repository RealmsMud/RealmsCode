/*
 * realm.h
 *   Realms
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

#ifndef REALMSCODE_REALM_H
#define REALMSCODE_REALM_H

enum Realm {
    NO_REALM =  0,
    MIN_REALM = 1,
    EARTH =     1,
    WIND =      2,
    FIRE =      3,
    WATER =     4,
    ELEC =      5,
    COLD =      6,

    MAX_REALM
};


#endif //REALMSCODE_REALM_H
