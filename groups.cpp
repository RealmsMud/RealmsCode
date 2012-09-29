/*
 * groups.cpp
 *	 Functions dealing with player groups.
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
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

// Mud includes
#include "mud.h"
#include "move.h"
#include "commands.h"

// C++ Includes
#include <sstream>
#include <iomanip>
#include <locale>


//*********************************************************************
//						cmdFollow
//*********************************************************************
// This command allows a player (or a monster) to follow another player.
// Follow loops are not allowed; i.e. you cannot follow someone who is
// following you.

int cmdFollow(Player* player, cmd* cmnd) {
	Player*	toFollow=0;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(cmnd->num < 2) {
		*player << "Follow whom?\n";
		return(0);
	}


	if(player->flagIsSet(P_SITTING)) {
		*player << "You need to stand first.\n";
		return(0);
	}

	player->unhide();
	lowercize(cmnd->str[1], 1);
	toFollow = player->getParent()->findPlayer(player, cmnd);
	if(!toFollow) {
		*player << "No one here by that name.\n";
		return(0);
	}

	if(toFollow == player && !player->getGroup(false)) {
		*player << "You can't group with yourself.\n";
		return(0);
	}

	if(toFollow->flagIsSet(P_NO_FOLLOW) && !player->isCt() && toFollow != player) {
		*player << toFollow << " does not welcome group members.\n";
		return(0);
	}

	if(toFollow->isRefusing(player->name)) {
		player->print("%M doesn't allow you to group with %s right now.\n", toFollow, toFollow->himHer());
		return(0);
	}

	if(toFollow->isGagging(player->name)) {
		*player << "You start following " << toFollow->getName() << ".\n";
		return(0);
	}

	if(toFollow->isEffected("mist") && !player->isCt()) {
        player->print("How can you group with a mist?\n");
        return(0);
    }


	Group* toJoin = toFollow->getGroup(false);
	if(toJoin && player->getGroupStatus() != GROUP_INVITED) {
        // Check if in same group
        if(toJoin == player->getGroup() ) {
            player->print("You can't. %s is in the same group as you!\n", toFollow->upHeShe());
            return(0);
        }
        if(toJoin->getGroupType() != GROUP_PUBLIC) {
            player->print("%s's group is invite only.\n", toFollow->getName());
            return(0);
        }
	}

	if(player->flagIsSet(P_NO_FOLLOW)) {
		player->print("You welcome group members again.\n");
		player->clearFlag(P_NO_FOLLOW);
	}
	if((toJoin && player->getGroupStatus() != GROUP_INVITED) || player == toFollow)
	    player->removeFromGroup(true);

	if(player == toFollow)
		return(0);

	if(toJoin)
	    player->addToGroup(toJoin);
	else
	    toFollow->createGroup(player);



	return(0);

}
//********************************************************************************
//* AddToGroup
//********************************************************************************
// Adds the creature to the group, and announces if requested
void Creature::addToGroup(Group* toJoin, bool announce) {
    toJoin->add(this);
    if(announce) {
    	*this << ColorOn << "^gYou join \"" << toJoin->getName() << "\".\n" << ColorOff;
        toJoin->sendToAll(bstring("^g") + getName() + " has joined your group.\n", this);
        broadcast(getSock(), getRoomParent(), "%M joins the group \"%s\".", this, toJoin->getName().c_str());
    }

}
//********************************************************************************
//* CreateGroup
//********************************************************************************
// Creates a new group with the creature as the leader, and crt as a member
void Creature::createGroup(Creature* crt) {
    this->print("You have formed a group.\n");

    // Note: Group leader is added to this group in the constructor
    group = new Group(this);
    groupStatus = GROUP_LEADER;

    crt->addToGroup(group);
}

//*********************************************************************
//						RemoveFromGroup
//*********************************************************************
// Removes the creature and all of their pets from the group

bool Creature::removeFromGroup(bool announce) {
    if(group) {
        if(groupStatus == GROUP_INVITED) {
        	if(announce) {
        		if(!pFlagIsSet(P_DM_INVIS) && !isEffected("incognito"))
        			group->sendToAll(getCrtStr(NULL, CAP) + " rejects the invitation to join your group.\n");
            	*this << ColorOn << "^gYou reject the invitation to join \"" << group->getName() << "\".\n^x" << ColorOff;
        	}
            group->remove(this);
            group = null;
        } else {
        	if(announce) {
        		if(!pFlagIsSet(P_DM_INVIS) && !isEffected("incognito"))
        			group->sendToAll(getCrtStr(NULL, CAP) + " leaves the group.\n", this);
        		if(group->getLeader() == this)
        			*this << ColorOn << "^gYou leave your group.^x\n" << ColorOff;
        		else
        			*this << ColorOn << "^gYou leave \"" << group->getName() << "\".\n^x" << ColorOff;
        	}
            group->remove(this);
            group = null;
        }
        groupStatus = GROUP_NO_STATUS;
        return(true);
    }

    return(false);
}

//*********************************************************************
//						cmdLose
//*********************************************************************
// This function allows a player to lose another player who might be
// following them. When successful, that player will no longer be
// following.

int cmdLose(Player* player, cmd* cmnd) {
	Creature* target=0;
	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(cmnd->num == 1) {

		if(!player->getGroup(false)) {
			*player << "You're not in a group.\n";
			return(0);
		}
		player->removeFromGroup(true);
		return(0);
	}

	player->unhide();

	Group* group = player->getGroup(true);

	if(!group) {
	    *player << "You are not in a group.\n";
	    return(0);
	}
	if(player != group->getLeader()) {
	    *player << "You are not the group leader.\n";
	    return(0);
	}

	lowercize(cmnd->str[1], 1);
	target = group->getMember(cmnd->str[1], cmnd->val[1], player, false);

	if(!target) {
		*player << "That person is not following you.\n";
		return(0);
	}

	player->removeFromGroup(true);

	return(0);

}


int printGroupSyntax(Player* player) {
    player->printColor("Syntax: group ^e<^xleave^e>\n");
    if(player->getGroupStatus() == GROUP_INVITED) {
    	player->printColor("              ^e<^xreject^e>\n");
    	player->printColor("              ^e<^xaccept^e>\n");
    }
    if(player->getGroupStatus() == GROUP_LEADER) {
        player->printColor("              ^e<^xpromote^e>^x ^e<^cplayer name^e>^x\n");
		player->printColor("              ^e<^xkick^e>^x ^e<^cplayer name^e>\n");
		player->printColor("              ^e<^xname^e>^x ^e<^cgroup name^e>\n");
		player->printColor("              ^e<^xtype^e>^x ^e<^cpublic/private/invite only^e>\n");
		player->printColor("              ^e<^xset^e>^x ^e<^csplit/xpsplit^e>\n");
		player->printColor("              ^e<^xclear^e>^x ^e<^csplit/xpsplit^e>\n");
		player->printColor("              ^e<^xdisband^e>^x\n");
    }
    if(player->getGroupStatus() == GROUP_LEADER
    		|| (player->getGroupStatus() == GROUP_MEMBER && player->getGroup(true) && player->getGroup(true)->getGroupType() < GROUP_PRIVATE) )
    {
		player->printColor("              ^e<^xinvite^e>^x ^e<^cplayer name^e>\n");
    }
    return(0);
}

//*********************************************************************
//						cmdGroup
//*********************************************************************
// This function allows you to see who is in a group or party of people
// who are following you.

int cmdGroup(Player* player, cmd* cmnd) {
	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);


	if(cmnd->num >= 2) {
	    int len = strlen(cmnd->str[1]);

	    if(!strncasecmp(cmnd->str[1], "invite", len))       return(Group::invite(player, cmnd));
	    else if(!strncasecmp(cmnd->str[1], "accept", len) || !strncasecmp(cmnd->str[1], "join", len))     return(Group::join(player, cmnd));
	    else if(!strncasecmp(cmnd->str[1], "leave", len)  || !strncasecmp(cmnd->str[1], "reject",len))    return(Group::leave(player, cmnd));
	    else if(!strncasecmp(cmnd->str[1], "disband", len)) return(Group::disband(player, cmnd));
	    else if(!strncasecmp(cmnd->str[1], "kick", len))    return(Group::kick(player, cmnd));
	    else if(!strncasecmp(cmnd->str[1], "promote", len)) return(Group::promote(player, cmnd));
	    else if(!strncasecmp(cmnd->str[1], "name", len)) 	return(Group::rename(player, cmnd));
	    else if(!strncasecmp(cmnd->str[1], "type", len)) 	return(Group::type(player, cmnd));
	    else if(!strncasecmp(cmnd->str[1], "set", len))     return(Group::set(player, cmnd, true));
	    else if(!strncasecmp(cmnd->str[1], "clear", len))   return(Group::set(player, cmnd, false));
	    else return(printGroupSyntax(player));
	}

	Group* group = player->getGroup(false);
    if(!group) {
        *player << "You are not in a group.\n";
        return(0);
    }
    if(player->getGroupStatus() == GROUP_INVITED) {
        *player << "You have been invited to join \"" << group->getName() << "\".\nTo accept, type <group accept>; To reject type <group reject>.\n";
        return(0);
    }
    *player << group->getName() << " " << group->getGroupTypeStr() << ":\n";
    *player << ColorOn << group->getFlagsDisplay() << "\n" << ColorOff;

    *player << ColorOn << group->getGroupList(player) << ColorOff;

	return(0);
}

int Group::invite(Player* player, cmd* cmnd) {
    Player* target = 0;

    if(cmnd->num < 3) {
        *player << "Invite who into your group?\n";
        return(0);
    }

    lowercize(cmnd->str[2], 1);
    target = gServer->findPlayer(cmnd->str[2]);

    if(!target || !player->canSee(target) || target == player) {
        *player << "That player is not logged on.\n";
        return(0);
    }

    if(Move::tooFarAway(player, target, "invite to a group"))
        return(0);

    if(target->getGroup(false)) {
        if(target->getGroupStatus() == GROUP_INVITED) {
            if(target->getGroup(false) == player->getGroup(false))
                *player << target << " is already considering joining your group.\n";
            else
                *player << target << " is already considering joining another group.\n";
        }
        else
            *player << target << " is already in another group.\n";
        return(0);
    }


    Group* group = player->getGroup(false);
    if(group) {
    	if(group->getGroupType() == GROUP_PRIVATE && player->getGroupStatus() != GROUP_LEADER) {
    		*player << "You are not the group leader of \"" << group->getName() << "\".\n";
    		return(0);
    	}
    	if(player->getGroupStatus() < GROUP_MEMBER) {
    		*player << "Reject your current group invitation before you try to start a group!\n";
    		return(0);
    	}
    }

    if(!group) {
        group = new Group(player);
        player->setGroupStatus(GROUP_LEADER);
    }

    group->add(target, false);
    target->setGroupStatus(GROUP_INVITED);

    *player << ColorOn << "^gYou invite " << target << " to join your group.\n" << ColorOff;
    *target << ColorOn << "^g" << player << " invites you to join \"" << group->getName() << "\".\n" << ColorOff;

    return(0);
}

// Accept an invitation and join a group
int Group::join(Player* player, cmd *cmnd)  {
	if(player->getGroupStatus() != GROUP_INVITED) {
		*player << "You have not been invited to join any groups.\n";
		return(0);
	}
	Group* toJoin = player->getGroup(false);
	if(!toJoin) {
		// Shouldn't happen
		*player << "You have not been invited to join any groups.\n";
		return(0);
	}
	player->addToGroup(toJoin, true);
    return(0);
}
int Group::reject(Player* player, cmd* cmnd) {
	if(player->getGroupStatus() != GROUP_INVITED) {
		*player << "You have not been invited to join any groups.\n";
		return(0);
	}
	Group* toReject = player->getGroup(false);
	if(!toReject) {
		// Shouldn't happen
		*player << "You have not been invited to join any groups.\n";
		return(0);
	}
	player->removeFromGroup(true);
    return(0);
}
int Group::disband(Player* player, cmd* cmnd) {
	Group* toDisband = player->getGroup(true);
	if(!toDisband) {
		*player << "You are not in a group.\n";
		return(0);
	}
	if(player->getGroupStatus() != GROUP_LEADER) {
		*player << "You are not the group leader of \"" << toDisband->getName() << "\".\n";
		return(0);
	}
	*player << "You disband \"" << toDisband->getName() << "\".\n";
	toDisband->sendToAll(bstring(player->getName()) + " disbands the group.\n", player);
	toDisband->disband();

    return(0);
}
int Group::promote(Player* player, cmd* cmnd) {
	Group* group = player->getGroup(true);
	if(!group) {
		*player << "You are not in a group.\n";
		return(0);
	}
	if(player->getGroupStatus() != GROUP_LEADER) {
		*player << "You are not the group leader of \"" << group->getName() << "\".\n";
		return(0);
	}

    Player* target = 0;

    if(cmnd->num < 3) {
        *player << "Who would you like to promote to leader?\n";
        return(0);
    }

    lowercize(cmnd->str[2], 1);
    target = gServer->findPlayer(cmnd->str[2]);

    if(!target || !player->canSee(target) || target == player) {

        *player << "That player is not logged on.\n";
        return(0);
    }

    if(target->getGroup(true) != group) {
    	*player << target->getName() << " is not in your group!\n";
    	return(0);
    }
    group->setLeader(target);

    *player << ColorOn << "^gYou promote " << target->getName() << " to group leader\nYou are now a member of \"" << group->getName() << "\".\n" << ColorOff;
	group->sendToAll(bstring("^g") + player->getName() + " promotes " + target->getName() + " to group leader.^x\n", player);

	*target << ColorOn << "^gYou are now the group leader of \"" << group->getName() << "\".\n^x" << ColorOff;


    return(0);
}
int Group::kick(Player* player, cmd* cmnd) {
	Group* group = player->getGroup(true);
	if(!group) {
		*player << "You are not in a group.\n";
		return(0);
	}
	if(group->getGroupType() == GROUP_PRIVATE && player->getGroupStatus() != GROUP_LEADER) {
		*player << "You are not the group leader of \"" << group->getName() << "\".\n";
		return(0);
	}

    Player* target = 0;

    if(cmnd->num < 3) {
        *player << "Who would you like to kick from your group?\n";
        return(0);
    }

    lowercize(cmnd->str[2], 1);
    target = gServer->findPlayer(cmnd->str[2]);

    if(!target || !player->canSee(target) || target == player) {

        *player << "That player is not logged on.\n";
        return(0);
    }

    // We can also remove invitations from people by "kicking" them
    if(target->getGroup(false) != group) {
    	*player << target->getName() << " is not in your group!\n";
    	return(0);
    }
    if(target->getGroupStatus() == GROUP_INVITED) {
    	*player << "You rescind the group invitation from " << target->getName() << ".\n";
    	group->sendToAll(bstring(player->getName()) + " rescinds the invitation for " + target->getName() + " to join the group.\n", player);
    	*target << player << " rescinds your invitation to join \"" << group->getName() << "\".\n";
    	target->removeFromGroup(false);
    }
    else {
    	if(player->getGroupStatus() != GROUP_LEADER) {
    		*player << "You can't kick people out of the group!\n";
    		return(0);
    	} else {
			*player << ColorOn << "^gYou kick " << target->getName() << " out of your group.^x\n" << ColorOff;
			group->sendToAll(bstring("^g") + player->getName() + " kicks " + target->getName() + " out of the group.^x\n", player);
			target->removeFromGroup(true);
    	}
    }
    return(0);
}
int Group::leave(Player* player, cmd* cmnd) {
	if(!player->getGroup(false)) {
		*player << "You're not in a group.\n";
		return(0);
	}
	if(!strncmp(cmnd->str[1], "reject", strlen(cmnd->str[1])) && player->getGroupStatus() != GROUP_INVITED) {
	    *player << "You have no group invitations to reject.\n";
	    return(0);
	}
	player->removeFromGroup(true);

    return(0);
}
int Group::rename(Player* player, cmd* cmnd) {
	Group* group = player->getGroup(true);
	if(!group) {
		*player << "You are not in a group.\n";
		return(0);
	}
	if(player->getGroupStatus() != GROUP_LEADER) {
		*player << "You are not the group leader of \"" << group->getName() << "\".\n";
		return(0);
	}

	if(cmnd->num < 3) {
		*player << "What would you like to name your group?\n";
		return(0);
	}

	bstring newName = getFullstrText(cmnd->fullstr, 2);
	if(newName.empty()) {
		*player << "What would you like to name your group?\n";
		return(0);
	}
	group->setName(newName);
	*player << ColorOn << "^gYou rename your group to \"" << newName << "\".\n^x" << ColorOff;
	group->sendToAll(bstring("^g") + player->getName() + " renames the group to \"" + newName + "\".\n^x", player, true);

    return(0);
}
int Group::type(Player* player, cmd* cmnd) {
	Group* group = player->getGroup(true);
	const char *errorMsg = "What would you like to switch your group to? (Public, Private, Invite Only)?\n";
	if(!group) {
		*player << "You are not in a group.\n";
		return(0);
	}
	if(player->getGroupStatus() != GROUP_LEADER) {
		*player << "You are not the group leader of \"" << group->getName() << "\".\n";
		return(0);
	}

	if(cmnd->num < 3) {
		*player << errorMsg;
		return(0);
	}

	bstring newName = getFullstrText(cmnd->fullstr, 2);
	if(newName.empty()) {
	    *player << errorMsg;
		return(0);
	}
	int len = newName.length();
	const char *str = newName.c_str();
	if(len >= 2) {
		if(!strncasecmp(str, "public", len)) {
			group->setGroupType(GROUP_PUBLIC);
			*player << ColorOn << "^gYou change the group type to Public.\n" << ColorOff;
			group->sendToAll(bstring("^g") + player->getName() + " changes the group to Public.\n^x", player);
			return(0);
		} else if(!strncasecmp(str, "private", len)) {
			group->setGroupType(GROUP_PRIVATE);
			*player << ColorOn << "^gYou change the group type to Private.^x\n" << ColorOff;
			group->sendToAll(bstring("^g") + player->getName() + " changes the group to Private.^x\n", player);
			return(0);
		}
	}
	if(!strncasecmp(str, "invite only", len) || !strncmp(str, "inviteonly", len)) {
		group->setGroupType(GROUP_INVITE_ONLY);
		*player << ColorOn << "^gYou change the group type to Invite Only.^x\n" << ColorOff;
		group->sendToAll(bstring("^g") + player->getName() + " changes the group to Invite Only.\n^x", player);
		return(0);
	}
	*player << errorMsg;
    return(0);
}

int Group::set(Player* player, cmd* cmnd, bool set) {
    Group* group = player->getGroup(true);
    const char* errorMsg;
    if(set)
        errorMsg = "What group flag would you like to set? (Currently available: Split, XpSplit).\n";
    else
        errorMsg = "What group flag would you like to clear? (Currently available: Split, XpSplit).\n";

    if(!group) {
        *player << "You are not in a group.\n";
        return(0);
    }
    if(player->getGroupStatus() != GROUP_LEADER) {
        *player << "You are not the group leader of \"" << group->getName() << "\".\n";
        return(0);
    }

    if(cmnd->num < 3) {
        *player << errorMsg;
        return(0);
    }

    bstring newName = getFullstrText(cmnd->fullstr, 2);
    if(newName.empty()) {
        *player << errorMsg;
        return(0);
    }
    int len = newName.length();
    const char *str = newName.c_str();
    if(!strncasecmp(str, "split", len)) {
        if(set) {
            group->setFlag(GROUP_SPLIT_GOLD);
            group->sendToAll("^gGold will now be split with the group.^x\n");
        } else {
            group->clearFlag(GROUP_SPLIT_GOLD);
            group->sendToAll("^gGold will no longer be split with the group.^x\n");
        }
        return(0);
    } else if(!strncasecmp(str, "xpsplit", len)) {
        if(set) {
            group->setFlag(GROUP_SPLIT_EXPERIENCE);
            group->sendToAll("^gGroup experience split enabled.^x\n");
        } else {
            group->clearFlag(GROUP_SPLIT_EXPERIENCE);
            group->sendToAll("^gGroup experience split disabled.^x\n");
        }
        return(0);
    }

    *player << errorMsg;
    return(0);
}
//********************************************************************
//						doFollow
//********************************************************************

void Player::doFollow(BaseRoom* oldRoom) {
	Group* group = getGroup(true);
	if(getGroupStatus() == GROUP_LEADER && group) {
		for(Creature* crt : group->members) {
			if(crt->getRoomParent() == oldRoom) {
				Player* pFollow = crt->getAsPlayer();
				Monster* mFollow = crt->getAsMonster();
				if(pFollow) {
					pFollow->deleteFromRoom();
					pFollow->addToRoom(getRoomParent());
				} else {
					mFollow->deleteFromRoom();
					mFollow->addToRoom(getRoomParent());
				}
			}
		}
	}
	doPetFollow();
}

//********************************************************************
//						doPetFollow
//********************************************************************

void Player::doPetFollow() {
	for(Monster*pet : pets ){
		if(pet && pet->getRoomParent() != getRoomParent()) {
			pet->deleteFromRoom();
			pet->addToRoom(getRoomParent());
		}
		// TODO: Not sure if we need this check any more
		if(alias_crt && alias_crt->getRoomParent() != getRoomParent()) {
			alias_crt->deleteFromRoom();
			alias_crt->addToRoom(getRoomParent());
		}
	}
}

//*********************************************************************
//						getsGroupExperience
//*********************************************************************

bool Creature::getsGroupExperience(Monster* target) {
	// We can get exp if we're a player...
	return(isPlayer() &&
		// And idle less than 2 minutes
		getAsPlayer()->getIdle() < 120 &&
		// And we haven't abused group exp
		!flagIsSet(P_GROUP_EXP_ABUSE) &&
		// And we're not a DM invis person
		!flagIsSet(P_DM_INVIS) &&
		// And we are visible! (No more clerics/mages sitting invis leeching)
		!isInvisible() &&
		// No invis players
		!flagIsSet(P_HIDDEN) &&
		// No mists either
		!isEffected("mist") &&
		// no self-declared AFK people
		!flagIsSet(P_AFK) &&
		// no unconscious people
		!flagIsSet(P_UNCONSCIOUS) &&
		// no statues
		!isEffected("petrification") &&
		// and they're on the enemy list
		target->isEnemy(this)
	);
}
