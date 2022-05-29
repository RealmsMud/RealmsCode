/*
 * damage.cpp
 *   Damage
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

#include <algorithm>
#include "damage.hpp"

//*********************************************************************
//                      Damage
//*********************************************************************

Damage::Damage() {
    reset();
}

int Damage::get() const { return(damage); }
int Damage::getBonus() const { return(bonus); }
int Damage::getDrain() const { return(drain); }
int Damage::getReflected() const { return(reflected); }
int Damage::getDoubleReflected() const { return(doubleReflected); }
int Damage::getPhysicalReflected() const { return(physicalReflected); }

ReflectedDamageType Damage::getPhysicalReflectedType() const { return(physicalReflectedType); }
int Damage::getPhysicalBonusReflected() const { return(physicalBonusReflected); }

void Damage::add(int d) {
    damage = std::max<int>(1, damage + d);
}

void Damage::set(int d) { damage = d; }

void Damage::includeBonus(int fraction) {
    if(!fraction)
        fraction = 1;
    damage += bonus / fraction;
    physicalReflected += physicalBonusReflected / fraction;
}

void Damage::setBonus(int b) { bonus = b; }
void Damage::setBonus(Damage dmg) {
    setBonus(dmg.get());
    physicalBonusReflected = dmg.getPhysicalReflected();
}

void Damage::setDrain(int d) { drain = d; }
void Damage::setReflected(int r) { reflected = r; }
void Damage::setDoubleReflected(int r) { doubleReflected = r; }
void Damage::setPhysicalReflected(int r) { physicalReflected = r; }
void Damage::setPhysicalReflectedType(ReflectedDamageType type) { physicalReflectedType = type; }

void Damage::reset() {
    damage = bonus = drain = reflected = doubleReflected = physicalReflected = physicalBonusReflected = 0;
    physicalReflectedType = REFLECTED_NONE;
}

