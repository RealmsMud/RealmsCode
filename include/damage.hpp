/*
 * damage.h
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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _DAMAGE_H
#define _DAMAGE_H

enum ReflectedDamageType {
    REFLECTED_NONE =    0,
    REFLECTED_MAGIC =   1,
    REFLECTED_PHYSICAL =    2, // generic (pass to doReflectionDamage)
    REFLECTED_FIRE_SHIELD = 3
};

class Damage {
public:
    Damage();
    void reset();

    [[nodiscard]] unsigned int get() const;
    [[nodiscard]] unsigned int getBonus() const;
    [[nodiscard]] unsigned int getDrain() const;
    [[nodiscard]] unsigned int getReflected() const;
    [[nodiscard]] unsigned int getDoubleReflected() const;
    [[nodiscard]] unsigned int getPhysicalReflected() const;
    [[nodiscard]] ReflectedDamageType getPhysicalReflectedType() const;
    [[nodiscard]] unsigned int getPhysicalBonusReflected() const;

    void add(unsigned int d);
    void set(unsigned int d);
    void includeBonus(int fraction=1);
    void setBonus(unsigned int b);
    void setDrain(unsigned int b);
    void setBonus(Damage dmg);
    void setReflected(unsigned int r);
    void setDoubleReflected(unsigned int r);
    void setPhysicalReflected(unsigned int r);
    void setPhysicalReflectedType(ReflectedDamageType type);
protected:
    unsigned int damage{};
    unsigned int bonus{};
    unsigned int drain{};
    unsigned int reflected{};
    unsigned int doubleReflected{};
    unsigned int physicalReflected{};
    unsigned int physicalBonusReflected{};
    ReflectedDamageType physicalReflectedType;
};


#endif  /* _DAMAGE_H */

