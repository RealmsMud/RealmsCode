/*
 * enchantment.cpp
 *   Enchantment spells
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

#include <ctime>                       // for time
#include <string>                      // for allocator, string

#include "cmd.hpp"                     // for cmd
#include "config.hpp"                  // for Config, gConfig
#include "deityData.hpp"               // for DeityData
#include "flags.hpp"                   // for M_PERMANENT_MONSTER, M_RESIST_...
#include "global.hpp"                  // for CastType, CreatureClass, CastT...
#include "lasttime.hpp"                // for lasttime
#include "magic.hpp"                   // for SpellData, checkRefusingMagic
#include "money.hpp"                   // for GOLD, Money
#include "mud.hpp"                     // for DL_SILENCE, LT_ENCHA, LT_SPELL
#include "mudObjects/container.hpp"    // for Container
#include "mudObjects/creatures.hpp"    // for Creature
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/objects.hpp"      // for Object, ObjectType, ObjectType...
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "proto.hpp"                   // for broadcast, bonus, crtWisdom
#include "random.hpp"                  // for Random
#include "statistics.hpp"              // for Statistics
#include "stats.hpp"                   // for Stat
#include "structs.hpp"                 // for daily, saves
#include "wanderInfo.hpp"              // for WanderInfo


//*********************************************************************
//                     isResistableEnchantment 
//*********************************************************************
// This function determines which enchantment/trickery spells are currently checked by
// Creature::checkResistEnchantments()...For class/race/mtypes...primarily 
// used for hold spells, and fear/scare
bool isResistableEnchantment(const std::string spell) {
    return(spell == "hold-undead" || spell == "hold-animal" || spell == "hold-plant" || 
                        spell == "hold-person" || spell == "hold-monster" || spell == "hold-elemental" || 
                        spell == "hold-fey" || spell == "scare" || spell == "fear");
}

//*********************************************************************
//                      splHoldPerson
//*********************************************************************
int splHoldPerson(const std::shared_ptr<Creature>& caster, cmd* cmnd, SpellData* spellData) {
    
    if(spellData->how == CastType::CAST && !caster->isCt() &&
        !caster->isArcaneCaster() && !caster->isDivineCaster())
    {
        *caster << "You are unable to comprehend the subtle nuances of that spell.\n";
        return(0);
    }
    
    if (spellData->how == CastType::CAST && caster->getClass() == CreatureClass::RANGER) {
        *caster << "The complexity of that type of enchantment/trickery magic eludes you.\n";
        return(0);
    }

    return(doHoldSpells(caster, cmnd, spellData, "hold-person"));

}

//*********************************************************************
//                      splHoldMonster
//*********************************************************************
int splHoldMonster(const std::shared_ptr<Creature>& caster, cmd* cmnd, SpellData* spellData) {


    if(spellData->how == CastType::CAST && !caster->isCt() &&
        !caster->isArcaneCaster() && !caster->isDivineCaster())
    {
        *caster << "You are unable to comprehend the subtle nuances of that spell.\n";
        return(0);
    }

    if (spellData->how == CastType::CAST && caster->getClass() == CreatureClass::RANGER) {
        *caster << "The complexity of that type of enchantment/trickery magic eludes you.\n";
        return(0);
    }

    return(doHoldSpells(caster, cmnd, spellData, "hold-monster"));

}

//*********************************************************************
//                      splHoldUndead
//*********************************************************************
int splHoldUndead(const std::shared_ptr<Creature>& caster, cmd* cmnd, SpellData* spellData) {

    if(spellData->how == CastType::CAST && !caster->isCt() &&
        !caster->isPureArcaneCaster() && !caster->isPureDivineCaster())
    {
        *caster << "You are unable to comprehend the subtle nuances of that spell.\n";
        return(0);
    }

    return(doHoldSpells(caster, cmnd, spellData, "hold-undead"));

}

//*********************************************************************
//                      splHoldAnimal
//*********************************************************************
int splHoldAnimal(const std::shared_ptr<Creature>& caster, cmd* cmnd, SpellData* spellData) {

    if(spellData->how == CastType::CAST && !caster->isCt() &&
                        caster->getClass() != CreatureClass::DRUID &&
                             caster->getClass() != CreatureClass::RANGER &&
                                    caster->getClass() != CreatureClass::CLERIC) 
    {
        *caster << "You are not enough in tune with nature to understand that spell.\n";
        return(0);
    }

    if (spellData->how == CastType::CAST && 
                caster->getClass() == CreatureClass::CLERIC && 
                         caster->getDeity() != LINOTHAN &&
                                caster->getDeity() != MARA) 
    {
        *caster << gConfig->getDeity(caster->getDeity())->getName() << "does not grant the ability to cast that spell. Only Mara and Linothan do.\n";
        return(0);

    }

    return(doHoldSpells(caster, cmnd, spellData, "hold-animal"));

}

//*********************************************************************
//                      splHoldPlant
//*********************************************************************
int splHoldPlant(const std::shared_ptr<Creature>& caster, cmd* cmnd, SpellData* spellData) {

    if(spellData->how == CastType::CAST && !caster->isCt() &&
                        caster->getClass() != CreatureClass::DRUID &&
                             caster->getClass() != CreatureClass::RANGER &&
                                    caster->getClass() != CreatureClass::CLERIC) 
    {
        *caster << "You are not enough in tune with nature to understand that spell.\n";
        return(0);
    }

    if (spellData->how == CastType::CAST && 
                caster->getClass() == CreatureClass::CLERIC && 
                        caster->getDeity() != LINOTHAN &&
                                    caster->getDeity() != MARA) 
    {
        *caster << gConfig->getDeity(caster->getDeity())->getName() << "does not grant the ability to cast that spell. Only Mara and Linothan do.\n";
        return(0);

    }

    return(doHoldSpells(caster, cmnd, spellData, "hold-plant"));

}

//*********************************************************************
//                      splHoldElemental
//*********************************************************************
int splHoldElemental(const std::shared_ptr<Creature>& caster, cmd* cmnd, SpellData* spellData) {

    if(spellData->how == CastType::CAST && !caster->isCt() &&
        !caster->isPureArcaneCaster() && caster->getClass() != CreatureClass::DRUID)
    {
        *caster << "You are unable to comprehend the subtle arcane and natural nuances of such a powerful spell.\n";
        return(0);
    }

    return(doHoldSpells(caster, cmnd, spellData, "hold-elemental"));

}

//*********************************************************************
//                      splHoldFey
//*********************************************************************
int splHoldFey(const std::shared_ptr<Creature>& caster, cmd* cmnd, SpellData* spellData) {

    if(spellData->how == CastType::CAST && !caster->isCt() &&
       caster->getClass() != CreatureClass::DRUID && caster->getClass() != CreatureClass::CLERIC)
    {
        *caster << "You are unable to comprehend such powerful natural magic.\n";
        return(0);
    }

    if (spellData->how == CastType::CAST && caster->getClass() == CreatureClass::CLERIC && caster->getDeity() != MARA && caster->getDeity() != LINOTHAN) {
        *caster << gConfig->getDeity(caster->getDeity())->getName() << "does not grant the ability to cast that spell. Only Mara and Linothan do.\n";
        return(0);
    }

    return(doHoldSpells(caster, cmnd, spellData, "hold-fey"));

}

//*********************************************************************
//                      doHoldSpells
//*********************************************************************

int doHoldSpells(const std::shared_ptr<Creature>& caster, cmd* cmnd, SpellData* spellData, const char* spell) {
    int  dur=0, strength=0, chance=0, dmgToBreak=0;
    bool burnThrough = false, prepared = false, noSkill=false;
    std::shared_ptr<Creature> target=nullptr;
    std::string magicSkill = "";
    EffectInfo* holdEffect = nullptr;

    if(caster->isPlayer() && !caster->flagIsSet(P_PTESTER) && !caster->isCt()) {
         if(spellData->how == CastType::CAST) {
            *caster << ColorOn << "^yAll hold spells are currently disabled except for p-testers and staff.\n";
            *caster << "If you're bored and wish to help p-test hold spells sometime, check with Ocelot.\n" << ColorOff;
        }
        else
            *caster << ColorOn << "^cNothing happens.\n" << ColorOff;

        return(0);
    }

    // If using a wand, use the wand's object->getLevel() or default 10 if it's not set
    // Otherwise use spellData->level (based on enchantment magic skill)
    if (spellData->object) 
        strength = (spellData->object->getLevel()>0?spellData->object->getLevel():10);
    else
        strength = spellData->level;

    if (caster->isArcaneCaster())
        magicSkill = "enchantment";
    else if (caster->isDivineCaster())
        magicSkill = "trickery";
    else
    {
        noSkill = true;
    }

    

    if(caster->isPlayer())
        dmgToBreak = (noSkill?((strength*5)/2):caster->getSkillGained(magicSkill)/2);
    else
        dmgToBreak = (noSkill?((strength*5)/2):((strength*5)/2));

    dmgToBreak = std::max(40,dmgToBreak);

    if(cmnd->num == 2) {
        if (spellData->how != CastType::POTION) {
            *caster << "On what target?\n";
            return(0);
        }
        // Check resistances since we drank a potion with a hold spell....
        else if (caster->isEffected("free-action") || caster->checkResistEnchantments(caster,spell,false)) {
            *caster << ColorOn << "^yYou resisted being magically held.\n" << ColorOff;
            return(1);
        }
        else {

            *caster << ColorOn << "^YYou are magically held!\n" << ColorOff;
            broadcast(caster->getSock(), caster->getParent(), "^Y%M is magically held!", caster.get());

            dur = Random::get(25,30) - (caster->getWillpower()/10);
        
            caster->unhide();
            caster->addEffect(spell, (long)dur, strength, caster, true, caster);
            holdEffect = caster->getEffect(spell);
            if(holdEffect)
                holdEffect->setExtra(Random::get(dmgToBreak/2,dmgToBreak));

            return(1);
        }

    } else {
        if (caster->noPotion(spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = caster->getParent()->findCreature(caster, cmnd->str[2], cmnd->val[2], true, true);
        if (!target) {
            *caster << "Cannot find that target.\n";
            return(0);
        }

        if (target == caster) {
            *caster << ColorOn << "^cThat would be a pretty stupid thing to do, don't you think?\n" << ColorOff;
            return(0);
        }

        if (caster->isPlayer() && target->isPlayer() && !target->flagIsSet(P_PTESTER)) {
            *caster << setf(CAP) << target << " is not a p-tester.\n";
            *caster << "Hold spells can only be cast on other p-testers right now.\n";
            return(0);
        }

        if (caster->isPlayer() && target->isPlayer() && target->inCombat(false)) {
            *caster << "Not in the middle of combat.\n";
            return(0);
        }

        if (caster->isPlayer() && caster->getRoomParent()->isPkSafe() && (target->isPlayer() || target->isPet())) {
            *caster << "You can't do that in here. That'd be annoying.\n";
            return(0);
        }

        caster->smashInvis();
        caster->unhide();

        if (target->isStaff()) {
            *caster << ColorOn << "^yYour spell failed.\n" << ColorOff;
            return(0);
        }

        *caster << "You cast a " << spell << " spell on " << target << ".\n";
        *target << setf(CAP) << caster << " casts a " << spell << " spell on you.\n";
        broadcast(caster->getSock(), target->getSock(), caster->getParent(), "%M casts a %s spell on %N.", caster.get(), spell, target.get());

        // Targets cannot have any hold spell cast on them when they are already under one
        if (target->isMagicallyHeld()) {
            *caster << ColorOn << "^y" << setf(CAP) << target << " is already affected by holding magic. Your spell had no effect.\n" << ColorOff;
            *target << ColorOn << "^y" << setf(CAP) << caster << "'s spell had no effect.\n" << ColorOff;
            broadcast(caster->getSock(), target->getSock(), caster->getParent(), "^y%M's spell had no effect.^x", caster.get());
            return(0);
            }

        EffectInfo* freeActionEffect = nullptr;
        if ((freeActionEffect = target->getEffect("free-action"))) {

            //If hold spell strength == free-action effect strength, 25% chance to burn through free-action
            if ((freeActionEffect->getStrength() == strength) && (Random::get(1,100) <= 25))
                burnThrough = true;

            //Permanent free-action cannot be burned through; We'll be nice and not make the caster use MP
            if (freeActionEffect->isPermanent()) {
                *caster << ColorOn << "^g" << "The free-action effect on " << target << " is permanent!\nYour spell dissipates.\n" << ColorOff;
                *target << ColorOn << "^g" << "Your free-action effect protected you from " << caster << "'s holding magic.\n" << ColorOff;
                broadcast(caster->getSock(), target->getSock(), caster->getParent(), "^g%M's spell dissipated.^x", caster.get());
                return(0);
            }
            else if (((freeActionEffect->getStrength() > strength) && !burnThrough && !caster->isCt())) {
                *caster << ColorOn << "^g" << "The free-action effect on " << target << 
                                                (burnThrough?" managed to deflect your spell.":" is currently too powerful for you.") << "\nYour spell dissipates.\n" << ColorOff;
                *target << ColorOn << "^g" << "Your free-action effect protected you from " << caster << "'s holding magic.\n" << ColorOff;
                broadcast(caster->getSock(), target->getSock(), caster->getParent(), "^g%M's spell dissipated.^x", target.get());
                return(1);
            }
            else {
                target->removeEffect("free-action");
                burnThrough = true;
            }
         }

        if (target->checkResistEnchantments(caster, spell, true)) {
            if (target->isMonster() && target->getAsMonster()->getEnchantmentImmunity(caster, spell, false))
                return(0);
            else
                return(1);
        }

        chance = 500 + (25*(strength - target->getLevel())) + (3*(caster->getWillpower() - target->getWillpower()));

        //If a target monster is smart enough, it might be prepared for a hold spell if caster already is an enemy
        if(target->isMonster() && target->getAsMonster()->isEnemy(caster) &&
                            target->intelligence.getCur() >= 130 && Random::get(1,100) <= 50) {
            *caster << ColorOn << "^r" << setf(CAP) << target << " was prepared for your spell!\n" << ColorOff;
            broadcast(caster->getSock(), caster->getParent(), "^r%M was prepared for %N's spell!^x", target.get(), caster.get());
            prepared = true;
            chance /= 2;
        }

        int roll = Random::get(1,1000);
        if (caster->isCt() || (caster->isPlayer() && caster->flagIsSet(P_PTESTER))) {
            *caster << ColorOn << "^D*PTest*\n";
            *caster << "strength = " << strength << "\n";
            *caster << "target->getLevel() = " << target->getLevel() << "\n";
            *caster << "caster->getWillpower() = " << caster->getWillpower() << "\n";
            *caster << "target->getWillpower() = " << target->getWillpower() << "\n\n";
            *caster << "Chance = 500 + " << (25*(strength - target->getLevel())) << " + " << 
                            (3*(caster->getWillpower() - target->getWillpower())) << " = " << chance << (chance<50?" (50)":"") << "\n";  
            *caster << "Roll = " << roll << "\n" << ColorOff;
        }

        chance = std::max(50,chance);

        if ((roll <= chance) || caster->isCt()) {
            dur = Random::get(25,30) + ((caster->getWillpower()/20) - (target->getWillpower()/20));
            dur += (strength - target->getLevel())/3;
            if (prepared) // Duration 25% less if monster prepared for the spell
                dur -= (dur/4);
            dur = std::max(6, std::min(60,dur));   

            if (target->isMonster())
                target->getAsMonster()->addEnemy(caster, true);

            if (burnThrough) {
                *caster << ColorOn << "^GYour spell burned through " << target << "'s free-action effect!\n" << ColorOff;
                *target << ColorOn << "^G" << setf(CAP) << caster << "'s spell burned through your free-action effect!\n" << ColorOff;
                broadcast(caster->getSock(), target->getSock(), caster->getParent(), "^G%M's spell burned through %N's free-action effect!^x", caster.get(), target.get());
            }             

            *caster << ColorOn << "^Y" << setf(CAP) << target << " is magically held in place!\n" << ColorOff;
            *target << ColorOn << "^YYou are magically held in place!\n" << ColorOff;
            broadcast(caster->getSock(), target->getSock(), caster->getParent(), "^Y%M is magically held in place!^x", target.get());
            
            
            
            target->addEffect(spell, long(dur), strength, caster, true, caster);

            holdEffect = target->getEffect(spell);
            if(holdEffect)
                holdEffect->setExtra(Random::get(dmgToBreak/2,dmgToBreak));

            if (caster->isCt() || caster->flagIsSet(P_PTESTER))
                *caster << ColorOn << "^D*Staff*\nduration = " << dur << " sec\nDmg to break = " << holdEffect->getExtra() << "\n" << ColorOff;
            
        } else {
            *caster << ColorOn << "^y" << setf(CAP) << target << " resisted your spell.\n" << ColorOff;
            *target << ColorOn << "^yYou resisted " << caster << "'s spell.\n" << ColorOff;
            broadcast(caster->getSock(), target->getSock(), caster->getParent(), "^y%M resisted %N's spell.^x",target.get(), caster.get());
        }


    }
    return(1);
}


//*********************************************************************
//                      splScare
//*********************************************************************

int splScare(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Player> target=nullptr;
    int     bns=0;
    long    t = time(nullptr);

    if(player->getClass() !=  CreatureClass::CLERIC && player->getClass() !=  CreatureClass::MAGE &&
        player->getClass() !=  CreatureClass::LICH && player->getClass() !=  CreatureClass::DEATHKNIGHT &&
        player->getClass() !=  CreatureClass::BARD && !player->isEffected("vampirism") &&
        !player->isCt() && spellData->how == CastType::CAST
    ) {
        *player << "You are unable to cast that spell.\n";
        return(0);
    }

    // Cast scare
    if(cmnd->num == 2) {
        if(spellData->how != CastType::POTION) {
            *player << "That player is not here.\n";
            return(0);
        }

        if(player->isEffected("berserk")) {
            *player << "You are too enraged to drink that right now.\n";
            return(0);
        }
        if(player->isEffected("courage")) {
            *player << "You feel a strange tingle.\n";
            return(0);
        }

        *player << "You are suddenly terrified to the bone!\n";
        broadcast(player->getSock(), player->getParent(), "%M becomes utterly terrified!", player.get());

        target = player->getAsPlayer();

        // killed while fleeing?
        if(target->flee(true) == 2)
            return(1);


    // Cast scare on another player
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);

        target = player->getParent()->findPlayer(player, cmnd, 2);
        if(!target) {
            *player << "That player is not here.\n";
            return(0);
        }

        if (target == player) {
            *player << "Why would you try to cast a scare spell on yourself? Just flee!\n";
            return(0);
        }

        if(target->inCombat(false) && !player->flagIsSet(P_OUTLAW)) {
            *player << "Not in the middle of combat.\n";
            return(0);
        }

        if(!player->isCt() && target->isCt()) {
            *player << ColorOn << "^yYour spell failed.\n" << ColorOff;
            return(0);
        }

        if(!player->isDm() && target->isDm()) {
            *player << ColorOn << "^yYour spell failed.\n" << ColorOff;
            return(0);
        }

        if(!player->isCt() && !dec_daily(&player->daily[DL_SCARES]) && !player->flagIsSet(P_PTESTER)) {
            *player << "You cannot cast that again today.\n";
            return(0);
        }


        *player << "You cast a scare spell on " << target << ".\n";
        *target << setf(CAP) << player << " casts a scare spell on you.\n";
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a scare spell on %N.", player.get(), target.get());


        if (target->checkResistEnchantments(player, "scare", true)) {
            return(1);
        }

        if(target->isEffected("courage") && !player->isCt()) {
            *player << ColorOn << "^y" << setf(CAP) << target << "is too courageous for that to work right now.\n";
            *target << "You shrugged off " << player << "'s spell due to your magical courage.\n";
             broadcast(player->getSock(), target->getSock(), player->getParent(), "%M's spell had no effect.", player.get());
            return(1);
        }

        if(target->isMagicallyHeld()) {
            *player << setf(CAP) << target << " is currently magically held! " << target->upHeShe() << " can't run anywhere!\n";
            *target << "You are suddenly more terrified than ever right now, but you can't move to flee!\n";
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M looks really terrified, but %s can't move to flee!", target.get(), target->heShe());
            return(1);
        }

        target->wake("Terrible nightmares disturb your sleep!");

        // Spell can only target players. I guess we'll leave this here in a "just in case" type scenario...
        // Pets are immune.
        if(target->isPet()) {
            *player << setf(CAP) << target->getMaster() << "'s " << target->getCName() << " is too loyal to be magically scared.\n";
            return(0);
        }

        bns = 5*((int)target->getLevel() - (int)spellData->level);
        if(!target->chkSave(SPL, player, bns) || player->isCt()) {

            if(spellData->how == CastType::CAST && player->isPlayer())
                player->getAsPlayer()->statistics.offensiveCast();

            target->clearFlag(P_SITTING);
            *target << "You are suddenly terrified to the bone!\n";
            broadcast(target->getSock(), player->getRoomParent(), "%M becomes utterly terrified!", target.get());

            target->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
            target->lasttime[LT_SPELL].ltime = t;
            target->lasttime[LT_SPELL].interval = 3L;

            // killed while fleeing?
            if(target->flee(true) == 2)
                return(1);

            target->stun(Random::get(5,8));

        } else {
            *player << setf(CAP) << target << " resisted your spell.\n";
            broadcast(player->getSock(), target->getSock(), target->getRoomParent(), "%M resisted %N's scare spell.",target.get(), player.get());
            *target << setf(CAP) << player << " tried to cast a scare spell on you.\n";
        }
    }
    return(1);
}

//*********************************************************************
//                      splCourage
//*********************************************************************

int splCourage(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;
    long    dur=0;
    int     noCast=0;

    if(spellData->how != CastType::POTION) {
        if(!player->isCt() && player->getClass() !=  CreatureClass::PALADIN && player->getClass() !=  CreatureClass::CLERIC) {
            player->print("Your class is unable to cast that spell.\n");
            return(0);
        }

        if(player->getClass() == CreatureClass::CLERIC) {
            switch(player->getDeity()) {
            case CERIS:
            case ARAMON:
            case ARACHNUS:
            case JAKAR:
                noCast=1;
            break;
            }
        }

        if(noCast) {
            player->print("%s does not allow you to cast that spell.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
            return(0);
        }
    }

    if(cmnd->num == 2) {
        target = player;

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Courage spell cast.\nYou feel unnaturally brave.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts a courage spell on %sself.", player.get(), player->himHer());
        } else if(spellData->how == CastType::POTION)
            player->print("You feel unnaturally brave.\n");

    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);

        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            player->print("You don't see that person here.\n");
            return(0);
        }

        if(checkRefusingMagic(player, target))
            return(0);

        player->print("Courage cast on %s.\n", target->getCName());
        target->print("%M casts a courage spell on you.\n", player.get());
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a courage spell on %N.", player.get(), target.get());
    }

    target->removeEffect("fear", true, false);

    if(target->isEffected("fear")) {
        player->print("Your spell fails.\n");
        target->print("The spell fails.\n");
        return(0);
    }

    if(spellData->how == CastType::CAST) {
        dur = std::max(300, 900 + bonus(player->intelligence.getCur()) * 300);

        if(player->getRoomParent()->magicBonus()) {
            player->print("The room's magical properties increase the power of your spell.\n");
            dur += 300L;
        }
    } else
        dur = 600;

    target->addEffect("courage", dur, 1, player, true, player);
    return(1);
}

//*********************************************************************
//                      splFear
//*********************************************************************
// The fear spell causes the monster to have a high wimpy / flee
// percentage and a penality of -2 on all attacks

int splFear(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;
    int     dur=0;

    if(spellData->how == CastType::CAST) {
        dur = 300 + (Random::get(1, 30) * 10) + player->intelligence.getCur();
    } else if(spellData->how == CastType::SCROLL)
        dur = 300 + (Random::get(1, 15) * 10) + (player->intelligence.getCur()/2);
    else
        dur = 300 + (Random::get(1, 10) * 10);

    if(spellData->how == CastType::POTION) 
        dur = Random::get(1,120) + 180L;

    if(player->spellFail( spellData->how))
        return(0);

    // fear on self
    if(cmnd->num == 2) {
        target = player;

        if(player->isEffected("resist-magic"))
            dur /= 2;

        if(spellData->how == CastType::CAST || spellData->how == CastType::WAND || spellData->how == CastType::SCROLL) {
            *player << "A terrifying magical fear flows into you and then quickly dissipates.\n";
            broadcast(player->getSock(), player->getParent(), "%M casts a fear spell on %sself.", player.get(), player->himHer());
            return(0);
        }

        if(spellData->how == CastType::POTION && player->getClass() == CreatureClass::PALADIN) {
            *player << "You feel a jitter, then shrug it off.\n";
            return(0);
        } else if(spellData->how == CastType::POTION)
            *player << "You begin to shake in terror.\n";

    // fear a monster or player
    } else {
        if(player->noPotion( spellData))
            return(0);

        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);
        if(!target || target == player) {
            *player << "That's not here.\n";
            return(0);
        }

        if(spellData->how == CastType::WAND || spellData->how == CastType::SCROLL || spellData->how == CastType::CAST) {
            if( player->isPlayer() &&
                target->isPlayer() &&
                player->getRoomParent()->isPkSafe() &&
                !target->flagIsSet(P_OUTLAW)
            ) {
                *player << "You cannot cast that spell on " << target << " in this room.\n";
                return(0);
            }
        }

        if(!player->canAttack(target))
            return(0);

        
        if( (target->mFlagIsSet(M_PERMANENT_MONSTER))) {
            *player << setf(CAP) << target << " is too strong willed for that. Your spell had no effect.\n";
            broadcast(player->getSock(), target->getSock(), player->getParent(),
                             "%M casts a fear spell on %N.\n%M brushes it off and attacks %N.", player.get(), target.get(), target.get(), player.get());
         
            target->getAsMonster()->addEnemy(player, true);
            return(0);
        }
        

        if (target->isEffected("resist-magic"))
            dur /= 2;

        player->smashInvis();

        target->wake("Terrible nightmares disturb your sleep!");

        if(target->chkSave(SPL, player, 0) && !player->isCt()) {
            *target << setf(CAP) << player << " tried to cast a fear spell on you!\n";
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to cast a fear spell on %N!", player.get(), target.get());
            *player << "The spell had no effect.\n";
            return(0);
        }

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            *player << "Fear spell cast on " << target << ".\n";
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts fear on %N.", player.get(), target.get());
            *target << setf(CAP) << player << " casts a fear spell on you.\n";
        }

        if (target->checkResistEnchantments(player, "fear", true))
            return(1);

        
    }

    target->addEffect("fear", dur, 1, player, true, player);

    if(spellData->how == CastType::CAST && player->isPlayer())
        player->getAsPlayer()->statistics.offensiveCast();
    return(1);
}


//*********************************************************************
//                      splSilence
//*********************************************************************
// Silence causes a player or monster to lose their voice, making them
// unable to casts spells, use scrolls, speak, yell, or broadcast

int splSilence(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;
    int     bns=0, canCast=0, mpCost=0;
    long    dur=0;


    if(player->getClass() == CreatureClass::BUILDER) {
        player->print("You cannot cast this spell.\n");
        return(0);
    }

    if( player->getClass() == CreatureClass::LICH ||
        player->getClass() == CreatureClass::MAGE ||
        player->getClass() == CreatureClass::CLERIC ||
        player->isCt()
    )
        canCast = 1;

    if(!canCast && player->isPlayer() && spellData->how == CastType::CAST) {
        player->print("You are unable to cast that spell.\n");
        return(0);
    }

    if(player->getClass() == CreatureClass::MAGE || player->getClass() == CreatureClass::LICH)
        mpCost = 35;
    else
        mpCost = 20;

    if(spellData->how == CastType::CAST && !player->checkMp(mpCost))
        return(0);

    player->smashInvis();

    if(spellData->how == CastType::CAST) {
        dur = Random::get(180,300) + 3*bonus(player->intelligence.getCur());
    } else if(spellData->how == CastType::SCROLL)
        dur = Random::get(30,120) + bonus(player->intelligence.getCur());
    else
        dur = Random::get(30,60);



    if(player->spellFail( spellData->how))
        return(0);

    // silence on self
    if(cmnd->num == 2) {
        if(spellData->how == CastType::CAST)
            player->subMp(mpCost);

        target = player;
        if(player->isEffected("resist-magic"))
            dur /= 2;

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            broadcast(player->getSock(), player->getParent(), "%M casts silence on %sself.", player.get(), player->himHer());
        } else if(spellData->how == CastType::POTION)
            player->print("Your throat goes dry and you cannot speak.\n");

    // silence a monster or player
    } else {
        if(player->noPotion( spellData))
            return(0);

        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target || target == player) {
            player->print("That's not here.\n");
            return(0);
        }

        if(!player->canAttack(target))
            return(0);

        if(player->isPlayer() && target->mFlagIsSet(M_PERMANENT_MONSTER)) {
            if(!dec_daily(&player->daily[DL_SILENCE]) && !player->isCt()) {
                player->print("You have done that enough times for today.\n");
                return(0);
            }
        }

        bns = ((int)target->getLevel() - (int)spellData->level)*10;

        if( (target->isPlayer() && target->isEffected("resist-magic")) ||
            (target->isMonster() && target->isEffected("resist-magic"))
        ) {
            bns += 50;
            dur /= 2;
        }

        if(target->isPlayer() && player->isCt())
            dur = 600L;

        if(target->mFlagIsSet(M_PERMANENT_MONSTER))
            bns = (target->saves[SPL].chance)/3;

        if(spellData->how == CastType::CAST)
            player->subMp(mpCost);

        target->wake("Terrible nightmares disturb your sleep!");

        if(target->chkSave(SPL, player, bns) && !player->isCt()) {
            target->print("%M tried to cast a silence spell on you!\n", player.get());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to cast a silence spell on %N!", player.get(), target.get());
            player->print("Your spell fizzles.\n");
            return(0);
        }


        if(player->isPlayer() && target->isPlayer()) {
            if(!dec_daily(&player->daily[DL_SILENCE]) && !player->isCt()) {
                player->print("You have done that enough times for today.\n");
                return(0);
            }
        }


        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Silence casted on %s.\n", target->getCName());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a silence spell on %N.", player.get(), target.get());

            logCast(player, target, "silence");

            target->print("%M casts a silence spell on you.\n", player.get());
        }

        if(target->isMonster())
            target->getAsMonster()->addEnemy(player);
    }

    target->addEffect("silence", dur, 1, player, true, player);

    if(spellData->how == CastType::CAST && player->isPlayer())
        player->getAsPlayer()->statistics.offensiveCast();
    if(player->isCt() && target->isPlayer())
        target->setFlag(P_DM_SILENCED);

    return(1);
}

//*********************************************************************
//                      canEnchant
//*********************************************************************

bool canEnchant(const std::shared_ptr<Player>& player, SpellData* spellData) {
    if(!player->isStaff()) {
        if(spellData->how == CastType::CAST && player->getClass() !=  CreatureClass::MAGE && player->getClass() !=  CreatureClass::LICH) {
            player->print("Only mages may enchant objects.\n");
            return(false);
        }
        if(spellData->how == CastType::CAST && player->getClass() == CreatureClass::MAGE && player->hasSecondClass()) {
            player->print("Only pure mages may enchant objects.\n");
            return(false);
        }
        
        if(spellData->level < 16 && !player->isCt()) {
            player->print("You are not experienced enough to cast that spell yet.\n");
            return(false);
        }
    }
    return(true);
}

bool canEnchant(const std::shared_ptr<Creature>& player, std::shared_ptr<Object>  object) {
    if(player->isStaff())
        return(true);
    if( object->flagIsSet(O_CUSTOM_OBJ) ||
        (object->getType() != ObjectType::ARMOR && object->getType() != ObjectType::WEAPON && object->getType() != ObjectType::MISC) ||
        object->flagIsSet(O_NULL_MAGIC) ||
        object->flagIsSet(O_NO_ENCHANT)
    ) {
        player->print("That cannot be enchanted.\n");
        return(false);
    }
    if(object->getAdjustment()) {
        player->printColor("%O is already enchanted.\n", object.get());
        return(false);
    }
    return(true);
}

//*********************************************************************
//                      decEnchant
//*********************************************************************

bool decEnchant(const std::shared_ptr<Player>& player, CastType how) {
    if(how == CastType::CAST) {
        if(!dec_daily(&player->daily[DL_ENCHA]) && !player->isCt()) {
            player->print("You have enchanted enough today.\n");
            return(false);
        }
    }
    return(true);
}


//*********************************************************************
//                      splEnchant
//*********************************************************************
// This function allows mages to enchant items.

int splEnchant(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Player> pPlayer = player->getAsPlayer();
    if(!pPlayer)
        return(0);

    std::shared_ptr<Object> object=nullptr;
    int     adj=1;

    if(!canEnchant(pPlayer, spellData))
        return(0);

    if(cmnd->num < 3) {
        pPlayer->print("Cast the spell on what?\n");
        return(0);
    }

    object = pPlayer->findObject(pPlayer, cmnd, 2);
    if(!object) {
        pPlayer->print("You don't have that in your inventory.\n");
        return(0);
    }

    if(!canEnchant(pPlayer, object))
        return(1);

    if(!decEnchant(pPlayer, spellData->how))
        return(0);

    if((pPlayer->getClass() == CreatureClass::MAGE || pPlayer->getClass() == CreatureClass::LICH || pPlayer->isStaff()) && spellData->how == CastType::CAST)
        adj = std::min<int>(4, (spellData->level / 5));

    object->setAdjustment(std::max<int>(adj, object->getAdjustment()));

    if(object->getType() == ObjectType::WEAPON) {
        object->setShotsMax(object->getShotsMax() + adj * 10);
        object->incShotsCur(adj * 10);
    }
    object->value.set(500 * adj, GOLD);

    pPlayer->printColor("%O begins to glow brightly.\n", object.get());
    broadcast(pPlayer->getSock(), pPlayer->getRoomParent(), "%M enchants %1P.", pPlayer.get(), object.get());

    if(!pPlayer->isDm())
        log_immort(true, pPlayer, "%s enchants a %s^g in room %s.\n", pPlayer->getCName(), object->getCName(),
            pPlayer->getRoomParent()->fullName().c_str());

    return(1);
}


//*********************************************************************
//                      cmdEnchant
//*********************************************************************
// This function enables mages to randomly enchant an item
// for a short period of time.

int cmdEnchant(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Object>  object=nullptr;
    int     dur;

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(!player->knowsSkill("enchant") || player->hasSecondClass()) {
        player->print("You lack the training to enchant objects.\n");
        return(0);
    }

    if(player->isBlind()) {
        player->printColor("^CYou can't do that! You're blind!\n");
        return(0);
    }
    if(cmnd->num < 2) {
        player->print("Enchant what?\n");
        return(0);
    }

    object = player->findObject(player, cmnd, 1);
    if(!object) {
        player->print("You don't have that item in your inventory.\n");
        return(0);
    }

    if(!canEnchant(player, object))
        return(0);

// I do suppose temp enchant sucks enough that we can let it go without daily limits -Bane

//  if(!dec_daily(&player->daily[DL_ENCHA]) && !player->isCt()) {
//      player->print("You have enchanted enough today.\n");
//      return(0);
//  }


    dur = std::max(600, std::min(7200, (int)(player->getSkillLevel("enchant")*100) + bonus(player->intelligence.getCur()) * 100));
    object->lasttime[LT_ENCHA].interval = dur;
    object->lasttime[LT_ENCHA].ltime = time(nullptr);

    object->randomEnchant((int)player->getSkillLevel("enchant")/2);

    player->printColor("%O begins to glow brightly.\n", object.get());
    broadcast(player->getSock(), player->getParent(), "%M enchants %1P.", player.get(), object.get());
    player->checkImprove("enchant", true);
    if(!player->isDm())
        log_immort(true, player, "%s temp_enchants a %s in room %s.\n", player->getCName(), object->getCName(),
            player->getRoomParent()->fullName().c_str());

    object->setFlag(O_TEMP_ENCHANT);
    return(0);
}

//*********************************************************************
//                      splStun
//*********************************************************************

int splStun(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Player> pPlayer = player->getAsPlayer();
    std::shared_ptr<Creature> target=nullptr;
    std::shared_ptr<Monster> mTarget=nullptr;
    int     dur=0, bns=0, mageStunBns=0;

    if(pPlayer)
        pPlayer->computeLuck();     // luck for stun

    player->smashInvis();

    if( player->getClass() == CreatureClass::LICH ||
         (pPlayer && pPlayer->getClass() == CreatureClass::MAGE && !pPlayer->hasSecondClass()) ||
         player->isCt())
        mageStunBns = Random::get(1,5);

    if((target = player->findMagicVictim(cmnd->str[2], cmnd->val[2], spellData, true, false, "Cast on what?\n", "You don't see that here.\n")) == nullptr)
        return(0);

    // stun self
    if(target == player) {

        if(spellData->how == CastType::CAST) {
            dur = bonus(player->intelligence.getCur()) * 2 +
                  ((player->getAsPlayer())->getLuck() / 10) + mageStunBns;
        } else
            dur = dice(2, 6, 0);

        dur = std::max(5, dur);
        player->updateAttackTimer(true, dur*10);

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("You're stunned.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts a stun spell on %sself.", player.get(), player->himHer());
        } else if(spellData->how == CastType::POTION)
            player->print("You feel dizzy.\n");

    // Stun a monster or player
    } else {
        mTarget = target->getAsMonster();

        if(mTarget) {
            if(player->flagIsSet(P_LAG_PROTECTION_SET))
                player->setFlag(P_LAG_PROTECTION_ACTIVE);
        }

        if(!player->canAttack(target))
            return(0);

        if(mTarget) {
            if(Random::get(1,100) < mTarget->getMagicResistance()) {
                player->printColor("^MYour spell has no effect on %N.\n", mTarget.get());
                if(player->isPlayer())
                    broadcast(player->getSock(), player->getParent(), "%M's spell has no effect on %N.", player.get(), mTarget.get());
                return(0);
            }

        }


        if(mTarget && player->isPlayer()) {
            if( !mTarget->flagIsSet(M_RESTRICT_EXP_LEVEL) &&
                mTarget->flagIsSet(M_LEVEL_RESTRICTED) &&
                mTarget->getMaxLevel() &&
                !mTarget->flagIsSet(M_AGGRESSIVE) &&
                !mTarget->isEnemy(player)
            ) {
                if( spellData->level > mTarget->getMaxLevel() &&
                    !player->checkStaff("You shouldn't be picking on %s.\n", mTarget->himHer())
                )
                    return(0);
            }
        }


        if(spellData->how == CastType::CAST) {
            dur = bonus(player->intelligence.getCur()) * 2 + dice(2, 6, 0) + mageStunBns;
        } else
            dur = dice(2, 5, 0);

        if(player->isPlayer() && !mTarget) {
            dur = ((spellData->level - target->getLevel()) +
                    (bonus(player->intelligence.getCur()) -
                     bonus(target->intelligence.getCur())));
            if(dur < 3)
                dur = 3;

            dur += mageStunBns;

            if(target->isEffected("resist-magic") || target->flagIsSet(P_RESIST_STUN))
                dur= 3;

        }

        if( target->isEffected("resist-magic") ||
            target->flagIsSet(mTarget ? M_RESIST_STUN_SPELL : P_RESIST_STUN))
        {
            if(spellData->how != CastType::CAST)
                dur = 0;
            else
                dur = 3;
        } else
            dur = std::max(5, dur);

        if(mTarget) {

            if(mTarget->flagIsSet(M_MAGE_LICH_STUN_ONLY) && player->getClass() !=  CreatureClass::MAGE && player->getClass() !=  CreatureClass::LICH && !player->isStaff()) {
                player->print("Your training in magic is insufficient to properly stun %N!\n", mTarget.get());
                if(spellData->how != CastType::CAST)
                dur = 0;
                else
                    dur = 3;
            }

            if( player->isPlayer() &&
                !mTarget->isEffected("resist-magic") &&
                !mTarget->flagIsSet(M_RESIST_STUN_SPELL) &&
                mTarget->flagIsSet(M_LEVEL_BASED_STUN)
            ) {
                if(((int)mTarget->getLevel() - (int)spellData->level) > ((player->getClass() == CreatureClass::LICH || player->getClass() == CreatureClass::MAGE) ? 6:4))
                    dur = std::max(3, bonus(player->intelligence.getCur()));
                else
                    dur = std::max(5, dur);
            }

            if(player->isEffected("reflect-magic") && mTarget->isEffected("reflect-magic")) {
                player->print("Your stun is cancelled by both your magical-shields!\n");
                mTarget->addEnemy(player);
                return(1);
            }
            if(mTarget->isEffected("reflect-magic")) {
                player->print("Your stun is reflected back at you!\n");
                broadcast(player->getSock(), mTarget->getSock(), player->getRoomParent(), "%M's stun is reflected back at %s!", player.get(), player->himHer());
                if(mTarget->isDm() && !player->isDm())
                    dur = 0;
                player->stun(dur);
                mTarget->addEnemy(player);
                return(1);
            }

        } else {
            if(player->isEffected("reflect-magic") && target->isEffected("reflect-magic")) {
                player->print("Your stun is cancelled by both your magical-shields!\n");
                target->print("Your magic-shield reflected %N's stun!\n", player.get());
                return(1);
            }
            if(target->isEffected("reflect-magic")) {
                player->print("Your stun is reflected back at you!\n");
                target->print("%M's stun is reflected back at %s!\n", player.get(), player->himHer());
                if(player->flagIsSet(P_RESIST_STUN) || player->isEffected("resist-magic"))
                    dur = 3;
                broadcast(player->getSock(), target->getSock(), player->getParent(), "%M's stun is reflected back at %s!", player.get(), player->himHer());
                if(target->isDm() && !player->isDm())
                    dur = 0;
                player->stun(dur);
                return(1);
            }
        }

        if(player->isMonster() && !mTarget && target->isEffected("reflect-magic")) {
            if(player->isEffected("reflect-magic")) {
                target->print("Your magic-shield reflects %N's stun!\n", player.get());
                target->print("%M's magic-shield cancels it!\n", player.get());
                return(1);
            }
            if(player->flagIsSet(M_RESIST_STUN_SPELL) || player->isEffected("resist-magic"))
                dur = 3;
            target->print("%M's stun is reflected back at %s!\n", player.get(), player->himHer());
            if(target->isDm() && !player->isDm())
                dur = 0;
            player->stun(dur);
            return(1);
        }

        if(!mTarget && player->isEffected("berserk"))
            dur /= 2;

        if(target->isDm() && !player->isDm())
            dur = 0;

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Stun cast on %s.\n", target->getCName());
            broadcast(player->getSock(), target->getSock(), player->getParent(),
                "%M casts stun on %N.", player.get(), target.get());

            logCast(player, target, "stun");

            if(mTarget && player->isPlayer()) {
                if(mTarget->flagIsSet(M_YELLED_FOR_HELP) && (Random::get(1,100) <= (std::max(25, mTarget->inUniqueRoom() ? mTarget->getUniqueRoomParent()->wander.getTraffic() : 25)))) {
                    mTarget->summonMobs(player);
                    mTarget->clearFlag(M_YELLED_FOR_HELP);
                    mTarget->setFlag(M_WILL_YELL_FOR_HELP);
                }

                if(mTarget->flagIsSet(M_WILL_YELL_FOR_HELP) && !mTarget->flagIsSet(M_YELLED_FOR_HELP)) {
                    mTarget->checkForYell(player);
                }

                if(mTarget->flagIsSet(M_LEVEL_BASED_STUN) && (((int)mTarget->getLevel() - (int)spellData->level) > ((player->getClass() == CreatureClass::LICH || player->getClass() == CreatureClass::MAGE) ? 6:4))) {
                    player->printColor("^yYour magic is currently too weak to fully stun %N!\n", mTarget.get());

                    switch(Random::get(1,9)) {
                    case 1:
                        player->print("%M laughs wickedly at you.\n", target.get());
                        break;
                    case 2:
                        player->print("%M shrugs off your weak attack.\n", target.get());
                        break;
                    case 3:
                        player->print("%M glares menacingly at you.\n", target.get());
                        break;
                    case 4:
                        player->print("%M grins defiantly at your weak stun magic.\n", target.get());
                        break;
                    case 5:
                        player->print("What were you thinking?\n");
                        break;
                    case 6:
                        player->print("Uh oh...you're in trouble now.\n");
                        break;
                    case 7:
                        player->print("%M says, \"That tickled. Now it's my turn...\"\n", target.get());
                        break;
                    case 8:
                        player->print("I hope you're prepared to meet your destiny.\n");
                        break;
                    case 9:
                        player->print("Your stun just seemed to bounce off. Strange....\n");
                        break;
                    default:
                        break;
                    }
                }
            }

            player->wake("You awaken suddenly!");
            target->print("%M stunned you.\n", player.get());


            if((!(mTarget && mTarget->flagIsSet(M_RESIST_STUN_SPELL)) && (spellData->level <= target->getLevel())
                && !target->isCt())
            ) {

                if(mTarget && mTarget->flagIsSet(M_PERMANENT_MONSTER))
                    bns = 10;

                if(!mTarget && target->getClass() == CreatureClass::CLERIC && target->getDeity() == ARES)
                    bns += 25;

                if(target->chkSave(SPL, player, bns)) {
                    player->printColor("^y%M avoided the full power of your stun!\n", target.get());
                    if(target->isPlayer())
                        broadcast(player->getSock(), player->getParent(), "%M avoided a full stun.", target.get());
                    dur /= 2;
                    dur = std::min(5,dur);
                    target->print("You avoided a full stun.\n");
                }
            }

            target->stun(dur);
        }

        if(mTarget)
            mTarget->addEnemy(player);
    }

    return(1);
}

//*********************************************************************
//                      splGlobeOfSilence
//*********************************************************************

int splGlobeOfSilence(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    int strength = 1;
    long duration = 600;

    if(player->noPotion( spellData))
        return(0);

    if(player->getRoomParent()->isPkSafe() && !player->isCt()) {
        player->print("That spell is not allowed here.\n");
        return(0);
    }

    player->print("You cast a globe of silence spell.\n");
    broadcast(player->getSock(), player->getParent(), "%M casts a globe of silence spell.", player.get());

    if(player->getRoomParent()->hasPermEffect("globe-of-silence")) {
        player->print("The spell didn't take hold.\n");
        return(0);
    }

    if(spellData->how == CastType::CAST) {
        if(player->getRoomParent()->magicBonus())
            player->print("The room's magical properties increase the power of your spell.\n");
    }

    player->getRoomParent()->addEffect("globe-of-silence", duration, strength, player, true, player);
    return(1);
}
