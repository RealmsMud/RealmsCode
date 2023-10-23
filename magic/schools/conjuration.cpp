/*
 * conjuration.cpp
 *   Conjuration spells
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

#include <strings.h>                 // for strncasecmp
#include <cstring>                   // for strncpy, strtok, strlen
#include <ctime>                     // for time, time_t
#include <string>                    // for allocator, string
#include <type_traits>               // for enable_if<>::type

#include "cmd.hpp"                   // for cmd
#include "color.hpp"                 // for stripColor
#include "dice.hpp"                  // for Dice
#include "effects.hpp"               // for EffectInfo, Effect
#include "flags.hpp"                 // for M_PLUS_TWO, M_ENCHANTED_WEAPONS_...
#include "global.hpp"                // for CreatureClass, CastType, CastTyp...
#include "lasttime.hpp"              // for lasttime
#include "magic.hpp"                 // for SpellData, CONJURATION, NO_DOMAIN
#include "monType.hpp"               // for MONSTER
#include "mud.hpp"                   // for LT_INVOKE, LT_SPELL, LT_TICK
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/exits.hpp"      // for Exit
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/objects.hpp"    // for Object
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for broadcast, findExit, bonus, getO...
#include "random.hpp"                // for Random
#include "realm.hpp"                 // for Realm, EARTH, MAX_REALM, COLD, ELEC
#include "server.hpp"                // for Server, gServer
#include "stats.hpp"                 // for Stat
#include "structs.hpp"               // for creatureStats


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
                  "winter wolf", "abominable snowman", "young white dragon", "white dragon", "venerable white dragon" } }
};

char bardConjureTitles[][10][35] = {
        { "dancing bunny rabbit", "ebony squirrel", "bearded lady", "blue chimpanzee",
          "singing sword", "redskin wild-elf", "mechanical dwarf", "psychotic animated cello", "clockwork wyrm", "untuned rabid barisax" },
        { "talking sewer rat", "alley cat", "chattering tea pot", "ivory jaguar", "invisible entity",
          "singing velociraptor", "cackling scarecrow", "shade", "miming storm giant", "googleplex" },
        { "angry teddy bear", "red spider monkey", "wild-eyed poet", "psychotic hand puppet",
          "schizophrenic serial killer clown", "sabre-tooth gnome", "albino dark-elf", "headless horseman",
          "rampaging bass drum", "vorpal bunny" }
};

char mageConjureTitles[][10][35] = {
        { "large crow", "unseen servant", "jade rat", "guardian daemon", "marble owl",
          "chasme demon", "bone devil", "balor", "devil princess", "daemon overlord" },

        { "large black beetle", "raven", "ebony cat", "quasit", "iron cobra", "shadow daemon", "succubus", "large blue demon",
          "ultradaemon", "hellfire darklord" },

        { "white shrew", "red-blue tarantula", "azurite hawk", "fiendish imp",
          "pseudo dragon", "styx devil", "glabrezu demon", "pit fiend", "demon prince", "abyssal lord" }
};

//typedef struct {
//  short   hp;
//  short   mp;
//  short   armor;
//  short   thaco;
//  short   ndice;
//  short   sdice;
//  short   pdice;
//  short   str;
//  short   dex;
//  short   con;
//  short   intel;
//  short   pie;
//  long    realms;
//} creatureStats;
//
// 0 = weak, 1 = normal, 2 = buff
creatureStats conjureStats[3][40]  =
{
    // Weak
    {
        {25,4,100,21,1,2,0,16,10,12,10,10,0},
        {29,8,110,20,1,6,0,16,10,12,10,10,250},
        {33,12,120,19,1,7,0,16,10,14,10,10,500},
        {37,16,130,18,2,2,2,16,10,15,10,10,750},
        {41,20,140,17,2,4,0,16,10,16,10,10,1250},
        {45,24,150,16,2,4,4,16,10,16,11,10,1500},
        {49,28,160,15,2,3,5,16,10,16,12,10,3000},
        {53,32,170,14,3,4,2,16,10,16,13,10,6000},
        {57,36,180,13,3,5,1,16,10,16,14,12,11250},
        {61,40,190,12,4,4,2,16,10,16,15,12,13250},
        {65,44,200,11,3,6,1,16,10,16,16,12,15000},
        {69,48,210,10,2,9,6,16,10,16,16,13,17500},
        {73,52,220,9,4,4,3,16,10,16,16,14,20000},
        {77,56,230,8,4,5,3,16,10,16,17,14,22500},
        {81,60,240,7,4,5,5,16,10,16,18,16,25000},
        {85,64,250,6,4,5,7,16,10,17,18,17,27500},
        {89,68,260,5,4,5,9,18,10,18,19,18,30000},
        {93,72,270,4,5,6,4,18,10,18,20,18,32500},
        {97,76,280,3,5,6,8,18,10,18,20,19,35000},
        {101,80,290,2,4,7,8,18,10,18,20,19,37500},
        {105,84,300,1,9,5,1,18,10,18,20,19,40000},
        {109,88,310,0,8,5,8,19,10,19,20,19,47500},
        {113,92,320,-1,10,5,2,19,10,19,20,19,55000},
        {117,96,330,-2,10,5,5,19,10,19,20,19,62500},
        {121,100,340,-3,20,3,0,20,10,19,20,20,70000},
        {125,104,350,-4,20,3,2,20,10,19,20,20,77500},
        {129,108,360,-5,13,4,9,20,10,20,20,20,85000},
        {133,112,370,-6,13,5,2,20,10,20,20,20,92500},
        {137,116,380,-7,15,4,6,21,12,20,21,20,100000},
        {141,120,390,-8,16,3,13,21,12,21,21,21,107500},
        {145,124,400,-9,16,3,15,22,13,22,22,22,120000},
        {149,128,410,-10,16,3,17,23,13,22,22,22,130000},
        {153,132,420,-11,16,3,19,23,14,23,22,22,140000},
        {157,136,430,-12,16,3,21,23,14,23,23,23,150000},
        {161,140,440,-13,16,3,23,24,15,23,23,23,165000},
        {165,144,450,-14,16,3,25,24,15,23,24,23,180000},
        {169,148,460,-15,16,3,27,24,16,24,24,24,195000},
        {173,152,470,-16,16,3,29,25,17,24,24,25,210000},
        {177,156,480,-17,16,3,31,25,18,25,25,25,230000},
        {181,160,490,-18,16,3,33,26,19,25,26,25,250000}
    },
    // Normal
    {
        {27,5,200,20,1,4,0,16,10,10,10,10,0},
        {32,10,215,19,2,3,0,17,10,12,10,10,500},
        {37,15,230,18,2,3,2,18,10,14,10,10,1000},
        {42,20,245,17,2,3,2,18,10,15,10,10,1500},
        {47,25,260,16,2,3,4,18,10,16,10,10,2500},
        {52,30,275,15,4,2,4,18,10,17,12,10,3000},
        {57,35,290,14,2,4,6,18,10,18,14,10,6000},
        {62,40,305,13,4,3,4,18,10,18,15,10,12000},
        {67,45,320,12,5,3,3,18,10,18,16,10,22500},
        {72,50,335,11,3,5,5,18,10,18,17,10,26500},
        {77,55,350,10,3,6,4,18,10,18,18,10,30000},
        {82,60,365,9,3,6,6,18,10,18,18,12,35000},
        {87,65,380,8,4,4,8,18,10,18,18,14,40000},
        {92,70,395,7,4,5,8,18,10,18,18,15,45000},
        {97,75,410,6,5,5,5,18,10,18,18,16,50000},
        {102,80,425,5,4,6,8,18,10,18,18,17,55000},
        {107,85,440,4,4,6,9,18,10,18,18,18,60000},
        {112,90,465,3,5,6,8,19,10,19,19,19,65000},
        {117,95,480,2,6,5,10,20,10,20,20,20,70000},
        {122,100,495,1,4,7,10,21,10,20,20,20,75000},
        {127,105,510,0,9,5,4,21,11,21,20,20,80000},
        {132,110,525,-2,8,5,10,21,11,21,21,20,95000},
        {137,115,540,-4,10,5,6,21,11,21,21,21,110000},
        {142,120,555,-7,10,5,8,21,11,22,21,21,125000},
        {147,125,570,-8,15,4,0,21,11,22,22,21,140000},
        {152,130,585,-8,20,3,4,21,11,22,22,21,155000},
        {157,135,600,-8,13,4,12,21,11,22,22,21,170000},
        {162,140,615,-8,13,5,5,22,11,22,23,22,185000},
        {167,145,630,-9,15,4,10,22,11,23,23,22,200000},
        {172,150,645,-10,18,3,16,23,11,23,23,23,215000},
        {177,155,660,-10,18,3,19,23,11,23,23,23,230000},
        {182,160,675,-11,18,3,22,24,13,24,24,23,250000},
        {187,165,690,-12,18,3,25,24,14,24,24,24,275000},
        {192,170,705,-13,18,3,28,25,15,25,25,24,300000},
        {197,175,720,-14,18,3,31,25,16,25,25,25,325000},
        {202,180,735,-15,18,3,34,26,17,26,26,25,350000},
        {207,185,750,-16,18,3,37,26,18,26,26,26,375000},
        {212,190,765,-17,18,3,40,27,19,27,27,26,410000},
        {217,195,780,-18,18,3,43,27,20,27,27,27,450000},
        {222,200,795,-19,18,3,46,28,21,28,28,27,490000}
    },
    // Buff
    {
        {30,6,300,20,2,2,1,16,12,10,10,10,0},
        {36,12,320,19,3,2,0,16,12,10,10,10,1000},
        {42,18,340,18,3,2,3,17,13,12,10,10,2000},
        {48,24,360,16,3,3,3,18,13,14,10,10,3000},
        {54,30,380,15,3,2,6,18,13,15,10,10,5000},
        {60,36,400,14,4,2,7,18,14,16,10,10,6000},
        {66,42,420,13,4,2,7,18,14,17,12,10,12000},
        {72,48,440,12,4,3,6,18,15,18,14,10,24000},
        {78,54,460,11,4,4,4,18,15,18,14,10,45000},
        {84,60,480,10,5,3,4,18,15,18,14,10,53000},
        {90,66,500,9,6,3,3,18,15,18,15,10,60000},
        {96,72,520,8,3,6,8,18,16,18,16,10,70000},
        {102,78,540,7,4,4,11,18,16,18,16,10,80000},
        {108,84,560,6,5,4,9,18,17,18,18,10,90000},
        {114,90,580,5,6,5,6,18,17,18,18,12,100000},
        {120,96,600,4,4,6,10,18,18,18,18,14,110000},
        {126,102,620,3,5,6,10,18,18,18,18,15,120000},
        {132,108,640,2,5,6,10,18,19,18,18,16,130000},
        {138,114,660,1,6,6,10,18,19,18,18,17,140000},
        {144,120,680,0,7,4,11,18,20,18,18,18,150000},
        {150,126,700,-1,9,5,7,18,20,19,19,19,160000},
        {156,132,720,-3,8,5,14,20,21,20,20,20,190000},
        {162,138,740,-5,10,5,9,21,21,20,21,20,220000},
        {168,144,760,-8,10,5,11,22,22,21,22,20,250000},
        {174,150,780,-9,12,5,0,23,23,22,23,21,280000},
        {180,156,800,-9,20,3,6,23,23,22,23,21,310000},
        {186,162,820,-10,13,4,16,23,23,22,23,22,340000},
        {192,168,840,-10,13,5,9,23,23,23,23,23,370000},
        {198,174,860,-10,15,4,14,24,24,24,24,24,400000},
        {204,180,880,-11,15,4,20,25,25,25,25,25,430000},
        {210,186,900,-12,15,4,26,25,25,25,25,25,460000},
        {216,192,920,-13,15,4,32,26,26,26,26,26,500000},
        {222,198,940,-14,15,4,38,26,26,26,26,26,540000},
        {228,204,960,-15,15,4,44,27,27,27,27,27,580000},
        {234,210,980,-16,15,4,50,27,27,27,27,27,625000},
        {240,216,1000,-17,15,4,56,28,28,28,28,28,670000},
        {246,222,1020,-18,15,4,62,28,28,28,28,28,715000},
        {252,228,1040,-19,15,4,68,29,29,29,29,29,760000},
        {258,234,1060,-20,15,4,74,29,29,29,29,29,810000},
        {264,240,1080,-21,15,4,80,30,30,30,30,30,860000}
    }
};




//*********************************************************************
//                      petTalkDesc
//*********************************************************************

void petTalkDesc(const std::shared_ptr<Monster>&  pet, std::shared_ptr<Creature> owner) {
    std::string name = owner->getName(), desc = "";

    if(owner->flagIsSet(P_DM_INVIS))
        name = "Someone";

    desc = "It's wearing a tag that says, '";
    desc += name;
    desc += "'s, hands off!'.";
    pet->setDescription(desc);

    desc = "I serve only ";
    desc += name;
    desc += "!";
    pet->setTalk(desc);
    pet->escapeText();
}

//*********************************************************************
//                      getPetTitle
//*********************************************************************

int getPetTitle(CastType how, int skLevel, bool weaker, bool undead) {
    int title=0, num=0;
    if(how == CastType::CAST || how == CastType::SKILL || how == CastType::WAND) {
        if(weaker) {
            num = skLevel / 2;
            if(num < 1)
                num = 1;
            title = (num + 2) / 3;
        } else {
            title = (skLevel + 2) / 3;
        }

        if(title > 10)
            title = 10;
    } else {
        title = (undead ? 3 : 1);
    }
    return(std::max(1, title));
}


//*********************************************************************
//                      conjure
//*********************************************************************
int conjureCmd(const std::shared_ptr<Player>& player, cmd* cmnd) {
    SpellData data;
    data.set(CastType::SKILL, CONJURATION, NO_DOMAIN, nullptr, player);
    if(!data.check(player))
        return(0);
    return(conjure(player, cmnd, &data));
}

int conjure(const std::shared_ptr<Creature>&player, cmd *cmnd, SpellData *spellData) {
    std::shared_ptr<Player>pPlayer = player->getAsPlayer();
    if (!pPlayer)
        return (0);

    std::shared_ptr<Monster> target = nullptr;
    int title = 0, mp = 0, realm = 0, level = 0, spells = 0, chance = 0, sRealm = 0;
    int buff = 0, hp_percent = 0, mp_percent = 0, a = 0, rnum = 0, cClass = 0, skLevel = 0;
    int interval = 0, n = 0, x = 0, hplow = 0, hphigh = 0, mplow = 0, mphigh = 0;
    time_t t, i, len = 0;;
    const char *delem;

    // BUG: Name is not long enough for "schizophrenic serial killer clown"
    char *s, *p, name[80];

    delem = " ";

    if (!player->ableToDoCommand())
        return (0);

    if (player->noPotion(spellData))
        return (0);

    if (spellData->how == CastType::SKILL && !player->knowsSkill("conjure")) {
        player->print("The conjuring of elementals escapes you.\n");
        return (0);
    }

    /*
    if(spellData->how == CastType::SKILL) {
        skLevel = (int)player->getSkillLevel("conjure");
    } else {
        skLevel = player->getLevel();
    }
    */
    // TODO: conjure/animate is no longer a skill until progression has been fixed
    skLevel = player->getLevel();

    if (spellData->how == CastType::SKILL &&
        player->getClass() == CreatureClass::CLERIC &&
        player->getDeity() == GRADIUS &&
        (player->getAdjustedAlignment() == BLOODRED ||
         player->getAdjustedAlignment() == ROYALBLUE
        )) {
        player->print("Your alignment is out of harmony.\n");
        return (0);
    }

    if (player->getClass() == CreatureClass::BUILDER) {
        player->print("You cannot conjure pets.\n");
        return (0);
    }

    if (player->hasPet() && !player->checkStaff("Only one conjuration at a time!\n"))
        return (0);

    if (spellData->object) {
        level = spellData->object->getQuality() / 10;
    } else {
        level = skLevel;
    }

    title = getPetTitle(spellData->how, level,
                        spellData->how == CastType::WAND && player->getClass() != CreatureClass::DRUID &&
                        !(player->getClass() == CreatureClass::CLERIC && player->getDeity() == GRADIUS), false);
    mp = 4 * title;

    if (spellData->how == CastType::CAST && !player->checkMp(mp))
        return (0);

    t = time(nullptr);
    i = LT(player, LT_INVOKE);
    if (!player->isCt()) {
        if ((i > t) && (spellData->how != CastType::WAND)) {
            player->pleaseWait(i - t);
            return (0);
        }
        if (spellData->how == CastType::CAST) {
            if (player->spellFail(spellData->how)) {
                player->subMp(mp);
                return (0);
            }
        }
    }

    if (player->getClass() == CreatureClass::DRUID ||
        (spellData->how == CastType::SKILL &&
         !(player->getClass() == CreatureClass::CLERIC && player->getDeity() == GRADIUS)) ||
        player->isDm()) {

        if ((cmnd->num < 3 && (spellData->how == CastType::CAST || spellData->how == CastType::WAND)) ||
            (cmnd->num < 2 && spellData->how == CastType::SKILL)
                ) {
            player->print("Conjure what kind of elemental (earth, air, water, fire, electricity, cold)?\n");
            return (0);
        }
        s = cmnd->str[spellData->how == CastType::SKILL ? 1 : 2];
        len = strlen(s);

        if (!strncasecmp(s, "earth", len))
            realm = EARTH;
        else if (!strncasecmp(s, "air", len))
            realm = WIND;
        else if (!strncasecmp(s, "fire", len))
            realm = FIRE;
        else if (!strncasecmp(s, "water", len))
            realm = WATER;
        else if (!strncasecmp(s, "cold", len))
            realm = COLD;
        else if (!strncasecmp(s, "electricity", len))
            realm = ELEC;

        else if (player->isDm() && !strncasecmp(s, "mage", len))
            realm = CONJUREMAGE;
        else if (player->isDm() && !strncasecmp(s, "bard", len))
            realm = CONJUREBARD;
        else {
            player->print("Conjure what kind of elemental (earth, air, water, fire, electricity, cold)?\n");
            return (0);
        }
    } else {
        if (player->getClass() == CreatureClass::CLERIC && player->getDeity() == GRADIUS)
            realm = EARTH;
        else if (player->getClass() == CreatureClass::BARD)
            realm = CONJUREBARD;
        else
            realm = CONJUREMAGE;
    }

    // 0 = weak, 1 = normal, 2 = buff
    buff = Random::get(1, 3) - 1;

    target = std::make_shared<Monster>();
    if (!target) {
        player->print("Cannot allocate memory for target.\n");
        return (PROMPT);
    }

    // Only level 30 titles
    int titleIdx = std::min(29, title);
    if (realm == CONJUREBARD) {
        target->setName(bardConjureTitles[buff][titleIdx - 1]);
        strncpy(name, bardConjureTitles[buff][titleIdx - 1], 79);
    } else if (realm == CONJUREMAGE) {
        target->setName(mageConjureTitles[buff][titleIdx - 1]);
        strncpy(name, mageConjureTitles[buff][titleIdx - 1], 79);
    } else {
        target->setName(conjureTitles[realm - 1][buff][titleIdx - 1]);
        strncpy(name, conjureTitles[realm - 1][buff][titleIdx - 1], 79);
    }
    name[79] = '\0';

    if (spellData->object) {
        level = spellData->object->getQuality() / 10;
    } else {
        level = skLevel;
    }

    switch (buff) {
        case 0:
            level -= 3;
            break;
        case 1:
            level -= 2;
            break;
        case 2:
            level -= Random::get(0, 1);
            break;
    }

    level = std::max(1, std::min(MAXALVL, level));

    p = strtok(name, delem);
    if (p)
        strncpy(target->key[0], p, 19);
    p = strtok(nullptr, delem);
    if (p)
        strncpy(target->key[1], p, 19);
    p = strtok(nullptr, delem);
    if (p)
        strncpy(target->key[2], p, 19);

    target->setType(MONSTER);
    bool combatPet = false;

    if (realm == CONJUREBARD) {
        chance = Random::get(1, 100);
        if (chance > 50)
            cClass = Random::get(1, 13);

        if (cClass) {
            switch (cClass) {
                case 1:
                    target->setClass(CreatureClass::ASSASSIN);
                    break;
                case 2:
                    target->setClass(CreatureClass::BERSERKER);
                    combatPet = true;
                    break;
                case 3:
                    target->setClass(CreatureClass::CLERIC);
                    break;
                case 4:
                    target->setClass(CreatureClass::FIGHTER);
                    combatPet = true;
                    break;
                case 5:
                    target->setClass(CreatureClass::MAGE);
                    break;
                case 6:
                    target->setClass(CreatureClass::PALADIN);
                    break;
                case 7:
                    target->setClass(CreatureClass::RANGER);
                    break;
                case 8:
                    target->setClass(CreatureClass::THIEF);
                    break;
                case 9:
                    target->setClass(CreatureClass::PUREBLOOD);
                    break;
                case 10:
                    target->setClass(CreatureClass::MONK);
                    break;
                case 11:
                    target->setClass(CreatureClass::DEATHKNIGHT);
                    break;
                case 12:
                    target->setClass(CreatureClass::WEREWOLF);
                    break;
                case 13:
                    target->setClass(CreatureClass::ROGUE);
                    break;
            }
        }
        //if(target->getClass() == CreatureClass::CLERIC && Random::get(1,100) > 50)
        //  target->learnSpell(S_HEAL);

        if (target->getClass() == CreatureClass::MAGE && Random::get(1, 100) > 50)
            target->learnSpell(S_RESIST_MAGIC);
    }


    target->setLevel(std::min(level, skLevel + 1));
    level--;
    target->strength.setInitial(conjureStats[buff][level].str * 10);
    target->dexterity.setInitial(conjureStats[buff][level].dex * 10);
    target->constitution.setInitial(conjureStats[buff][level].con * 10);
    target->intelligence.setInitial(conjureStats[buff][level].intel * 10);
    target->piety.setInitial(conjureStats[buff][level].pie * 10);

    target->strength.restore();
    target->dexterity.restore();
    target->constitution.restore();
    target->intelligence.restore();
    target->piety.restore();

    // This will be adjusted in 2.50, for now just str*2
    target->setAttackPower(target->strength.getCur() * 2);

    // There are as many variations of elementals on the elemental
    // planes as there are people on the Prime Material. Therefore,
    // the elementals summoned have varying hp and mp stats. -TC
    if (player->getClass() == CreatureClass::CLERIC && player->getDeity() == GRADIUS) {
        hp_percent = 8;
        mp_percent = 4;
    } else {
        hp_percent = 6;
        mp_percent = 8;
    }

    hphigh = conjureStats[buff][level].hp;
    hplow = (conjureStats[buff][level].hp * hp_percent) / 10;
    target->hp.setInitial(Random::get(hplow, hphigh));
    target->hp.restore();

    if (!combatPet) {
        mphigh = conjureStats[buff][level].mp;
        mplow = (conjureStats[buff][level].mp * mp_percent) / 10;
        target->mp.setInitial(std::max(10, Random::get(mplow, mphigh)));
        target->mp.restore();
    }

    //target->hp.getMax() = target->hp.getCur() = conjureStats[buff][level].hp;
    // target->mp.getMax() = target->mp.getCur() = conjureStats[buff][level].mp;
    target->setArmor(conjureStats[buff][level].armor);

    int skill = ((target->getLevel() - (combatPet ? 0 : 1)) * 10) + (buff * 5);

    target->setDefenseSkill(skill);
    target->setWeaponSkill(skill);

    target->setExperience(0);
    target->damage.setNumber(conjureStats[buff][level].ndice);
    target->damage.setSides(conjureStats[buff][level].sdice);
    target->damage.setPlus(conjureStats[buff][level].pdice);
    target->first_tlk = nullptr;
    target->setParent(nullptr);

    for (n = 0; n < 20; n++)
        target->ready[n] = nullptr;

    if (!combatPet) {
        for (Realm r = MIN_REALM; r < MAX_REALM; r = (Realm) ((int) r + 1))
            target->setRealm(Random::get((conjureStats[buff][level].realms * 3) / 4, conjureStats[buff][level].realms),
                             r);
    }


    target->lasttime[LT_TICK].ltime =
    target->lasttime[LT_TICK_SECONDARY].ltime =
    target->lasttime[LT_TICK_HARMFUL].ltime = time(nullptr);

    target->lasttime[LT_TICK].interval =
    target->lasttime[LT_TICK_SECONDARY].interval = 60;
    target->lasttime[LT_TICK_HARMFUL].interval = 30;

    target->getMobSave();

    if (!combatPet) {
        if (player->getDeity() == GRADIUS || realm == CONJUREBARD)
            target->setCastChance(Random::get(5, 10)); // cast precent
        else
            target->setCastChance(Random::get(20, 50));    // cast precent
        target->proficiency[1] = realm;
        target->setFlag(M_CAST_PRECENT);
        target->setFlag(M_CAN_CAST);

        target->learnSpell(S_VIGOR);
        if (target->getLevel() > 10 && player->getDeity() != GRADIUS)
            target->learnSpell(S_MEND_WOUNDS);
        if (target->getLevel() > 7 && player->getDeity() != GRADIUS && player->getClass() != CreatureClass::DRUID)
            target->learnSpell(S_CURE_POISON);


        switch (buff) {
            case 0:
                // Wimpy mob -- get 1-2 spells
                spells = Random::get(1, 2);
                break;
            case 1:
                // Medium mob -- get 1-4 spells
                spells = Random::get(2, 4);
                break;
            case 2:
                // Buff mob -- get 2-5 spells
                spells = Random::get(2, 5);
                break;
        }

        // Give higher level pets higher chance for more spells
        if (target->getLevel() > 25)
            spells += 2;
        else if (target->getLevel() > 16)
            spells += 1;

        if (target->getLevel() < 3)
            spells = 1;
        else if (target->getLevel() < 7)
            spells = std::min(spells, 2);
        else if (target->getLevel() < 12)
            spells = std::min(spells, 3);
        else if (target->getLevel() < 16)
            spells = std::min(spells, 4);
        else
            spells = std::min(spells, 5);

    }

