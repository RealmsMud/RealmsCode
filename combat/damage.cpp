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

unsigned int Damage::get() const { return(damage); }
unsigned int Damage::getBonus() const { return(bonus); }
unsigned int Damage::getDrain() const { return(drain); }
unsigned int Damage::getReflected() const { return(reflected); }
unsigned int Damage::getDoubleReflected() const { return(doubleReflected); }
unsigned int Damage::getPhysicalReflected() const { return(physicalReflected); }

ReflectedDamageType Damage::getPhysicalReflectedType() const { return(physicalReflectedType); }
unsigned int Damage::getPhysicalBonusReflected() const { return(physicalBonusReflected); }

void Damage::add(unsigned int d) {
    damage = std::max<int>(1, (int)damage + d);
}

void Damage::set(unsigned int d) { damage = d; }

void Damage::includeBonus(int fraction) {
    if(!fraction)
        fraction = 1;
    damage += bonus / fraction;
    physicalReflected += physicalBonusReflected / fraction;
}

void Damage::setBonus(unsigned int b) { bonus = b; }
void Damage::setBonus(Damage dmg) {
    setBonus(dmg.get());
    physicalBonusReflected = dmg.getPhysicalReflected();
}

void Damage::setDrain(unsigned int d) { drain = d; }
void Damage::setReflected(unsigned int r) { reflected = r; }
void Damage::setDoubleReflected(unsigned int r) { doubleReflected = r; }
void Damage::setPhysicalReflected(unsigned int r) { physicalReflected = r; }
void Damage::setPhysicalReflectedType(ReflectedDamageType type) { physicalReflectedType = type; }

void Damage::reset() {
    damage = bonus = drain = reflected = doubleReflected = physicalReflected = physicalBonusReflected = 0;
    physicalReflectedType = REFLECTED_NONE;
}

