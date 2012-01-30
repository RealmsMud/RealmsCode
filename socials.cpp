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
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "mud.h"
#include "commands.h"
#include "socials.h"


void Config::clearSocials() {
    for(SocialMap::value_type p : socials) {
        if(p.second)
            delete p.second;
    }
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

const bstring& SocialCommand::getSelfNoTarget() const  {
    return(selfNoTarget);
}
const bstring& SocialCommand::getRoomNoTarget() const {
    return(roomNoTarget);
}

const bstring& SocialCommand::getSelfOnTarget() const {
    return(selfOnTarget);
}
const bstring& SocialCommand::getRoomOnTarget() const {
    return(roomOnTarget);
}
const bstring& SocialCommand::getVictimOnTarget() const {
    return(victimOnTarget);
}

const bstring& SocialCommand::getSelfOnSelf() const {
    return(selfOnSelf);
}
const bstring& SocialCommand::getRoomOnSelf() const {
    return(roomOnSelf);
}

int cmdSocial(Creature* creature, cmd* cmnd) {
    BaseRoom* room = creature->getRoom();

    assert(room);

    Player  *player=0, *pTarget=0;
    Creature* target=0;

    player = creature->getPlayer();
    if(!creature->ableToDoCommand(cmnd))
        return(0);

    bstring str = cmnd->myCommand->getName();
    SocialCommand* social = dynamic_cast<SocialCommand*>(cmnd->myCommand);

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
        target = room->findCreature(creature, cmnd->str[1], cmnd->val[1], true, true);
        if(target)
            pTarget = target->getPlayer();
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
        room->wake("You awaken suddenly!", true);

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

        bstring toRoom = social->getRoomOnTarget();
        room->doSocialEcho(toRoom, creature, target);

        socialHooks(creature, target, str);
    } else {
        // Social no target

        *creature << social->getSelfNoTarget() << "\n";

        bstring toRoom = social->getRoomNoTarget();

        if(!toRoom.empty() && room) {
            room->doSocialEcho(toRoom, creature, target);
            socialHooks(creature, str);
        }


    }

    return(0);
}


void BaseRoom::doSocialEcho(bstring str, const Creature* actor, const Creature* target) {
    if(str.empty() || !actor)
        return;
    str.Replace("*A-HISHER*", actor->hisHer());
    str.Replace("*A-HIMHER*", actor->himHer());
    str.Replace("*A-HESHE*", actor->heShe());

    ctag* cp;
    cp = first_ply;
    Player *ply;
    while(cp) {
        ply = cp->crt->getPlayer();
        cp = cp->next_tag;

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



int SocialCommand::execute(Creature* player, cmd* cmnd) {
    return((fn)(player, cmnd));
}
