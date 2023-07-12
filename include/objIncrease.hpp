/*
 * objIncrease.h
 *   Object increase
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
#include "json.hpp"

enum IncreaseType {
    UnknownIncrease =     0,
    SkillIncrease =     1,
    LanguageIncrease =   2
};

class ObjIncrease {
public:
    IncreaseType type;
    std::string increase;
    int amount{};
    bool onlyOnce{};
    bool canAddIfNotKnown{};


public:
    ObjIncrease();
    void reset();
    bool isValid() const;
    ObjIncrease& operator=(const ObjIncrease& o);
    void    save(xmlNodePtr curNode) const;
    void    load(xmlNodePtr curNode);

public:
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ObjIncrease, type, increase, amount, onlyOnce, canAddIfNotKnown);
};


