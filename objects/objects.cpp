/*
 * objects.cpp
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
#include <cstring>                     // for strlen, strcpy, memset, strcmp
#include <list>                        // for list, list<>::const_iterator
#include <map>                         // for operator==, _Rb_tree_const_ite...
#include <ostream>                     // for operator<<, basic_ostream, ost...
#include <set>                         // for set<>::iterator, set
#include <string>                      // for string, operator==, char_traits
#include <string_view>                 // for string_view, operator<<
#include <utility>                     // for pair

#include "area.hpp"                    // for MapMarker, Area
#include "catRef.hpp"                  // for CatRef
#include "clans.hpp"                   // for Clan
#include "commands.hpp"                // for getFullstrTextTrun
#include "config.hpp"                  // for Config, gConfig
#include "dice.hpp"                    // for Dice
#include "flags.hpp"                   // for O_NULL_MAGIC, O_SOME_PREFIX
#include "global.hpp"                  // for CreatureClass, CreatureClass::...
#include "hooks.hpp"                   // for Hooks
#include "lasttime.hpp"                // for lasttime
#include "money.hpp"                   // for Money
#include "mudObjects/areaRooms.hpp"    // for AreaRoom
#include "mudObjects/container.hpp"    // for ObjectSet
#include "mudObjects/creatures.hpp"    // for Creature
#include "mudObjects/exits.hpp"        // for Exit
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/mudObject.hpp"    // for MudObject
#include "mudObjects/objects.hpp"      // for Object, DroppedBy, ObjectType
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "objIncrease.hpp"             // for ObjIncrease
#include "proto.hpp"                   // for getCatRef, int_to_text, broadcast
#include "random.hpp"                  // for Random
#include "range.hpp"                   // for Range
#include "server.hpp"                  // for Server, gServer
#include "size.hpp"                    // for NO_SIZE
#include "stats.hpp"                   // for Stat
#include "structs.hpp"                 // for SEX_FEMALE, SEX_MALE, SEX_NONE
#include "xml.hpp"                     // for loadObject, loadRoom


bool Object::operator< (const Object& t) const {
    return(getCompareStr().compare(t.getCompareStr()) < 0);
}

std::string Object::getCompareStr() const {
    return(fmt::format("{}-{}-{}-{}", getName(), adjustment, shopValue, getId()));
}

const std::string & DroppedBy::getName() const {
    return(name);
}
const std::string & DroppedBy::getIndex() const {
    return(index);
}
const std::string & DroppedBy::getId() const {
    return(id);
}
const std::string & DroppedBy::getType() const {
    return(type);
}
void DroppedBy::clear() {
    name.clear();
    index.clear();
    id.clear();
    type.clear();
}

//*********************************************************************
//                      operator<<
//*********************************************************************

std::ostream& operator<<(std::ostream& out, const DroppedBy& drop) {
    out << drop.name << " (" << drop.index;
    if(!drop.id.empty()) {
        if(!drop.index.empty())
            out << ":";
        out << drop.id;
    }
    out << ") [" << drop.type << "]";
    return(out);
}

std::string DroppedBy::str() {
    std::ostringstream oStr;
    oStr << name << " (" << index;
    if(!id.empty()) {
        if(!index.empty())
            oStr << ":";
        oStr << id;
    }
    oStr << ") [" << type << "]";
    return(oStr.str());
}

void Object::validateId() {
    if(id.empty() || id == "-1") {
        setId(gServer->getNextObjectId());
    }
}

void Object::setDroppedBy(const std::shared_ptr<MudObject>& dropper, std::string_view pDropType) {

    droppedBy.name = dropper->getName();
    droppedBy.id = dropper->getId();

    auto mDropper = dynamic_pointer_cast<Monster>(dropper);
    auto oDropper = dynamic_pointer_cast<Object>(dropper);
    auto rDropper = dynamic_pointer_cast<UniqueRoom>(dropper);
    auto aDropper = dynamic_pointer_cast<AreaRoom>(dropper);

    if(mDropper) {
        droppedBy.index = mDropper->info.str();
    } else if(oDropper) {
        droppedBy.index = oDropper->info.str();
    } else if(rDropper) {
        droppedBy.index = rDropper->info.str();
    } else if(aDropper) {
        if(auto area = aDropper->area.lock()) {
            droppedBy.index = area->name + aDropper->fullName();
        }
    }
    else {
        droppedBy.index = "";
    }

    droppedBy.type = pDropType;
}
//*********************************************************************
//                          Object
//*********************************************************************

Object::Object() {
    int i;

    id = "-1";
    version = "0.00";
    description = effect = "";
    memset(key, 0, sizeof(key));
    memset(use_output, 0, sizeof(use_output));
    memset(use_attack, 0, sizeof(use_attack));
    type = ObjectType::MISC;
    weight = adjustment = shotsMax = shotsCur = armor =
        wearflag = magicpower = level = requiredSkill = clan =
        special = delay = quality = effectStrength = effectDuration = chargesCur = chargesMax = 0;
    flags.reset();
    questnum = 0;
    numAttacks = bulk = maxbulk = lotteryCycle = 0;
    coinCost = shopValue = 0;
    size = NO_SIZE;

    for(i=0; i<6; i++)
        lotteryNumbers[i] = 0;

    material = NO_MATERIAL;
    keyVal = minStrength = 0;
    compass = nullptr;
    increase = nullptr;
    recipe = 0;
    made = 0;
    extra = 0;
}


Object::Object(Object& o): MudObject(o) {
    objCopy(o);
}
Object::Object(const Object& o): MudObject(o) {
    objCopy(o);
}


Object::~Object() {
    delete compass;
    delete increase;
    alchemyEffects.clear();
    compass = nullptr;
    objects.clear();
    gServer->removeDelayedActions(this);
}

//*********************************************************************
//                          init
//*********************************************************************
// Initiate the object - used when the object is first introduced into the
// world.

void Object::init(bool selRandom) {
    // this object may turn into another object
    if(selRandom)
        selectRandom();

    if(flagIsSet(O_RANDOM_ENCHANT))
        randomEnchant();

    loadContainerContents();
    setMade();
    setFlag(O_JUST_LOADED);
}

//*********************************************************************
//                          getNewPotion
//*********************************************************************

std::shared_ptr<Object> Object::getNewPotion() {
    std::shared_ptr<Object> newPotion = std::make_shared<Object>();

    newPotion->type = ObjectType::POTION;
    newPotion->setName( "generic potion");
    strcpy(newPotion->key[0], "generic");
    strcpy(newPotion->key[1], "potion");
    newPotion->weight = 1;
    newPotion->setFlag(O_SAVE_FULL);

    return newPotion;
}

//*********************************************************************
//                          doCopy
//*********************************************************************

void Object::objCopy(const Object& o) {
    int     i=0;

    description = o.description;

    droppedBy = o.droppedBy;

    plural = o.plural;
    for(i=0; i<3; i++)
        strcpy(key[i], o.key[i]);
    strcpy(use_output, o.use_output);
    strcpy(use_attack, o.use_attack);
    version = o.version;
    weight = o.weight;
    type = o.type;
    adjustment = o.adjustment;
    chargesMax = o.chargesMax;
    chargesCur = o.chargesCur;
    shotsMax = o.shotsMax;
    shotsCur = o.shotsCur;
    damage = o.damage;
    armor = o.armor;
    wearflag = o.wearflag;
    magicpower = o.magicpower;
    info = o.info;
    level = o.level;
    quality = o.quality;
    requiredSkill = o.requiredSkill;
    clan = o.clan;
    special = o.special;
    flags = o.flags;
    questnum = o.questnum;
//  parent = o.parent;
    for(i=0; i<4; i++)
        lasttime[i] = o.lasttime[i];

    for(i=0; i<3; i++)
        in_bag[i] = o.in_bag[i];

    lastMod = o.lastMod;

    bulk = o.bulk;
    delay = o.delay;
    maxbulk = o.maxbulk;
    numAttacks = o.numAttacks;

    lotteryCycle = o.lotteryCycle;

    coinCost = o.coinCost;
    deed = o.deed;
    shopValue = o.shopValue;

    value.set(o.value);

    made = o.made;
    keyVal = o.keyVal;
    extra = o.extra;
    questOwner = o.questOwner;

    minStrength = o.minStrength;
    material = o.material;

    for(i=0; i<6; i++)
        lotteryNumbers[i] = o.lotteryNumbers[i];
    refund.set(o.refund);
    size = o.size;

    if(o.compass) {
        if(compass != nullptr)
            delete compass;
        compass = new MapMarker;
        *compass = *o.compass;
    }
    if(o.increase) {
        if(increase != nullptr)
            delete increase;
        increase = new ObjIncrease;
        *increase = *o.increase;
    }

    recipe = o.recipe;
    effect = o.effect;
    effectDuration = o.effectDuration;
    effectStrength = o.effectStrength;

    // copy everything contained inside this object
    std::shared_ptr<Object>newObj;
    for(const auto& obj : o.objects) {
        newObj = std::make_shared<Object>(*obj);
        addObj(newObj, false);
    }
    for(const auto& p : o.alchemyEffects) {
        alchemyEffects[p.first] = p.second;
    }
    subType = o.subType;

    std::list<CatRef>::const_iterator it;
    randomObjects.clear();
    for(it = o.randomObjects.begin() ; it != o.randomObjects.end() ; it++) {
        randomObjects.push_back(*it);
    }
}

DroppedBy& DroppedBy::operator=(const DroppedBy& o) {
    if(this == &o)
        return(*this);

    name = o.name;
    index = o.index;
    id = o.id;
    type = o.type;
    return(*this);
}

bool Object::operator==(const Object& o) const {
    int     i=0;

    if( description != o.description ||
        version != o.version ||
        weight != o.weight ||
        type != o.type ||
        adjustment != o.adjustment ||
        shotsMax != o.shotsMax ||
        shotsCur != o.shotsCur ||
        damage != o.damage ||
        armor != o.armor ||
        wearflag != o.wearflag ||
        magicpower != o.magicpower ||
        info != o.info ||
        level != o.level ||
        quality != o.quality ||
        requiredSkill != o.requiredSkill ||
        clan != o.clan ||
        special != o.special ||
        questnum != o.questnum ||
        lastMod != o.lastMod ||
        bulk != o.bulk ||
        delay != o.delay ||
        maxbulk != o.maxbulk ||
        numAttacks != o.numAttacks ||
        lotteryCycle != o.lotteryCycle ||
        coinCost != o.coinCost ||
        deed != o.deed ||
        shopValue != o.shopValue ||
        made != o.made ||
        keyVal != o.keyVal ||
        extra != o.extra ||
        questOwner != o.questOwner ||
        minStrength != o.minStrength ||
        material != o.material ||
        size != o.size ||
        recipe != o.recipe ||
        effect != o.effect ||
        effectDuration != o.effectDuration ||
        effectStrength != o.effectStrength ||
        subType != o.subType ||
        randomObjects.size() != o.randomObjects.size() ||
        alchemyEffects.size() != o.alchemyEffects.size() ||
        !objects.empty() ||
        !o.objects.empty() ||
        strcmp(use_output, o.use_output) != 0 ||
        strcmp(use_attack, o.use_attack) != 0 ||
        value != o.value ||
        refund != o.refund
    )
        return(false);

    for(i=0; i<3; i++)
        if(strcmp(key[i], o.key[i]) != 0)
            return(false);
    if(flags != o.flags)
        return(false);
    for(i=0; i<4; i++) {
        if( lasttime[i].interval != o.lasttime[i].interval ||
            lasttime[i].ltime != o.lasttime[i].ltime ||
            lasttime[i].misc != o.lasttime[i].misc
        )
            return(false);
    }
    for(i=0; i<3; i++)
        if(in_bag[i] != o.in_bag[i])
            return(false);
    for(i=0; i<6; i++)
        if(lotteryNumbers[i] != o.lotteryNumbers[i])
            return(false);

    if(compass && o.compass) {
        if(*compass != *o.compass)
            return(false);
    } else if(compass || o.compass)
        return(false);

    /*
    for(std::pair<int, std::string> p : o.alchemyEffects) {
        alchemyEffects.insert(p);
    }
    */

    std::list<CatRef>::const_iterator rIt, orIt;
    rIt = randomObjects.begin();
    orIt = o.randomObjects.begin();
    for(; rIt != randomObjects.end() ;) {
        if(*rIt != *orIt)
            return(false);
        rIt++;
        orIt++;
    }
    return(true);
}
bool Object::operator!=(const Object& o) const {
    return(!(*this==o));
}

