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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <stdexcept>

#include "commands.hpp"
#include "container.hpp"
#include "config.hpp"
#include "creatures.hpp"
#include "craft.hpp"
#include "mud.hpp"
#include "rooms.hpp"
#include "skills.hpp"
#include "unique.hpp"
#include "xml.hpp"
#include "objects.hpp"

const std::map<ObjectType,const char*> Object::objTypeToString = {
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
};


bstring Object::getTypeName() const {
    return objTypeToString.at(type);
}

bstring Object::getMaterialName() const {
    switch(material) {
        case WOOD:
            return("wood");
        case GLASS:
            return("glass");
        case CLOTH:
            return("cloth");
        case PAPER:
            return("paper");
        case IRON:
            return("iron");
        case STEEL:
            return("steel");
        case MITHRIL:
            return("mithril");
        case ADAMANTIUM:
            return("adamantium");
        case STONE:
            return("stone");
        case ORGANIC:
            return("organic");
        case BONE:
            return("bone");
        case LEATHER:
            return("leather");
        default:
            return("none");
    }
}

//*********************************************************************
//                      cmpName
//*********************************************************************
// used to compare names (alphabetically) to each other when the
// object starts with a color character - chops off the front color
// chars

//char* Object::cmpName() {
//  int i=0;
//  while(name[i] == '^' && name[i+1] != '^')
//      i += 2;
//  return(&name[i]);
//}

//*********************************************************************
//                      add_obj_obj
//*********************************************************************
// This function adds the object toAdd to the current object

void Object::addObj(Object *toAdd, bool incShots) {
    toAdd->validateId();

    Hooks::run(toAdd, "beforeAddObject", this, "beforeAddToObject");

    add(toAdd);
    if(incShots)
        incShotsCur();
    Hooks::run(toAdd, "afterAddObject", this, "afterAddToObject");
}

//*********************************************************************
//                      del_obj_obj
//*********************************************************************
// This function removes the object pointed to by the first parameter
// from the object pointed to by the second.

void Object::delObj(Object  *toDel) {
    Hooks::run(this, "beforeRemoveObject", toDel, "beforeRemoveFromObject");

    decShotsCur();
    toDel->removeFrom();
    Hooks::run(this, "afterRemoveObject", toDel, "afterRemoveFromObject");
}


//*********************************************************************
//                      listObjects
//*********************************************************************
// This function produces a string which lists all the objects in a
// player's, room's or object's inventory.

bool listObjectSee(const Player* player, Object* object, bool showAll) {
    return(player->isStaff() || (
        player->canSee(object) &&
        (showAll || !object->flagIsSet(O_HIDDEN)) &&
        !object->flagIsSet(O_SCENERY)
    ) );
}

bstring Container::listObjects(const Player* player, bool showAll, char endColor) const {
    Object  *object=0;
    int     num=1, n=0, flags = player->displayFlags();
    bstring str = "";

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
    char    m = mrand(1,100);
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
int new_scroll(int level, Object **new_obj) {
    int lvl, realm;


    const char  *delem;
    char    name[128];
    int     num=0;
    char    *p;
    *new_obj = new Object;
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

    lvl = MIN(lvl,(mrand(1,100)<=10 ? 3:2));

    if(mrand(1,100) > 10)
        realm = mrand(EARTH,WATER);
    else
        realm = mrand(EARTH,COLD); // cold/elect rarer



    // No index for this object, need to set the save full flag
    (*new_obj)->setFlag(O_SAVE_FULL);

    (*new_obj)->setMagicpower(ospell[(lvl-1)*6 +(realm-1)].splno+1);
    (*new_obj)->description = "It has arcane runes.";
    delem = " ";

    num = mrand(1,10);
    strcpy(name, scrollDesc[realm-1][num-1]);
    strcat(name, " ");
    num = mrand(1,2);
    strcat(name, scrollType[lvl-1][num-1]);
    (*new_obj)->setName( name);
    p = strtok(name, delem);
    if(p)
        strcpy((*new_obj)->key[0], p);
    p = strtok(nullptr, delem);
    if(p)
        strcpy((*new_obj)->key[1], p);
    p = strtok(nullptr, delem);
    if(p)
        strcpy((*new_obj)->key[2], p);

    (*new_obj)->setWearflag(HELD);
    (*new_obj)->setType(ObjectType::SCROLL);
    (*new_obj)->setWeight(1);
    (*new_obj)->setShotsMax(1);
    (*new_obj)->setShotsCur(1);

    if(lvl >= 4) {
        lvl -= 4;
        lvl *= 6;
        (*new_obj)->setQuestnum(22 + lvl + (realm-1));
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
        if(mrand(1,100) <= 4 && !isPet())
            return(1);
    }
    return(0);
}

void Object::loadContainerContents() {
    int     count=0, a=0;
    Object  *newObj=0;

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
        if(!loadObject(in_bag[a], &newObj))
            continue;

        if(mrand(1,100)<25) {
            newObj->setDroppedBy(this, "ContainerContents");
            addObj(newObj);
        } else {
            delete newObj;
        }
    }
}

