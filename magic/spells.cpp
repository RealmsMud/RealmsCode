/*
 * Spells.cpp
 *   Additional spell-casting routines.
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

#include <fmt/format.h>                          // for format
#include <boost/algorithm/string/case_conv.hpp>  // for to_lower_copy
#include <boost/algorithm/string/join.hpp>       // for join
#include <boost/iterator/iterator_facade.hpp>    // for operator!=

#include <cstdlib>                               // for qsort
#include <cstring>                               // for memcpy, strcpy, strcmp
#include <list>                                  // for list, operator==
#include <set>                                   // for operator==, _Rb_tree...
#include <string>                                // for string, allocator
#include "anchor.hpp"                            // for Anchor
#include "area.hpp"                              // for MapMarker
#include "catRef.hpp"                            // for CatRef
#include "cmd.hpp"                               // for cmd
#include "color.hpp"                             // for stripColor
#include "commands.hpp"                          // for cmdSpells, spellsUnder
#include "config.hpp"                            // for Config, gConfig, Spe...
#include "creatureStreams.hpp"                   // for Streamable, ColorOn
#include "effects.hpp"                           // for Effect, EffectInfo
#include "flags.hpp"                             // for P_AFK, P_FREE_ACTION
#include "free_crt.hpp"                          // for free_crt
#include "global.hpp"                            // for CreatureClass, Creat...
#include "magic.hpp"                             // for SpellData, Divine
#include "mudObjects/creatures.hpp"              // for Creature
#include "mudObjects/players.hpp"                // for Player
#include "proto.hpp"                             // for zero, get_spell_list...
#include "server.hpp"                            // for Server, gServer
#include "structs.hpp"                           // for Spell, PFNCOMPARE
#include "xml.hpp"                               // for loadPlayer


//*********************************************************************
//                      clearSpells
//*********************************************************************

void Config::clearSpells() {
    spells.clear();
}

//*********************************************************************
//                      Spell
//*********************************************************************

int dmSpellList(Player* player, cmd* cmnd) {
    const Spell* spell=nullptr;

    player->printColor("^YSpells\n");
    for(const auto& sp : gConfig->spells) {
        *player << ColorOn << fmt::format("  {}   {} - {}\n    Script: ^y{}^x\n", spell->name, spell->priority, spell->description, spell->script);
    }

    return(0);
}

//*********************************************************************
//                      spellSkill
//*********************************************************************

std::string spellSkill(SchoolOfMagic school) {
    switch(school) {
    case ABJURATION:
        return("abjuration");
    case CONJURATION:
        return("conjuration");
    case DIVINATION:
        return("divination");
    case ENCHANTMENT:
        return("enchantment");
    case EVOCATION:
        return("evocation");
    case ILLUSION:
        return("illusion");
    case NECROMANCY:
        return("necromancy");
    case TRANSLOCATION:
        return("translocation");
    case TRANSMUTATION:
        return("transmutation");
    default:
        return("");
    }
}

//*********************************************************************
//                      spellSkill
//*********************************************************************

std::string spellSkill(DomainOfMagic domain) {
    switch(domain) {
    case HEALING:
        return("healing");
    case DESTRUCTION:
        return("destruction");
    case EVIL:
        return("evil");
    case GOOD:
        return("good");
    case KNOWLEDGE:
        return("knowledge");
    case PROTECTION:
        return("protection");
    case NATURE:
        return("nature");
    case AUGMENTATION:
        return("augmentation");
    case TRICKERY:
        return("trickery");
    case TRAVEL:
        return("travel");
    case CREATION:
        return("creation");
    default:
        return("");
    }
}

//*********************************************************************
//                      getCastingType
//*********************************************************************

MagicType Creature::getCastingType() const {
    CreatureClass cls = getClass();

    if(isPlayer()) {
        if(getAsConstPlayer()->getSecondClass() == CreatureClass::MAGE)
            cls = CreatureClass::MAGE;
        else if(getAsConstPlayer()->getSecondClass() == CreatureClass::CLERIC)
            cls = CreatureClass::CLERIC;
    }

    switch(cls) {
        case CreatureClass::DUNGEONMASTER:
        case CreatureClass::CARETAKER:
        case CreatureClass::BUILDER:

        // pure arcane
        case CreatureClass::MAGE:
        case CreatureClass::LICH:
        // hybrid arcane
        case CreatureClass::BARD:
        case CreatureClass::PUREBLOOD:
            return(Arcane);
        // pure divine
        case CreatureClass::CLERIC:
        case CreatureClass::DRUID:
        // hybrid divine
        case CreatureClass::PALADIN:
        case CreatureClass::RANGER:
        case CreatureClass::DEATHKNIGHT:
            return(Divine);
        default:
            return(NO_MAGIC_TYPE);
    }
}

//*********************************************************************
//                      isPureCaster
//*********************************************************************

bool Creature::isPureCaster() const {
    int second = isPlayer() ? getAsConstPlayer()->hasSecondClass() : 0;
    return( (cClass == CreatureClass::MAGE && !second) ||
            cClass == CreatureClass::LICH ||
            (cClass == CreatureClass::CLERIC && !second) ||
            cClass == CreatureClass::DRUID
    );
}

//*********************************************************************
//                      isHybridCaster
//*********************************************************************

bool Creature::isHybridCaster() const {
    CreatureClass second = isPlayer() ? getAsConstPlayer()->getSecondClass() : CreatureClass::NONE;
    return( cClass == CreatureClass::BARD ||
            cClass == CreatureClass::DEATHKNIGHT ||
            cClass == CreatureClass::PALADIN ||
            cClass == CreatureClass::RANGER ||
            cClass == CreatureClass::PUREBLOOD ||
            second == CreatureClass::MAGE ||
            (cClass == CreatureClass::MAGE && second != CreatureClass::NONE) ||
            (cClass == CreatureClass::CLERIC && second != CreatureClass::NONE)
    );
}

//*********************************************************************
//                      SpellData
//*********************************************************************

SpellData::SpellData() {
    how = CastType::CAST;
    splno = 0;
    level = 0;
    object = nullptr;
    school = NO_SCHOOL;
    skill = "";
}

//*********************************************************************
//                      set
//*********************************************************************

void SpellData::set(CastType h, SchoolOfMagic s, DomainOfMagic d, Object* obj, const Creature* caster) {
    how = h;
    school = s;
    domain = d;
    object = obj;
    if(caster->getCastingType() == Divine)
        skill = spellSkill(domain);
    else
        skill = spellSkill(school);
    level = caster->getLevel();
    /*
     * Start small: don't do skill-based levels yet
    if(skill == "") {
        level = caster->getLevel();
    } else {
        double l = caster->getSkillLevel(skill);
        if(l > 0)
            level = (int)l;
    }
    */
}