//*********************************************************************
//                          getActualWeight
//*********************************************************************
// This function computes the total amount of weight of an object and
// all its contents.

int Object::getActualWeight() const {
    int     n=0;

    n = weight;

    if(!flagIsSet(O_WEIGHTLESS_CONTAINER)) {
        for(const auto& obj : objects) {
            n += obj->getActualWeight();
        }
    }

    return(n);
}

//*********************************************************************
//                          getActualBulk
//*********************************************************************

int Object::getActualBulk() const {
    int     n=0;

    if(flagIsSet(O_BULKLESS_OBJECT))
        return(0);

    if(bulk <= 0) {
        switch(type) {
        case    ObjectType::WEAPON:
            {
                std::string category = getWeaponCategory();
                if(category == "crushing")
                    n = 5;
                else if(category == "piercing")
                    n = 4;
                else if(category == "slashing")
                    n = 4;
                else if(category == "ranged")
                    n = 6;
                else if(category == "chopping")
                    n = 8;
                else
                    n = 4;
            }
            break;
        case    ObjectType::MONEY:
        case    ObjectType::POISON:
        case    ObjectType::POTION:
            n = 4;
            break;
        case ObjectType::ARMOR:
            switch(wearflag) {
            case BODY:
                n = 20;
                break;
            case ARMS:
                n = 12;
                break;
            case LEGS:
                n = 14;
                break;
            case NECK:
                n = 6;
                break;
            case BELT:
            case HANDS:
            case HEAD:
            case FEET:
            case HELD:
            case FACE:
                n = 4;
                break;
            case FINGER:

            case FINGER2:
            case FINGER3:
            case FINGER4:
            case FINGER5:
            case FINGER6:
            case FINGER7:
            case FINGER8:
                n = 1;
                break;
            case SHIELD:
                n = 10;
                break;
            default:
                n =1;
                break;
            }
            break;
        case ObjectType::SCROLL:
        case ObjectType::WAND:
        case ObjectType::SONGSCROLL:
        case ObjectType::MISC:
            n = 3;
            break;
        case ObjectType::KEY:
            n = 1;
            break;
        case ObjectType::CONTAINER:
            n = 5;
            break;
        case ObjectType::LIGHTSOURCE:
        case ObjectType::BANDAGE:
            n = 2;
            break;
        }
    } else
        n = bulk;

    return(n);
}

