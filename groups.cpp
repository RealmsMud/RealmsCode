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
		player->print("Follow who?\n");
		return(0);
	}


	if(player->flagIsSet(P_SITTING)) {
		player->print("You need to stand first.\n");
		return(0);
	}

	player->unhide();
	lowercize(cmnd->str[1], 1);
	toFollow = player->getRoom()->findPlayer(player, cmnd);
	if(!toFollow) {
		player->print("No one here by that name.\n");
		return(0);
	}

	if(toFollow == player && !player->getGroup(false)) {
		player->print("You can't group with yourself.\n");
		return(0);
	}

	if(toFollow->flagIsSet(P_NO_FOLLOW) && !player->isCt() && toFollow != player) {
		player->print("%M does not welcome group members.\n", toFollow);
		return(0);
	}

	if(toFollow->isRefusing(player->name)) {
		player->print("%M doesn't allow you to group with %s right now.\n", toFollow, toFollow->himHer());
		return(0);
	}

	if(toFollow->isGagging(player->name)) {
		player->print("You start following %s.\n", toFollow->name);
		return(0);
	}

	if(toFollow->flagIsSet(P_MISTED) && !player->isCt()) {
        player->print("How can you group with a mist?\n");
        return(0);
    }


	Group* toJoin = toFollow->getGroup(false);
	if(toJoin && player->getGroupStatus() != GROUP_INVITED) {
        // Check if in same group
        if(toJoin == player->getGroup() ) {
            player->print("You can't. %s is in the same group as you!.\n", toFollow->upHeShe());
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
    	print("You join %s.\n", toJoin->getName().c_str());
        toJoin->sendToAll(bstring(getName()) + " has joined your group.\n", this);
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
//						numFollowers
//*********************************************************************

int Creature::numFollowers() {
	int num=0;
//	ctag *cp = first_fol;
//	while(cp) {
//		num += 1 + cp->crt->numFollowers();
//		cp = cp->next_tag;
//	}
	return(num);
}

//*********************************************************************
//						RemoveFromGroup
//*********************************************************************
// Removes the creature and all of their pets from the group

bool Creature::removeFromGroup(bool announce) {
    if(group) {
        if(groupStatus == GROUP_INVITED) {
            if(!pFlagIsSet(P_DM_INVIS) && !pFlagIsSet(P_INCOGNITO) && announce)
                group->sendToAll(getCrtStr(NULL, CAP) + " rejects the invitation to join your group.\n");
            print("You reject the invitation to join %s.\n", group->getName().c_str());
            group->remove(this);
            group = null;
        } else {
        	if(announce) {
        		if(!pFlagIsSet(P_DM_INVIS) && !pFlagIsSet(P_INCOGNITO))
        			group->sendToAll(getCrtStr(NULL, CAP) + " leaves the group.\n", this);
        		if(group->getLeader() == this)
        			print("You leave your group.\n");
        		else
        			print("You leave %s.\n", group->getName().c_str());
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
			player->print("You're not in a group.\n");
			return(0);
		}
		player->removeFromGroup(true);
		return(0);
	}

	player->unhide();

	Group* group = player->getGroup(true);
	if(player != group->getLeader()) {
	    player->print("You are not the group leader.\n");
	    return(0);
	}

	lowercize(cmnd->str[1], 1);
	target = group->getMember(cmnd->str[1], cmnd->val[1], player, false);

	if(!target) {
		player->print("That person is not following you.\n");
		return(0);
	}

	player->removeFromGroup(true);

	return(0);

}


int printGroupSyntax(Player* player) {
    player->printColor("Syntax: guild ^e<^xinvite^e>^x ^e<^cplayer name^e>\n");
    player->printColor("              ^e<^xkick^e>^x ^e<^cplayer name^e>\n");
    player->printColor("              ^e<^xaccept^e>\n");
    player->printColor("              ^e<^xreject^e>\n");
    player->printColor("              ^e<^xpromote^e>^x ^e<^cplayer name^e>^x\n");
    player->printColor("              ^e<^xleave^e>\n");
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

	    if(!strncmp(cmnd->str[1], "invite", len))       return(Group::invite(player, cmnd));
	    else if(!strncmp(cmnd->str[1], "accept", len) || !strncmp(cmnd->str[1], "join", len))  return(Group::join(player, cmnd));
	    else if(!strncmp(cmnd->str[1], "leave", len))   return(Group::leave(player, cmnd));
	    else if(!strncmp(cmnd->str[1], "disband", len)) return(Group::disband(player, cmnd));
	    else if(!strncmp(cmnd->str[1], "kick", len))    return(Group::kick(player, cmnd));
	    else if(!strncmp(cmnd->str[1], "promote", len)) return(Group::promote(player, cmnd));
	    else return(printGroupSyntax(player));
	}

	Group* group = player->getGroup(false);
    if(!group) {
        *player << "You are not in a group.\n";
        return(0);
    }
    if(player->getGroupStatus() == GROUP_INVITED) {
        *player << "You have been invited to join \"" << group->getName() << "\".\n";
        return(0);
    }
    //*player << group->getGroupList(player);
    player->printColor("%s\n", group->getGroupList(player).c_str());
	return(0);
}

int Group::invite(Player* player, cmd* cmnd) {
    Player* target = 0;

    if(cmnd->num < 3) {
        player->print("Invite who into your guild?\n");
        return(0);
    }

    lowercize(cmnd->str[2], 1);
    target = gServer->findPlayer(cmnd->str[2]);

    if(!target || !player->canSee(target) || target == player) {
        player->print("%s is not on.\n", cmnd->str[2]);
        return(0);
    }

    if(Move::tooFarAway(player, target, "invite to a group"))
        return(0);

    if(target->getGroup(false)) {
        if(target->getGroupStatus() == GROUP_INVITED)
            *player << target << " is already considering joining another group.\n";
        else
            *player << target << " is already in another group.\n";
        return(0);
    }


    Group* group = player->getGroup(false);

    if(!group) {
        group = new Group(player);
        player->setGroupStatus(GROUP_LEADER);
    }

    group->add(target);
    target->setGroup(group);
    target->setGroupStatus(GROUP_INVITED);

    *player << "You invite " << target << " to join your group.\n";
    *target << player << " invites you to join \"" << group->getName() << "\".\n";

    return(0);
}

int Group::join(Player* player, cmd *cmnd) {

    return(0);
}
int Group::reject(Player* player, cmd* cmnd) {

    return(0);
}
int Group::disband(Player* player, cmd* cmnd) {

    return(0);
}
int Group::promote(Player* player, cmd* cmnd) {

    return(0);
}
int Group::kick(Player* player, cmd* cmnd) {

    return(0);
}
int Group::leave(Player* player, cmd* cmnd) {

    return(0);
}
//********************************************************************
//						doFollow
//********************************************************************

void Player::doFollow() {
	Group* group = getGroup(true);
	if(getGroupStatus() == GROUP_LEADER && group) {
		for(Creature* crt : group->members) {
			if(crt->getRoom() != getRoom()) {
				Player* pFollow = crt->getPlayer();
				Monster* mFollow = crt->getMonster();
				if(pFollow) {
					pFollow->deleteFromRoom();
					pFollow->addToRoom(getRoom());
				} else {
					mFollow->deleteFromRoom();
					mFollow->addToRoom(getRoom());
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
		if(pet && pet->getRoom() != getRoom()) {
			pet->deleteFromRoom();
			pet->addToRoom(getRoom());
		}
		// TODO: Not sure if we need this check any more
		if(alias_crt && alias_crt->getRoom() != getRoom()) {
			alias_crt->deleteFromRoom();
			alias_crt->addToRoom(getRoom());
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
		getPlayer()->getIdle() < 120 &&
		// And we haven't abused group exp
		!flagIsSet(P_GROUP_EXP_ABUSE) &&
		// And we're not a DM invis person
		!flagIsSet(P_DM_INVIS) &&
		// And we are visible! (No more clerics/mages sitting invis leeching)
		!isInvisible() &&
		// No invis players
		!flagIsSet(P_HIDDEN) &&
		// No mists either
		!flagIsSet(P_MISTED) &&
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
