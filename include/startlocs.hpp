/*
 * start.h
 *   Player starting location code.
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

#ifndef _START_H
#define _START_H

class Location;
class CatRef;
class bstring;

class StartLoc {
public:
    StartLoc();
    void    load(xmlNodePtr curNode);
    void    save(xmlNodePtr curNode) const;

    [[nodiscard]] bstring getName() const;
    [[nodiscard]] bstring getBindName() const;
    [[nodiscard]] bstring getRequiredName() const;
    [[nodiscard]] Location getBind() const;
    [[nodiscard]] Location getRequired() const;
    [[nodiscard]] CatRef getStartingGuide() const;
    bool    swap(Swap s);
    [[nodiscard]] bool    isDefault() const;
    void    setDefault();

protected:
    bstring name;
    bstring bindName;
    bstring requiredName;
    Location bind;
    Location required;
    CatRef startingGuide;
    bool    primary;
};


#endif  /* _START_H */

