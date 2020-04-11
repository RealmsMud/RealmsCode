/*
 * playerTitle.h
 *   Player titles
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

#ifndef _PLAYERTITLE_H
#define _PLAYERTITLE_H

#include <libxml/parser.h>  // for xmlNodePtr

class PlayerTitle {
public:
    PlayerTitle();
    void    load(xmlNodePtr rootNode);
    bstring getTitle(bool sexMale) const;
protected:
    bstring     male;
    bstring     female;
};


#endif  /* _PLAYERTITLE_H */

