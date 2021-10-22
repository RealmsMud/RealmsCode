/*
 * carry.h
 *   Carried objects file
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

#ifndef _CARRY_H
#define _CARRY_H

#include <libxml/parser.h>  // for xmlNodePtr

#include "catRef.hpp"       // for CatRef

class Carry {
public:
    Carry();
    xmlNodePtr save(xmlNodePtr curNode, const char* childName, bool saveNonZero, int pos=0) const;
    void    load(xmlNodePtr curNode);
    [[nodiscard]] std::string str(std::string current = "", char color = '\0') const;
    Carry& operator=(const Carry& cry);
    bool    operator==(const Carry& cry) const;
    bool    operator!=(const Carry& cry) const;

    CatRef info;
    int numTrade;
};


#endif  /* _CARRY_H */

