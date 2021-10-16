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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <cassert>             // for assert

#include "bstring.hpp"          // for bstring
#include "cmd.hpp"              // for cmd
#include "commands.hpp"         // for orderPet, cmdSocial
#include "config.hpp"           // for Config, SocialMap
#include "container.hpp"        // for Container, PlayerSet
#include "creatureStreams.hpp"  // for Streamable
#include "creatures.hpp"        // for Creature, Player
#include "flags.hpp"            // for P_AFK, P_SLEEPING
#include "global.hpp"           // for CAP
#include "proto.hpp"            // for socialHooks, actionShow
#include "socials.hpp"          // for SocialCommand


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

int cmdSocial(Creature* creature, cmd* cmnd) {
    Container* parent = creature->getParent();

    assert(parent);

    Player  *player=nullptr, *pTarget=nullptr;
    Creature* target=nullptr;

    player = creature->getAsPlayer();
    if(!creature->ableToDoCommand(cmnd))
        return(0);

    bstring str = cmnd->myCommand->getName();
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

        bstring toSelf = social->getSelfOnTarget();
        toSelf.Replace("*TARGET*", target->getCrtStr(creature).c_str());
        toSelf.Replace("*VICTIM*", target->getCrtStr(creature).c_str());
        *creature << toSelf << "\n";

        if(actionShow(pTarget, creature)) {
            bstring toTarget = social->getVictimOnTarget();
            toTarget.Replace("*A-HISHER*", creature->hisHer());
            toTarget.Replace("*A-HIMHER*", creature->himHer());
            toTarget.Replace("*A-HESHE*", creature->heShe());
            toTarget.Replace("*ACTOR*", creature->getCrtStr(target, CAP).c_str());

            *target << toTarget << "\n";
        }

        std::string_view toRoom = social->getRoomOnTarget();
        parent->doSocialEcho(toRoom, creature, target);

        socialHooks(creature, target, str);
    } else {
        // Social no target

        *creature << social->getSelfNoTarget() << "\n";

        std::string_view toRoom = social->getRoomNoTarget();

        if(!toRoom.empty() && parent) {
            parent->doSocialEcho(toRoom, creature, target);
            socialHooks(creature, str);
        }


    }

    return(0);
}


void Container::doSocialEcho(bstring str, const Creature* actor, const Creature* target) {
    if(str.empty() || !actor)
        return;
    str.Replace("*A-HISHER*", actor->hisHer());
    str.Replace("*A-HIMHER*", actor->himHer());
    str.Replace("*A-HESHE*", actor->heShe());

    for(Player* ply : players) {
        if(ply == actor || ply == target) continue;

        bstring out = str;
        out.Replace("*ACTOR*", actor->getCrtStr(ply, CAP).c_str());
        if(target) {
            out.Replace("*TARGET*", target->getCrtStr(ply).c_str());
            out.Replace("*VICTIM*", target->getCrtStr(ply).c_str());
        }
        *ply << out << "\n";
    }

}



int SocialCommand::execute(Creature* player, cmd* cmnd) const {
    return((fn)(player, cmnd));
}
