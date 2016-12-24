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
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

// Mud includes
#include "creatures.h"
#include "group.h"
#include "mud.h"
#include "server.h"

// C++ Includes
#include <iomanip>

//################################################################################
//# Group methods for groups
//################################################################################

Group::Group(Creature* pLeader) {
    //if(pLeader.inGroup())
    //  throw(std::runtime_error("Error: Leader already in another group\n"));
    flags = 0;
    add(pLeader);
    leader = pLeader;
    groupType = GROUP_DEFAULT;
    if(pLeader->pFlagIsSet(P_XP_DIVIDE))
        setFlag(GROUP_SPLIT_EXPERIENCE);
    if(pLeader->pFlagIsSet(P_GOLD_SPLIT))
        setFlag(GROUP_SPLIT_GOLD);

    name = bstring(leader->getName()) + "'s group";
    description = "A group, lead by " + bstring(leader->getName());
    // Register us in the server's list of groups
    gServer->registerGroup(this);
}

Group::~Group() {
    // Unregister us from the server's list of groups
    removeAll();
    gServer->unRegisterGroup(this);
}


//********************************************************************************
//* Add
//********************************************************************************
// Adds a creature to the group, and adds his pets too if the addPets parameter
// is set

bool Group::add(Creature* newMember, bool addPets) {
    Group* oldGroup = newMember->getGroup(false);
    if(oldGroup && oldGroup != this) {
        oldGroup->remove(newMember);
        oldGroup = nullptr;
    }

    // No adding someone twice
    if(!oldGroup) {
        newMember->setGroup(this);
        members.push_back(newMember);
    }
    if(addPets) {
        for(Monster* mons : newMember->pets) {
            add(mons);
        }
    }
    newMember->setGroupStatus(GROUP_MEMBER);
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
    CreatureList::iterator it = std::find(members.begin(), members.end(), toRemove);
    if(it != members.end()) {
        toRemove->setGroup(nullptr);
        toRemove->setGroupStatus(GROUP_NO_STATUS);

        // Iterator is invalid now, do not try to access it after this
        members.erase(it);
        // Remove any pets this player had in the group
        for(Monster* mons : toRemove->pets) {
            if(remove(mons))
                return(true);
        }

        // See if the group should be disbanded
        if(this->getSize(false, false) <= 1)
            return(disband());

        // We've already checked for a disband, now check for a leadership change
        if(toRemove == leader) {
            leader = this->getMember(1, false);

            // Something's wrong here
            if(!leader) {
                std::clog << "Couldn't find a replacement leader.\n";
                return(disband());
            }

            leader->setGroupStatus(GROUP_LEADER);
            leader->print("You are now the group leader.\n");
            sendToAll(bstring(leader->getName()) + " is now the group leader.\n", leader);
        }


    }
    return(false);
}

//********************************************************************************
//* disband
//********************************************************************************
// Removes all players from a group and deletes it
bool Group::disband() {
    sendToAll("Your group has been disbanded.\n");
    removeAll();
    delete this;
    return(true);
}

//********************************************************************************
//* remove
//********************************************************************************
// Removes all players from a group (but does not delete it)
void Group::removeAll() {
    for(Creature* crt : members) {
        crt->setGroup(nullptr);
        crt->setGroupStatus(GROUP_NO_STATUS);
    }
    members.clear();
}

//********************************************************************************
//* size
//********************************************************************************
// Returns the absolute size of the group
int Group::size() {
    int count=0;
    for(Creature* crt : members) {
        if(crt->getGroupStatus() >= GROUP_MEMBER)
            count++;
    }
    return(count);

}

//********************************************************************************
//* getSize
//********************************************************************************
// Parameters: countDmInvis - Should we count DM invis players or not?
//             membersOnly  - Should we count only members, or include invited people as well
// Returns: The number of players in the group
int Group::getSize(bool countDmInvis, bool membersOnly) {
    int count=0;
    for(Creature* crt : members) {
        if((countDmInvis || !crt->pFlagIsSet(P_DM_INVIS)) && crt->isPlayer() && (crt->getGroupStatus() >= GROUP_MEMBER || ! membersOnly))
            count++;
    }
    return(count);

}
//********************************************************************************
//* getNumInSameRoup
//********************************************************************************
// Returns the number of group members in the same room as the target
int Group::getNumInSameRoom(Creature* target) {
    int count=0;
    for(Creature* crt : members) {
        if(crt != target && target->inSameRoom(crt))
            count++;
    }
    return(count);
}
//********************************************************************************
//* getNumPlyInSameRoup
//********************************************************************************
// Returns the number of group members (players) in the same room as the target

int Group::getNumPlyInSameRoom(Creature* target) {
    int count=0;
    for(Creature* crt : members) {
        if(crt != target && crt->isPlayer() && target->inSameRoom(crt))
            count++;
    }
    return(count);
}

//********************************************************************************
//* getMember
//********************************************************************************
// Parameters: countDmInvis - Should we count DM invis players or not?
// Returns: The chosen players in the group