//*********************************************************************
//                          raceRestrict
//*********************************************************************
// become a random object

void Object::selectRandom() {
    std::list<CatRef>::const_iterator it;
    CatRef cr;
    std::shared_ptr<Object>  object=nullptr;
    int which = randomObjects.size();

    // Load all means we will become every object in this list (trade only)
    // rather than a random one. Thus, don't use this code.
    if(!which || flagIsSet(O_LOAD_ALL))
        return;
    which = Random::get(1, which);

    for(it = randomObjects.begin(); it != randomObjects.end(); it++) {
        which--;
        if(!which) {
            cr = *it;
            break;
        }
    }

    if(!loadObject(cr, object))
        return;


    object->setDroppedBy(Containable::downcasted_shared_from_this<Object>(), "RandomItemParent");
    *this = *object;
    
    object.reset();
}

//*********************************************************************
//                          raceRestrict
//*********************************************************************

bool Object::raceRestrict(const std::shared_ptr<Creature> & creature, bool p) const {
    if(raceRestrict(creature)) {
        if(p) creature->checkStaff("Your race prevents you from using %P.\n", this);
        if(!creature->isStaff()) return(true);
    }
    return(false);
}

bool Object::raceRestrict(const std::shared_ptr<const Creature> & creature) const {
    bool pass = false;

    // if no flags are set
    if( !flagIsSet(O_SEL_DWARF) &&
        !flagIsSet(O_SEL_ELF) &&
        !flagIsSet(O_SEL_HALFELF) &&
        !flagIsSet(O_SEL_HALFLING) &&
        !flagIsSet(O_SEL_HUMAN) &&
        !flagIsSet(O_SEL_ORC) &&
        !flagIsSet(O_SEL_HALFGIANT) &&
        !flagIsSet(O_SEL_GNOME) &&
        !flagIsSet(O_SEL_TROLL) &&
        !flagIsSet(O_SEL_HALFORC) &&
        !flagIsSet(O_SEL_OGRE) &&
        !flagIsSet(O_SEL_DARKELF) &&
        !flagIsSet(O_SEL_GOBLIN) &&
        !flagIsSet(O_SEL_MINOTAUR) &&
        !flagIsSet(O_SEL_SERAPH) &&
        !flagIsSet(O_SEL_KOBOLD) &&
        !flagIsSet(O_SEL_CAMBION) &&
        !flagIsSet(O_SEL_BARBARIAN) &&
        !flagIsSet(O_SEL_KATARAN) &&
        !flagIsSet(O_SEL_TIEFLING) &&
        !flagIsSet(O_SEL_KENKU) &&
        !flagIsSet(O_RSEL_INVERT)
    )
        return(false);

    // if the race flag is set and they match, they pass
    pass = (
        (flagIsSet(O_SEL_DWARF) && creature->isRace(DWARF)) ||
        (flagIsSet(O_SEL_ELF) && creature->isRace(ELF)) ||
        (flagIsSet(O_SEL_HALFELF) && creature->isRace(HALFELF)) ||
        (flagIsSet(O_SEL_HALFLING) && creature->isRace(HALFLING)) ||
        (flagIsSet(O_SEL_HUMAN) && creature->isRace(HUMAN)) ||
        (flagIsSet(O_SEL_ORC) && creature->isRace(ORC)) ||
        (flagIsSet(O_SEL_HALFGIANT) && creature->isRace(HALFGIANT)) ||
        (flagIsSet(O_SEL_GNOME) && creature->isRace(GNOME)) ||
        (flagIsSet(O_SEL_TROLL) && creature->isRace(TROLL)) ||
        (flagIsSet(O_SEL_HALFORC) && creature->isRace(HALFORC)) ||
        (flagIsSet(O_SEL_OGRE) && creature->isRace(OGRE)) ||
        (flagIsSet(O_SEL_DARKELF) && creature->isRace(DARKELF)) ||
        (flagIsSet(O_SEL_GOBLIN) && creature->isRace(GOBLIN)) ||
        (flagIsSet(O_SEL_MINOTAUR) && creature->isRace(MINOTAUR)) ||
        (flagIsSet(O_SEL_SERAPH) && creature->isRace(SERAPH)) ||
        (flagIsSet(O_SEL_KOBOLD) && creature->isRace(KOBOLD)) ||
        (flagIsSet(O_SEL_CAMBION) && creature->isRace(CAMBION)) ||
        (flagIsSet(O_SEL_BARBARIAN) && creature->isRace(BARBARIAN)) ||
        (flagIsSet(O_SEL_KATARAN) && creature->isRace(KATARAN)) ||
        (flagIsSet(O_SEL_TIEFLING) && creature->isRace(TIEFLING)) ||
        (flagIsSet(O_SEL_KENKU) && creature->isRace(KENKU))
    );

    if(flagIsSet(O_RSEL_INVERT)) pass = !pass;

    return(!pass);
}

