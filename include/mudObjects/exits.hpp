/*
 * exits.h
 *   Header file for exits
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

#ifndef _EXITS_H
#define _EXITS_H

#include "lasttime.hpp"
#include "location.hpp"
#include "mudObjects/mudObject.hpp"
#include "swap.hpp"

enum Direction {
    NoDirection = 0,
    North =     1,
    East =      2,
    South =     3,
    West =      4,
    Northeast = 5,
    Northwest = 6,
    Southeast = 7,
    Southwest = 8
};

Direction getDir(std::string str);
std::string getDirName(Direction dir);

#define EXIT_KEY_LENGTH 20
class Exit: public MudObject {
public:
    Exit();
    ~Exit();
    bool operator< (const MudObject& t) const;

    int readFromXml(xmlNodePtr rootNode, BaseRoom* room, bool offline=false);
    int saveToXml(xmlNodePtr parentNode) const;

    void escapeText();

    [[nodiscard]] short getLevel() const;
    [[nodiscard]] std::string getOpen() const;
    [[nodiscard]] short getTrap() const;
    [[nodiscard]] short getKey() const;
    [[nodiscard]] std::string getKeyArea() const;
    [[nodiscard]] short getToll() const;
    [[nodiscard]] std::string getPassPhrase() const;
    [[nodiscard]] short getPassLanguage() const;
    [[nodiscard]] Size getSize() const;
    [[nodiscard]] std::string getDescription() const;
    [[nodiscard]] Direction getDirection() const;
    [[nodiscard]] std::string getEnter() const;
    [[nodiscard]] BaseRoom* getRoom() const;

    void setLevel(short lvl);
    void setOpen(std::string_view o);
    void setTrap(short t);
    void setKey(short k);
    void setKeyArea(std::string_view k);
    void setToll(short t);
    void setPassPhrase(std::string_view phrase);
    void setPassLanguage(short lang);
    void setDescription(std::string_view d);
    void setSize(Size s);
    void setDirection(Direction d);
    void setEnter(std::string_view e);
    void setRoom(BaseRoom* room);

    void checkReLock(Creature* creature, bool sneaking);

    [[nodiscard]] std::string blockedByStr(char color, std::string_view spell, std::string_view effectName, bool detectMagic, bool canSee) const;
    Exit* getReturnExit(const BaseRoom* parent, BaseRoom** targetRoom) const;
    void doDispelMagic(BaseRoom* parent);  // true if the exit was destroyed by dispel-magic
    [[nodiscard]] bool isWall(std::string_view name) const;
    bool isConcealed(const Creature* viewer=nullptr) const;

    bool isDiscoverable() const;
    bool hasBeenUsedBy(std::string id) const;
    bool hasBeenUsedBy(Player* player) const;

//// Effects
    bool pulseEffects(time_t t);
    bool doEffectDamage(Creature* target);

    void addEffectReturnExit(const std::string &effect, long duration, int strength, const Creature* owner);
    void removeEffectReturnExit(const std::string &effect, BaseRoom* rParent);
protected:
    short   level;
    std::string open;           // output on open
    short   trap;           // Exit trap
    short   key;            // more keys when short
    std::string keyArea;
    short   toll;           // exit toll cost
    std::string passphrase;
    short   passlang;
    std::string description;
    Size    size;
    std::string enter;
    BaseRoom* parentRoom;   // Pointer to the room this exit is in
    Direction direction;

public:
    // almost ready to be made protected - just need to get
    // loading of flags done
    char        flags[16]{};      // Max exit flags 128 now

    struct      lasttime ltime; // Timed open/close

    char        desc_key[3][EXIT_KEY_LENGTH]{}; // Exit keys

    char        clanFlags[4]{};   // clan allowed flags
    char        classFlags[4]{};  // class allowed flags
    char        raceFlags[4]{};   // race allowed flags

    std::list<std::string> usedBy; // ids of players that have used this exit

    Location target;

    [[nodiscard]] bool flagIsSet(int flag) const;
    void setFlag(int flag);
    void clearFlag(int flag);
    bool toggleFlag(int flag);

    bool raceRestrict(const Creature* creature) const;
    bool classRestrict(const Creature* creature) const;
    bool clanRestrict(const Creature* creature) const;
    bool alignRestrict(const Creature* creature) const;
};


#endif  /* _EXITS_H */

