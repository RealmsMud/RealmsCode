/*
 * access.cpp
 *   This file contains the routines necessary to access arrays
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

#include <cstdio>                  // for sprintf
#include <cstring>                 // for strcat, strcpy
#include <map>                     // for operator==, map<>::iterator, map
#include <string>                  // for string, char_traits, to_string
#include <string_view>             // for operator==, basic_string_view, str...
#include <utility>                 // for pair

#include "calendar.hpp"            // for Calendar, cDay
#include "config.hpp"              // for Config, gConfig, DeityDataMap
#include "deityData.hpp"           // for DeityData
#include "global.hpp"              // for CreatureClass, CreatureClass::CLAS...
#include "mudObjects/players.hpp"  // for Player
#include "playerClass.hpp"         // for PlayerClass
#include "playerTitle.hpp"         // for PlayerTitle
#include "proto.hpp"               // for getLastDigit, ltoa, up, getClassAb...
#include "random.hpp"              // for Random

//
//      class
//
char class_str[][16] = { "None", "Assassin", "Berserker", "Cleric", "Fighter", "Mage", "Paladin", "Ranger", "Thief",
        "Pureblood", "Monk", "Death Knight", "Druid", "Lich", "Werewolf", "Bard", "Rogue", "Builder", "Unused",
        "Caretaker", "Dungeonmaster" };

char class_abbrev[][8] = { "None", "Assn", "Bers", "Cleric", "Ftr", "Mage", "Pal", "Rang", "Thief", "Vamp", "Monk", "Dknght",
        "Druid", "Lich", "Were", "Bard", "Rogue", "Build", "Crt", "CT", "DM" };

char shortClassAbbrev[][8] = { "A", "Be", "Cl", "F", "M", "P", "R", "T", "Va", "Mo", "Dk", "Dr", "L", "W", "Bd", "Ro",
        "Bu", "Cr", "CT", "DM" };



//
//      mobs
//
char mob_skill_str[][16] = { "Horrible", "Poor", "Fair", "Decent", "Average", "Talented", "Very Good", "Exceptional",
        "Master", "Grand Master", "Godlike" };

int permAC[30] = {
        25, 70, 110, 155, 200, 245, 290, 335, 380, 425, 470, 510, 535, 560, 580, 600, 620, 645,
        670, 690, 710, 730, 760, 780, 820, 870, 890, 910, 960, 1000 };


//
//      language
//

char lang_color[LANGUAGE_COUNT][3] = {
    "^m",                       // Alien
    "^c",                       // Dwarven
    "^g",                       // Elven
    "^c",                       // Halfling
    "^c",                       // Common
    "^y",                       // Orcish
    "^y",                       // Giantkin
    "^b",                       // Gnomish
    "^g",                       // Trollish
    "^y",                       // Ogrish
    "^m",                       // Dark-Elven (Drow)
    "^g",                       // Goblinoid
    "^y",                       // Minotaur
    "^B",                       // Celestial
    "^c",                       // Kobold
    "^R",                       // Infernal
    "^y",                       // Schnai
    "^r",                       // Kataran
    "^g",                       // Druidic
    "^y",                       // Wolfen
    "^m",                       // Thieves' Cant
    "^m",                       // Arcanic (Mages/Lich)
    "^m",                       // Abyssal
    "^r",                       // Tiefling
    "^y",                       // Kenku
    "^M",                       // Fey
    "^g",                       // Lizardman
    "^y",                       // Centaur
    "^D",                       // Duergar
    "^Y",                       // Gnoll
    "^y",                       // Bugbear
    "^G",                       // Hobgoblin
    "^c",                       // Brownie
    "^Y",                       // Firbolg
    "^r",                       // Satyr
    "^m",                       // Quickling

};

char language_adj[][32] = { "an alien language", "dwarven", "elven", "halfling", "common", "orcish", "giantkin",
        "gnomish", "trollish", "ogrish", "darkelf", "goblinoid", "minotaur", "celestial", "kobold", "infernal",
        "barbarian", "kataran", "druidic", "wolfen", "thieves' cant", "arcanic", "abyssal", "tiefling", "kenku",
        "fey", "lizardman", "centaur", "duergar", "gnoll", "bugbear", "hobgoblin", "brownie", "firbolg", "satyr", "quickling"};

char language_verb[][3][24] = {
    // Unknown (alien)
    {"blab", "mysteriously say", "rapidly gibber"},
    // Dwarven
    {"mutter", "utter", "grumble"},
    // Elven
    {"say", "speak", "lecture"},
    // Halfling
    {"say", "speak", "utter"},
    // Common
    {"say", "speak", "chat"},
    // Orcish
    {"grunt", "squeal", "snort"},
    // Giantkin
    {"boom", "speak", "bellow"},
    // Gnomish
    {"say", "speak", "utter"},
    // Trollish*/
    {"snarl", "spit", "gutterly cough"},
    // Ogrish
    {"grunt", "snarl", "boom"},
    // Dark elven
    {"scoff", "sneer", "preen"},
    // Goblinoid
    {"sputter", "snort", "cough"},
    // Minotaur
    {"snort", "grunt", "speak"},
    // Celestial
    {"speak", "sing", "eloquently speak"},
    // Kobold
    {"bark", "snort", "growl"},
    // Infernal
    {"snarl", "growl", "gutterly speak"},
    // Schnai
    {"growl", "snarl", "sneer"},
    // Kataran
    {"purr", "growl", "spit"},
    // Druidic
    {"mutter", "say", "sound"},
    // Wolfen
    {"loudly bark", "growl", "bay"},
    // Thieves' Cant
    {"gesture", "allude", "mumble"},
    // Arcanic
    {"gibber", "quickly speak", "chatter"},
    // Abyssal
    {"growl", "snarl", "cough"},
    // Tiefling
    {"jabber", "spit", "snarl"},
    //Kenku
    {"squawk", "cluck", "screech"},
    //Fey
    {"sing", "whistle", "buzz"},
    //Lizardman
    {"hiss", "spit", "whisper"},
    //Centaur
    {"bray", "boom", "whinny"},
    //Duergar
    {"growl", "spit", "grumble"},
    //Gnoll
    {"yip", "bark", "growl"},
    //Bugbear
    {"gnarl", "grunt", "snarl"},
    //Hobgoblin
    {"snort", "stutter", "cough"},
    //Brownie
    {"squeak", "scritch", "shrill"},
    //Firbolg
    {"grunt", "bellow", "heavily cough"},
    //Satyr
    {"groan", "gutterly growl", "grunt"},
    //Quickling
    {"sputter", "rapidly squeak", "jabber"}

};