//*********************************************************************
//                      cmdKeep
//*********************************************************************

int cmdKeep(Player* player, cmd* cmnd) {
    Object  *object = player->findObject(player, cmnd, 1);

    if(!object) {
        player->print("You don't have that in your inventory.\n");
        return(0);
    }
    if(object->flagIsSet(O_KEEP)) {
        player->print("That's already being kept.\n");
        return(0);
    }

    player->printColor("You will keep %P.\n", object);
    object->setFlag(O_KEEP);
    return(0);
}

//*********************************************************************
//                      cmdUnkeep
//*********************************************************************

int cmdUnkeep(Player* player, cmd* cmnd) {
    Object  *object=0;

    if(!strcmp(cmnd->str[1], "all")) {
        for(Object* obj : player->objects ) {
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

    player->printColor("You no longer are keeping %P.\n", object);
    object->clearFlag(O_KEEP);
    return(0);
}

//*********************************************************************
//                      cantDropInBag
//*********************************************************************

bool cantDropInBag(Object* object) {
    for(Object* obj : object->objects) {
        if(obj->flagIsSet(O_NO_DROP))
            return(true);
    }
    return(false);
}

//*********************************************************************
//                      findObj
//*********************************************************************

MudObject* Creature::findObjTarget(ObjectSet &set, int findFlags, bstring str, int val, int* match) {
    if(set.empty())
        return(nullptr);

    for(Object* obj : set) {
        if(keyTxtEqual(obj, str.c_str()) && canSee(obj)) {
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

int displayObject(Player* player, Object* target) {
    int percent=0;
    unsigned int i=0;
    char str[2048];
    char filename[256];
    bstring inv = "";
    bstring requiredSkillString = "";

    int flags = player->displayFlags();

    // special 2 is a combo lock, should have normal descriptions
    if(target->getSpecial() == 1) {
        strcpy(str, target->getCName());
        for(i=0; i<strlen(str); i++)
            if(str[i] == ' ')
                str[i] = '_';
        sprintf(filename, "%s/%s.txt", Path::Sign, str);
        if(target->flagIsSet(O_LOGIN_FILE))
            viewLoginFile(player->getSock(), filename);
        else
            viewFile(player->getSock(), filename);
        return(0);
    }
    std::ostringstream oStr;

    if(target->description != "") {
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
        if(inv != "")
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

    if(target->getRequiredSkill() && !requiredSkillString.empty()) {
    	oStr << "It requires minimum proficiency of '" << getSkillLevelStr(target->getRequiredSkill()) << "' with " << requiredSkillString << ".\n";
    }

    if(target->flagIsSet(O_NO_DROP))
        oStr << target->getObjStr(nullptr, flags | CAP, 1) << " cannot be dropped.\n";

    if(Lore::isLore(target))
        oStr << target->getObjStr(nullptr, flags | CAP, 1) << " is an object of lore.\n";

    if(target->flagIsSet(O_DARKMETAL))
        oStr << "^yIt is vulnerable to sunlight.\n";

    if(target->flagIsSet(O_DARKNESS))
        oStr << "^DIt is engulfed by an aura of darkness.\n";

    if(Unique::isUnique(target))
        oStr << target->getObjStr(nullptr, flags | CAP, 1) << " is a unique or limited object.\n";


    if(target->getType() == ObjectType::WEAPON && target->flagIsSet(O_ENVENOMED)) {
        oStr << "^gIt drips with poison.\n";
        if((player->getClass() == CreatureClass::ASSASSIN && player->getLevel() >= 10) || player->isCt()) {
            oStr << "^gTime remaining before poison deludes: " <<
               timestr(MAX<long>(0,(target->lasttime[LT_ENVEN].ltime+target->lasttime[LT_ENVEN].interval-time(0)))) << ".\n";
        }
    }

    if( target->getType() == ObjectType::POISON && ((player->getClass() == CreatureClass::ASSASSIN && player->getLevel() >= 10) || player->isCt()))
        oStr << target->getObjStr(nullptr, flags | CAP, 1) << " is a poison.\n";

    if(target->flagIsSet(O_COIN_OPERATED_OBJECT))
        oStr << target->getObjStr(nullptr, flags | CAP, 1) << " costs " << target->getCoinCost() << "gold coins per use.\n";

    if(target->getShotsCur() > 0)
        percent = (100 * (target->getShotsCur())) / (MAX<short>(target->getShotsMax(),1));
    else
        percent = -1;

    if(player->isCt() || !Unique::isUnique(target)) {
        if( target->getType() == ObjectType::WEAPON ||
            target->getType() == ObjectType::ARMOR ||
            target->getType() == ObjectType::LIGHTSOURCE ||
            target->getType() == ObjectType::WAND ||
            target->getType() == ObjectType::KEY ||
            target->getType() == ObjectType::POISON ||
            target->getType() == ObjectType::BANDAGE
        ) {
            if(percent >= 90)
                oStr << "It is in pristine condition.\n";
            else if(percent >= 75)
                oStr << "It is in excellent condition.\n";
            else if(percent >= 60)
                oStr << "It is in good condition.\n";
            else if(percent >= 45)
                oStr << "It has a few scratches.\n";
            else if(percent >= 30)
                oStr << "It has many scratches and dents.\n";
            else if(percent >= 10)
                oStr << "It is in bad condition.\n";
            else if(target->getShotsCur())
                oStr << "It looks like it could fall apart any moment now.\n";
            else
                oStr << "It is broken or used up.\n";
        }

        if(target->flagIsSet(O_TEMP_ENCHANT) && (player->getClass() == CreatureClass::MAGE
                || player->isCt() || player->isEffected("detect-magic")))
            oStr << "It is weakly enchanted.\n";

    }

    if(target->flagIsSet(O_WEAPON_CASTS) && flags && MAG) {
        if(target->getChargesCur() > 0)
            percent = (100 * (target->getChargesCur())) / (MAX<short>(target->getChargesMax(),1));
        else
            percent = -1;

        if(percent >= 90)
            oStr << "It shimmers brightly.\n";
        else if(percent >= 75)
            oStr << "It has a bright glow about it.\n";
        else if(percent >= 50)
            oStr << "It has a glow about it.\n";
        else if(percent >= 30)
            oStr << "It has a dull glow about it.\n";
        else if(percent >= 10)
            oStr << "It has a faint glow about it.\n";
        else if(target->getChargesCur())
            oStr << "It has a very faint glow about it.\n";
        else
            oStr << "It no longer glows.\n";
    }


    if(target->getSize() != NO_SIZE)
        oStr << "It is ^W" << getSizeName(target->getSize()).c_str() << "^x.\n";

    if(target->getQuestOwner() != "") {
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

void getDamageString(char atk[50], Creature* player, Object *weapon, bool critical) {
    if(!weapon) {
        strcpy(atk, "^Rpunched^x");

        if(player->getClass() == CreatureClass::MONK) {
            switch(critical ? mrand(10,12) : mrand(1,9)) {
            case 1: strcpy(atk, "punched");             break;
            case 2: strcpy(atk, "backhanded");          break;
            case 3: strcpy(atk, "smacked");             break;
            case 4: strcpy(atk, "roundhouse kicked");   break;
            case 5: strcpy(atk, "chopped");             break;
            case 6: strcpy(atk, "slapped");             break;
            case 7: strcpy(atk, "whacked");             break;
            case 8: strcpy(atk, "jabbed");              break;
            case 9: strcpy(atk, "throttled");           break;
            case 10: strcpy(atk, "decimated");          break;
            case 11: strcpy(atk, "annihilated");        break;
            case 12: strcpy(atk, "obliterated");        break;
            default:
                break;
            }
        } else if(player->isEffected("lycanthropy")) {
            switch(critical ? mrand(10,12) : mrand(1,9)) {
            case 1: strcpy(atk, "clawed");          break;
            case 2: strcpy(atk, "rended");          break;
            case 3: strcpy(atk, "ripped");          break;
            case 4: strcpy(atk, "slashed");         break;
            case 5: strcpy(atk, "cleaved");         break;
            case 6: strcpy(atk, "shredded");        break;
            case 7: strcpy(atk, "thrashed");        break;
            case 8: strcpy(atk, "maimed");          break;
            case 9: strcpy(atk, "mangled");         break;
            case 10: strcpy(atk, "mutilated");      break;
            case 11: strcpy(atk, "disembowled");    break;
            case 12: strcpy(atk, "slaughtered");    break;
            default: strcpy(atk, "clawed");         break;
            }
        }
    } else if(weapon->use_attack[0]) {
        strcpy(atk, weapon->use_attack);
    } else {
        strcpy(atk, weapon->getWeaponVerbPast().c_str());
    }
}

//*********************************************************************
//                      getWeaponDelay
//*********************************************************************

bool Object::showAsSame(const Player* player, const Object* object) const {
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

void Object::nameCoin(bstring type, unsigned long value) {
    char temp[80];
    snprintf(temp, 80, "%lu %s coin%s", value, type.c_str(), value != 1 ? "s" : "");
    setName(temp);
}

//*********************************************************************
//                      killMortalObjectsOnFloor
//*********************************************************************

void BaseRoom::killMortalObjectsOnFloor() {
    UniqueRoom* room = getAsUniqueRoom();

    // if area = shop, kill in main room, but not in storage
    if(room && room->info.isArea("shop") && !room->flagIsSet(R_SHOP))
        return;

    Object* object=0;
    bool    sunlight = isSunlight(), isStor = room && room->info.isArea("stor");
    ObjectSet::iterator it;
    for( it = objects.begin() ; it != objects.end() ; ) {
        object = (*it++);

        // if area = stor, don't kill in storage chest
        if(isStor && object->info.id == 1 && object->info.isArea("stor"))
            continue;

        if(sunlight && object->flagIsSet(O_DARKMETAL)) {
            broadcast(nullptr, this, "^yThe %s^y was destroyed by the sunlight!", object->getCName());
            object->deleteFromRoom();
            delete object;
        } else if(!Unique::canLoad(object)) {
            broadcast(nullptr, this, "^yThe %s^y vanishes!", object->getCName());
            object->deleteFromRoom();
            delete object;
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
        for(Player* ply : players) {
            ply->killDarkmetal();
        }
        for(Monster* mons : monsters) {
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
    Player  *pTarget = getAsPlayer();
    Object  *object=0;
    int     i=0;
    bool    found=false;

    if(!getRoomParent()->isSunlight())
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
        if(object && object->flagIsSet(O_DARKMETAL)) {
            if(pTarget)
                printColor("^yYour %s was destroyed by the sunlight!\n", object->getCName());
            else if(isPet())
                printColor("^y%M's %s was destroyed by the sunlight!\n", this, object->getCName());
            delObj(object, true, false, false, false);
            delete object;
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
            pTarget->ready[i]->flagIsSet(O_DARKMETAL)
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
    Object* object=0;
    ObjectSet::iterator it;
    for(it = objects.begin() ; it != objects.end() ; ) {
        object = (*it++);

        if(!Unique::canLoad(object)) {
            delObj(object);
            delete object;
        }
    }
}

void Object::killUniques() {
    Object* object=0;
    ObjectSet::iterator it;
    for(it = objects.begin() ; it != objects.end() ; ) {
        object = (*it++);

        if(!Unique::canLoad(object)) {
            delObj(object);
            delete object;
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
short Object::getMagicpower() const { return(magicpower); }
short Object::getLevel() const { return(level); }
int Object::getRequiredSkill() const { return(requiredSkill); }
short Object::getMinStrength() const { return(minStrength); }
short Object::getClan() const { return(clan); }
short Object::getSpecial() const { return(special); }
short Object::getQuestnum() const { return(questnum); }
bstring Object::getEffect() const { return(effect); }
long Object::getEffectDuration() const { return(effectDuration); }
short Object::getEffectStrength() const { return(effectStrength); }
unsigned long Object::getCoinCost() const { return(coinCost); }
unsigned long Object::getShopValue() const { return(shopValue); }
long Object::getMade() const { return(made); }
int Object::getLotteryCycle() const { return(lotteryCycle); }
short Object::getLotteryNumbers(short i) const {return(lotteryNumbers[i]); }
int Object::getRecipe() const { return(recipe); }
Material Object::getMaterial() const { return(material); }

const bstring Object::getSubType() const { return(subType); }
short Object::getDelay() const { return(delay); }
short Object::getExtra() const { return(extra); }
short Object::getWeaponDelay() const {
    if(!delay)
        return(DEFAULT_WEAPON_DELAY);
    return(delay);
}
bstring Object::getQuestOwner() const { return(questOwner); }

bstring Object::getSizeStr() const{
    return(getSizeName(size));
}

//*********************************************************************
//                      isQuestOwner
//*********************************************************************
// This is only used for determining certain objects for specific reasons
// (usually quest related). This should not be used for general in-my-inventory
// ownership.

bool Object::isQuestOwner(const Player* player) const {
    if(questOwner == "")
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

bstring Object::getWearName() {
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
    for(Object* obj : objects) {
        if(!permOnly || (permOnly && (obj->flagIsSet(O_PERM_ITEM))))
            total++;
    }
    return(total);
}
