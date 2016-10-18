/*
 * healmagic.cpp
 *   Healing magic routines.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "effects.h"
#include "move.h"
#include "web.h"


//*********************************************************************
//                      healCombatCheck
//*********************************************************************

bool healCombatCheck(Creature* healer, Creature* target, bool print=true) {
    Monster* mTarget = target->getAsMonster();

    if(!mTarget)
        return(true);
    if(mTarget->isPet() && mTarget->getMaster() == healer)
        return(true);
    if(!mTarget->getAsMonster()->nearEnemy())
        return(true);
    if(target->getAsMonster()->isEnemy(healer))
        return(true);

    // if healer is in group with pet, they can heal
    if(healer->getGroup() == mTarget->getGroup())
        return(true);
    if(print)
        healer->print("Not while %N is in combat with someone.\n", target);
    return(false);
}


//*********************************************************************
//                      getHeal
//*********************************************************************

int getHeal(Creature *healer, Creature* target, int spell) {
    Player  *pHealer = healer->getAsPlayer();
    int     level=0, statBns=0, base=0, mod=0, heal=0, dmg=0;
    int     vigPet=0;

    if(target) {
        /* Mobs with intelligence > 12 do not like people vigging their enemies and
           will attack them. -TC */
        if(target != healer && target->isPlayer() && target->inCombat()) {
            for(Monster* mons : target->getRoomParent()->monsters) {
                if(mons->intelligence.getCur() > 120 && mons->getAsMonster()->isEnemy(target))
                    mons->getAsMonster()->addEnemy(healer);

            }
        }

        if(target == healer)
            target = 0;
    }

    level = healer->getLevel();
    if( pHealer &&
        pHealer->getClass() == CLERIC &&
        (   pHealer->getSecondClass() == FIGHTER ||
            pHealer->getSecondClass() == ASSASSIN
        )
    )
        level /= 2;

    if(target)
        vigPet = target->isPet() && target->getMaster() == healer;

    switch(spell) {
    case S_VIGOR:
        base = mrand(1,8); //1d8
        break;
    case S_MEND_WOUNDS:
        base = dice(2,7,0); //2d7
        break;
    case S_REJUVENATE:
        base = dice(1,6,1); // 1d6+1
        break;
    }

    switch(healer->getClass()) {
    case CLERIC:
        statBns = bonus((int)healer->piety.getCur());

        switch(healer->getDeity()) {
        case CERIS:
            if(healer->getAdjustedAlignment() == BLOODRED || healer->getAdjustedAlignment() == ROYALBLUE) {
                healer->print("Your healing is not as effective while so far out of natural balance.\n");
                level /= 2;
            }


            mod = level / 2 + mrand(1, 1 + level / 2);
            if(healer->getAdjustedAlignment() == NEUTRAL) {
                if(spell != S_REJUVENATE)
                    healer->print("Your harmonial balance gains you power with Ceris.\n");
                mod += mrand(1,4);
            }

            if(spell == S_MEND_WOUNDS)
                mod += level / 2;

            if(spell == S_REJUVENATE)
                mod = level / 3 + mrand(1, 1 + level / 3);

            break;
        case ARES:
            level /= 2; // Ares clerics vig as if 1/2 their level

            if(healer->getAdjustedAlignment() >= BLUISH || healer->getAdjustedAlignment() <= REDDISH) {
                healer->print("Your soul is too far out of balance.\n");
                level /= 2;
            }

            mod = level / 2 + mrand(1, 1 + level / 2);

            if(spell == S_MEND_WOUNDS)
                mod += level / 2;

            break;
        case KAMIRA:
            level = (level*3)/4;
            if(healer->getAdjustedAlignment() < PINKISH) {
                healer->print("Being evil at heart is distorting your healing magic.\n");
                level /= 2;
            }
            mod = level / 2 + mrand(1, 1 + level / 2);

            if(spell == S_MEND_WOUNDS)
                mod += level / 2;
            break;
        case JAKAR:
            if(healer->getAdjustedAlignment() != NEUTRAL) {
                healer->print("Your soul is out of balance.\n");
                level /= 2;
            }

            mod = level / 2 + mrand(1, 1 + level / 2);
            if(spell == S_MEND_WOUNDS)
                mod += level / 2;

            mod = (mod*3)/4;
            break;
        }// end deity switch
        break;

    case DRUID:
        statBns = MAX(bonus((int)healer->intelligence.getCur()), bonus((int)healer->constitution.getCur()));
        mod = level/4 + mrand(1, 1 + level / 5);
        break;
    case THIEF:
    case ASSASSIN:
    case ROGUE:
    case FIGHTER:
    case BERSERKER:

    case WEREWOLF:
        statBns = bonus((int)healer->constitution.getCur());
        mod = 0;
        break;
    case MONK:
        statBns = bonus((int)healer->constitution.getCur());
        mod = mrand(1,6);
        break;

    case RANGER:
    case BARD:
    case PUREBLOOD:
        statBns = MAX(bonus((int)healer->piety.getCur()), bonus((int)healer->intelligence.getCur()));
        mod = mrand(1,6);
        break;
    case MAGE:
        statBns = MAX(bonus((int)healer->piety.getCur()), bonus((int)healer->intelligence.getCur()));
        mod = 0;
        break;

    case CARETAKER:
    case DUNGEONMASTER:
        statBns = MAX(bonus((int)healer->piety.getCur()), bonus((int)healer->intelligence.getCur()));
        mod = mrand(level/2, level);
        break;

    default:
        statBns = 0;
        mod = 0;
        break;
    } // end class switch



    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------

    // clerics and paladins of Gradius
    if(healer->getDeity() == GRADIUS) {
        statBns = bonus((int)healer->piety.getCur());

        if(pHealer && !pHealer->alignInOrder()) {
            healer->print("You are out of balance with the earth. Your healing is weakened.\n");
            level /= 2;
        }

        if(target && target->isAntiGradius()) {
            healer->print("%s is very puzzled by your healing beings of vile races.\n", gConfig->getDeity(healer->getDeity())->getName().c_str());
            mod /= 2;
        }

        if(healer->getClass() == CLERIC) {
            mod = level / 2 + mrand(1, 1 + level / 2);
            if(spell == S_MEND_WOUNDS)
                mod += level / 2;
            mod = (mod*3)/4;
        } else {
            if(spell == S_VIGOR)
                mod = level / 3 + mrand(1, 1 + level / 4);
            else
                mod = level / 2 + mrand(1, 1 + level / 3);
        }

    // clerics and paladins of Enoch and Linothan
    } else if(healer->getDeity() == ENOCH || healer->getDeity() == LINOTHAN) {
        statBns = bonus((int)healer->piety.getCur());

        if(pHealer && !pHealer->alignInOrder()) {
            healer->print("Your healing magic is weakened from lack of faith in %s.\n", gConfig->getDeity(healer->getDeity())->getName().c_str());
            level /= 2;
        }

        if(target && target->getAdjustedAlignment() < NEUTRAL && healer != target) {
            healer->print("It concerns %s that you heal the unrighteous and impure of heart.\n", gConfig->getDeity(healer->getDeity())->getName().c_str());
            healer->subAlignment(1);
            mod /= 2;
        }

        if(healer->getClass() == CLERIC) {
            mod = level / 2 + mrand(1, 1 + level / 2);
            if(spell == S_MEND_WOUNDS)
                mod += level / 2;
        } else {
            if(spell == S_VIGOR)
                mod = level / 3 + mrand(1, 1 + level / 4);
            else
                mod = level / 2 + mrand(1, 1 + level / 3);
        }

    // clerics and deathknights of Aramon and Arachnus
    } else if(healer->getDeity() == ARAMON || healer->getDeity() == ARACHNUS) {
        statBns = bonus((int)healer->piety.getCur());

        if(pHealer && !pHealer->alignInOrder())
            level /= 2;

        if(healer->getAdjustedAlignment() >= LIGHTBLUE) {

            healer->print("FOOL! You dare turn from %s's evil ways!\n", gConfig->getDeity(healer->getDeity())->getName().c_str());
            dmg = mrand(1,10);
            healer->print("You are shocked for %d damage by %s's wrath!\n", dmg, gConfig->getDeity(healer->getDeity())->getName().c_str());
            broadcast(healer->getSock(), healer->getRoomParent(), "%M doubles over in pain and wretches.\n", healer);
            healer->hp.decrease(dmg);
            mod = 0;

        } else if(target && healer != target && !vigPet) {

            if(target->getAdjustedAlignment() > LIGHTBLUE) {
                healer->print("How DARE you heal the righteous and good of heart!\n");
                dmg = mrand(1,10);
                healer->print("You are shocked for %d damage by %s's wrath!\n", dmg, gConfig->getDeity(healer->getDeity())->getName().c_str());
                broadcast(healer->getSock(), healer->getRoomParent(), "%M doubles over in pain and wretches.\n", healer);
                healer->hp.decrease(dmg);
                mod = 0;
            } else if(  healer->getDeity() == ARAMON || (
                healer->getDeity() == ARACHNUS &&
                (healer->willIgnoreIllusion() ? target->getRace() : target->getDisplayRace()) != DARKELF
            )) {
                if(healer->getDeity() == ARAMON)
                    healer->print("%s disapproves of healing others than yourself.\n", gConfig->getDeity(healer->getDeity())->getName().c_str());
                else
                    healer->print("%s disapproves of healing non-dark-elves.\n", gConfig->getDeity(healer->getDeity())->getName().c_str());
                mod = mrand(1, (1 + level / 5));
            }

        } else {

            if(healer->getClass() == CLERIC) {
                mod = level / 2 + mrand(1, 1 + level / 2);
                if(spell == S_MEND_WOUNDS)
                    mod += level / 2;
            } else {
                mod = level / 4 + mrand(1, 1 + level / 4);
            }

        }

    }

    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------


    heal = base + statBns + mod;

    if(healer->getRoomParent()->magicBonus()) {
        if(spell == S_MEND_WOUNDS)
            heal += mrand(1, 6);
        else
            heal += mrand(1, 3);
        healer->print("The room's magical properties increase the power of your spell.\n");
    }

    if(target && healer != target) {
        if(target->isPlayer()) {
            if(target->isEffected("stoneskin"))
                heal /= 2;

            if(target->flagIsSet(P_POISONED_BY_PLAYER))
                heal = mrand(1,4);

            if(target->flagIsSet(P_OUTLAW) && target->getRoomParent()->isOutlawSafe())
                heal = mrand(1,3);

        }
    } else {
        if(healer->isEffected("stoneskin"))
            heal /= 2;
        if(healer->flagIsSet(P_OUTLAW) && healer->getRoomParent()->isOutlawSafe())
            heal = mrand (1,3);
    }

    return(MAX(1, heal));
}


