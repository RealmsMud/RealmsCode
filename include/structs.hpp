/*
 * structs.h
 *   Main data structure and type definitions
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
#pragma once

#include <functional>
#include <list>

#include "anchor.hpp"
#include "delayedAction.hpp"
#include "dice.hpp"
#include "global.hpp"
#include "money.hpp"
#include "namable.hpp"
#include "realm.hpp"


class Object;
class Creature;
class Player;
class Monster;
class Exit;
class BaseRoom;
class UniqueRoom;
class Faction;
class Socket;
class MapMarker;
class EffectInfo;
class AlchemyEffect;
class MudObject;


enum AlcoholState {
    ALCOHOL_SOBER = 0,
    ALCOHOL_TIPSY = 1,
    ALCOHOL_DRUNK = 2,
    ALCOHOL_INEBRIATED = 3
};

enum Sex {
    SEX_NONE = 0,
    SEX_FEMALE = 1,
    SEX_MALE = 2
};


enum EffectParentType {
    EFFECT_NO_PARENT,
    EFFECT_CREATURE,
    EFFECT_ROOM,
    EFFECT_EXIT
};



class MudFlag {
public:
    MudFlag();

    int id;
    std::string name;
    std::string desc;
};


// TODO: Rework first_charm and remove this
// Enemy effectList tags
typedef struct enm_tag {
public:
    enm_tag() { next_tag = nullptr; enemy[0] = 0; damage = 0; owner[0] = 0; };
    struct enm_tag  *next_tag;
    char            enemy[80]{};
    int             damage;
    char            owner[80]{};
} etag;

// for changestats
typedef struct StatsContainer {
    int st;
    int de;
    int co;
    int in;
    int pi;
} StatsContainer;

// Talk effectList tags
typedef struct tlk_tag {
public:
    tlk_tag() { next_tag = nullptr; key = nullptr; response = nullptr; type = 0; action = nullptr; target = nullptr; on_cmd = 0; if_cmd = 0;
            test_for = 0; do_act = 0; success = 0; if_goto_cmd = 0; not_goto_cmd = 0; goto_cmd = 0; arg1 = arg2 = 0; };
    struct tlk_tag *next_tag;
    char    *key;
    char    *response;
    char    type;
    char    *action;
    char    *target;
    char    on_cmd;     // action functions
    char    if_cmd;     // if # command succesful
    char    test_for;   // test for condition in Room
    char    do_act;     // do action
    char    success;    // command was succesful
    char    if_goto_cmd;    // used to jump to new cmd point
    char    not_goto_cmd;   // jump to cmd if not success
    char    goto_cmd;   // unconditional goto cmd
    int arg1;
    int arg2;
} ttag;


typedef struct tic {
public:
    tic() {amountCur = amountMax = amountTmp = 0; interval = intervalMax = intervalTmp = 0; };
    char    amountCur;
    char    amountMax;
    char    amountTmp;

    long    interval;
    long    intervalMax;
    long    intervalTmp;
} tic;


typedef struct spellTimer {
public:
    spellTimer() { interval = ltime = misc = misc2 = castLevel = 0; };
    long        interval;
    long        ltime;
    short       misc;
    short       misc2;
    char        castLevel;
} spellTimer;


// Daily-use operation struct
typedef struct daily {
public:
    daily() { max = cur = ltime = 0; };
    short       max;
    short       cur;
    long        ltime;
} daily;



typedef struct lockout {
public:
    lockout() { address[0] = password[0] = userid[0] = 0; };
    char        address[80]{};
    char        password[20]{};
    char        userid[9]{};
} lockout;


typedef struct osp_t {
    int     splno{};
    Realm   realm;
    int     mp{};
    Dice    damage;
    char    bonus_type{};
    bool    drain{};
} osp_t;


typedef struct osong_t {
    int     songno{};
    Dice    damage;
} osong_t;


typedef struct saves {
public:
    saves() { chance = gained = misc = 0; };
    short   chance; // Percent chance to save.
    short   gained;
    short   misc;
} saves;



// These are special defines to reuse creature structure while still
// making the code readable.


typedef struct tagPlayer {
    std::shared_ptr<Player> ply;
    Socket* sock;
//  iobuf   *io;
//  extra   *extr;
} plystruct;


typedef struct {
    short    poison;    // Poison, disease, noxious clouds, etc...
    short    death;     // Deadly traps, massive damage, energy drain attacks, massive tramples, disintegrate spells, fatally wounds from touch, etc..
    short    breath;    // Exploding monsters, firey dragon breath, exploding traps...
    short    mental;    // Charm spells, hold-person spells, confusion spells, teleport traps, mp drain traps...
    short    spells;    // Offensive spell damage, stun spells, dispel-magic traps/spells, other adverse spell effects..
} saves_struct;


typedef struct {
    short   hp;
    short   mp;
    short   armor;
    short   thaco;
    short   ndice;
    short   sdice;
    short   pdice;
    short   str;
    short   dex;
    short   con;
    short   intel;
    short   pie;
    long    realms;
} creatureStats;


// ******************
//   MudMethod
// ******************
// Base class for Command, Spell, Song, etc
class MudMethod : public virtual Nameable {
public:
    ~MudMethod() override = default;
    int priority{};

    [[nodiscard]] bool exactMatch(const std::string& toMatch) const;
    bool partialMatch(const std::string& toMatch) const;
};

// ******************
//   MysticMethod
// ******************
// Base class for songs & spells
class MysticMethod: public MudMethod {
public:
    ~MysticMethod() override = default;

    [[nodiscard]] std::string_view getScript() const;

public:
    std::string script;
    
protected:
    std::list<std::string> nameParts;
    void parseName();
};

// ******************
//   Spell
// ******************

class Spell: public MysticMethod {
public:
    explicit Spell(xmlNodePtr rootNode);
    Spell(std::string_view pCmdStr) {
        name = pCmdStr;
    }
    ~Spell() override = default;

    void save(xmlNodePtr rootNode) const;
};

#include "songs.hpp"

// Base class for Ply/Crt commands
class Command: public virtual MudMethod {
public:
    ~Command() = default;
    bool    (*auth)(const std::shared_ptr<Creature> &){};

    virtual int execute(const std::shared_ptr<Creature>& player, cmd* cmnd) const = 0;
};

// these are supplemental to the cmd class
class CrtCommand: public Command {
public:
    CrtCommand(std::string_view pCmdStr, int pPriority, int (*pFn)(const std::shared_ptr<Creature>& player, cmd* cmnd), bool (*pAuth)(const std::shared_ptr<Creature> &), std::string_view pDesc): fn(pFn)
    {
        name = pCmdStr;
        priority = pPriority;
        auth = pAuth;
        description = pDesc;
    };
    CrtCommand(std::string_view pCmdStr) {
        name = pCmdStr;
    }
    ~CrtCommand() = default;
    int (*fn)(const std::shared_ptr<Creature>& player, cmd* cmnd);
    int execute(const std::shared_ptr<Creature>& player, cmd* cmnd) const;
};

class PlyCommand: public Command {
public:
    PlyCommand(std::string_view pCmdStr, int pPriority, int (*pFn)(const std::shared_ptr<Player>& player, cmd* cmnd), bool (*pAuth)(const std::shared_ptr<Creature> &), std::string_view pDesc): fn(pFn)
    {
        name = std::string(pCmdStr);
        priority = pPriority;
        auth = pAuth;
        description = std::string(pDesc);
    };
    PlyCommand(std::string_view pCmdStr) {
        name = pCmdStr;
    }
    ~PlyCommand() = default;
    int (*fn)(const std::shared_ptr<Player>& player, cmd* cmnd);
    int execute(const std::shared_ptr<Creature>& player, cmd* cmnd) const;
};

typedef int (*PFNCOMPARE)(const void *, const void *);
