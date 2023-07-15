/*
 * attack.cpp
 *   Routines to handle player combat
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

#include <cstring>                   // for strcpy
#include <ctime>                     // for time
#include <list>                      // for list, operator==, _List_iterator
#include <map>                       // for operator==, _Rb_tree_iterator
#include <set>                       // for set
#include <string>                    // for string, allocator, operator+
#include <type_traits>               // for enable_if<>::type
#include <utility>                   // for pair

#include "cmd.hpp"                   // for cmd
#include "commands.hpp"              // for cmdAttack
#include "config.hpp"                // for Config, gConfig, DeityDataMap
#include "creatureStreams.hpp"       // for Streamable, ColorOff, ColorOn
#include "damage.hpp"                // for Damage, REFLECTED_FIRE_SHIELD
#include "deityData.hpp"             // for DeityData
#include "dice.hpp"                  // for Dice
#include "effects.hpp"               // for EffectInfo
#include "factions.hpp"              // for Faction
#include "flags.hpp"                 // for P_SITTING, M_UNKILLABLE, P_CHAOTIC
#include "global.hpp"                // for CreatureClass, WIELD, HELD, ARAMON
#include "lasttime.hpp"              // for lasttime
#include "magic.hpp"                 // for get_spell_function, splOffensive
#include "monType.hpp"               // for noLivingVulnerabilities, ARACHNID
#include "mud.hpp"                   // for LT_MAUL, ospell, LT
#include "mudObjects/container.hpp"  // for Container, PlayerSet
#include "mudObjects/creatures.hpp"  // for Creature, ATTACK_BASH, ATTACK_KICK
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/objects.hpp"    // for Object
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "playerClass.hpp"           // for PlayerClass
#include "proto.hpp"                 // for broadcast, bonus, get_spell_lvl
#include "raceData.hpp"              // for RaceData
#include "random.hpp"                // for Random
#include "realm.hpp"                 // for NO_REALM, Realm, FIRE
#include "statistics.hpp"            // for Statistics
#include "stats.hpp"                 // for Stat
#include "structs.hpp"               // for osp_t



std::shared_ptr<Creature> Creature::findVictim(const std::string &toFind, int num, bool aggressive, bool selfOk, const std::string &noVictim, const std::string &notFound) {
    std::shared_ptr<Creature> victim=nullptr;
    std::shared_ptr<Player> pVictim=nullptr;
    if(toFind.empty()) {
        if(hasAttackableTarget()) {
            return(getTarget());
        }
        if(!noVictim.empty())
            *this << ColorOn << noVictim << ColorOff;;
        return(nullptr);
    } else {
        victim = getRoomParent()->findCreature(Containable::downcasted_shared_from_this<Creature>(), toFind, num, true, true);

        if(victim)
            pVictim = victim->getAsPlayer();

        if(!victim || (aggressive && (pVictim || victim->isPet()) && toFind.length() < 3) || (!selfOk && victim.get() == this)) {
            if(!notFound.empty())
                *this << ColorOn << notFound << ColorOff;
            return(nullptr);
        }
        return(victim);
    }
}
std::shared_ptr<Creature> Creature::findVictim(cmd* cmnd, int cmndNo, bool aggressive, bool selfOk, const std::string &noVictim, const std::string &notFound) {
    return(findVictim(cmnd->str[cmndNo], cmnd->val[cmndNo], aggressive, selfOk, noVictim, notFound));
}

//*********************************************************************
//                      attack
//*********************************************************************
// This function allows the player pointed to by the first parameter
// to attack a monster.

int cmdAttack(const std::shared_ptr<Creature>& creature, cmd* cmnd) {
    std::shared_ptr<Creature>victim=nullptr;
    std::shared_ptr<Player> pVictim=nullptr, pPlayer=nullptr;

    if(!creature->ableToDoCommand())
        return(0);

    if (creature->isMagicallyHeld(true))
        return(0);

    pPlayer = creature->getAsPlayer();
    std::shared_ptr<Monster>  pet = creature->getAsMonster();
    if(pet) {
        if(cmnd->num < 2) {
            pet->getMaster()->print("%M stops attacking.\n", pet.get());
            pet->clearEnemyList();
            return(0);
        }
    }

    if(!(victim = creature->findVictim(cmnd, 1, true, false, "Attack what?\n", "You don't see that here.\n")))
        return(0);

    pVictim = victim->getAsPlayer();

    if(!creature->canAttack(victim))
        return(0);

    // pet attack
    if(!pPlayer) {
        if(pet) {
            pet->smashInvis();
            pet->getMaster()->smashInvis();

            if(pet->isEnemy(victim)) {
                pet->getMaster()->print("%M is already attacking %N.\n", pet.get(), victim.get());
            } else {
                creature->getMaster()->print("%M attacks %N.\n", pet.get(), victim.get());
                broadcast(creature->getMaster()->getSock(), pet->getRoomParent(), "%M tells %N to attack %N.",
                    pet->getMaster().get(), pet.get(), victim.get());
                pet->addEnemy(victim);

                if(!pVictim) {
                    // monster will attack the owner if their pet attacks
                    victim->getAsMonster()->addEnemy(pet->getMaster());
                    // Activates lag protection.
                    if(pet->getMaster()->flagIsSet(P_LAG_PROTECTION_SET))
                        pet->getMaster()->setFlag(P_LAG_PROTECTION_ACTIVE);
                }
            }
        }
    } else
        pPlayer->attackCreature(victim, ATTACK_NORMAL);

    return(0);
}

//*********************************************************************
//                      canAttack
//*********************************************************************

bool Creature::canAttack(const std::shared_ptr<Creature>& target, bool stealing) {
    std::shared_ptr<Creature>check=nullptr;
    std::shared_ptr<Player> pCheck=nullptr, pThis = getAsPlayer();
    bool    holy_war;
    std::string verb = stealing ? "steal from" : "attack";

    if (!target)
        return(false);

    // monsters don't use this function, but pets have to obey their masters
    if(!pThis) {
        if(isPet()) {
            if(target == getMaster()) {
                getMaster()->print("Pets cannot %s their masters.\n", verb.c_str());
                return(false);
            }
            return(getMaster()->canAttack(target));
        } else
            return(true);
    }

    clearFlag(P_AFK);

    if (isMagicallyHeld(true))
        return(false);

    if( target->isPet() && target->getMaster().get() == this &&
        !checkStaff("You cannot %s your pet.\n", verb.c_str()) )
        return(false);



    // if they're trying to kill the pet, we should check the player
    // for PK-ability
    check = target->isPet() ? target->getMaster() : target;
    pCheck = check->getAsPlayer();


    // no attacking staff, but let them attack a staff's pet if
    // it's currently attacking them
    if( check->isStaff() &&
        cClass < check->getClass() &&
        !(target->isPet() && target->getAsMonster()->isEnemy(Containable::downcasted_shared_from_this<Creature>()) ))
    {
        print("You are not allowed to %s %N.\n", verb.c_str(), target.get());
        return(false);
    }

    if(isCt()) {
        stand();
        return(true);
    }

    // this only happens on autoattack, otherwise the findCreature would prevent us from
    // getting to this message. for this reason, we don't need to print a message.
    if(!canSee(target))
        return(false);

    // builders got restricted attacks
    if(cClass == CreatureClass::BUILDER) {
        if(pCheck) {
            print("You are not allowed to %s players.\n", verb.c_str());
            return(false);
        }
        if(!flagIsSet(P_BUILDER_MOBS)) {
            print("You are not allowed to %s monsters.\n", verb.c_str());
            return(false);
        }
        if(!pThis->checkBuilder(getUniqueRoomParent())) {
            print("Error: Room number not inside any of your alotted ranges.\n");
            return(false);
        }
    }

    if(flagIsSet(P_SITTING))
        stand();


    // we are attacking a player or a pet
    if(pCheck) {

        if(pCheck->flagIsSet(P_DIED_IN_DUEL)) {
            print("%M just lost a duel!\nYou can't %s ", target.get(), verb.c_str());
            if(pCheck==target)
                print("%s", target->himHer());
            else
                print("%N", target.get());
            print(" right now.\n");
            return(false);
        }

        if(pCheck->isEffected("petrification") && !isCt()) {
            print("You can't %s %N!", verb.c_str(), target.get());
            if(pCheck==target)
                print("%s", target->upHeShe());
            else
                print("%M", pCheck.get());
            print("'s petrified!\n");
            return(false);
        }

        // will charm stop them?
        if(vampireCharmed(pCheck) || (pCheck->hasCharm(getName()) && flagIsSet(P_CHARMED))) {
            print("You like %s too much to do that.\n", pCheck->getCName());
            return(false);
        } else {
            if(target->isPlayer())
                target->getAsPlayer()->delCharm(Containable::downcasted_shared_from_this<Creature>());
        }

        // check outlawness
        if(pCheck->flagIsSet(P_OUTLAW)) {

            if(pCheck->getRoomParent()->isOutlawSafe()) {
                print("You cannot %s outlaws in this room.\n", verb.c_str());
                return(false);
            }

        // almost everything else concerns non-outlaws
        } else {

            // you can attack pets if they are in combat,
            // but not their master
            if(gConfig->getPkillInCombatDisabled() && !target->isPet() && target->inCombat(false)) {
                print("Not in the middle of combat.\n");
                return(false);
            }

            if(getRoomParent()->isPkSafe()) {
                print("No %s allowed in this room.\n", stealing ? "stealing" : "killing");
                return(false);
            }

            // if not dueling
            if(!induel(pThis, pCheck)) {

                if(pCheck->flagIsSet(P_NO_PKILL)) {
                    print("You cannot %s %N right now.\n", verb.c_str(), check.get());
                    return(false);
                }


                // Paladins-DKs and Enoch-Aramon are always at war
                holy_war =
                    ((pCheck->getClass() == CreatureClass::DEATHKNIGHT && cClass == CreatureClass::PALADIN) ||
                    (cClass == CreatureClass::DEATHKNIGHT && pCheck->getClass() == CreatureClass::PALADIN) ||
                    ((deity == LINOTHAN || deity == MARA) && pCheck->getDeity() == ARACHNUS) ||
                    (deity == ARACHNUS && (pCheck->getDeity() == LINOTHAN || pCheck->getDeity() == MARA)) ||
                    (deity == ENOCH && pCheck->getDeity() == ARAMON) ||
                    (deity == ARAMON && pCheck->getDeity() == ENOCH) );


                if(!holy_war &&
                    (!flagIsSet(P_PLEDGED) || !pCheck->flagIsSet(P_PLEDGED)) &&
                    (   (
                        pThis &&
                        !gConfig->getGuild( pThis->getGuild() )
                    ) || !gConfig->getGuild( pCheck->getGuild() )
                    )
                ) {
                    if(!flagIsSet(P_CHAOTIC)) {
                        print("Sorry, you're lawful.\n");
                        return(false);
                    }
                    if(!pCheck->flagIsSet(P_CHAOTIC)) {
                        print("Sorry, %N is lawful.\n", check.get());
                        return(false);
                    }
                }

                if(pCheck->getLevel() <= 5 && level >= 10) {
                    printColor("^yPick on someone your own size!\n");
                    return(false);
                }

                if(pCheck->getLevel() >= 10 && level <= 5) {
                    printColor("^yPick on someone you can kill.\n");
                    return(false);
                }

            }
        }


        clearFlag(P_NO_PKILL);

    } else {
        std::shared_ptr<Monster>  mTarget = target->getAsMonster();

        // attacking a mob
        if( mTarget->flagIsSet(M_UNKILLABLE) &&
            !checkStaff("You cannot %s %s.\n", stealing ? "steal from" : "harm", mTarget->himHer()))
            return(false);

        if( mTarget->flagIsSet(M_PERMENANT_MONSTER) &&
            level < 3 &&
            mTarget->getLevel() > level &&
            !getRoomParent()->flagIsSet(R_ONE_PERSON_ONLY) &&
            !mTarget->isEnemy(Containable::downcasted_shared_from_this<Creature>()) &&
            !checkStaff("That wouldn't be very prudent at this time.\n")
        )
            return(false);
    }



    if(!stealing && pThis && target->isPlayer()) {
        for(const auto& pet : target->pets) {
            if(pet && pet->isPet() &&
                pet->getMaster() == target &&
                this != target->getMaster().get())
            {
                if(!pet->isEnemy(Containable::downcasted_shared_from_this<Creature>())) {
                    pet->addEnemy(Containable::downcasted_shared_from_this<Creature>());

                    broadcast(target->getSock(), getRoomParent(), "%M stands loyally before its master!", pet.get());
                }
                break;
            }
        }
    }

    return(true);
}

//*********************************************************************
//                      getUnarmedWeaponSkill
//*********************************************************************

std::string Player::getUnarmedWeaponSkill() const {
    if(isEffected("lycanthropy"))
        return("claw");
    auto it = gConfig->classes.find(getClassString());
    if(it == gConfig->classes.end() || !(*it).second)
        return("bare-hand");
    return((*it).second->getUnarmedWeaponSkill());
}

//*********************************************************************
//                      checkWeapon
//*********************************************************************
// Called multiple times during an attack - before swing, after swing, on fumble,
// and in the event of our weapon shattering. If the weapon is no longer usable,
// it handles the removing of the weapon, the ending of the attack (no more swings
// with this weapon) and recalculating the player's attack power.

void checkWeapon(const std::shared_ptr<Player>& player, std::shared_ptr<Object> &weapon, bool alwaysRemove, int* loc, int* attacks, bool* wielding, bool multiWeapon, UnequipAction action = UNEQUIP_ADD_TO_INVENTORY) {
    // Nothing to break
    if(!*wielding || !weapon)
        return;

    // Not broken!
    if(!alwaysRemove && !player->breakObject(weapon, *loc))
        return;

    if(*loc != -1) {
        player->unequip(*loc, action);
    } else {
        // ::isDm -> Global isDm function, not class local isDm function
        broadcast(::isDm, "^g>>> Fumble: BadLoc (Loc:%d) %'s %s\n", *loc, player->getCName(), weapon->getCName());
        if(player->ready[WIELD-1] == weapon) {
            player->unequip(WIELD);
        } else if(player->ready[HELD-1] == weapon) {
            player->unequip(HELD);
        }
    }

    if(multiWeapon)
        *attacks = 1;

    weapon.reset();
    *loc = -1;
    *wielding = false;

    player->computeAttackPower();
}

//*********************************************************************
//                      attackCreature
//*********************************************************************
// This function does the actual attacking.  The first parameter contains
// a pointer to the attacker and the second contains a pointer to the
// victim.  A 1 is returned if the attack results in death.

int Player::attackCreature(const std::shared_ptr<Creature> &victim, AttackType attackType) {
    std::shared_ptr<Player> pVictim = nullptr;
    std::shared_ptr<Monster> mVictim = nullptr;
    bool duelWield = false, wielding = false;
    bool multiWeapon = false;
    std::shared_ptr<Object> weapon = nullptr;
    int attacks = 1, attacked = 0;//, enchant=0;
    int loc = -1;
    EffectInfo *deathSickness = getEffect("death-sickness");
    Damage attackDamage;

    long t = time(nullptr);
    int drain = 0, hit = 0, wcdmg = 0;
    bool glow = true;

    std::string atk;

    if(!victim)
        return(0);

    pVictim = victim->getAsPlayer();
    mVictim = victim->getAsMonster();

    if(!ableToDoCommand())
        return(0);

    if(isMagicallyHeld(false))
        return(0);

    if(attackType != ATTACK_BASH && attackType != ATTACK_AMBUSH && attackType != ATTACK_MAUL && attackType != ATTACK_KICK) {
        if(!checkAttackTimer())
            return(0);

        updateAttackTimer();


        // TODO: Make this based on the strength of the haste effect
        // Haste Adjustment
        // Base attack = 3, haste = -1 for 2
        // this results in a 1/3rd reduction in speed, or 2/3rd of normal
        // This is fine for weapons with 3 delay..but we can have 2, 1, etc delay and
        // a -1 across the board causes problems, so change it to 2/3rd and 4/3
        if(isEffected("frenzy") || isEffected("haste"))
            setAttackDelay( (int)((getAttackDelay()*2.0)/3.0));
        if(isEffected("slow") && !isEffected("haste") && !isEffected("frenzy"))
            setAttackDelay( (int)((getAttackDelay()*4.0)/3.0));


        if(deathSickness && Random::get(1,100) < deathSickness->getStrength()) {
            *this << ColorOn << "^yYou cough heavily as you attack.\n" << ColorOff;
            modifyAttackDelay(10);
        }

        if(isEffected("lycanthropy")) {
            if(LT(this, LT_MAUL) <= t) {
                lasttime[LT_MAUL].ltime = t;
                if(isEffected("slow") && !isEffected("haste") && !isEffected("frenzy"))
                    lasttime[LT_MAUL].interval = 4L;
                else
                    lasttime[LT_MAUL].interval = 3L;
            }
        }
    }

    smashInvis();
    interruptDelayedActions();

    if(isBlind())
        modifyAttackDelay(30);

    if(!pVictim && mVictim) {
        // Activates Lag protection.
        if(flagIsSet(P_LAG_PROTECTION_SET))
            setFlag(P_LAG_PROTECTION_ACTIVE);

        if(mVictim->addEnemy(Containable::downcasted_shared_from_this<Player>()) && attackType == ATTACK_NORMAL) {
            *this << "You attack " << mVictim.get() << ".\n";
            broadcast(getSock(), getRoomParent(), "%M attacks %N.", this, mVictim.get());
        }

        if(mVictim->flagIsSet(M_ONLY_HARMED_BY_MAGIC) && !checkStaff("Your weapon%s had no effect on %N.\n", duelWield ? "s" : "", mVictim.get())) {
            getParent()->wake("Loud noises disturb your sleep.", true);
            return(0);
        }
    } else if(pVictim && attackType == ATTACK_NORMAL) {
        *pVictim << this << " attacked you!\n";

        broadcast(getSock(), pVictim->getSock(), getRoomParent(), "%M attacked %N!", this, pVictim.get());
    }

    if(attackType != ATTACK_KICK && attackType != ATTACK_MAUL) {
        // A monk that has no weapon, no holding item, but is wearing gloves gets to use the enchant off of them
        if (cClass == CreatureClass::MONK && !ready[WIELD - 1] && !ready[HELD - 1] && ready[HANDS - 1]) {
            //enchant = abs(ready[HANDS-1]->adjustment);
            weapon = nullptr;
            wielding = false;
            loc = HANDS;
        } else if (ready[WIELD - 1]) {
            weapon = ready[WIELD - 1];
            //enchant = abs(ready[WIELD-1]->adjustment);
            wielding = true;
            loc = WIELD;

            // No multiple attacks for bash
            if (attackType != ATTACK_BASH) {
                // Two attacks for duel wield
                if (ready[HELD - 1] && ready[HELD - 1]->getWearflag() == WIELD) {
                    duelWield = true;
                    attacks++;
                }
            }
        }
    } else if(attackType == ATTACK_KICK) {
        // kick
        if(ready[FEET-1]) {
            weapon = ready[FEET-1];
            //enchant = abs(weapon->adjustment);
            duelWield = false;
            loc = FEET;
        }
    }

    // No numAttacks for ambush
    if(weapon && weapon->getNumAttacks() && attackType != ATTACK_AMBUSH && attackType != ATTACK_BASH && attackType != ATTACK_MAUL) {
        // Random number of attacks 1-numAttacks
        attacks = Random::get<short>(1, weapon->getNumAttacks());
        // TODO: maybe add a flag so always certain # of attacks
        multiWeapon = true;
        // No multi attack weapons if duel wielding
        duelWield = false;
    }

    // While the current attack is less than how many attacks we have, keep going
    while(attacked++ < attacks) {
        // Using a do/while so we can do a break and drop to the end
        do {

            checkWeapon(Containable::downcasted_shared_from_this<Player>(), weapon, false, &loc, &attacks, &wielding, multiWeapon);
            if(wielding) {
                if(breakObject(weapon, loc)) {
                    weapon = nullptr;
                    wielding = false;
                    loc = -1;
                    if(multiWeapon)
                        attacks = 1;
                    break;
                }
            }

            int resultFlags = 0;
            int altSkillLevel = -1;

            if(attackType == ATTACK_KICK) {
                // Can't fumble your boots when kicking
                resultFlags |= NO_FUMBLE;
                altSkillLevel = (int)getSkillGained("kick");
            } else if(attackType == ATTACK_MAUL) {
                resultFlags |= NO_FUMBLE;
                altSkillLevel = (int)getSkillGained("maul");
            }

            AttackResult result = getAttackResult(victim, weapon, resultFlags, altSkillLevel);
            // We can only fumble on the first hit of a multi weapon attack,
            // so if this is a fumble and NOT the first hit, change it to a normal hit
            if(result == ATTACK_FUMBLE && multiWeapon && attacked != 1)
                result = ATTACK_HIT;

            // Special rules for ambush
            if(attackType == ATTACK_AMBUSH) {
                if(result == ATTACK_HIT || result == ATTACK_CRITICAL || result == ATTACK_BLOCK || result == ATTACK_GLANCING) {
                    if(!pVictim && victim->flagIsSet(M_NO_BACKSTAB))
                        result = ATTACK_MISS;
                }
            }

            statistics.swing();

            if( result == ATTACK_HIT || result == ATTACK_CRITICAL || result == ATTACK_BLOCK || result == ATTACK_GLANCING ) {
                // move glow string into hit if so we aren't glowing if we miss
                if(!canHit(victim, weapon, glow))
                    break;
                // only glow once
                glow=false;

                float multiplier = 1.0;
                if(attackType == ATTACK_AMBUSH)
                    multiplier = 1.5;
                else if(attackType == ATTACK_BASH) {
                    if(cClass == CreatureClass::BERSERKER)
                        multiplier = .75;
                    else
                        multiplier = 0.5;

                    if(ready[SHIELD-1] && ready[SHIELD-1]->flagIsSet(O_ENHANCE_BASH))
                        multiplier += .1;
                }


                bool computeBonus = (!multiWeapon || (multiWeapon && attacked==1));
                attackDamage.reset();

                // Return of 1 means the weapon was shattered or otherwise rendered unsuable
                if(computeDamage(victim, weapon, attackType, result, attackDamage, computeBonus, drain, multiplier) == 1)
                    checkWeapon(Containable::downcasted_shared_from_this<Player>(), weapon, true, &loc, &attacks, &wielding, multiWeapon, UNEQUIP_DELETE);


                if(result == ATTACK_BLOCK) {
                    *this << ColorOn << "^C" << victim.get() << " partially blocked your attack!\n" << ColorOff;
                    *victim << ColorOn << "^CYou manage to partially block " << this << "'s attack!\n" << ColorOff;
                }

                if(result == ATTACK_CRITICAL)
                    statistics.critical();
                else
                    statistics.hit();
                if(victim->isPlayer())
                    victim->getAsPlayer()->statistics.wasHit();

                // Determine here how to handle the bonus.
                // If it's a multi attack weapon, divide the bonus over all of the attacks
                // If it's a multi attack (ambush, etc) no division of bonus
                if(!multiWeapon) {
                    attackDamage.includeBonus();
                } else {
                    attackDamage.includeBonus(attacks);
                }
                bool showToRoom = false;
                bool wasKilled = false, freeTarget = false, meKilled;

                if(attackType == ATTACK_BASH) {
                    atk =" bashed";
                    showToRoom = true;
                } else if(attackType == ATTACK_KICK) {
                    atk =" kicked";
                    showToRoom = true;
                } else if(attackType == ATTACK_MAUL) {
                    atk =" mauled";
                    showToRoom = true;
                } else {
                    atk = getDamageString(Containable::downcasted_shared_from_this<Player>(), weapon, result == ATTACK_CRITICAL ? true : false);
                }

                if(showToRoom)
                    broadcast(getSock(), getRoomParent(), "%M %s %N.", this, atk.c_str(), victim.get());
                log_immort(false, Containable::downcasted_shared_from_this<Player>(), "%s %s %s for %d damage.\n", getCName(), atk.c_str(), victim->getCName(), attackDamage.get());
                *this << ColorOn << "You " << atk.c_str() << " " << victim.get() << " for " << customColorize("*CC:DAMAGE*").c_str() << attackDamage.get() << " ^xdamage.\n" << ColorOff;
                *victim << ColorOn << this << " " << atk.c_str() << " " << "you" << (victim->isBrittle() ? "r brittle body":"") << " for " << victim->customColorize("*CC:DAMAGE*").c_str() << attackDamage.get() << " ^xdamage!\n" << ColorOff;



                meKilled = doReflectionDamage(attackDamage, victim);

                if(!meKilled && weapon && weapon->getMagicpower() && weapon->flagIsSet(O_WEAPON_CASTS) && weapon->getChargesCur() > 0)
                    wcdmg += castWeapon(victim, weapon, wasKilled);

                broadcastGroup(false, victim, "^M%M^x %s ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n", this, atk.c_str(), victim.get(), attackDamage.get()+drain, victim->heShe(), victim->getStatusStr(attackDamage.get()+drain));

                statistics.attackDamage(attackDamage.get()+drain, Statistics::damageWith(Containable::downcasted_shared_from_this<Player>(), weapon));

                if(weapon && !Random::get(0, 3))
                    weapon->decShotsCur();


                checkWeapon(Containable::downcasted_shared_from_this<Player>(), weapon, false, &loc, &attacks, &wielding, multiWeapon);


                attackDamage.add(wcdmg);

                if(!meKilled && drain && victim->hp.getCur() > attackDamage.get()) {

                    drain = std::min<int>(victim->hp.getCur() - attackDamage.get(), drain);
                    *this << ColorOn << "Your aura of evil drained an extra " << customColorize("*CC:DAMAGE*").c_str() << drain << " ^xhit point" << (drain == 1 ? "" : "s") << " of damage!\n" << ColorOff; 
                    *victim << ColorOn << "^r" << this << "'s aura of evil drained " << victim->customColorize("*CC:DAMAGE*").c_str() << drain << " ^rhit point" << (drain == 1 ? "" : "s") << " from you.\n" << ColorOff;
                    attackDamage.add(drain);
                    if(!pVictim)
                        hp.increase(drain);
                }

                if(attackType == ATTACK_BASH) {
                    if(victim->isPlayer())
                        victim->stun(Random::get(4,6));
                    else
                        victim->stun(Random::get(5,8));
                    checkImprove("bash", true);
                } else if(attackType == ATTACK_MAUL) {
                    int dur = 0;
                    if(!pVictim) {
                        if(!victim->flagIsSet(M_RESIST_CIRCLE) && !victim->isUndead())
                            dur = std::max(3, (Random::get(3,5) + (std::min(2,((::bonus(strength.getCur()) -
                                ::bonus(victim->strength.getCur()))/2)))));
                        else
                            dur = Random::get(2,4);
                    } else
                        dur = std::max(3,(Random::get(3,5) + (::bonus(strength.getCur()) - ::bonus(victim->strength.getCur()))));
                    victim->stun(dur);
                    checkImprove("maul", true);
                } else if(attackType == ATTACK_AMBUSH) {
                    checkImprove("ambush", true);
                } else if(attackType == ATTACK_KICK) {
                    checkImprove("kick", true);
                } else if(!pVictim && mVictim) {
                    std::string weaponSkill;
                    if(weapon)
                        weaponSkill = weapon->getWeaponType();
                    else
                        weaponSkill = getUnarmedWeaponSkill();

                    if(!weaponSkill.empty())
                        checkImprove(weaponSkill, true);
                }

                getParent()->wake("Loud noises disturb your sleep.", true);

                if( doDamage(victim, attackDamage.get(), CHECK_DIE, PHYSICAL_DMG, freeTarget) ||
                    wasKilled ||
                    meKilled )
                {
                    Creature::simultaneousDeath(Containable::downcasted_shared_from_this<Player>(), victim, false, freeTarget);
                    return(1);
                }

            } else if(result == ATTACK_MISS) {
                statistics.miss();
                if(victim->isPlayer())
                    victim->getAsPlayer()->statistics.wasMissed();

                if(attackType == ATTACK_AMBUSH && attacked == 1) {
                    // If we miss on the first attack, no more attacks because ambush was detected
                    attacked = attacks;
                    *this << "Your ambush failed!\n";
                    checkImprove("ambush", false);
                    broadcast(getSock(), getRoomParent(), "%s ambush was detected.", upHisHer());
                    setAttackDelay(getAttackDelay() * 2);
                    break;
                } else if(attackType == ATTACK_BASH) {
                    *this << "Your bash failed.\n";
                    checkImprove("bash", false);
                    *victim << this << " tried to bash you.\n";
                    broadcast(getSock(), victim->getSock(), victim->getRoomParent(), "%M tried to bash %N.", this, victim.get());
                    break;
                } else if(attackType == ATTACK_KICK) {
                    *this << "Your kick was ineffective.\n";
                    checkImprove("kick", false);
                    *victim << this << " tried to kick you.\n";
                    broadcast(getSock(), victim->getSock(), victim->getRoomParent(), "%M tried to kick %N.", this, victim.get());
                    break;
                } else if(attackType == ATTACK_MAUL) {
                    *this << "You failed to maul " << victim.get() << "\n.";
                    checkImprove("maul", false);
                    *victim << this << " tried to maul you.\n";
                    broadcast(getSock(), victim->getSock(), victim->getRoomParent(), "%M tried to maul %N.", this, victim.get());
                    break;
                }

                *this << ColorOn << "^cYou missed.\n" << ColorOff;
                *victim << ColorOn << "^c" << this << " missed.\n" << ColorOff;
                if(!pVictim)
                    broadcast(getSock(), victim->getSock(), victim->getRoomParent(), "^c%M missed %N.", this, victim.get());

                // TODO: Weapons: Look at this rate of increase
                if(!pVictim && mVictim) {
                    std::string weaponSkill;
                    if(weapon)
                        weaponSkill = weapon->getWeaponType();
                    else
                        weaponSkill = getUnarmedWeaponSkill();

                    if(!weaponSkill.empty())
                        checkImprove(weaponSkill, false);
                }
            }  else if(result == ATTACK_DODGE) {
                victim->dodge(Containable::downcasted_shared_from_this<Player>());
            } else if(result == ATTACK_PARRY) {
                int parryResult = victim->parry(Containable::downcasted_shared_from_this<Player>());
                // check riposte death here
                if(parryResult == 2) {
                    // Damn, we're dead, no more attacks then
                    getParent()->wake("Loud noises disturb your sleep.", true);
                    return(0);
                }
            } else if(result == ATTACK_FUMBLE) {
                statistics.fumble();
                *this << ColorOn << "^gYou FUMBLED your weapon.\n" << ColorOff;
                broadcast(getSock(), getRoomParent(), "^g%M fumbled %s weapon.", this, hisHer());

                checkWeapon(Containable::downcasted_shared_from_this<Player>(), weapon, true, &loc, &attacks, &wielding, multiWeapon);

            }  else {
                *this << ColorOn << "^RError!!! Unhandled attack result: " << result << "\n" << ColorOff;
            }

        } while(false); // End DO

        // If duel wielding, weapon is now second weapon, go on for the next attack
        if(attacked == 1 && duelWield) {
            weapon = ready[HELD-1];
            if(weapon) {
                //enchant = weapon->adjustment;
                wielding = true;
                loc = HELD;
            }
        }
    } // End attack loop

    if(hit == 1 && !pVictim)
        victim->flee();

    getParent()->wake("Loud noises disturb your sleep.", true);
    return(0);
}

//*********************************************************************
//                      castWeapon
//*********************************************************************

int Creature::castWeapon(const std::shared_ptr<Creature>& target, std::shared_ptr<Object>weapon, bool &meKilled) {
    Damage attackDamage;
    const char  *spellname;
    osp_t   *osp;
    int     (*fn)(SpellFn), c=0;
    int     splno=0, slvl=0;

    if(weapon == nullptr)
        weapon = ready[WIELD-1];
    if(weapon == nullptr)
        return(0);

    if(getRoomParent()->flagIsSet(R_NO_MAGIC) || weapon->getMagicpower() < 1)
        return(0);

    splno = weapon->getMagicpower() - 1;
    // Do we have sufficient charges to cast?
    if(weapon->getChargesCur() <= 0)
        return(0);

    if(splno < 0)
        return(0);

    fn = get_spell_function(splno);
    spellname = get_spell_name(splno);
    for(c = 0; ospell[c].splno != get_spell_num(splno); c++)
        if(ospell[c].splno == -1)
            return(0);
    osp = (osp_t *)&ospell[c];

    if((int(*)(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData, const char*, osp_t*))fn == splOffensive) {

        if(target->isMonster() && (get_spell_lvl(splno) > 0)) {
            slvl = get_spell_lvl(splno);

            if( (target->flagIsSet(M_NO_LEVEL_FOUR) && slvl <= 4) ||
                (target->flagIsSet(M_NO_LEVEL_THREE) && slvl <= 3) ||
                (target->flagIsSet(M_NO_LEVEL_TWO) && slvl <= 2) ||
                (target->flagIsSet(M_NO_LEVEL_ONE) && slvl <= 1))
            {
                printColor("^yYour %s's %s spell fails!\n", weapon->getCName(), spellname);
                weapon->decShotsCur();
                return(0);
            }
        }

        attackDamage.set(std::max(1, osp->damage.roll()));
        target->modifyDamage(Containable::downcasted_shared_from_this<Player>(), MAGICAL, attackDamage, osp->realm, weapon, -1);

        if(target->negAuraRepel()) {
            printColor("^c%M's negative aura absorbed some of the damage.\n", target.get());
            target->printColor("^cYour negative aura repels some of the damage.\n");
        }


        printColor("Your %s casts a %s spell on %s for %s%d^x damage.\n", weapon->getCName(),
            spellname, target->getCName(), customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());

        broadcast(getSock(), target->getSock(), getRoomParent(), "%M's %s casts a %s spell on %N.", this, weapon->getCName(), spellname, target.get());
        target->printColor("^M%M's^x %s casts a %s spell on you for %s%d^x damage.\n",
            this, weapon->getCName(), spellname, target->customColorize("*CC:DAMAGE*").c_str(), attackDamage.get());

        meKilled = doReflectionDamage(attackDamage, target);

        weapon->decChargesCur();
    }
    return(attackDamage.get());
}



//********************************************************************
//                      modifyDamage
//********************************************************************
// Modifies the damage of the attack for a variety of reasons:
//      1) Offguard (sitting / sleeping)
//      2) Lich being brittle
//      3) Undead fighting someone with undead-ward
//      4) Armor spell
//      5) Reflect-magic spell
//      6) Magic saving throws
//      7) Elemental-realm resistance spells vs magic
//      8) Elemental-realm resistance spells vs pets
//      9) Resist-magic spell
//      10) Lich natural magic resistance
//      11) Weapon-resistance spells
//      12) Berserked damage reduction (player)
//      13) Extra berserked damage
//      14) Armor damage reduction
//      15) Stoneskin spell
//      16) Werewolf silver vulnerability

void Creature::modifyDamage(const std::shared_ptr<Creature>& enemy, int dmgType, Damage& attackDamage, Realm pRealm, const std::shared_ptr<Object>&  weapon, short saveBonus, short offguard, bool computingBonus) {
    std::shared_ptr<Player> player = getAsPlayer();
    const EffectInfo *effect = nullptr;
    dmgType = std::max(0, dmgType);

// TODO: Dom: drain hp (dk)

    //
    // catch off-guard
    //
    if(enemy) {
        if( pFlagIsSet(P_SITTING) || pFlagIsSet(P_SLEEPING))
        {

            if(offguard != OFFGUARD_NOPRINT && !computingBonus) {
                if(enemy->isPlayer())
                    enemy->printColor("^rYou catch %N off guard!\n", this);

                printColor("^r%M catches you off guard!\n", enemy.get());
                print("You %s up.\n", flagIsSet(P_SITTING) ? "stand" : "wake");
            }

            // off-guard penalties are worse for sleeping
            if(flagIsSet(P_SLEEPING)) {
                stun(Random::get(7,15));
                attackDamage.set(attackDamage.get() * 2);
                if(offguard != OFFGUARD_NOREMOVE)
                    wake("", true);
            } else {
                stun(Random::get(3,4));
                attackDamage.set(attackDamage.get() * 3 / 2);
                if(offguard != OFFGUARD_NOREMOVE)
                    clearFlag(P_SITTING);
            }

            if(offguard != OFFGUARD_NOPRINT && !computingBonus)
                broadcast(enemy->getSock(), getSock(), getRoomParent(), "^r%M caught %N off guard!", enemy.get(), this);
        }
    }

    //
    // always check undead-ward
    //
    if(enemy && enemy->isUndead()) {
        if(getRoomParent()->isEffected("unhallow")) attackDamage.set(attackDamage.get() * 3 / 2);
        if(getRoomParent()->isEffected("hallow"))   attackDamage.set(attackDamage.get() * 2 / 3);
        if(isEffected("undead-ward"))               attackDamage.set(attackDamage.get() / 2);
    }

    if(dmgType == NEGATIVE_ENERGY || dmgType == MAGICAL_NEGATIVE) {
        if(isEffected("drain-shield")) attackDamage.set(attackDamage.get() / 3);
    }

    if(dmgType == MAGICAL || dmgType == MAGICAL_NEGATIVE) {
        //
        // reflect-magic works against any magic attack
        //
        if(enemy && enemy.get() != this && isEffected("reflect-magic")) {
            effect = getEffect("reflect-magic");
            // the strength of the effect represents the chance to reflect the spell
            if(effect->getStrength() >= Random::get(1,100)) {
                attackDamage.set(attackDamage.get() / 2);
                attackDamage.setReflected(attackDamage.get());
                enemy->modifyDamage(nullptr, dmgType, attackDamage, pRealm);
            }
        }

        //
        // spell saves
        //
        if(enemy) { //  && !(enemy->isMonster() && slvl < 3)
            if(chkSave(SPL, enemy, saveBonus)) {
                attackDamage.set(attackDamage.get() / 2);
                if(!computingBonus) {
                    printColor("^yYou avoided full damage!\n");
                    if(enemy.get() != this)
                        enemy->print("%M avoided full damage.\n", this);
                }
            }
        }

        //
        // if it's an elemental pRealm, check resistances
        //
        if(pRealm != NO_REALM)
            attackDamage.set(checkRealmResist(attackDamage.get(), pRealm));

        //
        // modify damage for resist-magic
        // and lich natural resist magic
        //
        attackDamage.set(doResistMagic(attackDamage.get(), enemy));
    }

    if(dmgType == PHYSICAL_DMG) {
        //
        // Spells to damage attacker on physical attacks. The original damage is not changed.
        // Don't run when computing bonus damage
        //
        if(!computingBonus && enemy && enemy.get() != this && isEffected("fire-shield")) {
            effect = getEffect("fire-shield");
            Realm reflectedRealm = NO_REALM;
            Damage reflectedDamage;

            // specific to fire-shield
            attackDamage.setPhysicalReflectedType(REFLECTED_FIRE_SHIELD);
            reflectedRealm = FIRE;
            // can be expanded to include other types

            reflectedDamage.set(effect->getStrength());

            enemy->modifyDamage(Containable::downcasted_shared_from_this<Creature>(), MAGICAL, reflectedDamage, reflectedRealm);

            // physicalReflected will hurt enemy
            attackDamage.setPhysicalReflected(reflectedDamage.get());
            // physical-shields can be reflected by reflect-magic
            attackDamage.setDoubleReflected(reflectedDamage.getReflected());
        }

        //
        // Liches are brittle to physical attacks
        //
        if(isBrittle()) {
            float brittle = 0;
            if     (level < 7)  brittle = 8;
            else if(level < 13) brittle = 5;
            else if(level < 19) brittle = 3;
            else if(level < 25) brittle = 2.5;
            else                brittle = 2;

            attackDamage.set((int)((double)attackDamage.get() + (double)attackDamage.get() / brittle));
        }

        //
        // check weapon-resistance spell
        //
        // TODO: doesn't work?
        if(isMonster() && enemy && enemy->isPlayer()) {
            if(weapon) {
                // if it is footwear, they're kicking something and that's a blunt attack
                if(weapon->getWearflag() == FEET) {
                    attackDamage.set(doWeaponResist(attackDamage.get(), "crushing"));
                } else {
                    std::string category = weapon->getWeaponCategory();
                    if(category != "none")
                        attackDamage.set(doWeaponResist(attackDamage.get(), category));
                }
            } else {
                // TODO: Dom: ???
                // we're using out fists! yarr! (or claws)
                if(enemy->isEffected("lycanthropy")) {
                    attackDamage.set(doWeaponResist(attackDamage.get(), "slashing"));
                } else {
                    // Bare-handed
                    attackDamage.set(doWeaponResist(attackDamage.get(), "crushing"));
                }
            }
        }

        // players take less damage while berserked
        if(enemy && isEffected("berserk")) {
            // zerkers: 1/5
            // everyone else: 1/7
            attackDamage.set(attackDamage.get() - (attackDamage.get() / (cClass == CreatureClass::BERSERKER ? 5 : 7)));
            attackDamage.set(std::max<int>(1, attackDamage.get()));
        }

        // monsters do more damage while berserked
        if(enemy && isEffected("berserk"))
            attackDamage.set(attackDamage.get() * 3 / 2);

        // armor damage reduction
        if(enemy) {
            const float damageReduction = enemy->getDamageReduction(Containable::downcasted_shared_from_this<Creature>());
            attackDamage.set(attackDamage.get() - (int)((float)attackDamage.get() * damageReduction));
        }

        // Werewolf silver vulnerability
        if(weapon && weapon->flagIsSet(O_SILVER_OBJECT) && isEffected("lycanthropy"))
            attackDamage.set(attackDamage.get() * 2);
    }

    // if it's a pet, check elemental pRealm resistance
    if(enemy) {
        bool resistPet=false, immunePet=false, vulnPet=false;
        checkResistPet(enemy, resistPet, immunePet, vulnPet);

        if(resistPet)
            attackDamage.set(attackDamage.get() / 2);
        if(vulnPet)
            attackDamage.add(Random::get(std::max<int>(1, attackDamage.get()/6),std::max<int>(2, attackDamage.get()/2)));
        if(immunePet)
            attackDamage.set(1);
    }
    //Check for magical armor spell effects and update them...stoneskin, armor, etc..
    //Updates damage too if necessary...
    //Weird logic is in order to stop this from being called twice due to computing 
    //bonus damage. Mob's don't pass computeBonus in their computeDamage() function
    //call, whereas players do.
    if (((player && computingBonus)) || ((enemy && enemy->isMonster())))  
        applyMagicalArmor(attackDamage,dmgType);

    attackDamage.set(std::max<int>(0, attackDamage.get()));

    // check drain last
    if(dmgType == NEGATIVE_ENERGY || dmgType == MAGICAL_NEGATIVE) {
        if(canBeDrained()) {
            if(dmgType == NEGATIVE_ENERGY)
                attackDamage.setDrain(attackDamage.get() / 2);
            else
                attackDamage.setDrain(Random::get<int>(0, attackDamage.get() / 4));

            // don't drain more than can be drained!
            attackDamage.setDrain(std::min<int>(attackDamage.getDrain(), hp.getCur()));

            // liches can't use drain damage as their HP is their MP
            // they can do more damage instead
            if(enemy && enemy->getClass() == CreatureClass::LICH) {
                attackDamage.add(attackDamage.getDrain());
                attackDamage.setDrain(0);
            }
        }
    }
}

//*********************************************************************
//                      canBeDrained
//*********************************************************************

bool Creature::canBeDrained() const {
    if(isUndead())
        return(false);
    if(isEffected("drain-shield"))
        return(false);
    if(monType::noLivingVulnerabilities(type))
        return(false);
    return(true);
}

//*********************************************************************
//                      doWeaponResist
//*********************************************************************

int Creature::doWeaponResist(int dmg, const std::string &weaponCategory) const {
    if(isEffected("resist-" + weaponCategory)) {
        dmg /= 2;
    }

    if(isEffected("immune-" + weaponCategory)) {
        dmg = 1;
    }


    if(isEffected("vuln-" + weaponCategory)) {
        dmg = dmg * 3 / 2;
    }

    return(dmg);
}


//*********************************************************************
//                      willAggro
//*********************************************************************

bool Monster::willAggro(const std::shared_ptr<Player>& player) const {

    // sometimes we should never attack a player, no matter what
    if(!player || !canSee(player) || player->isStaff())
        return(false);
    if(isEnemy(player))
        return(false);
    if(player->isUnconscious() || player->isEffected("petrification"))
        return(false);
    if(isUndead() && player->isEffected("undead-ward"))
        return(false);
    if(flagIsSet(M_UNKILLABLE))
        return(false);

    // swimming fish won't attack flying people
    // check for water, too
    //if(area_room && player->isEffected("fly") && !isEffected("fly"))
    //  return(false);

    // under these circumstances, always attack
    if(flagIsSet(M_ALWAYS_AGGRESSIVE))
        return(true);

    if( !primeFaction.empty() &&
        !flagIsSet(M_NO_FACTION_AGGRO) &&
        Faction::willAggro(player, primeFaction)
    )
        return(true);


    // sometimes we might want to be nice to people
    if(isUndead()) {
        if(player->isUndead())
            return(false);
        if(player->getClass() == CreatureClass::CLERIC && player->getDeity() == ARAMON)
            return(false);
    }
    if(player->getClan() && clan && player->getClan() == clan)
        return(false);
    if(player->isEffected("lycanthropy") && isEffected("lycanthropy"))
        return(false);
    if(type == ARACHNID && player->getClass() == CreatureClass::CLERIC && player->getDeity() == ARAMON)
        return(false);
    if(player->isEffected("vampirism") && isEffected("vampirism"))
        return(false);


    // will they aggro anyone based on some simple flags that are set?
    if(flagIsSet(M_AGGRESSIVE))
        return(true);
    if(flagIsSet(M_AGGRESSIVE_GOOD) && player->getAdjustedAlignment() > NEUTRAL)
        return(true);
    if(flagIsSet(M_AGGRESSIVE_EVIL) && player->getAdjustedAlignment() < NEUTRAL)
        return(true);

    // will they aggro anyone based on their race?
    RaceDataMap ::iterator rIt;
    RaceData* rData=nullptr;
    for(rIt = gConfig->races.begin() ; rIt != gConfig->races.end() ; rIt++) {
        rData = (*rIt).second;

        if(player->isRace(rData->getId()) && isRaceAggro(rData->getId(), true))
            return(true);
    }

    // will they aggro anyone based on their class?
    for(int i=1; i<static_cast<int>(STAFF); i++) {
        if((player->getClass() == static_cast<CreatureClass>(i) || player->getSecondClass() == static_cast<CreatureClass>(i)) && isClassAggro(i, true))
            return(true);
    }

    // will they aggro anyone based on their deity?
    if(player->getDeity()) {
        const auto deity = gConfig->getDeity(player->getDeity());
        if (deity &&isDeityAggro(deity->getId(), true)) {
            return true;
        }
    }

    return false;
}

//*********************************************************************
//                      whoToAggro
//*********************************************************************

std::shared_ptr<Player> Monster::whoToAggro() const {
    std::list<std::shared_ptr<Player>> players;
    int total=0, pick=0;
    const std::shared_ptr<const BaseRoom> myRoom = getConstRoomParent();

    if(!myRoom) {
        broadcast(::isDm, "^g *** Monster '%s' has no room in whoToAggro!", getCName());
        return (nullptr);
    }

    for(const auto& ply : myRoom->players) {
        if(auto player = ply.lock()) {
            if (canSee(player) && !player->flagIsSet(P_HIDDEN)) {
                if (willAggro(player)) {
                    total += std::max<int>(1, 300 - player->piety.getCur());
                    players.push_back(player);
                }
            }
        }
    }
    if(players.empty())
        return(nullptr);
    if(players.size() == 1)
        return(players.front());

    // we now have a list of players we want to aggro; use some criteria to
    // decide which of them we hate the most

    // this logic is currently copied from old mordor lowest peity algorithm:
    // higher piety = lower chance of being picked
    pick = Random::get<int>(1, total);
    total = 0;

    for(const auto& player : players) {
        total += std::max<int>(1, 300 - player->piety.getCur());
        if(total >= pick)
            return(player);
    }
    return(nullptr);
}