//  // Druid and gradius pets only
//  if(realm != CONJUREBARD && realm != CONJUREMAGE) {
//
//      // TODO: Add realm based attacks
//      // ie: earth - smother, water - ?? fire - ??
//  }
    //Gradius Earth Pets
    if (player->getDeity() == GRADIUS) {
        int enchantedOnlyChance = 0;
        int bashChance = 0;
        int circleChance = 0;

        if (target->getLevel() <= 12)
            enchantedOnlyChance = 50;
        else
            enchantedOnlyChance = 101;

        if (target->getLevel() >= 10)
            bashChance = 50;

        if (target->getLevel() >= 16)
            circleChance = 75;
        else if (target->getLevel() >= 13)
            circleChance = 50;

        if (combatPet) {
            bashChance += 25;
            circleChance += 25;
        }

        if (Random::get(1, 100) <= enchantedOnlyChance)
            target->setFlag(M_ENCHANTED_WEAPONS_ONLY);

        if (Random::get(1, 100) <= bashChance)
            target->addSpecial("bash");

        if (Random::get(1, 100) <= circleChance)
            target->addSpecial("circle");


        if (target->getLevel() >= 10) {

            int numResist = Random::get(1, 3);
            for (a = 0; a < numResist; a++) {
                rnum = Random::get(1, 5);
                switch (rnum) {
                    case 1:
                        target->addEffect("resist-slashing");
                        break;
                    case 2:
                        target->addEffect("resist-piercing");
                        break;
                    case 3:
                        target->addEffect("resist-crushing");
                        break;
                    case 4:
                        target->addEffect("resist-ranged");
                        break;
                    case 5:
                        target->addEffect("resist-chopping");
                        break;
                }
            }
        }

        if (target->getLevel() >= 13) {
            if (Random::get(1, 100) <= 50) {
                target->setFlag(M_PLUS_TWO);
                target->clearFlag(M_ENCHANTED_WEAPONS_ONLY);
            }
            target->setFlag(M_REGENERATES);
            if (Random::get(1, 100) <= 15)
                target->addSpecial("smother");
        }

        if (target->getLevel() >= 16) {
            target->setFlag(M_PLUS_TWO);
            if (Random::get(1, 100) <= 20) {
                target->setFlag(M_PLUS_THREE);
                target->clearFlag(M_PLUS_TWO);
                target->clearFlag(M_ENCHANTED_WEAPONS_ONLY);
            }
            target->setFlag(M_REGENERATES);
            if (Random::get(1, 100) <= 30)
                target->addSpecial("smother");

            if (Random::get(1, 100) <= 15)
                target->addSpecial("trample");

            target->addPermEffect("immune-earth");
        }

        if (target->getLevel() >= 19) {
            target->setFlag(M_PLUS_TWO);
            if (Random::get(1, 100) <= 40) {
                target->setFlag(M_PLUS_THREE);
                target->clearFlag(M_PLUS_TWO);
                target->clearFlag(M_ENCHANTED_WEAPONS_ONLY);
            }
            target->setFlag(M_REGENERATES);
            if (Random::get(1, 100) <= 40)
                target->addSpecial("smother");

            if (Random::get(1, 100) <= 25)
                target->addSpecial("trample");

            target->addPermEffect("immune-earth");
            target->setFlag(M_NO_LEVEL_ONE);
            target->setFlag(M_RESIST_STUN_SPELL);
        }

    }

    if (!combatPet) {
        sRealm = realm;
        for (x = 0; x < spells; x++) {
            if (realm == CONJUREBARD || realm == CONJUREMAGE)
                sRealm = getRandomRealm();
            target->learnSpell(getOffensiveSpell((Realm) sRealm, x)); // TODO: fix bad cast
        }
    }

    player->print("You conjure a %s.\n", target->getCName());
    player->checkImprove("conjure", true);
    broadcast(player->getSock(), player->getParent(), "%M conjures a %s.", player.get(), target->getCName());


    if (!combatPet) {
        chance = Random::get(1, 100);
        if (chance > 50)
            target->learnSpell(S_DETECT_INVISIBILITY);
        if (chance > 90)
            target->learnSpell(S_TRUE_SIGHT);
    }

    // add it to the room, make it active, and make it follow the summoner
    target->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
    target->lasttime[LT_MOB_THIEF].ltime = t;
    target->addToRoom(player->getRoomParent());
    gServer->addActive(target);

    player->addPet(target);
