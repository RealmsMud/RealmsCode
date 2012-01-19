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
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef GROUP_H_
#define GROUP_H_

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

typedef std::list<Creature*> CreatureList;

class Group {
public:
    friend std::ostream& operator<<(std::ostream& out, const Group* group);
    friend std::ostream& operator<<(std::ostream& out, const Group& group);

    static int invite(Player* player, cmd* cmnd);
    static int join(Player* player, cmd *cmnd);
    static int reject(Player* player, cmd* cmnd);
    static int disband(Player* player, cmd* cmnd);
    static int promote(Player* player, cmd* cmnd);
    static int kick(Player* player, cmd* cmnd);
    static int leave(Player* player, cmd* cmnd);

public:
    Group(Creature* pLeader);
    ~Group();

    bool add(Creature* newMember);
    bool remove(Creature* toRemove);
    void removeAll();
    bool disband();

    bool setLeader(Creature* newLeader);
    void setName(bstring newName);
    void setDescription(bstring newDescription);

    bool inGroup(Creature* target);

    Creature* getLeader();
    int size();
    int getSize(bool countDmInvis = false);
    int getNumInSameRoom(Creature* target);
    Creature* getMember(int num, bool countDmInvis = false);
    Creature* getMember(bstring name, int num, Creature* searcher = NULL, bool includePets = false);
    GroupType getGroupType();
    bstring& getName();
    bstring& getDescription();
    bstring getGroupList(Creature* viewer);

    void sendToAll(bstring msg, Creature* ignore = NULL, bool ignorePets = true);


public:
    CreatureList members;

private:
    Creature* leader;
    bstring name;
    bstring description;

    GroupType groupType;
};


#endif /* GROUP_H_ */
