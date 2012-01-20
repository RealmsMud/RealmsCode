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

// Mud includes
#include "mud.h"
#include "group.h"

// C++ Includes
#include <iomanip>


Group::Group(Creature* pLeader) {
    //if(pLeader.inGroup())
    //  throw new bstring("Error: Leader already in another group\n");
    add(pLeader);
    leader = pLeader;
    groupType = GROUP_DEFAULT;
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


bool Group::add(Creature* newMember) {
    Group* oldGroup = newMember->getGroup(false);
    if(oldGroup && oldGroup != this) {
        oldGroup->remove(newMember);
        oldGroup = NULL;
    }

    // No adding someone twice
    if(!oldGroup) {
        newMember->setGroup(this);
        members.push_back(newMember);
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
        if(this->getSize(false, false) == 1)
        	return(disband());

        // We've already checked for a disband, now check for a leadership change
        if(toRemove == leader) {
        	leader = this->getMember(1, false);

        	// Something's wrong here
        	if(!leader) {
        		std::cout << "Couldn't find a replacement leader.\n";
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
        crt->setGroup(NULL);
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
// Returns: The number of players in the group
int Group::getSize(bool countDmInvis, bool membersOnly) {
	int count=0;
	for(Creature* crt : members) {
		if((countDmInvis || !crt->pFlagIsSet(P_DM_INVIS)) && crt->isPlayer() && (crt->getGroupStatus() >= GROUP_MEMBER || ! membersOnly))
			count++;
	}
	return(count);

}

int Group::getNumInSameRoom(Creature* target) {
	int count=0;
	for(Creature* crt : members) {
		if(crt != target && target->inSameRoom(crt))
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
    return(NULL);
}
//********************************************************************************
//* getMember
//********************************************************************************
// Parameters: 	name 		- Name (possibly partial) of the match we're looking for
//			   	num			- Number in the list for a match
//				Searcher	- The creature doing the search (allows nulls)
//				includePets - Include pets in the search
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
	return(NULL);
}

GroupType Group::getGroupType() {
    return(groupType);
}
void Group::setGroupType(GroupType newType) {
	groupType = newType;
}
bool Group::setLeader(Creature* newLeader) {
    //if(newLeader->getGroup() != this)
    //  return(false);
	leader->setGroupStatus(GROUP_MEMBER);
	newLeader->setGroupStatus(GROUP_LEADER);
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
        if((!ignorePets || !crt->isPet()) && crt != ignore && crt->getGroupStatus() >= GROUP_MEMBER ) {
            crt->print("%s", msg.c_str());
        }
    }
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

bstring Group::getGroupList(Creature* viewer) {
    int i = 0;
    std::ostringstream oStr;

    oStr << getName() << " " << getGroupTypeStr() << ":\n";

    for(Creature* target : members) {
        bool isPet = target->isPet();
        if(!viewer->pFlagIsSet(P_NO_EXTRA_COLOR) && viewer->isEffected("know-aura") && target->getGroupStatus() != GROUP_INVITED)
            oStr << target->alignColor();
        oStr << ++i << ") ";
        if(isPet)
            oStr << target->getMaster()->getName() << "'s " << target->getName();
        else
            oStr << target->getName();

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
    if(group && newGroup != NULL) {
        std::cout << "Setting group for " << getName() << " but they already have a group." << std::endl;
    }

    group = newGroup;
}
void Creature::setGroupStatus(GroupStatus newStatus) {
    groupStatus = newStatus;
}

GroupStatus Creature::getGroupStatus() {
    return(groupStatus);
}

bool Creature::inSameGroup(Creature* target) {
	if(!target) return(false);
	return(getGroup() == target->getGroup());
}

Creature* Creature::getGroupLeader() {
	group = getGroup();
	if(!group) return(NULL);
	return(group->getLeader());
}


bool Server::registerGroup(Group* toRegister) {
    std::cout << "Registering " << toRegister->getName() << std::endl;
    groups.push_back(toRegister);
    return(true);
}

bool Server::unRegisterGroup(Group* toUnRegister) {
    std::cout << "Unregistering " << toUnRegister->getName() << std::endl;
    groups.remove(toUnRegister);
    return(true);
}

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
bstring Server::getGroupList() {
    std::ostringstream oStr;
    int i=1;
    for(Group* group : groups) {
        oStr << i++ << ") " << group->getName() << " " << group->getGroupTypeStr() <<  std::endl << group;
    }
    return(oStr.str());
}