//  addFollower(player, target, FALSE);

    petTalkDesc(target, player);
    target->setFlag(M_PET);

    if (!combatPet) {
        if (realm < MAX_REALM)
            target->setBaseRealm((Realm) realm); // TODO: fix bad cast
    }

    // find out how long it's going to last and create all the timeouts
    x = player->piety.getCur();
    if (player->getClass() == CreatureClass::DRUID && player->constitution.getCur() > player->piety.getCur())
        x = player->constitution.getCur();
    x = bonus(x) * 60L;

    interval = (60L * Random::get(2, 4)) + (x >= 0 ? x : 0); //+ 60 * title;

    target->lasttime[LT_INVOKE].ltime = t;
    target->lasttime[LT_INVOKE].interval = interval;
    player->lasttime[LT_INVOKE].ltime = t;
    player->lasttime[LT_INVOKE].interval = 600L;
    // pets use LT_SPELL
    target->lasttime[LT_SPELL].ltime = t;
    target->lasttime[LT_SPELL].interval = 3;

    if (player->isCt())
        player->lasttime[LT_INVOKE].interval = 6L;

    if (spellData->how == CastType::CAST)
        player->mp.decrease(mp);

    if (spellData->how == CastType::SKILL)
        return (PROMPT);
    else
        return (1);
}

