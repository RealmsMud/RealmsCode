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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "calendar.h"


// Stats

char stat_names[][4] = { "STR", "DEX", "CON", "INT", "PTY", "CHA" };

char* getStatName(int stat) {
    stat = tMIN<int>(tMAX<int>(stat - 1, 0), MAX_STAT);
    return(stat_names[stat]);
}

char save_names[][4] = { "LCK", "POI", "DEA", "BRE", "MEN", "SPL" };

char* getSaveName(int save) {
    save = tMIN<int>(tMAX<int>(save, 0), MAX_SAVE-1);
    return(save_names[save]);
}

//
//      object
//

char object_type[][20] = { "error", "error", "error", "instrument", "herb",
        "weapon", "piece of armor", "potion", "scroll", "magic wand", "container", "money", "key",
        "light source", "miscellaneous item", "song scroll", "poison", "bandage", "ammo", "quiver", "lottery ticket",
        "unknown item" };

//
//      class
//
char class_str[][16] = { "None", "Assassin", "Berserker", "Cleric", "Fighter", "Mage", "Paladin", "Ranger", "Thief",
        "Pureblood", "Monk", "Death Knight", "Druid", "Lich", "Werewolf", "Bard", "Rogue", "Builder", "Unused",
        "Caretaker", "Dungeonmaster" };

char class_abbrev[][8] = { "Assn", "Bers", "Cleric", "Ftr", "Mage", "Pal", "Rang", "Thief", "Vamp", "Monk", "Dknght",
        "Druid", "Lich", "Were", "Bard", "Rogue", "Build", "Crt", "CT", "DM" };

char shortClassAbbrev[][8] = { "A", "Be", "Cl", "F", "M", "P", "R", "T", "Va", "Mo", "Dk", "Dr", "L", "W", "Bd", "Ro",
        "Bu", "Cr", "CT", "DM" };



//
//      mobs
//
char mob_trade_str[][16] = { "None", "Smithy", "Banker", "Armorer", "Weaponsmith", "Merchant", "Training Perm" };

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
    ASSERTLOG( nIndex < CLASS_COUNT );

    nIndex = MAX( 0, MIN(nIndex, CLASS_COUNT - 1 ) );

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

    num = mrand(1,3);
    lang = MAX(0, MIN(lang, LANGUAGE_COUNT-1));

    return(language_verb[lang][num-1]);
}



//*********************************************************************
//          get_trade_string()
//*********************************************************************

char *get_trade_string(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < MOBTRADE_COUNT );

    nIndex = MAX( 0, MIN(nIndex, MOBTRADE_COUNT) );
    return(mob_trade_str[nIndex]);

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

const char *get_mflag(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < MAX_MONSTER_FLAGS+1 );

    nIndex = MAX( 0, MIN(nIndex, MAX_MONSTER_FLAGS ) );

    // flags are offset by 1
    nIndex++;
    return(gConfig->mflags[nIndex].name.c_str());
}

const char *get_pflag(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < MAX_PLAYER_FLAGS+1 );

    nIndex = MAX( 0, MIN(nIndex, MAX_PLAYER_FLAGS ) );

    // flags are offset by 1
    nIndex++;
    return(gConfig->pflags[nIndex].name.c_str());
}

const char *get_rflag(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < MAX_ROOM_FLAGS+1 );

    nIndex = MAX( 0, MIN(nIndex, MAX_ROOM_FLAGS ) );

    // flags are offset by 1
    nIndex++;
    return(gConfig->rflags[nIndex].name.c_str());
}

const char *get_xflag(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < MAX_EXIT_FLAGS+1 );

    nIndex = MAX( 0, MIN(nIndex, MAX_EXIT_FLAGS ) );

    // flags are offset by 1
    nIndex++;
    return(gConfig->xflags[nIndex].name.c_str());
}
const char *get_oflag(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < MAX_OBJECT_FLAGS+1 );

    nIndex = MAX( 0, MIN(nIndex, MAX_OBJECT_FLAGS ) );

    // flags are offset by 1
    nIndex++;
    return(gConfig->oflags[nIndex].name.c_str());
}

char *getClassAbbrev(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < CLASS_COUNT );

    nIndex = MAX( 0, MIN(nIndex, CLASS_COUNT - 1 ) );

    return(class_abbrev[nIndex-1] );
}

char *getClassName(Player* player) {
    static char classname[1024];

    if(!player->getSecondClass())
        return(get_class_string( player->getClass()));

    strcpy(classname, "");
    strcpy(classname, getShortClassAbbrev( player->getClass() ) );
    strcat(classname, "/");
    strcat(classname, getShortClassAbbrev( player->getSecondClass() ) );

    return(classname);
}