//
//      other
//
char save_str[MAX_SAVE_COLOR][20] = { "Normal", "Good", "Above Average", "Resistant", "Excellent", "Incredible",
        "Amazing", "Superb", "Impervious", "Unbelievable", "Immune" };

char save_color[MAX_SAVE_COLOR] = { 'g', 'c', 'r', 'y', 'm', 'B', 'G', 'C', 'R', 'Y', 'M'};

    // Jakar
    //{ "Tinkerer", "Haggler", "Inventor", "Merchant Priest", "Divine Trademaster",
    //  "Emerald Bishop", "Diamond Patriarch", "Golden Cardinal", "25-27","28-30"},

    // Jakar
    //{ "Tinkerer", "Haggler", "Inventor", "Merchant Priestess", "Divine Trademistress",
    //  "Emerald Prelate", "Diamond Matriarch", "Golden Lama", "25-27","28-30"}

char scrollDesc [][10][20] =
{
    // Earth Realm
    {"dirty", "earthen", "sandy", "dusty", "terran", "hardened", "unbreakable", "petrified", "metallic", "earthy"},
    // Air Realm
    {"whistling", "airy", "translucent", "whisping", "windy", "transparent", "invisible", "flying", "soaring", "breathing"},
    // Fire Realm
    {"scorched", "burning", "red-hot", "blackened", "burnt", "heated", "magma", "firey", "seething", "white-hot"},
    // Water Realm
    {"watery", "waterlogged", "waterproof", "wet", "misty", "sea blue", "azurite", "seaweed", "saltwater", "coral"},
    // Elec Realm
    {"crackling", "electric", "magnetic", "shocking", "electrostatic", "copper", "conductive", "sparking", "bipolar", "ozone"},
    // Cold Realm
    {"freezing", "frosty", "icy", "arctic", "icicled", "dry ice", "frozen", "sublimating", "melting", "ice blue"}
};

