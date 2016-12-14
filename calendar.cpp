/*
 * calendar.cpp
 *   Calendar, weather, and time functions.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "calendar.h"
//#include "structs.h"
//#include "mextern.h"
//#include "xml.h"
//#include <sstream>

//*********************************************************************
//                      cWeather
//*********************************************************************

cWeather::cWeather() {
    sunrise = sunset = earthTrembles = heavyFog =
    beautifulDay = brightSun = glaringSun = heat =
    still = lightBreeze = strongWind = windGusts =
    galeForce = clearSkies = lightClouds = thunderheads =
    lightRain = heavyRain = sheetsRain = torrentRain = "";
}


void cWeather::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Sunrise")) xml::copyToBString(sunrise, childNode);
        else if(NODE_NAME(childNode, "Sunset")) xml::copyToBString(sunset, childNode);
        else if(NODE_NAME(childNode, "EarthTrembles")) xml::copyToBString(earthTrembles, childNode);
        else if(NODE_NAME(childNode, "HeavyFog")) xml::copyToBString(heavyFog, childNode);
        else if(NODE_NAME(childNode, "BeautifulDay")) xml::copyToBString(beautifulDay, childNode);
        else if(NODE_NAME(childNode, "BrightSun")) xml::copyToBString(brightSun, childNode);
        else if(NODE_NAME(childNode, "GlaringSun")) xml::copyToBString(glaringSun, childNode);
        else if(NODE_NAME(childNode, "Heat")) xml::copyToBString(heat, childNode);
        else if(NODE_NAME(childNode, "Still")) xml::copyToBString(still, childNode);
        else if(NODE_NAME(childNode, "LightBreeze")) xml::copyToBString(lightBreeze, childNode);
        else if(NODE_NAME(childNode, "StrongWind")) xml::copyToBString(strongWind, childNode);
        else if(NODE_NAME(childNode, "WindGusts")) xml::copyToBString(windGusts, childNode);
        else if(NODE_NAME(childNode, "GaleForce")) xml::copyToBString(galeForce, childNode);
        else if(NODE_NAME(childNode, "ClearSkies")) xml::copyToBString(clearSkies, childNode);
        else if(NODE_NAME(childNode, "LightClouds")) xml::copyToBString(lightClouds, childNode);
        else if(NODE_NAME(childNode, "Thunderheads")) xml::copyToBString(thunderheads, childNode);
        else if(NODE_NAME(childNode, "LightRain")) xml::copyToBString(lightRain, childNode);
        else if(NODE_NAME(childNode, "HeavyRain")) xml::copyToBString(heavyRain, childNode);
        else if(NODE_NAME(childNode, "SheetsRain")) xml::copyToBString(sheetsRain, childNode);
        else if(NODE_NAME(childNode, "TorrentRain")) xml::copyToBString(torrentRain, childNode);
        else if(NODE_NAME(childNode, "NoMoon")) xml::copyToBString(noMoon, childNode);
        else if(NODE_NAME(childNode, "SliverMoon")) xml::copyToBString(sliverMoon, childNode);
        else if(NODE_NAME(childNode, "HalfMoon")) xml::copyToBString(halfMoon, childNode);
        else if(NODE_NAME(childNode, "WaxingMoon")) xml::copyToBString(waxingMoon, childNode);
        else if(NODE_NAME(childNode, "FullMoon")) xml::copyToBString(fullMoon, childNode);

        childNode = childNode->next;
    }
}


void cWeather::save(xmlNodePtr curNode) const {
    xml::newStringChild(curNode, "Sunrise", sunrise);
    xml::newStringChild(curNode, "Sunset", sunset);
    xml::newStringChild(curNode, "EarthTrembles", earthTrembles);
    xml::newStringChild(curNode, "HeavyFog", heavyFog);
    xml::newStringChild(curNode, "BeautifulDay", beautifulDay);
    xml::newStringChild(curNode, "BrightSun", brightSun);
    xml::newStringChild(curNode, "GlaringSun", glaringSun);
    xml::newStringChild(curNode, "Heat", heat);
    xml::newStringChild(curNode, "Still", still);
    xml::newStringChild(curNode, "LightBreeze", lightBreeze);
    xml::newStringChild(curNode, "StrongWind", strongWind);
    xml::newStringChild(curNode, "WindGusts", windGusts);
    xml::newStringChild(curNode, "GaleForce", galeForce);
    xml::newStringChild(curNode, "ClearSkies", clearSkies);
    xml::newStringChild(curNode, "LightClouds", lightClouds);
    xml::newStringChild(curNode, "Thunderheads", thunderheads);
    xml::newStringChild(curNode, "LightRain", lightRain);
    xml::newStringChild(curNode, "HeavyRain", heavyRain);
    xml::newStringChild(curNode, "SheetsRain", sheetsRain);
    xml::newStringChild(curNode, "TorrentRain", torrentRain);
    xml::newStringChild(curNode, "NoMoon", noMoon);
    xml::newStringChild(curNode, "SliverMoon", sliverMoon);
    xml::newStringChild(curNode, "HalfMoon", halfMoon);
    xml::newStringChild(curNode, "WaxingMoon", waxingMoon);
    xml::newStringChild(curNode, "FullMoon", fullMoon);
}

bstring cWeather::get(WeatherString w) const {
    switch(w) {
    case WEATHER_SUNRISE:
        return(sunrise);
    case WEATHER_SUNSET:
        return(sunset);

    case WEATHER_EARTH_TREMBLES:
        return(earthTrembles);
    case WEATHER_HEAVY_FOG:
        return(heavyFog);

    case WEATHER_BEAUTIFUL_DAY:
        return(beautifulDay);
    case WEATHER_BRIGHT_SUN:
        return(brightSun);
    case WEATHER_GLARING_SUN:
        return(glaringSun);
    case WEATHER_HEAT:
        return(heat);

    case WEATHER_STILL:
        return(still);
    case WEATHER_LIGHT_BREEZE:
        return(lightBreeze);
    case WEATHER_STRONG_WIND:
        return(strongWind);
    case WEATHER_WIND_GUSTS:
        return(windGusts);
    case WEATHER_GALE_FORCE:
        return(galeForce);

    case WEATHER_CLEAR_SKIES:
        return(clearSkies);
    case WEATHER_LIGHT_CLOUDS:
        return(lightClouds);
    case WEATHER_THUNDERHEADS:
        return(thunderheads);

    case WEATHER_LIGHT_RAIN:
        return(lightRain);
    case WEATHER_HEAVY_RAIN:
        return(heavyRain);
    case WEATHER_SHEETS_RAIN:
        return(sheetsRain);
    case WEATHER_TORRENT_RAIN:
        return(torrentRain);

    case WEATHER_NO_MOON:
        return(noMoon);
    case WEATHER_SLIVER_MOON:
        return(sliverMoon);
    case WEATHER_HALF_MOON:
        return(halfMoon);
    case WEATHER_WAXING_MOON:
        return(waxingMoon);
    case WEATHER_FULL_MOON:
        return(fullMoon);
    default:
        break;
    }
    return("");
}

//*********************************************************************
//                      cDay
//*********************************************************************

cDay::cDay() {
    month = day = year = 0;
}

short cDay::getMonth() const { return(month); }
short cDay::getDay() const { return(day); }
int cDay::getYear() const { return(year); }

void cDay::setMonth(short m) { month = m; }
void cDay::setDay(short d) { day = d; }
void cDay::setYear(int y) { year = y; }

void cDay::save(xmlNodePtr curNode) const {
    xml::saveNonZeroNum(curNode, "Year", year);
    xml::saveNonZeroNum(curNode, "Month", month);
    xml::saveNonZeroNum(curNode, "Day", day);
}

void cDay::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Day")) xml::copyToNum(day, childNode);
        else if(NODE_NAME(childNode, "Month")) xml::copyToNum(month, childNode);
        else if(NODE_NAME(childNode, "Year")) xml::copyToNum(year, childNode);
        childNode = childNode->next;
    }
}


//*********************************************************************
//                      cMonth
//*********************************************************************

cMonth::cMonth() {
    id = days = 0;
    name = "";
}

short cMonth::getId() const { return(id); }
bstring cMonth::getName() const { return(name); }
short cMonth::getDays() const { return(days); }

//*********************************************************************
//                      save
//*********************************************************************

void cMonth::save(xmlNodePtr curNode) const {
    xml::newNumProp(curNode, "id", id);
    xml::saveNonNullString(curNode, "Name", name);
    xml::saveNonZeroNum(curNode, "Days", days);
}


//*********************************************************************
//                      load
//*********************************************************************

void cMonth::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    id = xml::getIntProp(curNode, "id");
    while(childNode) {
        if(NODE_NAME(childNode, "Days")) xml::copyToNum(days, childNode);
        else if(NODE_NAME(childNode, "Name")) xml::copyToBString(name, childNode);
        childNode = childNode->next;
    }
}


//*********************************************************************
//                      cSeason
//*********************************************************************

cSeason::cSeason() {
    id = NO_SEASON;
    month = day = 0;
    name = "";
}

Season cSeason::getId() const { return(id); }
bstring cSeason::getName() const { return(name); }
short cSeason::getMonth() const { return(month); }
short cSeason::getDay() const { return(day); }
cWeather *cSeason::getWeather() { return(&weather); }

//*********************************************************************
//                      save
//*********************************************************************

void cSeason::save(xmlNodePtr curNode) const {
    xml::newNumProp(curNode, "id", (int)id);
    xml::saveNonNullString(curNode, "Name", name);
    xml::saveNonZeroNum(curNode, "Month", month);
    xml::saveNonZeroNum(curNode, "Day", day);

    xmlNodePtr childNode = xml::newStringChild(curNode, "Weather");
    weather.save(childNode);
}


//*********************************************************************
//                      load
//*********************************************************************

void cSeason::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    id = (Season)xml::getIntProp(curNode, "id");
    while(childNode) {
        if(NODE_NAME(childNode, "Month")) xml::copyToNum(month, childNode);
        else if(NODE_NAME(childNode, "Day")) xml::copyToNum(day, childNode);
        else if(NODE_NAME(childNode, "Name")) xml::copyToBString(name, childNode);
        else if(NODE_NAME(childNode, "Weather")) {
            weather.load(childNode);
        }
        childNode = childNode->next;
    }
}


//*********************************************************************
//                      Calendar
//*********************************************************************

Calendar::Calendar() {
    totalDays = curYear = curMonth = curDay = shipUpdates = adjHour = 0;
    lastPirate = "";
    load();
    // set the startup time to whatever is saved in the xml file
}

//*********************************************************************
//                      getTotalDays
//*********************************************************************

int Calendar::getTotalDays() const { return(totalDays); }

//*********************************************************************
//                      getCurYear
//*********************************************************************

int Calendar::getCurYear() const { return(curYear); }

//*********************************************************************
//                      getCurMonth
//*********************************************************************

short Calendar::getCurMonth() const { return(curMonth); }

//*********************************************************************
//                      getCurDay
//*********************************************************************

short Calendar::getCurDay() const { return(curDay); }

//*********************************************************************
//                      getAdjHour
//*********************************************************************

short Calendar::getAdjHour() const { return(adjHour); }

//*********************************************************************
//                      getCurSeason
//*********************************************************************

cSeason *Calendar::getCurSeason() const { return(curSeason); }

//*********************************************************************
//                      whatSeason
//*********************************************************************

Season Calendar::whatSeason() const { return(curSeason->getId()); }

//*********************************************************************
//                      getLastPirate
//*********************************************************************

bstring Calendar::getLastPirate() const { return(lastPirate); }

//*********************************************************************
//                      resetToMidnight
//*********************************************************************

void Calendar::resetToMidnight() {
    adjHour = 0;
    shipUpdates = 0;
    setLastPirate("");
}

//*********************************************************************
//                      isBirthday
//*********************************************************************

bool Calendar::isBirthday(const Player* target) const {
    cDay* b = target->getBirthday();
    if(!b)
        return(0);
    return(b->getDay() == curDay && b->getMonth() == curMonth);
}

//*********************************************************************
//                      getMonth
//*********************************************************************

cMonth* Calendar::getMonth(short id) const {
    std::list<cMonth*>::const_iterator mt;

    for(mt = months.begin() ; mt != months.end() ; mt++) {
        if(id == (*mt)->getId())
            return(*mt);
    }
    return(0);
}

//*********************************************************************
//                      setSeason
//*********************************************************************

void Calendar::setSeason() {
    std::list<cSeason*>::iterator st;

    // we start at the end
    for(st = seasons.begin() ; st != seasons.end() ; st++)
        curSeason = (*st);

    for(st = seasons.begin() ; st != seasons.end() ; st++) {
        if( curMonth > (*st)->getMonth() ||
            (curMonth == (*st)->getMonth() && curDay >= (*st)->getDay())
        )
            curSeason = (*st);
    }
}

//*********************************************************************
//                      printtime
//*********************************************************************

void Calendar::printtime(const Player* player) const {
    int daytime = gConfig->currentHour(), sun=0;
    cMonth* month = getMonth(curMonth);
    const CatRefInfo* cri = gConfig->getCatRefInfo(player->getConstRoomParent(), 1);
    bstring yearsSince = "";
    int year = curYear;

    if(cri)
        year += cri->getYearOffset();

    if(cri && cri->getYearsSince() != "") {
        yearsSince += cri->getYearsSince();
    } else {
        yearsSince = "the dawn of time";
    }

    if(isDay()) {
        // this is an easy one
        sun = SUNSET - daytime;
    } else {
        // nighttime wraps, so add 24 if needed
        sun = SUNRISE - daytime;
        if(sun < 0)
            sun += 24;
    }

    if(month) {
        player->print("It is the %s of %s, the %s month of the year.",
            getOrdinal(curDay).c_str(), month->getName().c_str(),
            getOrdinal(curMonth).c_str());
    } else {
        player->print("It is the %s day of the %s month of the year.",
            getOrdinal(curDay).c_str(), getOrdinal(curMonth).c_str());
    }
    if(curSeason)
        player->print(" It is %s.", curSeason->getName().c_str());
    player->print("\n");

    player->print("It has been %d year%s since %s.\n",
        year, year==1 ? "" : "s", yearsSince.c_str());
    player->print("The time is %d:%02d %s. It is %s", gConfig->currentHour(true),
        gConfig->currentMinutes(), daytime > 11 ? "PM" : "AM", isDay() ? "day" : "night");

    if(sun > 0) {
        player->print(", the sun will %s in %d hour%s.",
            isDay() ? "set" : "rise",
            sun, sun==1 ? "" : "s");
    } else
        player->print(".");
    player->print("\n");
}

//*********************************************************************
//                      save
//*********************************************************************

void Calendar::save() const {
    xmlDocPtr   xmlDoc;
    xmlNodePtr      rootNode, curNode, childNode;
    char            filename[80];

    // we only save if we're up on the main port
    if(gConfig->getPortNum() != 3333)
        return;

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Calendar", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    xml::saveNonZeroNum(rootNode, "TotalDays", totalDays);
    // only save ship updates if we're inaccurate
    //if(shipUpdates != gConfig->expectedShipUpdates())
    //  xml::saveNonZeroInt(rootNode, "ShipUpdates", gConfig->expectedShipUpdates() - shipUpdates);
    xml::newStringChild(rootNode, "LastPirate", lastPirate);

    curNode = xml::newStringChild(rootNode, "Current");
    xml::saveNonZeroNum(curNode, "Year", curYear);
    xml::saveNonZeroNum(curNode, "Month", curMonth);
    xml::saveNonZeroNum(curNode, "Day", curDay);
    xml::saveNonZeroNum(curNode, "Hour", gConfig->currentHour());

    curNode = xml::newStringChild(rootNode, "Seasons");
    std::list<cSeason*>::const_iterator st;
    for(st = seasons.begin() ; st != seasons.end() ; st++) {
        childNode = xml::newStringChild(curNode, "Season");
        (*st)->save(childNode);
    }

    curNode = xml::newStringChild(rootNode, "Months");
    std::list<cMonth*>::const_iterator mt;
    for(mt = months.begin() ; mt != months.end() ; mt++) {
        childNode = xml::newStringChild(curNode, "Month");
        (*mt)->save(childNode);
    }

    sprintf(filename, "%s/calendar.xml", Path::PlayerData);

    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
}

//*********************************************************************
//                      advance
//*********************************************************************

void Calendar::advance() {
    cMonth* month=0;

    curDay++;
    totalDays++;

    // get the month to see if we overflow
    month = getMonth(curMonth);

    while(curDay > month->getDays()) {
        // next month!
        curDay -= month->getDays();
        advanceMonth();
        month = getMonth(curMonth);
    }

    setSeason();
}

//*********************************************************************
//                      advanceMonth
//*********************************************************************

void Calendar::advanceMonth() {
    std::list<cMonth*>::iterator mt;
    cMonth  *month=0;

    for(mt = months.begin() ; mt != months.end() ; mt++) {
        if(curMonth == (*mt)->getId()) {
            month = (*mt);
            break;
        }
    }

    if(month)
        mt++;
    if(mt == months.end()) {
        curYear++;
        mt = months.begin();
    }

    month = (*mt);
    curMonth = month->getId();
}

//*********************************************************************
//                      loadCurrent
//*********************************************************************

void Calendar::loadCurrent(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Year")) xml::copyToNum(curYear, childNode);
        else if(NODE_NAME(childNode, "Month")) xml::copyToNum(curMonth, childNode);
        else if(NODE_NAME(childNode, "Day")) xml::copyToNum(curDay, childNode);
        else if(NODE_NAME(childNode, "Hour")) xml::copyToNum(adjHour, childNode);
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      loadSeasons
//*********************************************************************

void Calendar::loadSeasons(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    cSeason* season=0;

    while(childNode) {
        if(NODE_NAME(childNode, "Season")) {
            season = new cSeason;
            season->load(childNode);
            seasons.push_back(season);
        }
        childNode = childNode->next;
    }
}


//*********************************************************************
//                      loadMonths
//*********************************************************************

void Calendar::loadMonths(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    cMonth* month=0;

    while(childNode) {
        if(NODE_NAME(childNode, "Month")) {
            month = new cMonth;
            month->load(childNode);
            months.push_back(month);
        }
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      load
//*********************************************************************

void Calendar::load() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    xmlNodePtr  curNode;
    char        filename[80];

    sprintf(filename, "%s/calendar.xml", Path::PlayerData);

    if(!file_exists(filename))
        merror("Unable to find calendar file", FATAL);

    if((xmlDoc = xml::loadFile(filename, "Calendar")) == nullptr)
        merror("Unable to read calendar file", FATAL);

    rootNode = xmlDocGetRootElement(xmlDoc);
    curNode = rootNode->children;


    while(curNode) {
             if(NODE_NAME(curNode, "TotalDays")) xml::copyToNum(totalDays, curNode);
        else if(NODE_NAME(curNode, "ShipUpdates")) xml::copyToNum(shipUpdates, curNode);
        else if(NODE_NAME(curNode, "LastPirate")) {
            xml::copyToBString(lastPirate, curNode);
        } else if(NODE_NAME(curNode, "Current"))
            loadCurrent(curNode);
        else if(NODE_NAME(curNode, "Seasons"))
            loadSeasons(curNode);
        else if(NODE_NAME(curNode, "Months"))
            loadMonths(curNode);
        curNode = curNode->next;
    }

    setSeason();

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
}


//*********************************************************************
//                      clear
//*********************************************************************

void Calendar::clear() {
    std::list<cSeason*>::iterator st;
    cSeason *season=0;

    for(st = seasons.begin() ; st != seasons.end() ; st++) {
        season = (*st);
        delete season;
    }
    seasons.clear();


    std::list<cMonth*>::iterator mt;
    cMonth *month=0;

    for(mt = months.begin() ; mt != months.end() ; mt++) {
        month = (*mt);
        delete month;
    }
    months.clear();

    delete this;
}

//*********************************************************************
//                      setLastPirate
//*********************************************************************

void Calendar::setLastPirate(bstring name) {
    std::ostringstream oStr;
    oStr << "The " << getOrdinal(curDay).c_str() << " of " << getMonth(curMonth)->getName();
    if(name != "")
        oStr << " by " << name;
    oStr << ".";
    lastPirate = oStr.str();
}


//*********************************************************************
//                      checkBirthdays
//*********************************************************************

int checkBirthdays(Player* player, cmd* cmnd) {
    const Player* target=0;
    const Calendar* calendar = gConfig->getCalendar();
    bool    found = false;

    if(!player->ableToDoCommand())
        return(0);

    if(player->isBlind()) {
        player->printColor("^CYou're blind!\n");
        return(0);
    }

    player->print("It is the %s of %s, the %s month of the year.\n",
        getOrdinal(calendar->getCurDay()).c_str(),
        calendar->getMonth(calendar->getCurMonth())->getName().c_str(),
        getOrdinal(calendar->getCurMonth()).c_str());

    calendar = gConfig->getCalendar();
    for(std::pair<bstring, Player*> p : gServer->players) {
        target = p.second;

        if(!target->isConnected())
            continue;
        if(target->isStaff())
            continue;
        if(!player->canSee(target))
            continue;

        if(gConfig->calendar->isBirthday(target)) {
            player->printColor("^yToday is %s's birthday! %s is %d years old.\n",
                target->getCName(), target->upHeShe(), target->getAge());
            found = true;
        }
    }

    if(!found)
        player->print("No one online has a birthday today!\n");
    player->print("\n");
    return(0);
}

//*********************************************************************
//                      loadCalendar
//*********************************************************************

void Config::loadCalendar() {
    if(calendar)
        calendar->clear();
    calendar = new Calendar;
}

//*********************************************************************
//                      getCalendar
//*********************************************************************

const Calendar* Config::getCalendar() const {
    return(calendar);
}

//*********************************************************************
//                      reloadCalendar
//*********************************************************************

int reloadCalendar(Player* player) {
    char    filename[80], filename2[80], command[255];

    sprintf(filename, "%s/calendar.load.xml", Path::Game);
    sprintf(filename2, "%s/calendar.xml", Path::PlayerData);

    if(!file_exists(filename)) {
        player->print("File %s does not exist!\n", filename);
        return(0);
    }

    sprintf(command, "cp %s %s", filename, filename2);
    system(command);
    gConfig->loadCalendar();

    player->printColor("^gCalendar reloaded from %s!\n", filename);
    return(0);
}

//*********************************************************************
//                      currentHour
//*********************************************************************

int Config::currentHour(bool format) const {
    int h = ((time(0) - StartTime) / 120 + calendar->getAdjHour()) % 24;
    if(format)
        h = (h % 12 == 0 ? 12 : h % 12);
    return(h);
}

//*********************************************************************
//                      currentMinutes
//*********************************************************************

int Config::currentMinutes() const {
    return(((time(0) - StartTime) / 2) % 60);
}

//*********************************************************************
//                      resetMinutes
//*********************************************************************

void Config::resetMinutes() {
    StartTime = StartTime + currentMinutes() * 2 - 1;
    gConfig->calendar->shipUpdates = 0;
}

//*********************************************************************
//                      expectedShipUpdates
//*********************************************************************

int Config::expectedShipUpdates() const {
    return((time(0) - StartTime) / 2);
}

//*********************************************************************
//                      isDay
//*********************************************************************

bool isDay() {
    int t = gConfig->currentHour();
    return(t >= SUNRISE && t < SUNSET);
}

//*********************************************************************
//                      isWinter
//*********************************************************************

bool BaseRoom::isWinter() const {
    if(!flagIsSet(R_WINTER_COLD))
        return(false);
    return(flagIsSet(R_ALWAYS_WINTER) || gConfig->getCalendar()->whatSeason() == WINTER);
}
