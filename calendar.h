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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef CALENDAR_H_
#define CALENDAR_H_

// __..--..__..--..__..--..__..--..__..--..__..--..__..--..__..--..
//
//          cWeather
//
// --..__..--..__..--..__..--..__..--..__..--..__..--..__..--..__..

class cWeather {
protected:
    bstring sunrise;
    bstring sunset;
    bstring earthTrembles;
    bstring heavyFog;
    bstring beautifulDay;
    bstring brightSun;
    bstring glaringSun;
    bstring heat;
    bstring still;
    bstring lightBreeze;
    bstring strongWind;
    bstring windGusts;
    bstring galeForce;
    bstring clearSkies;
    bstring lightClouds;
    bstring thunderheads;
    bstring lightRain;
    bstring heavyRain;
    bstring sheetsRain;
    bstring torrentRain;
    bstring noMoon;
    bstring sliverMoon;
    bstring halfMoon;
    bstring waxingMoon;
    bstring fullMoon;

public: 
    cWeather();
    void load(xmlNodePtr curNode);
    void save(xmlNodePtr curNode) const;
    bstring get(WeatherString w) const;
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

    short getMonth() const;
    short getDay() const;
    int getYear() const;

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
    bstring     name;
    short       month;
    short       day;

    cWeather    weather;

public:
    cSeason();

    void load(xmlNodePtr curNode);
    void save(xmlNodePtr curNode) const;

    Season getId() const;
    bstring getName() const;
    short getMonth() const;
    short getDay() const;

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
    bstring     name;
    short       days;

public:
    cMonth();

    void load(xmlNodePtr curNode);
    void save(xmlNodePtr curNode) const;

    short getId() const;
    bstring getName() const;
    short getDays() const;
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
    bstring lastPirate;

    cSeason *curSeason;
    std::list<cSeason*> seasons;
    std::list<cMonth*> months;
    void    advanceMonth();

public:
    Calendar();
    int     shipUpdates;

    void clear();
    Season  whatSeason() const;
    cMonth* getMonth(short id) const;
    void    setSeason();
    void    advance();
    void    printtime(const Player* player) const;
    bool    isBirthday(const Player* target) const;
    bstring getLastPirate() const;
    void    setLastPirate(bstring name);
    void    resetToMidnight();

    void    load();
    void    save() const;
    void    loadMonths(xmlNodePtr curNode);
    void    loadSeasons(xmlNodePtr curNode);
    void    loadCurrent(xmlNodePtr curNode);

    int     getTotalDays() const;
    int     getCurYear() const;
    short   getCurMonth() const;
    short   getCurDay() const;
    short   getAdjHour() const;

    cSeason *getCurSeason() const;
};


#endif /*CALENDAR_H_*/