char scrollType [][2][20] =
{
    // level 1
    {"parchment","papyrus"},
    // level 2
    {"scroll","journal"},
    // level 3
    {"tablet","spellbook"},
    // level 4
    {"folio","volume"},
    // level 5
    {"cuneiform tablet","tome"},
    // level 6
    {"runed cloth","manual"}
};




static char number[][15] = { "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
        "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen",
        "twenty", "twenty-one", "twenty-two", "twenty-three", "twenty-four", "twenty-five", "twenty-six",
        "twenty-seven", "twenty-eight", "twenty-nine", "thirty" };
//*********************************************************************
//          int_to_test()
//*********************************************************************
std::string intToText(int nNumber, bool cap) {
    std::string toReturn;

    // check for array bounds
    if(nNumber < 31 && nNumber >= 0) {
        toReturn = number[nNumber];
    } else {
        toReturn = std::to_string(nNumber);
    }

    if(cap)
        toReturn[0] = up(toReturn[0]);
    return(toReturn);
}

std::string getOrdinal(int num) {
    std::string ordinal;
    char buff[32];
    int last=0;

    ltoa(num, buff, 10);
    ordinal = buff;

    last = getLastDigit(num, 2);

    if(last > 10&& last < 14)
        ordinal += "th";
    else {
        last = getLastDigit(num, 1);

        switch (last) {
        case 1:
            ordinal += "st";
            break;
        case 2:
            ordinal += "nd";
            break;
        case 3:
            ordinal += "rd";
            break;
        default:
            ordinal += "th";
            break;
        }
    }
    return(ordinal);
}

int get_perm_ac(int nIndex) {
    nIndex = std::max( 0, std::min(nIndex, 29 ) );

    return(permAC[nIndex]);
}
//*********************************************************************
//          get_class_string()
//*********************************************************************
char *get_class_string(int nIndex) {
    // do bounds checking
    nIndex = std::max( 0, std::min(nIndex, static_cast<int>(CreatureClass::CLASS_COUNT) - 1 ) );

    return(class_str[nIndex]);
}

char* get_lang_color(int nIndex) {
    nIndex = std::max( 0, std::min(nIndex, LANGUAGE_COUNT-1 ) );

    return(lang_color[nIndex]);
}

char *get_language_adj(int nIndex) {
    // do bounds checking
    nIndex = std::max( 0, std::min(nIndex, LANGUAGE_COUNT - 1 ) );

    return(language_adj[nIndex]);
}

// language verb

char *get_language_verb(int lang) {
    int num;

    num = Random::get(1,3);
    lang = std::max(0, std::min(lang, LANGUAGE_COUNT-1));

    return(language_verb[lang][num-1]);
}

//*********************************************************************
//          get_skill_string()
//*********************************************************************

char *get_skill_string(int nIndex) {
    // do bounds checking
    nIndex = std::max( 0, std::min(nIndex, 10) );

    return(mob_skill_str[nIndex]);
}
char *get_save_string(int nIndex) {
    // do bounds checking
    nIndex = std::max( 0, std::min(nIndex, MAX_SAVE_COLOR-1 ) );

    return(save_str[nIndex]);
}

char get_save_color(int nIndex) {
    nIndex = std::max( 0, std::min(nIndex, MAX_SAVE_COLOR-1 ) );

    return(save_color[nIndex]);
}