void niceExp(Creature *healer, Creature *creature, int heal, int how) {
    Player  *player=0, *target=0;
    int     exp=0;

    if(how != CAST)
        return;

    // only players are allowed to use this function! It's called from creature-spells
    // though, so we have to cast
    player = healer->getAsPlayer();
    target = creature->getAsPlayer();

    if(!player || !target || healer == target)
        return;

    if(player->getDeity() == CERIS || player->getDeity() == ENOCH || player->getDeity() == LINOTHAN)
        exp = MAX(1, heal) / 4;
    else
        exp = MAX(1, heal) / 3;

    if(player->getDeity() == ARAMON)
        exp = 0;

    // also see getHeal for copy of this
    if( player->getDeity() == ARACHNUS && (
        target->getAdjustedAlignment() > LIGHTBLUE ||
        (player->willIgnoreIllusion() ? target->getRace() : target->getDisplayRace()) != DARKELF)
    )
        exp = 0;


    if( exp &&
        (target->hp.getCur() < target->hp.getMax()) &&
        player->halftolevel()
    ) {
        player->addExperience(exp);
        healer->print("You %s %d experience for your deed.\n", gConfig->isAprilFools() ? "lose" : "gain", exp);
    }
}

//*********************************************************************
//                      canCastHealing
//*********************************************************************

