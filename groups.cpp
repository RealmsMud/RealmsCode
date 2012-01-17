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

#include "mud.h"
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


	Group* toJoin = toFollow->getGroup();
	if(toJoin) {
        // Check if in same group
        if(toJoin == player->getGroup()) {
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
	if(player->getGroup(false))
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
    // TODO: check if we have a group already
    if(announce)
        toJoin->sendToAll(bstring(getName()) + " has joined your group.\n");
    toJoin->add(this);
}
//********************************************************************************
//* CreateGroup
//********************************************************************************
// Creates a new group with the creature as the leader, and crt as a member
void Creature::createGroup(Creature* crt) {
    this->print("You have formed a group.\n");
    group = new Group(this);
    group->add(this);
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
//						addFollower
//*********************************************************************
// This function will make follower follow pCreature, pCreature must be a player
// follower must NOT be following anyone else

//
//int addFollower(Creature * pCreature, Creature *follower, int notify) {
//	ctag	*pp=0, *cp=0, *last=0, *cpnext=0;
//	Player* creature = pCreature->getPlayer(), *pFollower = follower->getPlayer();
//
//	ASSERTLOG( creature );
//	ASSERTLOG( follower->following == NULL );
//
//	follower->following = creature;
//
//	pp = new ctag;
//	if(!pp)
//		merror("follow", FATAL);
//
//	pp->crt = follower;
//	pp->next_tag = 0;
//
//	// creature = leader
//	// add 2: 1 for leader, 1 for new follower
//	int numFollowers = creature->numFollowers() + 2;
//	// Statistic for largest group
//	// call statistics.group(#) on everyone in the group
//	//creature->statistics.group(numFollowers);
//	//if(pFollower)
//	//    pFollower->statistics.group(numFollowers);
//
//
//	if(!creature->first_fol) {
//		creature->first_fol = pp;
//	} else {
//		// Add monsters to the start of the follow list, players to the end
//		if(!pFollower) {
//			pp->next_tag = creature->first_fol;
//			creature->first_fol = pp;
//
//			// skip over the creature we just added
//			cp = pp->next_tag;
//			while(cp) {
//				if(cp->crt->isPlayer())
//					cp->crt->getPlayer()->statistics.group(numFollowers);
//				cp = cp->next_tag;
//			}
//		} else {
//			cp = creature->first_fol;
//			while(cp) {
//				if(cp->crt->isPlayer())
//					cp->crt->getPlayer()->statistics.group(numFollowers);
//				last = cp;
//				cp = cp->next_tag;
//			}
//			last->next_tag = pp;
//		}
//	}
//
//	if(pFollower) {
//		pFollower->unmist();
//
//
//		pFollower->print("You start following %s.\n", creature->name);
//
//		if(!isDm(creature) && isStaff(creature))
//			log_immort(true, creature, "%s follows %s in room %s.\n", pFollower->name, creature->name,
//				pFollower->getRoom()->fullName().c_str());
//		else if(!isDm(pFollower))
//			log_immort(true, pFollower, "%s follows %s in room %s.\n", pFollower->name, creature->name,
//				pFollower->getRoom()->fullName().c_str());
//
//
//		if(!pFollower->flagIsSet(P_DM_INVIS) && !pFollower->flagIsSet(P_INCOGNITO) && !creature->isGagging(follower->name)) {
//			creature->print("%M starts following you.\n", pFollower);
//			broadcast(pFollower->getSock(), creature->getSock(), pFollower->getRoom(),
//				"%M follows %N.", pFollower, creature);
//		}
//		cp 	= pFollower->first_fol;
//		while(cp) {
//			cpnext = cp->next_tag; // No Guarantee cp will still be valid if we call
//								   // doStopFollowing
//			if(cp->crt->isPlayer()) {
//				if(!cp->crt->flagIsSet(P_DM_INVIS) && !cp->crt->flagIsSet(P_INCOGNITO))
//					follower->print("%s must also follow %s now to follow you.\n", cp->crt->name, creature->name);
//				cp->crt->print("You must follow %s now to follow %s.\n", creature->name, follower->name);
//				cp->crt->print("You stop following %s.\n", follower->name);
//				doStopFollowing(cp->crt, FALSE);
//			}
//			cp = cpnext;
//		}
//	}
//	return(0);
//}
//********************************************************************
//                      remFromGroup
//********************************************************************
// if player is in a group, they will be removed from it.

//void remFromGroup(Creature* player) {
//  ctag    *cp=0, *prev=0;
//  Creature *leader=0;
//
//  if(!player->following)
//          return;
//
//  leader = player->following;
//
//  cp = leader->first_fol;
//  if(cp->crt == player) {
//          leader->first_fol = cp->next_tag;
//          delete cp;
//  } else {
//          while(cp) {
//                  if(cp->crt == player) {
//                          prev->next_tag = cp->next_tag;
//                          delete cp;
//                          break;
//                  }
//                  prev = cp;
//                  cp = cp->next_tag;
//          }
//
//  }
//  player->following = 0;
//}
//*********************************************************************
//						doStopFollowing
//*********************************************************************
// Causes the passed creature to stop following whoever they are
// following, and if notify is TRUE, notify that person if they can
// can see the person

//int doStopFollowing(Creature *target, int notify) {
//	ctag		*cp, *prev;
//	Creature * following; // The person they are following
//	Player* pTarget = target->getPlayer(), *pFollowing=0;
//
//	if(target->following == NULL)
//		return(0);
//
//	following = target->following;
//	cp = following->first_fol;
//
//	if(cp->crt == target) {
//		// They are the first person following them, so get rid of them
//		following->first_fol = cp->next_tag;
//		delete cp;
//	} else {
//		while(cp) {
//			// Go through and find them
//			if(cp->crt == target) {
//				prev->next_tag = cp->next_tag;
//				delete cp;
//				break;
//			}
//			prev = cp;
//			cp = cp->next_tag;
//		}
//	}
//	target->following = 0;
//
//	if(pTarget && notify == TRUE) {
//		pTarget->print("You stop following %s.\n", following->name);
//		pFollowing = following->getPlayer();
//
//		if(!pTarget->flagIsSet(P_DM_INVIS) && !pTarget->flagIsSet(P_INCOGNITO) && pFollowing && !pFollowing->isGagging(pTarget->name))
//			pFollowing->print("%M stops following you.\n", pTarget);
//	}
//	return(1);
//}

bool Creature::removeFromGroup(bool announce) {
    if(group) {
        if(!pFlagIsSet(P_DM_INVIS) && !pFlagIsSet(P_INCOGNITO)) {
            if(groupStatus == GROUP_INVITED) {

            } else {
                group->sendToAll(getCrtStr(NULL, CAP) + " leaves the group.\n");
            }
        }
        if(groupStatus == GROUP_INVITED) {
            if(!pFlagIsSet(P_DM_INVIS) && !pFlagIsSet(P_INCOGNITO))
                group->sendToAll(getCrtStr(NULL, CAP) + " rejects the invitation to join your group.\n");
            // They're not in the group, so no group cleanup needed
            group = null;
        } else {
            if(!pFlagIsSet(P_DM_INVIS) && !pFlagIsSet(P_INCOGNITO))
                group->sendToAll(getCrtStr(NULL, CAP) + " leaves the group.\n", this);
            group->remove(this);
            group = null;
        }
        groupStatus = GROUP_NO_STATUS;
        return(true);
    }

    return(false);
}
//*********************************************************************
//						doLose
//*********************************************************************
// Causes pFollower to stop following pCreature

//int doLose(Creature* crt, Creature* follower, int notify) {
//	ASSERTLOG( crt != NULL );
//	ASSERTLOG( follower != NULL );
//	Player* pCreature = crt->getPlayer();
//	Player *pFollower = follower->getPlayer();
//
//	doStopFollowing(follower, FALSE);
//
//	if(pCreature && pFollower && notify == TRUE) {
//		pCreature->print("You lose %s.\n", pFollower->himHer());
//
//		if(pFollower->isWatching(pCreature->name))
//			pFollower->delWatching(pCreature->name);
//		if(pCreature->isWatching(pFollower->name))
//			pCreature->delWatching(pFollower->name);
//
//		if(!pCreature->flagIsSet(P_DM_INVIS) && !pFollower->flagIsSet(P_INCOGNITO)) {
//			pFollower->print("%M loses you.\n", pCreature);
//			broadcast(pCreature->getSock(), pFollower->getSock(), pCreature->getRoom(), "%M loses %N.", pCreature, pFollower);
//		}
//	}
//	return(1);
//}

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

	// Loop through group members and remove that person, do not allow them to remove pets
	lowercize(cmnd->str[1], 1);
	//target = findCreature(player, player->first_fol, cmnd);
	ctag* cp = player->first_fol;
	int match = 0;
	while(cp) {
		if(!player->canSee(cp->crt)) {
			cp = cp->next_tag;
			continue;
		}
		if(keyTxtEqual(cp->crt, cmnd->str[1])) {
			match++;
			if(match == cmnd->val[1]) {
				target = cp->crt;
				break;
			}
		}
		cp = cp->next_tag;
	}
	if(!target) {
		player->print("That person is not following you.\n");
		return(0);
	}

	if(target->following != player) {
		player->print("That person is not following you.\n");
		return(0);
	}
	if(target->isPet()) {
		player->print("You can't lose your own pet!\n");
		return(0);
	}

	doLose(player, target, TRUE);

	return(0);

}

//*********************************************************************
//						groupLine
//*********************************************************************
//
// this function is responsible for printing out a line of group member info
//

bstring groupLine(Creature* player, Creature* target) {
	bool	isPet = target->isPet();
	std::ostringstream oStr;

	if(!player->flagIsSet(P_NO_EXTRA_COLOR) && player->isEffected("know-aura"))
		oStr << target->alignColor();

	oStr << "  ";
	if(isPet)
		oStr << target->following->name << "'s " << target->name;
	else
		oStr << target->name;
	oStr << "^x";

	if(	player->isCt() ||
		(isPet && !target->following->flagIsSet(P_NO_SHOW_STATS)) ||
		(!isPet && !target->flagIsSet(P_NO_SHOW_STATS)) ||
		(isPet && target->following == player) ||
		(!isPet && target == player)
	) {
		oStr << " - " << (target->hp.getCur() < target->hp.getMax() && !player->flagIsSet(P_NO_EXTRA_COLOR) ? "^R" : "")
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
	return(oStr.str());
}


//*********************************************************************
//						showGroupMembers
//*********************************************************************

bstring showGroupMembers(Player* player, ctag* cp, int *num) {
	Creature* follower=0;
	std::ostringstream oStr;
	while(cp) {
		follower = cp->crt;
		cp = cp->next_tag;

		// we assume that, if they're not a player, they're a pet
		// and are following someone
		// sending true to canSee skips invis/mist checks
		if(!player->canSee(follower->isPlayer() ? follower : follower->following, true))
			continue;

		// print out the follower
		if(follower != player) {
			(*num)++;
			oStr << groupLine(player, follower);
		}

		if(follower->first_fol)
			oStr << showGroupMembers(player, follower->first_fol, num);

	}
	return(oStr.str());
}

//*********************************************************************
//						cmdGroup
//*********************************************************************
// This function allows you to see who is in a group or party of people
// who are following you.

int cmdGroup(Player* player, cmd* cmnd) {
	Creature *leader=0;
	int num=0;
	std::ostringstream oStr;

	player->clearFlag(P_AFK);

	if(!player->ableToDoCommand())
		return(0);

	if(player->following)
		leader = player->following;
	else
		leader = player;

	oStr << "People in your party:\n";

	// you can always see who you're following
	if(player != leader) {
		// print out the leader
		num++;
		oStr << groupLine(player, leader);
	}

	oStr << showGroupMembers(player, leader->first_fol, &num);

	if(!num)
		oStr << "  No one but you.\n";
	player->printColor("%s\n", oStr.str().c_str());
	return(0);
}




//********************************************************************
//						doFollow
//********************************************************************

void Player::doFollow() {
	ctag* cp=0;

	cp = first_fol;
	while(cp) {
		Player* pFollow = cp->crt->getPlayer();
		Monster* mFollow = cp->crt->getMonster();
		if(cp->crt->getRoom() != getRoom()) {
			if(pFollow) {
				pFollow->deleteFromRoom();
				pFollow->addToRoom(getRoom());
			} else {
				mFollow->deleteFromRoom();
				mFollow->addToRoom(getRoom());
			}
		}
		cp = cp->next_tag;
	}
}

//********************************************************************
//						doPetFollow
//********************************************************************

void Player::doPetFollow() {
	Monster *pet = getPet();

	if(pet && pet->getRoom() != getRoom()) {
		pet->deleteFromRoom();
		pet->addToRoom(getRoom());
	}
	if(alias_crt && alias_crt->getRoom() != getRoom()) {
		alias_crt->deleteFromRoom();
		alias_crt->addToRoom(getRoom());
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
