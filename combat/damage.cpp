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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "damage.hpp"

//*********************************************************************
//                      Damage
//*********************************************************************

Damage::Damage() {
    reset();
}

//*********************************************************************
//                      get
//*********************************************************************

int Damage::get() const { return(damage); }

//*********************************************************************
//                      getBonus
//*********************************************************************

int Damage::getBonus() const { return(bonus); }

//*********************************************************************
//                      getDrain
//*********************************************************************

int Damage::getDrain() const { return(drain); }

//*********************************************************************
//                      getReflected
//*********************************************************************

int Damage::getReflected() const { return(reflected); }

//*********************************************************************
//                      getDoubleReflected
//*********************************************************************

int Damage::getDoubleReflected() const { return(doubleReflected); }

//*********************************************************************
//                      getPhysicalReflected
//*********************************************************************

int Damage::getPhysicalReflected() const { return(physicalReflected); }

//*********************************************************************
//                      getPhysicalReflectedType
//*********************************************************************

ReflectedDamageType Damage::getPhysicalReflectedType() const { return(physicalReflectedType); }

//*********************************************************************
//                      getPhysicalBonusReflected
//*********************************************************************

int Damage::getPhysicalBonusReflected() const { return(physicalBonusReflected); }

//*********************************************************************
//                      add
//*********************************************************************

void Damage::add(int d) { damage += d; }

//*********************************************************************
//                      set
//*********************************************************************

void Damage::set(int d) { damage = d; }

//*********************************************************************
//                      includeBonus
//*********************************************************************

void Damage::includeBonus(int fraction) {
    if(!fraction)
        fraction = 1;
    damage += bonus / fraction;
    physicalReflected += physicalBonusReflected / fraction;
}

//*********************************************************************
//                      setBonus
//*********************************************************************

void Damage::setBonus(int b) { bonus = b; }

//*********************************************************************
//                      setDrain
//*********************************************************************

void Damage::setDrain(int d) { drain = d; }

//*********************************************************************
//                      setBonus
//*********************************************************************

void Damage::setBonus(Damage dmg) {
    setBonus(dmg.get());
    physicalBonusReflected = dmg.getPhysicalReflected();
}

//*********************************************************************
//                      setReflected
//*********************************************************************

void Damage::setReflected(int r) { reflected = r; }

//*********************************************************************
//                      setDoubleReflected
//*********************************************************************

void Damage::setDoubleReflected(int r) { doubleReflected = r; }

//*********************************************************************
//                      setPhysicalReflected
//*********************************************************************

void Damage::setPhysicalReflected(int r) { physicalReflected = r; }

//*********************************************************************
//                      setPhysicalReflectedType
//*********************************************************************

void Damage::setPhysicalReflectedType(ReflectedDamageType type) { physicalReflectedType = type; }

//*********************************************************************
//                      reset
//*********************************************************************

void Damage::reset() {
    damage = bonus = drain = reflected = doubleReflected = physicalReflected = physicalBonusReflected = 0;
    physicalReflectedType = REFLECTED_NONE;
}