bool canCastHealing(Creature* player, Creature* target, bool rej=false, bool healSpell=false, bool print=true) {
    if(!healCombatCheck(player, target, print))
        return(false);

    if(target->isEffected("petrification")) {
        if(print)
            player->print("Your %s magic is ineffective.\n", rej ? "healing" : "rejuvinating");
        return(false);
    }

    // for a lich, a heal spell is offensive magic
    if(target->getClass() == LICH && healSpell)
        return(true);

    if(target->getClass() == LICH) {
        if(print)
            player->print("The aura of death around %N nullifies your %s magic.\n", target, rej ? "healing" : "rejuvinating");
        return(false);
    }
    if(!rej && target->isEffected("stoneskin")) {
        // don't check this for liches
        if(print)
            player->print("%M's stoneskin spell reflects your %s magic.\n", target, rej ? "healing" : "rejuvinating");
        return(false);
    }

    if(checkRefusingMagic(player, target, true, print))
        return(false);

    if(!rej && target->hp.getCur() >= target->hp.getMax()) {
        if(print)
            player->print("%M is at full health right now.\n", target);
        return(false);
    }
    return(true);
}

//*********************************************************************
//                      castHealingSpell
//*********************************************************************

int castHealingSpell(Creature* player, cmd* cmnd, SpellData* spellData, const char* spellName, const char* feelBetter, int flag, int defaultAmount) {
    Creature* target=0;
    char    capSpellName[25];
    int     heal=0;

    strcpy(capSpellName, spellName);
    capSpellName[0] = toupper(capSpellName[0]);

    if(player->getClass() == LICH) {
        player->print("Your class prevents you from casting that spell.\n");
        return(0);
    }


    // Self
    if(cmnd->num == 2) {

        if( player->hp.getCur() >= player->hp.getMax() &&
            !player->checkStaff("You are already at full health right now.\n") )
            return(0);

        if(spellData->how == CAST) {

            if(player->isPlayer())
                player->getAsPlayer()->statistics.healingCast();
            heal = getHeal(player, 0, flag);

        } else
            heal = defaultAmount;

        player->doHeal(player, heal);

        if(spellData->how == CAST || spellData->how == SCROLL) {
            player->print("%s spell cast.\n", capSpellName);
            broadcast(player->getSock(), player->getParent(), "%M casts a %s spell on %sself.",
                player, spellName, player->himHer());
            return(1);
        } else {
            player->print("%s\n", feelBetter);
            return(1);
        }

    // Cast vigor on another player or monster
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);
        if(!target) {
            player->print("That person is not here.\n");
            return(0);
        }

        if(!canCastHealing(player, target))
            return(0);

        if(target->inCombat(false))
            player->smashInvis();

        if(spellData->how == CAST) {
            if(player->isPlayer())
                player->getAsPlayer()->statistics.healingCast();
            heal = getHeal(player, target, flag);
        } else {
            heal = defaultAmount;
        }

        player->doHeal(target, heal);

        if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {
            player->print("%s spell cast on %N.\n", capSpellName, target);
            target->print("%M casts a %s spell on you.\n",  player, spellName);
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a %s spell on %N.",
                player, spellName, target);

            niceExp(player, target, heal, spellData->how);
            return(1);
        }
    }

    return(1);
}