//*********************************************************************
//                      check
//*********************************************************************

bool SpellData::check(const Creature* player, bool skipKnowCheck) const {
    if(player->isMonster())
        return(true);
    if( !skipKnowCheck &&
        !skill.empty() &&
        !player->knowsSkill(skill) &&
        !player->checkStaff("You are unable to cast %s spells.\n", gConfig->getSkillDisplayName(skill).c_str())
    )
        return(false);
    if( (skill == "necromancy" || skill == "evil") &&
        player->getAdjustedAlignment() > (player->getClass() == CreatureClass::LICH ? PINKISH : REDDISH) &&
        !player->checkStaff("You must be evil to cast %s spells.\n", gConfig->getSkillDisplayName(skill).c_str())
    )
        return(false);
    if( skill == "good" &&
        player->getAdjustedAlignment() < BLUISH &&
        !player->checkStaff("You must be good to cast %s spells.\n", gConfig->getSkillDisplayName(skill).c_str())
    )
        return(false);

    return(true);
}

//*********************************************************************
//                      infoSpells
//*********************************************************************
// This function is the second half of info which outputs spells

void infoSpells(const Player* viewer, Creature* target, bool notSelf) {
    Player *player = target->getAsPlayer();
    const Anchor* anchor=nullptr;
    MagicType castingType = target->getCastingType();
    int     min = (castingType == Divine ? (int)MIN_DOMAIN : (int)MIN_SCHOOL) + 1, max = (castingType == Divine ? (int)MAX_DOMAIN : (int)MAX_SCHOOL);
    char    spl[max][MAXSPELL][24], list[MAXSPELL][24];
    int     count=0, j[max], l = sizeof(list);
    int     i=0,n=0;
    std::string str="";
    std::string skillName="";

    if(notSelf)
        viewer->printColor("\n^Y%M's Spells Known: ", target);
    else
        viewer->printColor("\n^YSpells known: ");

    zero(j, sizeof(j));
    zero(spl, sizeof(spl));
    for(i = 0; i < get_spell_list_size() ; i++) {
        if(target->spellIsKnown(i)) {
            count++;

            if(castingType == Divine) {
                n = (int)get_spell_domain(i);
                // monsters ignore these rules - move into the "other" category
                if(n == DOMAIN_CANNOT_CAST && !player)
                    n = NO_DOMAIN;
            } else {
                n = (int)get_spell_school(i);
                // monsters ignore these rules - move into the "other" category
                if(n == SCHOOL_CANNOT_CAST && !player)
                    n = NO_SCHOOL;
            }

            strcpy(spl[n][j[n]++], get_spell_name(i));
        }
    }

    if(!count) {
        viewer->print("None.");
    } else {
        viewer->print("%d", count);
        for(n = min; n < max; n++) {
            if(j[n]) {
                memcpy(list, spl[n], l);
                qsort((void *) list, j[n], 24, (PFNCOMPARE) strcmp);
                str = "";
                for(i = 0; i < j[n]; i++) {
                    str += list[i];
                    str += ", ";
                }

                if(castingType == Divine) {
                    if((DomainOfMagic)n == NO_DOMAIN) {
                        skillName = "Other";
                    } else if((DomainOfMagic)n == DOMAIN_CANNOT_CAST) {
                        skillName = "Not Castable";
                    } else {
                        skillName = gConfig->getSkillDisplayName(spellSkill((DomainOfMagic)n));
                    }
                } else{
                    if((SchoolOfMagic)n == NO_SCHOOL) {
                        skillName = "Other";
                    } else if((SchoolOfMagic)n == SCHOOL_CANNOT_CAST) {
                        skillName = "Not Castable";
                    } else {
                        skillName = gConfig->getSkillDisplayName(spellSkill((SchoolOfMagic)n));
                    }
                }
                str = str.substr(0, str.length() - 2) + ".";
                viewer->printColor("\n^W%s:^x %s", skillName.c_str(), str.c_str());
            }
        }
    }

    viewer->print("\n\n");




    spellsUnder(viewer, target, notSelf);

    if(!player)
        return;

    if(player->hasAnchor(0) || player->hasAnchor(1) || player->hasAnchor(2)) {
        viewer->printColor("^YCurrent Dimensional Anchors\n");

        for(i=0; i<MAX_DIMEN_ANCHORS; i++) {
            if(player->hasAnchor(i)) {
                anchor = player->getAnchor(i);
                viewer->printColor("^c%s ^x-> ^c%s", anchor->getAlias().c_str(),
                    anchor->getRoomName().c_str());

                if(viewer->isStaff())
                    viewer->print("  %s", (anchor->getMapMarker() ? anchor->getMapMarker()->str() : anchor->getRoom().displayStr()).c_str());

                viewer->print("\n");
            }
        }
    }
}

