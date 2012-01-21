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
    // Stream operators
	Streamable& operator<< (MudObject* obj);
	Streamable& operator<< (MudObject& obj);
	Streamable& operator<< (const bstring& str);

    void setManipFlags(int flags);
    int getManipFlags();

    void setManipNum(int num);
    int getManipNum();
    //Creature& operator<< (creatureManip& manip)

protected:
    int manipFlags;
    int manipNum;


    void doPrint(const bstring& toPrint);
};

class CrtManip
{
public:
	friend Streamable& operator << (Streamable&, const CrtManip&);
private:
	int _value;
};

CrtManip setf(int n);


#endif
