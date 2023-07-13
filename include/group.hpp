/*
 * Group.h
 *   Header file for groups
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
#include <memory>

class cmd;
class Creature;
class Player;

#include "size.hpp"

enum GroupStatus {
    GROUP_NO_STATUS,
    GROUP_INVITED,
    GROUP_MEMBER,
    GROUP_LEADER,

    GROUP_MAX_STATUS
};

enum GroupType {
    GROUP_PUBLIC,       // Anyone can join
    GROUP_INVITE_ONLY,  // Anyone in the group can invite people
    GROUP_PRIVATE,      // Only the leader can invite people

    GROUP_DEFAULT = GROUP_PUBLIC,

    GROUP_MAX_TYPE
};
enum GroupFlags {
    GROUP_NO_FLAG = -1,
    GROUP_SPLIT_EXPERIENCE = 0,
    GROUP_SPLIT_GOLD,

    GROUP_MAX_FLAG
};
typedef std::list<std::weak_ptr<Creature>> CreatureList;

class Group {
public:
    friend std::ostream& operator<<(std::ostream& out, const Group* group);
    friend std::ostream& operator<<(std::ostream& out, const Group& group);

    // Group commands
    static int invite(const std::shared_ptr<Player>& player, cmd* cmnd);
    static int join(const std::shared_ptr<Player>& player, cmd *cmnd);
    static int reject(const std::shared_ptr<Player>& player, cmd* cmnd);
    static int disband(const std::shared_ptr<Player>& player, cmd* cmnd);
    static int promote(const std::shared_ptr<Player>& player, cmd* cmnd);
    static int kick(const std::shared_ptr<Player>& player, cmd* cmnd);
    static int leave(const std::shared_ptr<Player>& player, cmd* cmnd);
    static int rename(const std::shared_ptr<Player>& player, cmd* cmnd);
    static int type(const std::shared_ptr<Player>& player, cmd* cmnd);
    static int set(const std::shared_ptr<Player>& player, cmd* cmnd, bool set);

public:
    Group(const std::shared_ptr<Creature>& pLeader);
    ~Group();

    // Add or remove players from a group
    bool add(const std::weak_ptr<Creature>& newMember, bool addPets = true);
    bool remove(const std::weak_ptr<Creature>& toRemove);
    void removeAll();
    bool disband();

    // Set information about a group
    bool setLeader(const std::shared_ptr<Creature>& newLeader);
    void setName(std::string_view newName);
    void setDescription(std::string_view newDescription);
    void setGroupType(GroupType newType);
    void setFlag(int flag);
    void clearFlag(int flag);


    // Various info about a group
    [[nodiscard]] bool flagIsSet(int flag) const;
    [[nodiscard]] bool inGroup(std::shared_ptr<Creature> target);
    [[nodiscard]] std::shared_ptr<Creature> getLeader() const;
    [[nodiscard]] int size();
    [[nodiscard]] int getSize(bool countDmInvis = false, bool membersOnly = true);
    [[nodiscard]] int getNumInSameRoom(const std::shared_ptr<Creature>& target);
    [[nodiscard]] int getNumPlyInSameRoom(const std::shared_ptr<Creature>& target);
    [[nodiscard]] std::shared_ptr<Creature> getMember(int num, bool countDmInvis = false);
    [[nodiscard]] std::shared_ptr<Creature> getMember(const std::string&  pName, int num, const std::shared_ptr<Creature>& searcher = nullptr, bool includePets = false);
    [[nodiscard]] GroupType getGroupType() const;
    [[nodiscard]] std::string getGroupTypeStr() const;
    [[nodiscard]] std::string getFlagsDisplay() const;
    [[nodiscard]] const std::string& getName() const;
    [[nodiscard]] const std::string& getDescription() const;
    std::string getGroupList(const std::shared_ptr<Creature>& viewer);


    void sendToAll(std::string_view msg, const std::shared_ptr<Creature>& ignore = nullptr, bool sendToInvited = false);

    [[nodiscard]] std::string getMsdp(const std::shared_ptr<Creature>& viewer) const;

public:
    CreatureList members;

private:
    std::weak_ptr<Creature> leader;
    std::string name;
    std::string description;

    int flags;

    GroupType groupType;
};

