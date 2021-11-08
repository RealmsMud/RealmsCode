/*
 * bans.h
 *   Header file for bans
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
#ifndef BANS_H_
#define BANS_H_

#include <libxml/parser.h>  // for xmlNodePtr


class Ban {
public:
    Ban();
    explicit Ban(xmlNodePtr curNode);
    void reset();
    bool matches(std::string_view toMatch);
    
public: // for now
    std::string     site;
    int         duration{};
    long        unbanTime{};
    std::string     by;
    std::string     time;
    std::string     reason;
    std::string     password;
    bool        isPrefix{};
    bool        isSuffix{};
};


#endif /*BANS_H_*/
