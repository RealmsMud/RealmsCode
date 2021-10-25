/*
 * calendar.h
 *   Header file for calendar
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
#ifndef CALENDAR_H_
#define CALENDAR_H_

#include <libxml/parser.h>  // for xmlNodePtr
#include <list>

#include "season.hpp"       // for Season
#include "weather.hpp"      // for WeatherString

class Player;

// __..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..
//
//          cWeather
//
// --..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..

class cWeather {
protected:
    std::string sunrise;
    std::string sunset;
    std::string earthTrembles;
    std::string heavyFog;
    std::string beautifulDay;
    std::string brightSun;
    std::string glaringSun;
    std::string heat;
    std::string still;
    std::string lightBreeze;
    std::string strongWind;
    std::string windGusts;
    std::string galeForce;
    std::string clearSkies;
    std::string lightClouds;
    std::string thunderheads;
    std::string lightRain;
    std::string heavyRain;
    std::string sheetsRain;
    std::string torrentRain;
    std::string noMoon;
    std::string sliverMoon;
    std::string halfMoon;
    std::string waxingMoon;
    std::string fullMoon;

public: 
    cWeather();
    void load(xmlNodePtr curNode);
    void save(xmlNodePtr curNode) const;
    [[nodiscard]] std::string get(WeatherString w) const;
};

// __..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..
//
//          cDay
//
// --..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..

class cDay {
protected:
    short       month;
    short       day;
    int         year;

public:
    cDay();

    void load(xmlNodePtr curNode);
    void save(xmlNodePtr curNode) const;

    [[nodiscard]] short getMonth() const;
    [[nodiscard]] short getDay() const;
    [[nodiscard]] int getYear() const;

    void setMonth(short m);
    void setDay(short d);
    void setYear(int y);
};

// __..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..
//
//          cSeason
//
// --..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..

class cSeason {
protected:
    Season      id;
    std::string     name;
    short       month;
    short       day;

    cWeather    weather;

public:
    cSeason();

    void load(xmlNodePtr curNode);
    void save(xmlNodePtr curNode) const;

    [[nodiscard]] Season getId() const;
    [[nodiscard]] std::string getName() const;
    [[nodiscard]] short getMonth() const;
    [[nodiscard]] short getDay() const;

    cWeather *getWeather();
};

// __..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..
//
//          cMonth
//
// --..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..

class cMonth {
protected:
    short       id;
    std::string     name;
    short       days;

public:
    cMonth();

    void load(xmlNodePtr curNode);
    void save(xmlNodePtr curNode) const;

    [[nodiscard]] short getId() const;
    [[nodiscard]] std::string getName() const;
    [[nodiscard]] short getDays() const;
};

// __..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..
//
//          Calendar
//
// --..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..

class Calendar {
protected:
    int     totalDays;
    int     curYear;
    short   curMonth;
    short   curDay;
    short   adjHour;
    std::string lastPirate;

    cSeason *curSeason{};
    std::list<cSeason*> seasons;
    std::list<cMonth*> months;
    void    advanceMonth();

public:
    Calendar();
    int     shipUpdates;

    void clear();
    [[nodiscard]] Season  whatSeason() const;
    cMonth* getMonth(short id) const;
    void    setSeason();
    void    advance();
    void    printtime(const Player* player) const;
    bool    isBirthday(const Player* target) const;
    [[nodiscard]] std::string getLastPirate() const;
    void    setLastPirate(std::string_view name);
    void    resetToMidnight();

    void    load();
    void    save() const;
    void    loadMonths(xmlNodePtr curNode);
    void    loadSeasons(xmlNodePtr curNode);
    void    loadCurrent(xmlNodePtr curNode);

    [[nodiscard]] int     getTotalDays() const;
    [[nodiscard]] int     getCurYear() const;
    [[nodiscard]] short   getCurMonth() const;
    [[nodiscard]] short   getCurDay() const;
    [[nodiscard]] short   getAdjHour() const;

    [[nodiscard]] cSeason *getCurSeason() const;
};


#endif /*CALENDAR_H_*/
