/*
 * groups.cpp
 *   Functions dealing with player groups.
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

#include <strings.h>                 // for strncasecmp
#include <cstring>                   // for strlen, strncmp
#include <list>                      // for operator==, _List_iterator, list
#include <string>                    // for string, operator+, allocator

#include "cmd.hpp"                   // for cmd
#include "commands.hpp"              // for getFullstrText, cmdFollow, cmdGroup
#include "creatureStreams.hpp"       // for Streamable, ColorOff, ColorOn
#include "flags.hpp"                 // for P_AFK, P_DM_INVIS, P_NO_FOLLOW
#include "global.hpp"                // for CAP
#include "group.hpp"                 // for Group, GROUP_LEADER, GROUP_INVITED
#include "move.hpp"                  // for tooFarAway
#include "mudObjects/container.hpp"  // for Container
#include "mudObjects/creatures.hpp"  // for Creature, PetList
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for lowercize, broadcast
#include "server.hpp"                // for Server, gServer



//*********************************************************************
//                      cmdFollow
//*********************************************************************
// This command allows a player (or a monster) to follow another player.
// Follow loops are not allowed; i.e. you cannot follow someone who is
// following you.

int cmdFollow(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> toFollow=nullptr;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(cmnd->num < 2) {
        *player << "Follow whom?\n";
        return(0);
    }


    if(player->flagIsSet(P_SITTING))
        player->stand();

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

    if(toFollow->isRefusing(player->getName())) {
        player->print("%M doesn't allow you to group with %s right now.\n", toFollow.get(), toFollow->himHer());
        return(0);
    }

    if(toFollow->isGagging(player->getName())) {
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
            player->print("%s's group is invite only.\n", toFollow->getCName());
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
    toJoin->add(Containable::downcasted_shared_from_this<Creature>());
    if(announce) {

        *this << ColorOn << "^gYou join \"" << toJoin->getName() << "\".\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "%M joins group: \"%s\"", this, toJoin->getName().c_str());
        toJoin->sendToAll(std::string("^g") + "<Group> " + getName() + " has joined the group.\n", Containable::downcasted_shared_from_this<Creature>());
        
    }

}
//********************************************************************************
//* CreateGroup
//********************************************************************************
// Creates a new group with the creature as the leader, and crt as a member
void Creature::createGroup(const std::shared_ptr<Creature>& crt) {
    this->print("You have formed a group.\n");

    // Note: Group leader is added to this group in the constructor
    group = new Group(Containable::downcasted_shared_from_this<Creature>());
    groupStatus = GROUP_LEADER;

    crt->addToGroup(group);

}

//*********************************************************************
//                      RemoveFromGroup
//*********************************************************************
// Removes the creature and all of their pets from the group

bool Creature::removeFromGroup(bool announce) {
    if(group) {
        auto cThis = Containable::downcasted_shared_from_this<Creature>();
        if(groupStatus == GROUP_INVITED) {
            if(announce) {
                if(!pFlagIsSet(P_DM_INVIS) && !isEffected("incognito"))
                    group->sendToAll(getCrtStr(nullptr, CAP) + " rejects the invitation to join your group.\n");
                *this << ColorOn << "^gYou reject the invitation to join \"" << group->getName() << "\".\n^x" << ColorOff;
            }
            group->remove(cThis);
            group = nullptr;
        } else {
            if(announce) {
                if(!pFlagIsSet(P_DM_INVIS) && !isEffected("incognito"))
                    group->sendToAll(std::string("^g") + "<Group> " + getCrtStr(nullptr, CAP) + " leaves the group.\n", cThis);
                if(group->getLeader() == cThis)
                    *this << ColorOn << "^gYou leave your group.^x\n" << ColorOff;
                else
                    *this << ColorOn << "^gYou leave \"" << group->getName() << "\".\n^x" << ColorOff;
            }
            group->remove(cThis);
            group = nullptr;
        }
        groupStatus = GROUP_NO_STATUS;
        return(true);
    }

    return(false);
}

//*********************************************************************
//                      cmdLose
//*********************************************************************
// This function allows a player to lose another player who might be
// following them. When successful, that player will no longer be
// following.

int cmdLose(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> target=nullptr;
    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
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


int printGroupSyntax(const std::shared_ptr<Player>& player) {
    player->printColor("Syntax: group ^e<^xleave^e>\n");
    if(player->getGroupStatus() == GROUP_INVITED) {
        player->printColor("              ^e<^xreject^e>\n");
        player->printColor("              ^e<^xaccept^e>\n");
    }
    if(player->getGroupStatus() == GROUP_LEADER) {
        player->printColor("              ^e<^xpromote^e>^x ^e<^cplayer name^e>^x\n");
        player->printColor("              ^e<^xtarget^e>^x ^e<^ctarget name^e>|^e<^c-c^e>^x\n");
        player->printColor("              ^e<^xmtarget^e>^x ^e<^cgroup member^e> ^e<^ctarget name^e>|^e<^c-c^e>^x\n");
        player->printColor("              ^e<^xkick^e>^x ^e<^cplayer name^e>\n");
        player->printColor("              ^e<^xname/rename^e>^x ^e<^cgroup name|rename^e>\n");
        player->printColor("              ^e<^xtype^e>^x ^e<^cpublic/private/invite only^e>\n");
        player->printColor("              ^e<^xset/clear^e>^x ^e<^csplit|xpsplit|autotarget|lgtignore^e>\n");
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
//                      cmdGroup
//*********************************************************************
// This function allows you to see who is in a group or party of people
// who are following you.

int cmdGroup(const std::shared_ptr<Player>& player, cmd* cmnd) {
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
        else if(!strncasecmp(cmnd->str[1], "target", len))  return(Group::target(player, cmnd));
        else if(!strncasecmp(cmnd->str[1], "mtarget", len)) return(Group::mtarget(player, cmnd));
        else if(!strncasecmp(cmnd->str[1], "name", len))    return(Group::rename(player, cmnd));
        else if(!strncasecmp(cmnd->str[1], "rename", len))  return(Group::rename(player, cmnd));
        else if(!strncasecmp(cmnd->str[1], "type", len))    return(Group::type(player, cmnd));
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
        *player << ColorOn << "You have been invited to join \"" << group->getName() << "\".\nTo accept, type: '^ygroup accept^g'; To reject, type: '^ygroup reject^g'^x\n" << ColorOff;
        return(0);
    }
    *player << group->getName() << " " << group->getGroupTypeStr() << ":\n";
    *player << ColorOn << group->getFlagsDisplay() << "\n" << ColorOff;

    *player << ColorOn << group->getGroupList(player) << ColorOff;

    return(0);
}

int Group::invite(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target = nullptr;

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
            *player << ColorOn << "^gThis group is currently set to private. Only the group leader can invite new members.^x\n";
            return(0);
        }
        if(player->getGroupStatus() < GROUP_MEMBER) {
            *player << ColorOn << "^gYou must reject your current group invitation before you try to start a group.^x\n" << ColorOff;
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
int Group::join(const std::shared_ptr<Player>& player, cmd *cmnd)  {
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
int Group::reject(const std::shared_ptr<Player>& player, cmd* cmnd) {
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
int Group::disband(const std::shared_ptr<Player>& player, cmd* cmnd) {
    Group* toDisband = player->getGroup(true);
    if(!toDisband) {
        *player << "You are not in a group.\n";
        return(0);
    }
    if(player->getGroupStatus() != GROUP_LEADER) {
        *player << "Only the group leader can disband the group.\n";
        return(0);
    }
    *player << "You disband the group.\n";

    toDisband->sendToAll(std::string("^g") + "<GroupLeader> The group has been disbanded.\n", player);
    toDisband->disband();

    return(0);
}

void Group::clearTargets() {
    for(auto it = members.begin() ; it != members.end() ; it++) {
        if(auto gMember = it->lock()) {
            if(!gMember->isPlayer() || !gMember->inSameRoom(getLeader()) || (gMember->isStaff() && gMember != getLeader()))
                continue;
            if(gMember == getLeader() && (flagIsSet(LEADER_IGNORE_GTARGET) || flagIsSet(GROUP_AUTOTARGET)))
                continue;
            if(flagIsSet(GROUP_AUTOTARGET) && gMember->inCombat())
                continue;
            if(gMember != getLeader())
                    *gMember << ColorOn << "^g" << (flagIsSet(GROUP_AUTOTARGET)?"<GroupAutoTarget>":"<GroupLeader>") 
                                << " Your target has been cleared.^x\n" << ColorOff;

             gMember->clearTarget();
        }
    }
    return;
}

void Group::setTargets(const std::shared_ptr<Creature>& target, int ordinalNumber) {
    if (!target)
        return;

    std::string numString = (ordinalNumber > 1 ? " (" + std::to_string(ordinalNumber) + ")" : "");

    for(auto it = members.begin() ; it != members.end() ; it++) {
        if(auto gMember = it->lock()) {
            if(!gMember->isPlayer() || !gMember->inSameRoom(getLeader()) || (gMember->isStaff() && gMember != getLeader()))
                continue;
            if(gMember == getLeader() && (flagIsSet(LEADER_IGNORE_GTARGET) || flagIsSet(GROUP_AUTOTARGET)))
                continue;
            if(flagIsSet(GROUP_AUTOTARGET) && gMember->inCombat())
                continue;
            if (gMember != getLeader()) 
                *gMember << ColorOn << "^g" << (flagIsSet(GROUP_AUTOTARGET)?"<GroupAutoTarget>":"<GroupLeader>") 
                                << " You are now targeting: ^y" << target->getCName() << numString << "^x\n" << ColorOff;
            gMember->addTarget(target,true);
        }
    }

return;
}

int Group::target(const std::shared_ptr<Player>& player, cmd* cmnd) {
    Group* group = player->getGroup(true);
    std::shared_ptr<Creature> target=nullptr;
    
    if(!group) {
        *player << "You are not in a group.\n";
        return(0);
    }
    if(player->getGroupStatus() != GROUP_LEADER) {
        *player << "You must be the group leader to set a group target.\n";
        return(0);
    }

    if(cmnd->num < 3) {
        *player << "You must pick a target.\n";
        return(0);
    }

    if (std::string(cmnd->str[2]) == "-c") {
        *player << ColorOn << "^gAll group member targets cleared." << (group->flagIsSet(LEADER_IGNORE_GTARGET)?" Your target was not cleared.":"") << "^x\n" << ColorOff;    
        group->clearTargets();
        return(0);
    }


    cmnd->str[2][0] = up(cmnd->str[2][0]);
    target = player->getRoomParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

    if (!target) {
        *player << "That target is not here.\n";
        return(0);
    }

    //std::string numString = (cmnd->val[2] > 1 ? " (" + std::to_string(cmnd->val[2]) + ")" : "");

    *player << ColorOn << "^gSetting group member targets to: ^y" << (cmnd->val[2]>1?(getOrdinal(cmnd->val[2])+" "):"") << target->getCName() << "'^x\n" << ColorOff;

    group->setTargets(target, cmnd->val[2]);

    return(0);
}

int Group::mtarget(const std::shared_ptr<Player>& player, cmd* cmnd) {
    Group* group = player->getGroup(true);
    std::shared_ptr<Player> gMember=nullptr;
    std::shared_ptr<Creature> target=nullptr;

    if(!group) {
        *player << "You are not in a group.\n";
        return(0);
    }

    if(player->getGroupStatus() != GROUP_LEADER) {
        *player << "You must be the group leader to set member targets.\n";
        return(0);
    }

    if(cmnd->num < 4) {
        *player << ColorOn << "^ySyntax: group mtarget (group member) (target|-c)^x\n" << ColorOff;
        return(0);
    }
    
    lowercize(cmnd->str[2], 1);
    gMember = gServer->findPlayer(cmnd->str[2]);

    if(!gMember || !player->canSee(gMember)) {

        *player << "That player is not logged on.\n";
        return(0);
    }

    if(player == gMember) {
        *player << "Use 'target' to set your own target, dummy.\n";
        return(0);
    }

    if(gMember->getGroup(true) != group) {
        *player << gMember->getName() << " is not in your group.\n";
        return(0);
    }

    if(!player->inSameRoom(gMember)) {
        *player << "You must be in the same room as " << gMember << " to set " << gMember->hisHer() << " target.\n";
        return(0);
    }

    if(gMember->isStaff() && !player->isCt()) {
        *player << "You are unable to set " << gMember << "'s target. The gods will not allow it.\n";
        return(0);
    }

    if (std::string(cmnd->str[3]) == "-c") {
        if (gMember->getTarget()) {
            *player << ColorOn << "^gYou clear " << gMember << "'s target. ^g(Last target: ^y'" << gMember->getTarget()->getCName() << "'^g)^x\n" << ColorOff;
            *gMember << ColorOn << "^g<GroupLeader> Your target has been cleared.\n" << ColorOff;
            gMember->clearTarget();
        }
        else
            *player << ColorOn << "^g" << gMember << " has no target to clear.^x\n";

        return(0);
    }

    cmnd->str[3][0] = up(cmnd->str[3][0]);
    target = player->getRoomParent()->findCreature(player, cmnd->str[3], cmnd->val[3], false);

    if (!target) {
        *player << "That target is not here.\n";
        return(0);
    }

    std::string numString = (cmnd->val[3] > 1 ? " (" + std::to_string(cmnd->val[3]) + ")" : "");

    *player << ColorOn << "^gSetting " << gMember << "'s target to: '^y" << target->getCName() << numString << "' ^g(Last target: ^y'" 
                                << (gMember->getAsCreature()->getTarget() ? gMember->getTarget()->getCName():"No-one!") << "'^g)^x\n" << ColorOff;
    *gMember << ColorOn << "^g<GroupLeader> Your target has been reset to: '^y" << target->getCName() << numString << "'^x\n" << ColorOff;
    gMember->addTarget(target,true);

    return(0);    
}

int Group::promote(const std::shared_ptr<Player>& player, cmd* cmnd) {
    Group* group = player->getGroup(true);
    if(!group) {
        *player << "You are not in a group.\n";
        return(0);
    }
    if(player->getGroupStatus() != GROUP_LEADER) {
        *player << "Only the group leader can promote someone to group leader.\n";
        return(0);
    }

    std::shared_ptr<Player> target = nullptr;

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
    group->sendToAll(std::string("^g") + "<GroupLeader> " + target->getName() + " has been promoted to group leader.^x\n", player);

    *target << ColorOn << "^gYou are now the group leader of \"" << group->getName() << "\".\n^x" << ColorOff;


    return(0);
}
int Group::kick(const std::shared_ptr<Player>& player, cmd* cmnd) {
    Group* group = player->getGroup(true);
    if(!group) {
        *player << "You are not in a group.\n";
        return(0);
    }
    if(group->getGroupType() == GROUP_PRIVATE && player->getGroupStatus() != GROUP_LEADER) {
        *player << "You are not the group leader of \"" << group->getName() << "\".\n";
        return(0);
    }

    std::shared_ptr<Player> target = nullptr;

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
        group->sendToAll(std::string(player->getName()) + "<Group> Group invite has been rescinded for " + target->getName() + " to join the group.\n", player);
        *target << player << " rescinds your invitation to join \"" << group->getName() << "\".\n";
        target->removeFromGroup(false);
    }
    else {
        if(player->getGroupStatus() != GROUP_LEADER) {
            *player << "Only the group leader can kick people from the group.\n";
            return(0);
        } else {
            *player << ColorOn << "^gYou kick " << target->getName() << " from the group.^x\n" << ColorOff;
            group->sendToAll(std::string("^g") + "<GroupLeader> " + target->getName() + " was kicked from the group.^x\n", player);
            target->removeFromGroup(true);
        }
    }
    return(0);
}
int Group::leave(const std::shared_ptr<Player>& player, cmd* cmnd) {
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
int Group::rename(const std::shared_ptr<Player>& player, cmd* cmnd) {
    Group* group = player->getGroup(true);
    if(!group) {
        *player << "You are not in a group.\n";
        return(0);
    }
    if(player->getGroupStatus() != GROUP_LEADER) {
        *player << "Only the group leader can rename the group.\n";
        return(0);
    }

    if(cmnd->num < 3) {
        *player << "What would you like to name your group?\n";
        return(0);
    }

    std::string newName = getFullstrText(cmnd->fullstr, 2);
    if(newName.empty()) {
        *player << "What would you like to name your group?\n";
        return(0);
    }
    group->setName(newName);
    *player << ColorOn << "^gYou rename the group to: \"" << newName << "\".\n^x" << ColorOff;
    group->sendToAll("^g<GroupLeader> Group has been renamed to: \"" + newName + "\"^x\n", player, true);

    return(0);
}
int Group::type(const std::shared_ptr<Player>& player, cmd* cmnd) {
    Group* group = player->getGroup(true);
    const char *errorMsg = "What would you like to switch your group to? (Public, Private, Invite Only)?\n";
    if(!group) {
        *player << "You are not in a group.\n";
        return(0);
    }
    if(player->getGroupStatus() != GROUP_LEADER) {
        *player << "Only the group leader can set the group type.\n";
        return(0);
    }

    if(cmnd->num < 3) {
        *player << errorMsg;
        return(0);
    }

    std::string newName = getFullstrText(cmnd->fullstr, 2);
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
            group->sendToAll(std::string("^g") + "<GroupLeader> Group type changed to: Public.\n^x", player);
            return(0);
        } else if(!strncasecmp(str, "private", len)) {
            group->setGroupType(GROUP_PRIVATE);
            *player << ColorOn << "^gYou change the group type to Private.^x\n" << ColorOff;
            group->sendToAll(std::string("^g") + "<GroupLeader> Group type changed to: Private.^x\n", player);
            return(0);
        }
    }
    if(!strncasecmp(str, "invite only", len) || !strncmp(str, "inviteonly", len)) {
        group->setGroupType(GROUP_INVITE_ONLY);
        *player << ColorOn << "^gYou change the group type to Invite Only.^x\n" << ColorOff;
        group->sendToAll(std::string("^g") + "<GroupLeader> Group type changed to: Invite Only.\n^x", player);
        return(0);
    }
    *player << errorMsg;
    return(0);
}

int Group::set(const std::shared_ptr<Player>& player, cmd* cmnd, bool set) {
    Group* group = player->getGroup(true);
    const char* errorMsg;
    if(set)
        errorMsg = "What group preference would you like to set? (split, xpsplit, autotarget, lgtignore)\n";
    else
        errorMsg = "What group preference would you like to clear? (split, xpsplit, autotarget, lgtignore)\n";

    if(!group) {
        *player << "You are not in a group.\n";
        return(0);
    }
    if(player->getGroupStatus() != GROUP_LEADER) {
        *player << "Only the group leader can set the group preferences.\n";
        return(0);
    }

    if(cmnd->num < 3) {
        *player << errorMsg;
        return(0);
    }

    std::string newName = getFullstrText(cmnd->fullstr, 2);
    if(newName.empty()) {
        *player << errorMsg;
        return(0);
    }
    int len = newName.length();
    const char *str = newName.c_str();
    if(!strncasecmp(str, "split", len)) {
        if(set) {
            group->setFlag(GROUP_SPLIT_GOLD);
            group->sendToAll("^g<GroupLeader> Gold will now be split with the group.^x\n");
        } else {
            group->clearFlag(GROUP_SPLIT_GOLD);
            group->sendToAll("^g<GroupLeader> Gold will no longer be split with the group.^x\n");
        }
        return(0);
    } else if(!strncasecmp(str, "xpsplit", len)) {
        if(set) {
            group->setFlag(GROUP_SPLIT_EXPERIENCE);
            group->sendToAll("^g<GroupLeader> Group experience split enabled.^x\n");
        } else {
            group->clearFlag(GROUP_SPLIT_EXPERIENCE);
            group->sendToAll("^g<GroupLeader> Group experience split disabled.^x\n");
        }
        return(0);
    } else if(!strncasecmp(str, "lgtignore", len)) {
        if(set) {
            group->setFlag(LEADER_IGNORE_GTARGET);
            *player << ColorOn << "^gLeader group target ignore: enabled.^x\n" << ColorOff;
            if(group->flagIsSet(GROUP_AUTOTARGET)) {
                group->clearFlag(GROUP_AUTOTARGET);
                group->sendToAll("^g<GroupLeader> Group autotarget: disabled.^x\n");
            }
        } else {
            group->clearFlag(LEADER_IGNORE_GTARGET);
            *player << ColorOn << "^gLeader group target ignore: disabled.^x\n" << ColorOff;
        }
        return(0);
    } else if(!strncasecmp(str, "autotarget", len)) {
        if(set) {
            group->setFlag(GROUP_AUTOTARGET);
            group->sendToAll("^g<GroupLeader> Group autotarget: enabled.^x\n");
            if(group->flagIsSet(LEADER_IGNORE_GTARGET)) {
                group->clearFlag(LEADER_IGNORE_GTARGET);
                *player << ColorOn << "^gLeader group target ignore: disabled.^x\n" << ColorOff;
            }
        } else {
            group->clearFlag(GROUP_AUTOTARGET);
            group->sendToAll("^g<GroupLeader> Group autotarget: disabled.^x\n");
        }
        return(0);
    }

    *player << errorMsg;
    return(0);
}
//********************************************************************
//                      doFollow
//********************************************************************

void Player::doFollow(const std::shared_ptr<BaseRoom>& oldRoom) {
    Group* group = getGroup(true);
    if(getGroupStatus() == GROUP_LEADER && group) {
        auto it = group->members.begin();
        while (it != group->members.end()) {
            auto crt = it->lock();
            if(!crt) {
                it = group->members.erase(it);
                continue;
            }
            it++;
            if(crt->getRoomParent() == oldRoom) {
                std::shared_ptr<Player> pFollow = crt->getAsPlayer();
                std::shared_ptr<Monster>  mFollow = crt->getAsMonster();
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
//                      doPetFollow
//********************************************************************

void Player::doPetFollow() {
    for(const auto& pet : pets ){
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
//                      getsGroupExperience
//*********************************************************************

bool Creature::getsGroupExperience(const std::shared_ptr<Monster>&  target) {
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
        target->isEnemy(Containable::downcasted_shared_from_this<Creature>())
    );
}
