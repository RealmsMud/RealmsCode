/*
 * asynch.h
 *   Asynchronous communication
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

#ifndef _ASYNCH_H
#define _ASYNCH_H

#include "proc.h"

class Player;

enum AsyncResult {
    AsyncExternal,
    AsyncLocal
};


class Async {
protected:
    int fds[2];
public:
    Async();
    AsyncResult branch(const Player* player, childType type);
};


#endif  /* _ASYNCH_H */

