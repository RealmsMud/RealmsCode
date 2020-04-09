/*
 * builder.cpp
 *   New routines for handling builder commands and involving building.
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
#include "commands.hpp"
#include "creatures.hpp"
#include "dm.hpp"
#include "mud.hpp"
#include "rooms.hpp"
#include "server.hpp"
#include "socket.hpp"
#include "xml.hpp"


//*********************************************************************
//                      dmMakeBuilder
//*********************************************************************

int dmMakeBuilder(Player* player, cmd* cmnd) {
    Player  *target=0;

    if(!cmnd->str[1][0]) {
        player->print("Builderize whom?");
        return(0);
    }
    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);
    if(!target || !player->canSee(target)) {
        player->print("%s is not on.\n", cmnd->str[1]);
        return(0);
    }

    if(target->isStaff()) {
        player->print("%s is already a member of the staff.\n", target->getCName());
        return(0);
    }
    if(target->getExperience()) {
        player->print("%s must have 0 experience to become a builder.\n", target->getCName());
        return(0);
    }

    target->setClass(CreatureClass::BUILDER);
    target->setSecondClass(CreatureClass::NONE);
    target->setDeity(0);
    target->initBuilder();
    target->setFlag(P_DM_INVIS);
    target->setFlag(P_NO_AUTO_WEAR);
    lose_all(target, true, "builder-promotion");

    target->printColor("\n\n^yYou are now a building member of the staff.\n\n");
    player->print("%s is now a builder.\n", target->getCName());
    log_immort(true, player, "%s made %s a builder.\n", player->getCName(), target->getCName());

    if( !target->inUniqueRoom() ||
        !target->getUniqueRoomParent()->info.isArea("test") ||
        target->getUniqueRoomParent()->info.id != 1)
    {
        UniqueRoom* uRoom=0;
        CatRef cr;
        cr.setArea("test");
        cr.id = 1;

        if(!loadRoom(cr, &uRoom)) {
            player->print("Error: could not load Builder Waiting Room (%s)\n", cr.str().c_str());
            return(0);
        }
        BaseRoom* oldRoom = target->getRoomParent();
        target->dmPoof(target->getRoomParent(), uRoom);

        target->deleteFromRoom();
        target->addToRoom(uRoom);
        target->doFollow(oldRoom);
    }


    target->save(true);
    return(0);
}

//*********************************************************************
//                      checkRangeRestrict
//*********************************************************************
// also in:
//  web sevrer - functions.inc - char_auth()
//  web site - inc.inc - inRange()
//  web server - editor.inc - isAuthorized()

bool Player::checkRangeRestrict(CatRef cr, bool reading) const {
    int     i=0;

    if(cClass != CreatureClass::BUILDER)
        return(false);

    // 0 gets set in the ranges, but they can't modify that room
    if(cr.id <= 0)
        return(true);

    // hardcode this range
    if(cr.isArea("test"))
        return(false);
    // check readonly ranges
    if(reading && (
        cr.isArea("scroll") ||
        cr.isArea("song") ||
        cr.isArea("skill") ||
        cr.isArea("potion")
    ))
        return(false);

    for(; i<MAX_BUILDER_RANGE; i++) {
        if(bRange[i].belongs(cr))
            return(false);
    }

    return(true);
}


//*********************************************************************
//                      checkBuilder
//*********************************************************************
// Checks against a builder's allowed room range. Returns true if legal,
// false if illegal.

bool Player::checkBuilder(UniqueRoom* room, bool reading) const {
    if(cClass != CreatureClass::BUILDER)
        return(true);
    if(!room || !room->info.id)
        return(false);
    return(checkBuilder(room->info, reading));
}

bool Player::checkBuilder(CatRef cr, bool reading) const {
    if(cClass != CreatureClass::BUILDER)
        return(true);

    if(checkRangeRestrict(cr, reading)) {
        printColor("\n^r*** Number assignment violation.\n");
        print("Your range assignments are as follows:\n");

        listRanges(this);

        print("\n");
        return(false);
    }

    return(true);
}

//*********************************************************************
//                      listRanges
//*********************************************************************

void Player::listRanges(const Player* viewer) const {

    if(viewer->fd == fd)
        viewer->print("Your assigned number ranges:\n\n");
    else
        viewer->print("Assigned number ranges for %s:\n\n", getCName());

    viewer->printColor("Perm:       ^ytest^x:  entire area\n");

    for(int i=0; i<MAX_BUILDER_RANGE; i++) {
        if(bRange[i].low.id == -1 && bRange[i].high == -1) {
            viewer->printColor("Range #%-2d:  ^y%s^x:  entire area\n", i+1,
                bRange[i].low.area.c_str());
        } else if(bRange[i].low.id && bRange[i].high) {
            viewer->printColor("Range #%-2d:  ^y%s^x: %5d to %-5d\n", i+1,
                bRange[i].low.area.c_str(), bRange[i].low.id, bRange[i].high);
        } else
            viewer->print("Range #%-2d:  empty\n", i+1);
    }
}


//*********************************************************************
//                      dmRange
//*********************************************************************

int dmRange(Player* player, cmd* cmnd) {
    Player  *target=0;
    int     offline=0;

    if(player->getClass() == CreatureClass::BUILDER || cmnd->num == 1) {
        player->listRanges(player);
        return(0);
    }

    cmnd->str[1][0] = up(cmnd->str[1][0]);
    target = gServer->findPlayer(cmnd->str[1]);
    if(!target) {
        if(!loadPlayer(cmnd->str[1], &target)) {
            player->print("Player does not exist.\n");
            return(0);
        } else
            offline = 1;
    }

    if(!target) {
        player->print("Player not found.\n");
        return(0);
    }

    target->listRanges(player);
    player->print("\n");


    if(offline)
        free_crt(target);

    return(0);
}


//*********************************************************************
//                      initBuilder
//*********************************************************************

void Player::initBuilder() {
    if(cClass != CreatureClass::BUILDER)
        return;

    bound.room.setArea("test");
    bound.room.id = 1;
    bound.mapmarker.reset();
    cClass2 = CreatureClass::NONE;

    doDispelMagic();

    dmMax(this, 0);

    cureDisease();
    curePoison();
    removeEffect("petrification");

    // builders are always watching each other build
    setFlag(P_LOG_WATCH);
    addEffect("incognito", -1);
}

//*********************************************************************
//                      builderObj
//*********************************************************************

bool builderObj(const Creature* player) {
    return(player->canBuildObjects());
}

//*********************************************************************
//                      builderMob
//*********************************************************************

bool builderMob(const Creature* player) {
    return(player->canBuildMonsters());
}

//*********************************************************************
//                      canBuildObjects
//*********************************************************************

bool Creature::canBuildObjects() const {
    if(!isStaff())
        return(false);
    if(cClass == CreatureClass::BUILDER && !flagIsSet(P_BUILDER_OBJS))
        return(false);
    return(true);
}

//*********************************************************************
//                      canBuildMonsters
//*********************************************************************

bool Creature::canBuildMonsters() const {
    if(!isStaff())
        return(false);
    if(cClass == CreatureClass::BUILDER && !flagIsSet(P_BUILDER_MOBS))
        return(false);
    return(true);
}

//*********************************************************************
//                      builderCanEdit
//*********************************************************************

bool Player::builderCanEditRoom(bstring action) {
    if(cClass == CreatureClass::BUILDER && !getRoomParent()->isConstruction()) {
        print("You cannot %s while you are in a room that is not under construction.\n", action.c_str());
        return(false);
    }
    return(true);
}

//*********************************************************************
//                      isConstruction
//*********************************************************************

bool BaseRoom::isConstruction() const {
    if(flagIsSet(R_CONSTRUCTION))
        return(true);
    const UniqueRoom* uRoom = getAsConstUniqueRoom();
    if(uRoom && uRoom->info.isArea("test"))
        return(true);
    return(false);
}
