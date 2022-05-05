/*
 * creatureEffects.cpp
 *   Creature Effect related routines.
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

#include "flags.hpp"                        // for P_DM_INVIS, P_CHAOTIC, O_DARKNESS
#include "hooks.hpp"                        // for Hooks
#include "mudObjects/creatures.hpp"         // for Creature, PetList
#include "mudObjects/objects.hpp"           // for Object, ObjectType, ObjectType...
#include "proto.hpp"                        // for bonus
#include "utils.hpp"                        // for MIN, MAX


//*********************************************************************
//                      doPetrificationDmg
//*********************************************************************

bool Creature::doPetrificationDmg() {
    if(!isEffected("petrification"))
        return(false);

    wake("Terrible nightmares disturb your sleep!");
    printColor("^c^#Petrification spreads toward your heart.\n");
    hp.decrease(MAX<int>(1,(hp.getMax()/15 - bonus(constitution.getCur()))));

    if(hp.getCur() < 1) {
        std::shared_ptr<Player> pThis = getAsPlayer();
        if(pThis)
            pThis->die(PETRIFIED);
        else
            die(Containable::downcasted_shared_from_this<Creature>());
        return(true);
    }
    return(false);
}

