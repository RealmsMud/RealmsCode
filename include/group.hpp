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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef GROUP_H_
#define GROUP_H_

#include <list>

class cmd;
class Creature;
class Player;

#include "common.hpp"
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
typedef std::list<Creature*> CreatureList;

class Group {
public:
    friend std::ostream& operator<<(std::ostream& out, const Group* group);
    friend std::ostream& operator<<(std::ostream& out, const Group& group);

    // Group commands
    static int invite(Player* player, cmd* cmnd);
    static int join(Player* player, cmd *cmnd);
    static int reject(Player* player, cmd* cmnd);
    static int disband(Player* player, cmd* cmnd);
    static int promote(Player* player, cmd* cmnd);
    static int kick(Player* player, cmd* cmnd);
    static int leave(Player* player, cmd* cmnd);
    static int rename(Player* player, cmd* cmnd);
    static int type(Player* player, cmd* cmnd);
    static int set(Player* player, cmd* cmnd, bool set);

public:
    Group(Creature* pLeader);
    ~Group();

    // Add or remove players from a group
    bool add(Creature* newMember, bool addPets = true);
    bool remove(Creature* toRemove);
    void removeAll();
    bool disband();

    // Set information about a group
    bool setLeader(Creature* newLeader);
    void setName(bstring newName);
    void setDescription(bstring newDescription);
    void setGroupType(GroupType newType);
    void setFlag(int flag);
    void clearFlag(int flag);


    // Various info about a group
    bool flagIsSet(int flag) const;
    bool inGroup(Creature* target);
    Creature* getLeader() const;
    int size();
    int getSize(bool countDmInvis = false, bool membersOnly = true);
    int getNumInSameRoom(Creature* target);
    int getNumPlyInSameRoom(Creature* target);
    Creature* getMember(int num, bool countDmInvis = false);
    Creature* getMember(bstring name, int num, Creature* searcher = nullptr, bool includePets = false);
    GroupType getGroupType() const;
    bstring getGroupTypeStr() const;
    bstring getFlagsDisplay();
    const bstring& getName() const;
    const bstring& getDescription() const;
    bstring getGroupList(Creature* viewer);


    void sendToAll(bstring msg, Creature* ignore = nullptr, bool sendToInvited = false);

    bstring getMsdp(Creature* viewer) const;

public:
    CreatureList members;

private:
    Creature* leader;
    bstring name;
    bstring description;

    int flags;

    GroupType groupType;
};


#endif /* GROUP_H_ */
