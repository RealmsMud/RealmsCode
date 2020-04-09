/*
 * god.cpp
 *   Code handling religious fighting
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
#include "creatures.hpp"
#include "mud.hpp"

int checkGodKill(Player *killer, Player *victim) {
    int     bns=0, penalty=0, levelDiff=0, base=0;
    bool    same=false;
    long    total=0;

    if(induel(killer, victim))
        return(0);
    if(killer->getLevel() < 4 || victim->getLevel() < 4)
        return(0);
    if(!killer->getDeity() || !victim->getDeity())
        return(0);
    if(victim->getNegativeLevels() || killer->getNegativeLevels())
        return(0);


    base = victim->getLevel() * 50;
    levelDiff = (int)victim->getLevel() - (int)killer->getLevel();

    if(levelDiff < 0)
        bns += 5 * victim->getLevel() * levelDiff;
    else
        bns += levelDiff * 50;


    total = MAX(1, Random::get((base + bns)/2, base + bns));

    if(killer->halftolevel())
        total = 0;
    if(killer->hasSecondClass())
        total = total * 3 / 4;

    penalty = MIN(Random::get(1000,1500), (bns*3)/2);

    if(killer->getLevel() > victim->getLevel() + 6)
        penalty = 100;

    if(killer->getDeity() == victim->getDeity()) {
        total = -100;
        penalty = 100;
        same = true;
    }

    if(!same) {
        penalty = MAX(50, penalty);

        killer->printColor("^cYou have defeated a member of an enemy religion!\n");
        killer->printColor("^cYour order honors you with %d experience.\n", total);

        victim->printColor("^cYou have been defeated by a member of a rival religion!\n");
        victim->printColor("^cYour order shames you by taking %d experience.\n", penalty);
    } else {
        killer->printColor("^cYou have shamelessly defeated a member of your own religion!\n");
        killer->printColor("^cYour order penalizes you for %d experience.\n", total);

        victim->printColor("^cYou have been defeated during senseless religious infighting!\n");
        victim->printColor("^cYour order penalizes you for %d experience.\n", penalty);
    }

    killer->addExperience(total);
    victim->subExperience(penalty);
    return(1);
}
