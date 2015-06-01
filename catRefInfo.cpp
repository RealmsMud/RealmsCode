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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "calendar.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <iostream>
#include <fstream>

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

    std::map<Season,cSeason*>::iterator st;
    cSeason *season=0;
    for(st = seasons.begin() ; st != seasons.end() ; st++) {
        season = (*st).second;
        delete season;
    }
    seasons.clear();
}

//*********************************************************************
//                      getId
//*********************************************************************

int CatRefInfo::getId() const {
    return(id);
}

//*********************************************************************
//                      getArea
//*********************************************************************

bstring CatRefInfo::getArea() const {
    return(area);
}

//*********************************************************************
//                      getName
//*********************************************************************

bstring CatRefInfo::getName() const {
    return(name);
}

//*********************************************************************
//                      getWorldName
//*********************************************************************

bstring CatRefInfo::getWorldName() const {
    return(worldName);
}

//*********************************************************************
//                      getYearsSince
//*********************************************************************

bstring CatRefInfo::getYearsSince() const {
    return(yearsSince);
}

//*********************************************************************
//                      getParent
//*********************************************************************

bstring CatRefInfo::getParent() const {
    return(parent);
}

//*********************************************************************
//                      getYearOffset
//*********************************************************************

int CatRefInfo::getYearOffset() const {
    return(yearOffset);
}

//*********************************************************************
//                      getLimbo
//*********************************************************************

int CatRefInfo::getLimbo() const {
    return(limbo);
}

//*********************************************************************
//                      getRecall
//*********************************************************************

int CatRefInfo::getRecall() const {
    return(recall);
}

//*********************************************************************
//                      getTeleportWeight
//*********************************************************************

int CatRefInfo::getTeleportWeight() const {
    return(teleportWeight);
}

//*********************************************************************
//                      getTeleportZone
//*********************************************************************

int CatRefInfo::getTeleportZone() const {
    return(teleportZone);
}

//*********************************************************************
//                      getTrackZone
//*********************************************************************

int CatRefInfo::getTrackZone() const {
    return(trackZone);
}

//*********************************************************************
//                      load
//*********************************************************************

void CatRefInfo::load(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    id = xml::getIntProp(rootNode, "id");
    while(curNode) {
             if(NODE_NAME(curNode, "Area")) xml::copyToBString(area, curNode);
        else if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
        else if(NODE_NAME(curNode, "Fishing")) xml::copyToBString(fishing, curNode);
        else if(NODE_NAME(curNode, "WorldName")) xml::copyToBString(worldName, curNode);
        else if(NODE_NAME(curNode, "YearsSince")) xml::copyToBString(yearsSince, curNode);
        else if(NODE_NAME(curNode, "Parent")) xml::copyToBString(parent, curNode);
        else if(NODE_NAME(curNode, "Limbo")) xml::copyToNum(limbo, curNode);
        else if(NODE_NAME(curNode, "YearOffset")) xml::copyToNum(yearOffset, curNode);
        else if(NODE_NAME(curNode, "Recall")) xml::copyToNum(recall, curNode);
        else if(NODE_NAME(curNode, "TeleportWeight")) xml::copyToNum(teleportWeight, curNode);
        else if(NODE_NAME(curNode, "TeleportZone")) xml::copyToNum(teleportZone, curNode);
        else if(NODE_NAME(curNode, "TrackZone")) xml::copyToNum(trackZone, curNode);
        else if(NODE_NAME(curNode, "Seasons")) loadSeasons(curNode);
        curNode = curNode->next;
    }

    // this would make an infinite loop
    if(parent == area)
        parent = "";
}

//*********************************************************************
//                      save
//*********************************************************************

void CatRefInfo::save(xmlNodePtr curNode) const {
    xmlNodePtr deepNode,subNode,childNode = xml::newStringChild(curNode, "Info");
    xml::newNumProp(childNode, "id", id);

    xml::saveNonNullString(childNode, "Area", area);
    xml::saveNonNullString(childNode, "Name", name);
    xml::saveNonNullString(childNode, "WorldName", worldName);
    xml::saveNonNullString(childNode, "YearsSince", yearsSince);
    xml::saveNonNullString(childNode, "Parent", parent);
    xml::saveNonZeroNum(childNode, "Limbo", limbo);
    xml::saveNonZeroNum(childNode, "YearOffset", yearOffset);
    xml::saveNonZeroNum(childNode, "Recall", recall);
    xml::saveNonZeroNum(childNode, "TeleportWeight", teleportWeight);
    xml::saveNonZeroNum(childNode, "TeleportZone", teleportZone);
    xml::saveNonZeroNum(childNode, "Track", trackZone);

    subNode = xml::newStringChild(childNode, "Seasons");
    std::map<Season,cSeason*>::const_iterator st;
    for(st = seasons.begin() ; st != seasons.end() ; st++) {
        deepNode = xml::newStringChild(subNode, "Season");
        (*st).second->save(deepNode);
    }
}

//*********************************************************************
//                      loadSeasons
//*********************************************************************

