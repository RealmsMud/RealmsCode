/*
 * CreatureSongs.cpp
 *   Creature Song Functions
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
#include <boost/algorithm/string/predicate.hpp>  // for iequals
#include <list>                                  // for operator==, _List_it...
#include <set>                                   // for operator==, _Rb_tree...
#include <string>                                // for basic_string, string

#include "effects.hpp"                           // for EffectInfo
#include "group.hpp"                             // for CreatureList, Group
#include "mud.hpp"                               // for LT_SONG_PLAYED
#include "mudObjects/container.hpp"              // for PlayerSet
#include "mudObjects/creatures.hpp"              // for Creature
#include "mudObjects/players.hpp"                // for Player
#include "mudObjects/rooms.hpp"                  // for BaseRoom
#include "songs.hpp"                             // for Song

class MudObject;

bool Creature::isPlaying() const {
    return(playing != nullptr);
}

const Song* Creature::getPlaying() {
    return(playing);
}

bool Creature::setPlaying(const Song* newSong, bool echo) {
    bool wasPlaying = false;
    if(isPlaying()) {
        stopPlaying(echo);
        wasPlaying = true;
    }

    playing = newSong;

    return(wasPlaying);
}

bool Creature::stopPlaying(bool echo) {
    if(!isPlaying())
        return(false);

    if(echo) {
        print("You stop playing \"%s\"\n", getPlaying()->getName().c_str());
        getRoomParent()->print(getSock(), "%M stops playing %s\n", this, getPlaying()->getName().c_str());
    }
    playing = nullptr;
    return(true);
}

bool Creature::pulseSong(long t) {
    if(!isPlaying())
        return(false);

    if(getLTLeft(LT_SONG_PLAYED, t))
        return(false);

    if(!playing)
        return(false);

    print("Pulsing song: %s\n", playing->getName().c_str());

    const auto& targetType = playing->getTargetType();

    if(boost::iequals(playing->getType(), "effect")) {
        // Group effects affect the singer as well
        if(boost::iequals(targetType, "self") || boost::iequals(targetType, "group")) {
            addEffect(playing->getEffect(), -2, -2, Containable::downcasted_shared_from_this<Creature>())->setDuration(playing->getDuration());
        }
        if(boost::iequals(targetType, "group") && (getGroup() != nullptr)) {
            Group* myGroup = getGroup();
            auto it = myGroup->members.begin();
            while(it != myGroup->members.end()) {
                if(auto crt = (*it).lock()) {
                    it++;
                    if (inSameRoom(crt))
                        crt->addEffect(playing->getEffect(), -2, -2, Containable::downcasted_shared_from_this<Creature>())->setDuration(playing->getDuration());
                } else {
                    it = myGroup->members.erase(it);
                }
            }
        }
        if(boost::iequals(targetType, "room")) {
            for(const auto& ply: getRoomParent()->players) {
                if(getAsPlayer() && ply->getAsPlayer() && getAsPlayer()->isDueling(ply->getName()))
                    continue;
                ply->addEffect(playing->getEffect(), -2, -2, Containable::downcasted_shared_from_this<Creature>())->setDuration(playing->getDuration());
            }
        }
    } else if(playing->getType() == "script") {
        print("Running script for song \"%s\"\n", playing->getName().c_str());
        std::shared_ptr<MudObject>target = nullptr;

        // Find the target here, if any
        if(boost::iequals(targetType, "target")) {
            if(hasAttackableTarget())
                target = getTarget();
        }

        playing->runScript(Containable::downcasted_shared_from_this<Creature>(), target);
    }


    setLastTime(LT_SONG_PLAYED, t, playing->getDelay());
    return(true);
}