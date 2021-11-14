/*
 * SkillLoader.cpp
 *   Skill related item loaders
 *   ____    _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_); / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C); 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C); Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <map>                        // for allocator, map
#include <string>                     // for string, operator<=>
#include <utility>                    // for move

#include "builders/skillBuilder.hpp"  // for SkillInfoBuilder
#include "config.hpp"                 // for Config, SkillInfoMap
#include "skills.hpp"                 // for SkillGainType, MEDIUM, HARD

bool Config::loadSkillGroups() {
    skillGroups.emplace("general", "General Skills");
    skillGroups.emplace("offensive", "Offensive Skills");
    skillGroups.emplace("defensive", "Defensive Skills");
    skillGroups.emplace("armor", "Armor");
    skillGroups.emplace("craft", "Crafting Skills");
    skillGroups.emplace("weapons", "Weapons");
    skillGroups.emplace("weapons-slashing", "Slashing Weapons");
    skillGroups.emplace("weapons-exotic", "Exotic Weapons");
    skillGroups.emplace("weapons-chopping", "Chopping Weapons");
    skillGroups.emplace("weapons-piercing", "Piercing Weapons");
    skillGroups.emplace("weapons-crushing", "Crushing Weapons");
    skillGroups.emplace("weapons-ranged", "Ranged Weapons");
    skillGroups.emplace("magic", "Magical Realms");
    skillGroups.emplace("arcane", "Schools of Magic");
    skillGroups.emplace("divine", "Domains of Magic");

    return true;
}

void addToMap(SkillInfo &&skillInfo, SkillInfoMap &skillInfoMap) {
    skillInfoMap.emplace(skillInfo.getName(), std::move(skillInfo));
}

bool Config::loadSkills() {

    /*
     * General
     */
    addToMap(SkillInfoBuilder()
        .name("barkskin").displayName("Barkskin")
        .group("defensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("dodge").displayName("Dodge")
        .knownOnly(true)
        .group("defensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("parry").displayName("Parry")
        .group("defensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("endurance").displayName("Endurance")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("howl").displayName("Howl")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("transmute").displayName("Transmute")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("enchant").displayName("Enchant")
        .group("general")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("teach").displayName("Teach")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("regenerate").displayName("Regenerate")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("meditate").displayName("Meditate")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("focus").displayName("Focus")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("pray").displayName("Pray")
        .group("general")
        .gainType(SkillGainType::HARD)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("enthrall").displayName("Enthrall Undead")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("animate").displayName("Animate Undead")
        .group("general")
        .gainType(SkillGainType::HARD)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("charm").displayName("Charm")
        .group("general")    
    , skills);
    addToMap(SkillInfoBuilder()
        .name("sing").displayName("Sing")
        .group("general")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("identify").displayName("Identify")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("sneak").displayName("Sneak")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("hide").displayName("Hide")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("envenom").displayName("Envenom")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("peek").displayName("Peek")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("steal").displayName("Steal")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("search").displayName("Search")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("pick").displayName("Picklock")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("conjure").displayName("Conjure Elemental")
        .group("general")
        .gainType(SkillGainType::HARD)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("track").displayName("Find Track")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("commune").displayName("Commune")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("dual").displayName("Dual Wield")
        .knownOnly(true)
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("mistbane").displayName("Mistbane")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("mist").displayName("Turn to Mist")
        .knownOnly(true)
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("hypnotize").displayName("Hypnotize")
        .group("general")
        .gainType(SkillGainType::HARD)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("frenzy").displayName("Frenzy")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("bloodsac").displayName("Blood Sacrifice")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("hands").displayName("Lay Hands")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("scout").displayName("Scout")
        .group("general")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("shoplift").displayName("Shoplift")
        .group("general")
    , skills);

    /*
     * Magic
     */
    addToMap(SkillInfoBuilder()
        .name("earth").displayName("Earth Realm")
        .group("magic")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("air").displayName("Air Realm")
        .group("magic")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("fire").displayName("Fire Realm")
        .group("magic")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("water").displayName("Water Realm")
        .group("magic")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("electric").displayName("Electric Realm")
        .group("magic")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("cold").displayName("Cold Realm")
        .group("magic")
    , skills);

    /*
     * Arcane
     */
    addToMap(SkillInfoBuilder()
        .name("abjuration").displayName("Abjuration")
        .group("arcane")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("conjuration").displayName("Conjuration")
        .group("arcane")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("divination").displayName("Divination")
        .group("arcane")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("enchantment").displayName("Enchantment")
        .group("arcane")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("evocation").displayName("Evocation")
        .group("arcane")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("illusion").displayName("Illusion")
        .group("arcane")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("necromancy").displayName("Necromancy")
        .group("arcane")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("transmutation").displayName("Transmutation")
        .group("arcane")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("translocation").displayName("Translocation")
        .group("arcane")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    /*
     * Divine
     */
    addToMap(SkillInfoBuilder()
        .name("healing").displayName("Healing")
        .group("divine")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("destruction").displayName("Destruction")
        .group("divine")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("evil").displayName("Evil")
        .group("divine")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("good").displayName("Good")
        .group("divine")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("knowledge").displayName("Knowledge")
        .group("divine")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("protection").displayName("Protection")
        .group("divine")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("nature").displayName("Nature")
        .group("divine")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("augmentation").displayName("Augmentation")
        .group("divine")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("trickery").displayName("Trickery")
        .group("divine")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("travel").displayName("Travel")
        .group("divine")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("creation").displayName("Creation")
        .group("divine")
        .gainType(SkillGainType::MEDIUM)
    , skills);

    /*
     * Offensive
     */
    addToMap(SkillInfoBuilder()
        .name("circle").displayName("Circle")
        .group("offensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("bash").displayName("Bash")
        .group("offensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("kick").displayName("Kick")
        .group("offensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("disarm").displayName("Disarm")
        .group("offensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("berserk").displayName("Berserk")
        .group("offensive")
        .gainType(SkillGainType::HARD)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("drain").displayName("Drain Life")
        .group("offensive")
        .gainType(SkillGainType::HARD)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("touch").displayName("Touch of Death")
        .group("offensive")
        .gainType(SkillGainType::HARD)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("turn").displayName("Turn Undead")
        .group("offensive")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("holyword").displayName("Holyword")
        .group("offensive")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("poison").displayName("Poison Strike")
        .group("offensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("creeping-doom").displayName("Creeping Doom")
        .group("offensive")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("smother").displayName("Smother")
        .group("offensive")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("backstab").displayName("Backstab")
        .group("offensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("ambush").displayName("Ambush")
        .group("offensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("bite").displayName("Bite")
        .group("offensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("maul").displayName("Maul")
        .group("offensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("renounce").displayName("Renounce")
        .group("offensive")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("harm").displayName("Harm Touch")
        .group("offensive")
        .gainType(SkillGainType::HARD)
    , skills);

    /*
     * Weapons
     */
    addToMap(SkillInfoBuilder()
        .name("slashing").displayName("Slashing Weapons")
        .group("weapons")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("exotic").displayName("Exotic Weapons")
        .group("weapons")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("chopping").displayName("Chopping Weapons")
        .group("weapons")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("piercing").displayName("Piercing Weapons")
        .group("weapons")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("crushing").displayName("Crushing Weapons")
        .group("weapons")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("ranged").displayName("Ranged Weapons")
        .group("weapons")
    , skills);

    /*
     * Weapon Sub-Skills
     */
    addToMap(SkillInfoBuilder()
        .name("claw").displayName("Werewolf Claws")
        .group("weapons-slashing")
        .baseSkill("slashing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("sword").displayName("Swords")
        .group("weapons-slashing")
        .baseSkill("slashing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("knife").displayName("Knives")
        .group("weapons-slashing")
        .baseSkill("slashing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("great-sword").displayName("Great Swords")
        .group("weapons-slashing")
        .baseSkill("slashing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("whip").displayName("Whips")
        .group("weapons-exotic")
        .baseSkill("exotic")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("axe").displayName("Axes")
        .group("weapons-chopping")
        .baseSkill("chopping")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("great-axe").displayName("Great Axes")
        .group("weapons-chopping")
        .baseSkill("chopping")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("rapier").displayName("Rapiers and Sabres")
        .group("weapons-piercing")
        .baseSkill("piercing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("spear").displayName("Spears")
        .group("weapons-piercing")
        .baseSkill("piercing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("dagger").displayName("Daggers")
        .group("weapons-piercing")
        .baseSkill("piercing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("polearm").displayName("Polearms")
        .group("weapons-exotic")
        .baseSkill("exotic")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("staff").displayName("Staves")
        .group("weapons-crushing")
        .baseSkill("crushing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("mace").displayName("Maces")
        .group("weapons-crushing")
        .baseSkill("crushing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("great-mace").displayName("Great-Maces")
        .group("weapons-crushing")
        .baseSkill("crushing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("club").displayName("Clubs and other Blunt Objects")
        .group("weapons-crushing")
        .baseSkill("crushing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("hammer").displayName("Hammers")
        .group("weapons-crushing")
        .baseSkill("crushing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("flail").displayName("Flails")
        .group("weapons-crushing")
        .baseSkill("crushing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("great-hammer").displayName("Great-Hammers")
        .group("weapons-crushing")
        .baseSkill("crushing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("bare-hand").displayName("Bare-Handed")
        .group("weapons-crushing")
        .baseSkill("crushing")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("bow").displayName("Bows")
        .group("weapons-ranged")
        .baseSkill("ranged")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("sling").displayName("Slings")
        .group("weapons-ranged")
        .baseSkill("ranged")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("arcane-weapon").displayName("Arcane Weapons")
        .group("weapons-ranged")
        .baseSkill("ranged")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("divine-weapon").displayName("Divine Weapons")
        .group("weapons-ranged")
        .baseSkill("ranged")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("crossbow").displayName("Crossbows")
        .group("weapons-ranged")
        .baseSkill("ranged")
        .knownOnly(true)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("thrown").displayName("Thrown Weapons")
        .group("weapons-ranged")
        .baseSkill("ranged")
        .knownOnly(true)
    , skills);

    /*
     * Defensive
     */
    addToMap(SkillInfoBuilder()
        .name("defense").displayName("Defense")
        .group("defensive")
    , skills);
    addToMap(SkillInfoBuilder()
        .name("block").displayName("Block")
        .group("defensive")
    , skills);

    /*
     * Armor
     */
    addToMap(SkillInfoBuilder()
        .name("cloth").displayName("Cloth Armor")
        .group("armor")
        .gainType(SkillGainType::HARD)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("leather").displayName("Leather Armor")
        .group("armor")
        .gainType(SkillGainType::HARD)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("chain").displayName("Chain Armor")
        .group("armor")
        .gainType(SkillGainType::HARD)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("scale").displayName("Scale Armor")
        .group("armor")
        .gainType(SkillGainType::HARD)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("ring").displayName("Ring Armor")
        .group("armor")
        .gainType(SkillGainType::HARD)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("plate").displayName("Plate Armor")
        .group("armor")
        .gainType(SkillGainType::HARD)
    , skills);

    /*
     * Crafting
     */
    addToMap(SkillInfoBuilder()
        .name("fishing").displayName("Fishing")
        .group("craft")
        .gainType(SkillGainType::EASY)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("cooking").displayName("Cooking")
        .group("craft")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("smithing").displayName("Smithing")
        .group("craft")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("tailoring").displayName("Tailoring")
        .group("craft")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    addToMap(SkillInfoBuilder()
        .name("carpentry").displayName("Carpentry")
        .group("craft")
        .gainType(SkillGainType::MEDIUM)
    , skills);
    return true;
}