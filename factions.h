/*
 * factions.h
 *   Header file for faction
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
#ifndef FACTION_H_
#define FACTION_H_

// adjusted faction = base + gained
// We have to make this distinction: if the max you can gain is 1k (numerical limit),
// and your initial is -250, the most you will ever get is 750. We allow these people
// with negative initial faction to reach 1250 because this will give them the
// equivalent of 1k. This also applies to initial positive faction.


class CrtFaction {
public:
    bstring name;
    int     value;
};


class FactionRegard {
public:
    FactionRegard();
    void load(xmlNodePtr rootNode);

    long getClassRegard(CreatureClass i) const;
    long getRaceRegard(int i) const;
    long getDeityRegard(int i) const;
    long getVampirismRegard() const;
    long getLycanthropyRegard() const;
    long getGuildRegard(int i) const;
    long getClanRegard(int i) const;
    long getOverallRegard() const;

    bstring guildDisplay() const;
    bstring clanDisplay() const;
protected:
    long classRegard[static_cast<int>(CreatureClass::CLASS_COUNT)];
    long raceRegard[RACE_COUNT];
    long deityRegard[DEITY_COUNT];
    long vampirismRegard;
    long lycanthropyRegard;
    std::map<int,long> guildRegard;
    std::map<int,long> clanRegard;
    long overallRegard;
};


class Faction {
public:
    Faction();
    void load(xmlNodePtr rootNode);

    bstring getName() const;
    bstring getDisplayName() const;
    bstring getParent() const;
    bstring getGroup() const;
    bstring getSocial() const;
    long getBaseRegard() const;
    long getClassRegard(CreatureClass i) const;
    long getRaceRegard(int i) const;
    long getDeityRegard(int i) const;
    long getVampirismRegard() const;
    long getLycanthropyRegard() const;
    long getInitialRegard(const Player* player) const;
    long getGuildRegard(int i) const;
    long getClanRegard(int i) const;

    const FactionRegard* getInitial();
    const FactionRegard* getMax();
    const FactionRegard* getMin();

    bool alwaysHates(const Player* player) const;
    long getUpperLimit(const Player* player) const;
    long getLowerLimit(const Player* player) const;
    Player* findAggro(BaseRoom* room);
    bool isParent() const;
    void setParent(bstring value);
    void setIsParent(bool value);

    static int getCutoff(int attitude);
    static int getAttitude(int regard);
    static bstring getNoun(int regard);
    static bstring getColor(int regard);
    static bstring getBar(int regard, bool alwaysPad);
    static void worshipSocial(Monster *monster);

    static bool willAggro(const Player* player, bstring faction);
    static bool willSpeakWith(const Player* player, bstring faction);
    static bool willDoBusinessWith(const Player* player, bstring faction);
    static bool willBeneCast(const Player* player, bstring faction);
    static bool willLetThrough(const Player* player, bstring faction);

    static Money adjustPrice(const Player* player, bstring faction, Money money, bool sell);
    static bool canPledgeTo(const Player* player, bstring faction);

    static const int WORSHIP        = 4;
    static const int REGARD         = 3;
    static const int ADMIRATION     = 2;
    static const int FAVORABLE      = 1;
    static const int INDIFFERENT    = 0;
    static const int DISAPPROVE     = -1;
    static const int DISFAVOR       = -2;   
    static const int CONTEMPT       = -3;
    static const int MALICE         = -4;

    // these are boundaries for adjusted ranges.
    static const int WORSHIP_CUTOFF         = 31000;    // 31000 and higher
    static const int REGARD_CUTOFF          = 25000;    // 25000 to 30999
    static const int ADMIRATION_CUTOFF      = 15000;    // 15000 to 24999
    static const int FAVORABLE_CUTOFF       = 8000;     // 8000 to 14999
    static const int INDIFFFERENT_CUTOFF    = 0;        // 0 to 7999
    static const int DISAPPROVE_CUTOFF      = -1000;    // -1000 to -1
    static const int DISFAVOR_CUTTOFF       = -7000;    // -7000 to -1001
    static const int CONTEMPT_CUTOFF        = -8000;    // -8000 to -7001
    static const int MALICE_CUTOFF          = -32000;   // -8001 and lower

    static const int MAX            = 45000;    // adjusted can't go over this amount
    static const int MIN            = -64999;   // adjusted can't go under this amount
    static const int ALWAYS_HATE    = -65000;   // set to this amount and you will always be hated
protected:
    bstring name;
    bstring parent;     // What faction this faction inherits from
    bstring group;      // What display group this faction is in
    bstring displayName;
    bstring social;
    long baseRegard;    // Regard they have to everyone before modification
    FactionRegard   initial;
    FactionRegard   max;
    FactionRegard   min;
    bool is_parent;
};


#endif /*FACTION_H_*/
