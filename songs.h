/*
 * Songs.h
 *	 Additional song-casting routines.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _SONGS_H
#define	_SONGS_H


// ******************
//   Song
// ******************

class Song: public MysticMethod {
public:
    Song(xmlNodePtr rootNode);
    ~Song() {};

    void save(xmlNodePtr rootNode) const;

    const bstring& getEffect() const;
    const bstring& getType() const;
    const bstring& getTargetType() const;
    bool runScript(MudObject* singer, MudObject* target = NULL);

    int getDelay();
    int getDuration();
private:
    Song() {};
    bstring effect;
    bstring type; // script, effect, etc
    bstring targetType; // Valid Targets: Room, Self, Group, Target, RoomBene, RoomAggro

    int delay;
    int duration;

};

#endif	/* _SONGS_H */

