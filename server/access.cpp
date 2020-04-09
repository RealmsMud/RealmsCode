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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "config.hpp"
#include "calendar.hpp"
#include "creatures.hpp"
#include "deityData.hpp"
#include "mud.hpp"
#include "playerClass.hpp"
#include "playerTitle.hpp"

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

unsigned int permAC[30] = {
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
    "^b",                       // Celestial
    "^c",                       // Kobold
    "^r",                       // Infernal
    "^y",                       // Schnai
    "^r",                       // Kataran
    "^g",                       // Druidic
    "^y",                       // Wolfen
    "^m",                       // Thieves' Cant
    "^m",                       // Arcanic (Mages/Lich)
    "^m",                       // Abyssal
    "^r",                       // Tiefling
};

char language_adj[][32] = { "an alien language", "dwarven", "elven", "halfling", "common", "orcish", "giantkin",
        "gnomish", "trollish", "ogrish", "darkelf", "goblinoid", "minotaur", "celestial", "kobold", "infernal",
        "barbarian", "kataran", "druidic", "wolfen", "thieves' cant", "arcanic", "abyssal", "tiefling" };

char language_verb[][3][24] = {
    // Unknown (alien)
    {"blab", "mysteriously say", "rapidly gibber"},
    // Dwarven
    {"mutter", "utter", "grumble"},
    // Elven
    {"say", "speak", "eloquently say"},
    // Halfling
    {"say", "speak", "utter"},
    // Common
    {"say", "speak", "say"},
    // Orcish
    {"grunt", "squeal", "snort"},
    // Giantkin
    {"boom", "speak", "say"},
    // Gnomish
    {"say", "speak", "utter"},
    // Trollish*/
    {"snarl", "spit", "gutterly cough"},
    // Ogrish
    {"grunt", "snarl", "boom"},
    // Dark elven
    {"snide", "sneer", "rapidly speak"},
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
    {"jabber", "spit", "snarl"}
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
bstring intToText(int nNumber, bool cap) {
    bstring toReturn;

    // check for array bounds
    if(nNumber < 31 && nNumber >= 0) {
        toReturn = number[nNumber];
    } else {
        toReturn = bstring(nNumber);
    }

    if(cap)
        toReturn[0] = up(toReturn[0]);
    return(toReturn);
}

bstring getOrdinal(int num) {
    bstring ordinal;
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
    nIndex = MAX( 0, MIN(nIndex, 29 ) );

    return(permAC[nIndex]);
}
//*********************************************************************
//          get_class_string()
//*********************************************************************
char *get_class_string(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < static_cast<int>(CreatureClass::CLASS_COUNT) );

    nIndex = MAX( 0, MIN(nIndex, static_cast<int>(CreatureClass::CLASS_COUNT) - 1 ) );

    return(class_str[nIndex]);
}

char* get_lang_color(int nIndex) {
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < LANGUAGE_COUNT );

    nIndex = MAX( 0, MIN(nIndex, LANGUAGE_COUNT-1 ) );

    return(lang_color[nIndex]);
}

char *get_language_adj(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < LANGUAGE_COUNT );

    nIndex = MAX( 0, MIN(nIndex, LANGUAGE_COUNT - 1 ) );

    return(language_adj[nIndex]);
}

// language verb

char *get_language_verb(int lang) {
    int num=0;

    ASSERTLOG( lang >= 0 );
    ASSERTLOG( lang < LANGUAGE_COUNT );

    num = Random::get(1,3);
    lang = MAX(0, MIN(lang, LANGUAGE_COUNT-1));

    return(language_verb[lang][num-1]);
}

//*********************************************************************
//          get_skill_string()
//*********************************************************************

char *get_skill_string(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < 11 );

    nIndex = MAX( 0, MIN(nIndex, 10) );

    return(mob_skill_str[nIndex]);
}
char *get_save_string(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < MAX_SAVE_COLOR );

    nIndex = MAX( 0, MIN(nIndex, MAX_SAVE_COLOR-1 ) );

    return(save_str[nIndex]);
}

