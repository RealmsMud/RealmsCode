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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _OBJINCREASE_H
#define _OBJINCREASE_H


enum IncreaseType {
    UnknownIncrease =     0,
    SkillIncrease =     1,
    LanguageIncrease =   2
};

class ObjIncrease {
public:
    ObjIncrease();
    void reset();
    bool isValid() const;
    ObjIncrease& operator=(const ObjIncrease& o);
    void    save(xmlNodePtr curNode) const;
    void    load(xmlNodePtr curNode);

    IncreaseType type;
    bstring increase;
    unsigned int amount;
    bool onlyOnce;
    bool canAddIfNotKnown;
};


#endif  /* _OBJINCREASE_H */

