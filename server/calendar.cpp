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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "calendar.hpp"

#include <cstdio>                 // for sprintf
#include <cstdlib>                // for system
#include <ctime>                  // for time
#include <ostream>                // for operator<<, basic_ostream, ostrings...

#include "catRefInfo.hpp"         // for CatRefInfo
#include "config.hpp"             // for Config, gConfig
#include "creatures.hpp"          // for Player
#include "flags.hpp"              // for R_ALWAYS_WINTER, R_WINTER_COLD
#include "mud.hpp"                // for StartTime, SUNRISE, SUNSET
#include "paths.hpp"              // for Game, PlayerData
#include "proto.hpp"              // for getOrdinal, file_exists, checkBirth...
#include "rooms.hpp"              // for BaseRoom
#include "server.hpp"             // for PlayerMap, Server, gServer

class cmd;

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
    month = 0;
    day = 0;
    year = 0;
}

short cDay::getMonth() const { return(month); }
short cDay::getDay() const { return(day); }
int cDay::getYear() const { return(year); }

void cDay::setMonth(short m) { month = m; }
void cDay::setDay(short d) { day = d; }
void cDay::setYear(int y) { year = y; }


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
//                      Calendar
//*********************************************************************

Calendar::Calendar() {
    totalDays = curYear = curMonth = curDay = shipUpdates = adjHour = 0;
    lastPirate = "";
    load();
    // set the startup time to whatever is saved in the xml file
}

int Calendar::getTotalDays() const { return(totalDays); }
int Calendar::getCurYear() const { return(curYear); }
short Calendar::getCurMonth() const { return(curMonth); }
short Calendar::getCurDay() const { return(curDay); }
short Calendar::getAdjHour() const { return(adjHour); }
cSeason *Calendar::getCurSeason() const { return(curSeason); }
Season Calendar::whatSeason() const { return(curSeason->getId()); }
bstring Calendar::getLastPirate() const { return(lastPirate); }

void Calendar::resetToMidnight() {
    adjHour = 0;
    shipUpdates = 0;
    setLastPirate("");
}

bool Calendar::isBirthday(const Player* target) const {
    cDay* b = target->getBirthday();
    if(!b)
        return(false);
    return(b->getDay() == curDay && b->getMonth() == curMonth);
}

cMonth* Calendar::getMonth(short id) const {
    for(const auto& mt : months) {
        if(id == mt->getId())
            return(mt);
    }
    return(nullptr);
}

void Calendar::setSeason() {
    // we start at the end
    for(auto st = seasons.begin() ; st != seasons.end() ; st++)
        curSeason = (*st);

    for(auto st = seasons.begin() ; st != seasons.end() ; st++) {
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

    if(cri && !cri->getYearsSince().empty()) {
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

//*********************************************************************
//                      advance
//*********************************************************************

void Calendar::advance() {
    cMonth* month=nullptr;

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
    cMonth  *month=nullptr;

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
//                      clear
//*********************************************************************

void Calendar::clear() {
    cSeason *season=nullptr;

    for(auto st = seasons.begin() ; st != seasons.end() ; st++) {
        season = (*st);
        delete season;
    }
    seasons.clear();


    cMonth *month=nullptr;

    for(auto mt = months.begin() ; mt != months.end() ; mt++) {
        month = (*mt);
        delete month;
    }
    months.clear();

    delete this;
}

//*********************************************************************
//                      setLastPirate
//*********************************************************************

void Calendar::setLastPirate(const bstring& name) {
    std::ostringstream oStr;
    oStr << "The " << getOrdinal(curDay).c_str() << " of " << getMonth(curMonth)->getName();
    if(!name.empty())
        oStr << " by " << name;
    oStr << ".";
    lastPirate = oStr.str();
}


//*********************************************************************
//                      checkBirthdays
//*********************************************************************

int checkBirthdays(Player* player, cmd* cmnd) {
    const Player* target=nullptr;
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

    for(const auto& p : gServer->players) {
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
    int h = ((time(nullptr) - StartTime) / 120 + calendar->getAdjHour()) % 24;
    if(format)
        h = (h % 12 == 0 ? 12 : h % 12);
    return(h);
}

//*********************************************************************
//                      currentMinutes
//*********************************************************************

int Config::currentMinutes() const {
    return(((time(nullptr) - StartTime) / 2) % 60);
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
    return((time(nullptr) - StartTime) / 2);
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
