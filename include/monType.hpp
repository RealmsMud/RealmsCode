/*
 * monType.h
 *   Monster Type functions
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

#include "size.hpp"

enum mType {
    INVALID        = -1,
    PLAYER          = 0,
    MONSTER         = 1,

    NPC             = 2,
    HUMANOID        = 2,

    GOBLINOID       = 3,
    MONSTROUSHUM    = 4,
    GIANTKIN        = 5,
    ANIMAL          = 6,
    DIREANIMAL      = 7,
    INSECT          = 8,
    INSECTOID       = 9,
    ARACHNID        = 10,
    REPTILE         = 11,
    DINOSAUR        = 12,
    AUTOMATON       = 13,
    AVIAN           = 14,
    FISH            = 15,
    PLANT           = 16,
    DEMON           = 17,
    DEVIL           = 18,
    DRAGON          = 19,
    BEAST           = 20,
    MAGICALBEAST    = 21,
    GOLEM           = 22,
    ETHEREAL        = 23,
    ASTRAL          = 24,
    GASEOUS         = 25,
    ENERGY          = 26,
    FAERIE          = 27,
    DEVA            = 28,
    ELEMENTAL       = 29,
    PUDDING         = 30,
    SLIME           = 31,
    UNDEAD          = 32,

    MAX_MOB_TYPES
};


namespace monType {
    bool isIntelligent(mType type);
    char *getName(mType type);
    int getHitdice(mType type);
    bool isUndead(mType type);
    bool isPlant(mType type);
    bool isElemental(mType type);
    bool isFey(mType type);
    bool isAnimal(mType type);
    bool isInsectLike(mType type);
    bool isHumanoidLike(mType type);
    bool isExtraplanar(mType type);
    bool isMindless(mType type);
    Size size(mType type);
    bool immuneCriticals(mType type);
    bool noLivingVulnerabilities(mType type);
}