//*********************************************************************
//                          classRestrict
//*********************************************************************
// We need more information here - we can't just pass/fail them like
// we usually do.

bool Object::classRestrict(const std::shared_ptr<Creature> & creature, bool p) const {
    if(classRestrict(creature)) {
        if(p) creature->checkStaff("Your class prevents you from using %P.\n", this);
        if(!creature->isStaff()) return(true);
    }
    return(false);
}

bool Object::classRestrict(const std::shared_ptr<const Creature> & creature) const {
    bool pass = false;
    const std::shared_ptr<const Player> player = creature->getAsConstPlayer();

    CreatureClass cClass = creature->getClass();
    if(player && player->getClass() == CreatureClass::MAGE && (player->getSecondClass() == CreatureClass::ASSASSIN || player->getSecondClass() == CreatureClass::THIEF))
        cClass = player->getSecondClass();

    if(flagIsSet(O_NO_MAGE) && (cClass == CreatureClass::MAGE || cClass == CreatureClass::LICH))
        return(true);

    // if no flags are set
    if( !flagIsSet(O_SEL_ASSASSIN) &&
        !flagIsSet(O_SEL_BERSERKER) &&
        !flagIsSet(O_SEL_CLERIC) &&
        !flagIsSet(O_SEL_FIGHTER) &&
        !flagIsSet(O_SEL_MAGE) &&
        !flagIsSet(O_SEL_PALADIN) &&
        !flagIsSet(O_SEL_RANGER) &&
        !flagIsSet(O_SEL_THIEF) &&
        !flagIsSet(O_SEL_VAMPIRE) &&
        !flagIsSet(O_SEL_MONK) &&
        !flagIsSet(O_SEL_DEATHKNIGHT) &&
        !flagIsSet(O_SEL_DRUID) &&
        !flagIsSet(O_SEL_LICH) &&
        !flagIsSet(O_SEL_WEREWOLF) &&
        !flagIsSet(O_SEL_BARD) &&
        !flagIsSet(O_SEL_ROGUE) &&
        !flagIsSet(O_CSEL_INVERT)
    ) {
        // we need to do some special rules before we say they can use the item

        // only blunts for monks
        if(wearflag == WIELD && cClass == CreatureClass::MONK && getWeaponCategory() != "crushing")
            return(true);

        // only sharps for wolves
        if(wearflag == WIELD && creature->isEffected("lycanthropy") && getWeaponCategory() != "slashing")
            return(true);

        if(type == ObjectType::ARMOR && (cClass == CreatureClass::MONK || creature->isEffected("lycanthropy")))
            return(true);

        // no rings or shields for monk/wolf/lich
        if( (   wearflag == FINGER ||
                wearflag == SHIELD
            ) &&
            (   cClass == CreatureClass::MONK ||
                creature->isEffected("lycanthropy") ||
                cClass == CreatureClass::LICH
            )
        )
            return(true);

        return(false);
    }

    // if the class flag is set and they match, they pass
    pass = (
        (flagIsSet(O_SEL_ASSASSIN) && cClass == CreatureClass::ASSASSIN) ||
        (flagIsSet(O_SEL_BERSERKER) && cClass == CreatureClass::BERSERKER) ||
        (flagIsSet(O_SEL_CLERIC) && cClass == CreatureClass::CLERIC) ||
        (flagIsSet(O_SEL_FIGHTER) && cClass == CreatureClass::FIGHTER) ||
        (flagIsSet(O_SEL_MAGE) && cClass == CreatureClass::MAGE) ||
        (flagIsSet(O_SEL_PALADIN) && cClass == CreatureClass::PALADIN) ||
        (flagIsSet(O_SEL_RANGER) && cClass == CreatureClass::RANGER) ||
        (flagIsSet(O_SEL_THIEF) && cClass == CreatureClass::THIEF) ||
        (flagIsSet(O_SEL_VAMPIRE) && creature->isEffected("vampirism")) ||
        (flagIsSet(O_SEL_MONK) && cClass == CreatureClass::MONK) ||
        (flagIsSet(O_SEL_DEATHKNIGHT) && cClass == CreatureClass::DEATHKNIGHT) ||
        (flagIsSet(O_SEL_DRUID) && cClass == CreatureClass::DRUID) ||
        (flagIsSet(O_SEL_LICH) && cClass == CreatureClass::LICH) ||
        (flagIsSet(O_SEL_WEREWOLF) && creature->isEffected("lycanthropy")) ||
        (flagIsSet(O_SEL_BARD) && cClass == CreatureClass::BARD) ||
        (flagIsSet(O_SEL_ROGUE) && cClass == CreatureClass::ROGUE)
    );

    if(flagIsSet(O_CSEL_INVERT)) pass = !pass;

    return(!pass);
}


