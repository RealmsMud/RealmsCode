/*
 * socials.cpp
 *   Functions for social(s)
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

#include <boost/algorithm/string/replace.hpp>  // for replace_all
#include <boost/iterator/iterator_traits.hpp>  // for iterator_value<>::type
#include <cassert>                             // for assert
#include <deque>                               // for _Deque_iterator
#include <functional>                          // for function
#include <set>                                 // for operator==, _Rb_tree_c...
#include <string>                              // for string, allocator, ope...
#include <string_view>                         // for string_view

#include "cmd.hpp"                             // for cmd
#include "commands.hpp"                        // for orderPet, cmdSocial
#include "config.hpp"                          // for Config, SocialSet
#include "creatureStreams.hpp"                 // for Streamable
#include "flags.hpp"                           // for P_AFK, P_SLEEPING
#include "global.hpp"                          // for CAP
#include "mudObjects/container.hpp"            // for Container, PlayerSet
#include "mudObjects/creatures.hpp"            // for Creature
#include "mudObjects/players.hpp"              // for Player
#include "proto.hpp"                           // for socialHooks, actionShow
#include "socials.hpp"                         // for SocialCommand
#include "structs.hpp"                         // for Command


void Config::clearSocials() {
    socials.clear();
}

bool SocialCommand::getWakeTarget() const {
    return(wakeTarget);
}
bool SocialCommand::getRudeWakeTarget() const {
    return(rudeWakeTarget);
}
bool SocialCommand::getWakeRoom() const {
    return(wakeRoom);
}

std::string_view SocialCommand::getSelfNoTarget() const  {
    return(selfNoTarget);
}
std::string_view SocialCommand::getRoomNoTarget() const {
    return(roomNoTarget);
}

std::string_view SocialCommand::getSelfOnTarget() const {
    return(selfOnTarget);
}
std::string_view SocialCommand::getRoomOnTarget() const {
    return(roomOnTarget);
}
std::string_view SocialCommand::getVictimOnTarget() const {
    return(victimOnTarget);
}

std::string_view SocialCommand::getSelfOnSelf() const {
    return(selfOnSelf);
}
std::string_view SocialCommand::getRoomOnSelf() const {
    return(roomOnSelf);
}

int cmdSocial(const std::shared_ptr<Creature>& creature, cmd* cmnd) {
    std::shared_ptr<Container> parent = creature->getParent();

    assert(parent);

    std::shared_ptr<Player> player=nullptr, pTarget=nullptr;
    std::shared_ptr<Creature> target=nullptr;

    player = creature->getAsPlayer();
    if(!creature->ableToDoCommand(cmnd))
        return(0);

    std::string str = cmnd->myCommand->getName();
    auto* social = dynamic_cast<const SocialCommand*>(cmnd->myCommand);

    if(!social)
        return(0);

    if(player) {
        creature->clearFlag(P_AFK);

        if(str != "sleep")
            player->unhide();

        // if they have a pet, they use orderPet instead
        if(str == "pet" && player->hasPet()) {
            return(orderPet(player, cmnd));
        }
    }

    if(cmnd->num == 2) {
        target = parent->findCreature(creature, cmnd->str[1], cmnd->val[1], true, true);
        if(target)
            pTarget = target->getAsPlayer();
        if( (!target || target == creature) )
        {
            *creature << "That is not here.\n";
            return(player ? 0 : 1);
        }
    }

    if(pTarget && pTarget->flagIsSet(P_SLEEPING)) {
        if(social->getRudeWakeTarget())
            pTarget->wake("You awaken suddenly!");
        else if(social->getWakeTarget())
            pTarget->wake("You wake up.");
    }
    if(social->getWakeRoom())
        parent->wake("You awaken suddenly!", true);

    if(target && !social->getSelfOnTarget().empty()) {
        // Social on Target

        std::string toSelf = std::string(social->getSelfOnTarget());
        boost::replace_all(toSelf,"*TARGET*", target->getCrtStr(creature));
        boost::replace_all(toSelf,"*VICTIM*", target->getCrtStr(creature));
        *creature << toSelf << "\n";

        if(actionShow(pTarget, creature)) {
            std::string toTarget = std::string(social->getVictimOnTarget());
            boost::replace_all(toTarget, "*A-HISHER*", creature->hisHer());
            boost::replace_all(toTarget, "*A-HIMHER*", creature->himHer());
            boost::replace_all(toTarget, "*A-HESHE*", creature->heShe());
            boost::replace_all(toTarget, "*A-UPHESHE*", creature->upHeShe());
            boost::replace_all(toTarget, "*ACTOR*", creature->getCrtStr(target, CAP));

            *target << toTarget << "\n";
        }

        std::string toRoom = std::string(social->getRoomOnTarget());
        parent->doSocialEcho(toRoom, creature, target);

        socialHooks(creature, target, str);
    } else {
        // Social no target

        *creature << social->getSelfNoTarget() << "\n";

        std::string toRoom = std::string(social->getRoomNoTarget());

        if(!toRoom.empty() && parent) {
            parent->doSocialEcho(toRoom, creature, target);
            socialHooks(creature, str);
        }


    }

    return(0);
}


void Container::doSocialEcho(std::string str, const std::shared_ptr<Creature> & actor, const std::shared_ptr<Creature> & target) {
    if(str.empty() || !actor)
        return;
    boost::replace_all(str, "*A-HISHER*", actor->hisHer());
    boost::replace_all(str, "*A-HIMHER*", actor->himHer());
    boost::replace_all(str, "*A-HESHE*", actor->heShe());
    boost::replace_all(str, "*A-UPHESHE*", actor->upHeShe());


    for(const auto& pIt: players) {
        if(auto ply = pIt.lock()) {
            if (ply == actor || ply == target) continue;

            std::string out = str;
            boost::replace_all(out, "*ACTOR*", actor->getCrtStr(ply, CAP));
            if (target) {
                boost::replace_all(out, "*TARGET*", target->getCrtStr(ply));
                boost::replace_all(out, "*VICTIM*", target->getCrtStr(ply));
            }
            *ply << out << "\n";
        }
    }

}



int SocialCommand::execute(const std::shared_ptr<Creature>& player, cmd* cmnd) const {
    return((fn)(player, cmnd));
}