//*********************************************************************
//                      splVigor
//*********************************************************************
// This function will cause the vigor spell to be cast on a player or
// another monster.

int splVigor(Creature* player, cmd* cmnd, SpellData* spellData) {
    return(castHealingSpell(player, cmnd, spellData, "vigor", "You feel better.", S_VIGOR, mrand(1, 8)));
}

//*********************************************************************
//                      splMendWounds
//*********************************************************************
// This function will cause the mend spell to be cast on a player or
// another monster. It heals 2d7 hit points plus any bonuses for
// intelligence. If the player is a cleric then there is an additional
// point of healing for each level of the cleric.

int splMendWounds(Creature* player, cmd* cmnd, SpellData* spellData) {
    return(castHealingSpell(player, cmnd, spellData, "mend-wounds", "You feel much better.", S_MEND_WOUNDS, dice(2, 7, 0)));
}

//*********************************************************************
//                      splRejuvenate
//*********************************************************************
// This function will cause the vigor spell to be cast on a player or
// another monster.

int splRejuvenate(Creature* player, cmd* cmnd, SpellData* spellData) {
    Creature* target=0;
    int     heal=0, mpHeal=0;

    if(!(player->getClass() == CLERIC && player->getDeity() == CERIS) && spellData->how == CAST && !player->isCt()) {
        player->print("%s does not grant you the power to cast that spell.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
        return(0);
    }



    if(player->getLevel() < 13 && !player->isCt()) {
        player->print("You are not high enough in your order to cast that spell.\n");
        return(0);
    }

    if(player->mp.getCur() < 8 && spellData->how == CAST) {
        player->print("Not enough magic points.\n");
        return(0);
    }


    if(player->flagIsSet(P_NO_TICK_MP)) {
        player->print("You cannot cast that spell right now.\n");
        return(0);
    }

    // Rejuvenate self
    if(cmnd->num == 2) {

        if( player->hp.getCur() >= player->hp.getMax() &&
            player->mp.getCur() >= player->mp.getMax() &&
            !player->checkStaff("You are already at full health and magic right now.\n") )
            return(0);

        if(spellData->how == CAST) {

            if(player->isPlayer())
                player->getAsPlayer()->statistics.healingCast();
            heal = getHeal(player, 0, S_REJUVENATE);
            mpHeal = getHeal(player, 0, S_REJUVENATE);

            if(!player->isCt()) {
                player->lasttime[LT_SPELL].ltime = time(0);
                player->lasttime[LT_SPELL].interval = 24L;
            }
            player->mp.decrease(8);

        } else {
            heal = mrand(1,8);
            mpHeal = mrand(1,8);
        }

        player->doHeal(player, heal);
        player->mp.increase(mpHeal);

        if(spellData->how == CAST || spellData->how == SCROLL) {

            player->print("Rejuvenate spell cast.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts a rejuvenate spell on %sself.",
                player, player->himHer());

        } else {
            player->print("You feel revitalized.\n");
        }

    // Cast rejuvenate on another player or monster
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);
        if(!target) {
            player->print("That person is not here.\n");
            return(0);
        }

        if(!canCastHealing(player, target, true))
            return(0);

        if(target->hp.getCur() >= target->hp.getMax() && target->mp.getCur() >= target->mp.getMax()) {
            player->print("%M is at full health and magic right now.\n", target);
            return(0);
        }

        if(target->flagIsSet(P_NO_TICK_MP) || target->flagIsSet(P_NO_TICK_HP)) {
            player->print("Your rejuvinating magic is ineffective.\n");
            return(0);
        }

        if(target->inCombat(false))
            player->smashInvis();

        if(spellData->how == CAST && !player->isCt()) {
            player->lasttime[LT_SPELL].ltime = time(0);
            player->lasttime[LT_SPELL].interval = 24L;
        }


        if(spellData->how == CAST) {
            if(player->isPlayer())
                player->getAsPlayer()->statistics.healingCast();
            player->mp.decrease(8);
            heal = getHeal(player, target, S_REJUVENATE);
            mpHeal = getHeal(player, target, S_REJUVENATE);

        } else {
            heal = mrand(1,8);
            mpHeal = mrand(1,8);
        }

        player->doHeal(target, heal);
        target->mp.increase(mpHeal);

        if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {

            player->print("Rejuvenate spell cast on %N.\n", target);
            target->print("%M casts a rejuvenate spell on you.\n", player);
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a rejuvenate spell on %N.",
                player, target);

            niceExp(player, target, (heal+mpHeal)/2, spellData->how);
        }
    }

    return(1);
}