//*********************************************************************
//                          levelRestrict
//*********************************************************************

bool Object::levelRestrict(const std::shared_ptr<Creature> & creature, bool p) const {
    if(level > creature->getLevel()) {
        if(p) creature->checkStaff("You are not experienced enough to use that.\n");
        if(!creature->isStaff()) return(true);
    }
    return(false);
}

bool Object::skillRestrict(const std::shared_ptr<Creature> & creature, bool p) const {
    int skillLevel = 0;

    std::string skill = "";

    if(type == ObjectType::ARMOR) {
        skill = getArmorType();
        if(skill == "shield" || skill == "ring")
            skill = "";
    } else if(type == ObjectType::WEAPON) {
        skill = getWeaponType();
    } else if(flagIsSet(O_FISHING)) {
        skill = "fishing";
    }
    if(!skill.empty()) {
        skillLevel = (int)creature->getSkillGained(skill);
        if(requiredSkill > skillLevel) {
            if(p) creature->checkStaff("You do not have enough training in ^W%s%s^x to use that!\n", skill.c_str(), type == ObjectType::ARMOR ? " armor" : "");
            if(!creature->isStaff()) return(true);
        }
    }

    return(false);
}

//*********************************************************************
//                          levelRestrict
//*********************************************************************

bool Object::alignRestrict(const std::shared_ptr<Creature> & creature, bool p) const {
    if( (flagIsSet(O_GOOD_ALIGN_ONLY) && creature->getAdjustedAlignment() < NEUTRAL) ||
        (flagIsSet(O_EVIL_ALIGN_ONLY) && creature->getAdjustedAlignment() > NEUTRAL) )
    {
        if(p) {
            creature->checkStaff("%O shocks you and you drop it.\n", this);
            if(!creature->isStaff())
                broadcast(creature->getSock(), creature->getConstRoomParent(), "%M is shocked by %P.", creature.get(), this);
        }
        if(!creature->isStaff()) return(true);
    }
    return(false);
}



