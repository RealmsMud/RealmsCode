/*
 * creatureStreams.h
 *   Handles streaming to creatures
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


#ifndef CREATURE_STREAMS_H
#define CREATURE_STREAMS_H

#include <map>

#include "stats.hpp"

class MudObject;
class Socket;
class CatRef;

class Streamable {
public:
    virtual ~Streamable() = default;
    void initStreamable();
    // Stream operators
    Streamable& operator<< (const std::shared_ptr<MudObject> obj);
    Streamable& operator<< (const MudObject* obj);
    Streamable& operator<< (const MudObject& obj);
    Streamable& operator<< (std::string_view str);
    Streamable& operator<< (int num);
    Streamable& operator<< (Stat& stat);
    Streamable& operator<< (const CatRef& cr);

    // This is to allow simple function based manipulators (like ColorOn, ColorOff)
    Streamable& operator <<( Streamable& (*op)(Streamable&));

    void setManipFlags(int flags);
    void setManipNum(int num);
    void setColorOn();
    void setColorOff();
    void setPagerOn();
    void setPagerOff();

    int getManipFlags();
    int getManipNum();
    //Creature& operator<< (creatureManip& manip)

protected:
    int manipFlags{};
    int manipNum{};
    bool streamColor{};
    bool pager{};
    bool petPrinted{};

    void doPrint(std::string_view toPrint);
    std::shared_ptr<Socket> getMySock();

};

inline Streamable& ColorOn(Streamable& out) {
    out.setColorOn();
    return out;
}

inline Streamable& ColorOff(Streamable& out) {
    out << "^x";
    out.setColorOff();
    return out;
}

inline Streamable& PagerOn(Streamable& out) {
    out.setPagerOn();
    return out;
}

inline Streamable& PagerOff(Streamable& out) {
    out.setPagerOff();
    return out;
}

class setf {
public:
    setf(int flags) : value(flags) {}
    Streamable& operator()(Streamable &) const;
    int value;
};

class setn {
public:
    setn(int flags) : value(flags) {}
    Streamable& operator()(Streamable &) const;
    int value;
};

Streamable& operator<<(Streamable& out, setf flags);
Streamable& operator<<(Streamable& out, setn num);


#endif
