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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#pragma once

// adjusted faction = base + gained
// We have to make this distinction: if the max you can gain is 1k (numerical limit),
// and your initial is -250, the most you will ever get is 750. We allow these people
// with negative initial faction to reach 1250 because this will give them the
// equivalent of 1k. This also applies to initial positive faction.

#include <map>

#include "global.hpp"
#include "money.hpp"

class BaseRoom;
class Monster;
class Player;


class FactionRegard {
public:
    FactionRegard();
    void load(xmlNodePtr rootNode);

    [[nodiscard]] long getClassRegard(CreatureClass i) const;
    [[nodiscard]] long getRaceRegard(int i) const;
    [[nodiscard]] long getDeityRegard(int i) const;
    [[nodiscard]] long getVampirismRegard() const;
    [[nodiscard]] long getLycanthropyRegard() const;
    [[nodiscard]] long getGuildRegard(int i) const;
    [[nodiscard]] long getClanRegard(int i) const;
    [[nodiscard]] long getOverallRegard() const;

    [[nodiscard]] std::string guildDisplay() const;
    [[nodiscard]] std::string clanDisplay() const;
protected:
    long classRegard[static_cast<int>(CreatureClass::CLASS_COUNT)]{};
    long raceRegard[RACE_COUNT]{};
    long deityRegard[DEITY_COUNT]{};
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

    [[nodiscard]] std::string getName() const;
    [[nodiscard]] std::string getDisplayName() const;
    [[nodiscard]] std::string getParent() const;
    [[nodiscard]] std::string getGroup() const;
    [[nodiscard]] std::string getSocial() const;
    [[nodiscard]] long getBaseRegard() const;
    [[nodiscard]] long getClassRegard(CreatureClass i) const;
    [[nodiscard]] long getRaceRegard(int i) const;
    [[nodiscard]] long getDeityRegard(int i) const;
    [[nodiscard]] long getVampirismRegard() const;
    [[nodiscard]] long getLycanthropyRegard() const;
    [[nodiscard]] long getInitialRegard(const std::shared_ptr<const Player> &player) const;
    [[nodiscard]] long getGuildRegard(int i) const;
    [[nodiscard]] long getClanRegard(int i) const;

    const FactionRegard* getInitial();
    const FactionRegard* getMax();
    const FactionRegard* getMin();

    bool alwaysHates(const std::shared_ptr<Player>& player) const;
    long getUpperLimit(const std::shared_ptr<Player>& player) const;
    long getLowerLimit(const std::shared_ptr<Player>& player) const;
    std::shared_ptr<Player> findAggro(const std::shared_ptr<BaseRoom>& room);
    [[nodiscard]] bool isParent() const;
    void setParent(std::string_view value);
    void setIsParent(bool value);

    static int getCutoff(int attitude);
    static int getAttitude(int regard);
    static std::string getNoun(int regard);
    static std::string getColor(int regard);
    static std::string getBar(int regard, bool alwaysPad);
    static void worshipSocial(const std::shared_ptr<Monster>& monster);

    static bool willAggro(const std::shared_ptr<const Player>& player, const std::string &faction);
    static bool willSpeakWith(const std::shared_ptr<const Player>& player, const std::string &faction);
    static bool willDoBusinessWith(const std::shared_ptr<const Player>& player, const std::string &faction);
    static bool willBeneCast(const std::shared_ptr<const Player>& player, const std::string &faction);
    static bool willLetThrough(const std::shared_ptr<const Player>& player, const std::string &faction);

    static Money adjustPrice(const std::shared_ptr<const Player>& player, const std::string &faction, Money money, bool sell);
    static bool canPledgeTo(const std::shared_ptr<const Player>& player, const std::string &faction);

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

    static const int MAX_FACTION            = 45000;    // adjusted can't go over this amount
    static const int MIN_FACTION            = -64999;   // adjusted can't go under this amount
    static const int ALWAYS_HATE            = -65000;   // set to this amount and you will always be hated
protected:
    std::string name;
    std::string parent;     // What faction this faction inherits from
    std::string group;      // What display group this faction is in
    std::string displayName;
    std::string social;
    long baseRegard;    // Regard they have to everyone before modification
    FactionRegard   initial;
    FactionRegard   max;
    FactionRegard   min;
    bool is_parent;
};

