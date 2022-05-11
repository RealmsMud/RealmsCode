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

bool Player::canFish(const Fishing** list, std::shared_ptr<Object>&  pole) const {
    pole = ready[HELD-1];

    if(!isStaff()) {
        if(!ableToDoCommand())
            return(false);

        if(isBlind()) {
            printColor("^CYou can't do that! You're blind!\n");
            return(false);
        }
        if(inCombat()) {
            printColor("You are too busy to do that now!\n");
            return(false);
        }

        if(isEffected("mist")) {
            printColor("You must be in corporeal form to work with items.\n");
            return(false);
        }
        if(isEffected("berserk")) {
            print("You are too angry to go fishing!\n");
            return(false);
        }

        if(!pole || !(pole)->flagIsSet(O_FISHING)) {
            printColor("You need a fishing pole to go fishing.\n");
            return(false);
        }
    }

    if(inAreaRoom())
        *list = getConstAreaRoomParent()->getFishing();
    else if(inUniqueRoom())
        *list = getConstUniqueRoomParent()->getFishing();

    if(!*list || (*list)->empty()) {
        printColor("You can't go fishing here!\n");
        return(false);
    }

    return(true);
}

//**********************************************************************
//                      failFishing
//**********************************************************************

bool failFishing(const std::shared_ptr<Player>& player, const std::string& adminMsg, bool almost=true) {
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

bool Player::doFish() {
    const Fishing* list=nullptr;
    const FishingItem* item;
    std::shared_ptr<Monster>  monster=nullptr;
    std::shared_ptr<Object>  pole=nullptr, fish=nullptr;
    double chance=0.0, comp, skill, quality;
    bool day = isDay();

    unhide();

    if(!canFish(&list, pole))
        return(false);

    if(!knowsSkill("fishing"))
        addSkill("fishing", 1);

    // did they break the fishing pole?
    if(pole) {
        if(!Random::get(0, 3))
            pole->decShotsCur();
        if(pole->getShotsCur() < 1) {
            breakObject(pole, HELD);
            return(false);
        }
    }
    auto pThis = Containable::downcasted_shared_from_this<Player>();
    skill = getSkillLevel("fishing");
    comp = skill * 100 / 40;  // turn from 1-40 to %
    comp = comp * 2 / 3;  // skill accounts for 2/3 of your success
    chance += comp;

    quality = pole ? pole->getQuality() : 0;
    comp = quality * 10 / 40;  // turn from 1-400 to %
    comp /= 3;  // the fishing pole accounts for 1/3 of your success
    chance += comp;

    chance = MAX<double>(10, MIN<double>(95, chance));

    if(Random::get(1,100) > chance)
        return(failFishing(pThis, "Dice roll.", false));

    item = list->getItem((short)skill, (short)quality);

    if(!item)
        return(failFishing(pThis, "No FishingItem found.", false));
    if(list->id == "river" && getRoomParent()->isWinter())
        return(failFishing(pThis, "Winter."));
    if(day && item->isNightOnly())
        return(failFishing(pThis, "Night-time only.", false));
    if(!day && item->isDayOnly())
        return(failFishing(pThis, "Day-time only.", false));

    if(!item->isMonster()) {
        // most fish they get will be objects
        if(!loadObject(item->getFish(), fish))
            return(failFishing(pThis, "Object failed to load.", false));
        if(!Unique::canGet(pThis, fish)) {
            fish.reset();
            return(failFishing(pThis, "Cannot have unique item."));
        }
        if(!Lore::canHave(pThis, fish, false)) {
            fish.reset();
            return(failFishing(pThis, "Cannot have lore item (inventory)."));
        }
        if(!Lore::canHave(pThis, fish)) {
            fish.reset();
            return(failFishing(pThis, "Cannot have lore item (bag)."));
        }
        if(getLevel() < fish->getLevel() && fish->getQuestnum() > 0) {
            fish.reset();
            return(failFishing(pThis, "Too low level."));
        }
        if((getWeight() + fish->getActualWeight()) > maxWeight()) {
            fish.reset();
            return(failFishing(pThis, "Too heavy."));
        }
        if(tooBulky(fish->getActualBulk())) {
            fish.reset();
            return(failFishing(pThis, "Too bulky."));
        }
        if(fish->getQuestnum() && questIsSet(fish->getQuestnum()-1)) {
            fish.reset();
            return(failFishing(pThis, "Already completed the quest."));
        }
    } else {
        // some fish they get will be monsters
        if(!loadMonster(item->getFish(), monster))
            return(failFishing(pThis, "Monster failed to load.", false));
        if(getRoomParent()->countCrt() + 1 >= getRoomParent()->getMaxMobs()) {
            monster.reset();
            return(failFishing(pThis, "Room too full.", false));
        }
        // TODO: do this so the print functions work properly
        monster->setParent(getParent());
//      monster->parent_rom = parent_rom;
//      monster->area_room = area_room;
    }

    // they caught something!

    if(!item->isMonster()) {
        if(item->getExp()) {
            if(!halftolevel()) {
                printColor("You %s %d experience for catching %1P!\n", gConfig->isAprilFools() ? "lose" : "gain", item->getExp(), fish.get());
                addExperience(item->getExp());
            }
        } else {
            printColor("You catch %1P!\n", fish.get());
        }
        broadcast(getSock(), getParent(), "%M catches something!", this);
    } else {
        if(item->getExp()) {
            if(!halftolevel()) {
                printColor("You %s %d experience for catching %1N!\n", gConfig->isAprilFools() ? "lose" : "gain", item->getExp(), monster.get());
                addExperience(item->getExp());
            }
        } else {
            printColor("You catch %1N!\n", monster.get());
        }
        broadcast(getSock(), getParent(), "%M catches %1N!", this, monster.get());
    }

    statistics.fish();
    checkImprove("fishing", true);

    if(!item->isMonster()) {
        doGetObject(fish, pThis);
    } else {
        monster->addToRoom(getRoomParent(), 1);

        // most fish will be angry about this
        if(item->willAggro()) {
            // don't let them swing right away
            monster->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
            monster->addEnemy(pThis, true);

            broadcast(hearMobAggro, "^y*** %s(R:%s) added %s to %s attack list (fishing aggro).",
                monster->getCName(), getRoomParent()->fullName().c_str(), getCName(), monster->hisHer());
        }
    }
    return(true);
}

void doFish(const DelayedAction* action) {
    if(auto t = action->target.lock()) {
        t->getAsPlayer()->doFish();
    }
}

//**********************************************************************
//                      cmdFish
//**********************************************************************

int cmdFish(const std::shared_ptr<Player>& player, cmd* cmnd) {
    const Fishing* list=nullptr;
    std::shared_ptr<Object>  pole=nullptr;

    player->unhide();

    if(gServer->hasAction(player, ActionFish)) {
        player->print("You are already fishing!\n");
        return(0);
    }

    if(!player->canFish(&list, pole))
        return(0);

    player->interruptDelayedActions();
    gServer->addDelayedAction(doFish, player, nullptr, ActionFish, 10 - (int)(player->getSkillLevel("fishing") / 10) - Random::get(0,3));

    player->print("You begin fishing.\n");
    broadcast(player->getSock(), player->getParent(), "%M begins fishing.", player.get());
    return(0);
}

//*********************************************************************
//                      FishingItem
//*********************************************************************

FishingItem::FishingItem() {
    dayOnly = nightOnly = monster = aggro = false;
    weight = minQuality = minSkill = 0;
    exp = 0;
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
    int total=0, pick;
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
    auto myArea = area.lock();
    if(!myArea) return(0);

    const std::shared_ptr<TileInfo>  tile = myArea->getTile(myArea->getTerrain(nullptr, mapmarker, y, x, 0, true), myArea->getSeasonFlags(mapmarker));
    std::shared_ptr<const AreaZone>  zone;
    const Fishing* list;
    std::list<std::shared_ptr<AreaZone> >::const_iterator it;

    if(!tile || !tile->isWater())
        return(nullptr);

    // zone comes first
    for(it = myArea->areaZones.begin() ; it != myArea->areaZones.end() ; it++) {
        zone = (*it);
        if(zone->inside(myArea, mapmarker) && !zone->getFishing().empty()) {
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
    const CatRefInfo* cri = gConfig->getCatRefInfo(Container::downcasted_shared_from_this<AreaRoom>());
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
    short y,x;
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
    const Fishing *list;

    // room fish list comes first
    if(!fishing.empty()) {
        list = gConfig->getFishing(fishing);
        if(list)
            return(list);
    }

    // then catrefinfo
    const CatRefInfo* cri = gConfig->getCatRefInfo(Container::downcasted_shared_from_this<UniqueRoom>());
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

int dmFishing(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::map<std::string, Fishing>::const_iterator it;
    std::list<FishingItem>::const_iterator ft;
    std::ostringstream oStr;
    const Fishing *list;
    const FishingItem *item;
    std::shared_ptr<Object>  fish=nullptr;
    std::shared_ptr<Monster>  monster=nullptr;
    std::string name;
    bool all = !strcmp(cmnd->str[1], "all");

    oStr.setf(std::ios::left, std::ios::adjustfield);
    oStr << "^yFishing Information:  Type ^y*fishing all^x to view all information.^x\n";

    for(it = gConfig->fishing.begin() ; it != gConfig->fishing.end() ; it++) {
        list = &(*it).second;
        oStr << "ID: ^C" << list->id << "^x\n"
             << "    Items:\n";

        for(ft = list->items.begin() ; ft != list->items.end() ; ft++) {
            item = &(*ft);

            oStr << "      ^c" << item->getFish().displayStr();


            name = "";
            if(!item->isMonster()) {
                // are they catching an object?
                if(loadObject(item->getFish(), fish)) {
                    name = fish->getName();
                    fish.reset();
                }
            } else {
                // or a monster?
                if(loadObject(item->getFish(), fish)) {
                    name = monster->getName();
                    monster.reset();
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