//*********************************************************************
//                      splDenseFog
//*********************************************************************

int splDenseFog(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    int strength = 10;
    long duration = 600;

    if(player->noPotion( spellData))
        return(0);

    if(!player->isCt()) {
        if(player->getRoomParent()->isUnderwater()) {
            player->print("Water currents prevent you from casting that spell.\n");
            return(0);
        }
    }

    player->print("You cast a dense fog spell.\n");
    broadcast(player->getSock(), player->getParent(), "%M casts a dense fog spell.", player.get());

    if(player->getRoomParent()->hasPermEffect("dense-fog")) {
        player->print("The spell didn't take hold.\n");
        return(0);
    }

    if(spellData->how == CastType::CAST) {
        if(player->getRoomParent()->magicBonus())
            player->print("The room's magical properties increase the power of your spell.\n");
    }

    player->getRoomParent()->addEffect("dense-fog", duration, strength, player, true, player);
    return(1);
}

//*********************************************************************
//                      splToxicCloud
//*********************************************************************

int splToxicCloud(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    int strength = spellData->level;
    long duration = 600;

    if(player->noPotion( spellData))
        return(0);

    if(!player->isCt()) {
        if(player->getRoomParent()->isPkSafe()) {
            player->print("This is a safe room. That spell is not allowed here.\n");
            return(0);
        }
        if(player->getRoomParent()->isUnderwater()) {
            player->print("Strong water currents prevent you from casting that spell.\n");
            return(0);
        }
    }

    player->print("You cast a toxic cloud spell.\n");
    broadcast(player->getSock(), player->getParent(), "%M casts a toxic cloud spell.", player.get());

    if(player->getRoomParent()->hasPermEffect("toxic-cloud")) {
        player->print("The spell didn't take hold.\n");
        return(0);
    }

    if(spellData->how == CastType::CAST) {
        if(player->getRoomParent()->magicBonus())
            player->print("The room's magical properties increase the power of your spell.\n");
    }

    player->getRoomParent()->addEffect("toxic-cloud", duration, strength, player, true, player);
    return(1);
}

