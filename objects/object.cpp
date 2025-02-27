/*
 * object.cpp
 *   Object routines.
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

#include <fmt/format.h>                // for format
#include <cstdio>                      // for sprintf
#include <cstring>                     // for strcpy, strtok, strcat, strcmp
#include <ctime>                       // for time
#include <map>                         // for operator==, _Rb_tree_const_ite...
#include <ostream>                     // for operator<<, basic_ostream, ost...
#include <set>                         // for set, set<>::iterator
#include <string>                      // for string, operator<<, char_traits
#include <string_view>                 // for string_view
#include <type_traits>                 // for enable_if<>::type
#include "catRef.hpp"                  // for CatRef

#include "cmd.hpp"                     // for cmd
#include "commands.hpp"                // for timestr, cmdKeep, cmdUnkeep
#include "config.hpp"                  // for Config, gConfig
#include "craft.hpp"                   // for operator<<, Recipe (ptr only)
#include "flags.hpp"                   // for O_KEEP, O_DARKMETAL, O_BEING_P...
#include "global.hpp"                  // for CAP, CreatureClass, CreatureCl...
#include "hooks.hpp"                   // for Hooks
#include "lasttime.hpp"                // for lasttime
#include "mud.hpp"                     // for LT_ENVEN, ospell, scrollDesc
#include "mudObjects/container.hpp"    // for ObjectSet, Container, MonsterSet
#include "mudObjects/creatures.hpp"    // for Creature
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/objects.hpp"      // for Object, ObjectType, Material
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "paths.hpp"                   // for Sign
#include "proto.hpp"                   // for broadcast, keyTxtEqual, cantDr...
#include "random.hpp"                  // for Random
#include "realm.hpp"                   // for EARTH, COLD, WATER
#include "size.hpp"                    // for getSizeName, NO_SIZE, Size
#include "skills.hpp"                  // for getSkillLevelStr
#include "socket.hpp"                  // for Socket
#include "stats.hpp"                   // for Stat
#include "structs.hpp"                 // for osp_t
#include "unique.hpp"                  // for Unique, Lore
#include "xml.hpp"                     // for loadObject

class MudObject;

const std::map<ObjectType, std::string> Object::objTypeToString = {
        {ObjectType::WEAPON, "weapon"},
        {ObjectType::INSTRUMENT, "instrument"},
        {ObjectType::HERB, "herb"},
        {ObjectType::ARMOR, "armor"},
        {ObjectType::POTION, "potion"},
        {ObjectType::SCROLL, "scroll"},
        {ObjectType::WAND, "wand"},
        {ObjectType::CONTAINER, "container"},
        {ObjectType::MONEY, "money"},
        {ObjectType::KEY, "key"},
        {ObjectType::LIGHTSOURCE , "lightsource"},
        {ObjectType::MISC, "misc"},
        {ObjectType::SONGSCROLL, "song scroll"},
        {ObjectType::POISON, "poison"},
        {ObjectType::BANDAGE, "bandage"},
        {ObjectType::AMMO, "ammo"},
        {ObjectType::QUIVER, "quiver"},
        {ObjectType::LOTTERYTICKET, "lottery ticket"},
        {ObjectType::GEMSTONE, "gemstone"},
};

const std::map<Material, std::string> Object::materialToString = {
        {Material::WOOD, "^ywood^x"},
        {Material::GLASS, "^wglass^x" },
        {Material::CLOTH, "^wcloth^x" },
        {Material::PAPER, "^wpaper^x" },
        {Material::IRON, "^Diron^x" },
        {Material::STEEL, "^wsteel^x" },
        {Material::MITHRIL, "^Wmithril^x" },
        {Material::ADAMANTIUM, "^Dadamantium^x" },
        {Material::STONE, "^wstone^x" },
        {Material::ORGANIC, "^worganic^x" },
        {Material::BONE, "^Wbone^x" },
        {Material::LEATHER, "^yleather^x" },
        {Material::DARKMETAL, "^Ddarkmetal^x" },
        {Material::CRYSTAL, "^wcrystal^x"},
        {Material::MCOPPER, "^rcopper^x"},
        {Material::MSILVER, "^Dsilver^x"},
        {Material::MGOLD, "^Ygold^x"},
        {Material::MPLATINUM, "^Wplatinum^x"},
        {Material::MALANTHIUM, "^calanthium^x"},
        {Material::MELECTRUM, "^yelectrum^x"},
        {Material::CERAMIC, "^wceramic^x"},
        {Material::CLAY, "^Dclay^x"},
        {Material::SOFTSTONE, "^wsoft stone^x"},
        {Material::HARDLEATHER, "^yhard leather^x"},
        {Material::BRONZE, "^ybronze^x"},
        {Material::ARGENTINE, "^Wargentine^x"},
        {Material::ELVENSTEEL, "^Welvensteel^x"},
        {Material::ELECTRITE, "^celectrite^x"},
        {Material::METEORIC_IRON, "^wmeteoric iron^x"},
        {Material::SHADOW_IRON, "^Dshadow iron^x"},
        {Material::ORICHALCUM, "^yorichalcum^x"},
        {Material::SCARLETITE, "^rscarletite^x"},
        {Material::TRUESILVER, "^Wtrue silver^x"},
        {Material::AMARANTHIUM, "^mamaranthium^x"},
        {Material::INFERNITE, "^Rinfernite^x"},
        {Material::CELESTITE, "^Bcelestite^x"},
        {Material::NEGATIVE_MITHRIL, "^Dnegative mithril^x"},
        {Material::NEGATIVE_STEEL, "^Dnegative steel^x"},
};

const std::string NONE_STR = "none";

const std::string & Object::getTypeName() const {
    if(objTypeToString.find(type) == objTypeToString.end())
        return(NONE_STR);

    return objTypeToString.at(type);
}

const std::string & Object::getMaterialName() const {
    if(materialToString.find(material) == materialToString.end())
        return(NONE_STR);

    return materialToString.at(material);
}



//*********************************************************************
//                      add_obj_obj
//*********************************************************************
// This function adds the object toAdd to the current object

void Object::addObj(std::shared_ptr<Object>toAdd, bool incShots) {
    toAdd->validateId();

    Hooks::run(toAdd, "beforeAddObject", Containable::downcasted_shared_from_this<Object>(), "beforeAddToObject");

    add(toAdd);
    if(incShots)
        incShotsCur();
    Hooks::run(toAdd, "afterAddObject", Containable::downcasted_shared_from_this<Object>(), "afterAddToObject");
}

//*********************************************************************
//                      del_obj_obj
//*********************************************************************
// This function removes the object pointed to by the first parameter
// from the object pointed to by the second.

void Object::delObj(std::shared_ptr<Object> toDel) {
    Hooks::run(Containable::downcasted_shared_from_this<Object>(), "beforeRemoveObject", toDel, "beforeRemoveFromObject");

    decShotsCur();
    toDel->removeFrom();
    Hooks::run(Containable::downcasted_shared_from_this<Object>(), "afterRemoveObject", toDel, "afterRemoveFromObject");
}


//*********************************************************************
//                      listObjects
//*********************************************************************
// This function produces a string which lists all the objects in a
// player's, room's or object's inventory.

bool listObjectSee(const std::shared_ptr<const Player> player, std::shared_ptr<Object>  object, bool showAll) {
    return(player->isStaff() || (player->canSee(object) && (showAll || !object->flagIsSet(O_HIDDEN)) && !object->flagIsSet(O_SCENERY)) );
}

std::string Container::listObjects(const std::shared_ptr<const Player> &player, bool showAll, char endColor, std::string filter) const {
    std::shared_ptr<Object> object=nullptr;
    int     num=1, n=0;
    int flags = player->displayFlags();
    std::string str = "";

    ObjectSet::iterator it;
    for( it = objects.begin() ; it != objects.end() ; ) {
        object = (*it++);
        if(object->inCreature()) {
            if(object->getCreatureParent()->mFlagIsSet(M_TRADES))
                continue;

            if( !showAll && (
                    object->flagIsSet(O_NOT_PEEKABLE) ||
                    Unique::is(object) ||
                    (object->inMonster() && object->flagIsSet(O_BODYPART)) ) )
                continue;

        }

        if(!listObjectSee(player, object, showAll))
            continue;

        if (!filter.empty()) {
            if (object->getTypeName() != filter)
                continue;
            //if (object->isTrash() && filter != "trash")
            //    continue;
            }

        num = 1;
        while(it != objects.end()) {
            if(object->showAsSame(player, (*it)) && listObjectSee(player, (*it), showAll)) {
                num++;
                it++;
            } else
                break;
        }
        if(n)
            str += ", ";
        str += object->getObjStr(player, flags, num);
        str += "^";
        str += endColor;
        n++;
    }
    return(str);
}


//*********************************************************************
//                      randomEnchant
//*********************************************************************
// This function randomly enchants an object if its random-enchant flag
// is set.

void Object::randomEnchant(int bonus) {
    char    m = Random::get(1,100);
    m += bonus;

    if(m > 98)
        setAdjustment(3);
    else if(m > 90)
        setAdjustment(2);
    else if(m > 50)
        setAdjustment(1);

    if(flagIsSet(O_CURSED) && adjustment > 0)
        setAdjustment(adjustment * -1);

    //armor += (int)(adjustment*4.4);
}

// min int 13, cast flag, or mage class
int new_scroll(int level, std::shared_ptr<Object> &new_obj) {
    int lvl, realm;


    const char  *delem;
    char    name[128];
    int     num=0;
    char    *p;
    new_obj = std::make_shared<Object>();
    // level 2-6 lvl 1 scrolls, level 7-11 lvl 2 scrolls, level 12-17 lvl 3 scrolls,
    // lvl 18-24 lvl 4 scrolls, lvl 25+ lvl 5 scrolls

    if(level == 1)
        return (-1);

    if(level < 7)
        lvl = 1;
    else if(level < 11)
        lvl = 2;
    else if(level < 17)
        lvl = 3;
    else if(level < 25)
        lvl = 4;
    else
        lvl = 5;

    lvl = std::min(lvl,(Random::get(1,100)<=10 ? 3:2));

    if(Random::get(1,100) > 10)
        realm = Random::get<int>(EARTH,WATER);
    else
        realm = Random::get<int>(EARTH,COLD); // cold/elect rarer



    // No index for this object, need to set the save full flag
    new_obj->setFlag(O_SAVE_FULL);

    new_obj->setMagicpower(ospell[(lvl-1)*6 +(realm-1)].splno+1);
    new_obj->description = "It has arcane runes.";
    delem = " ";

    num = Random::get(1,10);
    strcpy(name, scrollDesc[realm-1][num-1]);
    strcat(name, " ");
    num = Random::get(1,2);
    strcat(name, scrollType[lvl-1][num-1]);
    new_obj->setName( name);
    p = strtok(name, delem);
    if(p)
        strcpy(new_obj->key[0], p);
    p = strtok(nullptr, delem);
    if(p)
        strcpy(new_obj->key[1], p);
    p = strtok(nullptr, delem);
    if(p)
        strcpy(new_obj->key[2], p);

    new_obj->setWearflag(HELD);
    new_obj->setType(ObjectType::SCROLL);
    new_obj->setWeight(1);
    new_obj->setShotsMax(1);
    new_obj->setShotsCur(1);

    if(lvl >= 4) {
        lvl -= 4;
        lvl *= 6;
        new_obj->setQuestnum(22 + lvl + (realm-1));
    }

    return(1);
}

int Monster::checkScrollDrop() {
    if( intelligence.getCur() >= 120 ||
        flagIsSet(M_CAN_CAST) ||
        flagIsSet(M_CAST_PRECENT) ||
        cClass == CreatureClass::MAGE ||
        cClass == CreatureClass::LICH
    ) {
        if(Random::get(1,100) <= 4 && !isPet())
            return(1);
    }
    return(0);
}

void Object::loadContainerContents() {
    int     count=0, a=0;
    std::shared_ptr<Object> newObj=nullptr;

    if(type != ObjectType::CONTAINER)
        return;

    for(a=0; a<3; a++)
        if(in_bag[a].id)
            count++;

    if(!count)
        return;

    for(a=0; a<count; a++) {
        if(!in_bag[a].id)
            continue;
        if(!loadObject(in_bag[a], newObj))
            continue;

        if(Random::get(1,100)<25) {
            newObj->setDroppedBy(Containable::downcasted_shared_from_this<Object>(), "ContainerContents");
            addObj(newObj);
        }
    }
}

//*********************************************************************
//                      cmdKeep
//*********************************************************************

int cmdKeep(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Object> object = player->findObject(player, cmnd, 1);

    if(!object) {
        player->print("You don't have that in your inventory.\n");
        return(0);
    }
    if(object->flagIsSet(O_KEEP)) {
        player->print("That's already being kept.\n");
        return(0);
    }

    player->printColor("You will keep %P.\n", object.get());
    object->setFlag(O_KEEP);
    return(0);
}

//*********************************************************************
//                      cmdUnkeep
//*********************************************************************

int cmdUnkeep(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Object> object=nullptr;

    if(!strcmp(cmnd->str[1], "all")) {
        for(const auto& obj : player->objects ) {
            obj->clearFlag(O_KEEP);
        }
        player->print("All kept objects cleared.\n");
        return(0);
    }

    object = player->findObject(player, cmnd, 1);
    if(!object) {
        player->print("That is not in your inventory.\n");
        return(0);
    }

    if(!object->flagIsSet(O_KEEP)) {
        player->print("That is not currently protected by keep.\n");
        return(0);
    }

    player->printColor("You no longer are keeping %P.\n", object.get());
    object->clearFlag(O_KEEP);
    return(0);
}

//*********************************************************************
//                      cmdLabel
//*********************************************************************

int cmdLabel(const std::shared_ptr<Player>& player, cmd* cmnd) {
    if(cmnd->num < 2) {
        player->print("Label what? Type \"help label\" for details.\n");
        return(0);
    }

    if(cmnd->num < 3) {
        player->print("Label it as what? Type \"help label\" for details.\n");
        return(0);
    }

    std::shared_ptr<Object> object = player->findObject(player, cmnd, 1);
    if(!object) {
        player->print("You don't have that in your inventory.\n");
        return(0);
    }

    if(!strcasecmp(cmnd->str[2], "-c")) {
        if (object->isLabeledBy(player)) {
            object->removeLabel();
            player->printColor("Label cleared on %P.\n", object.get());
        } else {
            player->printColor("You have no label on %P.\n", object.get());
        }
        return(0);
    }

    object->setLabel(player, cmnd->str[2]);
    player->printColor("%P labeled as \"%s\".\n", object.get(), cmnd->str[2]);
    return(0);
}

//*********************************************************************
//                      cantDropInBag
//*********************************************************************

bool cantDropInBag(const std::shared_ptr<Object>&  object) {
    for(const auto& obj : object->objects) {
        if(obj->flagIsSet(O_NO_DROP))
            return(true);
    }
    return(false);
}

//*********************************************************************
//                      findObj
//*********************************************************************

std::shared_ptr<MudObject> Creature::findObjTarget(ObjectSet &set, int findFlags, const std::string& str, int val, int* match) {
    if(set.empty())
        return(nullptr);
    const auto cThis = getAsConstCreature();
    for(const auto& obj : set) {
        if(keyTxtEqual(obj, str.c_str()) || (obj->isLabeledBy(cThis) && obj->isLabelMatch(str))) {
            (*match)++;
            if(*match == val) {
                return(obj);
            }
        }
    }
    return(nullptr);
}

//*********************************************************************
//                      displayObject
//*********************************************************************

int displayObject(const std::shared_ptr<const Player> &player, const std::shared_ptr<const Object> &target) {
    int i=0;
    char str[2048];
    char filename[256];
    std::string inv;
    std::string requiredSkillString;

    int flags = player->displayFlags();

    // special 2 is a combo lock, should have normal descriptions
    if(target->getSpecial() == 1) {
        strcpy(str, target->getCName());
        for(i=0; i<strlen(str); i++)
            if(str[i] == ' ')
                str[i] = '_';
        sprintf(filename, "%s/%s.txt", Path::Sign.c_str(), str);
        player->getSock()->viewFile(filename, !target->flagIsSet(O_UNPAGED_FILE));
        return(0);
    }
    std::ostringstream oStr;

    if(!target->description.empty()) {
        oStr << target->description << "\n";
    } else if(!target->getRecipe()) {
        // don't show this message if we have a recipe
        oStr << "You see nothing special about it.\n";
    }

    if(target->getRecipe()) {
        Recipe *r = gConfig->getRecipe(target->getRecipe());
        if(!r)
            oStr << "Sorry, that recipe is faulty.\n";
        else
            oStr << r;
    }

    if(target->compass)
        oStr << target->getCompass(player, false);

    if(target->getType() == ObjectType::LOTTERYTICKET)
        oStr << "It is good for powerbone cycle #" << target->getLotteryCycle() << ".\n";

    if(player->isEffected("know-aura") || player->getClass() == CreatureClass::PALADIN || player->isCt()) {
        if(target->flagIsSet(O_GOOD_ALIGN_ONLY))
            oStr << "It has a blue aura.\n";
        if(target->flagIsSet(O_EVIL_ALIGN_ONLY))
            oStr << "It has a red aura.\n";
    }

    if(target->getType() == ObjectType::CONTAINER) {
        inv = target->listObjects(player, true);
        if(!inv.empty())
            oStr << "It contains: " << inv << ".\n";
    }

    if(target->getType() == ObjectType::ARMOR) {
    	requiredSkillString = target->getArmorType();
        oStr << target->getObjStr(nullptr, flags | CAP, 1) << " is considered ^W" << target->getArmorType() << "^x armor.\n";
    }

    if(target->getType() == ObjectType::WEAPON) {
    	requiredSkillString = target->getWeaponType();
        oStr << target->getObjStr(nullptr, flags | CAP, 1) << " is a " << target->getTypeName() << "(" <<  target->getWeaponType() <<").\n";

        if(target->flagIsSet(O_SILVER_OBJECT))
            oStr << "It is alloyed with pure silver.\n";
    }

    if(target->getMaterial())
        oStr << "Its material make-up is " << target->getMaterialName() << ".^x\n";

    if(target->getRequiredSkill() && !requiredSkillString.empty()) {
    	oStr << "It requires minimum proficiency of '" << getSkillLevelStr(target->getRequiredSkill()) << "' with " << requiredSkillString << ".\n";
    }

    if(target->flagIsSet(O_NO_DROP))
        oStr << target->getObjStr(nullptr, flags | CAP, 1) << " cannot be dropped.\n";

    if(Lore::isLore(target))
        oStr << target->getObjStr(nullptr, flags | CAP, 1) << " is an object of lore.\n";

    if(target->isDarkmetal())
        oStr << "^yIt is vulnerable to sunlight.\n";

    if(target->flagIsSet(O_DARKNESS))
        oStr << "^DIt is engulfed by an aura of darkness.\n";

    if(Unique::isUnique(target))
        oStr << target->getObjStr(nullptr, flags | CAP, 1) << " is a unique or limited object.\n";


    if(target->getType() == ObjectType::WEAPON && target->flagIsSet(O_ENVENOMED)) {
        oStr << "^gIt drips with poison.\n";
        if((player->getClass() == CreatureClass::ASSASSIN && player->getLevel() >= 10) || player->isCt()) {
            oStr << "^gTime remaining before poison deludes: " <<
               timestr(std::max<long>(0,(target->lasttime[LT_ENVEN].ltime+target->lasttime[LT_ENVEN].interval-time(nullptr)))) << ".\n";
        }
    }

    if( target->getType() == ObjectType::POISON && ((player->getClass() == CreatureClass::ASSASSIN && player->getLevel() >= 10) || player->isCt()))
        oStr << target->getObjStr(nullptr, flags | CAP, 1) << " is a poison.\n";

    if(target->flagIsSet(O_COIN_OPERATED_OBJECT))
        oStr << target->getObjStr(nullptr, flags | CAP, 1) << " costs " << target->getCoinCost() << "gold coins per use.\n";

    if(flags & MAG) {
        oStr << target->getDurabilityStr(true);
    } else if (player->isCt() || !Unique::isUnique(target)) {
        oStr << target->getDurabilityStr();

        if( target->flagIsSet(O_TEMP_ENCHANT) &&
            (player->getClass() == CreatureClass::MAGE || player->isCt() || player->isEffected("detect-magic"))
        ) {
            oStr << "It is weakly enchanted.\n";
        }  
    }

    if(target->getSize() != NO_SIZE)
        oStr << "It is ^W" << getSizeName(target->getSize()).c_str() << "^x.\n";

    if(!target->getQuestOwner().empty()) {
        if(target->isQuestOwner(player))
            oStr << "You own "<< target->getObjStr(nullptr, flags, 1) << ".\n";
        else
            oStr << "You do not own "<< target->getObjStr(nullptr, flags, 1) << ".\n";
    }

    if(target->getType() == ObjectType::POTION || target->getType() == ObjectType::HERB) {
        oStr << target->showAlchemyEffects(player);
    }

    player->printColor("%s", oStr.str().c_str());
    return(0);
}

//*********************************************************************
//                      getDamageString
//*********************************************************************

std::string getDamageString(const std::shared_ptr<Creature>& player, std::shared_ptr<Object> weapon, bool critical) {
    if(!weapon) {
        if(player->getClass() == CreatureClass::MONK) {
            switch(critical ? Random::get(11,14) : Random::get(1,10)) {
            case 1:  return "punched";
            case 2:  return "backhanded";
            case 3:  return "smacked";
            case 4:  return "roundhouse kicked";
            case 5:  return "chopped";
            case 6:  return "slapped";
            case 7:  return "whacked";
            case 8:  return "jabbed";
            case 9:  return "throttled";
            case 10: return "clouted";
            case 11: return "decimated";
            case 12: return "annihilated";
            case 13: return "obliterated";
            case 14: return "demolished";
            default:
                break;
            }
        } else if(player->isEffected("lycanthropy")) {
            switch(critical ? Random::get(11,14) : Random::get(1,10)) {
            case 1: return  "clawed";
            case 2: return  "rended";
            case 3: return  "ripped";
            case 4: return  "slashed";
            case 5: return  "ravaged";
            case 6: return  "shredded";
            case 7: return  "thrashed";
            case 8: return  "maimed";
            case 9: return  "mangled";
            case 10: return "lacerated";
            case 11: return "mutilated";
            case 12: return "eviscerated";
            case 13: return "disembowled";
            case 14: return "slaughtered";
            default: return "clawed";
            }
        }

        return "^Rpunched^x";
    } else if(weapon->use_attack[0]) {
        return weapon->use_attack;
    } else {
        return weapon->getWeaponVerbPast().c_str();
    }
    return "slapped";
}

//*********************************************************************
//                      getWeaponDelay
//*********************************************************************

bool Object::showAsSame(const std::shared_ptr<const Player> &player, const std::shared_ptr<const Object> & object) const {
    return( getName() == object->getName() &&
            flagIsSet(O_KEEP) == object->flagIsSet(O_KEEP) &&
            isEffected("invisibility") == object->isEffected("invisibility") &&
            isBroken() == object->isBroken() &&
            flagIsSet(O_BEING_PREPARED) == object->flagIsSet(O_BEING_PREPARED) &&
            (   adjustment == object->adjustment ||
                (!player || !player->isEffected("detect-magic")) ||
                (flagIsSet(O_NULL_MAGIC) && object->flagIsSet(O_NULL_MAGIC))
            ) &&
            (!player || (player && player->canSee(object)))
    );
}

//*********************************************************************
//                      nameCoin
//*********************************************************************

void Object::nameCoin(std::string_view pType, unsigned long pValue) {
    setName(fmt::format("{} {} coin{}", pValue, pType, pValue != 1 ? "s" : ""));
}

//*********************************************************************
//                      killMortalObjectsOnFloor
//*********************************************************************

void BaseRoom::killMortalObjectsOnFloor() {
    std::shared_ptr<UniqueRoom> room = getAsUniqueRoom();

    // if area = shop, kill in main room, but not in storage
    if(room && room->info.isArea("shop") && !room->flagIsSet(R_SHOP))
        return;

    std::shared_ptr<Object>  object=nullptr;
    bool    sunlight = isSunlight(), isStor = room && room->info.isArea("stor");
    ObjectSet::iterator it;
    for( it = objects.begin() ; it != objects.end() ; ) {
        object = (*it++);

        // if area = stor, don't kill in storage chest
        if(isStor && object->info.id == 1 && object->info.isArea("stor"))
            continue;

        if(sunlight && object->isDarkmetal()) {
            broadcast((std::shared_ptr<Socket> )nullptr, getAsRoom(), "^yThe %s^y was destroyed by the sunlight!", object->getCName());
            object->deleteFromRoom();
        } else if(!Unique::canLoad(object)) {
            broadcast((std::shared_ptr<Socket> )nullptr, getAsRoom(), "^yThe %s^y vanishes!", object->getCName());
            object->deleteFromRoom();
        } else
            object->killUniques();
    }
}

//*********************************************************************
//                      killMortalObjects
//*********************************************************************

void BaseRoom::killMortalObjects(bool floor) {
    if(tempNoKillDarkmetal)
        return;
    if(isSunlight()) {
        // kill all players' darkmetal
        for(const auto& pIt: players) {
            if(auto ply = pIt.lock()) {
                ply->killDarkmetal();
            }
        }
        for(const auto& mons : monsters) {
            mons->killDarkmetal();
        }
    }
    if(floor)
        killMortalObjectsOnFloor();
}

//*********************************************************************
//                      killMortalObjects
//*********************************************************************
// if allowed, will kill the darkmetal the target carries

void Creature::killDarkmetal() {
    std::shared_ptr<Player> pTarget = getAsPlayer();
    std::shared_ptr<Object> object=nullptr;
    int     i=0;
    bool    found=false;

    if(!getRoomParent() || !getRoomParent()->isSunlight())
        return;

    if(pTarget) {
        if( pTarget->getSock() == nullptr ||
            pTarget->isStaff() ||
            !pTarget->flagIsSet(P_DARKMETAL))
            return;
    }

    // kill anything not in a bag
    ObjectSet::iterator it;
    for( it = objects.begin() ; it != objects.end() ; ) {
        object = (*it++);
        if(object && object->isDarkmetal()) {
            if(pTarget)
                printColor("^yYour %s was destroyed by the sunlight!\n", object->getCName());
            else if(isPet())
                printColor("^y%M's %s was destroyed by the sunlight!\n", this, object->getCName());
            delObj(object, true, false, false, false);
            found = true;
        }
    }

    // go no further on mobs
    if(!pTarget) {
        checkDarkness();
        return;
    }

    // that includes EQ!
    for(i=0; i<MAXWEAR; i++) {
        if( pTarget->ready[i] &&
            pTarget->ready[i]->isDarkmetal()
        ) {
            printColor("^yYour %s was destroyed by the sunlight!\n", pTarget->ready[i]->getCName());
            // i is wearloc-1, so add 1.   Delete it when done
            pTarget->unequip(i+1, UNEQUIP_DELETE, false);
            found = true;
        }
    }

    if(found) {
        pTarget->checkDarkness();
        pTarget->computeAC();
        pTarget->computeAttackPower();
    }

    pTarget->clearFlag(P_DARKMETAL);
    pTarget->checkDarkness();
}

//*********************************************************************
//                      setTempNoKillDarkmetal
//*********************************************************************

void BaseRoom::setTempNoKillDarkmetal(bool noKillDarkmetal) {
    tempNoKillDarkmetal = noKillDarkmetal;
}

//*********************************************************************
//                      killUniques
//*********************************************************************

void Monster::killUniques() {
    std::shared_ptr<Object>  object=nullptr;
    ObjectSet::iterator it;
    for(it = objects.begin() ; it != objects.end() ; ) {
        object = (*it++);

        if(!Unique::canLoad(object)) {
            delObj(object);
        }
    }
}

void Object::killUniques() {
    std::shared_ptr<Object>  object=nullptr;
    ObjectSet::iterator it;
    for(it = objects.begin() ; it != objects.end() ; ) {
        object = (*it++);

        if(!Unique::canLoad(object)) {
            delObj(object);
        }
    }
}

short Object::getWeight() const { return(weight); }
short Object::getBulk() const { return(bulk); }
short Object::getMaxbulk() const { return(maxbulk); }
Size Object::getSize() const { return(size); }
ObjectType Object::getType() const { return(type); }
short Object::getWearflag() const { return(wearflag); }
short Object::getArmor() const { return(armor); }
short Object::getQuality() const { return(quality); }
short Object::getAdjustment() const { return(adjustment); }
short Object::getNumAttacks() const { return(numAttacks); }
short Object::getShotsMax() const { return(shotsMax); }
short Object::getShotsCur() const { return(shotsCur); }
short Object::getChargesMax() const { return(chargesMax); }
short Object::getChargesCur() const { return(chargesCur); }


float Object::getDurabilityPercent(bool charges) const {
    if(charges) 
        return(getPercentRemaining(chargesCur,chargesMax));
    else
        return(getPercentRemaining(shotsCur,shotsMax));
}

std::string Object::getDurabilityStr(bool charges) const {
    std::string str = "";
    float percent = getDurabilityPercent();

    if( type == ObjectType::WEAPON ||
        type == ObjectType::ARMOR ||
        type == ObjectType::LIGHTSOURCE ||
        type == ObjectType::WAND ||
        type == ObjectType::KEY ||
        type == ObjectType::POISON ||
        type == ObjectType::BANDAGE
    ) {
        if(percent >= 0.9){
            str += "It is in pristine condition.\n";
        } else if(percent >= 0.75) {
            str += "It is in excellent condition.\n";
        } else if(percent >= 0.6) {
            str += "It is in good condition.\n";
        } else if(percent >= 0.45) {
            str += "It has a few scratches.\n";
        } else if(percent >= 0.3) {
            str += "It has many scratches and dents.\n";
        } else if(percent >= 0.1) {
            str += "It is in bad condition.\n";
        } else if(shotsCur) {
            str += "It looks like it could fall apart any moment now.\n";
        } else {
            str += "It is broken or used up.\n";
        }
    }
    
    if(flagIsSet(O_WEAPON_CASTS) && charges) {
        percent = getDurabilityPercent(true);
        if(percent >= 0.9) {
            str += "It shimmers brightly.\n";
        } else if(percent >= 0.75) {
            str += "It has a bright glow about it.\n";
        } else if(percent >= 0.5){
            str += "It has a glow about it.\n";
        } else if(percent >= 0.3) {
            str += "It has a dull glow about it.\n";
        } else if(percent >= 0.1) {
            str += "It has a faint glow about it.\n";
        } else if(chargesCur){
            str += "It has a very faint glow about it.\n";
        } else {
            str += "It no longer glows.\n";
        }
    }
    return str;
}
std::string Object::getDurabilityIndicator() const {
    return progressBar(5, getDurabilityPercent());
}
short Object::getMagicpower() const { return(magicpower); }
short Object::getLevel() const { return(level); }
int Object::getRequiredSkill() const { return(requiredSkill); }
short Object::getMinStrength() const { return(minStrength); }
short Object::getClan() const { return(clan); }
short Object::getSpecial() const { return(special); }
short Object::getQuestnum() const { return(questnum); }
const std::string & Object::getEffect() const { return(effect); }
long Object::getEffectDuration() const { return(effectDuration); }
short Object::getEffectStrength() const { return(effectStrength); }
unsigned long Object::getCoinCost() const { return(coinCost); }
unsigned long Object::getShopValue() const { return(shopValue); }
long Object::getMade() const { return(made); }
int Object::getLotteryCycle() const { return(lotteryCycle); }
short Object::getLotteryNumbers(short i) const {return(lotteryNumbers[i]); }
int Object::getRecipe() const { return(recipe); }
Material Object::getMaterial() const { return(material); }

const std::string & Object::getSubType() const { return(subType); }
short Object::getDelay() const { return(delay); }
short Object::getExtra() const { return(extra); }
short Object::getWeaponDelay() const {
    if(!delay)
        return(DEFAULT_WEAPON_DELAY);
    return(delay);
}
const std::string & Object::getQuestOwner() const { return(questOwner); }

std::string Object::getSizeStr() const{
    return(getSizeName(size));
}

//*********************************************************************
//                      isQuestOwner
//*********************************************************************
// This is only used for determining certain objects for specific reasons
// (usually quest related). This should not be used for general in-my-inventory
// ownership.

bool Object::isQuestOwner(const std::shared_ptr<const Player>& player) const {
    if(questOwner.empty())
        return(true);
    if(questOwner != player->getName())
        return(false);
    // for sui/remake, a player of the same name will fail this check
    if(player->getCreated() > getMade())
        return(false);
    return(true);
}

//*********************************************************************
//                      getWearName
//*********************************************************************

std::string Object::getWearName() {
    switch(wearflag) {
    case 1:
        return("Body");
    case 2:
        return("Arms");
    case 3:
        return("Legs");
    case 4:
        return("Neck");
    case 5:
        return("Belt/Waist");
    case 6:
        return("Hands");
    case 7:
        return("Head");
    case 8:
        return("Feet");
    case 9:
        return("Finger");
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        return("Finger (won't work - set to wearloc 9)");
    case 17:
        return("Hold");
    case 18:
        return("Shield");
    case 19:
        return("Face");
    case 20:
        return("Wielded weapon");
    default:
        return("Unknown");
    }
}

//*********************************************************************
//                      count_obj
//*********************************************************************
// Return the total number of objects contained within an object.
// If perm_only != 0, then only the permanent objects are counted.

int Object::countObj(bool permOnly) {
    int total=0;
    for(const auto& obj : objects) {
        if(!permOnly || (permOnly && (obj->flagIsSet(O_PERM_ITEM))))
            total++;
    }
    return(total);
}