char *getClassAbbrev(int nIndex) {
    // do bounds checking
    nIndex = std::max( 0, std::min(nIndex, static_cast<int>(CreatureClass::CLASS_COUNT) - 1 ) );

    return(class_abbrev[nIndex] );
}

std::string getFullClassName(CreatureClass cClass, CreatureClass secondClass) {
    if (secondClass == CreatureClass::NONE) {
        return(get_class_string( static_cast<int>(cClass)));
    }
    std::ostringstream oStr;
    oStr <<  get_class_string(static_cast<int>(cClass) ) << "/" << get_class_string( static_cast<int>(secondClass) );
    return (oStr.str());

}
std::string getClassName(CreatureClass cClass, CreatureClass secondClass) {
    if (secondClass == CreatureClass::NONE) {
        return(get_class_string( static_cast<int>(cClass)));
    }
    std::ostringstream oStr;
    oStr <<  getShortClassAbbrev(static_cast<int>(cClass) ) << "/" << getShortClassAbbrev( static_cast<int>(secondClass) );
    return (oStr.str());

}

std::string getClassName(const std::shared_ptr<Player>& player) {
    return getClassName(player->getClass(), player->getSecondClass());
}


char *getShortClassAbbrev(int nIndex) {
    // do bounds checking
    nIndex = std::max( 0, std::min(nIndex, static_cast<int>(CreatureClass::CLASS_COUNT) - 1 ) );

    return(shortClassAbbrev[nIndex-1] );
}

std::string getShortClassName(const std::shared_ptr<const Player> &player) {
    if(!player->hasSecondClass())
        return(get_class_string(static_cast<int>(player->getClass())));

    std::ostringstream oStr;
    oStr << getShortClassAbbrev(player->getClassInt()) << "/" << getShortClassAbbrev(player->getSecondClassInt());

    return(oStr.str());
}


//*********************************************************************
//                      int_to_test()
//*********************************************************************
std::string int_to_text(int nNumber) {
    // check for array bounds
    if(nNumber < 31 && nNumber >= 0)
        return number[nNumber];
    else
        return std::to_string(nNumber);
}

bool isTitle(std::string_view str) {
    std::map<int, PlayerTitle*>::iterator tt;

    std::map<std::string, PlayerClass*>::iterator cIt;
    PlayerClass* pClass;
    PlayerTitle* title;
    for(cIt = gConfig->classes.begin() ; cIt != gConfig->classes.end() ; cIt++) {
        pClass = (*cIt).second;
        for(tt = pClass->titles.begin() ; tt != pClass->titles.end(); tt++) {
             title = (*tt).second;
             if(str == title->getTitle(true) || str == title->getTitle(false))
                 return(true);
        }
    }

    DeityDataMap::iterator it;
    DeityData* data;
    for(it = gConfig->deities.begin() ; it != gConfig->deities.end() ; it++) {
        data = (*it).second;
        for(tt = data->titles.begin() ; tt != data->titles.end(); tt++) {
             title = (*tt).second;
             if(str == title->getTitle(true) || str == title->getTitle(false))
                 return(true);
        }
    }

    return(false);
}

bool isClass(std::string_view str) {
    for(int i=1; i<static_cast<int>(CreatureClass::CLASS_COUNT); i++) {
        if(!str.compare(class_str[i]) || !str.compare(class_abbrev[i]))
            return(true);
    }

    return(false);
}

//*********************************************************************
//                      getAge
//*********************************************************************

int Player::getAge() const {
    const Calendar* calendar = gConfig->getCalendar();
    int age=0;

    if(birthday) {
        age = calendar->getCurYear() - birthday->getYear();
        if(calendar->getCurMonth() < birthday->getMonth())
            age--;
        if(calendar->getCurMonth() == birthday->getMonth() && calendar->getCurDay() < birthday->getDay())
            age--;
    }

    return(age);
}