//******************************************************************************************************
//                      doWallSpell
//******************************************************************************************************
int doWallSpell(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData, const char * wallSpell, int strength, long duration) {
    std::shared_ptr<Exit> exit=nullptr;
    std::shared_ptr<BaseRoom> targetRoom=nullptr;

    if(player->noPotion(spellData))
        return(0);

    if(player->getRoomParent()->isPkSafe() && !player->isCt()) {
            *player << "This is a safe room. That spell is not allowed here.\n";
            return(0);
        }

    if(cmnd->num > 2)
        exit = findExit(player, cmnd, 2);
    if(!exit) {
        *player << ColorOn << "Cast "<< wallSpell << " on which exit?\n" << ColorOff;
        return(0);
    }

    targetRoom = exit->target.loadRoom();
    if (!targetRoom) {
        *player << "Your spell fizzled.\n";
        return(0);
    }
    if (targetRoom->isPkSafe()) {
        *player << "That spell is not allowed here. The room beyond the '" << exit->getCName() << "' exit is a safe room.\n";
        return(0);
    }

    bool wallReplace=false;
    if (exit->isEffected(stripColor(wallSpell))) {
       auto* effInfo = exit->getEffect(stripColor(wallSpell));

        if (!player->isCt() && ((effInfo->getStrength() > strength) || (exit->hasPermEffect(stripColor(wallSpell))))) {
            *player << ColorOn << "The existing " << wallSpell << " on the '" << exit->getCName() << "' exit is too strong for you to replace.\nYour spell fizzled.\n" << ColorOff;
            broadcast(player->getSock(), player->getParent(), "%M tried to cast a %s spell in front of the '%s' exit. %s spell fizzled.^x", player.get(), wallSpell, exit->getCName(), player->upHisHer());
            return(0);
       }
       else 
            wallReplace=true;      
    }
    if (wallReplace) {
        *player << "You cast a new " << wallSpell << " to replace the one in front of the '" << exit->getCName() << "' exit.\n";
        broadcast(player->getSock(), player->getParent(), "%M casts a new %s spell to replace the one in front of the '%s' exit.^x", player.get(), wallSpell, exit->getCName());
    }
    else 
    {
        *player << "You cast a " << wallSpell << " in front of the '" << exit->getCName() << "' exit.\n";
        broadcast(player->getSock(), player->getParent(), "%M casts a %s spell in front of the '%s' exit.^x", player.get(), wallSpell, exit->getCName());
    }
    
    if(spellData->how == CastType::CAST) {
        if(player->getRoomParent()->magicBonus())
            *player << "The room's magical properties increase the power of your spell.\n";
    }

    if (wallReplace)
        exit->removeEffectReturnExit(stripColor(wallSpell), player->getRoomParent());

    exit->addEffectReturnExit(stripColor(wallSpell), duration, strength, player);
    return(1);

}

