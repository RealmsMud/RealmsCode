/*
 * catRef.cpp
 *   CatRef object
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

#include <boost/algorithm/string/case_conv.hpp>     // for to_lower_copy
#include <boost/iterator/iterator_facade.hpp>       // for operator!=
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <ostream>                                  // for basic_ostream::op...

#include <string>                                   // for string, allocator
#include <string_view>                              // for operator==, strin...
#include "area.hpp"                                 // for Area
#include "catRef.hpp"                               // for CatRef
#include "catRefInfo.hpp"                           // for CatRefInfo
#include "config.hpp"                               // for gConfig, Config
#include "mudObjects/areaRooms.hpp"                 // for AreaRoom
#include "mudObjects/creatures.hpp"                 // for Creature
#include "mudObjects/uniqueRooms.hpp"               // for UniqueRoom
#include "xml.hpp"                                  // for copyPropToString


std::ostream& operator<<(std::ostream& out, const CatRef& cr) {
    out << cr.str();
    return(out);
}

//*********************************************************************
//                      CatRef
//*********************************************************************

CatRef::CatRef() {
    clear();
    if(gConfig)
        setArea(gConfig->getDefaultArea());
    else
        setArea("misc");
}

//*********************************************************************
//                      setDefault
//*********************************************************************

void CatRef::setDefault(const Creature* target) {
    clear();
    setArea(gConfig->getDefaultArea());

    if(target) {
        if(target->inUniqueRoom()) {
            setArea(target->getConstUniqueRoomParent()->info.area);
        } else if(target->inAreaRoom()) {
            const CatRefInfo* cri = gConfig->getCatRefInfo("area", target->getConstAreaRoomParent()->area->id, true);
            if(cri)
                setArea(cri->getArea());
        }
    }
}

//*********************************************************************
//                      clear
//*********************************************************************

void CatRef::clear() {
    area = "";
    id = 0;
}

//*********************************************************************
//                      operators
//*********************************************************************

CatRef& CatRef::operator=(const CatRef& cr) {
    if(this == &cr)
        return *this;

    area = cr.area;
    id = cr.id;
    return(*this);
}

bool CatRef::operator==(const CatRef& cr) const {
    return(area == cr.area && id == cr.id);
}
bool CatRef::operator!=(const CatRef& cr) const {
    return(!(*this == cr));
}

//*********************************************************************
//                      str
//*********************************************************************

// version of string for reloading
std::string CatRef::str() const {
    std::ostringstream oStr;
    oStr << area << "." << id;
    return(oStr.str());
}

//*********************************************************************
//                      str
//*********************************************************************

std::string CatRef::displayStr(std::string_view current, char color) const {
    std::ostringstream oStr;
    // if we're in an area already, we can chop off some text because they
    // already know what the area is
    if (id && !area.empty() && (current.empty() || area != current)) {
        if (color)
            oStr << "^" << color << area << ":^x" << id;
        else
            oStr << id << " - " << area;
    } else
        oStr << id;
    return (oStr.str());
}

//*********************************************************************
//                      setArea
//*********************************************************************

void CatRef::setArea(std::string c) {
    if(c.length() > 20)
        c = c.substr(0, 20);
    area = boost::algorithm::to_lower_copy(c);
}

//*********************************************************************
//                      isArea
//*********************************************************************

bool CatRef::isArea(std::string_view c) const {
    return(area == c);
}

//*********************************************************************
//                      load
//*********************************************************************

void CatRef::load(xmlNodePtr curNode) {
    xml::copyPropToString(area, curNode, "Area");
    xml::copyToNum(id, curNode);
}

//*********************************************************************
//                      save
//*********************************************************************

xmlNodePtr CatRef::save(xmlNodePtr curNode, const char* childName, bool saveNonZero, int pos) const {
    if(!saveNonZero && !id)
        return(nullptr);
    curNode = xml::newNumChild(curNode, childName, id);
    xml::newProp(curNode, "Area", area);
    if(pos)
        xml::newNumProp(curNode, "Num", pos);
    return(curNode);
}

CatRef::CatRef(std::string& pArea, short pId) {
    area = pArea;
    id = pId;
}
