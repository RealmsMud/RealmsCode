/*
 * exits.h
 *	 Header file for exits
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
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _EXITS_H
#define	_EXITS_H

enum Direction {
	NoDirection = 0,
	North =		1,
	East =		2,
	South =		3,
	West =		4,
	Northeast =	5,
	Northwest =	6,
	Southeast =	7,
	Southwest =	8
};

Direction getDir(bstring str);
bstring getDirName(Direction dir);

#define EXIT_KEY_LENGTH 20
class Exit: public MudObject {
public:
	Exit();
	~Exit();
	bool operator< (const MudObject& t) const;

	int readFromXml(xmlNodePtr rootNode, BaseRoom* room);
	int saveToXml(xmlNodePtr parentNode) const;

	void escapeText();

	short getLevel() const;
	bstring getOpen() const;
	short getTrap() const;
	short getKey() const;
	bstring getKeyArea() const;
	short getToll() const;
	bstring getPassPhrase() const;
	short getPassLanguage() const;
	Size getSize() const;
	bstring getDescription() const;
	Direction getDirection() const;
	bstring getEnter() const;
	BaseRoom* getRoom() const;

	void setLevel(short lvl);
	void setOpen(bstring o);
	void setTrap(short t);
	void setKey(short k);
	void setKeyArea(bstring k);
	void setToll(short t);
	void setPassPhrase(bstring phrase);
	void setPassLanguage(short lang);
	void setDescription(bstring d);
	void setSize(Size s);
	void setDirection(Direction d);
	void setEnter(bstring e);
	void setRoom(BaseRoom* room);

	void checkReLock(Creature* creature, bool sneaking);

	bstring blockedByStr(char color, bstring spell, bstring effectName, bool detectMagic, bool canSee) const;
	Exit* getReturnExit(const BaseRoom* parent, BaseRoom** targetRoom) const;
	void doDispelMagic(BaseRoom* parent);  // true if the exit was destroyed by dispel-magic
	bool isWall(bstring name) const;
	bool isConcealed(const Creature* viewer=0) const;

//// Effects
	bool pulseEffects(time_t t);
	bool doEffectDamage(Creature* target);

	void addEffectReturnExit(bstring effect, long duration, int strength, const Creature* owner);
	void removeEffectReturnExit(bstring effect, BaseRoom* rParent);
protected:
	short	level;
	bstring open;			// output on open
	short	trap; 			// Exit trap
	short	key;			// more keys when short
	bstring keyArea;
	short	toll;			// exit toll cost
	bstring passphrase;
	short	passlang;
	bstring description;
	Size	size;
	bstring enter;
	BaseRoom* parentRoom;	// Pointer to the room this exit is in
	Direction direction;

public:
	// almost ready to be made protected - just need to get
	// loading of flags done
	char		flags[16]; 		// Max exit flags 128 now

	struct		lasttime ltime;	// Timed open/close

	char		desc_key[3][EXIT_KEY_LENGTH]; // Exit keys

	char		clanFlags[4]; 	// clan allowed flags
	char		classFlags[4];	// class allowed flags
	char		raceFlags[4]; 	// race allowed flags

	Location target;

	bool flagIsSet(int flag) const;
	void setFlag(int flag);
	void clearFlag(int flag);
	bool toggleFlag(int flag);

	bool raceRestrict(const Creature* creature) const;
	bool classRestrict(const Creature* creature) const;
	bool clanRestrict(const Creature* creature) const;
	bool alignRestrict(const Creature* creature) const;
};


#endif	/* _EXITS_H */