//*********************************************************************
//                      splHeal
//*********************************************************************
// This function will cause the heal spell to be cast on a player or
// another monster. It heals all hit points damage but only works 3
// times a day.

int splHeal(Creature* player, cmd* cmnd, SpellData* spellData) {
    Creature* creature=0;


    if(spellData->how == CAST && !player->isStaff()) {
        if(player->getClass() != CLERIC && player->getClass() != PALADIN && !player->isCt()) {
            player->print("Your class prohibits you from casting that spell.\n");
            return(0);
        }

        switch(player->getDeity()) {
        case ARACHNUS:
        case JAKAR:
        case GRADIUS:
        case ARES:
        case ARAMON:
        case KAMIRA:
            player->print("%s does not allow you to cast that spell.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
            return(0);
        default:
            break;
        }
    }


    // Heal self
    if(cmnd->num == 2) {

        if(!dec_daily(&player->daily[DL_FHEAL]) && spellData->how == CAST && !player->isCt() && player->isPlayer()) {
            player->print("You have been granted that spell enough today.\n");
            return(0);
        }

        //addhp(player, player->hp.getMax());
        player->doHeal(player, (player->hp.getMax() - player->hp.getCur()));
        //player->hp.restore();

        if(spellData->how == CAST || spellData->how == SCROLL) {
            if(player->isPlayer())
                player->getAsPlayer()->statistics.healingCast();
            player->print("Heal spell cast.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts a heal spell on %sself.", player,
                player->himHer());
            return(1);
        } else {
            player->print("You feel incredibly better.\n");
            return(1);
        }

    // Cast heal on another player or monster
    } else {
        if(player->noPotion( spellData))
            return(0);

        creature = player->getParent()->findCreature(player,  cmnd->str[2], cmnd->val[2], false);

        if(!creature) {
            player->print("That person is not here.\n");
            return(0);
        }

        if( !dec_daily(&player->daily[DL_FHEAL]) &&
            spellData->how == CAST &&
            !player->isCt() &&
            player->isPlayer()
        ) {
            player->print("You have been granted that spell enough times for today.\n");
            return(0);
        }

        if(spellData->how == CAST || spellData->how == SCROLL || spellData->how == WAND) {

            if(!canCastHealing(player, creature, false, true))
                return(0);

            //      addhp(creature, creature->hp.getMax());


            if(creature->isPlayer() && creature->getClass() == LICH) {

                if(!player->canAttack(creature))
                    return(0);

                if(player->getRoomParent()->isPkSafe() && !creature->flagIsSet(P_OUTLAW)) {
                    player->print("Your heal spell will harm a lich.\n");
                    player->print("No killing is allowed here.\n");
                    return(0);
                }
                if(creature->isEffected("resist-magic")) {
                    player->print("Your spell fizzles.\n");
                    if(spellData->how == CAST)
                        player->subMp(20);
                    return(0);
                }
            } else if(creature->isMonster() && creature->getClass() == LICH) {

                if(creature->isEffected("resist-magic")) {
                    player->print("Your spell fizzles.\n");
                    if(spellData->how == CAST)
                        player->subMp(20);
                    return(0);
                }
            }
            if(creature->getClass() == LICH) {
                int dmg;
                if(creature->hp.getCur() >= 4)
                    dmg = creature->hp.getCur() - mrand(2,4);
                else
                    dmg = creature->hp.getCur() - 1;

                creature->hp.decrease(dmg);

                player->print("The heal spell nearly kills %N!\n",creature);

                if(creature->isPlayer())
                    creature->print("%M's heal spell sucks away your life force!\n", player);

                broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M's heal spell nearly kills %N!",
                    player, creature);
                player->smashInvis();
                if(creature->isMonster())
                    creature->getAsMonster()->adjustThreat(player, dmg);
                return(1);
            }

            if(creature->inCombat(false))
                player->smashInvis();

            if(player->isPlayer())
                player->getAsPlayer()->statistics.healingCast();
            int healed = tMAX(creature->hp.getMax() - creature->hp.getCur() - mrand(1,4), 0);
            player->doHeal(creature, healed, 0.33);
            //creature->hp.setCur(MAX(1, creature->hp.getMax() - mrand(1,4)));

            player->print("Heal spell cast on %N.\n", creature);
            creature->print("%M casts a heal spell on you.\n", player);
            broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M casts a heal spell on %N.",
                player, creature);

            logCast(player, creature, "heal");
            return(1);
        }
    }

    return(1);
}

