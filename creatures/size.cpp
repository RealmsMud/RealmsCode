/*
 * size.cpp
 *   Size code.
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

#include <fmt/format.h>              // for format
#include <cstring>                   // for strncmp, strcmp
#include <map>                       // for map, operator==, map<>::const_it...
#include <ostream>                   // for basic_ostream::operator<<, opera...
#include <string>                    // for string, allocator, operator==

#include "cmd.hpp"                   // for cmd
#include "commands.hpp"              // for getFullstrText
#include "effects.hpp"               // for EffectInfo
#include "flags.hpp"                 // for O_CURSED
#include "global.hpp"                // for CastType, CastType::CAST, Creatu...
#include "magic.hpp"                 // for SpellData, checkRefusingMagic
#include "mudObjects/container.hpp"  // for Container
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/objects.hpp"    // for Object
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for broadcast, bonus, up
#include "size.hpp"                  // for Size, NO_SIZE, SIZE_COLOSSAL
#include "stats.hpp"                 // for Stat

//*********************************************************************
//                      getSize
//*********************************************************************

Size getSize(const std::string& str) {
    if(str.empty())
        return(NO_SIZE);
    auto n = str.length();

    if(!strncmp(str.c_str(), "fine", n))
        return(SIZE_FINE);
    else if(!strncmp(str.c_str(), "diminutive", n))
        return(SIZE_DIMINUTIVE);
    else if(!strncmp(str.c_str(), "tiny", n))
        return(SIZE_TINY);
    else if(!strncmp(str.c_str(), "small", n))
        return(SIZE_SMALL);
    else if(!strncmp(str.c_str(), "medium", n))
        return(SIZE_MEDIUM);
    else if(!strncmp(str.c_str(), "large", n))
        return(SIZE_LARGE);
    else if(!strncmp(str.c_str(), "huge", n))
        return(SIZE_HUGE);
    else if(!strncmp(str.c_str(), "gargantuan", n))
        return(SIZE_GARGANTUAN);
    else if(!strncmp(str.c_str(), "colossal", n))
        return(SIZE_COLOSSAL);
    return(NO_SIZE);
}

//*********************************************************************
//                      getSizeName
//*********************************************************************

const std::map<Size, std::string> sizeToString = {
        {Size::SIZE_FINE, "fine"},
        {Size::SIZE_DIMINUTIVE, "diminutive"},
        {Size::SIZE_TINY, "tiny"},
        {Size::SIZE_SMALL, "small"},
        {Size::SIZE_MEDIUM, "medium"},
        {Size::SIZE_LARGE, "large"},
        {Size::SIZE_HUGE, "huge"},
        {Size::SIZE_GARGANTUAN, "gargantuan"},
        {Size::SIZE_COLOSSAL, "colossal"},
};
const std::string NONE_STR = "none";

const std::string & getSizeName(Size size) {
    if(sizeToString.find(size) == sizeToString.end())
        return(NONE_STR);

    return sizeToString.at(size);
}

Size whatSize(int i) {
    switch(i) {
    case 1:
        return(SIZE_FINE);
    case 2:
        return(SIZE_DIMINUTIVE);
    case 3:
        return(SIZE_TINY);
    case 4:
        return(SIZE_SMALL);
    case 5:
        return(SIZE_MEDIUM);
    case 6:
        return(SIZE_LARGE);
    case 7:
        return(SIZE_HUGE);
    case 8:
        return(SIZE_GARGANTUAN);
    case 9:
        return(SIZE_COLOSSAL);
    default:
        return(NO_SIZE);
    }
}

int numIngredients(Size size) {
    switch(size) {
    case SIZE_HUGE:
        return(2);
    case SIZE_GARGANTUAN:
        return(4);
    case SIZE_COLOSSAL:
        return(8);
    case SIZE_FINE:
    case SIZE_DIMINUTIVE:
    case SIZE_TINY:
    case SIZE_SMALL:
    case SIZE_MEDIUM:
    case SIZE_LARGE:
    default:
        return(1);
    }
}

//*********************************************************************
//                      searchMod
//*********************************************************************

int searchMod(Size size) {
    switch(size) {
    case SIZE_FINE:
        return(-40);
    case SIZE_DIMINUTIVE:
        return(-20);
    case SIZE_TINY:
        return(-10);
    case SIZE_SMALL:
        return(-5);
    case SIZE_MEDIUM:
        return(0);
    case SIZE_LARGE:
        return(5);
    case SIZE_HUGE:
        return(10);
    case SIZE_GARGANTUAN:
        return(20);
    case SIZE_COLOSSAL:
        return(40);
    default:
        return(0);
    }
}

//*********************************************************************
//                      sizePower
//*********************************************************************

int sizePower(int lvl) {
    return(std::min(3, (lvl+10)/10));
}

//*********************************************************************
//                      splChangeSize
//*********************************************************************

int splChangeSize(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData, const std::string& effect) {
    std::string power, opposite = effect == "enlarge" ? "reduce" : "enlarge";
    EffectInfo *e=nullptr;
    std::shared_ptr<Creature> target=nullptr;
    int     strength=0, num=0, pos=0;

    std::string spell;
    if(effect == "enlarge")
        spell = "an enlarge spell";
    else
        spell = "a reduce spell";

    if(spellData->how == CastType::CAST && player->getClass() !=  CreatureClass::MAGE && player->getClass() !=  CreatureClass::LICH && !player->isStaff()) {
        player->print("You cannot cast that spell.\n");
        return(0);
    }

    // Cast on self
    if(cmnd->num == 2 || (cmnd->num == 4 && !strcmp(cmnd->str[2], "to"))) {
        target = player;
        pos = 2;

        if(player->isEffected(effect)) {
            player->bPrint(fmt::format("You are already magically {}d!\n", effect));
            return(0);
        }

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("You cast %s on yourself.\n", spell.c_str());
            broadcast(player->getSock(), player->getParent(), "%M casts %s on %sself.", player.get(), spell.c_str(), player->himHer());
        }

        // Cast the spell on another player
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);
        pos = 3;

        if(!target) {
            player->print("That person is not here.\n");
            return(0);
        }

        if(target->isEffected(effect)) {
            player->print("%M is already magically %sd!\n", target.get(), effect.c_str());
            return(0);
        }

        if(target->isPlayer()) {
            if(checkRefusingMagic(player, target))
                return(0);
            target->wake("You awaken suddenly!");
        }
        player->print("You cast %s on %N.\n", spell.c_str(), target.get());
        target->print("%M casts %s on you.\n", player.get(), spell.c_str());
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts %s on %N.", player.get(), spell.c_str(), target.get());
    }

    if(target->getSize() == NO_SIZE) {
        player->print("The spell had no effect; the target's size has not been set.\n");
        return(0);
    }

    if(target->isEffected(opposite)) {
        // reduce in strength
        e = target->getEffect(opposite);
        strength = sizePower(player->getLevel());
        num = e->getStrength();

        if(e->getDuration() == -1) {
            player->print("The spell has no effect.\n");
            if(player != target)
                target->print("The spell has no effect.\n");
            return(0);
        }

        // the difference between the power of the spells
        strength = num - strength;
        if(strength <= 0)
            target->removeEffect(opposite);
        else {
            e->setStrength(strength);
            target->changeSize(num, strength, opposite == "enlarge");
        }
        return(1);
    }

    if(spellData->how == CastType::CAST) {
        strength = sizePower(player->getLevel());
        num = std::max(300, 400 + bonus(player->intelligence.getCur()) * 400) + 20 * player->getLevel();

        if(player->getRoomParent()->magicBonus()) {
            player->print("The room's magical properties increase the power of your spell.\n");
            strength++;
            num += 200;
        }

        // cast enlarge [target] to [size]
        if( (cmnd->num == 5 && pos == 3) ||
            (cmnd->num == 4 && pos == 2)
        ) {
            power = getFullstrText(cmnd->fullstr, pos);
            if(power.substr(0, 3) == "to ") {
                power = power.substr(3);
                // reuse pos; it now stands for the size they want to send them to.
                // if the difference between the target's current size and desired
                // size is OK, then we let them.
                pos = getSize(power);
                int diff = (pos - target->getSize()) * (effect == "enlarge" ? 1 : -1);

                if(!pos || diff > strength || diff < 1) {
                    if(player == target)
                        player->print("Your magic cannot affect your size in that manner.\n");
                    else
                        player->print("Your magic cannot affect %N's size in that manner.\n", target.get());
                    return(0);
                }
                strength = diff;
            }
        }
        if(effect == "enlarge") {
            std::clog << "O:" << strength;
            strength = std::min(strength, MAX_SIZE - target->getSize());
            std::clog << " N:" << strength << std::endl;
        }
        else {
            std::clog << "O:" << strength;
            strength = std::min(strength, target->getSize() - NO_SIZE - 1);
            std::clog << " N:" << strength << std::endl;
        }

        target->addEffect(effect, num, strength, player, true, player);
//      target->addEffect(effect, -2, -2, player, true, player);
//      e = target->getEffect(effect);
//      e->setStrength(strength);
//      e->setDuration(num);
//      target->changeSize(0, strength, effect == "enlarge");
    } else {
        target->addEffect(effect);
    }
    return(1);
}

//*********************************************************************
//                      size changing spells
//*********************************************************************

int splEnlarge(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splChangeSize(player, cmnd, spellData, "enlarge"));
}
int splReduce(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splChangeSize(player, cmnd, spellData, "reduce"));
}

//********************************************************************
//                      changeSize
//********************************************************************

bool Creature::changeSize(int oldStrength, int newStrength, bool enlarge) {
    int change = (newStrength - oldStrength) * (enlarge ? 1 : -1);
    if(!change)
        return(true);

    if(size+change <= NO_SIZE || size+change > MAX_SIZE)
        return(false);
    size = whatSize(size+change);

    std::shared_ptr<Player> player = getAsPlayer();
    if(player) {
        wake("You awaken suddenly!");
        for(int i=0; i<MAXWEAR; i++) {
            if(player->ready[i] && !willFit(player->ready[i])) {
                if(player->ready[i]->flagIsSet(O_CURSED)) {
                    player->printColor("%O's cursed nature causes it to %s to fit your new size!\n", player->ready[i].get(), enlarge ? "grow" : "shrink");
                } else {
                    player->printColor("%O no longer fits!\n", player->ready[i].get());
                    // i is wearloc -1, so add 1
                    unequip(i+1);
                }
            }
        }

        player->computeAC();
        player->computeAttackPower();
    }

    return(true);
}