Creature* Group::getMember(int num, bool countDmInvis) {
    int count=0;
    for(Creature* crt : members) {
        if((countDmInvis || !crt->pFlagIsSet(P_DM_INVIS)) && crt->isPlayer() && crt->getGroupStatus() >= GROUP_MEMBER)
            count++;
        if(count == num)
            return(crt);
    }
    return(nullptr);
}
//********************************************************************************
//* getMember
//********************************************************************************
// Parameters:  name        - Name (possibly partial) of the match we're looking for
//              num         - Number in the list for a match
//              Searcher    - The creature doing the search (allows nulls)
//              includePets - Include pets in the search
// Returns: A pointer to the creature, if found
Creature* Group::getMember(bstring name, int num, Creature* searcher, bool includePets) {
    int match = 0;
    for(Creature* crt : members) {
        if(!crt->isPlayer() && !includePets) continue;
        if(crt->getGroupStatus() < GROUP_MEMBER) continue;
        if(!searcher || !searcher->canSee(crt)) continue;
        if(keyTxtEqual(crt, name.c_str())) {
            if(++match == num) {
                return(crt);
            }
        }
    }
    return(nullptr);
}


//********************************************************************************
//* inGroup
//********************************************************************************
// Returns: Is the target in this group?
bool Group::inGroup(Creature* target) {
    if(std::find(members.begin(), members.end(), target) == members.end())
        return(false);
    else
        return(true);
}

//********************************************************************************
//* SendToAll
//********************************************************************************
// Parameters: sendToInvited - Are invited members counted as in the group or not?
// Send msg to everyone in the group except ignore
void Group::sendToAll(bstring msg, Creature* ignore, bool sendToInvited) {
    for(Creature* crt : members) {
        if(!crt->isPet() && crt != ignore && (sendToInvited || crt->getGroupStatus() >= GROUP_MEMBER )) {
            *crt << ColorOn << msg << ColorOff;
        }
    }
}

//********************************************************************************
//* setGroupType
//********************************************************************************
void Group::setGroupType(GroupType newType) {
    groupType = newType;
}

//********************************************************************************
//* setName
//********************************************************************************
void Group::setName(bstring newName) {
    // Test validity of name here
    name = newName;
}

//********************************************************************************
//* setLeader
//********************************************************************************
bool Group::setLeader(Creature* newLeader) {
    //if(newLeader->getGroup() != this)
    //  return(false);
    leader->setGroupStatus(GROUP_MEMBER);
    newLeader->setGroupStatus(GROUP_LEADER);
    leader = newLeader;

    return(true);
}

//********************************************************************************
//* setDescription
//********************************************************************************
void Group::setDescription(bstring newDescription) {
    // Test validity of description here
    description = newDescription;
}

//********************************************************************************
//* getGroupType
//********************************************************************************
GroupType Group::getGroupType() {
    return(groupType);
}

//********************************************************************************
//* getLeader
//********************************************************************************
Creature* Group::getLeader() {
    return(leader);
}
//********************************************************************************
//* getName
//********************************************************************************
bstring& Group::getName() {
    return(name);
}
//********************************************************************************
//* getDescription
//********************************************************************************
bstring& Group::getDescription() {
    return(description);
}



//********************************************************************************
//* GetGroup
//********************************************************************************
// Parameter: bool inGroup - true - only return the group if they're at least a member
//                           false - return the group if they're an invitee as well
Group* Creature::getGroup(bool inGroup) {
    if(inGroup && groupStatus < GROUP_MEMBER)
        return(nullptr);

    return(group);
}
//********************************************************************************
//* SetGroup
//********************************************************************************
void Creature::setGroup(Group* newGroup) {
    // Remove from existing group (Shouldn't happen)
    if(group && newGroup != nullptr) {
        std::clog << "Setting group for " << getName() << " but they already have a group." << std::endl;
    }

    group = newGroup;
}
//********************************************************************************
//* SetGroupStatus
//********************************************************************************
void Creature::setGroupStatus(GroupStatus newStatus) {
    groupStatus = newStatus;
}

//********************************************************************************
//* GetGroupStatus
//********************************************************************************
GroupStatus Creature::getGroupStatus() {
    return(groupStatus);
}

//********************************************************************************
//* InSameGroup
//********************************************************************************
bool Creature::inSameGroup(Creature* target) {
    if(!target) return(false);
    return(getGroup() == target->getGroup());
}
//********************************************************************************
//* GetGroupLeader
//********************************************************************************
Creature* Creature::getGroupLeader() {
    group = getGroup();
    if(!group) return(nullptr);
    return(group->getLeader());
}

//********************************************************************************
//* Group Flags
//********************************************************************************
void Group::setFlag(int flag) {
    if(flag <= GROUP_NO_FLAG || flag >= GROUP_MAX_FLAG)
        return;
    flags |= (1 << flag);
}
void Group::clearFlag(int flag) {
    if(flag <= GROUP_NO_FLAG || flag >= GROUP_MAX_FLAG)
        return;
    flags &= ~(1 << flag);
}

bool Group::flagIsSet(int flag) {
    return(flags & (1 << flag));
}

//################################################################################
//# Server methods for groups
//################################################################################