//*********************************************************************
//                      removeStatEffects
//*********************************************************************

void Creature::removeStatEffects() {
    Player* pThis = getAsPlayer();
    if(pThis) {
        pThis->loseRage();
        pThis->loseFrenzy();
        pThis->losePray();
    }
    removeEffect("strength");
    removeEffect("enfeeblement");
    removeEffect("haste");
    removeEffect("slow");
    removeEffect("fortitude");
    removeEffect("weakness");
    removeEffect("insight");
    removeEffect("feeblemind");
    removeEffect("prayer");
    removeEffect("damnation");
}

//*********************************************************************
//                      doRessLoss
//*********************************************************************

int doResLoss(int curr, int prev, bool full) {
    return(MAX(0, (prev - (full ? 0 : (prev - curr) / 5))));
}

//*********************************************************************
//                      doRess
//*********************************************************************

int doRes(Creature* caster, cmd* cmnd, bool res) {
    // if ress=false, it's a bloodfusion
    Player  *player = caster->getAsPlayer();
    int     prevLevel=0;
    bool    full=false;
    long    t=0;
    Player *target=0;
    BaseRoom *newRoom=0;

    if(player && !player->isDm()) {
        if(player->getNegativeLevels()) {
            player->print("You cannot do that without your full life essence.\n");
            return(0);
        }
        if(player->mp.getCur() < player->mp.getMax()) {
            player->print("You must have full MP to cast that spell.\n");
            return(0);
        }
    } else {
        // a DM can set to a 100% restore instead of 80%
        if(cmnd->num == 4 && !strcasecmp(cmnd->str[3], "-full"))
            full = true;
    }


    if(cmnd->num < 2) {
        caster->print("%s?\n", res ? "Resurrect whom" : "Fuse whom with new blood");
        return(0);
    }

    cmnd->str[2][0] = up(cmnd->str[2][0]);
    target = gServer->findPlayer(cmnd->str[2]);
    if(!target) {
        caster->print("That player is not online.\n");
        return(0);
    }

    if(target->isDm() && target->flagIsSet(P_DM_INVIS) && !caster->isDm()) {
        caster->print("That player is not online.\n");
        return(0);
    }

    if(target->isStaff()) {
        caster->print("Staff characters do not need to be %s.\n",
            res ? "resurrected" : "fused with blood");
        return(0);
    }

    if(checkRefusingMagic(caster, target))
        return(0);

    if(Move::tooFarAway(caster, target, res ? "resurrect" : "fuse with new blood"))
        return(0);

    if(res) {
        if(target->isUndead()) {
            caster->print("Undead players cannot be resurrected.\n");
            return(0);
        }
    } else {
        if(!target->isUndead()) {
            caster->print("Only undead players can be fused with new blood.\n");
            return(0);
        }
    }


    if(!caster->isDm()) {
        if(target->getLocation() != target->getLimboRoom()) {
            caster->print("%s is not in limbo! How can you %s %s?\n",
                target->getCName(), res ? "resurrect" : "bloodfuse", target->himHer());
            return(0);
        }
        if(target->getLevel() < 7) {
            caster->print("%s is not yet powerful enough to be %s.\n",
                target->getCName(), res ? "resurrected" : "fused with blood");
            return(0);
        }
        if(!target->flagIsSet(P_KILLED_BY_MOB)) {
            caster->print("%s does not need to be %s.\n", target->getCName(), res ? "resurrected" : "fused with new blood");
            return(0);
        }
    }

    if(target == caster) {
        caster->print("You cannot %s.\n", res ? "resurrect yourself" : "fuse yourself with new blood");
        return(0);
    }

    if(player && !player->isDm() && player->daily[DL_RESURRECT].cur == 0) {
        player->print("You have done that enough times for today.\n");
        return(0);
    }

    if(!caster->isDm() && target->daily[DL_RESURRECT].cur == 0) {
        caster->print("Players can only be %s once per 24 hours real time.\n", res ? "resurrected" : "fused with new blood");
        return(0);
    }


    caster->print("You cast a %s spell on %N.\n", res ? "resurrection" : "bloodfusion", target);

    prevLevel = target->getLevel();
    long expGain = full ? (target->statistics.getLastExperienceLoss()*4/5) : (target->statistics.getLastExperienceLoss());

    EffectInfo *effect = target->getEffect("death-sickness");
    if(effect) {
        if(full)
            target->removeEffect("death-sickness");
        else
            effect->setDuration(effect->getDuration() / 3);
    }

    target->cureDisease();
    target->curePoison();
    target->removeEffect("hold-person");
    target->removeEffect("petrification");
    target->removeEffect("confusion");
    target->removeEffect("drunkenness");

    target->clearFlag(P_KILLED_BY_MOB);
    target->setTickDamage(0);

    if(player)
        dec_daily(&player->daily[DL_RESURRECT]);
    dec_daily(&target->daily[DL_RESURRECT]);


    broadcast(caster->getSock(), caster->getRoomParent(), "%M casts a %s spell on %N.", caster, res ? "resurrection" : "bloodfusion", target);

    logn("log.resurrect", "%s(L:%d) %s %s(L%d:%d) %s.\n", caster->getCName(), caster->getLevel(),
        res ? "resurrected" : "fused", target->getCName(), prevLevel, target->getLevel(),
        res ? "from the dead" : "with new blood");

    if(res)
        broadcast("^R### %M just resurrected %s from the dead!", caster, target->getCName());
    else
        broadcast("^R### %M just fused %s with new blood!", caster, target->getCName());


    if(player && !player->isDm()) {
        t = time(0);
        caster->setFlag(P_NO_TICK_MP);
        // caster cannot tick MP for 20 mins online time
        caster->lasttime[LT_NOMPTICK].ltime = t;
        caster->lasttime[LT_NOMPTICK].interval = 120L;

        broadcast(caster->getSock(), caster->getRoomParent(), "%M collapses from exhaustion.", caster);
        caster->print("You collapse from exhaustion.\n");
        caster->knockUnconscious(120);

        caster->mp.setCur(0);
    }

    newRoom = target->getRecallRoom().loadRoom(target);
    if(newRoom) {
        target->deleteFromRoom();
        target->addToRoom(newRoom);
        target->doPetFollow();
    }

    target->print("You have been %s!\n", res ? "resurrected from the dead" : "fused with new blood");


    // Gain back 80% of lost experience

    target->addExperience(expGain);
    // See if we need to relevel
    target->checkLevel();

    target->hp.restore();
    target->mp.restore();
    target->pp.restore();

    target->computeAttackPower();
    target->computeAC();

    target->clearAsEnemy();
    target->save(true);
    updateRecentActivity();
    return(1);
}

