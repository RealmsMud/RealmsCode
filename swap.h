/*
 * swap.h
 *   Code to swap rooms/monsters/objects between areas
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _SWAP_H
#define _SWAP_H

#include "bstring.h"
#include "catRef.h"

enum SwapType {
    SwapNone,
    SwapRoom,
    SwapObject,
    SwapMonster
};

class Swap {
public:
    Swap();

    void set(bstring mover, CatRef swapOrigin, CatRef swapTarget, SwapType swapType);
    bool match(CatRef o, CatRef t);

    bstring player;
    SwapType type;

    CatRef origin;
    CatRef target;
};


#endif  /* _SWAP_H */

