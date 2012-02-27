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
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */


#ifndef CREATURE_STREAMS_H
#define CREATURE_STREAMS_H

class Streamable {
public:
	virtual ~Streamable() {};
	void initStreamable();
    // Stream operators
	Streamable& operator<< (const MudObject* obj);
	Streamable& operator<< (const MudObject& obj);
	Streamable& operator<< (const bstring& str);
	Streamable& operator<< (const int num);
	Streamable& operator<< (Stat& stat);

	// This is to allow simple function based manipulators (like ColorOn, ColorOff)
	Streamable& operator <<( Streamable& (*op)(Streamable&));

    void setManipFlags(int flags);
    void setManipNum(int num);
    void setColorOn();
    void setColorOff();

    int getManipFlags();
    int getManipNum();
    //Creature& operator<< (creatureManip& manip)

protected:
    int manipFlags;
    int manipNum;
    bool streamColor;
    bool petPrinted;

    void doPrint(const bstring& toPrint);
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
