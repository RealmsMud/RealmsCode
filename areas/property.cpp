/*
 * property.h
 *   Code dealing with properties
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

#include <sstream>
#include <iomanip>

#include "catRefInfo.hpp"
#include "commands.hpp"
#include "config.hpp"
#include "creatures.hpp"
#include "dm.hpp"
#include "guilds.hpp"
#include "login.hpp"
#include "move.hpp"
#include "mud.hpp"
#include "rooms.hpp"
#include "property.hpp"
#include "server.hpp"
#include "socket.hpp"
#include "xml.hpp"

//*********************************************************************
//                      PartialOwner
//*********************************************************************

PartialOwner::PartialOwner() {
    name = "";
    zero(flags, sizeof(flags));
}

void PartialOwner::defaultFlags(PropType type) {
    switch(type) {
    case PROP_SHOP:
        setFlag(PROP_SHOP_CAN_STOCK);
        break;
    default:
        break;
    }
}

bstring PartialOwner::getName() const { return(name); }
void PartialOwner::setName(bstring str) { name = str; }

bool PartialOwner::flagIsSet(int flag) const {
    return(flags[flag/8] & 1<<(flag%8));
}
void PartialOwner::setFlag(int flag) {
    flags[flag/8] |= 1<<(flag%8);
}
void PartialOwner::clearFlag(int flag) {
    flags[flag/8] &= ~(1<<(flag%8));
}
bool PartialOwner::toggleFlag(int flag) {
    if(flagIsSet(flag))
        clearFlag(flag);
    else
        setFlag(flag);
    return(flagIsSet(flag));
}

void PartialOwner::load(xmlNodePtr rootNode) {
    xml::copyPropToBString(name, rootNode, "Name");
    loadBits(rootNode, flags);
}

void PartialOwner::save(xmlNodePtr rootNode) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "Owner") ;

    xml::newProp(curNode, "Name", name.c_str());
    for(int i=0; i<32; i++) {
        if(BIT_ISSET(flags, i))
            saveBit(curNode, i);
    }
}

//*********************************************************************
//                      Property
//*********************************************************************

Property::Property() {
    guild = 0;
    owner = dateFounded = location = "";
    logType = LOG_PARTIAL;
    type = PROP_NONE;
    zero(flags, sizeof(flags));
}

//*********************************************************************
//                      flagIsSet
//*********************************************************************

bool Property::flagIsSet(int flag) const {
    return(flags[flag/8] & 1<<(flag%8));
}

//*********************************************************************
//                      setFlag
//*********************************************************************

void Property::setFlag(int flag) {
    flags[flag/8] |= 1<<(flag%8);
}

//*********************************************************************
//                      clearFlag
//*********************************************************************

void Property::clearFlag(int flag) {
    flags[flag/8] &= ~(1<<(flag%8));
}

//*********************************************************************
//                      toggleFlag
//*********************************************************************

bool Property::toggleFlag(int flag) {
    if(flagIsSet(flag))
        clearFlag(flag);
    else
        setFlag(flag);
    return(flagIsSet(flag));
}

//*********************************************************************
//                      getGuild
//*********************************************************************

int Property::getGuild() const { return(guild); }

//*********************************************************************
//                      getArea
//*********************************************************************

bstring Property::getArea() const { return(area); }

//*********************************************************************
//                      getOwner
//*********************************************************************

bstring Property::getOwner() const { return(owner); }

//*********************************************************************
//                      getName
//*********************************************************************

bstring Property::getName() const { return(name); }

//*********************************************************************
//                      getDateFounded
//*********************************************************************

bstring Property::getDateFounded() const { return(dateFounded); }

//*********************************************************************
//                      getLocation
//*********************************************************************

bstring Property::getLocation() const { return(location); }

//*********************************************************************
//                      getType
//*********************************************************************

PropType Property::getType() const { return(type); }

//*********************************************************************
//                      getTypeName
//*********************************************************************

bstring Property::getTypeStr() const { return(getTypeStr(type)); }

//*********************************************************************
//                      getLogType
//*********************************************************************

PropLog Property::getLogType() const { return(logType); }

//*********************************************************************
//                      setGuild
//*********************************************************************

void Property::setGuild(int g) { guild = g; }

//*********************************************************************
//                      setArea
//*********************************************************************

void Property::setArea(bstring str) { area = str; }

//*********************************************************************
//                      setOwner
//*********************************************************************

void Property::setOwner(bstring str) { owner = str; }

//*********************************************************************
//                      setName
//*********************************************************************

void Property::setName(bstring str) { name = str; }

//*********************************************************************
//                      setDateFounded
//*********************************************************************

void Property::setDateFounded() {
    long    t = time(0);
    dateFounded = ctime(&t);
    dateFounded.trim();
}

//*********************************************************************
//                      setLocation
//*********************************************************************

void Property::setLocation(bstring str) { location = str; }

//*********************************************************************
//                      setType
//*********************************************************************

void Property::setType(PropType t) { type = t; }

//*********************************************************************
//                      setLogType
//*********************************************************************

void Property::setLogType(PropLog t) { logType = t; }

//*********************************************************************
//                      load
//*********************************************************************

void Property::load(xmlNodePtr rootNode) {
    xmlNodePtr childNode, curNode = rootNode->children;
    bstring temp = "";
    CatRef low;

    while(curNode) {
             if(NODE_NAME(curNode, "Owner")) xml::copyToBString(owner, curNode);
        else if(NODE_NAME(curNode, "Area")) xml::copyToBString(area, curNode);
        else if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
        else if(NODE_NAME(curNode, "DateFounded")) xml::copyToBString(dateFounded, curNode);
        else if(NODE_NAME(curNode, "Location")) xml::copyToBString(location, curNode);
        else if(NODE_NAME(curNode, "Guild")) xml::copyToNum(guild, curNode);
        else if(NODE_NAME(curNode, "Type")) type = (PropType)xml::toNum<int>(curNode);
        else if(NODE_NAME(curNode, "Flags")) loadBits(curNode, flags);

        else if(NODE_NAME(curNode, "Ranges")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Range")) {
                    Range r;
                    r.load(childNode);

                    // find the first in the range
                    if(!low.id)
                        low = r.low;

                    ranges.push_back(r);
                }
                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "PartialOwners")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Owner")) {
                    PartialOwner po;
                    po.load(childNode);
                    if(po.getName() != "")
                        partialOwners.push_back(po);
                }
                childNode = childNode->next;
            }
        }
        else if(NODE_NAME(curNode, "LogType")) logType = (PropLog)xml::toNum<int>(curNode);
        else if(NODE_NAME(curNode, "Log")) {
            childNode = curNode->children;
            while(childNode) {
                if(NODE_NAME(childNode, "Entry")) {
                    xml::copyToBString(temp, childNode);
                    log.push_back(temp);
                }
                childNode = childNode->next;
            }
        }
        curNode = curNode->next;
    }

    // backwards compatability - removable when all are updated
    if(type != PROP_STORAGE && area == "") {
        // load intro room, find the out exit, look at the room info
        UniqueRoom* room=0;
        if(loadRoom(low, &room)) {
            for(Exit* ext : room->exits) {
                if(ext->target.room.area != "shop" && low.area != ext->target.room.area) {
                    area = ext->target.room.area;
                    break;
                }
            }
        }
    }
}

//*********************************************************************
//                      save
//*********************************************************************

void Property::save(xmlNodePtr rootNode) const {
    xmlNodePtr childNode, curNode = xml::newStringChild(rootNode, "Property");

    xml::saveNonNullString(curNode, "Owner", owner.c_str());
    xml::saveNonNullString(curNode, "Area", area.c_str());
    xml::saveNonNullString(curNode, "Name", name.c_str());
    xml::saveNonNullString(curNode, "DateFounded", dateFounded.c_str());
    xml::saveNonNullString(curNode, "Location", location.c_str());
    xml::saveNonZeroNum(curNode, "Guild", guild);
    xml::saveNonZeroNum(curNode, "Type", (int)type);
    saveBits(curNode, "Flags", 32, flags);

    if(!ranges.empty()) {
        std::list<Range>::const_iterator rt;
        childNode = xml::newStringChild(curNode, "Ranges");

        for(rt = ranges.begin() ; rt != ranges.end() ; rt++)
            (*rt).save(childNode, "Range");
    }

    if(!partialOwners.empty()) {
        std::list<PartialOwner>::const_iterator pt;
        childNode = xml::newStringChild(curNode, "PartialOwners");

        for(pt = partialOwners.begin() ; pt != partialOwners.end() ; pt++)
            (*pt).save(childNode);
    }
    xml::saveNonZeroNum(curNode, "LogType", logType);
    if(!log.empty()) {
        std::list<bstring>::const_iterator it;
        childNode = xml::newStringChild(curNode, "Log");

        for(it = log.begin() ; it != log.end() ; it++) {
            xml::saveNonNullString(childNode, "Entry", (*it).c_str());
        }
    }
}

//*********************************************************************
//                      getTypeName
//*********************************************************************

bstring Property::getTypeStr(PropType propType) {
    switch(propType) {
    case PROP_STORAGE:
        return("storage room");
    case PROP_SHOP:
        return("shop");
    case PROP_GUILDHALL:
        return("guild hall");
    case PROP_HOUSE:
        return("personal house");
    default:
        break;
    }
    return("unknown type");
}

//*********************************************************************
//                      getTypeArea
//*********************************************************************

bstring Property::getTypeArea(PropType propType) {
    switch(propType) {
    case PROP_SHOP:
        return("shop");
    case PROP_STORAGE:
        return("stor");
    case PROP_GUILDHALL:
        return("guild");
    case PROP_HOUSE:
        return("house");
    default:
        break;
    }

    return("");
}

//*********************************************************************
//                      getLogTypeStr
//*********************************************************************

bstring Property::getLogTypeStr() const {
    switch(logType) {
    case LOG_NONE:
        return("This property is not recording actions.");
        break;
    case LOG_PARTIAL:
        return("This property is recording all actions of partial owners.");
        break;
    case LOG_ALL:
    default:
        break;
    }
    return("This property is recording all actions.");
}

//*********************************************************************
//                      addRange
//*********************************************************************

void Property::addRange(CatRef cr) {
    // first of all, let us intelligently try to update existing ranges
    std::list<Range>::iterator rt;
    for(rt = ranges.begin() ; rt != ranges.end() ; rt++) {
        if((*rt).isArea(cr.area) && (*rt).high == cr.id - 1) {
            (*rt).high = cr.id;
            return;
        }
    }
    Range r;
    r.low = cr;
    r.high = r.low.id;
    ranges.push_back(r);
}

void Property::addRange(bstring area, int low, int high) {
    // first of all, let us intelligently try to update existing ranges
    std::list<Range>::iterator rt;
    for(rt = ranges.begin() ; rt != ranges.end() ; rt++) {
        if((*rt).isArea(area) && (*rt).high == low - 1) {
            (*rt).high = high;
            return;
        }
    }
    Range r;
    r.low.setArea(area);
    r.low.id = low;
    r.high = high;
    ranges.push_back(r);
}

//*********************************************************************
//                      isOwner
//*********************************************************************

bool Property::isOwner(const bstring& str) const {
    return(owner == str);
}

//*********************************************************************
//                      belongs
//*********************************************************************

bool Property::belongs(CatRef cr) const {
    std::list<Range>::const_iterator rt;
    for(rt = ranges.begin() ; rt != ranges.end() ; rt++)
        if((*rt).belongs(cr))
            return(true);
    return(false);
}

//*********************************************************************
//                      getFlagList
//*********************************************************************

std::map<int, MudFlag>* Property::getFlagList() {
    switch(type) {
    case PROP_STORAGE:
        return(&gConfig->propStorFlags);
    case PROP_SHOP:
        return(&gConfig->propShopFlags);
    case PROP_GUILDHALL:
        return(&gConfig->propGuildFlags);
    case PROP_HOUSE:
        return(&gConfig->propHouseFlags);
    default:
        break;
    }
    return(0);
}

//*********************************************************************
//                      viewLogFlag
//*********************************************************************

int Property::viewLogFlag() const {
    switch(type) {
    case PROP_STORAGE:
        return(PROP_STOR_VIEW_LOG);
    case PROP_SHOP:
        return(PROP_SHOP_VIEW_LOG);
    default:
        // other proprties do not use this function
        break;
    }
    return(-1);
}

//*********************************************************************
//                      usePropFlags
//*********************************************************************

bool Property::usePropFlags(PropType propType) {
    return(propType == PROP_HOUSE || propType == PROP_GUILDHALL);
}

//*********************************************************************
//                      canEnter
//*********************************************************************

bool Property::canEnter(const Player* player, const UniqueRoom* room, bool p) {
    // staff may always go anywhere
    int staff = player->isStaff();

    if(room->info.isArea(getTypeArea(PROP_STORAGE))) {
        CatRef cr = gConfig->getSingleProperty(player, PROP_STORAGE);
        if(cr != room->info) {
            if(p) player->checkStaff("You are not permited in there.\n");
            if(!staff) return(false);
        }

        if(player->getLevel() < 10) {
            if(p) player->checkStaff("You must be level 10 to go there.\n");
            if(!staff) return(false);
        }
    }

    if( room->info.isArea(getTypeArea(PROP_GUILDHALL)) &&
        !room->flagIsSet(R_GUILD_OPEN_ACCESS)
    ) {
        const Guild *guild=0, *pGuild=0;

        if(player && player->getGuild())
            guild = gConfig->getGuild(player->getGuild());

        Property *prop = gConfig->getProperty(room->info);
        if(prop && prop->getGuild())
            pGuild = gConfig->getGuild(prop->getGuild());
        if(!pGuild) {
            if(p) player->checkStaff("Sorry, that guildhall is broken.\n");
            if(!staff) return(false);
        }

        if( (!guild || guild->getNum() != pGuild->getNum()) &&
            (!prop || !prop->flagIsSet(PROP_GUILD_PUBLIC))
        ) {
            if(p) player->checkStaff("You must belong to %s to go that way.\n", pGuild->getName().c_str());
            if(!staff) return(false);
        }
    }

    if(room->info.isArea(getTypeArea(PROP_HOUSE))) {
        Property *prop = gConfig->getProperty(room->info);

        if( prop &&
            prop->flagIsSet(PROP_HOUSE_PRIVATE) &&
            !prop->isOwner(player->getName()) &&
            !prop->isPartialOwner(player->getName())
        ) {
            if(p) player->checkStaff("That personal house is private property.\n");
            if(!staff) return(false);
        }
    }

    return(true);
}

//*********************************************************************
//                      goodNameDesc
//*********************************************************************

bool Property::goodNameDesc(const Player* player, bstring str, bstring fail, bstring disallow) {
    if(str == "") {
        player->print("%s\n", fail.c_str());
        return(false);
    }
    if(str.getLength() > 79) {
        player->print("Too long.\n");
        return(false);
    }
    if(Pueblo::is(str)) {
        player->print("That %s is not allowed.\n", disallow.c_str());
        return(false);
    }
    return(true);
}

//*********************************************************************
//                      goodExit
//*********************************************************************

bool Property::goodExit(const Player* player, const BaseRoom* room, const char *type, bstring xname) {
    if(xname.getLength() > 19) {
        player->print("%s exit name is too long!\n", type);
        return(false);
    }
    if(xname.getLength() < 3) {
        player->print("%s exit name is too short!\n", type);
        return(false);
    }

    if(isCardinal(xname)) {
        player->print("Exits cannot be made cardinal directions.\n");
        return(false);
    }

    int len = xname.getLength();
    for(int i=0; i<len; i++) {
        if(isupper(xname.getAt(i))) {
            player->print("Do not use capital letters in exit names.\n");
            return(false);
        }
        if(xname.getAt(i) == '^') {
            player->print("Carets (^) are not allowed in exit names.\n");
            return(false);
        }
    }

    for(Exit* ext : room->exits) {
        if(ext->getName() == xname) {
            player->print("An exit with that name already exists in this room.\n");
            return(false);
        }
    }

    return(true);
}

//*********************************************************************
//                      destroy
//*********************************************************************

void Property::destroy() {
    BaseRoom* outside=0;
    AreaRoom* aRoom=0;
    UniqueRoom  *room=0, *uRoom=0;
    std::list<Range>::iterator rt;

    for(rt = ranges.begin() ; rt != ranges.end() ; rt++) {
        for(; (*rt).low.id <= (*rt).high; (*rt).low.id++) {
            if(loadRoom((*rt).low, &room)) {
                // for every property besides storage rooms, we have to delete the
                // exit leading to the property

                if(type != PROP_STORAGE) {
                    // handle the exits that point outside
                    for(Exit* ext : room->exits) {
                        if( ext->target.mapmarker.getArea() ||
                            !ext->target.room.isArea(room->info.area) )
                        {
                            outside = ext->target.loadRoom();
                            if(outside) {
                                uRoom = outside->getAsUniqueRoom();
                                aRoom = outside->getAsAreaRoom();

                                // get rid of all exits
                                ExitList::iterator xit;
                                for(xit = outside->exits.begin() ; xit != outside->exits.end() ; ) {
                                    Exit* oExit = (*xit++);
                                    if(oExit->target.room == room->info) {
                                        broadcast(nullptr, outside, "%s closes its doors.", name.c_str());
                                        outside->delExit(oExit);
                                    }
                                }

                                // reset flags
                                if(uRoom) {
                                    if(type == PROP_SHOP && uRoom->flagIsSet(R_WAS_BUILD_SHOP)) {
                                        uRoom->clearFlag(R_WAS_BUILD_SHOP);
                                        uRoom->setFlag(R_BUILD_SHOP);
                                    } else if(type == PROP_GUILDHALL && uRoom->flagIsSet(R_WAS_BUILD_GUILDHALL)) {
                                        uRoom->clearFlag(R_WAS_BUILD_GUILDHALL);
                                        uRoom->setFlag(R_BUILD_GUILDHALL);
                                    }

                                    uRoom->saveToFile(0);
                                } else if(aRoom) {
                                    aRoom->save();
                                }
                            }
                        }
                    }
                }

                broadcast(nullptr, room, "%s closes its doors.", name.c_str());
                room->destroy();
            }
        }
    }

    delete this;
}

//*********************************************************************
//                      destroyProperties
//*********************************************************************
// This function is called when a player is destroyed and everything
// they own must be destroyed as well.

void Config::destroyProperties(bstring pOwner) {
    std::list<Property*>::iterator it;

    // giving guildhalls a new owner occurs when the player leaves the guild

    for(it = properties.begin() ; it != properties.end() ; ) {
        if((*it)->isOwner(pOwner)) {
            (*it)->destroy();
            it = properties.erase(it);
            continue;
        } else if((*it)->isPartialOwner(pOwner)) {
            (*it)->unassignPartialOwner(pOwner);
            (*it)->appendLog(pOwner, "%s has resigned as partial owner.", pOwner.c_str());
        }
        it++;
    }

    saveProperties();
}

//*********************************************************************
//                      destroyProperty
//*********************************************************************

void Config::destroyProperty(Property *p) {
    std::list<Property*>::iterator it;

    for(it = properties.begin() ; it != properties.end() ; it++) {
        if((*it) == p) {
            p->destroy();
            properties.erase(it);
            saveProperties();
            return;
        }
    }
}


//*********************************************************************
//                      partial owner functions
//*********************************************************************
// guildhalls do not use the partialowner list: they rely on getting
// their information from the guild

PartialOwner* Property::getPartialOwner(const bstring& pOwner) {
    std::list<PartialOwner>::iterator it;
    for(it = partialOwners.begin() ; it != partialOwners.end() ; it++) {
        if(pOwner == (*it).getName())
            return(&(*it));
    }
    return(0);
}

bool Property::isPartialOwner(const bstring& pOwner) {
    // shops will also check guild membership, if they're assigned to a guild
    if(type == PROP_SHOP) {
        if(!!getPartialOwner(pOwner))
            return(true);
    } else if(type != PROP_GUILDHALL)
        return(!!getPartialOwner(pOwner));
    if(!guild)
        return(false);
    Guild *g = gConfig->getGuild(guild);
    return(g && g->isMember(pOwner));
}

void Property::assignPartialOwner(bstring pOwner) {
    if(type == PROP_GUILDHALL)
        return;
    PartialOwner po;
    po.defaultFlags(type);
    po.setName(pOwner);
    partialOwners.push_back(po);
}

void Property::unassignPartialOwner(bstring pOwner) {
    std::list<PartialOwner>::iterator it;
    for(it = partialOwners.begin() ; it != partialOwners.end() ; it++) {
        if(pOwner == (*it).getName()) {
            partialOwners.erase(it);
            return;
        }
    }
}


//*********************************************************************
//                      addProperty
//*********************************************************************

void Config::addProperty(Property* p) {
    properties.push_back(p);
    saveProperties();
}

//*********************************************************************
//                      loadProperties
//*********************************************************************

bool Config::loadProperties() {
    xmlDocPtr xmlDoc;
    xmlNodePtr curNode;
    Property *p=0;

    char filename[80];
    snprintf(filename, 80, "%s/properties.xml", Path::PlayerData);
    xmlDoc = xml::loadFile(filename, "Properties");

    if(xmlDoc == nullptr)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode))
        curNode = curNode->next;

    if(curNode == 0) {
        xmlFreeDoc(xmlDoc);
        return(false);
    }

    clearProperties();
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "Property")) {
            p = new Property;
            p->load(curNode);
            properties.push_back(p);
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();

    return(true);
}

//*********************************************************************
//                      saveProperties
//*********************************************************************

bool Config::saveProperties() const {
    std::list<Property*>::const_iterator it;
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    char            filename[80];

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Properties", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    for(it = properties.begin() ; it != properties.end() ; it++) {
        (*it)->save(rootNode);
    }

    sprintf(filename, "%s/properties.xml", Path::PlayerData);
    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
    return(true);
}

//*********************************************************************
//                      getSingleProperty
//*********************************************************************
// cr.id = 0 means they have no property
// cr.id = -1 means they have too many properties
// otherwise, cr points to the property's first room

CatRef Config::getSingleProperty(const Player* player, PropType type) {
    std::list<Property*>::iterator it;
    CatRef  cr;

    for(it = gConfig->properties.begin() ; it != gConfig->properties.end() ; it++) {
        if((*it)->getType() != type)
            continue;
        if(type == PROP_STORAGE && !(*it)->isOwner(player->getName()) && !(*it)->isPartialOwner(player->getName()))
            continue;
        if(type == PROP_GUILDHALL && player->getGuild() != (*it)->getGuild())
            continue;

        if(cr.id)
            cr.id = -1;
        else
            cr = (*it)->ranges.front().low;
    }
    return(cr);
}

//*********************************************************************
//                      expelOnRemove
//*********************************************************************

bool Property::expelOnRemove() const {
    return( type == PROP_STORAGE ||
            type == PROP_GUILDHALL ||
            (type == PROP_HOUSE && flagIsSet(PROP_HOUSE_PRIVATE))
    );
}

//*********************************************************************
//                      expelToExit
//*********************************************************************
// can safely call this function; it will decide if the player needs
// to be kicked out or not

void Property::expelToExit(Player* player, bool offline) {
    if(!expelOnRemove() || player->inAreaRoom() || !belongs(player->currentLocation.room))
        return;

    AreaRoom* aRoom=0;
    UniqueRoom* uRoom=0;
    BaseRoom* newRoom=0;

    if(loadRoom(ranges.front().low, &uRoom)) {
        for(Exit* ext : uRoom->exits ) {
            if(!ext->target.room.id || !belongs(ext->target.room)) {
                newRoom = ext->target.loadRoom(player);
                if(newRoom)
                    break;
            }
        }
        uRoom = 0;
    }

    if(!newRoom)
        newRoom = player->bound.loadRoom(player);

    if(!newRoom)
        newRoom = abortFindRoom(player, "expelToExit");

    uRoom = newRoom->getAsUniqueRoom();
    aRoom = newRoom->getAsAreaRoom();

    if(offline) {
        player->currentLocation.room.clear();
        player->currentLocation.mapmarker.reset();
        if(uRoom)
            player->currentLocation.room = uRoom->info;
        else
            player->currentLocation.mapmarker = aRoom->mapmarker;
    } else {
        player->print("You are escorted off the premises.\n");
        broadcast(player->getSock(), player->getParent(), "%M is escorted off the premises.", player);
        player->deleteFromRoom();
        player->addToRoom(newRoom);
        player->doPetFollow();
    }
    player->save();
}

//*********************************************************************
//                      propCommands
//*********************************************************************

void propCommands(Player* player, Property *p) {
    player->print("Commands for this property:\n");

    if(p->isOwner(player->getName())) {
        // example showing width
        //player->printColor("           <commands>            - description\n");

        switch(p->getType()) {
        case PROP_STORAGE:
        case PROP_HOUSE:
            player->printColor("  property ^e<^xassign^e>^x ^e<^cplayer^e>^x     ^e-^x add a partial owner\n");
            player->printColor("           ^e<^xunassign^e>^x ^e<^cplayer^e>^x   ^e-^x remove a partial owner\n");
            break;
        case PROP_SHOP:
            player->printColor("  property ^e<^xhire^e>^x ^e<^cplayer^e>^x       ^e-^x add a partial owner\n");
            player->printColor("           ^e<^xfire^e>^x ^e<^cplayer^e>^x       ^e-^x remove a partial owner\n");
            if(player->getGuild())
                player->printColor("           ^e<^xguild^e>^x ^e<^xassign^e|^xremove^e>\n");
            break;
        case PROP_GUILDHALL:
            player->printColor("  property ^e<^xdestroy^e>^x             ^e-^x close this property down\n");
            break;
        default:
            player->print("  None.\n");
            return;
        }


        if(Property::usePropFlags(p->getType()))
            player->printColor("           ^e<^xflags^e>^x               ^e-^x view / set flags on property\n");
        else
            player->printColor("           ^e<^xflags^e>^x               ^e-^x view / set flags on partial owners\n");

        if(p->getType() != PROP_GUILDHALL) {
            player->printColor("           ^e<^xlog^e>^x ^e<^xdelete^e|^xrecord^e>^x ^e-^x view / delete / set log type\n");
            player->printColor("           ^e<^xdestroy^e>^x             ^e-^x close this property down\n");
        }
        player->printColor("           ^e<^xhelp^e>\n");
    } else {
        PartialOwner *po = p->getPartialOwner(player->getName());
        if(!po)
            player->print("  none.\n");
        else {
            player->printColor("  property ^e<^xunassign^e>\n");

            if(p->getType() != PROP_GUILDHALL) {
                player->printColor("           ^e<^xflags^e>\n");

                if( p->getType() != PROP_HOUSE &&
                    po &&
                    po->flagIsSet(p->viewLogFlag())
                )
                    player->printColor("           ^e<^xlog^e>\n");
            }
        }
    }

    player->print("\n");
}

//*********************************************************************
//                      propLog
//*********************************************************************
// handles property logging

void propLog(Player* player, cmd* cmnd, Property *p) {
    if(!cmnd || cmnd->num == 2) {
        player->print("Property Log For %s:\n", p->getName().c_str());
        player->printColor("^b--------------------------------------------------------------\n");
        player->printColor("%s\n", p->getLog().c_str());
    } else if(!strcmp(cmnd->str[2], "delete")) {
        player->print("Property log deleted.\n");
        p->clearLog();
        gConfig->saveProperties();
    } else if(!strcmp(cmnd->str[2], "record")) {
        if(cmnd->num == 3) {
            player->print("%s\n", p->getLogTypeStr().c_str());
            player->printColor("To change this, type ^yproperty log record <all|partial|none>^x.\n");
        } else {
            if(!strncmp(cmnd->str[3], "all", strlen(cmnd->str[3])))
                p->setLogType(LOG_ALL);
            else if(!strncmp(cmnd->str[3], "partial", strlen(cmnd->str[3])))
                p->setLogType(LOG_PARTIAL);
            else if(!strncmp(cmnd->str[3], "none", strlen(cmnd->str[3])))
                p->setLogType(LOG_NONE);
            else {
                player->printColor("Log recording type not understood.\nType ^yproperty log record <all|partial|none>^x.\n");
                return;
            }
            player->print("Property log type set.\n");
            gConfig->saveProperties();
        }
    } else {
        player->print("Command not understood.\n");
        propCommands(player, p);
    }
    return;
}

//*********************************************************************
//                      propDestroy
//*********************************************************************
// handles property destruction

void propDestroy(Player* player, cmd* cmnd, Property *p) {
    if(strcmp(cmnd->str[2], "confirm")) {
        player->printColor("^rAre you sure you wish to destroy this property?\n");
        player->printColor("Type \"property destroy confirm\" to destroy it.\n");
    } else {
        gConfig->destroyProperty(p);
    }
}

//*********************************************************************
//                      propAssignUnassign
//*********************************************************************
// handles assigning/unassigning of partial owners

void propAssignUnassign(Player* player, cmd* cmnd, Property *p, bool assign) {
    if(p->getType() == PROP_GUILDHALL) {
        player->print("Partial ownership of guildhalls is handled by guild membership.\n");
        return;
    }

    bool    offline=false;

    // find out who they're trying to work with
    if(cmnd->num < 3) {
        player->print("Player does not exist.\n");
        return;
    }

    cmnd->str[2][0] = up(cmnd->str[2][0]);
    Player* target = gServer->findPlayer(cmnd->str[2]);

    if(!target) {
        if(!loadPlayer(cmnd->str[2], &target)) {
            player->print("Player does not exist.\n");
            return;
        }
        offline = true;
    }
    if((target->isStaff() && !player->isStaff()) || target == player) {
        player->print("That player is not a valid target.\n");

        if(offline) free_crt(target);
        return;
    }


    // is this action even valid?
    if(assign && p->getPartialOwner(target->getName())) {
        player->print("%s is already a partial owner of this property.\n", target->getCName());

        if(offline) free_crt(target);
        return;
    } else if(!assign && !p->getPartialOwner(target->getName())) {
        player->print("%s is not a partial owner of this property.\n", target->getCName());

        if(offline) free_crt(target);
        return;
    }

    // their action is valid: let's continue
    if(assign) {

        if(Move::tooFarAway(player, target, "appoint as partial owner of this property")) {
            if(offline) free_crt(target);
            return;
        }

        // people can only belong to one storage room
        if(p->getType() == PROP_STORAGE) {
            CatRef  cr = gConfig->getSingleProperty(target, PROP_STORAGE);
            if(cr.id) {
                player->print("%s may only be affiliated with one storage room at a time.\n", target->getCName());

                if(offline) free_crt(target);
                return;
            }
        }

        p->assignPartialOwner(target->getName());
        p->appendLog(target->getName(), "%s is now a partial owner.", target->getCName());
        player->print("%s is now a partial owner of this property.\n", target->getCName());
        if(!offline) {
            target->printColor( "^c%s has appointed you partial owner of %s.\n",
                player->getCName(), p->getName().c_str());
            target->print("Type \"property\" for instructions on how to remove yourself\nif you do not wish to be partial owner.\n");
        }
    } else {
        p->unassignPartialOwner(target->getName());
        p->appendLog(target->getName(), "%s is no longer a partial owner.", target->getCName());
        player->print("%s is no longer a partial owner of this property.\n", target->getCName());
        if(!offline) {
            target->printColor( "^c%s has removed you as partial owner of %s.\n",
                player->getCName(), p->getName().c_str());
        }
        p->expelToExit(target, offline);
    }

    gConfig->saveProperties();
    if(offline) free_crt(target);
}

//*********************************************************************
//                      propFlags
//*********************************************************************
// handles viewing/setting of flags on partial owners

void propFlags(Player* player, cmd* cmnd, Property *p) {
    std::map<int, MudFlag>* list = p->getFlagList();
    std::map<int, MudFlag>::iterator it;
    PartialOwner *po=0;
    MudFlag f;
    bool propFlag = Property::usePropFlags(p->getType());

    if(cmnd->num == 2) {

        player->printColor("^yFlags for %sproperty type: %s^x\n\n",
            propFlag ? "" : "Partial Owners for ", p->getTypeStr().c_str());

        for(it = list->begin(); it != list->end() ; it++) {
            f = (*it).second;
            player->printColor("^c#%-2d^x %-20s ^e-^x %s\n",
                f.id, f.name.c_str(), f.desc.c_str());
        }

        player->print("\nThese commands allow you to change flags on %s:\n",
            propFlag ? "this property" : "partial owners");
        player->printColor("   property flags %s^e<^xview^e>\n",
            propFlag ? "" : "^e<^cplayer^e>^x ");
        player->printColor("   property flags %s^e<^xset^e|^xclear^e|^xtoggle^e>^x ^e<^cflag num^e>^x\n\n",
            propFlag ? "" : "^e<^cplayer^e>^x ");

    } else if(propFlag) {

        bstring action = cmnd->str[2];

        if(action == "" || action == "view") {

            player->printColor("^yFlags set for this property:^x\n\n");
            for(it = list->begin(); it != list->end() ; it++) {
                f = (*it).second;
                if(p->flagIsSet(f.id-1))
                    player->printColor("^c#%-2d^x %s\n", f.id, f.name.c_str());
            }
            player->print("\n");

        } else if(action == "set" || action == "clear" || action == "toggle") {

            int num = atoi(getFullstrText(cmnd->fullstr, 3).c_str());
            bool found=false;

            for(it = list->begin(); it != list->end() ; it++) {
                f = (*it).second;
                if(num == f.id) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                player->print("Invalid flag!\n");
                return;
            }

            num--;
            if(action == "set")
                p->setFlag(num);
            else if(action == "clear")
                p->clearFlag(num);
            else
                p->toggleFlag(num);


            if(p->flagIsSet(num))
                player->printColor("Flag ^c""%s""^x set on this property.\n", f.name.c_str());
            else
                player->printColor("Flag ^c""%s""^x cleared on this property.\n", f.name.c_str());

            gConfig->saveProperties();

        } else {
            player->print("Command not understood.\n");
            propCommands(player, p);
        }

    } else {

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        po = p->getPartialOwner(cmnd->str[2]);
        if(!po) {
            player->print("%s is not a partial owner of this property!\n", cmnd->str[2]);
            return;
        }

        bstring action = cmnd->str[3];

        if(action == "" || action == "view") {

            // if viewing self, show property name instead
            bstring title = po->getName();
            if(title == player->getName())
                title = p->getName();

            player->printColor("^yFlags set for %s:^x\n\n", title.c_str());
            for(it = list->begin(); it != list->end() ; it++) {
                f = (*it).second;
                if(po->flagIsSet(f.id-1))
                    player->printColor("^c#%-2d^x %s\n", f.id, f.name.c_str());
            }
            player->print("\n");

        } else if(action == "set" || action == "clear" || action == "toggle") {

            int num = atoi(getFullstrText(cmnd->fullstr, 4).c_str());
            bool found=false;

            for(it = list->begin(); it != list->end() ; it++) {
                f = (*it).second;
                if(num == f.id) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                player->print("Invalid flag!\n");
                return;
            }

            num--;
            if(action == "set")
                po->setFlag(num);
            else if(action == "clear")
                po->clearFlag(num);
            else
                po->toggleFlag(num);

            Player* target = gServer->findPlayer(po->getName().c_str());
            if(po->flagIsSet(num)) {
                player->printColor("Flag ^c""%s""^x set on %s.\n", f.name.c_str(),
                    po->getName().c_str());
                if(target)
                    target->printColor("Flag ^c""%s""^x has been set on you for %s.\n",
                        f.name.c_str(), p->getName().c_str());
            } else {
                player->printColor("Flag ^c""%s""^x cleared on %s.\n", f.name.c_str(),
                    po->getName().c_str());
                if(target)
                    target->printColor("Flag ^c""%s""^x has been cleared on you for %s.\n",
                        f.name.c_str(), p->getName().c_str());
            }

            gConfig->saveProperties();

        } else {
            player->print("Command not understood.\n");
            propCommands(player, p);
        }
    }
}

//*********************************************************************
//                      propRemove
//*********************************************************************
// handles assigning/unassigning of partial owners

void propRemove(Player* player, Property *p) {
    if(p->getType() == PROP_GUILDHALL) {
        player->print("Partial ownership of guildhalls is handled by guild membership.\n");
        return;
    }

    p->unassignPartialOwner(player->getName());
    p->appendLog(player->getName(), "%s has resigned as partial owner.", player->getCName());

    Player* owner = gServer->findPlayer(p->getOwner().c_str());
    if(owner) {
        owner->printColor("^c%s has resigned as partial owner of %s.\n",
            player->getCName(), p->getName().c_str());
    }

    player->print("You are no longer a partial owner of %s.\n", p->getName().c_str());
    gConfig->saveProperties();
}

//*********************************************************************
//                      propHelp
//*********************************************************************

void propHelp(Player* player, cmd* cmnd, PropType propType) {
    if(propType == PROP_GUILDHALL)
        strcpy(cmnd->str[1], "guildhalls");
    else if(propType == PROP_HOUSE)
        strcpy(cmnd->str[1], "houses");
    else
        strcpy(cmnd->str[1], "property");
    cmdHelp(player, cmnd);
}

//*********************************************************************
//                      cmdProperties
//*********************************************************************

int cmdProperties(Player* player, cmd* cmnd) {
    Property *p=0;
    PartialOwner *po=0;

    if(player->inUniqueRoom())
        p = gConfig->getProperty(player->getUniqueRoomParent()->info);

    int len = strlen(cmnd->str[1]);
    if(len && !strncmp(cmnd->str[1], "help", len)) {
        propHelp(player, cmnd, p ? p->getType() : PROP_NONE);
        return(0);
    }

    if(p)
        po = p->getPartialOwner(player->getName());

    // they only want to see their properties
    if(cmnd->num < 2) {

        player->printColor("Properties You Own\n");
        player->printColor("^b--------------------------------------------------------------\n");
        gConfig->showProperties(player, player);

        if(!p || (!p->isOwner(player->getName()) && !p->isPartialOwner(player->getName())))
            return(0);

        propCommands(player, p);

    } else if(!p) {

        // not in a property, trying to do stuff
        if(!strcmp(cmnd->str[1], "remove")) {
            int i=0;
            std::list<Property*>::iterator it;

            for(it = gConfig->properties.begin() ; it != gConfig->properties.end() ; it++) {
                if((*it)->isPartialOwner(player->getName()))
                    i++;
                if(i == cmnd->val[1]) {
                    propRemove(player, *it);
                    return(0);
                }
            }

            player->print("Property not found!\n");
        } else {
            player->print("Command not understood.\n");
            player->print("To manage your own properties, you must be on the premises.\n");
            player->print("Type \"property remove [num]\" to remove yourself as partial owner from a property.\n");
        }

    } else if(p->isOwner(player->getName())) {

        // they want to do something with this property
        bool found=true, assign=false;

        // figure out what they're trying to do
        if(!strcmp(cmnd->str[1], "destroy")) {

            propDestroy(player, cmnd, p);

        } else if(!strcmp(cmnd->str[1], "log") && p->getType() != PROP_HOUSE) {

            propLog(player, cmnd, p);

        } else if(p->getType() == PROP_SHOP && !strcmp(cmnd->str[1], "guild") && cmnd->num > 2) {

            // give them a cross-command shortcut
            cmdShop(player, cmnd);

        } else if(!strcmp(cmnd->str[1], "flags")) {

            propFlags(player, cmnd, p);

        } else {

            switch(p->getType()) {
            case PROP_STORAGE:
            case PROP_HOUSE:
                if(!strcmp(cmnd->str[1], "assign"))
                    assign = true;
                else if(strcmp(cmnd->str[1], "unassign"))
                    found = false;
                break;
            case PROP_SHOP:
                if(!strcmp(cmnd->str[1], "hire"))
                    assign = true;
                else if(strcmp(cmnd->str[1], "fire"))
                    found = false;
                break;
            default:
                found = false;
                break;
            }

            if(!found) {
                player->print("Command not understood.\n");
                propCommands(player, p);
                return(0);
            }

            propAssignUnassign(player, cmnd, p, assign);

        }

    } else if(po) {

        if( !strcmp(cmnd->str[1], "log") &&
            p->viewLogFlag() != -1 &&
            po->flagIsSet(p->viewLogFlag())
        ) {
            propLog(player, 0, p);
        } else if(!strcmp(cmnd->str[1], "flags") && p->getType() != PROP_HOUSE) {
            cmnd->num = 4;
            strcpy(cmnd->str[2], player->getCName());
            strcpy(cmnd->str[3], "view");
            propFlags(player, cmnd, p);
        } else if(!strcmp(cmnd->str[1], "unassign")) {
            propRemove(player, p);
            p->expelToExit(player, false);
        } else {
            player->print("Command not understood.\n");
            propCommands(player, p);
        }
    }

    return(0);
}


//*********************************************************************
//                      dmProperties
//*********************************************************************

int dmProperties(Player* player, cmd* cmnd) {
    bstring id = getFullstrText(cmnd->fullstr, 1);
    if(id == "") {
        player->printColor("Properties: Type ^c*property [num]^x to view more info.\n");
        player->printColor("Properties: Type ^c*property [type]^x to filter the list.\n");
        player->printColor("Properties: Type ^c*property destroy [num] [owner]^x to destroy a property.\n");
        player->printColor("^b--------------------------------------------------------------\n");
        gConfig->showProperties(player, 0);
    } else if(!strcmp(cmnd->str[1], "destroy")) {
        int num = cmnd->val[1];
        int i=1;

        if(!strlen(cmnd->str[2])) {
            player->printColor("You must specify the owner of the property you wish to destroy.\nType ^c*property destroy [num] [owner]^x\n");
            return(0);
        }
        cmnd->str[2][0] = up(cmnd->str[2][0]);

        player->print("Looking to destroy property #%d owned by %s.\n", num, cmnd->str[2]);
        std::list<Property*>::iterator it;
        for(it = gConfig->properties.begin() ; it != gConfig->properties.end() ; it++) {
            if(num == i++ && (*it)->isOwner(cmnd->str[2])) {
                player->printColor("^rProperty found...^x\n\n%s\n^rProperty destroyed!\n", (*it)->show().c_str());
                gConfig->destroyProperty(*it);
                return(0);
            }
        }
        player->print("Property not found!\n");
    } else if(!isdigit(id.getAt(0))) {
        PropType propType = PROP_NONE, i;
        int len = strlen(cmnd->str[1]);

        for(i = PROP_NONE; i < PROP_END && propType == PROP_NONE; i = (PropType)((int)i+1)) {
            if( !strncmp(cmnd->str[1], Property::getTypeStr(i).c_str(), len) ||
                (i == PROP_HOUSE && !strncmp(cmnd->str[1], "house", len))
            )
                propType = i;
        }

        player->printColor("Properties filtered on type: %s.\n", Property::getTypeStr(propType).c_str());
        player->printColor("^b--------------------------------------------------------------\n");
        gConfig->showProperties(player, 0, propType);
    } else {
        int num = atoi(id.c_str());
        int i=1;

        player->print("Property #%d\n", num);
        player->printColor("^b--------------------------------------------------------------\n");
        std::list<Property*>::iterator it;
        for(it = gConfig->properties.begin() ; it != gConfig->properties.end() ; it++) {
            if(num == i++) {
                player->printColor("%s\n", (*it)->show().c_str());
                return(0);
            }
        }
        player->print("Property not found!\n");
    }
    return(0);
}


//*********************************************************************
//                      show
//*********************************************************************

bstring Property::show(bool isOwner, bstring player, int *i) {
    std::ostringstream oStr;
    oStr.setf(std::ios::left, std::ios::adjustfield);

    oStr << "Name:     ^e" << name;
    if(player != "" && !!getPartialOwner(player))
        oStr << " (#" << (*i)++ << ")";

    oStr << "^x\n"
         << "Location: ^c" << location;
    if(area != "")
        oStr << " in " << gConfig->catRefName(area);

    oStr << "^x\n"
         << "Founded:  ^c" << std::setw(30) << dateFounded << "^x";

    if(!isOwner)
        oStr << " Owner: ^c" << std::setw(15) << owner << "^x";
    oStr << "Type: ^c" << getTypeStr() << "^x\n";

    // show them who else is part of this property
    if(guild) {
        const Guild *g = gConfig->getGuild(guild);

        if(!g) {
            if(type == PROP_GUILDHALL)
                oStr << "This guildhall is broken: guild ID " << guild << " does not exist.\n";
        } else {
            oStr << "Guild:    ^c#" << guild << " ^c" << g->getName() << "^x\n";
            std::list<bstring>::const_iterator pt;
            bool initial=false;

            oStr << "Guild Members: ^c";
            for(pt = g->members.begin() ; pt != g->members.end() ; pt++) {
                if(initial)
                    oStr << ", ";

                initial = true;
                if(!isOwner && player == (*pt))
                    oStr << "^e";
                oStr << (*pt);
                if(!isOwner && player == (*pt))
                    oStr << "^c";
            }
            oStr << "^x\n";
        }
    }

    if(!partialOwners.empty()) {
        std::list<PartialOwner>::const_iterator pt;
        bool initial=false;

        oStr << "Partial Owners: ^c";
        for(pt = partialOwners.begin() ; pt != partialOwners.end() ; pt++) {
            if(initial)
                oStr << ", ";

            initial = true;
            if(!isOwner && player == (*pt).getName())
                oStr << "^m";
            oStr << (*pt).getName();
            if(!isOwner && player == (*pt).getName())
                oStr << "^c";
        }
        oStr << "^x\n";
    }

    // admin info
    if(player == "") {
        oStr << "Ranges:\n";
        std::list<Range>::const_iterator rt;
        for(rt = ranges.begin() ; rt != ranges.end() ; rt++)
            oStr << "Low: ^c" << (*rt).low.str() << "^x  High: ^c" << (*rt).high << "^x\n";

        oStr << "Log Type: ^c" << getLogTypeStr() << "^x\n";
        if(!log.empty()) {
            oStr << "Log:\n^b-----------------^x\n";
            oStr << getLog();
        }
    }

    return(oStr.str());
}

//*********************************************************************
//                      showProperties
//*********************************************************************
// player=0 means we're using the admin version

void Config::showProperties(Player* viewer, Player* player, PropType propType) {
    std::list<Property*>::iterator it;
    std::ostringstream oStr;
    Property *p=0;
    bstring name = "";
    bool    isOwner=false, partialOwnerList=false;
    int     i=0;

    oStr.setf(std::ios::left, std::ios::adjustfield);
    if(player)
        name = player->getName();

    for(it = properties.begin() ; it != properties.end() ; it++) {
        p = (*it);


        if(player) {
            isOwner = p->isOwner(name);
            // guild halls don't count toward the partial owner list, since they can't
            // remove themselves with the property interface
            if(!partialOwnerList && player && p->getType() != PROP_GUILDHALL)
                partialOwnerList = !!p->getPartialOwner(name);
            if(!isOwner && !p->isPartialOwner(name))
                continue;
        }

        // when filtering, we still increment so we can show the correct id
        i++;

        if(propType != PROP_NONE && p->getType() != propType)
            continue;

        if(!player) {
            oStr << "^c#" << std::setw(3) << i << "^x Name: ^y" << std::setw(50) << p->getName()
                 << "^x Range: ^c" << p->ranges.front().str() << "^x\n";
            continue;
        }

        oStr << p->show(isOwner, name, &i) << "\n";
    }

    if(partialOwnerList) {
        oStr << "To remove yourself as partial owner from a property,\n"
             << "type \"property remove [num]\".\n\n";
    }

    viewer->printColor("%s", oStr.str().c_str());
}

//*********************************************************************
//                      clearProperties
//*********************************************************************

void Config::clearProperties() {
    Property* p;

    while(!properties.empty()) {
        p = properties.front();
        delete p;
        properties.pop_front();
    }
    properties.clear();
}

//*********************************************************************
//                      getProperty
//*********************************************************************

Property* Config::getProperty(CatRef cr) {
    std::list<Property*>::iterator it;

    for(it = properties.begin() ; it != properties.end() ; it++) {
        if((*it)->belongs(cr))
            return(*it);
    }

    return(0);
}

//*********************************************************************
//                      getAvailableProperty
//*********************************************************************

CatRef Config::getAvailableProperty(PropType type, int numRequired) {
    std::list<Property*>::iterator it;
    std::list<Range>::iterator rt;
    std::map<int, bool> sort;
    std::map<int, bool>::iterator st;
    Property *p;
    CatRef  cr;
    int     i=0;

    cr.setArea(Property::getTypeArea(type));

    if(cr.isArea(""))
        return(cr);

    for(it = properties.begin() ; it != properties.end() ; it++) {
        p = (*it);

        if(p->getType() != type)
            continue;

        for(rt = p->ranges.begin() ; rt != p->ranges.end() ; rt++) {
            if(!cr.isArea((*rt).low.area))
                continue;
            for(cr.id = (*rt).low.id; cr.id <= (*rt).high; cr.id++)
                sort[cr.id] = true;
        }
    }

    // if a entire-area range is found, we fail
    if(sort.find(-1) != sort.end()) {
        cr.id = 0;
        return(cr);
    }

    cr.id = 1;
    for(st = sort.begin() ; st != sort.end() ; st++) {
        i = (*st).first;

        // this room is used, keep looking
        if(cr.id == i) {
            cr.id++;
            continue;
        }

        // we've jumped over some empty rooms. we need to know if there's
        // enough space to fit this property
        if(i - cr.id >= numRequired)
            return(cr);

        // not enough room; keep looking
        cr.id = i + 1;
    }

    if(!validRoomId(cr))
        cr.id = 0;

    return(cr);
}

//*********************************************************************
//                      rename
//*********************************************************************

void storageName(UniqueRoom* room, const Player* player);
void setupShop(Property *p, Player* player, const Guild* guild, UniqueRoom* shop, UniqueRoom* storage);

void Property::rename(Player *player) {
    UniqueRoom  *shop=0, *storage=0;
    setOwner(player->getName());

    if(type == PROP_STORAGE) {
        if(loadRoom(ranges.front().low, &storage)) {
            storageName(storage, player);
            setName(storage->getName());
            storage->saveToFile(0);
        }
    } else if(type == PROP_SHOP) {
        CatRef cr = ranges.front().low;
        if(loadRoom(cr, &shop)) {
            cr.id++;
            if(loadRoom(cr, &storage)) {
                // only rename shops that aren't affiliated with a guild
                if(!guild)
                    setupShop(this, player, 0, shop, storage);
            }
        }
    }
}

//*********************************************************************
//                      renamePropertyOwner
//*********************************************************************

void Config::renamePropertyOwner(bstring oldName, Player *player) {
    std::list<Property*>::iterator it;

    for(it = properties.begin() ; it != properties.end() ; it++) {
        if((*it)->getOwner() == oldName)
            (*it)->rename(player);
    }

    saveProperties();
}


//*********************************************************************
//                      getLog
//*********************************************************************

bstring Property::getLog() const {
    if(log.empty())
        return("No entries in the log.\n");

    bstring str = "";
    std::list<bstring>::const_iterator it;

    for(it = log.begin() ; it != log.end() ; it++) {
        str += (*it);
        str += "\n";
    }

    return(str);
}

//*********************************************************************
//                      appendLog
//*********************************************************************

void Property::appendLog(bstring user, const char *fmt, ...) {
    if(logType == LOG_NONE)
        return;
    if(logType == LOG_PARTIAL && isOwner(user))
        return;

    char    *str;
    va_list ap;

    va_start(ap, fmt);
    if(vasprintf(&str, fmt, ap) == -1) {
        std::clog << "Error in Property::appendLog\n";
        return;
    }
    va_end(ap);

    long    t = time(0);
    bstring txt;
    txt = "^c";
    txt += ctime(&t);
    txt.trim();
    txt += ":^x ";
    txt += str;

    log.push_front(txt);
    while(log.size() > PROP_LOG_SIZE)
        log.pop_back();

    gConfig->saveProperties();
}

//*********************************************************************
//                      clearLog
//*********************************************************************

void Property::clearLog() {
    log.clear();
}


//*********************************************************************
//                      guildRoomSetup
//*********************************************************************
// this sets up the room to be part of the guild, then deletes it

void Property::guildRoomSetup(UniqueRoom *room, const Guild* guild, bool outside) {
    room->setName( guild->getName() + "'s Guild Hall");

    if(!outside) {
        room->setFlag(R_INDOORS);
        room->setFlag(R_SAFE_ROOM);
    }

    room->setShortDescription("You are in the ");
    room->appendShortDescription(guild->getName());
    room->appendShortDescription("'s guild hall.");

    room->saveToFile(0);
    delete room;
}

//*********************************************************************
//                      houseRoomSetup
//*********************************************************************
// this sets up the room as a personal house, then deletes it

void Property::houseRoomSetup(UniqueRoom *room, const Player* player, bool outside) {
    room->setName( player->getName() + "'s Personal House");

    if(!outside) {
        room->setFlag(R_INDOORS);
        room->setFlag(R_SAFE_ROOM);
    }

    room->setShortDescription("You are in ");
    room->appendShortDescription(player->getName());
    room->appendShortDescription("'s personal house.");

    room->saveToFile(0);
    delete room;
}

//*********************************************************************
//                      roomSetup
//*********************************************************************

void Property::roomSetup(UniqueRoom *room, PropType propType, const Player* player, const Guild* guild, bool outside) {
    if(propType == PROP_GUILDHALL)
        guildRoomSetup(room, guild, outside);
    else if(propType == PROP_HOUSE)
        houseRoomSetup(room, player, outside);
}

//*********************************************************************
//                      linkRoom
//*********************************************************************

void Property::linkRoom(BaseRoom* inside, BaseRoom* outside, bstring xname) {
    UniqueRoom* uRoom = outside->getAsUniqueRoom();
    AreaRoom* aRoom = outside->getAsAreaRoom();
    if(uRoom) {
        link_rom(inside, uRoom->info, xname.c_str());
        uRoom->saveToFile(0);
    } else {
        link_rom(inside, &aRoom->mapmarker, xname.c_str());
        aRoom->save();
    }

    uRoom = inside->getAsUniqueRoom();
    aRoom = inside->getAsAreaRoom();
    if(uRoom) {
        link_rom(outside, uRoom->info, "out");
        uRoom->saveToFile(0);
    } else {
        link_rom(outside, &aRoom->mapmarker, "out");
        aRoom->save();
    }
}

//*********************************************************************
//                      makeNextRoom
//*********************************************************************

UniqueRoom* Property::makeNextRoom(UniqueRoom* r1, PropType propType, CatRef cr, bool exits, const Player* player, const Guild* guild, BaseRoom* room, bstring xname, const char *go, const char *back, bool save) {
    UniqueRoom *r2 = new UniqueRoom;
    r2->info = cr;

    if(r1) {
        link_rom(r1, r2->info, go);
        link_rom(r2, r1->info, back);
    }

    if(exits)
        linkRoom(room, r2, xname);

    if(save) {
        roomSetup(r2, propType, player, guild);
        return(0);
    }
    return(r2);
}

//*********************************************************************
//                      rotateHouse
//*********************************************************************

int Property::rotateHouse(char *dir1, char *dir2, int rotation) {

    if(!rotation)
        strcpy(dir1, "west");
    else if(rotation == 1)
        strcpy(dir1, "north");
    else if(rotation == 2)
        strcpy(dir1, "east");
    else if(rotation == 3)
        strcpy(dir1, "south");

    strcpy(dir2, opposite_exit_name(dir1).c_str());
    return((rotation + 1) % 4);
}

//*********************************************************************
//                      isInside
//*********************************************************************

bool Property::isInside(const Player* player, const UniqueRoom* room, Property** p) {
    if(!(*p) || !room)
        return(false);

    if((*p)->getType() == PROP_GUILDHALL) {
        if( !(*p)->getGuild() ||
            !player->getGuild() ||
            player->getGuild() != (*p)->getGuild()
        )
            return(false);
    }
    return(true);
}

//*********************************************************************
//                      requireInside
//*********************************************************************

bool Property::requireInside(const Player* player, const UniqueRoom* room, Property** p, PropType propType) {
    if(room && !(*p))
        (*p) = gConfig->getProperty(room->info);

    if(!isInside(player, room, p)) {
        player->print("You must be inside your %s to perform this action.\n", getTypeStr(propType).c_str());
        return(false);
    }
    return(true);
}

//*********************************************************************
//                      descEdit
//*********************************************************************

bstring postText(bstring str);

void Property::descEdit(Socket* sock, bstring str) {
    bstring outstr = "";
    char    outcstr[160];
    int     ff=0;

    Player* ply = sock->getPlayer();
    if((str[0] == '.' || str[0] == '*') && !str[1]) {
        ply->clearFlag(P_READING_FILE);
        sock->restoreState();

        Property *p=0;
        if(!Property::requireInside(ply, ply->getUniqueRoomParent(), &p))
            return;

        FILE* fp = fopen(sock->tempstr[0], "r");
        if(!fp) {
            ply->print("Error replacing room description.\n");
            return;
        }

        ply->getUniqueRoomParent()->setLongDescription("");
        while(!feof(fp)) {
            fgets(outcstr, sizeof(outcstr), fp);
            ply->getUniqueRoomParent()->appendLongDescription(outcstr);
            strcpy(outcstr, "");
        }

        ply->getUniqueRoomParent()->setLongDescription(ply->getUniqueRoomParent()->getLongDescription().left(ply->getUniqueRoomParent()->getLongDescription().getLength()-1));
        fclose(fp);
        unlink(sock->tempstr[0]);

        ply->print("Room description replaced.\n");
        ply->getUniqueRoomParent()->escapeText();
        ply->getUniqueRoomParent()->saveToFile(0);
        return;
    }


    if(str[0] == '\\' && !str[1]) {
        unlink(sock->tempstr[0]);
        ply->clearFlag(P_READING_FILE);
        ply->print("Description editting cancelled.\n");
        sock->restoreState();
        return;
    }

    ff = open(sock->tempstr[0], O_CREAT | O_APPEND | O_RDWR, ACC);
    if(ff < 0)
        merror("Property::descEdit", FATAL);

    outstr = postText(str);
    write(ff, outstr.c_str(), outstr.getLength());
    close(ff);
    ply->print("-: ");

    gServer->processOutput();
    sock->intrpt &= ~1;
}

//*********************************************************************
//                      cmdHouse
//*********************************************************************

int cmdHouse(Player* player, cmd* cmnd) {
    Property::manage(player, cmnd, PROP_HOUSE, 1);
    return(0);
}

//*********************************************************************
//                      houseCanBuild
//*********************************************************************

bool Property::houseCanBuild(AreaRoom* aRoom, BaseRoom* room) {
    if(room->flagIsSet(R_BUILD_HOUSE))
        return(true);

    return(false);
}

//*********************************************************************
//                      buildFlag
//*********************************************************************

int Property::buildFlag(int propType) {
    if(propType == PROP_GUILDHALL)
        return(R_BUILD_GUILDHALL);
    return(0);
}

//*********************************************************************
//                      manageDesc
//*********************************************************************

void Property::manageDesc(Player* player, cmd* cmnd, PropType propType, int x) {
    char    file[80];
    int     ff=0;

    switch(propType) {
    case PROP_GUILDHALL:
        sprintf(file, "%s/%s_guildhall.txt", Path::Post, player->getCName());
        break;
    case PROP_HOUSE:
        sprintf(file, "%s/%s_house.txt", Path::Post, player->getCName());
        break;
    default:
        player->print("Error: this property type is not supported.\n");
        return;
    }

    ff = open(file, O_RDONLY, 0);
    close(ff);

    if(file_exists(file)) {
        player->print("Room description so far:\n\n");
        viewLoginFile(player->getSock(), file);
        player->print("\n\n");
    }

    player->print("You may edit this room's long description now.\nType '.' or '*' on a line by itself to finish or '\\' to cancel.\nEach line should be NO LONGER THAN 80 CHARACTERS.\n-: ");

    strcpy(player->getSock()->tempstr[0], file);

    player->setFlag(P_READING_FILE);
    gServer->processOutput();
    player->getSock()->setState(CON_EDIT_PROPERTY);
    player->getSock()->intrpt &= ~1;
}

//*********************************************************************
//                      manageShort
//*********************************************************************

void Property::manageShort(Player* player, cmd* cmnd, PropType propType, int x) {
    bstring desc = getFullstrText(cmnd->fullstr, 3-x);

    if(!Property::goodNameDesc(player, desc, "Set room short description to what?", "description string"))
        return;

    player->getUniqueRoomParent()->setShortDescription(desc);
    player->print("Property short description set to '%s'.\n", desc.c_str());
    player->getUniqueRoomParent()->saveToFile(0);
}

//*********************************************************************
//                      manageName
//*********************************************************************

void Property::manageName(Player* player, cmd* cmnd, PropType propType, int x) {
    bstring name = getFullstrText(cmnd->fullstr, 3-x);

    if(!Property::goodNameDesc(player, name, "Rename this room to what?", "room name"))
        return;


    player->getUniqueRoomParent()->setName(name);
    player->print("Room renamed to '%s'.\n", player->getUniqueRoomParent()->getCName());
    player->getUniqueRoomParent()->saveToFile(0);
}

//*********************************************************************
//                      manageFound
//*********************************************************************

void Property::manageFound(Player* player, cmd* cmnd, PropType propType, const Guild* guild, int x) {
    Object  *deed=0, *oHidden=0, *oConceal=0, *oInvis=0, *oFoyer=0;
    AreaRoom* aRoom = player->getAreaRoomParent();
    UniqueRoom* uRoom = player->getUniqueRoomParent();
    BaseRoom* room = player->getRoomParent();
    int canBuildFlag = Property::buildFlag(propType);
    CatRef cr;

    bstring xNameType = getTypeStr(propType);
    // caps!
    xNameType.setAt(0, toupper(xNameType.getAt(0)));

    if(!canBuildFlag || !room->flagIsSet(canBuildFlag)) {
        player->print("You can't build a %s here!\n", getTypeStr(propType).c_str());
        return;
    }


    const CatRefInfo* cri = gConfig->getCatRefInfo(room);

    if(!cri) {
        player->print("This area cannot support a %s.\n", getTypeStr(propType).c_str());
        return;
    }

    
    std::list<Property*>::const_iterator it;

    for(it = gConfig->properties.begin() ; it !=  gConfig->properties.end() ; it++) {
        if((*it)->getType() != propType)
            continue;
        if((*it)->getArea() != cri->getArea())
            continue;
        if(propType == PROP_STORAGE && !(*it)->isOwner(player->getName()) && !(*it)->isPartialOwner(player->getName()))
            continue;
        if(propType == PROP_GUILDHALL && player->getGuild() != (*it)->getGuild())
            continue;

        if(propType == PROP_GUILDHALL)
            player->print("Your guild already owns a guild hall in %s!\n", gConfig->catRefName(cri->getArea()).c_str());
        else if(propType == PROP_HOUSE)
            player->print("You already own a house in %s!\n", gConfig->catRefName(cri->getArea()).c_str());

        return;
    }

    
    bstring oName;
    for(Object* obj : player->objects) {
        // find their property related objects
        oName = obj->getName();
        if( (propType == PROP_GUILDHALL && oName.left(10) == "guildhall ") ||
            (propType == PROP_HOUSE && oName.left(6) == "house ") )
        {

            if( !deed && (
                    (   propType == PROP_GUILDHALL &&
                        oName.left(14) == "guildhall deed" &&
                        (   (uRoom && obj->deed.belongs(uRoom->info)) ||
                            (aRoom && obj->deed.isArea("area"))
                        )
                    ) || (
                        propType == PROP_HOUSE &&
                        oName.left(10) == "house deed"
                    )
                ) )
            {
                deed = obj;

            // see if they want the entrance hidden
            } else if(oName.right(15) == "hidden entrance") {
                oHidden = obj;

            // see if they want the entrance concealed
            } else if(oName.right(18) == "concealed entrance") {
                oConceal = obj;

            // see if they want the entrance invisible
            } else if(oName.right(18) == "invisible entrance") {
                oInvis = obj;

            // see if they want a foyer
            } else if(oName.right(5) == "foyer") {
                oFoyer = obj;

            }

        }
    }

    if(!deed) {
        player->print("You need a deed to build a %s!\nVisit a real estate office.\n", getTypeStr(propType).c_str());
        return;
    }



    bstring xname = getFullstrText(cmnd->fullstr, 3-x);
    if(xname == "") {
        player->print("Please enter an exit name followed by the number of the room you wish to connect to.\n");
        return;
    }
    int roomId=0;
    for(int i = xname.getLength()-1; i>0; i--) {
        if(xname.getAt(i) == ' ') {
            roomId = atoi(xname.right(xname.getLength()-i).c_str());
            xname = xname.left(i);
            break;
        }
    }

    xname.Replace("_", " ");

    if(!Property::goodExit(player, room, xNameType.c_str(), xname))
        return;

    // determine what layout we're using!
    bstring layout = deed->getName();
    bstring orientation = "";
    int extra=0;
    int req=0;

    if(layout.right(12) == "5 room cross") {
        layout = layout.right(12);
        req = 5;
    } else if(layout.right(13) == "4 room square") {
        layout = layout.right(13);
        req = 4;
    } else if(layout.right(5) == "tower") {
        // 5 room tower
        layout = layout.right(12);
        req = atoi(layout.left(1).c_str());
        layout = layout.right(5);
    } else if(layout.right(11) == "small house") {
        layout = layout.right(11);
        roomId = req = 1;
    } else if(layout.right(12) == "medium house") {
        orientation = getFullstrText(layout, 3).left(1);
        if(orientation != "n" && orientation != "e")
            orientation = "n";
        layout = layout.right(12);
        req = 2;
    } else if(layout.right(11) == "large house") {
        orientation = getFullstrText(layout, 3).left(1);
        if(orientation == "n")
            extra = 0;
        else if(orientation == "e")
            extra = 1;
        else if(orientation == "s")
            extra = 2;
        else {
            orientation = "w";
            extra = 3;
        }
        layout = layout.right(11);
        req = 4;
    } else {
        player->print("The layout specified by your %s deed '%s' could not be determined!\n", getTypeStr(propType).c_str(), deed->getCName());
        return;
    }

    if(!roomId) {
        player->print("That is not a valid room number to connect to.\n");
        return;
    }

    cr = gConfig->getAvailableProperty(propType, req);
    if(cr.id < 1) {
        player->print("Sorry, you can't build a new %s right now; try again later!\n", getTypeStr(propType).c_str());
        return;
    }

    if( roomId < 1 ||
        roomId > req ||
        (layout == "5 room cross" && roomId == 1)
    ) {
        player->print("That is not a valid room number to connect to.\n");
        return;
    }


    Property *p = new Property;
    p->found(player, propType);

    // foyer needs 1 extra room
    p->addRange(cr.area, cr.id, cr.id + req - (oFoyer ? 0 : 1));

    if(propType == PROP_GUILDHALL) {
        p->setGuild(player->getGuild());
        p->setName(guild->getName() + "'s Guild Hall");
    } else if(propType == PROP_HOUSE) {
        bstring pName = player->getName();
        pName += "'s House";
        p->setName(pName);
    }

    // if they have a foyer, make one
    UniqueRoom* rFoyer=0;
    bstring xPropName = xname;
    if(oFoyer) {
        rFoyer = makeNextRoom(0, propType, cr, true, player, guild, room, xname, "", "", false);
        if(propType == PROP_GUILDHALL) {
            rFoyer->setFlag(R_GUILD_OPEN_ACCESS);
            xname = "guild";
        } else if(propType == PROP_HOUSE)
            xname = "house";
        room = rFoyer;
        cr.id++;
    }


    // this is where the work of creating the rooms is done.
    if(layout == "5 room cross") {

        UniqueRoom* r1 = makeNextRoom(0, propType, cr, false, player, guild, room, xname, "", "", false);

        cr.id++; // north
        makeNextRoom(r1, propType, cr, roomId == 2, player, guild, room, xname, "north", "south", true);
        cr.id++; // east
        makeNextRoom(r1, propType, cr, roomId == 3, player, guild, room, xname, "east", "west", true);
        cr.id++; // south
        makeNextRoom(r1, propType, cr, roomId == 4, player, guild, room, xname, "south", "north", true);
        cr.id++; // west
        makeNextRoom(r1, propType, cr, roomId == 5, player, guild, room, xname, "west", "east", true);

        roomSetup(r1, propType, player, guild);

    } else if(layout == "4 room square") {

        UniqueRoom* nw = makeNextRoom(0, propType, cr, roomId == 1, player, guild, room, xname, "", "", false);
        cr.id++; // northeast
        UniqueRoom* ne = makeNextRoom(nw, propType, cr, roomId == 2, player, guild, room, xname, "east", "west", false);
        cr.id++; // southwest
        UniqueRoom* sw = makeNextRoom(nw, propType, cr, roomId == 3, player, guild, room, xname, "south", "north", false);
        cr.id++; // southeast
        UniqueRoom* se = makeNextRoom(ne, propType, cr, roomId == 4, player, guild, room, xname, "south", "north", false);

        link_rom(se, sw->info, "west");
        link_rom(sw, se->info, "east");

        nw->arrangeExits();
        roomSetup(nw, propType, player, guild);
        ne->arrangeExits();
        roomSetup(ne, propType, player, guild);
        se->arrangeExits();
        roomSetup(se, propType, player, guild);
        sw->arrangeExits();
        roomSetup(sw, propType, player, guild);

    } else if(layout == "tower") {

        UniqueRoom* r1 = makeNextRoom(0, propType, cr, roomId == 1, player, guild, room, xname, "", "", false);
        int i=1;

        while(i < req) {
            i++;
            cr.id++;

            UniqueRoom* r2 = makeNextRoom(r1, propType, cr, roomId == i, player, guild, room, xname, "up", "down", false);

            r1->arrangeExits();
            roomSetup(r1, propType, player, guild);
            r1 = r2;
        }

        r1->arrangeExits();
        roomSetup(r1, propType, player, guild);

    } else if(layout == "small house") {

        makeNextRoom(0, propType, cr, true, player, guild, room, xname, "", "", true);

    } else if(layout == "medium house") {

        UniqueRoom* r1 = makeNextRoom(0, propType, cr, roomId == 1, player, guild, room, xname, "", "", false);

        cr.id++;
        if(orientation == "n")
            makeNextRoom(r1, propType, cr, roomId == 2, player, guild, room, xname, "south", "north", true);
        else
            makeNextRoom(r1, propType, cr, roomId == 2, player, guild, room, xname, "east", "west", true);
        roomSetup(r1, propType, player, guild);

    } else if(layout == "large house") {

        char dir1[10], dir2[10];
        UniqueRoom* r1 = makeNextRoom(0, propType, cr, roomId == 1, player, guild, room, xname, "", "", false);

        cr.id++;
        extra = rotateHouse(dir1, dir2, extra);
        makeNextRoom(r1, propType, cr, roomId == 2, player, guild, room, xname, dir1, dir2, true);

        cr.id++;
        extra = rotateHouse(dir1, dir2, extra);
        makeNextRoom(r1, propType, cr, roomId == 3, player, guild, room, xname, dir1, dir2, true);

        cr.id++;
        extra = rotateHouse(dir1, dir2, extra);
        makeNextRoom(r1, propType, cr, roomId == 4, player, guild, room, xname, dir1, dir2, true);

        r1->arrangeExits();
        roomSetup(r1, propType, player, guild);

    } else {

        if(rFoyer)
            delete rFoyer;

        player->print("There was an error in construction of your %s!\n", getTypeStr(propType).c_str());
        delete p;
        return;

    }

    if(rFoyer)
        roomSetup(rFoyer, propType, player, guild, true);
    gConfig->addProperty(p);

    if(propType == PROP_GUILDHALL) {
        player->print("Congratulations! Your guild is now the owner of a brand new guild hall.\n");
        if(!player->flagIsSet(P_DM_INVIS)) {
            broadcast(player->getSock(), player->getParent(), "%M just opened a guild hall!", player);
            broadcast("### %s, leader of %s, just opened a guild hall!", player->getCName(), guild->getName().c_str());
        }

        if(player->inUniqueRoom())
            sendMail(gConfig->getReviewer(), player->getName() + " (" + guild->getName() + ") opened a guild hall in " + gConfig->catRefName(player->getUniqueRoomParent()->info.area) + ".\n");
    } else if(propType == PROP_HOUSE) {
        player->print("Congratulations! You are now the owner of a brand new house.\n");
        if(!player->flagIsSet(P_DM_INVIS))
            broadcast(player->getSock(), player->getParent(), "%M just built a house!", player);
    }

    player->delObj(deed, true, false, true, false);
    delete deed;
    Exit* exit = findExit(player, xPropName, 1);
    if(oHidden) {
        if(exit) {
            player->delObj(oHidden, true, false, true, false);
            delete oHidden;
            exit->setFlag(X_SECRET);
            player->printColor("Exit '%s^x' is now hidden.\n", exit->getCName());
        } else {
            player->print("There was an error making the %s entrance hidden!\n", getTypeStr(propType).c_str());
        }
    }
    if(oConceal) {
        if(exit) {
            player->delObj(oConceal, true, false, true, false);
            delete oConceal;
            exit->setFlag(X_CONCEALED);
            player->printColor("Exit '%s^x' is now concealed.\n", exit->getCName());
        } else {
            player->print("There was an error making the %s entrance concealed!\n", getTypeStr(propType).c_str());
        }
    }
    if(oInvis) {
        if(exit) {
            player->delObj(oInvis, true, false, true, false);
            delete oInvis;
            exit->addEffect("invisibility", -1);
            player->printColor("Exit '%s^x' is now invisible.\n", exit->getCName());
        } else {
            player->print("There was an error making the %s entrance invisible!\n", getTypeStr(propType).c_str());
        }
    }
    if(oFoyer) {
        player->delObj(oFoyer, true, false, true, false);
        delete oFoyer;
    }
    player->checkDarkness();

    if(propType == PROP_GUILDHALL) {
        player->getUniqueRoomParent()->clearFlag(R_BUILD_GUILDHALL);
        player->getUniqueRoomParent()->setFlag(R_WAS_BUILD_GUILDHALL);
    } else if(propType == PROP_HOUSE) {
        player->getUniqueRoomParent()->clearFlag(R_BUILD_HOUSE);
        player->getUniqueRoomParent()->setFlag(R_WAS_BUILD_HOUSE);
    }
    player->getUniqueRoomParent()->saveToFile(0);
}

//*********************************************************************
//                      manageExtend
//*********************************************************************

void Property::manageExtend(Player* player, cmd* cmnd, PropType propType, Property* p, const Guild* guild, int x) {
    BaseRoom* room = player->getRoomParent();
    Object* obj = player->findObject(player, cmnd, 3-x);
    CatRef cr;

    bstring xNameType = getTypeStr(propType);
    // caps!
    xNameType.setAt(0, toupper(xNameType.getAt(0)));
    
    if(!obj) {
        player->print("Object not found.\nYou need to purchase %s extensions from a realty office.\n", getTypeStr(propType).c_str());
        return;
    }


    bstring layout = obj->getName();
    if( (propType == PROP_GUILDHALL && layout.left(19) != "guildhall extension") ||
        (propType == PROP_HOUSE && layout.left(15) != "house extension")
    ) {
        player->printColor("%O is not a %s extension permit.\nYou need to purchase %s extensions from a realty office.\n",
            obj, getTypeStr(propType).c_str(), getTypeStr(propType).c_str());
        return;
    }
    layout = layout.right(layout.getLength() - 21);



    bstring xname = getFullstrText(cmnd->fullstr, 4-x);
    if(xname != "" && isdigit(xname.getAt(0)))
        xname = getFullstrText(xname, 1);

    xname.Replace("_", " ");
    if(!Property::goodExit(player, room, xNameType.c_str(), xname))
        return;


    cr = gConfig->getAvailableProperty(propType, 1);
    if(cr.id < 1) {
        player->print("Sorry, you can't expand your %s right now, try again later!\n", getTypeStr(propType).c_str());
        return;
    }


    UniqueRoom* target = new UniqueRoom;
    bool outside=false;

    if(layout == "normal room") {

        // normal rooms are acceptable

    } else if(layout == "outdoors") {

        outside = true;

    } else if(layout == "clinic") {

        target->setFlag(R_FAST_HEAL);

    } else if(layout == "post office") {

        target->setFlag(R_POST_OFFICE);

    } else if(layout == "recycling room") {

        target->setFlag(R_DUMP_ROOM);

    } else if(layout == "magic atrium") {

        target->setFlag(R_FAST_HEAL);
        target->setFlag(R_MAGIC_BONUS);

    } else {
        player->print("The extension type for this permit could not be determined!\n");
        delete target;
        return;
    }

    target->info = cr;
    linkRoom(player->getUniqueRoomParent(), target, xname);
    roomSetup(target, propType, player, guild, outside);

    player->delObj(obj, true);
    delete obj;

    player->print("Extension has been added to your property!\n");
    p->addRange(cr.area, cr.id, cr.id);
    gConfig->saveProperties();
}

//*********************************************************************
//                      manageRename
//*********************************************************************

void Property::manageRename(Player* player, cmd* cmnd, PropType propType, int x) {
    BaseRoom* room = player->getRoomParent();
    bstring origExit = cmnd->str[3-x];
    bstring newExit = getFullstrText(cmnd->fullstr, 4-x);

    bstring xNameType = getTypeStr(propType);
    // caps!
    xNameType.setAt(0, toupper(xNameType.getAt(0)));

    if(origExit == "" || newExit == "") {
        player->print("Rename which exit to what?\n");
        return;
    }

    if(isCardinal(newExit)) {
        player->print("Exits cannot be made cardinal directions.\n");
        return;
    }
    if(!Property::goodExit(player, room, xNameType.c_str(), newExit))
        return;


    Exit* found = 0;
    bool unique=true;
    for(Exit* ext : room->exits ) {
        // exact match
        if(ext->getName() == origExit) {
            unique = true;
            found = ext;
            break;
        }
        if(!strncmp(ext->getCName(), origExit.c_str(), origExit.getLength())) {
            if(found)
                unique = false;
            found = ext;
        }
    }

    if(!unique) {
        player->print("That exit name is not unique.\n");
        return;
    }
    if(!found) {
        player->print("That exit was not found in this room.\n");
        return;
    }

    if(isCardinal(found->getName())) {
        player->print("Cardinal directions cannot be renamed.\n");
        return;
    }

    found->setName( newExit.c_str());
    player->getUniqueRoomParent()->arrangeExits();
    player->print("Exit renamed to '%s'.\n", newExit.c_str());
    player->getUniqueRoomParent()->saveToFile(0);
}

//*********************************************************************
//                      manage
//*********************************************************************
// This function handles the commands for editting a property. It
// currently supports:
//      PROP_GUILDHALL
//      PROP_HOUSE

void Property::manage(Player* player, cmd* cmnd, PropType propType, int x) {
    BaseRoom* room = player->getRoomParent();
    const Guild* guild=0;
    Property *p=0;
    int canBuildFlag = Property::buildFlag(propType);

    int len = strlen(cmnd->str[2-x]);
    const char *syntax;

    if(player->getClass() == CreatureClass::BUILDER) {
        player->print("Builders have no need to manage property.");
        return;
    }

    if(propType == PROP_GUILDHALL) {
        if(player->getGuild())
            guild = gConfig->getGuild(player->getGuild());

        if(!guild) {
            syntax = "Syntax:   guild hall ^e<^xsurvey^e> [^call^e]^x\n"
                "Shortcut: ^cgh         ^e<^xhelp^e>^x\n\n";
        } else {

            syntax = "Syntax:   guild hall ^e<^xsurvey^e> [^call^e]^x\n"
                "Shortcut: ^cgh         ^e<^xfound^e> <^cexit name^e> <^croom #^e>^x\n"
                "                     ^e<^xname^e> <^croom name^e>^x\n"
                "                     ^e<^xshort^e> <^cshort description^e>^x\n"
                "                     ^e<^xdesc^e>^x - will enter editor\n"
                "                     ^e<^xrename^e> <^cexit name^e> <^cnew name^e>^x\n"
                "                     ^e<^xextend^e> <^cobject^e> <^cexit name^e>^x\n"
                "                     ^e<^xhelp^e>^x\n\n";
        }
    } else if(propType == PROP_HOUSE) {
        // TODO: can-do rules will be complicated

        syntax = "Syntax: house ^e<^xsurvey^e> [^call^e]^x\n"
            "              ^e<^xfound^e> <^cexit name^e> <^croom #^e>^x\n"
            "              ^e<^xname^e> <^croom name^e>^x\n"
            "              ^e<^xshort^e> <^cshort description^e>^x\n"
            "              ^e<^xdesc^e>^x - will enter editor\n"
            "              ^e<^xrename^e> <^cexit name^e> <^cnew name^e>^x\n"
            "              ^e<^xextend^e> <^cobject^e> <^cexit name^e>^x\n"
            "              ^e<^xhelp^e>^x\n\n";
    }


    if(!len) {
        if(!guild)
            player->printColor("^yYou are not a member of a guild.\n");
        else
            guild->guildhallLocations(player, "Your guildhall is located at %s in %s.\n");
        player->printColor(syntax);
        return;
    }

    if(!strncmp(cmnd->str[2-x], "help", len)) {
        propHelp(player, cmnd, propType);
        return;
    }

    if(!strncmp(cmnd->str[2-x], "survey", len)) {
        if(canBuildFlag && !strcmp(cmnd->str[3-x], "all")) {
            player->print("Searching for suitable %s locations in this city.\n", getTypeStr(propType).c_str());
            findRoomsWithFlag(player, player->getUniqueRoomParent()->info, canBuildFlag);
        } else {
            if(!canBuildFlag || !room->flagIsSet(canBuildFlag))
                player->print("You are unable to build a %s here.\n", getTypeStr(propType).c_str());
            else
                player->print("This site is open for a %s to be constructed.\n", getTypeStr(propType).c_str());
        }
        return;
    }


    
    if(propType == PROP_GUILDHALL) {
        if(!guild) {
            player->printColor("^yYou are not a member of a guild.\n");
            player->printColor(syntax);
            return;
        }

        // inside correct guild?
    }


    // guild bankers can perform these commands
    if(propType != PROP_GUILDHALL || player->getGuildRank() >= GUILD_BANKER) {
        if(!strncmp(cmnd->str[2-x], "desc", len)) {
            if(!Property::requireInside(player, player->getConstUniqueRoomParent(), &p, propType))
                return;
            
            Property::manageDesc(player, cmnd, propType, x);
            return;
        }

        if(!strncmp(cmnd->str[2-x], "short", len)) {
            if(!Property::requireInside(player, player->getConstUniqueRoomParent(), &p, propType))
                return;

            Property::manageShort(player, cmnd, propType, x);
            return;
        }

        if(!strncmp(cmnd->str[2-x], "name", len)) {
            if(!Property::requireInside(player, player->getConstUniqueRoomParent(), &p, propType))
                return;

            Property::manageName(player, cmnd, propType, x);
            return;
        }
    }



    if(propType == PROP_GUILDHALL) {
        // everything else requires a guildmaster
        if(player->getGuildRank() != GUILD_MASTER) {
            player->print("You are not the leader of the guild.\n");
            return;
        }
    } else if(propType == PROP_HOUSE) {
        if(player->getLevel() < 15) {
            player->print("You must be atleast level 15 to build a personal house.\n");
            return;
        }
    }



    if(!strncmp(cmnd->str[2-x], "found", len)) {
        Property::manageFound(player, cmnd, propType, guild, x);
        return;
    }


    if(!Property::requireInside(player, player->getConstUniqueRoomParent(), &p, propType))
        return;

    if(!strncmp(cmnd->str[2-x], "extend", len)) {
        Property::manageExtend(player, cmnd, propType, p, guild, x);
        return;
    }
    if(!strncmp(cmnd->str[2-x], "rename", len)) {
        Property::manageRename(player, cmnd, propType, x);
        return;
    }


    if(!guild)
        player->printColor("^yYou are not a member of a guild.\n");
    else
        guild->guildhallLocations(player, "Your guildhall is located at %s in %s.\n");
    
    player->printColor(syntax);
}

//*********************************************************************
//                      found
//*********************************************************************

void Property::found(const Player* player, PropType propType, bstring location, bool shouldSetArea) {
    setOwner(player->getName());
    setDateFounded();
    if(location == "")
        location = player->getConstUniqueRoomParent()->getName();
    setLocation(location);
    if(shouldSetArea && player->inUniqueRoom()) {
        bstring pArea = player->getConstUniqueRoomParent()->info.area;
        if(player->getConstUniqueRoomParent()->info.isArea("guild")) {
            // load the guild entrance, look for the out exit, load that room, get the area
            Property* p = gConfig->getProperty(player->getConstUniqueRoomParent()->info);

            CatRef cr = p->ranges.front().low;
            UniqueRoom* room=0;
            if(loadRoom(cr, &room)) {
                // Look for the first exit not linking to the shop
                for(Exit* ext : room->exits) {
                    if(!ext->target.room.isArea("guild")) {
                        pArea = ext->target.room.area;
                        break;
                    }
                }
            }
        }
        setArea(pArea);
    }
    setType(propType);
}