char *getShortClassAbbrev(int nIndex) {
    // do bounds checking
    ASSERTLOG( nIndex >= 0 );
    ASSERTLOG( nIndex < CLASS_COUNT );

    nIndex = MAX( 0, MIN(nIndex, CLASS_COUNT - 1 ) );

    return(shortClassAbbrev[nIndex-1] );
}

char *getShortClassName(const Player* player) {
    static char classname[1024];

    if(!player->getSecondClass())
        return(get_class_string(player->getClass()));

    strcpy(classname, "");
    strcpy(classname, getShortClassAbbrev(player->getClass()));
    strcat(classname, "/");
    strcat(classname, getShortClassAbbrev(player->getSecondClass()));

    return(classname);
}

//*********************************************************************
//                      getTitle
//*********************************************************************
// This function returns a string with the player's character title in
// it.  The title is determined by looking at the player's class and
// level.

bstring Player::getTitle() const {
    std::ostringstream titleStr;

    int titlnum;

    if(title == "" || title == " " || title == "[custom]") {
        titlnum = (level - 1) / 3;
        if(titlnum > 9)
            titlnum = 9;

        if(cClass == CLERIC)
            titleStr << gConfig->getDeity(deity)->getTitle(level, getSex() == SEX_MALE);
        else
            titleStr << gConfig->classes[get_class_string(cClass)]->getTitle(level, getSex() == SEX_MALE);

        if(cClass2) {
            titleStr <<  "/";

            if(cClass2 == CLERIC)
                titleStr << gConfig->getDeity(deity)->getTitle(level, getSex() == SEX_MALE);
            else
                titleStr << gConfig->classes[get_class_string(cClass2)]->getTitle(level, getSex() == SEX_MALE);
        }

        return(titleStr.str());

    } else {
        return(title);
    }
}

bstring Player::getCustomTitle() const { return(title); }
bstring Player::getTempTitle() const { return(tempTitle); }

bool Player::canChooseCustomTitle() const {
    const std::map<int, PlayerTitle*>* titles=0;
    std::map<int, PlayerTitle*>::const_iterator it;
    const PlayerTitle* title=0;

    if(cClass == CLERIC)
        titles = &(gConfig->getDeity(deity)->titles);
    else
        titles = &(gConfig->classes[get_class_string(cClass)]->titles);

    it = titles->find(level);
    if(it == titles->end())
        return(false);

    title = (*it).second;
    return(title && title->getTitle(getSex() == SEX_MALE) == "[custom]");
}

void Player::setTitle(bstring newTitle) { title = newTitle; }
void Player::setTempTitle(bstring newTitle) { tempTitle = newTitle; }

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

//*********************************************************************
//                      obj_type
//*********************************************************************
char *obj_type(int nType) {
    if(nType < 0)
        nType = 0;
    if(nType > MAX_OBJ_TYPE)
        nType = MAX_OBJ_TYPE;
    return(object_type[nType]);
}

