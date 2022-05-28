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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
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

    [[nodiscard]] int get() const;
    [[nodiscard]] int getBonus() const;
    [[nodiscard]] int getDrain() const;
    [[nodiscard]] int getReflected() const;
    [[nodiscard]] int getDoubleReflected() const;
    [[nodiscard]] int getPhysicalReflected() const;
    [[nodiscard]] ReflectedDamageType getPhysicalReflectedType() const;
    [[nodiscard]] int getPhysicalBonusReflected() const;

    void add(int d);
    void set(int d);
    void includeBonus(int fraction=1);
    void setBonus(int b);
    void setDrain(int b);
    void setBonus(Damage dmg);
    void setReflected(int r);
    void setDoubleReflected(int r);
    void setPhysicalReflected(int r);
    void setPhysicalReflectedType(ReflectedDamageType type);
protected:
    int damage{};
    int bonus{};
    int drain{};
    int reflected{};
    int doubleReflected{};
    int physicalReflected{};
    int physicalBonusReflected{};
    ReflectedDamageType physicalReflectedType;
};


#endif  /* _DAMAGE_H */