void CatRefInfo::loadSeasons(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    cSeason* season=0;

    while(childNode) {
        if(NODE_NAME(childNode, "Season")) {
            season = new cSeason;
            season->load(childNode);
            if(seasons[season->getId()])
                delete seasons[season->getId()];
            seasons[season->getId()] = season;
        }
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      getWeather
//*********************************************************************

const cWeather* CatRefInfo::getWeather() const {
    std::map<Season,cSeason*>::const_iterator st;
    st = seasons.find(gConfig->getCalendar()->getCurSeason()->getId());
    if(st != seasons.end())
        return((*st).second->getWeather());
    return(0);
}

//*********************************************************************
//                      str
//*********************************************************************

bstring CatRefInfo::str() const {
    std::ostringstream oStr;
    oStr << area;
    if(area == "area")
        oStr << " " << id;
    return(oStr.str());
}


//*********************************************************************
//                      loadCatRefInfo
//*********************************************************************

bool Config::loadCatRefInfo() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;
    CatRefInfo *cri=0;
    const CatRefInfo *parent=0;

    char filename[80];
    snprintf(filename, 80, "%s/catRefInfo.xml", Path::Game);
    xmlDoc = xml::loadFile(filename, "CatRefInfo");

    if(xmlDoc == NULL)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    bstring d = "";
    xml::copyPropToBString(d, curNode, "default");
    if(d != "")
        gConfig->defaultArea = d;

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode))
        curNode = curNode->next;

    if(curNode == 0) {
        xmlFreeDoc(xmlDoc);
        return(false);
    }

    clearCatRefInfo();
    while(curNode != NULL) {
        if(NODE_NAME(curNode, "Info")) {
            cri = new CatRefInfo;
            cri->load(curNode);
            catRefInfo.push_back(cri);
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();

    // propogate: this happens once, so if we ever need to know info from the
    // parent, we never need to look for it
    std::list<CatRefInfo*>::iterator it;
    for(it = catRefInfo.begin() ; it != catRefInfo.end() ; it++) {
        cri = (*it);
        if(cri->getParent() != "") {
            parent = getCatRefInfo(cri->getParent(), 0, true);
            if(parent)
                cri->copyVars(parent);
        }
    }
    return(true);
}

//*********************************************************************
//                      catRefName
//*********************************************************************

bstring Config::catRefName(bstring area) const {
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
//                      saveCatRefInfo
//*********************************************************************

void Config::saveCatRefInfo() const {
    std::list<CatRefInfo*>::const_iterator it;
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    char            filename[80];

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, NULL, BAD_CAST "CatRefInfo", NULL);
    xmlDocSetRootElement(xmlDoc, rootNode);
    xml::newProp(rootNode, "default", defaultArea.c_str());

    for(it = catRefInfo.begin() ; it != catRefInfo.end() ; it++)
        (*it)->save(rootNode);

    sprintf(filename, "%s/catRefInfo.xml", Path::Game);
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
}

//*********************************************************************
//                      getCatRefInfo
//*********************************************************************

const CatRefInfo* Config::getCatRefInfo(bstring area, int id, int shouldGetParent) const {
    std::list<CatRefInfo*>::const_iterator it;

    // prevent infinite loops from bad config files
    if(shouldGetParent > 30) {
        broadcast(isCt, "^yConfig::getCatRefInfo is terminating because it is looping too long.\nCheck the catrefinfo config file for possible looping issues.\n");
        return(0);
    }
    for(it = catRefInfo.begin() ; it != catRefInfo.end() ; it++) {
        if(area == (*it)->getArea() && id == (*it)->getId()) {
            if(shouldGetParent && (*it)->getParent() != "")
                return(getCatRefInfo((*it)->getParent(), 0, shouldGetParent+1));
            return(*it);
        }
    }

    return(0);
}

//*********************************************************************
//                      getCatRefInfo
//*********************************************************************

const CatRefInfo* Config::getCatRefInfo(const BaseRoom* room, int shouldGetParent) const {
    const AreaRoom* aRoom=0;
    const UniqueRoom* uRoom=0;
    int id = 0;
    bstring area = "";

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
        return(0);

    pick = mrand(1, total);
    total = 0;

    for(it = catRefInfo.begin() ; it != catRefInfo.end() ; it++) {
        if(!zone || zone == (*it)->getTeleportZone()) {
            total += (*it)->getTeleportWeight();
            if(total >= pick)
                return(*it);
        }
    }
    return(0);
}

//*********************************************************************
//                      dmCatRefInfo
//*********************************************************************

int dmCatRefInfo(Player* player, cmd* cmnd) {
    std::list<CatRefInfo*>::const_iterator it;
    std::ostringstream oStr;
    const CatRefInfo *cri=0;
    int i=0;

    oStr.setf(std::ios::left, std::ios::adjustfield);
    oStr << "^yCatRefInfo List^x\n";

    for(it = gConfig->catRefInfo.begin() ; it != gConfig->catRefInfo.end() ; it++) {
        cri = (*it);
        i = 20;
        if(cri->getName().getAt(0) == '^')
            i += 2;
        oStr << "  Area: ^c" << std::setw(10) << cri->str()
             << "^x Name: ^c" << std::setw(i)
             << cri->getName() << "^x ";
        if(cri->getParent() == "") {
            oStr << "TeleportZone: ^c" << std::setw(4) << cri->getTeleportZone() << "^x "
                 << "TrackZone: ^c" << std::setw(4) << cri->getTrackZone() << "^x "
                 << "TeleportWeight: ^c" << cri->getTeleportWeight();
            if(cri->getFishing() != "")
                oStr << " Fishing: ^c" << cri->getFishing();
            oStr << "^x\n"
                 << "                                              "
                 << "Limbo: ^c" << cri->getLimbo() << "^x    Recall: ^c" << cri->getRecall() << "^x\n";
        } else {
            oStr << "Parent: ^c" << cri->getParent() << "^x "
                 << "TeleportWeight: ^c" << cri->getTeleportWeight();
            if(cri->getFishing() != "")
                oStr << " Fishing: ^c" << cri->getFishing();
            oStr << "^x\n";
        }
    }

    player->printColor("%s\n", oStr.str().c_str());
    return(0);
}
