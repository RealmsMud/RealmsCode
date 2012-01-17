/*
 * Group.h
 *   Source file for groups
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


#include "mud.h"
#include "group.h"

Group::Group(Creature* pLeader) {
    //if(pLeader.inGroup())
    //  throw new bstring("Error: Leader already in another group\n");
    members.push_back(pLeader);
    //pLeader->setGroup(this);
    leader = pLeader;
    groupType = GROUP_DEFAULT;
    name = bstring(leader->getName()) + "'s group.";
    description = "A group, lead by " + bstring(leader->getName());
    // Register us in the server's list of groups
    // gServer->registerGroup(this);
}
Group::~Group() {
    // Unregister us from the server's list of groups
    // gServer->unRegisterGroup(this);
}
std::ostream& operator<<(std::ostream& out, const Group* group) {
    if(group)
        out << (*group);
    return(out);
}
std::ostream& operator<<(std::ostream& out, const Group& group) {
    int i = 0;
    for(Creature* crt : group.members) {
        out << ++i << ") " << crt->getName();
        if(crt->getGroupStatus() == GROUP_LEADER)
            out << " (Leader)";
        out << std::endl;
    }
    return(out);
}



bool Group::add(Creature* newMember, bool isLeader) {
//    if(newMember.inGroup())
//        return(false);
    newMember->setGroup(this);
    if(isLeader)
        newMember->setGroupStatus(GROUP_LEADER);
    else
        newMember->setGroupStatus(GROUP_MEMBER);

    members.push_back(newMember);
    for(Monster* mons : newMember->pets) {
        add(mons);
    }
    return(true);
}
//********************************************************************************
//* remove
//********************************************************************************
// Removes a creature from the group, adjusts leadership if necessary
// and disbands the group if necessary
//
// Returns: true  - The group was deleted and is no longer valid
//          false - The group still exists
bool Group::remove(Creature* toRemove) {
    GroupList::iterator it = std::find(members.begin(), members.end(), toRemove);
    if(it != members.end()) {
        toRemove->setGroup(NULL);
        toRemove->setGroupStatus(GROUP_NO_STATUS);

        // Iterator is invalid now, do not try to access it after this
        members.erase(it);
        // Remove any pets this player had in the group
        for(Monster* mons : toRemove->pets) {
            if(remove(mons))
                return(true);
        }

        // See if the group should be disbanded
        if(members.size() == 1) {
            sendToAll("Your group has been disbanded.\n");
            removeAll();
            delete this;
            return(true);
        }

        // We've already checked for a disband, now check for a leadership change
        if(toRemove == leader) {
            // Find the first non monster and promote them
            for(Creature* crt : members) {
                if(crt->isMonster()) continue;
                leader = crt;
                crt->setGroupStatus(GROUP_LEADER);
                crt->print("You are now the group leader.\n");
                sendToAll(bstring(crt->getName()) + " is now the group leader.\n", crt);
                break;
            }
        }


    }
    return(false);
}

//********************************************************************************
//* remove
//********************************************************************************
// Removes all players from a group (but does not delete it)
void Group::removeAll() {
    for(Creature* crt : members) {
        crt->setGroup(NULL);
        crt->setGroupStatus(GROUP_NO_STATUS);
    }
}

//********************************************************************************
//* getSize
//********************************************************************************
// Parameters: countDmInvis - Should we count DM invis players or not?
// Returns: The number of players in the group
int Group::getSize(bool countDmInvis) {
    if(countDmInvis)
        return(members.size());
    else {
        int count=0;
        for(Creature* crt : members) {
            if(!crt->pFlagIsSet(P_DM_INVIS) && crt->isPlayer())
                count++;
        }
        return(count);
    }

}
Creature* Group::getMember(int num, bool countDmInvis) {
    int count=0;
    for(Creature* crt : members) {
        if(!crt->pFlagIsSet(P_DM_INVIS) && crt->isPlayer())
            count++;
        if(count == num)
            return(crt);
    }
    return(NULL);
}

GroupType Group::getGroupType() {
    return(groupType);
}
bool Group::setLeader(Creature* newLeader) {
    //if(newLeader->getGroup() != this)
    //  return(false);

    leader = newLeader;
    return(true);
}

Creature* Group::getLeader() {
    return(leader);
}

bool Group::inGroup(Creature* target) {
    if(std::find(members.begin(), members.end(), target) == members.end())
        return(false);
    else
        return(true);
}

void Group::sendToAll(bstring msg, Creature* ignore, bool ignorePets) {
    for(Creature* crt : members) {
        if((!ignorePets || !crt->isPet()) && crt != ignore) {
            crt->print("%s", msg.c_str());
        }
    }
}

//********************************************************************************
//* GetGroup
//********************************************************************************
// Parameter: bool inGroup - true - only return the group if they're at least a member
//                           false - return the group if they're an invitee as well
Group* Creature::getGroup(bool inGroup) {
    if(inGroup && groupStatus < GROUP_MEMBER)
        return(NULL);

    return(group);
}
//********************************************************************************
//* SetGroup
//********************************************************************************
void Creature::setGroup(Group* newGroup) {
    // Remove from existing group (Shouldn't happen)
    if(group) {
        std::cout << "Setting group for " << getName() << " but they already have a group." << std::endl;
//        group->remove(this);
    }

    group = newGroup;
}
void Creature::setGroupStatus(GroupStatus newStatus) {
    groupStatus = newStatus;
}

GroupStatus Creature::getGroupStatus() {
    return(groupStatus);
}


void Group::setName(bstring newName) {
    // Test validity of name here
    name = newName;
}
void Group::setDescription(bstring newDescription) {
    // Test validity of description here
    description = newDescription;
}

bstring& Group::getName() {
    return(name);
}
bstring& Group::getDescription() {
    return(description);
}

