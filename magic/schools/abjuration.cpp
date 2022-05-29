/*
 * abjuration.cpp
 *   Abjuration spells
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

#include <cmath>                     // for pow
#include <cstdlib>                   // for abs
#include <ctime>                     // for time
#include <list>                      // for list, operator==, _List_const_it...
#include <string>                    // for allocator, string, basic_string

#include "cmd.hpp"                   // for cmd
#include "config.hpp"                // for Config, gConfig
#include "damage.hpp"                // for Damage, REFLECTED_MAGIC, REFLECT...
#include "deityData.hpp"             // for DeityData
#include "effects.hpp"               // for EffectInfo
#include "flags.hpp"                 // for P_DM_SILENCED, P_FREE_ACTION
#include "global.hpp"                // for CastType, CreatureClass, CastTyp...
#include "lasttime.hpp"              // for lasttime
#include "magic.hpp"                 // for SpellData, splGeneric, checkRefu...
#include "move.hpp"                  // for deletePortal
#include "mud.hpp"                   // for LT_FREE_ACTION
#include "mudObjects/container.hpp"  // for Container
#include "mudObjects/creatures.hpp"  // for Creature, NO_CHECK, PetList
#include "mudObjects/exits.hpp"      // for Exit
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for broadcast, bonus, up, broadcastG...
#include "random.hpp"                // for Random
#include "server.hpp"                // for Server, gServer
#include "statistics.hpp"            // for Statistics
#include "stats.hpp"                 // for Stat


//*********************************************************************
//                      protection from room damage spells
//*********************************************************************

int splHeatProtection(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "heat-protection", "heat-protection"));
}
int splEarthShield(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "an", "earth-shield", "earth-shield"));
}
int splBreatheWater(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "breathe-water", "breathe-water"));
}
int splWarmth(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "warmth", "warmth", true));
}
int splWindProtection(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "wind-protection", "wind-protection"));
}
int splStaticField(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "static-field", "static-field"));
}


//*********************************************************************
//                      splProtection
//*********************************************************************
// This function allows a spellcaster to cast a protection spell either
// on themself or on another player, improving the armor class by a
// score of 10.

int splProtection(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "protection", "protection"));
}


//*********************************************************************
//                      splUndeadWard
//*********************************************************************

int splUndeadWard(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    if(!player->isCt() && spellData->how == CastType::CAST) {
        if(player->getClass() !=  CreatureClass::CLERIC && player->getClass() !=  CreatureClass::PALADIN ) {
            player->print("Only clerics and paladins may cast that spell.\n");
            return(0);
        }
        if(player->getDeity() != ENOCH && player->getDeity() != LINOTHAN && player->getDeity() != CERIS) {
            player->print("%s will not allow you to cast that spell.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
            return(0);
        }
        if(!player->getDeity()) {
            // this should never happen
            player->print("You must belong to a religion to cast that spell.\n");
            return(0);
        }
    }

    return(splGeneric(player, cmnd, spellData, "an", "undead-ward", "undead-ward"));
}

//*********************************************************************
//                      splBless
//*********************************************************************
// This function allows a player to cast a bless spell on themself or
// on another player, reducing the target's thaco by 1.

int splBless(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "bless", "bless"));
}

//*********************************************************************
//                      splDrainShield
//*********************************************************************

int splDrainShield(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(splGeneric(player, cmnd, spellData, "a", "drain-shield", "drain-shield"));
}

//*********************************************************************
//                      reflect magic spells
//*********************************************************************
// The strength of the spell is the chance to reflect

int addReflectMagic(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData, const char* article, const char* spell, int strength, int level) {
    if(spellData->how == CastType::CAST) {
        if(!player->isMageLich())
            return(0);
        if(spellData->level < level) {
            player->print("You are not powerful enough to cast this spell.\n");
            return(0);
        }
    }
    return(splGeneric(player, cmnd, spellData, article, spell, "reflect-magic", strength));
}

int splBounceMagic(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(addReflectMagic(player, cmnd, spellData, "a", "bounce magic", 33, 9));
}
int splReboundMagic(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(addReflectMagic(player, cmnd, spellData, "a", "rebound magic", 66, 16));
}
int splReflectMagic(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(addReflectMagic(player, cmnd, spellData, "a", "reflect-magic", 100, 21));
}

//*********************************************************************
//                      splResistMagic
//*********************************************************************
// This function allows players to cast the resist-magic spell. It
// will allow the player to resist magical attacks from monsters

int splResistMagic(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    int strength=0;

    if(spellData->how == CastType::CAST && !player->isStaff()) {
        if( player->getClass() !=  CreatureClass::MAGE &&
            player->getClass() !=  CreatureClass::LICH
        ) {
            player->print("Only mages and liches may cast this spell.\n");
            return(0);
        }
        if(spellData->level < 16) {
            player->print("You are not experienced enough to cast that spell.\n");
            return(0);
        }
    }

    if(spellData->how == CastType::CAST)
        strength = spellData->level;

    return(splGeneric(player, cmnd, spellData, "a", "resist-magic", "resist-magic", strength));
}


//*********************************************************************
//                      splFreeAction
//*********************************************************************

int splFreeAction(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;
    std::shared_ptr<Player> pTarget=nullptr;
    long    t = time(nullptr);

    if(spellData->how != CastType::POTION && !player->isCt() && player->getClass() !=  CreatureClass::DRUID && player->getClass() !=  CreatureClass::CLERIC) {
        player->print("Your class is unable to cast that spell.\n");
        return(0);
    }


    if(cmnd->num == 2) {
        target = player;

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("Free-action spell cast.\nYou can now move freely.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts a free-action spell on %sself.", player.get(), player->himHer());
        } else if(spellData->how == CastType::POTION)
            player->print("You can now move freely.\n");

    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);

        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target || (target && target->isMonster())) {
            player->print("You don't see that person here.\n");
            return(0);
        }

        if(checkRefusingMagic(player, target))
            return(0);

        player->print("Free-action cast on %s.\n", target->getCName());
        target->print("%M casts a free-action spell on you.\n%s", player.get(), "You can now move freely.\n");
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a free-action spell on %N.",player.get(), target.get());
    }

    pTarget = target->getAsPlayer();

    if(pTarget) {
        pTarget->setFlag(P_FREE_ACTION);

        pTarget->lasttime[LT_FREE_ACTION].ltime = t;
        if(spellData->how == CastType::CAST) {
            pTarget->lasttime[LT_FREE_ACTION].interval = std::max(300, 900 +
                                                                  bonus(player->intelligence.getCur()) * 300);

            if(player->getRoomParent()->magicBonus()) {
                pTarget->print("The room's magical properties increase the power of your spell.\n");
                pTarget->lasttime[LT_FREE_ACTION].interval += 300L;
            }
        } else
            pTarget->lasttime[LT_FREE_ACTION].interval = 600;

        pTarget->clearFlag(P_STUNNED);
        pTarget->removeEffect("hold-person");
        pTarget->removeEffect("slow");

        pTarget->computeAC();
        pTarget->computeAttackPower();
    }
    return(1);
}

//*********************************************************************
//                      splRemoveFear
//*********************************************************************

int splRemoveFear(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;

    if(cmnd->num == 2) {
        target = player;

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {

            player->print("You cast remove-fear on yourself.\n");
            if(player->isEffected("fear"))
                player->print("You feel brave again.\n");
            else
                player->print("Nothing happens.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts remove-fear on %sself.", player.get(), player->himHer());
        } else if(spellData->how == CastType::POTION && player->isEffected("fear"))
            player->print("You feel brave again.\n");
        else if(spellData->how == CastType::POTION)
            player->print("Nothing happens.\n");

    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);
        if(!target) {
            player->print("That's not here.\n");
            return(0);
        }


        if(checkRefusingMagic(player, target))
            return(0);


        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("You cast remove-fear on %N.\n", target.get());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts remove-fear on %N.", player.get(), target.get());
            if(target->isPlayer()) {
                target->print("%M casts remove-fear on you.\n", player.get());
                if(target->isEffected("fear"))
                    target->print("You feel brave again.\n");
                else
                    player->print("Nothing happens.\n");
            }
        }
    }
    if(target->inCombat(false))
        player->smashInvis();

    target->removeEffect("fear", true, false);
    return(1);
}

//*********************************************************************
//                      splRemoveSilence
//*********************************************************************

int splRemoveSilence(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> target=nullptr;

    if(cmnd->num == 2) {
        target = player;
        if(spellData->how == CastType::POTION && player->isEffected("silence") && !player->flagIsSet(P_DM_SILENCED))
            player->print("You can speak again.\n");
        else {
            player->print("Nothing happens.\n");
            return(0);
        }

    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            player->print("That's not here.\n");
            return(0);
        }

        if(checkRefusingMagic(player, target))
            return(0);

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("You cast remove-silence on %N.\n", target.get());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts remove-silence on %N.", player.get(), target.get());

            logCast(player, target, "remove-silence");
            if(target->isPlayer()) {
                target->print("%M casts remove-silence on you.\n", player.get());
                if( (target->flagIsSet(P_DM_SILENCED) && player->isCt()) ||
                     !target->flagIsSet(P_DM_SILENCED)
                )
                    target->print("You can speak again.\n");
                else
                    target->print("Nothing happens.\n");
            }
        }
    }
    if(target->inCombat(false))
        player->smashInvis();

    target->removeEffect("silence");

    if(player->isCt() && target->isPlayer())
        target->clearFlag(P_DM_SILENCED);

    return(1);
}

//*********************************************************************
//                      splDispelAlign
//*********************************************************************

int splDispelAlign(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData, const char* spell, int align) {
    std::shared_ptr<Creature> target=nullptr;
    Damage damage;


    if((target = player->findMagicVictim(cmnd->str[2], cmnd->val[2], spellData, true, false, "Cast on what?\n", "You don't see that here.\n")) == nullptr)
        return(0);

    if(player == target) {
        // mobs can't cast on themselves!
        if(!player->getAsPlayer())
            return(0);

        // turn into a boolean!
        // are they in alignment?
        if(align == PINKISH)
            align = player->getAlignment() <= align;
        else
            align = player->getAlignment() >= align;

        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND)
            player->print("You cannot cast that spell on yourself.\n");
        else if(spellData->how == CastType::POTION && !align)
            player->print("Nothing happens.\n");
        else if(spellData->how == CastType::POTION && align) {
            player->smashInvis();

            damage.set(Random::get(spellData->level * 2, spellData->level * 4));
            damage.add(std::max(0, bonus(player->piety.getCur())));
            player->modifyDamage(player, MAGICAL, damage);

            player->print("You feel as if your soul is savagely ripped apart!\n");
            player->printColor("You take %s%d^x points of damage!\n", player->customColorize("*CC:DAMAGE*").c_str(), damage.get());
            broadcast(player->getSock(), player->getParent(), "%M screams and doubles over in pain!", player.get());
            player->hp.decrease(damage.get());
            if(player->hp.getCur() < 1) {
                player->print("Your body explodes. You're dead!\n");
                broadcast(player->getSock(), player->getParent(), "%M's body explodes! %s's dead!", player.get(), player->upHeShe());
                player->getAsPlayer()->die(EXPLODED);
            }
        }

    } else {
        if(player->spellFail( spellData->how))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            player->print("That's not here.\n");
            return(0);
        }

        if(!player->canAttack(target))
            return(0);

        if( (target->getLevel() > spellData->level + 7) ||
            (!player->isStaff() && target->isStaff())
        ) {
            player->print("Your magic would be too weak to harm %N.\n", target.get());
            return(0);
        }

        player->smashInvis();
        // turn into a boolean!
        // are they in alignment?
        if(align == PINKISH)
            align = target->getAlignment() <= align;
        else
            align = target->getAlignment() >= align;



        if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
            player->print("You cast %s on %N.\n", spell, target.get());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a %s spell on %N.", player.get(), spell, target.get());

            if(target->getAsPlayer()) {
                target->wake("Terrible nightmares disturb your sleep!");
                target->print("%M casts a %s spell on you.\n", player.get(), spell);
            }

            if(!align) {
                player->print("Nothing happens.\n");
            } else {
                damage.set(Random::get(spellData->level * 2, spellData->level * 4));
                damage.add(abs(player->getAlignment()/100));
                damage.add(std::max(0, bonus(player->piety.getCur())));
                target->modifyDamage(player, MAGICAL, damage);

                if(target->chkSave(SPL, player,0))
                    damage.set(damage.get() / 2);

                player->printColor("The spell did %s%d^x damage.\n", player->customColorize("*CC:DAMAGE*").c_str(), damage.get());
                player->print("%M screams in agony as %s soul is ripped apart!\n", target.get(), target->hisHer());
                target->printColor("You take %s%d^x points of damage as your soul is ripped apart!\n", target->customColorize("*CC:DAMAGE*").c_str(), damage.get());
                broadcastGroup(false, target, "%M cast a %s spell on ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n", player.get(), spell, target.get(), damage.get(), target->heShe(), target->getStatusStr(damage.get()));

                broadcast(player->getSock(), target->getSock(), player->getParent(), "%M screams and doubles over in pain!", target.get());

                // if the target is reflecting magic, force printing a message (it will say 0 damage)
                player->doReflectionDamage(damage, target, target->isEffected("reflect-magic") ? REFLECTED_MAGIC : REFLECTED_NONE);

                if(spellData->how == CastType::CAST && player->isPlayer())
                    player->getAsPlayer()->statistics.offensiveCast();

                player->doDamage(target, damage.get(), NO_CHECK);
                if(target->hp.getCur() < 1) {
                    target->print("Your body explodes! You die!\n");
                    broadcast(player->getSock(), target->getSock(), player->getParent(), "%M's body explodes! %s's dead!", target.get(), target->upHeShe());
                    player->print("%M's body explodes! %s's dead!\n", target.get(), target->upHeShe());
                    target->die(player);
                    return(2);
                }
            }

        }
    }

    return(1);
}

//*********************************************************************
//                      splDispelEvil
//*********************************************************************

int splDispelEvil(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {

    if(spellData->how != CastType::POTION && !player->isStaff()) {
        if(player->getClass() !=  CreatureClass::CLERIC && player->getClass() !=  CreatureClass::PALADIN && player->isPlayer()) {
            player->print("Your class is unable to cast that spell.\n");
            return(0);
        }
        if(player->getClass() == CreatureClass::CLERIC && player->getDeity() != ENOCH && player->getDeity() != LINOTHAN) {
            player->print("%s will not allow you to cast that spell.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
            return(0);
        }
        if(player->getAlignment() < LIGHTBLUE) {
            player->print("You have to be good to cast that spell.\n");
            return(0);
        }
    }

    return(splDispelAlign(player, cmnd, spellData, "dispel-evil", PINKISH));
}

//*********************************************************************
//                      splDispelGood
//*********************************************************************

int splDispelGood(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {

    if(spellData->how != CastType::POTION && !player->isStaff()) {
        if(player->getClass() !=  CreatureClass::CLERIC && player->getClass() !=  CreatureClass::DEATHKNIGHT && player->isPlayer()) {
            player->print("Your class is unable to cast that spell.\n");
            return(0);
        }
        if(player->getClass() == CreatureClass::CLERIC && player->getDeity() != ARAMON && player->getDeity() != ARACHNUS) {
            player->print("Your deity will not allow you to cast that vile spell.\n");
            return(0);
        }
        if(player->getAlignment() > PINKISH) {
            player->print("You have to be evil to cast that spell.\n");
            return(0);
        }
    }

    return(splDispelAlign(player, cmnd, spellData, "dispel-good", LIGHTBLUE));
}

//*********************************************************************
//                      splArmor
//*********************************************************************
// This spell allows a mage to cast an armor shield around themself which gives themself an
// AC of 6. It sets a pool of virtual HP to effect.strength which is equal to the caster's
// max HP. The armor can take that much damage before dispelling, or its time can run out.
// This spell does NOT absorb damage, it only protects by giving better AC. -TC

int splArmor(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Player> pPlayer = player->getAsPlayer();
    int mpNeeded=0;

    if(!pPlayer)
        return(0);

    mpNeeded = spellData->level;

    if(spellData->how == CastType::CAST && !pPlayer->checkMp(mpNeeded))
        return(0);

    if(!pPlayer->isCt()) {
        if(pPlayer->getClass() !=  CreatureClass::MAGE && pPlayer->getClass() != CreatureClass::LICH && pPlayer->getSecondClass() != CreatureClass::MAGE) {
            if(spellData->how == CastType::CAST) {
                player->print("The arcane nature of that spell eludes you.\nOnly mages, multi-class mages, and liches can cast the armor spell.\n");
                return(0);
            } else {
                player->print("Nothing happens.\n");
                return(0);
            }
        }
    }


    bool multi=false;

    multi = ( (pPlayer->getClass() == CreatureClass::MAGE && (pPlayer->getSecondClass() == CreatureClass::THIEF || pPlayer->getSecondClass() == CreatureClass::ASSASSIN)) ||
         (pPlayer->getClass() == CreatureClass::FIGHTER && pPlayer->getSecondClass() == CreatureClass::MAGE) ||
         (pPlayer->getClass() == CreatureClass::THIEF && pPlayer->getSecondClass() == CreatureClass::MAGE) );
        


    if(pPlayer->spellFail( spellData->how)) {
        if(spellData->how == CastType::CAST)
            pPlayer->subMp(mpNeeded);
        return(0);
    }

    // Cast armor on self

    int strength = 0;
    int duration = 0;
    if(spellData->how == CastType::CAST) {
        duration = 1800 + bonus(pPlayer->intelligence.getCur());

        if(spellData->how == CastType::CAST)
            pPlayer->subMp(mpNeeded);
        if(pPlayer->getRoomParent()->magicBonus()) {
            player->print("The room's magical properties increase the power of your spell.\n");
            duration += 600L;
        }

        //Armor effect strength (vHp):
        //For Multi-class mage = current hp
        //For Lich = current hp + 30% (since they have higher HP, and can also regen in combat, and have drain life)
        //For Mage = current hp * 2
        if(multi)
            strength = pPlayer->hp.getCur();
        else if (pPlayer->getClass() == CreatureClass::LICH)
            strength = mpNeeded + (std::max<int>(1,(pPlayer->hp.getCur() + ((pPlayer->hp.getCur()*30)/100))));
        else // we have a mage or staff
            strength = pPlayer->hp.getCur()*2;
    } else { // everybody else, strength is current hp
        duration = 600L;
        strength = pPlayer->hp.getCur();
    }

    pPlayer->addEffect("armor", duration, strength, player, true, player);
    pPlayer->computeAC();
    pPlayer->printColor("^BYour magical armor will remain until it takes ^C%d^B damage.\n", strength);

    if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
        player->print("Armor spell cast.\n");
        broadcast(pPlayer->getSock(), pPlayer->getRoomParent(),"%M casts an armor spell on %sself.", pPlayer.get(), pPlayer->himHer());
    }

    return(1);
}

//*********************************************************************
//                      splStoneskin
//*********************************************************************
// This spell allows a mage or lich to use magic to make their skin
// stone-hard to ward off physical attacks.

int splStoneskin(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Player> pPlayer = player->getAsPlayer();
    int     mpNeeded=0, multi=0;

    if(player->getClass() !=  CreatureClass::LICH)
        mpNeeded = player->mp.getMax()/2;
    else
        mpNeeded = player->hp.getMax()/2;

    if(spellData->how == CastType::CAST && !player->checkMp(mpNeeded))
        return(0);

    if(!player->isCt()) {
        if(player->getClass() !=  CreatureClass::LICH && player->getClass() !=  CreatureClass::MAGE && (!pPlayer || pPlayer->getSecondClass() != CreatureClass::MAGE)) {
            if(spellData->how == CastType::CAST) {
                player->print("The arcane nature of that spell eludes you.\n");
                return(0);
            } else {
                player->print("Nothing happens.\n");
                return(0);
            }
        }
        if(spellData->how == CastType::CAST && spellData->level < 13) {
            player->print("You are not powerful enough to cast this spell.\n");
            return(0);
        }
    }


    if( pPlayer && (pPlayer->getSecondClass() == CreatureClass::MAGE ||
        (pPlayer->getClass() == CreatureClass::MAGE && pPlayer->hasSecondClass()))
    ) {
        multi=1;
    }

    if(player->spellFail( spellData->how) && spellData->how != CastType::POTION) {
        if(spellData->how == CastType::CAST)
            player->subMp(mpNeeded);
        return(0);
    }

    int duration = 0;
    int strength = 0;
    // Cast stoneskin on self
    if(spellData->how == CastType::CAST) {
        duration = 240 + 10*bonus(player->intelligence.getCur());
        if(spellData->how == CastType::CAST)
            player->subMp(mpNeeded);
        if(player->getRoomParent()->magicBonus()) {
            player->print("The room's magical properties increase the power of your spell.\n");
            duration += 300L;
        }

        if(!multi)
            strength = spellData->level;
        else
            strength = std::max<int>(1, spellData->level-3);
    } else {
        duration = 180L;
        strength = spellData->level/2;
    }

    player->addEffect("stoneskin", duration, strength, player, true, player);

    if(spellData->how == CastType::CAST || spellData->how == CastType::SCROLL || spellData->how == CastType::WAND) {
        player->print("Stoneskin spell cast.\nYou feel impervious.\n");
        broadcast(player->getSock(), player->getParent(),"%M casts a stoneskin spell on %sself.", player.get(), player->himHer());
    } else if(spellData->how == CastType::POTION)
        player->print("You feel impervious.\n");

    return(1);
}

//*********************************************************************
//                      doDispelMagic
//*********************************************************************

int doDispelMagic(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData, const char* spell, int numDispel) {
    std::shared_ptr<Creature> target=nullptr;
    int     chance=0;

    if(spellData->how == CastType::CAST &&
        player->getClass() !=  CreatureClass::MAGE &&
        player->getClass() !=  CreatureClass::LICH &&
        player->getClass() !=  CreatureClass::CLERIC &&
        player->getClass() !=  CreatureClass::DRUID &&
        !player->isStaff()
    ) {
        player->print("Only mages, liches, clerics, and druids may cast that spell.\n");
        return(0);
    }

    if(cmnd->num == 2) {
        target = player;
        broadcast(player->getSock(), player->getParent(), "%M casts a %s spell on %sself.", player.get(), spell, player->himHer());


        player->print("You cast a %s spell on yourself.\n", spell);
        player->print("Your spells begin to dissolve away.\n");


        player->doDispelMagic(numDispel);
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findPlayer(player, cmnd, 2);
        if(!target) {
            // dispel-magic on an exit
            cmnd->str[2][0] = low(cmnd->str[2][0]);
            std::shared_ptr<Exit> exit = findExit(player, cmnd, 2);

            if(exit) {
                player->printColor("You cast a %s spell on the %s^x.\n", spell, exit->getCName());
                broadcast(player->getSock(), player->getParent(), "%M casts a %s spell on the %s^x.", player.get(), spell, exit->getCName());

                if(exit->flagIsSet(X_PORTAL))
                    Move::deletePortal(player->getRoomParent(), exit);
                else
                    exit->doDispelMagic(player->getRoomParent());
                return(0);
            }

            player->print("You don't see that player here.\n");
            return(0);
        }

        if(player->isPlayer() && player->getRoomParent()->isPkSafe() && (!player->isCt()) && !target->flagIsSet(P_OUTLAW)) {
            player->print("That spell is not allowed here.\n");
            return(0);
        }

        if(!target->isEffected("petrification")) {
            if(!player->canAttack(target))
                return(0);
        } else {
            player->print("You cast a %s spell on %N.\n%s body returns to flesh.\n", spell, target.get(), target->upHisHer());
            if(Random::get(1,100) < 50) {

                target->print("%M casts a %s spell on you.\nYour body returns to flesh.\n", player.get(), spell);
                broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a %s spell on %N.\n%s body returns to flesh.\n", player.get(), spell, target.get(), target->upHisHer());
                target->removeEffect("petrification");
                return(1);
            } else {
                target->print("%M casts a dispel-magic spell on you.\n", player.get());
                broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a dispel-magic spell on %N.\n",player.get(), target.get());
                return(0);
            }


        }


        chance = 50 - (10*(bonus(player->intelligence.getCur()) -
                           bonus(target->intelligence.getCur())));

        chance += (spellData->level - target->getLevel())*10;

        chance = std::min(chance, 90);

        if((target->isEffected("resist-magic")) && target->isPlayer())
            chance /=2;




        if( player->isCt() ||
            (Random::get(1,100) <= chance && !target->chkSave(SPL, player, 0)))
        {
            player->print("You cast a %s spell on %N.\n", spell, target.get());

            logCast(player, target, spell);

            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a %s spell on %N.", player.get(), spell, target.get());
            target->print("%M casts a %s spell on you.\n", player.get(), spell);
            target->print("Your spells begin to dissolve away.\n");


            target->doDispelMagic(numDispel);

            for(const auto& pet : target->pets) {
                if(pet) {
                    if(player->isCt() || !pet->chkSave(SPL, player, 0)) {
                        player->print("Your spell bansished %N's %s!\n", target.get(), pet->getCName());
                        target->print("%M's spell banished your %s!\n%M fades away.\n", player.get(), pet->getCName(), pet.get());
                        gServer->delActive(pet.get());
                        pet->die(pet->getMaster());
                    }
                }
            }
        } else {
            player->printColor("^yYour spell fails against %N.\n", target.get());
            target->print("%M tried to cast a dispel-magic spell on you.\n", player.get());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M tried to cast a dispel-magic spell on %N.", player.get(), target.get());
            return(0);
        }
    }
    return(1);
}

//*********************************************************************
//                      dispel-magic spells
//*********************************************************************

int splCancelMagic(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(doDispelMagic(player, cmnd, spellData, "cancel-magic", Random::get(1,2)));
}

int splDispelMagic(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(doDispelMagic(player, cmnd, spellData, "dispel-magic", Random::get(3,5)));
}

int splAnnulMagic(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(doDispelMagic(player, cmnd, spellData, "annul-magic", -1));
}

//********************************************************************
//                      doDispelMagic
//********************************************************************
// num = -1 means dispel all effects

static const std::list<std::string> dispellableEffects = {
    "anchor",
    "hold-person",
    "strength",
    "haste",
    "fortitude",
    "insight",
    "prayer",
    "enfeeblement",
    "slow",
    "weakness",
    "prayer",
    "damnation",
    "confusion",

    "enlarge",
    "reduce",
    "darkness",
    "infravision",
    "invisibility",
    "detect-invisible",
    "undead-ward",
    "bless",
    "detect-magic",
    "protection",
    "tongues",
    "comprehend-languages",
    "true-sight",
    "fly",
    "levitate",
    "drain-shield",
    "resist-magic",
    "know-aura",
    "warmth",
    "breathe-water",
    "earth-shield",
    "reflect-magic",
    "camouflage",
    "heat-protection",
    "farsight",
    "pass-without-trace",
    "resist-earth",
    "resist-air",
    "resist-fire",
    "resist-water",
    "resist-cold",
    "resist-electricity",
    "wind-protection",
    "static-field",

    "illusion",
    "blur",
    "fire-shield",
    "greater-invisibility",

};

void Creature::doDispelMagic(int num) {
    EffectInfo* effect=nullptr;
    std::list<std::string>::const_iterator it;

    // create a list of possible effects

    // true we'll show, false don't remove perm effects

    // num = -1 means dispel all effects
    if(num <= -1) {
        for(auto& effectName : dispellableEffects)
            removeEffect(effectName, true, false);
        return;
    }

    int numEffects=0, choice=0;
    while(num > 0) {
        // how many effects are we under?
        for(auto& effectName: dispellableEffects) {
            effect = getEffect(effectName);
            if(effect && !effect->isPermanent())
                numEffects++;
        }

        // none? we're done
        if(!numEffects)
            return;
        // which effect shall we dispel?
        choice = Random::get(1, numEffects);
        numEffects = 0;

        // find it and get rid of it!
        for(auto& effectName: dispellableEffects) {
            effect = getEffect(effectName);
            if(effect && !effect->isPermanent()) {
                numEffects++;
                if(choice == numEffects) {
                    removeEffect(*it, true, false);
                    // stop the loop!
                    choice = 0;
                    break;
                }
            }
        }

        num--;
    }
}

//*********************************************************************
//                      doDispelMagic
//*********************************************************************

void Exit::doDispelMagic(const std::shared_ptr<BaseRoom>& parent) {
    bringDownTheWall(getEffect("wall-of-fire"), parent, shared_from_this());
    bringDownTheWall(getEffect("wall-of-thorns"), parent, shared_from_this());

    // true we'll show, false don't remove perm effects
    removeEffect("concealed", true, false);
}

//*********************************************************************
//                      fire shield spells
//*********************************************************************
// This adds the actual fire-shield effect, include mage-lich restrict and
// strength of the spell. Strength and level restrict follow this chart:
//  1=1, 2=4, 3=9, 4=16

int addFireShield(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData, const char* article, const char* spell, int strength) {
    if(spellData->how == CastType::CAST) {
        if(!player->isMageLich())
            return(0);
        if(spellData->level < pow(strength, 2)) {
            player->print("You are not powerful enough to cast this spell.\n");
            return(0);
        }
    }

    return(splGeneric(player, cmnd, spellData, article, spell, "fire-shield", (int)pow(strength, 2)));
}

int splRadiation(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(addFireShield(player, cmnd, spellData, "a", "radiation", 1));
}
int splFieryRetribution(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(addFireShield(player, cmnd, spellData, "a", "fiery retribution", 2));
}
int splAuraOfFlame(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(addFireShield(player, cmnd, spellData, "an", "aura of flame", 3));
}
int splBarrierOfCombustion(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    return(addFireShield(player, cmnd, spellData, "a", "barrier of combustion", 4));
}

//*********************************************************************
//                      doReflectionDamage
//*********************************************************************
// return true if we were killed by the reflection
// passing printZero as REFLECTED_MAGIC or REFLECTED_PHYSICAL will print, even if damage is 0

bool Creature::doReflectionDamage(Damage pDamage, const std::shared_ptr<Creature>& target, ReflectedDamageType printZero) {
    int dmg = pDamage.getReflected() ? pDamage.getReflected() : pDamage.getPhysicalReflected();

    // don't quit if we want to print 0
    if(!dmg && printZero == REFLECTED_NONE)
        return(false);

    if(pDamage.getReflected() || printZero == REFLECTED_MAGIC) {
        target->printColor("Your shield of magic reflects %s%d^x damage.\n", target->customColorize("*CC:DAMAGE*").c_str(), dmg);
        printColor("%M's shield of magic flares up and hits you for %s%d^x damage.\n", target.get(), customColorize("*CC:DAMAGE*").c_str(), dmg);
        broadcastGroup(false, Containable::downcasted_shared_from_this<Creature>(), "%M's shield of magic flares up and hits %N for *CC:DAMAGE*%d^x damage.\n", target.get(), this, dmg);
    } else {
        switch(pDamage.getPhysicalReflectedType()) {
        case REFLECTED_FIRE_SHIELD:
        default:
            target->printColor("Your shield of fire reflects %s%d^x damage.\n", target->customColorize("*CC:DAMAGE*").c_str(), dmg);
            printColor("%M's shield of fire flares up and hits you for %s%d^x damage.\n", target.get(), customColorize("*CC:DAMAGE*").c_str(), dmg);
            broadcastGroup(false, Containable::downcasted_shared_from_this<Creature>(), "%M's shield of fire flares up and hits %N for *CC:DAMAGE*%d^x damage.\n", target.get(), this, dmg);
            break;
        }
    }

    if(pDamage.getDoubleReflected()) {
        Damage reflectedDamage;
        reflectedDamage.setReflected(pDamage.getDoubleReflected());
        target->doReflectionDamage(reflectedDamage, Containable::downcasted_shared_from_this<Creature>());
    }

    // we may have gotten this far for printing reasons (even on zero), but really, go no further
    if(!dmg)
        return(false);

    if(target->isPlayer() && isMonster())
        getAsMonster()->adjustThreat(target, dmg);

    hp.decrease(dmg);
    return hp.getCur() <= 0;
}