//*********************************************************************
//                      splWallOfFire
//*********************************************************************

int splWallOfFire(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    int strength = spellData->level;
    long duration = (long)strength * 15;

    if(!player->isCt()) {
        if (!player->isPureArcaneCaster() && !player->isPureDivineCaster()) {
            *player << "You lack the arcane or divine fundamentals to cast that spell.\n";
            return(0);
        }
        if(player->getRoomParent()->isUnderwater()) {
            *player << "Being underwater prevents you from casting that spell here.\n";
            return(0);
        }

    }
    if (player->getRoomParent()->flagIsSet(R_COLD_BONUS) || player->getRoomParent()->flagIsSet(R_WINTER_COLD))
        duration = (duration*2)/3;
    if (player->getRoomParent()->flagIsSet(R_WATER_BONUS))
        duration /= 2;
    if (player->getRoomParent()->flagIsSet(R_FIRE_BONUS))
        duration *= 2;

    if (player->getClass() == CreatureClass::DRUID)
        duration = (duration*3)/2;

    return(doWallSpell(player, cmnd, spellData, "^Rwall-of-fire^x", strength, duration));
}

//*********************************************************************
//                      splWallOfSleet
//*********************************************************************

int splWallOfSleet(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    int strength = spellData->level;
    long duration = (long)strength * 15;

    if(!player->isCt()) {
        if (!player->isPureArcaneCaster() && !player->isPureDivineCaster()) {
            *player << "You lack the arcane or divine fundamentals to cast that spell.\n";
            return(0);
        }
        if(player->getRoomParent()->isUnderwater()) {
            *player << "Being underwater prevents you from casting that spell here.\n";
            return(0);
        }

    }
    if (player->getRoomParent()->flagIsSet(R_FIRE_BONUS) && player->getRoomParent()->flagIsSet(R_PLAYER_HARM))
        duration = (duration*2)/3;
    if (player->getRoomParent()->flagIsSet(R_FIRE_BONUS) || (player->getRoomParent()->flagIsSet(R_DESERT_HARM) && isDay()))
        duration /= 2;
    if (player->getRoomParent()->flagIsSet(R_COLD_BONUS))
        duration *= 2;

    if (player->getClass() == CreatureClass::DRUID)
        duration = (duration*3)/2;

    return(doWallSpell(player, cmnd, spellData, "^Cwall-of-sleet^x", strength, duration));
}