//*********************************************************************
//                          sexRestrict
//*********************************************************************

bool Object::sexRestrict(const std::shared_ptr<Creature> & creature, bool p) const {
    Sex sex = creature->getSex();
    if(flagIsSet(O_MALE_ONLY) && sex != SEX_MALE) {
        if(p) creature->checkStaff("Only males can use that.\n");
        if(!creature->isStaff()) return(true);
    }
    if(flagIsSet(O_FEMALE_ONLY) && sex != SEX_FEMALE) {
        if(p) creature->checkStaff("Only females can use that.\n");
        if(!creature->isStaff()) return(true);
    }
    if(flagIsSet(O_SEXLESS_ONLY) && sex != SEX_NONE) {
        if(p) creature->checkStaff("Only creatures without a gender can use that.\n");
        if(!creature->isStaff()) return(true);
    }
    return(false);
}


//*********************************************************************
//                          strRestrict
//*********************************************************************

bool Object::strRestrict(const std::shared_ptr<Creature>& creature, bool p) const {
    if(minStrength > creature->strength.getCur()) {
        if(!p) creature->checkStaff("You are currently not strong enough to use that.\n");
        if(!creature->isStaff()) return(true);
    }
    return(false);
}


//*********************************************************************
//                          clanRestrict
//*********************************************************************

bool Object::clanRestrict(const std::shared_ptr<Creature> & creature, bool p) const {
    if(clanRestrict(creature)) {
        if(p) creature->checkStaff("Your allegience prevents you from using %P.\n", this);
        if(!creature->isStaff()) return(true);
    }
    return(false);
}

bool Object::clanRestrict(const std::shared_ptr<const Creature> & creature) const {
    int c = creature->getClan();
    if(creature->getDeity()) {
        const Clan* clan = gConfig->getClanByDeity(creature->getDeity());
        if(clan && clan->getId() != 0)
            c = clan->getId();
    }

    // if the object requires pledging, let any clan pass
    if(flagIsSet(O_PLEDGED_ONLY))
        return(!c);

    // if no flags are set
    if( !flagIsSet(O_CLAN_1) &&
        !flagIsSet(O_CLAN_2) &&
        !flagIsSet(O_CLAN_3) &&
        !flagIsSet(O_CLAN_4) &&
        !flagIsSet(O_CLAN_5) &&
        !flagIsSet(O_CLAN_6) &&
        !flagIsSet(O_CLAN_7) &&
        !flagIsSet(O_CLAN_8) &&
        !flagIsSet(O_CLAN_9) &&
        !flagIsSet(O_CLAN_10) &&
        !flagIsSet(O_CLAN_11) &&
        !flagIsSet(O_CLAN_12) )
        return(false);

    // if the clan flag is set and they match, they pass
    if( (flagIsSet(O_CLAN_1) && c == 1) ||
        (flagIsSet(O_CLAN_2) && c == 2) ||
        (flagIsSet(O_CLAN_3) && c == 3) ||
        (flagIsSet(O_CLAN_4) && c == 4) ||
        (flagIsSet(O_CLAN_5) && c == 5) ||
        (flagIsSet(O_CLAN_6) && c == 6) ||
        (flagIsSet(O_CLAN_7) && c == 7) ||
        (flagIsSet(O_CLAN_8) && c == 8) ||
        (flagIsSet(O_CLAN_9) && c == 9) ||
        (flagIsSet(O_CLAN_10) && c == 10) ||
        (flagIsSet(O_CLAN_11) && c == 11) ||
        (flagIsSet(O_CLAN_12) && c == 12) )
        return(false);

    return(true);
}


