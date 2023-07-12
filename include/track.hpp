/*
 * track.h
 *   Track file
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

#include <libxml/parser.h>  // for xmlNodePtr

#include "size.hpp"

class Track {
public:
    Track();
    void    load(xmlNodePtr curNode);
    void    save(xmlNodePtr curNode) const;

    short getNum() const;
    Size getSize() const;
    std::string getDirection() const;

    void setNum(short n);
    void setSize(Size s);
    void setDirection(std::string_view dir);
protected:
    short   num;
    Size    size;
    std::string direction;
};