char conjureTitles[][3][10][30] = {
// earth
        {
        // weak
                { "lesser earth elemental", "marble mongoose", "rock ape", "earth spider", "giant badger",
                        "steel snake", "golden fox", "giant rock worm", "glassteel golem", "earth demon" },
                // normal
                { "giant mole", "obsidian worm", "mud sloth", "crystal wolf", "earth elemental", "granite elephant",
                        "iron sentinel", "greater earth elemental", "steel tiger", "adamantium tiger" },
                // buff
                { "pet rock", "sandman", "rock wolverine", "rock demon", "marble tiger", "earth devil",
                        "crystal sentinel", "galeb duhr", "steel hydra", "adamantium dragon" } },
        // air
        {
        // weak
                { "lesser air elemental", "wind snake", "wind rat", "zephyr cat", "wind imp", "aerial tiger",
                        "cloud ogre", "silver eagle", "cloud giant", "winged elf lord" },
                // normal
                { "giant wasp", "aerial vortex", "air sentinel", "winged cobra", "air elemental", "aerial stalker",
                        "wind devil", "greater air elemental", "cloud drake", "djinn air lord" },
                // buff
                { "wind bat", "baby griffon", "dust devil", "winged elf", "winged unicorn", "black pegasus",
                        "djinn", "jade falcon", "ki-rin", "cloud giant wizard" } },
        // fire
        {
        // weak
                { "lesser fire elemental", "giant firefly", "lesser fire demon", "fire mephit", "fire cat",
                        "salamander", "firey skeleton warrior", "crimson lion", "fire drake", "lava raptor" },
                // normal
                { "fire sphere", "fire snake", "fire beetle", "flame spider", "fire elemental", "firehawk",
                        "fire angel", "greater fire elemental", "fire kraken", "phoenix" },
                // buff
                { "burning bush", "fire asp", "flame sprite", "ruby serpent", "crimson iguana", "brimstone demon",
                        "efretti", "horned fire devil", "fire giant shaman", "venerable red dragon" } },
        // water
        {
        // weak
                { "lesser water elemental", "aquatic elf", "acidic blob", "vapor rat", "water weird", "fog beast",
                        "white crocodile", "steam jaguar", "water-logged troll", "bronze dragon" },
                // normal
                { "giant frog", "water imp", "mist devil", "water scorpion", "water elemental", "steam spider",
                        "blood elemental", "greater water elemental", "acid devil", "water elemental lord" },
                // buff
                { "mist rat", "creeping fog", "water mephit", "mist spider", "carp dragon", "giant water slug",
                        "giant squid", "mist dragon", "kraken", "sea titan" } },
        // electricity
        {
        // weak
                { "lesser lightning elemental", "lightning ball", "storm cloud", "lightning imp", "crackling mephit",
                        "crackling orb", "storm mephit", "storm eagle", "storm sentinel", "greater lightning elemental" },
                // normal
                { "spark", "shocker lizard", "thunder hawk", "electric eel", "lightning demon",
                        "shocker imp", "shocking salamander", "thunderbolt", "storm giant", "storm giant shaman" },
                // buff
                { "static ball", "lightning serpent", "lightning devil", "thundercloud", "thunder cat",
                        "lightning djinn", "crackling roc", "young blue dragon", "blue dragon", "venerable blue dragon" } },
        // cold
        {
        // weak
                { "lesser ice elemental", "snow storm", "polar bear cub", "frost revenant", "swirling blizzard",
                        "frozen beast", "snow witch", "crystalline golem", "ice troll", "greater ice elemental" },
                // normal
                { "ice rat", "snowman", "snow eagle", "winter mephit", "ice elemental",
                        "glacier wolf", "frozen guardian", "white remorhaz", "frost giant shaman", "frost giant lord" },
                // buff
                { "winter fox", "winter wolf cub", "ice mephit", "polar bear", "frozen yeti",
                        "winter wolf", "abominable snowman", "young white dragon", "white dragon", "venerable white dragon" } } };

char bardConjureTitles[][10][35] = { { "dancing bunny rabbit", "ebony squirrel", "bearded lady", "blue chimpanzee",
        "singing sword", "redskin wild-elf", "mechanical dwarf", "psychotic animated cello", "clockwork wyrm",
        "untuned rabid barisax" },
        { "talking sewer rat", "alley cat", "chattering tea pot", "ivory jaguar", "invisible entity",
                "singing velociraptor", "cackling scarecrow", "shade", "miming storm giant", "googleplex" },

        { "angry teddy bear", "red spider monkey", "wild-eyed poet", "psychotic hand puppet",
                "schizophrenic serial killer clown", "sabre-tooth gnome", "albino dark-elf", "headless horseman",
                "rampaging bass drum", "vorpal bunny" } };

char mageConjureTitles[][10][35] = { { "large crow", "unseen servant", "jade rat", "guardian daemon", "marble owl",
        "chasme demon", "bone devil", "balor", "devil princess", "daemon overlord" },

{ "large black beetle", "raven", "ebony cat", "quasit", "iron cobra", "shadow daemon", "succubus", "large blue demon",
        "ultradaemon", "hellfire darklord" }, { "white shrew", "red-blue tarantula", "azurite hawk", "fiendish imp",
        "pseudo dragon", "styx devil", "glabrezu demon", "pit fiend", "demon prince", "abyssal lord" } };

bool isTitle(bstring str) {
    std::map<int, PlayerTitle*>::iterator tt;

    std::map<bstring, PlayerClass*>::iterator cIt;
    PlayerClass* pClass=0;
    PlayerTitle* title=0;
    for(cIt = gConfig->classes.begin() ; cIt != gConfig->classes.end() ; cIt++) {
        pClass = (*cIt).second;
        for(tt = pClass->titles.begin() ; tt != pClass->titles.end(); tt++) {
             title = (*tt).second;
             if(str == title->getTitle(true) || str == title->getTitle(false))
                 return(true);
        }
    }

    std::map<int, DeityData*>::iterator it;
    DeityData* data=0;
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
    for(int i=1; i<CLASS_COUNT; i++) {
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
