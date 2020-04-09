/*
 * wanderInfo.h
 *   Random monster wandering info
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

#ifndef _WANDERINFO_H
#define _WANDERINFO_H

#include <map>

#include "common.hpp"

class CatRef;
class Player;

class WanderInfo {
public:
    WanderInfo();
    CatRef  getRandom() const;
    void    load(xmlNodePtr curNode);
    void    save(xmlNodePtr curNode) const;
    void    show(const Player* player, const bstring& area="") const;

    std::map<int, CatRef> random;

    short getTraffic() const;
    void setTraffic(short t);
    unsigned long getRandomCount() const;
protected:
    short   traffic;
};

#endif  /* _WANDERINFO_H */