//*********************************************************************
//                          lawchaoRestrict
//*********************************************************************

bool Object::lawchaoRestrict(const std::shared_ptr<Creature> & creature, bool p) const {
    if( (flagIsSet(O_CHAOTIC_ONLY) && !creature->flagIsSet(P_CHAOTIC)) ||
        (flagIsSet(O_LAWFUL_ONLY) && creature->flagIsSet(P_CHAOTIC)) )
    {
        if(p) creature->checkStaff("You are unable to use %P.\n", this);
        if(!creature->isStaff()) return(true);
    }
    return(false);
}


//*********************************************************************
//                      doRestrict
//*********************************************************************

bool Object::doRestrict(const std::shared_ptr<Creature> &creature, bool p) {
    if (clanRestrict(creature, p) || levelRestrict(creature, p) || skillRestrict(creature, p) || strRestrict(creature, p) || classRestrict(creature, p)
     || raceRestrict(creature, p) || sexRestrict(creature, p) || clanRestrict(creature, p) || lawchaoRestrict(creature, p))
        return (true);
    if (alignRestrict(creature, p)) {
        if (p && !creature->isStaff()) {
            creature->delObj(Containable::downcasted_shared_from_this<Object>(), false, true);
            addToRoom(creature->getRoomParent());
        }
        return (true);
    }
    return (false);
}

//*********************************************************************
//                          getCompass
//*********************************************************************

std::string Object::getCompass(const std::shared_ptr<const Creature> & creature, bool useName) const {
    std::ostringstream oStr;

    if(useName)
        oStr << getObjStr(creature, CAP, 1);
    else
        oStr <<  "It";
    oStr <<  " ";

    if(!compass) {
        oStr << "appears to be broken.\n";
        return(oStr.str());
    }
    if(!creature->inAreaRoom()) {
        oStr << "is currently spinning in circles.\n";
        return(oStr.str());

    }
    const MapMarker& mapmarker = creature->getConstAreaRoomParent()->mapmarker;

    if( !mapmarker.getArea() || !compass->getArea() || mapmarker.getArea() != compass->getArea() || mapmarker == *compass) {
        oStr << "is currently spinning in circles.\n";
        return(oStr.str());
    }

    oStr << "points " << mapmarker.direction(*compass) << ". The target is " << mapmarker.distance(*compass) << ".\n";
    return(oStr.str());
}


//*********************************************************************
//                      getObjStr
//*********************************************************************