//*********************************************************************
//                      splWallOfLightning
//*********************************************************************

int splWallOfLightning(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    int strength = spellData->level;
    long duration = (long)strength * 15;

    if(!player->isCt()) {
        if (!player->isPureArcaneCaster() && !player->isPureDivineCaster()) {
            *player << "You lack the arcane or divine fundamentals to cast that spell.\n";
            return(0);
        }
        if(player->getRoomParent()->isUnderwater()) {
            *player << ColorOn << "^#^cThe massive amount of water surrounding you diffuses the electricity with a loud BANG!\n" << ColorOff;
            broadcast(player->getSock(), player->getParent(), "^#^c%M tried to cast a ^cwall-of-lightning^x spell. The surrounding water diffused it with a loud BANG!^x", player.get());
            return(0);
        }
    }

    if (player->getRoomParent()->flagIsSet(R_WATER_BONUS))
        duration /= 2;
    if (player->getRoomParent()->flagIsSet(R_ELEC_BONUS))
        duration *= 2;
    if (player->getClass() == CreatureClass::DRUID)
        duration = (duration*3)/2;

    return(doWallSpell(player, cmnd, spellData, "^cwall-of-lightning^x", strength, duration));
}

//*********************************************************************
//                      splWallOfForce
//*********************************************************************
int splWallOfForce(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    int strength = spellData->level;
    long duration = (long)strength * 15;


    if(!player->isCt() && !player->isPureArcaneCaster()) {
        *player << "You lack the arcane fundamentals to cast that spell.\n";
        return(0);
    }


    return(doWallSpell(player, cmnd, spellData, "^Mwall-of-force^x", strength, duration));
}

