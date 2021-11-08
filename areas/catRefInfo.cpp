/*
 * catRefInfo.h
 *   Category Reference info
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

#include <ostream>                // for ostringstream, basic_ostream, basic...
#include <sstream>
#include <string>

#include "calendar.hpp"           // for cSeason, Calendar, cWeather (ptr only)
#include "catRefInfo.hpp"         // for CatRefInfo
#include "config.hpp"             // for Config, gConfig
#include "proto.hpp"              // for broadcast, isCt
#include "random.hpp"             // for Random
#include "rooms.hpp"              // for BaseRoom, AreaRoom, UniqueRoom
#include "season.hpp"             // for Season

//*********************************************************************
//                      CatRefInfo
//*********************************************************************

CatRefInfo::CatRefInfo() { clear(); }
CatRefInfo::~CatRefInfo() { clear(); }

void CatRefInfo::clear() {
    id=0;
    parent = name = worldName = area = yearsSince = fishing = "";
    limbo = recall = 0;
    teleportWeight = teleportZone = trackZone = 0;
    yearOffset = 0;

    for(auto& [eSea, season] : seasons) {
        delete season;
    }
    seasons.clear();
}

int CatRefInfo::getId() const { return(id); }

std::string CatRefInfo::getArea() const { return(area); }
std::string CatRefInfo::getName() const { return(name); }
std::string CatRefInfo::getWorldName() const { return(worldName); }
std::string CatRefInfo::getYearsSince() const { return(yearsSince); }
std::string CatRefInfo::getParent() const { return(parent); }

int CatRefInfo::getYearOffset() const { return(yearOffset); }
int CatRefInfo::getLimbo() const { return(limbo); }
int CatRefInfo::getRecall() const { return(recall); }
int CatRefInfo::getTeleportWeight() const { return(teleportWeight); }
int CatRefInfo::getTeleportZone() const { return(teleportZone); }
int CatRefInfo::getTrackZone() const { return(trackZone); }


//*********************************************************************
//                      getWeather
//*********************************************************************

const cWeather* CatRefInfo::getWeather() const {
    std::map<Season,cSeason*>::const_iterator st;
    st = seasons.find(gConfig->getCalendar()->getCurSeason()->getId());
    if(st != seasons.end())
        return((*st).second->getWeather());
    return(nullptr);
}

//*********************************************************************
//                      str
//*********************************************************************

std::string CatRefInfo::str() const {
    std::ostringstream oStr;
    oStr << area;
    if(area == "area")
        oStr << " " << id;
    return(oStr.str());
}


//*********************************************************************
//                      catRefName
//*********************************************************************

std::string Config::catRefName(std::string_view area) const {
    const CatRefInfo* cri = gConfig->getCatRefInfo(area);
    if(cri)
        return(cri->getName());
    return("");
}

//*********************************************************************
//                      copyVars
//*********************************************************************
// only copy the vars it inherits from the parent

void CatRefInfo::copyVars(const CatRefInfo* cri) {
    teleportZone = cri->teleportZone;
    trackZone = cri->trackZone;
}

//*********************************************************************
//                      copyVars
//*********************************************************************

void Config::clearCatRefInfo() {
    CatRefInfo* cr;

    while(!catRefInfo.empty()) {
        cr = catRefInfo.front();
        delete cr;
        catRefInfo.pop_front();
    }
    catRefInfo.clear();
}


//*********************************************************************
//                      getCatRefInfo
//*********************************************************************

const CatRefInfo* Config::getCatRefInfo(std::string_view area, int id, int shouldGetParent) const {
    std::list<CatRefInfo*>::const_iterator it;

    // prevent infinite loops from bad config files
    if(shouldGetParent > 30) {
        broadcast(isCt, "^yConfig::getCatRefInfo is terminating because it is looping too long.\nCheck the catrefinfo config file for possible looping issues.\n");
        return(nullptr);
    }
    for(it = catRefInfo.begin() ; it != catRefInfo.end() ; it++) {
        if(area == (*it)->getArea() && id == (*it)->getId()) {
            if(shouldGetParent && !(*it)->getParent().empty())
                return(getCatRefInfo((*it)->getParent(), 0, shouldGetParent+1));
            return(*it);
        }
    }

    return(nullptr);
}

//*********************************************************************
//                      getCatRefInfo
//*********************************************************************

const CatRefInfo* Config::getCatRefInfo(const BaseRoom* room, int shouldGetParent) const {
    const AreaRoom* aRoom=nullptr;
    const UniqueRoom* uRoom=nullptr;
    int id = 0;
    std::string area = "";

    if(room) {
        uRoom = room->getAsConstUniqueRoom();
        aRoom = room->getAsConstAreaRoom();
    }

    if(uRoom) {
        area = uRoom->info.area;
    } else if(aRoom) {
        area = "area";
        id = aRoom->area->id;
    } else {
        area = "misc";
    }

    return(getCatRefInfo(area, id, shouldGetParent));
}

//*********************************************************************
//                      getRandomCatRefInfo
//*********************************************************************
// used by teleport spell, so check teleportZone

const CatRefInfo* Config::getRandomCatRefInfo(int zone) const {
    std::list<CatRefInfo*>::const_iterator it;
    int total=0, pick=0;

    for(it = catRefInfo.begin() ; it != catRefInfo.end() ; it++) {
        if(!zone || zone == (*it)->getTeleportZone())
            total += (*it)->getTeleportWeight();
    }
    if(!total)
        return(nullptr);

    pick = Random::get(1, total);
    total = 0;

    for(it = catRefInfo.begin() ; it != catRefInfo.end() ; it++) {
        if(!zone || zone == (*it)->getTeleportZone()) {
            total += (*it)->getTeleportWeight();
            if(total >= pick)
                return(*it);
        }
    }
    return(nullptr);
}