std::string Object::getObjStr(const std::shared_ptr<const Creature> & viewer, unsigned int ioFlags, int num) const {
    std::ostringstream objStr;
    std::string toReturn;
    char ch;

    if(flagIsSet(O_DARKNESS))
        objStr << "^D";

    if(viewer != nullptr)
        ioFlags |= viewer->displayFlags();
    bool irrPlural = false;


    // use new plural code - on object
    if(num > 1 && !plural.empty()) {
        objStr << int_to_text(num) << " "  << plural;
        irrPlural = true;
    }

    if(!irrPlural) {
        // Either not an irregular plural, or we couldn't find a match in the irregular plural file
        if(num == 0) {
            if(!flagIsSet(O_NO_PREFIX)) {
                objStr << "the " << getName();
            } else
                objStr << getName();
        }
        else if(num == 1) {
            if(flagIsSet(O_NO_PREFIX) || (info.id == 0 && !strcmp(key[0], "gold") && type == ObjectType::MONEY))
                objStr <<  "";
            else if(flagIsSet(O_SOME_PREFIX))
                objStr << "some ";
            else {
                // handle articles even when the item starts with a color
                int pos=0;
                while(getName()[pos] == '^') pos += 2;
                ch = low(getName()[pos]);

                if(ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u')
                    objStr << "an ";
                else
                    objStr << "a ";
            }
            objStr << getName();
        }
        else {
            auto nameStr = int_to_text(num) + " ";

            if(flagIsSet(O_SOME_PREFIX))
                nameStr += "sets of ";

            nameStr += getName();

            if(!flagIsSet(O_SOME_PREFIX)) {
                auto last = nameStr.at(nameStr.length() - 1);
                if(last == 's' || last == 'x') {
                    nameStr += "es";
                } else {
                    nameStr += 's';
                }
            }
            objStr << nameStr;
        }
    }


    if(flagIsSet(O_NULL_MAGIC) && ((ioFlags & ISDM) || (ioFlags & ISCT))) {
        objStr << " (+" << adjustment << ")(n)";
    } else if((ioFlags & MAG) && adjustment && !flagIsSet(O_NULL_MAGIC)) {
        objStr << " (";
        if(adjustment >= 0)
            objStr << "+";
        objStr << adjustment << ")";
    } else if((ioFlags & MAG) && (magicpower || flagIsSet(O_MAGIC)) && !flagIsSet(O_NULL_MAGIC))
        objStr << " (M)";


    if(ioFlags & ISDM) {
        if(flagIsSet(O_HIDDEN))
            objStr <<  " (h)";
        if(isEffected("invisibility"))
            objStr << " (*)";
        if(flagIsSet(O_SCENERY))
            objStr << " (s)";
        if(flagIsSet(O_NOT_PEEKABLE))
            objStr << "(NoPeek)";
        if(flagIsSet(O_NO_STEAL))
            objStr << "(NoSteal)";
        if(flagIsSet(O_BODYPART))
            objStr << "(BodyPart)";

    }

    if(isBroken())
        objStr << " (broken)";
    objStr << "^x";

    toReturn = objStr.str();

    if(ioFlags & CAP) {
        int pos = 0;
        // don't capitalize colors
        while(toReturn[pos] == '^') pos += 2;
        toReturn[pos] = up(toReturn[pos]);
    }

    return(toReturn);
}


//*********************************************************************
//                      popBag
//*********************************************************************
// removes an object from the bag, usually when the bag is stolen or destroyed

void Object::popBag(const std::shared_ptr<Creature>& creature, bool quest, bool drop, bool steal, bool bodypart, bool dissolve) {
    std::shared_ptr<Object>  object=nullptr;

    ObjectSet::iterator it;
    for(it = objects.begin() ; it != objects.end() ; ) {
        object = (*it++);

        if( (quest && object->questnum) ||
            (dissolve && object->flagIsSet(O_RESIST_DISOLVE)) ||
            (drop && object->flagIsSet(O_NO_DROP)) ||
            (steal && object->flagIsSet(O_NO_STEAL)) ||
            (bodypart && object->flagIsSet(O_BODYPART))
        ) {
            this->delObj(object);
            creature->addObj(object);
        }
    }
}

//*********************************************************************
//                      isKey
//*********************************************************************

bool Object::isKey(const std::shared_ptr<UniqueRoom>& room, const std::shared_ptr<Exit> exit) const {
    // storage room exist must be a storage room key
    if(exit->flagIsSet(X_TO_STORAGE_ROOM))
        return(getName() == "storage room key");

    // key numbers must match
    if(getKey() != exit->getKey())
        return(false);

    if(!room)
        return(true);

    // For the key to work, it must be from the same area
    // ie: no using oceancrest keys in highport!
    std::string area = exit->getKeyArea();
    if(area.empty())
        area = room->info.area;

    if(!info.isArea("") && !info.isArea(area))
        return(false);
    return(true);
}


//*********************************************************************
//                      spawnObjects
//*********************************************************************
// room = the catref for the destination room

void spawnObjects(const std::string &room, const std::string &objects) {
    std::shared_ptr<UniqueRoom> dest = nullptr;
    std::shared_ptr<Object>  object=nullptr;
    CatRef  cr;

    getCatRef(room, cr, nullptr);

    if(!loadRoom(cr, dest))
        return;

    std::string obj;
    int i=0;
    dest->expelPlayers(true, false, false);

    // make sure any existing objects of this type are removed
    if(!dest->objects.empty()) {
        do {
            obj = getFullstrTextTrun(objects, i++);
            if(!obj.empty())
            {
                getCatRef(obj, cr, nullptr);
                ObjectSet::iterator it;
                for( it = dest->objects.begin() ; it != dest->objects.end() ; ) {
                    object = (*it++);

                    if(object->info == cr) {
                        object->deleteFromRoom();
                        object.reset();
                    }
                }
            }
        } while(!obj.empty());
    }

    // add the objects
    i=0;

    do {
        obj = getFullstrTextTrun(objects, i++);
        if(!obj.empty())
        {
            getCatRef(obj, cr, nullptr);

            if(loadObject(cr, object)) {
                // no need to spawn darkmetal items in a sunlit room
                if(object->flagIsSet(O_DARKMETAL) && dest->isSunlight())
                    object.reset();
                else {
                    object->addToRoom(dest);
                    object->setDroppedBy(dest, "SpawnObjects");
                }
            }
        }
    } while(!obj.empty());
}


double Object::getDps() const {
    short num = numAttacks;
    if(!numAttacks)
        num = 1;

    return(( (damage.average() + adjustment) * (1+num) / 2.0) / (getWeaponDelay()/10.0));
}


std::string Object::getFlagList(std::string_view sep) const {
    std::ostringstream ostr;
    bool found = false;
    for(int i=0; i<MAX_OBJECT_FLAGS; i++) {
        if(flagIsSet(i)) {
            if(found)
                ostr << sep;

            ostr << gConfig->getOFlag(i) << "(" << i+1 << ")";
            found = true;
        }
    }

    if(!found)
        return("None");
    else
        return ostr.str();
}