//*********************************************************************
//                      splResurrect
//*********************************************************************

int splResurrect(Creature* player, cmd* cmnd, SpellData* spellData) {

    if(spellData->how != CAST)
        return(0);

    if(!player->isDm()) {
        if(player->getClass() != CLERIC && player->getDeity() != CERIS) {
            player->print("You cannot cast that spell.\n");
            return(0);
        }
        if(!player->spellIsKnown(S_RESURRECT)) {
            player->print("You do not know that spell.\n");
            return(0);
        }
        if(player->getLevel() < 19) {
            player->print("You are not high enough level to cast that spell.\n");
            return(0);
        }
        if(player->getAdjustedAlignment() != NEUTRAL) {
            player->print("You must be neutral in order to cast that spell.\n");
            return(0);
        }
    }

    return(doRes(player, cmnd, true));
}

//*********************************************************************
//                      splBloodfusion
//*********************************************************************

int splBloodfusion(Creature* player, cmd* cmnd, SpellData* spellData) {

    if(spellData->how != CAST)
        return(0);

    if(!player->isDm()) {
        if(player->getClass() != CLERIC && player->getDeity() != ARAMON) {
            player->print("You cannot cast that spell.\n");
            return(0);
        }
        if(!player->spellIsKnown(S_BLOODFUSION)) {
            player->print("You do not know that spell.\n");
            return(0);
        }
        if(player->getLevel() < 22) {
            player->print("You are not high enough level to cast that spell.\n");
            return(0);
        }
        if(player->getAdjustedAlignment() != BLOODRED) {
            player->print("You are not evil enough to cast that spell.\n");
            return(0);
        }
    }

    return(doRes(player, cmnd, false));
}