bool Server::registerGroup(Group* toRegister) {
    std::clog << "Registering " << toRegister->getName() << std::endl;
    groups.push_back(toRegister);
    return(true);
}

bool Server::unRegisterGroup(Group* toUnRegister) {
    std::clog << "Unregistering " << toUnRegister->getName() << std::endl;
    groups.remove(toUnRegister);
    return(true);
}


//################################################################################
//# Stream Operators and toString methods
//################################################################################

//********************************************************************************
//* GetGroupTypeStr
//********************************************************************************

bstring Group::getGroupTypeStr() {
    switch(getGroupType()) {
        case GROUP_PUBLIC:
        default:
            return("(Public)");
            break;
        case GROUP_INVITE_ONLY:
            return("(Invite Only)");
            break;
        case GROUP_PRIVATE:
            return("(Private)");
            break;
    }
    return("**Unknown**");
}
//********************************************************************************
//* GetGroupTypeStr
//********************************************************************************
bstring displayPref(bstring name, bool set) {
    return(name + (set ? "^gon^x" : "^roff^x"));
}
bstring Group::getFlagsDisplay() {
    std::ostringstream oStr;
    oStr << displayPref("Group Experience Split: ", flagIsSet(GROUP_SPLIT_EXPERIENCE));
    oStr << ", ";
    oStr << displayPref("Split Gold: ", flagIsSet(GROUP_SPLIT_GOLD));
    oStr << ".";
    return(oStr.str());
}

//********************************************************************************
//* GetGroupList
//********************************************************************************
// Returns the group listing used for displaying to staff
bstring Server::getGroupList() {
    std::ostringstream oStr;
    int i=1;
    for(Group* group : groups) {
        oStr << i++ << ") " << group->getName() << " " << group->getGroupTypeStr() <<  std::endl << group;
    }
    return(oStr.str());
}
//********************************************************************************
//* GetGroupTypeStr
//********************************************************************************
// Returns the group listing used for displaying to group members
bstring Group::getGroupList(Creature* viewer) {
    int i = 0;
    std::ostringstream oStr;

    for(Creature* target : members) {
        if(!viewer->isStaff() && (target->pFlagIsSet(P_DM_INVIS) || (target->isEffected("incognito") && !viewer->inSameRoom(target))))
            continue;
        bool isPet = target->isPet();
        oStr << ++i << ") ";
        if(!viewer->pFlagIsSet(P_NO_EXTRA_COLOR) && viewer->isEffected("know-aura") && target->getGroupStatus() != GROUP_INVITED)
            oStr << target->alignColor();

        if(isPet)
            oStr << target->getMaster()->getName() << "'s " << target->getName();
        else
            oStr << target->getName();
        oStr << "^x";
        if(target == leader) {
            oStr << " (Leader)";
        } else if(target->getGroupStatus() == GROUP_INVITED) {
            oStr << " (Invited).\n";
            continue;
        }
        if( viewer->isCt() ||
                (isPet && !target->getMaster()->flagIsSet(P_NO_SHOW_STATS)) ||
                (!isPet && !target->pFlagIsSet(P_NO_SHOW_STATS)) ||
                (isPet && target->getMaster() == viewer) ||
                (!isPet && target == viewer))
        {
            oStr << " - " << (target->hp.getCur() < target->hp.getMax() && !viewer->pFlagIsSet(P_NO_EXTRA_COLOR) ? "^R" : "")
                 << std::setw(3) << target->hp.getCur() << "^x/" << std::setw(3) << target->hp.getMax()
                 << " Hp - " << std::setw(3) << target->mp.getCur() << "/" << std::setw(3)
                 << target->mp.getMax() << " Mp";

            if(!isPet) {
                if(target->isEffected("blindness"))
                    oStr << ", Blind";
                if(target->isEffected("drunkenness"))
                    oStr << ", Drunk";
                if(target->isEffected("confusion"))
                    oStr << ", Confused";
                if(target->isDiseased())
                    oStr << ", Diseased";
                if(target->isEffected("petrification"))
                    oStr << ", Petrified";
                if(target->isPoisoned())
                    oStr << ", Poisoned";
                if(target->isEffected("silence"))
                    oStr << ", Silenced";
                if(target->flagIsSet(P_SLEEPING))
                    oStr << ", Sleeping";
                else if(target->flagIsSet(P_UNCONSCIOUS))
                    oStr << ", Unconscious";
                if(target->isEffected("wounded"))
                    oStr << ", Wounded";
            }

            oStr << ".";
        }
        oStr << "\n";
    }
    return(oStr.str());
}

std::ostream& operator<<(std::ostream& out, const Group* group) {
    if(group)
        out << (*group);
    return(out);
}
std::ostream& operator<<(std::ostream& out, const Group& group) {
    int i = 0;
    for(Creature* crt : group.members) {
        out << "\t" << ++i << ") " << crt->getName();
        if(crt->getGroupStatus() == GROUP_LEADER)
            out << " (Leader)";
        else if(crt->getGroupStatus() == GROUP_INVITED)
            out << " (Invited)";
        out << std::endl;
    }
    return(out);
}
