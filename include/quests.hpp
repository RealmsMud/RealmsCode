/*
 * Quests.h
 *   New questing system
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

#include <list>
#include <map>
#include <nlohmann/json.hpp>

#include "catRef.hpp"
#include "money.hpp"

class Player;
class Monster;
class Object;
class UniqueRoom;

enum class QuestRepeatFrequency {
    REPEAT_NEVER = 0,
    REPEAT_UNLIMITED = 1,
    REPEAT_DAILY = 2,
    REPEAT_WEEKLY = 3,
};

enum class QuestEligibility {
    INELIGIBLE = 0,
    INELIGIBLE_HAS_QUEST = 1,
    INELIGIBLE_NOT_REPETABLE = 2,
    INELIGIBLE_DAILY_NOT_EXPIRED = 3,
    INELIGIBLE_WEEKLY_NOT_EXPIRED = 4,
    INELIGIBLE_UNCOMPLETED_PREREQUISITES = 5,
    INELIGIBLE_FACTION = 6,
    INELIGIBLE_LEVEL = 7,
    ELIGIBLE = 8,
    ELIGIBLE_DAILY = 9,
    ELIGIBLE_WEEKLY = 10,
};

enum class QuestTurninStatus {
    NO_TURNINS = 0,
    UNCOMPLETED_TURNINS = 1,
    COMPLETED_TURNINS = 2,
    COMPLETED_DAILY_TURNINS = 3,
};

// Descriptive information about a quest
class QuestInfo {
public:
    static CatRef getQuestId(xmlNodePtr curNode);
    static CatRef getQuestId(const std::string& strId);
    static void saveQuestId(xmlNodePtr curNode, const CatRef& questId);

public:
    friend void to_json(nlohmann::json &j, const QuestInfo &quest);
    friend void from_json(const nlohmann::json &j, QuestInfo &quest);

public:
    QuestInfo();
    QuestInfo(xmlNodePtr rootNode);

    void reset();

    [[nodiscard]] CatRef getId() const;
    [[nodiscard]] std::string getName() const;
    [[nodiscard]] std::string getDisplayName() const;
    [[nodiscard]] std::string getDisplayString() const;
    [[nodiscard]] bool isRepeatable() const;
    [[nodiscard]] int getTimesRepeatable() const;
    [[nodiscard]] QuestEligibility getEligibility(const std::shared_ptr<const Player> &player, const std::shared_ptr<const Monster> &giver) const;
    [[nodiscard]] const CatRef & getTurnInMob() const;
    [[nodiscard]] bool canGetQuest(const std::shared_ptr<const Player> &player, const std::shared_ptr<const Monster> &giver) const;

    void printReceiveString(const std::shared_ptr<Player> &player, const std::shared_ptr<const Monster> &giver) const;
    void printCompletionString(const std::shared_ptr<Player> &player, const std::shared_ptr<const Monster> &giver) const;
    void giveInitialitems(const std::shared_ptr<const Monster> &giver, const std::shared_ptr<Player> &player) const;
private:
    CatRef questId;
    std::string name;
    std::string description;        // Description of the quest
    std::string receiveString;      // String that is output to the player when they receive this quest
    std::string completionString;   // String that is output to the player when they complete this quest

    std::string revision;           // This will be used for making sure all QuestCompletions are synced up with the
                                    // parent quest in case we make changes to them

    bool repeatable{};                        // Can this quest be repeated multiple times?
    QuestRepeatFrequency repeatFrequency;   // How often can this quest be repeated
    int timesRepeatable{};                  // Number of times the quest can be repeated.  0 for infinite
    bool sharable{};                          // Can this quest be shared with other players?
    int minLevel{};                           // Minimum required level to get this quest
    int minFaction{};                         // Minimum required faction to get this quest (Based on mob's primeFaction)
    int level{};                              // Level of this quest, used to adjust rewards

    std::list<CatRef> preRequisites;        // A list of quests that must have been completed before someone is allowed
                                            // to get this quest

    std::list<QuestCatRef> initialItems;    // Items the player is given when the quest begins
    std::list<QuestCatRef> mobsToKill;      // A list of monsters that need to be killed for this quest
    std::list<QuestCatRef> itemsToGet;      // A list of items that need to be obtained for this quest
    std::list<QuestCatRef> roomsToVisit;    // Rooms they need to visit before they can finish this quest

    // Completion Info
    CatRef turnInMob;                           // Monster that we turn this quest into for completion
    Money cashReward;                           // Coin Reward
    long expReward{};                           // Exp reward
    short alignmentChange{};			        // Amount alignment changes by this amount upon quest completion
    short alignmentShift{};                     // Alignment will shift this many tiers upon quest completion
    std::list<QuestCatRef> itemRewards;         // Items rewarded on completion
    std::map<std::string, long> factionRewards; // Factions to be modified

    friend class QuestCompletion;
};

// Class to keep track of what has been completed on a given quest for a player so far
class QuestCompletion {
public:
    QuestCompletion(QuestInfo* parent, std::shared_ptr<Player> player);
    QuestCompletion(xmlNodePtr rootNode, const std::shared_ptr<Player>& player);
    xmlNodePtr save(xmlNodePtr rootNode) const;

    //int getQuestId();
    void resetParentQuest();
    [[nodiscard]] QuestInfo* getParentQuest() const;
private:
    CatRef questId;
    QuestInfo *parentQuest{}; // What quest are we keeping track of?
    std::weak_ptr<Player> parentPlayer;   // Parent Player for this quest
    std::string revision;

    std::list<QuestCatRef> mobsKilled;      // How many of the required monsters have we killed
    std::list<QuestCatRef> roomsVisited;    // How many of the required rooms have we visited.

    // NOTE: Don't keep track of items here, we'll calculate that whenever we
    //       look at the quest completion info, because they might dispose of
    //       items between checks, they can't however change what rooms they've
    //       been to, or what monsters they've killed

    bool mobsCompleted;
    bool itemsCompleted;
    bool roomsCompleted;
public:
    void updateMobKills(const std::shared_ptr<Monster>&  monster);
    void updateItems(const std::shared_ptr<Object>&  object);
    void updateRooms(const std::shared_ptr<UniqueRoom>& room);
    std::string getStatusDisplay();

    bool checkQuestCompletion(bool showMessage = true);
    [[nodiscard]] bool hasRequiredMobs() const;
    [[nodiscard]] bool hasRequiredItems() const;
    [[nodiscard]] bool hasRequiredRooms() const;

    bool complete(const std::shared_ptr<Monster>&  monster);
};

class QuestCompleted {
private:
    int times{};
    time_t lastCompleted{};
    void init();
public:
    QuestCompleted(xmlNodePtr rootNode);
    QuestCompleted(int pTimes);  // Legacy
    QuestCompleted(const QuestCompleted &qc);
    QuestCompleted();

    xmlNodePtr save(xmlNodePtr rootNode, const CatRef& questId) const;

    void complete();

    [[nodiscard]] time_t getLastCompleted() const;
    [[nodiscard]] int getTimesCompleted() const;
};


class TalkResponse {
public:
    TalkResponse();
    TalkResponse(xmlNodePtr rootNode);
    xmlNodePtr saveToXml(xmlNodePtr rootNode) const;

    std::list<std::string> keywords; // Multiple keywords!
    std::string response; // What he'll respond with, ^C type colors allowed
    std::string action; // Cast on a player, flip them off, give them an item, give them a quest....etc
    QuestInfo* quest; // Link to associated quest (if any)
private:
    void parseQuest();
};

time_t getDailyReset();
time_t getWeeklyReset();


