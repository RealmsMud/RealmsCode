/*
 * combat.cpp
 *   Routines to handle monster combat
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

#include <cstring>                     // for strcpy
#include <ctime>                       // for time
#include <list>                        // for operator==, list, _List_iterator
#include <set>                         // for operator==, _Rb_tree_const_ite...
#include <string>                      // for allocator, string, operator==

#include "catRef.hpp"                  // for CatRef
#include "damage.hpp"                  // for Damage
#include "effects.hpp"                 // for EffectInfo
#include "fighters.hpp"                // for FOCUS_DAMAGE_IN, FOCUS_DAMAGE_OUT
#include "flags.hpp"                   // for M_WILL_YELL_FOR_HELP, M_YELLED...
#include "global.hpp"                  // for WIELD, HELD, SPL, DEA, POI
#include "lasttime.hpp"                // for lasttime
#include "mud.hpp"                     // for LT_SAVES, LT_AGGRO_ACTION, LT
#include "mudObjects/container.hpp"    // for MonsterSet, ObjectSet
#include "mudObjects/creatures.hpp"    // for Creature, ATTACK_BLOCK, CHECK_...
#include "mudObjects/exits.hpp"        // for Exit
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/objects.hpp"      // for Object, ObjectType, ObjectType...
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom, ExitList
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "proto.hpp"                   // for broadcast, bonus, broadcastGroup
#include "random.hpp"                  // for Random
#include "server.hpp"                  // for Server, gServer
#include "socket.hpp"                  // for Socket
#include "specials.hpp"                // for SpecialAttack
#include "statistics.hpp"              // for Statistics
#include "stats.hpp"                   // for Stat, MOD_CUR_MAX
#include "structs.hpp"                 // for saves
#include "unique.hpp"                  // for isLimited, remove
#include "wanderInfo.hpp"              // for WanderInfo
#include "xml.hpp"                     // for loadMonster


//*********************************************************************
//                      doLagProtect
//*********************************************************************
// comes in as a creature, our job to turn into a player

bool Creature::doLagProtect() {
    std::shared_ptr<Player> pThis = getAsPlayer();

    if(!pThis)
        return(false);

    if(pThis                ->flagIsSet(P_LAG_PROTECTION_SET) && pThis->flagIsSet(P_LAG_PROTECTION_ACTIVE))
        if(pThis->lagProtection())
            return(true);

    return(false);
}


//*********************************************************************
//                      updateCombat
//*********************************************************************

bool Monster::updateCombat() {
    std::shared_ptr<BaseRoom> room=nullptr;
    std::shared_ptr<Creature> target=nullptr;
    std::shared_ptr<Player> pTarget=nullptr;
    std::shared_ptr<Monster>  mTarget=nullptr;
    char    atk[30];
    int     n, rtn=0, yellchance=0, num=0, breathe=0;
    int     x=0;
    bool    monstervmonster, casted=false;
    bool    resistPet=false, immunePet=false, vulnPet=false;
    bool    willCast=false, antiMagic=false, isCharmed=false;

    room = getRoomParent();

    strcpy(atk, "");

    target = getTarget();

    if(!target) return(false);

    // is this a player?
    pTarget = target->getAsPlayer();
    mTarget = target->getAsMonster();

    // If we're fighting a pet, see if we'll ignore the pet and attack a player instead
    if(target->isPet() && target->getMaster()) {
        if(target->inSameRoom(target->getMaster())) {
            if(Random::get(1,100) < 20 || target->intelligence.getCur() >= 150 || target->flagIsSet(M_KILL_MASTER_NOT_PET)) {
                target = target->getMaster();
                addEnemy(target);
            }
        }
    }

    // No fighting in a safe room!
    if( pTarget && isPet() && !getMaster()->isCt() && getRoomParent()->isPkSafe())
        return(false);

    // Stop fighting if it's a no exp loss monster
    if( pTarget && flagIsSet(M_NO_EXP_LOSS) && !nearEnmPly() && !isPet() && target->inCombat(Containable::downcasted_shared_from_this<Monster>()))
        return(false);

    monstervmonster = (!pTarget && !target->isPet() && isMonster() && !isPet());

    room->wake("Loud noises disturb your sleep.", true);


    if(target.get() == this)
        return(false);
    if(target->hasCharm(getName()) && flagIsSet(M_CHARMED))
        return(false);


    target->checkTarget(Containable::downcasted_shared_from_this<Monster>());

    n = 20;
    if(flagIsSet(M_CAST_PRECENT))
        n = cast;

    // Make it so more intelligent monsters won't sit and cast spells all day
    // on a person who is more than 50% r-m!
    int resistAmount = 100 - doResistMagic(100);
    if(resistAmount < 100) {
        if(intelligence.getCur() > 250)  {
            if(resistAmount >= 90)      n /= 5;
            else if(resistAmount >= 75) n /= 3;
            else                        n = n * 2 / 3;
        } else if(intelligence.getCur() > 210) {
            if(resistAmount >= 90)      n /= 3;
            else if(resistAmount >= 75) n /= 2;
            else                        n = n * 5 / 6;
        } else if(intelligence.getCur() > 170) {
            if(resistAmount >= 80)      n /= 2;
            else                        n = n * 5 / 6;
        }
    }
    // Also if the target is reflecting magic
    if(target->isEffected("reflect-magic")) {
        if(intelligence.getCur() >= 280)
            n /= 5;
        else if(intelligence.getCur() >= 250)
            n /= 3;
        else if(intelligence.getCur() >= 210)
            n /= 2;
        else if(intelligence.getCur() >= 170)
            n = n * 5 / 6;
    }


    if(Random::get(1,100) < n) {
        willCast = true;
        if(room->checkAntiMagic(Containable::downcasted_shared_from_this<Monster>()))
            antiMagic = true;
    }
    if(isUndead() && target->isEffected("undead-ward"))
        willCast = false;

    // handle spell-invisible monsters
    EffectInfo* effect = getEffect("invisibility");
    if(effect && effect->getDuration() != -1) {
        broadcast(getSock(), room, "%M fades into view.", this);
        removeEffect("invisibility");
    }

    lasttime[LT_AGGRO_ACTION].ltime = time(nullptr);

    if( willCast && flagIsSet(M_CAN_CAST) && canSpeak() && !getRoomParent()->flagIsSet(R_NO_MAGIC) && !antiMagic && !isCharmed) {
        if(target->doLagProtect())
            return(true);

        rtn = castSpell( target );

        if(rtn == 2)
            return(true);
        else if(rtn == 1) {
            casted = true;
            if(pTarget && pTarget->flagIsSet(P_WIMPY) && !pTarget->flagIsSet(P_LAG_PROTECTION_OPERATING)) {
                if(pTarget->hp.getCur() <= pTarget->getWimpy()) {
                    pTarget->flee(false, true);
                    return(true);
                }
            }
        }
    }


    // Don't jump to your aid if it is itself...lol
    if(pTarget) {
        for(const auto& pet : pTarget->pets) {
            if( pet->isPet() && getMaster() != pTarget && pet->getMaster() == pTarget && !pet->isEnemy(this) && pet.get() != this ) {
                auto enemy = findFirstEnemyCrt(pet);
                pet->addEnemy(enemy);
                pTarget->print("%M jumps to your aid against %M!!\n", pet.get(), enemy.get());
                break;
            }
        }
    }

    if(hp.getCur() <= hp.getMax()/3 && flagIsSet(M_WILL_BERSERK))
        berserk();


    if(casted)
        return(true);

    // Try running special attacks if we have any
    if(!specials.empty() && runSpecialAttacks(target))
        return(true);



    if(!isCharmed) {
        if(canSpeak()) {

            if(flagIsSet(M_YELLED_FOR_HELP))
                yellchance = 15+(2*bonus(intelligence.getCur()));
            else
                yellchance = 25+(2*bonus(intelligence.getCur()));

            if(flagIsSet(M_WILL_YELL_FOR_HELP) && ((Random::get(1,100) < yellchance) || (hp.getCur() <= hp.getCur()/5))) {
                if(!flagIsSet(M_WILL_BE_HELPED))
                    broadcast(getSock(), room, "%M yells for help!", this);
                setFlag(M_YELLED_FOR_HELP);
                setFlag(M_WILL_BE_ASSISTED);
                clearFlag(M_WILL_YELL_FOR_HELP);
                return(false);
            }
        }
    }

    if( flagIsSet(M_YELLED_FOR_HELP) &&
        (Random::get(1,100) < (std::max(15, inUniqueRoom() ? getUniqueRoomParent()->wander.getTraffic() : 15))) &&
        !flagIsSet(M_WILL_YELL_FOR_HELP)
    ) {
        setFlag(M_WILL_YELL_FOR_HELP);
        if(summonMobs(target))
            return(false);
    }

    // Check resisting of elemental pets
    if(isPet()) {
        target->checkResistPet(Containable::downcasted_shared_from_this<Monster>(), resistPet, immunePet, vulnPet);
        if( !pTarget && !casted && (target->flagIsSet(M_ONLY_HARMED_BY_MAGIC) || immunePet)) {
            getMaster()->print("%M's attack has no effect on %N.\n", this, target.get());
            return(false);
        }
    }
    // We'll be nice for now...no critical hits from mobs :)
    int resultFlags = NO_CRITICAL | NO_FUMBLE;
    AttackResult result = getAttackResult(target, nullptr, resultFlags);

    if(isPet()) {
        // We are a pet, and we're attacking.  Smash invis of our owner.
        getMaster()->smashInvis();
    }


    if(result == ATTACK_HIT || result == ATTACK_CRITICAL || result == ATTACK_BLOCK) {
        Damage attackDamage;
        int drain = 0;
        bool wasKilled = false, freeTarget = false, meKilled;

        computeDamage(target, ready[WIELD - 1], ATTACK_NORMAL, result, attackDamage, true, drain);
        attackDamage.includeBonus();

        target->printColor("^r");

        if(!breathe) {
            if(attack[0][0] || attack[1][0] || attack[2][0]) {
                do {
                    num = Random::get(1,3);
                    if(attack[num-1][0])
                        strcpy(atk, attack[num-1]);
                } while(!attack[num-1][0]);
            } else {
                strcpy(atk, "hit");
            }
        }

        if(!atk[0]) // just in case
            strcpy(atk, "hit");

        if(result == ATTACK_BLOCK) {
            printColor("^C%M partially blocked your attack!\n", target.get());
            target->printColor("^CYou manage to partially block %N's attack!\n", this);
        }

        target->printColor("^r%M %s you%s for ^R%d^r damage.\n", this, atk,
            target->isBrittle() ? "r brittle body" : "", std::max<int>(1, attackDamage.get()));

        if(!isPet())
            target->checkImprove("defense", false);

        if(pTarget) {
            pTarget->damageArmor(attackDamage.get());
            pTarget->statistics.wasHit();
        }

        if(ready[WIELD - 1]) {
            if(ready[WIELD - 1]->getMagicpower() && ready[WIELD - 1]->flagIsSet(O_WEAPON_CASTS) && ready[WIELD - 1]->getChargesCur() > 0)
                attackDamage.add(castWeapon(target, ready[WIELD - 1], wasKilled));
        }

        broadcastGroup(false, target, "^M%M^x %s ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n", this, atk, target.get(), attackDamage.get(), target->heShe(), target->getStatusStr(attackDamage.get()));

        if(target->pFlagIsSet(P_LAG_PROTECTION_SET))
            target->pSetFlag(P_LAG_PROTECTION_ACTIVE); // lagprotect auto-activated on being hit.

        if(target->isPet())
            target->getMaster()->printColor("%M hit ^M%N^x for %s%d^x damage.\n", this, target.get(), target->getMaster()->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());

        if(isPet()) {
            getMaster()->printColor("%M hit %N for %s%d^x damage.\n", this, target.get(), getMaster()->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());

            castDelay(PET_CAST_DELAY);

            if(mTarget) {
                mTarget->addEnemy(Containable::downcasted_shared_from_this<Monster>());
            }

        } else if(monstervmonster) {
            // Output only when monster v. monster
            broadcast(getSock(), target->getSock(), room, "%M hits %N.", this, target.get());
            mTarget->addEnemy(Containable::downcasted_shared_from_this<Monster>());
        }



        if(flagIsSet(M_WILL_POISON))
            tryToPoison(target);
        if(flagIsSet(M_CAN_STONE))
            tryToStone(target);
        if(flagIsSet(M_DISEASES))
            tryToDisease(target);

        if( (flagIsSet(M_WOUNDING) && x <= 15) &&
            (target->getClass() !=  CreatureClass::LICH) &&
            !target->isEffected("stoneskin")
        ) {
            if(!target->chkSave(DEA, Containable::downcasted_shared_from_this<Monster>(),0)) {
                target->printColor("^RThe wound is festering and unclean.\n");
                broadcastGroup(false, target, "%M wounds are festering and unclean.\n", target.get());
                broadcast(getSock(), target->getSock(), room, "%M's wounds are festering and unclean.\n", target.get());

                target->addEffect("wounded", -1, 1, Containable::downcasted_shared_from_this<Monster>(), false, target);
            }
        }

        meKilled = doReflectionDamage(attackDamage, target);

        if( pTarget && (
                flagIsSet(M_STEAL_WHEN_ATTACKING) ||
                flagIsSet(M_STEAL_ALWAYS)
            ) &&
            Random::get(1,100) <= 10 && (
                !pTarget->hasCharm(getName()) &&
                flagIsSet(M_CHARMED)
            )
        ) {
            if(steal(pTarget)) {
                if(wasKilled)
                    pTarget->checkDie(Containable::downcasted_shared_from_this<Monster>());
                if(meKilled)
                    checkDie(pTarget);
                return(false);
            }
        }

        if(flagIsSet(M_WILL_BLIND))
            tryToBlind(target);

        if(pTarget && flagIsSet(M_DISOLVES_ITEMS) && Random::get(1,100) <= 15)
            pTarget->dissolveItem(Containable::downcasted_shared_from_this<Monster>());

        if( doDamage(target, attackDamage.get(), CHECK_DIE_ROB, PHYSICAL_DMG, freeTarget) ||
            wasKilled ||
            meKilled
        ) {
            Creature::simultaneousDeath(Containable::downcasted_shared_from_this<Monster>(), target, false, freeTarget);
            return(true);
        }

        if( pTarget &&
            pTarget->flagIsSet(P_WIMPY) &&
            !pTarget->flagIsSet(P_LAG_PROTECTION_OPERATING) &&
            !pTarget->flagIsSet(P_LAG_PROTECTION_ACTIVE))
        {
            if(pTarget->hp.getCur() <= pTarget->getWimpy()) {
                pTarget->flee(false, true);
                return(true);
            }
        } else if(pTarget && pTarget->isEffected("fear")) {
            int ff;
            ff = 40 + (1- (pTarget->hp.getCur()/pTarget->hp.getMax()))*40 +
                 ::bonus(pTarget->constitution.getCur()) * 3 +
                 ((pTarget->getClass() == CreatureClass::PALADIN ||
                         pTarget->getClass() == CreatureClass::DEATHKNIGHT) ? -10 : 0);
            if(ff < Random::get(1,100)) {
                pTarget->flee(true);
                return(true);
            }
        }

        if(target->doLagProtect())
            return(true);

    } else if(result == ATTACK_MISS) {
        target->printColor("^c%M missed you.\n", this);
        if(!isPet())
            target->checkImprove("defense", true);

        if(target->doLagProtect())
            return(true);

        // Output only when monster v. monster
        if(monstervmonster) {
            broadcast(target->getSock(), target->getSock(), room, "%M misses %N.", this, target.get());
        } else if(isPet()) {
            getMaster()->printColor("^c%M misses %N.\n", this, target.get());
        }

        if(mTarget)
            mTarget->addEnemy(Containable::downcasted_shared_from_this<Monster>());

        if(pTarget) {
            pTarget->statistics.wasMissed();
            if( pTarget->flagIsSet(P_WIMPY) &&
                !pTarget->flagIsSet(P_LAG_PROTECTION_OPERATING)
            ) {
                if(pTarget->hp.getCur() <= pTarget->getWimpy()) {
                    pTarget->flee(false, true);
                    return(true);
                }
            }
        }
    } else if(result == ATTACK_DODGE) {
        if(!isPet())
            target->checkImprove("defense", true);
        target->dodge(Containable::downcasted_shared_from_this<Monster>());
        return(true);
    }
    else if(result == ATTACK_PARRY) {
        if(!isPet())
            target->checkImprove("defense", true);
        target->parry(Containable::downcasted_shared_from_this<Monster>());
        return(true);
    }

    return(false);
}

//*********************************************************************
//                      zapMp
//*********************************************************************

bool Monster::zapMp(std::shared_ptr<Creature>victim, SpecialAttack* attack) {
    //Player    *pVictim = victim->getPlayer();
    int     n=0;

    if(victim->mp.getCur() <= 0)
        return(false);

    n = std::min<int>(victim->mp.getCur(), (Random::get<unsigned short>(1+level/2, level) + bonus(intelligence.getCur())));

    victim->printColor("^M%M zaps your magical talents!\n", this);
    victim->printColor("%M stole %d magic points!\n", this, n);
    broadcast(victim->getSock(), victim->getSock(), victim->getRoomParent(), "^M%M zapped %N!", this, victim.get());

    if(victim->chkSave(MEN, Containable::downcasted_shared_from_this<Monster>(), 0))
        n /= 2;

    victim->mp.decrease(n);

    return(true);
}

//*********************************************************************
//                      regenerate
//*********************************************************************

//*********************************************************************
//                      steal
//*********************************************************************
// Returns 0 if unable to steal for some reason but returns 1 if
// succeeded or steal failed

bool Monster::steal(std::shared_ptr<Player>victim) {
    int chance=0, inventory=0, i=0;
    bool isContainer=false;
    std::shared_ptr<Object>  object=nullptr;

    if(!victim || !canSee(victim) || victim->isStaff())
        return(false);
    if(victim->isEffected("petrification") || victim->isEffected("mist"))
        return(false);
    if(victim->objects.empty())
        return(false);

    lasttime[LT_STEAL].ltime = time(nullptr);

    i = victim->countInv();
    if(i < 1)
        return(false);

    if(i == 1)
        inventory = 1;
    else
        inventory = Random::get(1, i - 1);

    i = 1;
    for(const auto& obj : victim->objects) {
        if(i++ == inventory) {
            object = obj;
            break;
        }
    }

    if(object->getType() == ObjectType::CONTAINER) {
       isContainer=true;
    }
    // Max chance for a mob to steal a stealable bag no matter what the level difference, is 5%
    // Impossible to steal any bag if mob is same level or lower than player
    // Any mob within 10 levels of a player always has at least a 1% chance to steal a stealable non-bag
    // Any mob has a minimum of 95% to steal a non-bag, despite how much higher level they are than a player
    //   They always have a 5% chance to fail.
    if(isContainer)
        chance = std::min(5,(level * (level - victim->getLevel())));
    else
        chance = std::min(95,std::max(1,4 * (level - victim->getLevel())));

    // If a player is 10+ levels higher than a mob, they never have to worry about a mob succeeding in stealing. 
    // The 1% min chance above for stealing non-bags is nullified. The mob will still try though.
    if (victim->getLevel() - level >= 10)
        chance = 0;

    // Check objects in bag; if unstealable object found, bag cannot be stolen
    if (isContainer) {
        for(const auto& cntObj : object->objects) {
            if (cntObj->flagIsSet(O_NO_STEAL) ||
                cntObj->getQuestnum()) {
                chance = 0;
                break;
            }
        }
        // The more objects inside a weighted or bulky container, the harder it is to steal it
        // Slight penalty for the mob the fuller the container is.
        // Max chance to steal a bag is never more than 5%. This brings that down even further.
        if (!(object->flagIsSet(O_WEIGHTLESS_CONTAINER) || object->flagIsSet(O_BULKLESS_CONTAINER))) {
             if (object->countObj(false) >= (object->getShotsMax()*90/100))  // 90%+ full
                chance -= 3;
            else if (object->countObj(false) >= (object->getShotsMax()*50/100)) // 50 to 89% full
                chance -= 2;
            else if (object->countObj(false) >= 1) // 1 to 49% full
                chance --; 
        }

    }

    if( object->getQuestnum() ||
        (flagIsSet(M_CANT_BE_STOLEN_FROM) ||
            object->flagIsSet(O_NO_STEAL)) ||
            object->flagIsSet(O_STARTING) ||
             object->flagIsSet(O_CUSTOM_OBJ))
        chance = 0;

    chance = std::max(0,chance);

    if(Random::get(1, 100) <= chance) {
        victim->delObj(object, false, true);
        addObj(object);
        victim->printColor("^Y%M stole %P^Y from you!\n", this, object.get());
        logn("log.msteal", "%s(L%d) stole %s from %s(L%d) in room %s.\n",
                 getCName(), level, object->getCName(), victim->getCName(), victim->getLevel(), getRoomParent()->fullName().c_str());
    } else {
            broadcast(victim->getSock(), getRoomParent(), "%M tried to steal from %N.", this, victim.get());
            victim->printColor("^Y%M tried to steal %P^Y from you.\n", this, object.get());
    }
    return(true);
}

//*********************************************************************
//                      berserk
//*********************************************************************

void Monster::berserk() {

    // TODO: Change Berserk into an effect
    if(isEffected("berserk"))
        return;
    addEffect("berserk", 60, 50);
    clearFlag(M_WILL_BERSERK);
    strength.addModifier("Berserk", 50, MOD_CUR_MAX);

    broadcast((std::shared_ptr<Socket> )nullptr, getRoomParent(), "^R%M goes berserk!", this);
}

//*********************************************************************
//                      summonMobs
//*********************************************************************

bool Monster::summonMobs(const std::shared_ptr<Creature>&victim) {
    int     mob=0, arrive=0, i=0, found=0;
    std::shared_ptr<Monster> monster=nullptr;

    // Calls for help will be suspended if max mob allowance
    // in room is already reached.

    for(i=0; i<NUM_RESCUE; i++) {
        if(validMobId(rescue[i]))
            found++;
    }

    if(!found)
        return(false);

    if(getRoomParent()->countCrt() >= getRoomParent()->getMaxMobs())
        return(false);

    found = Random::get(1, found)-1;

    for(mob=0; found; mob++) {
        if(validMobId(rescue[mob]))
            found--;
    }


    arrive = Random::get(1,2);

    if(victim->pFlagIsSet(P_LAG_PROTECTION_SET))
        victim->pSetFlag(P_LAG_PROTECTION_ACTIVE);


    for(i=0; i < arrive; i++) {
        monster = nullptr;
        if(!loadMonster(rescue[mob], monster))
            return(false);

        gServer->addActive(monster);
        monster->addToRoom(victim->getRoomParent());
        monster->addEnemy(victim);

        if(victim->getMaster() && victim->isMonster())
            monster->addEnemy(victim->getMaster());

        broadcast(getSock(), victim->getRoomParent(), "%M runs to the aid of %N!", monster.get(), this);

        monster->setFlag(M_WILL_ASSIST);
        monster->setFlag(M_WILL_BE_ASSISTED);
        monster->clearFlag(M_PERMENANT_MONSTER);
    }

    return(true);
}

//*********************************************************************
//                      check_for_yell
//*********************************************************************

bool Monster::checkForYell(const std::shared_ptr<Creature>& target) {
    int yellchance;

    if(!(flagIsSet(M_WILL_YELL_FOR_HELP) || flagIsSet(M_YELLED_FOR_HELP)))
        return(false);

    if(!canSpeak())
        return(false);


    if(flagIsSet(M_YELLED_FOR_HELP))
        yellchance = 35+(2*bonus(intelligence.getCur()));
    else
        yellchance = 50+(2*bonus(intelligence.getCur()));


    if(flagIsSet(M_WILL_YELL_FOR_HELP) && ((Random::get(1,100) < yellchance) || (hp.getCur() <= hp.getCur()/5))) {
        if(!flagIsSet(M_WILL_BE_HELPED))
            broadcast(getSock(), target->getRoomParent(), "%M yells for help!", this);
        setFlag(M_YELLED_FOR_HELP);
        setFlag(M_WILL_BE_ASSISTED);
        clearFlag(M_WILL_YELL_FOR_HELP);
        return(true);  // Monster yells
    }


    return(false); // Monster does not yell
}

//*********************************************************************
//                      lagProtection
//*********************************************************************

bool Player::lagProtection() {
    std::shared_ptr<BaseRoom> room = getRoomParent();
    int     t=0, idle=0;

    // Can't do anything while unconsious!
    if(flagIsSet(P_UNCONSCIOUS))
        return(false);

    t = time(nullptr);
    idle = t - getSock()->ltime;


    // This number can be changed.
    if(idle < 10L) {
        //print("testing for idle..\n");
        return(false);
    }

    // Only lag protect below wimpy if auto attack is on
    if(autoAttackEnabled() && hp.getCur() > getWimpy())
        return(false);


    printColor("^cPossible lagout detected.\n");
    statistics.lagout();
    if(!flagIsSet(P_NO_LAG_HAZY) && hp.getCur() < hp.getMax()/2) {

        setFlag(P_LAG_PROTECTION_OPERATING);

        if(useRecallPotion(1, 1))
            return(true);
    }

    broadcast(getSock(), room, "%M enters autoflee lag protection routine.", this);

    print("Auto flee routine initiated.\n");
    if(flagIsSet(P_SITTING))
        stand();
    printColor("Searching for suitable exit........\n");


    if(ready[WIELD-1]) {
        if(!ready[WIELD-1]->flagIsSet(O_CURSED)) {
            ready[WIELD-1]->clearFlag(O_WORN);
            printColor("Removing %s.\n", ready[WIELD-1]->getCName());
            unequip(WIELD);
        }
    }

    if(ready[HELD-1]) {
        if(!ready[HELD-1]->flagIsSet(O_CURSED)) {
            ready[HELD-1]->clearFlag(O_WORN);
            printColor("Removing %s.\n", ready[HELD-1]->getCName());
            unequip(HELD);
        }
    }

    for(const auto& ext : room->exits) {
        // Opens all unlocked exits.
        if(!ext->flagIsSet(X_LOCKED))
            ext->clearFlag(X_CLOSED);
    }

    setFlag(P_LAG_PROTECTION_OPERATING);
    if(flee(false, true)) {
        if(isStaff()) {
            broadcast(::isStaff, "^C### %s(L%d) fled due to lag protection. HP: %d/%d. Room: %s.",
            getCName(), level, hp.getCur(), hp.getMax(), getRoomParent()->fullName().c_str());
        } else {
            broadcast(::isWatcher, "^C### %s(L%d) fled due to lag protection. HP: %d/%d. Room: %s.",
            getCName(), level, hp.getCur(), hp.getMax(), getRoomParent()->fullName().c_str());
        }
        logn("log.lprotect","### %s(L%d) fled due to lag protection. HP: %d/%d. Room: %s.\n",
            getCName(), level, hp.getCur(), hp.getMax(), getRoomParent()->fullName().c_str());

        return(true);
    }
    return(false);
}


//*********************************************************************
//                      chkSave
//*********************************************************************
// This function is used for saving throws. It is sent savetype, which
// is the save category, creature, which is the creature saving,
// and if an this is involved, target is sent. If there is
// no this, then target will be 0. The bonus variable is
// a special bonus which can be sent, based on conditions set in
// the calling function. the chksave function returns 0
// if the creature does not save, and 1 if it does. How the save
// affects creature is determined in the code of the calling function. -- TC

bool Creature::chkSave(short savetype, const std::shared_ptr<Creature>& target, short bns) {
    std::shared_ptr<Player> pCreature=nullptr;
    int     chance, gain, i, ringbonus=0, opposing=0, upchance = 0, upchanceRoll=0, natural=1;
    int     roll=0, nogain=0;
    long    j=0, t=0, duration=0;


    pCreature = getAsPlayer();

    if(target && target.get() != this)
        opposing = 1;

    chance = 100*saves[savetype].chance;
    gain = saves[savetype].gained;

    // Determine a save bonus from any magical rings. Only one ring of highest + is used.
    for(i=FINGER1; i < FINGER8+1; i++) {
        if(!ready[i])       
            continue;
        if(ready[i]->getAdjustment() > 0 && ready[i]->getAdjustment() > ringbonus) {
            if(isCt() || (pCreature && pCreature->flagIsSet(P_SAVE_DEBUG))) printColor("^yHighest worn ring: %s(+%d)\n", ready[i]->getCName() ? ready[i]->getCName():"", ready[i]->getAdjustment());
            ringbonus = ready[i]->getAdjustment();
            continue;
        }
    }

    chance += ringbonus*500;
  

    if( ready[HELD-1] && ready[HELD-1]->flagIsSet(O_LUCKY) &&
        ready[HELD-1]->getType() != ObjectType::WEAPON && ready[HELD-1]->getAdjustment() > 0) {
        chance += ready[HELD-1]->getAdjustment()*500;
        if(isCt() || (pCreature && pCreature->flagIsSet(P_SAVE_DEBUG))) 
            print("Checking for lucky held item: %s(+%d)\n", ready[HELD-1]->getCName() ? ready[HELD-1]->getCName():"", ready[i]->getAdjustment());
        }

    switch(savetype) {
    case POI:
        chance += constitution.getCur();
        if(hp.getCur() <= hp.getMax()/3)
            chance /= 2;    // More vulnerable to poison if weakened severely.
        if(pCreature && pCreature->isEffected("berserk"))
            chance += 1500;   // Poison doesn't harm berserk players as much.
        break;

    case DEA: // bonuses set for death traps and death saves are sent with the
              // bonus variable from the calling function.

        break;
    case BRE: // Check armor worn for magical adjustments and add to chance based on armor piece
        if(ready[BODY-1]) 
            chance += 50 * ready[BODY-1]->getAdjustment();  
        if(ready[ARMS-1])
            chance += 20 * ready[ARMS-1]->getAdjustment();          
        if(ready[LEGS-1])
            chance += 25 * ready[LEGS-1]->getAdjustment();
        if(ready[NECK-1])
            chance += 5 * ready[NECK-1]->getAdjustment();            
        if(ready[BELT-1])
            chance += 5 * ready[BELT-1]->getAdjustment();           
        if(ready[HANDS-1])
            chance += 5 * ready[HANDS-1]->getAdjustment();          
        if(ready[HEAD-1]) 
            chance += 15 * ready[HEAD-1]->getAdjustment();           
        if(ready[FEET-1]) 
            chance += 10 * ready[FEET-1]->getAdjustment();            
        if(ready[FACE-1])
            chance += 5 * ready[FACE-1]->getAdjustment();
        if(ready[SHIELD-1]) {
            if(ready[SHIELD-1]->flagIsSet(O_SMALL_SHIELD))
                chance += 10 * ready[SHIELD-1]->getAdjustment();
            else
                chance += 40 * ready[SHIELD-1]->getAdjustment();
        }
        chance += dexterity.getCur() * 2;
        break;
    case MEN:
        if(opposing)
            chance -= ( 5*((target->intelligence.getCur()+target->piety.getCur())/2) - ((intelligence.getCur()+piety.getCur())/2));
        else
            chance += (intelligence.getCur()+piety.getCur())/2;
        break;
    case SPL:
        if(opposing)
            chance -= 2*(target->intelligence.getCur() - intelligence.getCur());
        else
            chance += 2*intelligence.getCur();

        if(pCreature && pCreature->isEffected("resist-magic")) {
            natural = 0;
            chance += 2500;
        }
        break;
    case LCK:
        chance += 100*((saves[POI].chance + saves[DEA].chance +
                saves[BRE].chance + saves[MEN].chance +
                saves[SPL].chance)/5);
        break;
    default:
        break;
    }

    if(bns)
        chance += 100*bns;

    if(pCreature && pCreature->getClass() == CreatureClass::CLERIC && pCreature->getDeity() == KAMIRA)
        chance += 1000;

    chance = std::max(1,std::min(9900, chance)); // always a 1% chance to fail

     // Gaining saves is on a timer so people can't easily spam in/out/in/out room to instantly raise various save types
     // They can still do it, but it takes a little more work
     j = LT(this, LT_SAVES);
     t = time(nullptr);
     if(j > t) {
        nogain = 1;

     if(isCt() || (pCreature && pCreature->flagIsSet(P_SAVE_DEBUG))) {
        duration = j-t;
        if(duration < 0)
            duration *=-1;
        if (duration == 1)
            printColor("^DSave gain chance delay: 1 more second.\n");
        else
            printColor("^DSave gain chance delay: %d more seconds.\n", duration);
        }
    }

    roll = Random::get(1,10000);
    if(isCt() || (pCreature && pCreature->flagIsSet(P_SAVE_DEBUG))) {
        printColor("^ySave[%d] chance unmodified: %d\n", savetype, 
            savetype != LCK ? saves[savetype].chance*100 : 100*((saves[POI].chance+saves[DEA].chance+saves[BRE].chance+saves[MEN].chance+saves[SPL].chance)/5));
        printColor("^ySave[%d] chance modified: %d\n", savetype, chance);
        printColor("^ySave[%d] roll (1d10000): %d%s %s\n", savetype, roll, ((roll <= chance) && natural)  ? "^g[SUCCESS]":"^r[FAIL]", natural==0 ? "^D(resist-magic)":"");
        if(saves[savetype].gained == 5) 
            printColor("^DNo gains for save[%d] remaining this level - save cannot go up.\n", savetype);
    }

    if((roll >= 8500 || roll <= 2000) && gain < 5) {       // Simulates d20 critical failure roll of 17-20 or critical success roll of 1-4 - essentially 40% chance
        if( savetype != LCK && (isCt() || (pCreature && pCreature->flagIsSet(P_SAVE_DEBUG)))) {
            if(roll >=8500) printColor("^RCritical save failure!! ^y(%d >= 8500)\n", roll);
            if(roll <=2000) printColor("^GCritical save success!! ^y(%d <= 2000)\n", roll);
        }
          
        upchance = std::max(20,(99 - saves[savetype].chance)); // Always at least a 20% chance save will go up
        if(bns == -1 ||                                   // Calling function indicates specific circumstance will not raise the save
           savetype == LCK ||                             // Luck save is an average of all other saves and does not raise
           level < 10 ||                                  // Won't start raising until level 10; this has to do with the level 40 max and overall spread
           (target->isPlayer() && savetype == SPL) ||     // Players casting offensive spells on one another won't raise the target's spell save - too ez to abuse
           saves[savetype].chance >= 99 ||                // If unmodified save chance is already 99+, it will not raise anymore
           nogain)                                        // nogain == 1, so save gain timer is not ready yet
            upchance = 0;

        upchanceRoll=Random::get(1,100);

        if (isCt() || (pCreature && pCreature->flagIsSet(P_SAVE_DEBUG))) 
            printColor("^ySave Upchance: %d%s\nSave UpchanceRoll(1d100): %d%s\n", upchance, (99-saves[savetype].chance) < 20 ? "(MAX)":"", upchanceRoll, upchanceRoll<=upchance ? "^g[SUCCESS]^y":"^r[FAIL]^y");
        else {

            if ((upchanceRoll<=upchance) && savetype != LCK) {
                
                if (roll >= 8500)
                    printColor("^RCritical save failure!!\n");
                else
                    printColor("^GCritical save success!!\n");

            }
        }

        if(upchanceRoll <= upchance) {

            //Timer resets when we successfully raise a save
            if(pCreature) {
                pCreature->lasttime[LT_SAVES].ltime = t;
                if(isCt())
                    pCreature->lasttime[LT_SAVES].interval = 0;
                else
                    pCreature->lasttime[LT_SAVES].interval = 60L; 
            }

            switch(savetype) {
                case POI:
                    if(!opposing || (!(opposing && target->isPlayer()))) {
                        if(!immuneToPoison() || !immuneToDisease()) {
                            printColor("^GYour resistance against poison and disease has increased.\n");
                            if (pCreature) logn("log.saveswtf", "chkSave(): %s[L%d]'s POI save just went up. Upchance: %d, UpChanceRoll: %d, Prev: %d%, New: %d%, Gains: %d\n", 
                                pCreature->getCName(),pCreature->getLevel(), upchance, upchanceRoll, saves[POI].chance, saves[POI].chance+2, saves[POI].gained+1);
                            saves[POI].chance += 2;
                            saves[POI].gained += 1;
                        }
                    }
                    break;
                case DEA:
                    printColor("^DYour chances of avoiding deadly traps and death magic have increased.\n");
                    if (pCreature) logn("log.saveswtf", "chkSave(): %s[L%d]'s DEA save just went up. Upchance: %d, UpChanceRoll: %d, Prev: %d%, New: %d%, Gains: %d\n", 
                        pCreature->getCName(), pCreature->getLevel(), upchance, upchanceRoll, saves[DEA].chance, saves[DEA].chance+2, saves[DEA].gained+1);
                    saves[DEA].chance += 2;
                    saves[DEA].gained += 1;
                    break;
                case BRE:
                    printColor("^RYour ability to avoid breath weapons and explosions has increased.\n");
                     if (pCreature) logn("log.saveswtf", "chkSave(): %s[L%d]'s BRE save just went up. Upchance: %d, UpChanceRoll: %d, Prev: %d%, New: %d%, Gains: %d\n", 
                        pCreature->getCName(), pCreature->getLevel(), upchance, upchanceRoll, saves[BRE].chance, saves[BRE].chance+2, saves[BRE].gained+1);
                    saves[BRE].chance += 2;
                    saves[BRE].gained += 1;
                    break;
                case MEN:
                    printColor("^YYou are better able to avoid mental attacks.\n");
                    if (pCreature) logn("log.saveswtf", "chkSave(): %s[L%d]'s MEN save just went up. Upchance: %d, UpChanceRoll: %d, Prev: %d%, New: %d%, Gains: %d\n", 
                        pCreature->getCName(), pCreature->getLevel(), upchance, upchanceRoll, saves[MEN].chance, saves[MEN].chance+2, saves[MEN].gained+1);
                    saves[MEN].chance += 2;
                    saves[MEN].gained += 1;
                    break;

                case SPL:
                    if(opposing) {
                        if(target->isMonster()) {
                            if(natural == 1) {
                                printColor("^MYour ability to avoid or withstand spells and magical attacks has increased.\n");
                                if (pCreature) logn("log.saveswtf", "chkSave(): %s[L%d]'s SPL save just went up. (OPPOSING) Upchance: %d, UpChanceRoll: %d, Prev: %d%, New: %d%, Gains: %d\n", 
                                    pCreature->getCName(), pCreature->getLevel(), upchance, upchanceRoll, saves[SPL].chance, saves[SPL].chance+2, saves[SPL].gained+1);
                                saves[SPL].chance += 2;
                                saves[SPL].gained += 1;
                            }
                        } else
                            break;
                    } else {
                        if(natural == 1) {
                            printColor("^MYour resistance against spells and magical attacks has increased.\n");
                            if (pCreature) logn("log.saveswtf", "chkSave(): %s[L%d]'s SPL save just went up. Upchance: %d, UpChanceRoll: %d, Prev: %d%, New: %d%, Gains: %d\n", 
                                pCreature->getCName(), pCreature->getLevel(), upchance, upchanceRoll, saves[SPL].chance, saves[SPL].chance+2, saves[SPL].gained+1);
                            saves[SPL].chance += 2;
                            saves[SPL].gained += 1;
                        }
                    }
                    break;

                default:
                    break;
                }//end switch

                if(isCt() || (pCreature && pCreature->flagIsSet(P_SAVE_DEBUG)))
                     printColor("^YSave[%d] gained so far: %d\n", savetype, saves[savetype].gained);

                if(saves[savetype].gained == 5)
                    printColor("^DYou must level up in order to raise this save further.\n");

            }//end upchance check

        }

    if(isPlayer())
        getAsPlayer()->statistics.attemptSave();
    if(roll <= chance && natural == 1) {
        // save made
        if(isPlayer())
            getAsPlayer()->statistics.save();
        return(true);
    }
    return(false);
}

//*********************************************************************
//                      clearAsEnemy
//*********************************************************************
// This function clears the creature sent to it from the enemy list of every
// other creature in the vicinity. -TC

void Creature::clearAsEnemy() {
    if(!getRoomParent())
        return;

    for(const auto& mons : getRoomParent()->monsters) {
        mons->clearEnemy(Containable::downcasted_shared_from_this<Creature>());
    }
}

//*********************************************************************
//                      damageArmor
//*********************************************************************

void Player::damageArmor(int dmg) {
    std::shared_ptr<Object> armor=nullptr;

    // Damage armor 1/10th of the time
    if(Random::get(1, 10) == 1)
        return;

    int wear = chooseItem();
    if(!wear)
        return;

    armor = ready[wear-1];
    if(!armor)
        return;

    // Rings will remain undamageable
    if(armor->getWearflag() >= FINGER && armor->getWearflag() <= FINGER8)
        return;

    if(armor->getType() != ObjectType::ARMOR)
        return;

    std::string armorType = armor->getArmorType();
    int armorSkill = (int)getSkillGained(armorType);
    int avoidChance = armorSkill / 4;


    // Make armor skill worth something, the higher the skill, the lower the chance it'll take damage
    if(Random::get(1,100) > avoidChance) {
        printColor("Your ^W%s^x just got a little more scratched.\n", armor->getCName());
        armor->decShotsCur();
        if(armorType != "shield")
            checkImprove(armorType, true);
    }

    checkArmor(wear);
}

//*********************************************************************
//                      checkArmor
//*********************************************************************

void Player::checkArmor(int wear) {
    if(!ready[wear-1])
        return;

    if(ready[wear-1]->getShotsCur() < 1) {
        printColor("Your %s fell apart.\n", ready[wear-1]->getCName());
        if(ready[wear-1]->flagIsSet(O_CURSED)) {
            logn("log.curseabuse", "%s's %s fell apart. Room: %s\n",
                getCName(), ready[wear-1].get(), getRoomParent()->fullName().c_str());
        }
        broadcast(getSock(), getRoomParent(), "%M's %s fell apart.", this, ready[wear-1]->getCName());

        Limited::remove(Containable::downcasted_shared_from_this<Player>(), ready[wear-1]);
        unequip(wear, Limited::isLimited(ready[wear-1]) ? UNEQUIP_DELETE : UNEQUIP_ADD_TO_INVENTORY);
        computeAC();
    }
}

//*********************************************************************
//                      findFirstEnemyCrt
//*********************************************************************

std::shared_ptr<Creature>Creature::findFirstEnemyCrt(const std::shared_ptr<Creature>& pet) {
    if(!pet->getMaster())
        return(Containable::downcasted_shared_from_this<Creature>());

    for(const auto& mons : pet->getRoomParent()->monsters) {
        if(mons == pet) continue;
        if(mons->getAsMonster()->isEnemy(pet->getMaster()) && mons->getName() == getName()) return(mons);
    }

    return(Containable::downcasted_shared_from_this<Creature>());
}


//*********************************************************************
//                      doDamage
//*********************************************************************
// Handles actually dealing damage to someone.  Will check if the target died
// and will increase battle focus for fighters
//  1 = died, 0 = didn't die

int Creature::doDamage(std::shared_ptr<Creature> target, int dmg, DeathCheck shouldCheckDie, DamageType dmgType) {
    bool freeTarget=true;
    return(doDamage(target, dmg, shouldCheckDie, dmgType, freeTarget));
}

int Creature::doDamage(const std::shared_ptr<Creature>& target, int dmg, DeathCheck shouldCheckDie, DamageType dmgType, bool &freeTarget) {
    std::shared_ptr<Player> pTarget = target->getAsPlayer();
    std::shared_ptr<Monster>  mTarget = target->getAsMonster();
    std::shared_ptr<Player> pThis = getAsPlayer();
    std::shared_ptr<Monster>  mThis = getAsMonster();
    std::shared_ptr<Creature>  cThis = getAsCreature();
    
    int m = std::min(target->hp.getCur(), dmg);

    target->hp.decrease(dmg);
    //checkTarget(target);
    if(mTarget) {
        mTarget->lasttime[LT_AGGRO_ACTION].ltime = time(nullptr);
        mTarget->adjustThreat(Containable::downcasted_shared_from_this<Creature>(), m);
    }
    if(mThis) {
        mThis->adjustContribution(target, dmg/2);
    }

    if(this != target.get()) {
        // If we're a player and a fighter, give them some focus
        if(pThis && pThis->getClass() == CreatureClass::FIGHTER && !pThis->hasSecondClass()) {
            // Increase battle focus here
            pThis->increaseFocus(FOCUS_DAMAGE_OUT, dmg, target);
        }
        // If the target is a player and isn't dead give them some focus
        if(pTarget && pTarget->hp.getCur() > 0 && pTarget->getClass() == CreatureClass::FIGHTER && !pTarget->hasSecondClass()) {
            // Increase battle focus here
            pTarget->increaseFocus(FOCUS_DAMAGE_IN, dmg);
        }
    }

    
    target->doCheckBreakMagicalHolds(cThis);


    if(shouldCheckDie == CHECK_DIE)
        return(target->checkDie(Containable::downcasted_shared_from_this<Creature>(), freeTarget));
    else if(shouldCheckDie == CHECK_DIE_ROB)
        return(target->checkDieRobJail(mThis, freeTarget));

    freeTarget = false;
    return(0);
}

//*********************************************************************
//                      simultaneousDeath
//*********************************************************************
// Due to spells like reflect-magic and fire-shield, it is possible for, during
// an attack, both attacker and defender to be killed. This function handles
// properly freeing the combatants in those scenarios.

void Creature::simultaneousDeath(const std::shared_ptr<Creature>& attacker, std::shared_ptr<Creature> target, bool freeAttacker, bool freeTarget) {
    // be very careful: we have to free the targets in the right order

    // !freeTarget means 1) we haven't check if they've died or 2) they didnt die. Either way, check again.
    if(!freeTarget) {
        target->checkDie(attacker, freeTarget);
    }
    // !freeAttacker means 1) we haven't check if they've died or 2) they didnt die. Either way, check again.
    if(!freeAttacker) {
        attacker->checkDie(target, freeAttacker);
    }

    // clean up anything that might still need cleaning
    if(freeTarget && target->isMonster()) {
        target->getAsMonster()->finishMobDeath(attacker);
        target = nullptr;
    }
    if(freeAttacker && attacker->isMonster()) {
        attacker->getAsMonster()->finishMobDeath(target);
    }
}

//*********************************************************************
//                      tryToPoison
//*********************************************************************
// noRandomChance = always can poison if they don't save (for breath attacks)
// Return: true if they are poisoned, false if they aren't

bool Monster::tryToPoison(const std::shared_ptr<Creature>& target, SpecialAttack* pAttack) {
    std::shared_ptr<Player> pTarget = target->getAsPlayer();
    std::shared_ptr<BaseRoom> room=getRoomParent();
    int saveBonus = 0;
    if(pAttack)
        saveBonus = pAttack->saveBonus;

    // If this is from a special attack, we don't need the WILL_POISON flag
    if(!flagIsSet(M_WILL_POISON) && !pAttack)
        return(false);
    if(pTarget && target->isEffected("stoneskin"))
        return(false);
    if(target->isPoisoned())
        return(false);

    if(!pAttack && Random::get(1, 100) > 15)
        return(false);

    if(target->immuneToPoison()) {
        target->printColor("^G%M tried to poison you!\n", this);
        broadcastGroup(false, target, "%M tried to poison %N.\n", this, target.get());
        broadcast(getSock(), target->getSock(), room, "%M tried to poison %N.", this, target.get());
        return(false);
    }

    int duration=0;

    if(!target->chkSave(POI, Containable::downcasted_shared_from_this<Monster>(), saveBonus)) {
        target->printColor("^G%M poisons you!\n", this);
        broadcastGroup(false, target, "^G%M poisons %N.\n", this, target.get());
        broadcast(getSock(), target->getSock(), room, "^G%M poisons %N.", this, target.get());

        if(poison_dur) {
            duration = poison_dur - 12*bonus(target->constitution.getCur());
            duration = std::max(120, std::min(duration, 1200));
        } else {
            duration = (Random::get(2,3)*60) - 12*bonus(target->constitution.getCur());
        }

        target->poison(isPet() ? getMaster() : Containable::downcasted_shared_from_this<Monster>(), poison_dmg ? poison_dmg : level, duration);
        return(true);
    } else if(pAttack) {
        target->print("You avoided being poisoned!\n");
    }

    return(false);
}

//*********************************************************************
//                      tryToStone
//*********************************************************************
// Return: true if they have been petrified, false if they haven't

bool Monster::tryToStone(const std::shared_ptr<Creature>& target, SpecialAttack* pAttack) {
    std::shared_ptr<Player> pTarget = target->getAsPlayer();
    int     bns=0;
    bool    avoid=false;

    // If a special attack we don't need the can stone flag
    if(!flagIsSet(M_CAN_STONE) && !pAttack)
        return(false);

    if(!pTarget)
        return(false);
    if(pTarget->isEffected("petrification") || pTarget->isUnconscious())
        return(false);

    if(!pAttack && Random::get(1, 100) > 5)
        return(false);

    if((pTarget->isEffected("resist-earth") || pTarget->isEffected("resist-magic")) && Random::get(1,100) <= 50)
        avoid = true;

    pTarget->wake("Terrible nightmares disturb your sleep!");
    bns = 10*(pTarget->getLevel()-level) + 3*bonus(pTarget->constitution.getCur());
    if(pAttack)
        bns += pAttack->saveBonus;

    bns = std::max(0,std::min(bns,75));

    if(pTarget->isStaff() || avoid || pTarget->chkSave(DEA, pTarget, bns)) {
        broadcast(getSock(), getRoomParent(), "%M tried to petrify %N!", this, pTarget.get());
        target->printColor("^c%M tried to petrify you!^x\n", this);
    } else {
        pTarget->addEffect("petrification", 0, 0, nullptr, true, Containable::downcasted_shared_from_this<Monster>());
        broadcast(target->getSock(), getRoomParent(), "%M turned %N to stone!", this, pTarget.get());
        target->printColor("^D%M turned you to stone!^x\n", this);
        pTarget->clearAsEnemy();
        pTarget->removeFromGroup();
        //remFromGroup(pTarget);
        return(true);
    }

    return(false);
}

//*********************************************************************
//                      tryToDisease
//*********************************************************************
// Return: true if they have been diseased, false if they haven't

bool Monster::tryToDisease(const std::shared_ptr<Creature>& target, SpecialAttack* pAttack) {
    int bns = 0;

    // Don't need the diseases flag if this is a special attack
    if(!flagIsSet(M_DISEASES) && !pAttack)
        return(false);
    if(target->isPlayer() && target->isEffected("stoneskin"))
        return(false);
    if(!pAttack && Random::get(1, 100) > 15)
        return(false);

    if(target->immuneToDisease()) {
        target->printColor("^c%M tried to infect you!\n", this);
        broadcastGroup(false, target, "%M tried to infect %N.\n", this, target.get(), 1);
        broadcast(getSock(), target->getSock(), getRoomParent(), "%M tried to infect %N.", this, target.get());
        return(false);
    }

    if(pAttack)
        bns = pAttack->saveBonus;

    if(!target->chkSave(POI, Containable::downcasted_shared_from_this<Monster>(), bns)) {
        target->printColor("^D%M infects you.\n", this);
        broadcastGroup(false, target, "%M infects %N.\n", this, target.get(), 1);
        broadcast(getSock(), target->getSock(), getRoomParent(), "%M infected %N.", this, target.get());
        if(isPet())
            getMaster()->printColor("^D%M infects %N.\n", this, target.get());
        else
            target->disease(Containable::downcasted_shared_from_this<Monster>(), target->hp.getMax()/20);
        return(true);
    } else if(pAttack) {
        target->print("You narrowly avoid catching a disease!\n");
    }

    return(false);
}

//*********************************************************************
//                      tryToBlind
//*********************************************************************
// Return: true if they have been blinded, false if they haven't

bool Monster::tryToBlind(const std::shared_ptr<Creature>& target, SpecialAttack* pAttack) {
    // Don't need the blind flag if it's a special attack
    if(!flagIsSet(M_WILL_BLIND) && !pAttack)
        return(false);

    if(!pAttack && Random::get(1, 100) > 15)
        return(false);
    int bns = 0;
    if(pAttack)
        bns = pAttack->saveBonus;
    if(!target->chkSave(LCK, Containable::downcasted_shared_from_this<Monster>(), bns)) {
        target->printColor("^y%M blinds your eyes.\n",this);
        broadcastGroup(false, target, "%M blinds %N.\n", this, target.get());
        if(isPet()) {
            getMaster()->printColor("^y%M blinds %N.\n", this, target.get());
        }
        target->addEffect("blindness", 180 - (target->constitution.getCur()/10), 1, Containable::downcasted_shared_from_this<Monster>(), true, Containable::downcasted_shared_from_this<Monster>());
        return(true);
    } else if(pAttack) {
        target->print("You narrowly avoided going blind!\n");
    }
    return(false);
}

