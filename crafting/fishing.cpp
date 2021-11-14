/*
 * fishing.cpp
 *   Fishing
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

#include <cstring>                     // for strcmp
#include <list>                        // for _List_const_iterator, list
#include <map>                         // for map, operator==, map<>::const_...
#include <ostream>                     // for operator<<, basic_ostream, cha...
#include <string>                      // for string, allocator, operator<<
#include <string_view>                 // for string_view
#include <utility>                     // for pair

#include "area.hpp"                    // for Area, AreaZone, TileInfo
#include "catRef.hpp"                  // for CatRef
#include "catRefInfo.hpp"              // for CatRefInfo
#include "cmd.hpp"                     // for cmd
#include "config.hpp"                  // for Config, gConfig
#include "delayedAction.hpp"           // for ActionFish, DelayedAction
#include "fishing.hpp"                 // for FishingItem, Fishing
#include "flags.hpp"                   // for O_FISHING
#include "global.hpp"                  // for HELD, DEFAULT_WEAPON_DELAY
#include "mudObjects/areaRooms.hpp"    // for AreaRoom
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/mudObject.hpp"    // for MudObject
#include "mudObjects/objects.hpp"      // for Object
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "proto.hpp"                   // for broadcast, isDay, doGetObject
#include "random.hpp"                  // for Random
#include "server.hpp"                  // for Server, gServer
#include "statistics.hpp"              // for Statistics
#include "unique.hpp"                  // for Lore, Unique
#include "utils.hpp"                   // for MAX, MIN
#include "xml.hpp"                     // for loadObject, loadMonster

//**********************************************************************
//                      canFish
//**********************************************************************

bool canFish(const Player* player, const Fishing** list, Object** pole) {
    *pole = player->ready[HELD-1];

    if(!player->isStaff()) {
        if(!player->ableToDoCommand())
            return(false);

        if(player->isBlind()) {
            player->printColor("^CYou can't do that! You're blind!\n");
            return(false);
        }
        if(player->inCombat()) {
            player->printColor("You are too busy to do that now!\n");
            return(false);
        }

        if(player->isEffected("mist")) {
            player->printColor("You must be in corporeal form to work with items.\n");
            return(false);
        }
        if(player->isEffected("berserk")) {
            player->print("You are too angry to go fishing!\n");
            return(false);
        }

        if(!*pole || !(*pole)->flagIsSet(O_FISHING)) {
            player->printColor("You need a fishing pole to go fishing.\n");
            return(false);
        }
    }

    if(player->inAreaRoom())
        *list = player->getConstAreaRoomParent()->getFishing();
    else if(player->inUniqueRoom())
        *list = player->getConstUniqueRoomParent()->getFishing();

    if(!*list || (*list)->empty()) {
        player->printColor("You can't go fishing here!\n");
        return(false);
    }

    return(true);
}

//**********************************************************************
//                      failFishing
//**********************************************************************

bool failFishing(Player* player, const std::string& adminMsg, bool almost=true) {
    if(almost) {
        switch(Random::get(0,1)) {
        case 1:
            player->print("You almost caught a fish, but it got away.\n");
            break;
        default:
            player->print("You almost caught something, but it got away.\n");
            break;
        }
    } else {
        switch(Random::get(0,1)) {
        case 1:
            player->print("You failed to catch anything.\n");
            break;
        default:
            player->print("You failed to catch any fish.\n");
            break;
        }
    }
    if(player->isCt())
        player->checkStaff("%s\n", adminMsg.c_str());
    // don't improve if they almost caught something
    if(!almost)
        player->checkImprove("fishing", false);
    return(false);
}

//**********************************************************************
//                      doFish
//**********************************************************************

bool hearMobAggro(Socket* sock);

bool doFish(Player* player) {
    const Fishing* list=nullptr;
    const FishingItem* item=nullptr;
    Monster* monster=nullptr;
    Object* pole=nullptr, *fish=nullptr;
    double chance=0.0, comp=0.0, skill=0.0, quality=0.0;
    bool day = isDay();

    player->unhide();

    if(!canFish(player, &list, &pole))
        return(false);

    if(!player->knowsSkill("fishing"))
        player->addSkill("fishing", 1);

    // did they break the fishing pole?
    if(pole) {
        if(!Random::get(0, 3))
            pole->decShotsCur();
        if(pole->getShotsCur() < 1) {
            player->breakObject(pole, HELD);
            return(false);
        }
    }

    skill = player->getSkillLevel("fishing");
    comp = skill * 100 / 40;  // turn from 1-40 to %
    comp = comp * 2 / 3;  // skill accounts for 2/3 of your success
    chance += comp;

    quality = pole ? pole->getQuality() : 0;
    comp = quality * 10 / 40;  // turn from 1-400 to %
    comp /= 3;  // the fishing pole accounts for 1/3 of your success
    chance += comp;

    chance = MAX<double>(10, MIN<double>(95, chance));

    if(Random::get(1,100) > chance)
        return(failFishing(player, "Dice roll.", false));

    item = list->getItem((short)skill, (short)quality);

    if(!item)
        return(failFishing(player, "No FishingItem found.", false));
    if(list->id == "river" && player->getRoomParent()->isWinter())
        return(failFishing(player, "Winter."));
    if(day && item->isNightOnly())
        return(failFishing(player, "Night-time only.", false));
    if(!day && item->isDayOnly())
        return(failFishing(player, "Day-time only.", false));

    if(!item->isMonster()) {
        // most fish they get will be objects
        if(!loadObject(item->getFish(), &fish))
            return(failFishing(player, "Object failed to load.", false));
        if(!Unique::canGet(player, fish)) {
            delete fish;
            return(failFishing(player, "Cannot have unique item."));
        }
        if(!Lore::canHave(player, fish, false)) {
            delete fish;
            return(failFishing(player, "Cannot have lore item (inventory)."));
        }
        if(!Lore::canHave(player, fish)) {
            delete fish;
            return(failFishing(player, "Cannot have lore item (bag)."));
        }
        if(player->getLevel() < fish->getLevel() && fish->getQuestnum() > 0) {
            delete fish;
            return(failFishing(player, "Too low level."));
        }
        if((player->getWeight() + fish->getActualWeight()) > player->maxWeight()) {
            delete fish;
            return(failFishing(player, "Too heavy."));
        }
        if(player->tooBulky(fish->getActualBulk())) {
            delete fish;
            return(failFishing(player, "Too bulky."));
        }
        if(fish->getQuestnum() && player->questIsSet(fish->getQuestnum()-1)) {
            delete fish;
            return(failFishing(player, "Already completed the quest."));
        }
    } else {
        // some fish they get will be monsters
        if(!loadMonster(item->getFish(), &monster))
            return(failFishing(player, "Monster failed to load.", false));
        if(player->getRoomParent()->countCrt() + 1 >= player->getRoomParent()->getMaxMobs()) {
            delete monster;
            return(failFishing(player, "Room too full.", false));
        }
        // TODO: do this so the print functions work properly
        monster->setParent(player->getParent());
//      monster->parent_rom = player->parent_rom;
//      monster->area_room = player->area_room;
    }

    // they caught something!

    if(!item->isMonster()) {
        if(item->getExp()) {
            if(!player->halftolevel()) {
                player->printColor("You %s %d experience for catching %1P!\n", gConfig->isAprilFools() ? "lose" : "gain", item->getExp(), fish);
                player->addExperience(item->getExp());
            }
        } else {
            player->printColor("You catch %1P!\n", fish);
        }
        broadcast(player->getSock(), player->getParent(), "%M catches something!", player);
    } else {
        if(item->getExp()) {
            if(!player->halftolevel()) {
                player->printColor("You %s %d experience for catching %1N!\n", gConfig->isAprilFools() ? "lose" : "gain", item->getExp(), monster);
                player->addExperience(item->getExp());
            }
        } else {
            player->printColor("You catch %1N!\n", monster);
        }
        broadcast(player->getSock(), player->getParent(), "%M catches %1N!", player, monster);
    }

    player->statistics.fish();
    player->checkImprove("fishing", true);

    if(!item->isMonster()) {
        doGetObject(fish, player);
    } else {
        monster->addToRoom(player->getRoomParent(), 1);

        // most fish will be angry about this
        if(item->willAggro()) {
            // don't let them swing right away
            monster->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
            monster->addEnemy(player, true);

            broadcast(hearMobAggro, "^y*** %s(R:%s) added %s to %s attack list (fishing aggro).",
                monster->getCName(), player->getRoomParent()->fullName().c_str(), player->getCName(), monster->hisHer());
        }
    }
    return(true);
}

void doFish(const DelayedAction* action) {
    doFish(action->target->getAsPlayer());
}

//**********************************************************************
//                      cmdFish
//**********************************************************************

int cmdFish(Player* player, cmd* cmnd) {
    const Fishing* list=nullptr;
    Object* pole=nullptr;

    player->unhide();

    if(gServer->hasAction(player, ActionFish)) {
        player->print("You are already fishing!\n");
        return(0);
    }

    if(!canFish(player, &list, &pole))
        return(0);

    player->interruptDelayedActions();
    gServer->addDelayedAction(doFish, player, nullptr, ActionFish, 10 - (int)(player->getSkillLevel("fishing") / 10) - Random::get(0,3));

    player->print("You begin fishing.\n");
    broadcast(player->getSock(), player->getParent(), "%M begins fishing.", player);
    return(0);
}

//*********************************************************************
//                      FishingItem
//*********************************************************************

FishingItem::FishingItem() {
    dayOnly = nightOnly = monster = aggro = false;
    weight = minQuality = minSkill = exp = 0;
}

CatRef FishingItem::getFish() const { return(fish); }

bool FishingItem::isDayOnly() const { return(dayOnly); }
bool FishingItem::isNightOnly() const { return(nightOnly); }

short FishingItem::getWeight() const { return(weight); }
short FishingItem::getMinQuality() const { return(minQuality); }
short FishingItem::getMinSkill() const { return(minSkill); }
long FishingItem::getExp() const { return(exp); }

bool FishingItem::isMonster() const { return(monster); }
bool FishingItem::willAggro() const { return(aggro); }

//*********************************************************************
//                      Fishing
//*********************************************************************

Fishing::Fishing() {
    id = "";
}

//*********************************************************************
//                      empty
//*********************************************************************

bool Fishing::empty() const {
    return(items.empty());
}

//*********************************************************************
//                      getItem
//*********************************************************************

const FishingItem* Fishing::getItem(short skill, short quality) const {
    int total=0, pick=0;
    std::list<FishingItem>::const_iterator it;
    bool day = isDay();

    for(it = items.begin() ; it != items.end() ; it++) {
        if(skill < (*it).getMinSkill() || quality < (*it).getMinQuality())
            continue;
        if((day && (*it).isNightOnly()) || (!day && (*it).isDayOnly()))
            continue;
        total += (*it).getWeight();
    }

    // nothing to catch?
    if(!total)
        return(nullptr);

    // which item to pick?
    pick = Random::get(1, total);
    total = 0;

    for(it = items.begin() ; it != items.end() ; it++) {
        if(skill < (*it).getMinSkill() || quality < (*it).getMinQuality())
            continue;
        if((day && (*it).isNightOnly()) || (!day && (*it).isDayOnly()))
            continue;
        total += (*it).getWeight();
        if(total >= pick)
            return(&(*it));
    }
    return(nullptr);
}

//*********************************************************************
//                      doGetFishing
//*********************************************************************

const Fishing* AreaRoom::doGetFishing(short y, short x) const {
    const TileInfo* tile = area->getTile(area->getTerrain(nullptr, &mapmarker, y, x, 0, true), area->getSeasonFlags(&mapmarker));
    const AreaZone* zone=nullptr;
    const Fishing* list=nullptr;
    std::list<AreaZone*>::const_iterator it;

    if(!tile || !tile->isWater())
        return(nullptr);

    // zone comes first
    for(it = area->zones.begin() ; it != area->zones.end() ; it++) {
        zone = (*it);
        if(zone->inside(area, &mapmarker) && !zone->getFishing().empty()) {
            list = gConfig->getFishing(zone->getFishing());
            if(list)
                return(list);
        }
    }

    // then tile
    if(!tile->getFishing().empty()) {
        list = gConfig->getFishing(tile->getFishing());
        if(list)
            return(list);
    }

    // then catrefinfo
    const CatRefInfo* cri = gConfig->getCatRefInfo(this);
    if(cri && !cri->getFishing().empty()) {
        list = gConfig->getFishing(cri->getFishing());
        if(list)
            return(list);
    }

    return(0);
}

//*********************************************************************
//                      getFishing
//*********************************************************************

const Fishing* AreaRoom::getFishing() const {
    short y=0,x=0;
    const Fishing* list = doGetFishing(0, 0);
    if(list)
        return(list);

    for(y=-1; y<=1; y++) {
        for(x=-1; x<=1; x++) {
            // we check 0,0 in the beginning
            if(!x && !y)
                continue;
            list = doGetFishing(y, x);
            if(list)
                return(list);
        }
    }

    return(0);
}

const Fishing* UniqueRoom::getFishing() const {
    const Fishing *list=0;

    // room fish list comes first
    if(!fishing.empty()) {
        list = gConfig->getFishing(fishing);
        if(list)
            return(list);
    }

    // then catrefinfo
    const CatRefInfo* cri = gConfig->getCatRefInfo(this);
    if(cri && !cri->getFishing().empty()) {
        list = gConfig->getFishing(cri->getFishing());
        if(list)
            return(list);
    }

    return(nullptr);
}

std::string AreaZone::getFishing() const { return(fishing); }
std::string TileInfo::getFishing() const { return(fishing); }
std::string CatRefInfo::getFishing() const { return(fishing); }
std::string UniqueRoom::getFishingStr() const { return(fishing); }

//*********************************************************************
//                      setFishing
//*********************************************************************

void UniqueRoom::setFishing(std::string_view id) { fishing = id; }



//*********************************************************************
//                      clearFishing
//*********************************************************************

void Config::clearFishing() {
    fishing.clear();
}

//*********************************************************************
//                      getFishing
//*********************************************************************

const Fishing *Config::getFishing(const std::string &id) const {
    auto it = fishing.find(id);

    if(it == fishing.end())
        return(nullptr);

    return(&(*it).second);
}

//*********************************************************************
//                      dmFishing
//*********************************************************************

int dmFishing(Player* player, cmd* cmnd) {
    std::map<std::string, Fishing>::const_iterator it;
    std::list<FishingItem>::const_iterator ft;
    std::ostringstream oStr;
    const Fishing *list=nullptr;
    const FishingItem *item=nullptr;
    Object* fish=nullptr;
    Monster* monster=nullptr;
    std::string name="";
    bool all = !strcmp(cmnd->str[1], "all");

    oStr.setf(std::ios::left, std::ios::adjustfield);
    oStr << "^yFishing Information:  Type ^y*fishing all^x to view all information.^x\n";

    for(it = gConfig->fishing.begin() ; it != gConfig->fishing.end() ; it++) {
        list = &(*it).second;
        oStr << "ID: ^C" << list->id << "^x\n"
             << "    Items:\n";

        for(ft = list->items.begin() ; ft != list->items.end() ; ft++) {
            item = &(*ft);

            oStr << "      ^c" << item->getFish().str();


            name = "";
            if(!item->isMonster()) {
                // are they catching an object?
                if(loadObject(item->getFish(), &fish)) {
                    name = fish->getName();
                    delete fish;
                }
            } else {
                // or a monster?
                if(loadObject(item->getFish(), &fish)) {
                    name = monster->getName();
                    delete monster;
                }
            }


            if(!name.empty())
                oStr << "^x, ^c" << name;
            oStr << "^x\n";

            if(all) {
                oStr << "        Day Only? " << (item->isDayOnly() ? "^gYes" : "^rNo")
                     << "^x Night Only? " << (item->isNightOnly() ? "^gYes" : "^rNo") << "^x\n"
                     << "        Weight: ^c" << item->getWeight() << "^x Exp: ^c" << item->getExp() << "^x\n"
                     << "        MinQuality: ^c" << item->getMinQuality() << "^x ^cMinSkill: " << item->getMinSkill() << "^x\n";
            }
        }
    }

    player->printColor("%s\n", oStr.str().c_str());
    return(0);
}