char get_save_color(int nIndex) {
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < MAX_SAVE_COLOR );

    nIndex = MAX( 0, MIN(nIndex, MAX_SAVE_COLOR-1 ) );

    return(save_color[nIndex]);
}

char *getClassAbbrev(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < static_cast<int>(CreatureClass::CLASS_COUNT) );

    nIndex = MAX( 0, MIN(nIndex, static_cast<int>(CreatureClass::CLASS_COUNT) - 1 ) );

    return(class_abbrev[nIndex] );
}

char *getClassName(Player* player) {
    static char classname[1024];

    if(!player->hasSecondClass())
        return(get_class_string( static_cast<int>(player->getClass())));

    strcpy(classname, "");
    strcpy(classname, getShortClassAbbrev( player->getClassInt() ) );
    strcat(classname, "/");
    strcat(classname, getShortClassAbbrev( player->getSecondClassInt() ) );

    return(classname);
}


char *getShortClassAbbrev(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < static_cast<int>(CreatureClass::CLASS_COUNT) );

    nIndex = MAX( 0, MIN(nIndex, static_cast<int>(CreatureClass::CLASS_COUNT) - 1 ) );

    return(shortClassAbbrev[nIndex-1] );
}

char *getShortClassName(const Player* player) {
    static char classname[1024];

    if(!player->hasSecondClass())
        return(get_class_string(static_cast<int>(player->getClass())));

    strcpy(classname, "");
    strcpy(classname, getShortClassAbbrev(player->getClassInt()));
    strcat(classname, "/");
    strcat(classname, getShortClassAbbrev(player->getSecondClassInt()));

    return(classname);
}


//*********************************************************************
//                      int_to_test()
//*********************************************************************
char *int_to_text(int nNumber) {
    static char strNum[15];
    char *strReturn;

    // check for array bounds
    if(nNumber < 31 && nNumber >= 0) {
        strReturn = number[nNumber];
    } else {
        sprintf(strNum, "%d", nNumber );
        strReturn = strNum;
    }

    return(strReturn);
}

//*********************************************************************
//                      get_quest_exp()
//*********************************************************************
long get_quest_exp(int nQuest) {
    // quests are 1 based and this array is zero based
    // so subtract one first
    nQuest--;

    nQuest = MAX(0, MIN(nQuest, numQuests ) );

    return(gConfig->questTable[nQuest]->exp);
}

//*********************************************************************
//                      get_quest_name()
//*********************************************************************
const char *get_quest_name(int nIndex) {
    // do bounds checking
    ASSERTLOG(nIndex >= -1);
    ASSERTLOG(nIndex < 64);

    nIndex = MAX(-1, MIN(nIndex, numQuests));

    if(nIndex==-1)
        return("None");
    if(nIndex >= numQuests)
        return("Unknown");
    else
        return(gConfig->questTable[nIndex]->name);
}

bool isTitle(const bstring& str) {
    std::map<int, PlayerTitle*>::iterator tt;

    std::map<bstring, PlayerClass*>::iterator cIt;
    PlayerClass* pClass=nullptr;
    PlayerTitle* title=nullptr;
    for(cIt = gConfig->classes.begin() ; cIt != gConfig->classes.end() ; cIt++) {
        pClass = (*cIt).second;
        for(tt = pClass->titles.begin() ; tt != pClass->titles.end(); tt++) {
             title = (*tt).second;
             if(str == title->getTitle(true) || str == title->getTitle(false))
                 return(true);
        }
    }

    std::map<int, DeityData*>::iterator it;
    DeityData* data=nullptr;
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

bool isClass(char str[80]) {
    for(int i=1; i<static_cast<int>(CreatureClass::CLASS_COUNT); i++) {
        if(!strcmp(class_str[i], str) || !strcmp(class_abbrev[i], str))
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
