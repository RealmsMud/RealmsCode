/*
 * catRef.h
 *   CatRef object
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _CATREF_H
#define _CATREF_H

#include <unordered_map>
#include <string>
#include <functional>

#include "common.hpp"

class Creature;

class CatRef {
public:
    CatRef();
    void    setDefault(const Creature* target);
    void    clear();
    xmlNodePtr save(xmlNodePtr curNode, const char* childName, bool saveNonZero, int pos=0) const;
    void    load(xmlNodePtr curNode);
    CatRef& operator=(const CatRef& cr);
    bool    operator==(const CatRef& cr) const;
    bool    operator!=(const CatRef& cr) const;
    bstring rstr() const;
    bstring str(bstring current = "", char color = '\0') const;
    void    setArea(bstring c);
    bool    isArea(bstring c) const;

    bstring area;
    short   id;
};

namespace std {
    template <>
        struct hash<CatRef>{
        public :
            size_t operator()(const CatRef &cr ) const {
                return hash<string>()(cr.str());
            }
    };
};

#endif  /* _CATREF_H */

