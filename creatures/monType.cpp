/*
 * monType.cpp
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

#include "monType.hpp"  // for mType, MAX_MOB_TYPES, UNDEAD, AUTOMATON, ELEM...
#include "size.hpp"     // for NO_SIZE, SIZE_FINE, SIZE_GARGANTUAN, SIZE_HUGE

char mobtype_name[MAX_MOB_TYPES][20] = { "Player", "Monster", "Humanoid", "Goblinoid", "Monstrous Humanoid",
        "Giantkin", "Animal", "Dire Animal", "Insect", "Insectoid", "Arachnid", "Reptile", "Dinosaur", "Automaton",
        "Avian", "Fish", "Plant", "Demon", "Devil", "Dragon", "Beast", "Magical Beast", "Golem", "Ethereal", "Astral",
        "Gaseous", "Energy", "Faerie", "Deva", "Elemental", "Pudding", "Slime", "Undead", "Ooze", "Modron", "Daemon" };

int mob_hitdice[MAX_MOB_TYPES] = { 0, 10, 10, 8, 10, 16, 6, 10, 4, 10, 10, 8, 12, 12, 8, 8, 10, 18, 18, 24, 12, 16, 16,
        12, 12, 10, 14, 8, 20, 14, 14, 10, 14, 12, 14, 16 };

//*********************************************************************
//                      noLivingVulnerabilities
//*********************************************************************
// immunity to: gas-breath, drowning, poison, disease

bool monType::noLivingVulnerabilities(mType type) {
    switch(type) {
    case AUTOMATON:
    case GOLEM:
    case ETHEREAL:
    case GASEOUS:
    case ENERGY:
    case ELEMENTAL:
    case PUDDING:
    case SLIME:
    case UNDEAD:
    case OOZE:
        return(true);

    default:
        return(false);
    }
}

//*********************************************************************
//                      isIntelligent
//*********************************************************************

bool monType::isIntelligent(mType type) {
    switch(type) {
    case DEMON:
    case DEVA:
    case DEVIL:
    case DRAGON:
    case FAERIE:
    case HUMANOID:
    case GIANTKIN:
    case GOBLINOID:
    case MONSTROUSHUM:
    case UNDEAD:
    case MODRON:
    case DAEMON:
        return(true);

    default:
        return(false);
    }
}

//*********************************************************************
//                      isUndead
//*********************************************************************

bool monType::isUndead(mType type) {
    return(type == UNDEAD);
}

//*********************************************************************
//                      isPlant
//*********************************************************************
bool monType::isPlant(mType type) {
    return(type == PLANT);
}
//*********************************************************************
//                      isElemental
//*********************************************************************
bool monType::isElemental(mType type) {
    return(type == ELEMENTAL);
}
//*********************************************************************
//                      isFey
//*********************************************************************
bool monType::isFey(mType type) {
    return(type == FAERIE);
}
//*********************************************************************
//                      isAnimal
//*********************************************************************
bool monType::isAnimal(mType type) {
    return(type == ANIMAL || type == DIREANIMAL || type == FISH ||
           type == AVIAN || type == REPTILE || type == DINOSAUR);
}
//*********************************************************************
//                      isInsectLike
//*********************************************************************
bool monType::isInsectLike(mType type) {
    return(type == INSECT || type == ARACHNID);
}
//*********************************************************************
//                      isHumanoidLike
//*********************************************************************
bool monType::isHumanoidLike(mType type) {
    return(type == HUMANOID || type == GOBLINOID || type == INSECTOID || type == MONSTROUSHUM);
}
//*********************************************************************
//                      isExtraplanar
//*********************************************************************
bool monType::isExtraplanar(mType type) {
    return(type == ASTRAL || type == ETHEREAL || type == DEVA || 
           type == DEMON || type == DEVIL || type == MODRON || type == DAEMON);
}
//*********************************************************************
//                      isMindless
//*********************************************************************
bool monType::isMindless(mType type) {
    return(type == INSECT || type == GOLEM || type == AUTOMATON);
}

bool monType::isImmuneEnchantments(mType type) {

    return(type == GASEOUS || type == ENERGY || type == ETHEREAL ||
           type == ASTRAL || type == INSECT || type == SLIME ||
           type == PUDDING || type == GOLEM || type == AUTOMATON ||
           type == OOZE);
}



//*********************************************************************
//                      size
//*********************************************************************
// can we assume they are a certain size based on their type?

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

Size monType::size(mType type) {
    switch(type) {
    case INSECT:
        return(SIZE_FINE);

    case FISH:
        return(SIZE_TINY);

    case ANIMAL:
    case FAERIE:
    case GOBLINOID:
    case REPTILE:
        return(SIZE_SMALL);

    case ARACHNID:
    case BEAST:
    case DIREANIMAL:
    case HUMANOID:
    case INSECTOID:
    case MAGICALBEAST:
    case MONSTROUSHUM:
    case UNDEAD:
        return(SIZE_MEDIUM);

    case AUTOMATON:
    case DEMON:
    case DEVA:
    case DAEMON:
    case DEVIL:
    case ELEMENTAL:
    case GIANTKIN:
    case GOLEM:
    case MODRON:
        return(SIZE_LARGE);

    case DINOSAUR:
        return(SIZE_HUGE);

    case DRAGON:
        return(SIZE_GARGANTUAN);

    // we can't really say much about the rest
    default:
        return(NO_SIZE);
    }
}

//*********************************************************************
//                      immuneCriticals
//*********************************************************************

bool monType::immuneCriticals(mType type) {
    switch(type) {
    case AUTOMATON:
    case GOLEM:
    case ETHEREAL:
    case GASEOUS:
    case ENERGY:
    case ELEMENTAL:
    case PUDDING:
    case SLIME:
    case OOZE:
        return(true);

    default:
        return(false);
    }
}

//*********************************************************************
//                      getName
//*********************************************************************

char *monType::getName(mType type) {
    // do bounds checking
    return(mobtype_name[(int)type]);
}

//*********************************************************************
//                      getHitdice
//*********************************************************************

int monType::getHitdice(mType type) {
    return(mob_hitdice[(int)type]);
}
