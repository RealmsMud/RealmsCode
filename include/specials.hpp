/*
 * specials.h
 *   Special attacks
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
/*          Web Editor
 *           _   _  ____ _______ ______
 *          | \ | |/ __ \__   __|  ____|
 *          |  \| | |  | | | |  | |__
 *          | . ` | |  | | | |  |  __|
 *          | |\  | |__| | | |  | |____
 *          |_| \_|\____/  |_|  |______|
 *
 *      If you change anything here, make sure the changes are reflected in the web
 *      editor! Either edit the PHP yourself or tell Dominus to make the changes.
 */

#ifndef SPECIALS_H_
#define SPECIALS_H_

#include <libxml/parser.h>  // for xmlNodePtr

#include "dice.hpp"
#include "lasttime.hpp"
#include "realm.hpp"

class BaseRoom;
class Creature;

enum SpecialType {
    SPECIAL_NO_TYPE     = 0,
    SPECIAL_EARTH       = EARTH,    // 1 - Realm based breath weapons
    SPECIAL_WIND        = WIND,     // 2
    SPECIAL_FIRE        = FIRE,     // 3
    SPECIAL_WATER       = WATER,    // 4
    SPECIAL_ELECTRIC    = ELEC,     // 5
    SPECIAL_COLD        = COLD,     // 6
    SPECIAL_WEAPON      = 7,
    SPECIAL_GENERAL     = 8,
    SPECIAL_NO_DAMAGE   = 9,        // Non damaging attacks (petrify, blind, confusion)
    SPECIAL_BREATH      = 10,       // Non realm breath weapon (poison, etc)
    SPECIAL_STEAL       = 11,
    SPECIAL_EXP_DRAIN   = 12,       // Drains experience instead of damage
    SPECIAL_PETRIFY     = 13,       // Attack can petrify the target
    SPECIAL_CONFUSE     = 14,       // Attack can render the target confused

    MAX_SPECIAL
};

enum SpecialAttackFlags {
    SA_NO_FLAG                  = 0,
    SA_SINGLE_TARGET            = 1,    // Attack a single target (bash/bite/backstab etc)
    SA_AE_ENEMY                 = 2,    // Hit all enemies in the room
    SA_AE_ALL                   = 3,    // Hit everyone (except itself) in the room
    SA_AE_PLAYER                = 4,    // Hit all players in the room
    SA_AE_MONSTER               = 5,    // Hit all monsters in the room
    SA_REQUIRE_HIDE             = 6,    // Attacker must be hidden, ie: for backstab
    // These are for weapon type attacks
    SA_NO_DODGE                 = 7,    // This attack cannot be dodged
    SA_NO_PARRY                 = 8,    // This attack cannot be parried
    SA_NO_BLOCK                 = 9,    // This attack cannot be blocked
    // End weapon type attack flags
    SA_UNDEAD_ONLY              = 10,   // Only effective against undead
    SA_NO_UNDEAD                = 11,   // Not effective against undead targets
    SA_NO_UNDEAD_WARD           = 12,   // Attack won't go off if undead and target has u-w
    SA_NO_DRAIN_SHIELD          = 13,   // Drain shield stops damage
    SA_UNDEAD_WARD_REDUCE       = 14,   // Damage reduced if undead vs undead-ward
    SA_INTELLIGENCE_REDUCE      = 15,   // High intelligence helps mitigate damage
    SA_BERSERK_REDUCE           = 16,   // Berserk substantially reduces damage
    SA_DRAIN_SHIELD_REDUCE      = 17,   // Drain shield reduces damage
    SA_SAVE_NO_DAMAGE           = 18,   // No damage on an effective save
    SA_HALF_HP_DAMAGE           = 19,   // Half of the target's hp as damage
    SA_CAN_DISINTEGRATE         = 20,   // Chance to kill in one shot
    SA_RESIST_MAGIC_NO_DAMAGE   = 21,   // No damage if the target is resisting magic
    SA_EARTH_SHIELD_REDUCE      = 22,   // Earth-Shield will help reduce damage
    SA_ACID                     = 23,   // Attack can disove an item (including inventory)
    SA_DISSOLVE                 = 24,   // Attack can dissolve a worm item
    SA_POISON                   = 25,   // Attack can poison the target
    SA_DISEASE                  = 26,   // Attack can disease the target
    SA_BLIND                    = 27,   // Attack can blind the target
    SA_UNCONSCIOUS              = 28,   // Attack can knock someone unconscious
    SA_ZAP_MANA                 = 29,   // Attack can drain the target's mana
    SA_RANDOMIZE_STUN           = 30,   // Add some randomness to the stun time
    SA_CHECK_DIE_ROB            = 31,   // Check for robbery and not just death
    SA_DRAINS_DAMAGE            = 32,   // Attacker absorbs some of the damage to their hp
    SA_DAY_ONLY                 = 33,   // Only during the day
    SA_NIGHT_ONLY               = 34,   // Only at night
    SA_CHECK_PHYSICAL_DAMAGE    = 35,   // Check Physical Damage
    SA_CHECK_NEGATIVE_ENERGY    = 36,   // Check Negative Energy Damage
    SA_NO_ATTACK_ON_LOW_HP      = 37,   // Attack will not go off if under 20% hp
    SA_HOLY_WAR                 = 38,   // Attack will only go of people in a holy war! (dk/paly)
    SA_NO_UNCONSCIOUS           = 39,   // No sleeping, blind or otherwise unconscious targets
    SA_TARGET_NEEDS_MANA        = 40,   // Don't do the attack if the target has no mana (mana stealing for example)
    SA_NO_ROOM_MESSAGE_ON_SAVE  = 41,   // Don't print roomStr on save
    SA_BREATHING_TARGETS        = 42,   // Only breathing targets are effected
    SA_MAX_FLAG
};
enum SaveBonus {
    BONUS_NONE          = 0,