//*********************************************************************
//                      splWallOfThorns
//*********************************************************************

int splWallOfThorns(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    int strength = spellData->level;
    long duration = (long)strength * 15;

     if(!player->isCt()) {
        if (player->getClass() != CreatureClass::DRUID && player->getClass() != CreatureClass::RANGER &&
            !(player->getClass() == CreatureClass::CLERIC && (player->getDeity() == LINOTHAN || player->getDeity() == MARA))) {
            *player << "Only druids, rangers, and clerics of Linothan or Mara can cast that spell.\n";
            return(0);
        }
    }

    
    return(doWallSpell(player, cmnd, spellData, "^ywall-of-thorns^x", strength, duration));
}

//*********************************************************************
//                      bringDownTheWall
//*********************************************************************

void bringDownTheWall(EffectInfo* effect, const std::shared_ptr<BaseRoom>& room, std::shared_ptr<Exit> exit) {
    if(!effect)
        return;

    std::shared_ptr<BaseRoom> targetRoom=nullptr;
    std::string name = effect->getName();

    targetRoom = exit->target.loadRoom();
    if (!targetRoom)
        return;

    if(effect->isPermanent()) {
        // fake being removed
        auto* ef = effect->getEffect();
        room->effectEcho(ef->getRoomDelStr(), exit);

        // extra of 2 means a 2 pulse (21-40 seconds) duration
        effect->setExtra(2);

        exit = exit->getReturnExit(room, targetRoom);
        if(exit) {
            effect = exit->getEffect(name);
            if(effect) {
                targetRoom->effectEcho(ef->getRoomDelStr(), exit);
                effect->setExtra(2);
            }
        }
    } else {
        exit->removeEffectReturnExit(name, room);
    }
}
