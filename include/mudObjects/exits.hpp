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

#include <boost/dynamic_bitset.hpp>
#include "lasttime.hpp"
#include "location.hpp"
#include "mudObjects/mudObject.hpp"
#include "swap.hpp"

enum Direction {
    NoDirection = 0,
    North = 1,
    East = 2,
    South = 3,
    West = 4,
    Northeast = 5,
    Northwest = 6,
    Southeast = 7,
    Southwest = 8,
    Up = 9,
    Down = 10
};

Direction getDir(std::string str);
std::string getDirName(Direction dir);

#define EXIT_KEY_LENGTH 20

class Exit : public virtual MudObject, public inheritable_enable_shared_from_this<Exit> {
public:
    Exit();
    ~Exit();
    bool operator<(const MudObject &t) const;

    int readFromXml(xmlNodePtr rootNode, bool offline, const std::string &version);
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
    [[nodiscard]] std::shared_ptr<BaseRoom> getRoom() const;

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
    void setRoom(const std::shared_ptr<BaseRoom>& room);

    void checkReLock(const std::shared_ptr<Creature> &creature, bool sneaking);

    [[nodiscard]] std::string blockedByStr(char color, std::string_view spell, std::string_view effectName, bool detectMagic, bool canSee) const;
    std::shared_ptr<Exit> getReturnExit(const std::shared_ptr<const BaseRoom> &parent, std::shared_ptr<BaseRoom> &targetRoom) const;
    void doDispelMagic(const std::shared_ptr<BaseRoom> &parent);  // true if the exit was destroyed by dispel-magic
    [[nodiscard]] bool isWall(std::string_view name) const;
    bool isConcealed(const std::shared_ptr<const Creature> &viewer = nullptr) const;

    bool isDiscoverable() const;
    bool hasBeenUsedBy(const std::string &id) const;
    bool hasBeenUsedBy(const std::shared_ptr<const Creature> &creature) const;

//// Effects
    bool pulseEffects(time_t t);
    bool doEffectDamage(const std::shared_ptr<Creature> &pTarget);

    void addEffectReturnExit(const std::string &effect, long duration, int strength, const std::shared_ptr<Creature> &owner);
    void removeEffectReturnExit(const std::string &effect, const std::shared_ptr<BaseRoom> &rParent);
protected:
    short level;
    std::string open;           // output on open
    short trap;           // Exit trap
    short key;            // more keys when short
    std::string keyArea;
    short toll;           // exit toll cost
    std::string passphrase;
    short passlang;
    std::string description;
    Size size;
    std::string enter;
    std::weak_ptr<BaseRoom> parentRoom;   // Pointer to the room this exit is in
    Direction direction;

public:
    // almost ready to be made protected - just need to get
    // loading of flags done
    boost::dynamic_bitset<> flags{128};

    LastTime ltime; // Timed open/close

    char desc_key[3][EXIT_KEY_LENGTH]{}; // Exit keys

    boost::dynamic_bitset<> clanFlags{32};  // clan allowed flags
    boost::dynamic_bitset<> classFlags{32}; // class allowed flags
    boost::dynamic_bitset<> raceFlags{32};  // race allowed flags

    std::set<std::string> usedBy; // ids of players that have used this exit

    Location target;

    [[nodiscard]] bool flagIsSet(int flag) const;
    void setFlag(int flag);
    void clearFlag(int flag);
    bool toggleFlag(int flag);

    bool raceRestrict(const std::shared_ptr<const Creature> &creature) const;
    bool classRestrict(const std::shared_ptr<const Creature> &creature) const;
    bool clanRestrict(const std::shared_ptr<const Creature> &creature) const;
    bool alignRestrict(const std::shared_ptr<const Creature> &creature) const;
    const char *hisHer() const;
    const char *himHer() const;
    const char *heShe() const;
    const char *upHisHer() const;
    const char *upHeShe() const; 
};


#endif  /* _EXITS_H */