    BONUS_LEVEL_CON     = 1,
    BONUS_LEVEL_DEX     = 2,
    BONUS_LEVEL_INT     = 3,

    BONUS_MAX
};
enum SpecialSaveType {
    SAVE_NONE           = 0,
    // These use the save function
    SAVE_LUCK           = 1,
    SAVE_POISON         = 2,
    SAVE_DEATH          = 3,
    SAVE_BREATH         = 4,
    SAVE_MENTAL         = 5,
    SAVE_SPELL          = 6,

    // These will compute the chance based on the target's stat
    SAVE_STRENGTH       = 7,
    SAVE_DEXTERITY      = 8,
    SAVE_CONSTITUTION   = 9,
    SAVE_INTELLIGENCE   = 10,
    SAVE_PIETY          = 11,

    SAVE_LEVEL          = 12,

    SAVE_MAX
};

class SpecialAttack {
private:
    std::string name;

    std::string verb;           // For broadcast group, *attacker* *verb* *target*:
                            // ie: The monster backstabbed john for 25 damage!

    std::string selfStr;        // What we show to the attacker
    std::string selfFailStr;

    std::string targetStr;
    std::string roomStr;

    // Fail strings are generally only for weapon type attacks, for non weapon attacks you want to fail
    // use save strings and set SAVE_NO_DAMAGE flag
    std::string targetFailStr;  // Output on a failed attack: ie - missed backstab
    std::string roomFailStr;    // Output on a failed attack

    SpecialSaveType saveType;   // Save, stat or level to check for save
    SaveBonus saveBonus;        // What save bonuses
    int maxBonus{};             // Max save bonus

    // Strings to print on a sucessful save
    std::string targetSaveStr;
    std::string selfSaveStr;
    std::string roomSaveStr;

    int chance{};         // Chance 1-100 that this attack will be executed
    int delay{};          // Delay between uses
    lasttime ltime;     // When we last used it, when we can use it again, etc
    int stunLength{};
    SpecialType type;   // Fire, water, general breath, weapon attack, etc
    char flags[8]{};      // 8*8 flags
    Dice damage;

    int limit{};          // Max number of times this attack can be used in a monster's lifetime
    int used{};           // How many times we've used this attack since being alive (Doesn't save)

private:
    void reset();
    int computeDamage(bool saved = false);
    std::string modifyAttackString(std::string_view input, Creature* viewer, Creature* attacker, Creature* target, int dmg = -1);
    void printToRoom(BaseRoom* room, std::string_view str, Creature* attacker, Creature* target, int dmg = -1);
    void setFlag(int flag);
    void clearFlag(int flag);

    friend class Creature;
    friend class Monster;
    friend class Player;
public:
    SpecialAttack();
    SpecialAttack(xmlNodePtr rootNode);


    friend std::ostream& operator<<(std::ostream& out, const SpecialAttack& attack);

    std::string getName();

    bool save(xmlNodePtr rootNode) const;


    [[nodiscard]] bool isAreaAttack() const;
    [[nodiscard]] bool flagIsSet(int flag) const;

    void printFailStrings(Creature* attacker, Creature* target);
    void printRoomString(Creature* attacker, Creature* target = nullptr);
    void printTargetString(Creature* attacker, Creature* target, int dmg = -1);

    void printRoomSaveString(Creature* attacker, Creature* target);
    void printTargetSaveString(Creature* attacker, Creature* target, int dmg = -1);

};



#endif /*SPECIALS_H_*/
