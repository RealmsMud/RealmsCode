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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <algorithm>                 // for find
#include <iomanip>                   // for operator<<, setw
#include <list>                      // for operator==, list, _List_iterator
#include <ostream>                   // for operator<<, basic_ostream, ostri...
#include <string>                    // for operator<<, char_traits, string
#include <string_view>               // for string_view

#include "creatureStreams.hpp"       // for Streamable, ColorOff, ColorOn
#include "flags.hpp"                 // for P_DM_INVIS, P_NO_EXTRA_COLOR
#include "group.hpp"                 // for Group, CreatureList, GROUP_MEMBER
#include "mudObjects/creatures.hpp"  // for Creature, PetList
#include "mudObjects/monsters.hpp"   // for Monster
#include "proto.hpp"                 // for keyTxtEqual
#include "server.hpp"                // for Server, GroupList, gServer
#include "stats.hpp"                 // for Stat

//################################################################################
//# Group methods for groups
//################################################################################

Group::Group(const std::shared_ptr<Creature>& pLeader) {
    flags = 0;
    add(pLeader);
    leader = pLeader;
    groupType = GROUP_DEFAULT;
    if(pLeader->pFlagIsSet(P_XP_DIVIDE))
        setFlag(GROUP_SPLIT_EXPERIENCE);
    if(pLeader->pFlagIsSet(P_GOLD_SPLIT))
        setFlag(GROUP_SPLIT_GOLD);

    name = std::string(pLeader->getName()) + "'s group";
    description = "A group, lead by " + std::string(pLeader->getName());
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

bool Group::add(const std::weak_ptr<Creature>& newMember, bool addPets) {
    if(auto target = newMember.lock()) {
        Group *oldGroup = target->getGroup(false);
        if (oldGroup && oldGroup != this) {
            oldGroup->remove(target);
            oldGroup = nullptr;
        }

        // No adding someone twice
        if (!oldGroup) {
            target->setGroup(this);
            members.push_back(target);
        }
        if (addPets) {
            for (const auto &mons: target->pets) {
                add(mons);
            }
        }
        target->setGroupStatus(GROUP_MEMBER);
        return(true);
    } else {
        return false;
    }
}

//********************************************************************************
//* remove
//********************************************************************************
// Removes a creature from the group, adjusts leadership if necessary
// and disbands the group if necessary
//
// Returns: true  - The group was deleted and is no longer valid
//          false - The group still exists
bool Group::remove(const std::weak_ptr<Creature>& pToRemove) {
    const auto toRemove = pToRemove.lock();
    if(!toRemove) {
        return size() == 0;
    }
    const auto it = std::find_if(members.begin(), members.end(), [&toRemove](const std::weak_ptr<Creature>& ptr1) {
        return ptr1.lock() == toRemove;
    });

    if(it != members.end()) {
        toRemove->setGroup(nullptr);
        toRemove->setGroupStatus(GROUP_NO_STATUS);

        // Iterator is invalid now, do not try to access it after this
        members.erase(it);
        // Remove any pets this player had in the group
        for(const auto& mons : toRemove->pets) {
            if(remove(mons))
                return(true);
        }

        // See if the group should be disbanded
        if(this->getSize(false, false) <= 1)
            return(disband());

        // We've already checked for a disband, now check for a leadership change
        auto curLeader = leader.lock();
        if(toRemove == curLeader) {
            leader = this->getMember(1, false);

            // Something's wrong here
            auto newLeader = leader.lock();
            if(!newLeader) {
                std::clog << "Couldn't find a replacement leader.\n";
                return(disband());
            }

            newLeader->setGroupStatus(GROUP_LEADER);
            newLeader->print("You are now the group leader.\n");
            sendToAll(std::string(newLeader->getName()) + " is now the group leader.\n", newLeader);
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
    auto it = members.begin();
    while (it != members.end()) {
        if(auto crt = it->lock()) {
            crt->setGroup(nullptr);
            crt->setGroupStatus(GROUP_NO_STATUS);
        }
        it++;
    }
    members.clear();
}

//********************************************************************************
//* size
//********************************************************************************
// Returns the absolute size of the group
int Group::size() {
    int count=0;
    auto it = members.begin();
    while (it != members.end()) {
        if(auto crt = it->lock()) {
            if(crt->getGroupStatus() >= GROUP_MEMBER)
                count++;
            it++;
        } else {
            it = members.erase(it);
        }
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
    auto it = members.begin();
    while (it != members.end()) {
        if(auto crt = it->lock()) {
            if ((countDmInvis || !crt->pFlagIsSet(P_DM_INVIS)) && crt->isPlayer() && (crt->getGroupStatus() >= GROUP_MEMBER || !membersOnly))
                count++;
            it++;
        } else {
            it = members.erase(it);
        }
    }
    return(count);

}
//********************************************************************************
//* getNumInSameRoup
//********************************************************************************
// Returns the number of group members in the same room as the target
int Group::getNumInSameRoom(const std::shared_ptr<Creature>& target) {
    int count=0;
    auto it = members.begin();
    while (it != members.end()) {
        if(auto crt = it->lock()) {
            if (crt != target && target->inSameRoom(crt))
                count++;
            it++;
        } else {
            it = members.erase(it);
        }
    }
    return(count);
}
//********************************************************************************
//* getNumPlyInSameRoup
//********************************************************************************
// Returns the number of group members (players) in the same room as the target

int Group::getNumPlyInSameRoom(const std::shared_ptr<Creature>& target) {
    int count=0;
    auto it = members.begin();
    while (it != members.end()) {
        if(auto crt = it->lock()) {
            if(crt != target && crt->isPlayer() && target->inSameRoom(crt))
                count++;
            it++;
        } else {
            it = members.erase(it);
        }
    }
    return(count);
}

//********************************************************************************
//* getMember
//********************************************************************************
// Parameters: countDmInvis - Should we count DM invis players or not?
// Returns: The chosen players in the group

std::shared_ptr<Creature> Group::getMember(int num, bool countDmInvis) {
    int count=0;
    auto it = members.begin();
    while (it != members.end()) {
        if(auto crt = it->lock()) {
            if ((countDmInvis || !crt->pFlagIsSet(P_DM_INVIS)) && crt->isPlayer() && crt->getGroupStatus() >= GROUP_MEMBER)
                count++;
            if (count == num)
                return (crt);
            it++;
        } else {
            it = members.erase(it);
        }
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
std::shared_ptr<Creature> Group::getMember(const std::string& pName, int num, const std::shared_ptr<Creature>& searcher, bool includePets) {
    int match = 0;
    auto it = members.begin();
    while (it != members.end()) {
        if(auto crt = it->lock()) {
            it++;
            if(!crt->isPlayer() && !includePets) continue;
            if(crt->getGroupStatus() < GROUP_MEMBER) continue;
            if(!searcher || !searcher->canSee(crt)) continue;
            if(keyTxtEqual(crt, pName.c_str())) {
                if(++match == num) {
                    return(crt);
                }
            }
        } else {
            it = members.erase(it);
        }
    }
    return(nullptr);
}


//********************************************************************************
//* inGroup
//********************************************************************************
// Returns: Is the target in this group?
bool Group::inGroup(std::shared_ptr<Creature> target) {
    const auto it = std::find_if(members.begin(), members.end(), [&target](const std::weak_ptr<Creature>& ptr1) {
        return ptr1.lock() == target;
    });

    if(it == members.end())
        return(false);
    else
        return(true);
}

//********************************************************************************
//* SendToAll
//********************************************************************************
// Parameters: sendToInvited - Are invited members counted as in the group or not?
// Send msg to everyone in the group except ignore
void Group::sendToAll(std::string_view msg, const std::shared_ptr<Creature>& ignore, bool sendToInvited, bool gtargetChange) {
    auto it = members.begin();
    while (it != members.end()) {
        if(auto crt = it->lock()) {
            if (!crt->isPet() && crt != ignore && 
                (sendToInvited || crt->getGroupStatus() >= GROUP_MEMBER) &&
                     !(gtargetChange && crt->flagIsSet(P_NO_GROUP_TARGET_MSG))) { 
                *crt << ColorOn << msg << ColorOff;
            }
            it++;
        } else {
            it = members.erase(it);
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
void Group::setName(std::string_view newName) {
    // Test validity of name here
    name = newName;
}

//********************************************************************************
//* setLeader
//********************************************************************************
bool Group::setLeader(const std::shared_ptr<Creature>& newLeader) {
    //if(newLeader->getGroup() != this)
    //  return(false);
    if(auto oldLeader = leader.lock()) {
        oldLeader->setGroupStatus(GROUP_MEMBER);
    }
    newLeader->setGroupStatus(GROUP_LEADER);
    leader = newLeader;

    return(true);
}

//********************************************************************************
//* setDescription
//********************************************************************************
void Group::setDescription(std::string_view newDescription) {
    // Test validity of description here
    description = newDescription;
}

//********************************************************************************
//* getGroupType
//********************************************************************************
GroupType Group::getGroupType() const {
    return(groupType);
}

//********************************************************************************
//* getLeader
//********************************************************************************
std::shared_ptr<Creature> Group::getLeader() const {
    return(leader.lock());
}
//********************************************************************************
//* getName
//********************************************************************************
const std::string& Group::getName() const {
    return(name);
}
//********************************************************************************
//* getDescription
//********************************************************************************
const std::string&Group::getDescription() const {
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
bool Creature::inSameGroup(const std::shared_ptr<Creature>& target) {
    if(!target) return(false);
    return(getGroup() == target->getGroup());
}
//********************************************************************************
//* GetGroupLeader
//********************************************************************************
std::shared_ptr<Creature> Creature::getGroupLeader() {
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

bool Group::flagIsSet(int flag) const {
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

std::string Group::getGroupTypeStr() const {
    switch(getGroupType()) {
        case GROUP_PUBLIC:
        default:
            return("(Public)");
        case GROUP_INVITE_ONLY:
            return("(Invite Only)");
        case GROUP_PRIVATE:
            return("(Private)");
    }
}

//********************************************************************************
//* GetGroupTypeStr
//********************************************************************************
std::string displayPref(const std::string &name, bool set) {
    return(name + (set ? "^gon^x" : "^roff^x"));
}
std::string Group::getFlagsDisplay() const {
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
std::string Server::getGroupList() {
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
std::string Group::getGroupList(const std::shared_ptr<Creature>& viewer) {
    int i = 0;
    std::ostringstream oStr;

    auto it = members.begin();
    auto curLeader = leader.lock();
    while (it != members.end()) {
        auto crt = it->lock();
        if(!crt) {
            it = members.erase(it);
            continue;
        }
        it++;

        if(!viewer->isStaff() && (crt->pFlagIsSet(P_DM_INVIS) || (crt->isEffected("incognito") && !viewer->inSameRoom(crt))))
            continue;
        bool isPet = crt->isPet();
        oStr << ++i << ") ";
        if(!viewer->pFlagIsSet(P_NO_EXTRA_COLOR) && viewer->isEffected("know-aura") && crt->getGroupStatus() != GROUP_INVITED)
            oStr << crt->alignColor();

        if(isPet)
            oStr << crt->getMaster()->getName() << "'s " << crt->getName();
        else
            oStr << crt->getName();
        oStr << "^x";
        if(crt == curLeader) {
            oStr << " (Leader)";
        } else if(crt->getGroupStatus() == GROUP_INVITED) {
            oStr << " (Invited).\n";
            continue;
        }
        if(viewer->isCt() ||
           (isPet && !crt->getMaster()->flagIsSet(P_NO_SHOW_STATS)) ||
           (!isPet && !crt->pFlagIsSet(P_NO_SHOW_STATS)) ||
           (isPet && crt->getMaster() == viewer) ||
           (!isPet && crt == viewer))
        {
            oStr << " - " << (crt->hp.getCur() < crt->hp.getMax() && !viewer->pFlagIsSet(P_NO_EXTRA_COLOR) ? "^R" : "")
                 << std::setw(3) << crt->hp.getCur() << "^x/" << std::setw(3) << crt->hp.getMax()
                 << " Hp - " << std::setw(3) << crt->mp.getCur() << "/" << std::setw(3)
                 << crt->mp.getMax() << " Mp";

            if(!isPet) {
                if(crt->isEffected("blindness"))
                    oStr << ", Blind";
                if(crt->isEffected("drunkenness"))
                    oStr << ", Drunk";
                if(crt->isEffected("confusion"))
                    oStr << ", Confused";
                if(crt->isDiseased())
                    oStr << ", Diseased";
                if(crt->isEffected("petrification"))
                    oStr << ", Petrified";
                if(crt->isPoisoned())
                    oStr << ", Poisoned";
                if(crt->isEffected("silence"))
                    oStr << ", Silenced";
                if(crt->flagIsSet(P_SLEEPING))
                    oStr << ", Sleeping";
                else if(crt->flagIsSet(P_UNCONSCIOUS))
                    oStr << ", Unconscious";
                if(crt->isEffected("wounded"))
                    oStr << ", Wounded";
            }

            oStr << " - ^gTargeting: ";
            if(auto target = crt->myTarget.lock())
                oStr << "^y" << target->getCName();
            else
                oStr << "^yNo-one!";
            oStr << "^x";
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
    for(const auto& member : group.members) {
        auto crt = member.lock();
        if(!crt) continue;

        out << "\t" << ++i << ") " << crt->getName();
        if(crt->getGroupStatus() == GROUP_LEADER)
            out << " (Leader)";
        else if(crt->getGroupStatus() == GROUP_INVITED)
            out << " (Invited)";
        out << std::endl;
    }
    return(out);
}
