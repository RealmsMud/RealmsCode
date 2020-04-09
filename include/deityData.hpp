/*
 * deityData.h
 *   Deity data storage file
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

#ifndef _DEITYDATA_H
#define _DEITYDATA_H

#include <map>

#include "common.hpp"

class PlayerTitle;

class DeityData {
protected:
    int id;
    bstring name;

public:
    DeityData(xmlNodePtr rootNode);

    std::map<int, PlayerTitle*> titles;

    int getId() const;
    bstring getName() const;
    bstring getTitle(int lvl, bool male, bool ignoreCustom=true) const;
};


#endif  /* _DEITYDATA_H */