//*********************************************************************
//                      cmdSpells
//*********************************************************************

int cmdSpells(Creature* player, cmd* cmnd) {
    Creature* target = player;
    Player  *pTarget=nullptr, *viewer=nullptr;
    bool    notSelf=false;

    if(player->isPet()) {
        viewer = player->getPlayerMaster();
        notSelf = true;
    } else {
        viewer = player->getAsPlayer();
    }

    viewer->clearFlag(P_AFK);

    if(player->isBraindead()) {
        player->print("You are brain-dead. You can't do that.\n");
        return(0);
    }

    if(player->isCt()) {
        if(cmnd->num > 1) {
            cmnd->str[1][0] = up(cmnd->str[1][0]);
            target = gServer->findPlayer(cmnd->str[1]);
            notSelf = true;

            if(!target) {
                if(!loadPlayer(cmnd->str[1], &pTarget)) {
                    player->print("Player does not exist.\n");
                    return(0);
                }
                infoSpells(viewer, pTarget, notSelf);
                free_crt(pTarget);
                return(0);
            } else {
                if(!player->canSee(target)) {
                    player->print("That player is not logged on.\n");
                    return(0);
                }
            }
        }
    }

    infoSpells(viewer, target, notSelf);
    return(0);
}

//*********************************************************************
//                      spellsUnder
//*********************************************************************

std::string effectSpellName(std::string effect) {
    effect = stripColor(boost::algorithm::to_lower_copy(effect));

    if(effect == "silent!") return("silence");
    if(effect == "blessed") return("bless");
    if(effect == "detect-invisible") return("detect-invisibility");
    if(effect == "invisible") return("invisibility");

    return(effect);
}

//*********************************************************************
//                      spellsUnder
//*********************************************************************

void spellsUnder(const Player *viewer, const Creature* target, bool notSelf) {
    std::string str;
    const Player* player = target->getAsConstPlayer();
    std::list<std::string> spells;
    const Effect* effect=nullptr;

    // effects are flagged as whether or not they are a spell effect
    for(const auto& effectInfo : target->effects.effectList) {
        effect = effectInfo->getEffect();
        if(!effect->isSpell())
            continue;
        spells.push_back(effectSpellName(effect->getDisplay()));
    }

    // we want some flags to show up, too
    if(player) {
        if(player->flagIsSet(P_FREE_ACTION))
            spells.emplace_back("free-action");
        if(player->flagIsSet(P_STUNNED))
            spells.emplace_back("stun");
    }


    spells.sort();
    if(!spells.empty())
        str = boost::algorithm::join(spells, ", ");
    else
        str = "None";

    if(notSelf)
        (Streamable &) *viewer << ColorOn << "^W" << target << "'s ";
    (Streamable &) *viewer << ColorOn << "^WSpells Under:^x " << str << ".\n\n" << ColorOff;
}