//*********************************************************************
//                      splRestore
//*********************************************************************
// This function allows a player to cast the restore spell using either
// a potion or a wand.  Restore should not be a cast-able spell because
// it can restore magic points to full.

int splRestore(Creature* player, cmd* cmnd, SpellData* spellData) {
    Creature* target=0;

    if(spellData->how == CAST && player->isPlayer() && !player->isStaff()) {
        player->print("You may not cast that spell.\n");
        return(0);
    }

    // Cast restore on self
    if(cmnd->num == 2) {
        target = player;

        if(spellData->how == CAST || spellData->how == WAND) {
            player->print("Restore spell cast.\n");
            broadcast(player->getSock(), player->getParent(), "%M casts restore on %sself.", player, player->himHer());
        } else if(spellData->how == POTION)
            player->print("You feel restored.\n");

    // Cast restore on another player
    } else {
        if(player->noPotion( spellData))
            return(0);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            player->print("That person is not here.\n");
            return(0);
        }


        player->print("Restore spell cast on %N.\n", target);
        target->print("%M casts a restore spell on you.\n", player);
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a restore spell on %N.",
            player, target);

        logCast(player, target, "restore");
    }
    player->doHeal(target, dice(2, 10, 0));

    if(mrand(1, 100) < 34)
        target->mp.restore();

    if(player->isStaff()) {
        target->mp.restore();
        target->hp.restore();
    }

    return(1);
}

//*********************************************************************
//                      splRoomVigor
//*********************************************************************

int splRoomVigor(Creature* player, cmd* cmnd, SpellData* spellData) {
    int      heal=0;

    if(spellData->how == POTION) {
        player->print("The spell fizzles.\n");
        return(0);
    }

    if(player->getClass() != CLERIC && !player->isCt()) {
        player->print("Only clerics may cast that spell.\n");
        return(0);
    }

    player->print("You cast vigor on everyone in the room.\n");
    broadcast(player->getSock(), player->getParent(), "%M casts vigor on everyone in the room.\n", player);

    heal = mrand(1, 6) + bonus((int) player->piety.getCur());

    if(player->getRoomParent()->magicBonus()) {
        heal += mrand(1, 3);
        player->print("\nThe room's magical properties increase the power of your spell\n");
    }

    for(Player* ply : player->getRoomParent()->players) {
        if(canCastHealing(player, ply, false, false, false)) {
            if(ply != player)
                ply->print("%M casts vigor on you.\n", player);
            player->doHeal(ply, heal);
            if(ply->inCombat(false))
                player->smashInvis();
        }
    }

    return(1);
}
