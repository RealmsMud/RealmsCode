/*
 * monsters.cpp
 *   Monster routines.
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

#include <cmath>                       // for pow
#include <cstring>                     // for strcpy, strcmp
#include <ctime>                       // for time
#include <set>                         // for operator==, _Rb_tree_const_ite...
#include <string>                      // for allocator, string, operator==

#include "catRef.hpp"                  // for CatRef
#include "cmd.hpp"                     // for cmd
#include "factions.hpp"                // for Faction
#include "flags.hpp"                   // for M_FAST_WANDER, M_PERMENANT_MON...
#include "global.hpp"                  // for CreatureClass, CreatureClass::...
#include "lasttime.hpp"                // for lasttime
#include "location.hpp"                // for Location
#include "magic.hpp"                   // for get_spell_function, splOffensive
#include "monType.hpp"                 // for isIntelligent
#include "money.hpp"                   // for Money, GOLD
#include "move.hpp"                    // for getString
#include "mud.hpp"                     // for ospell, PET_CAST_DELAY, LT
#include "mudObjects/container.hpp"    // for ObjectSet, PlayerSet, MonsterSet
#include "mudObjects/creatures.hpp"    // for Creature, CHECK_DIE, NUM_ASSIS...
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/objects.hpp"      // for Object, ObjectType, ObjectType...
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "proto.hpp"                   // for broadcast, bonus, get_spell_lvl
#include "random.hpp"                  // for Random
#include "realm.hpp"                   // for MAX_REALM, NO_REALM
#include "server.hpp"                  // for Server, GOLD_OUT, gServer
#include "socket.hpp"                  // for Socket
#include "stats.hpp"                   // for Stat
#include "structs.hpp"                 // for osp_t
#include "unique.hpp"                  // for Unique
#include "wanderInfo.hpp"              // for WanderInfo
#include "xml.hpp"                     // for loadRoom


char Monster::mob_trade_str[][16]  = { "None", "Smithy", "Banker", "Armorer", "Weaponsmith", "Merchant", "Training Perm" };

// TODO switch this strcmp to compare
bool Monster::operator <(const Monster& t) const {
    return(strcmp(this->getCName(), t.getCName()) < 0);
}
/*
 * mTypes formatted as case for use in functions below
 *
    case HUMANOID:
    case GOBLINOID:
    case MONSTROUSHUM:
    case GIANTKIN:
    case ANIMAL:
    case DIREANIMAL:
    case INSECT:
    case INSECTOID:
    case ARACHNID:
    case REPTILE:
    case DINOSAUR:
    case AUTOMATON:
    case AVIAN:
    case FISH:
    case PLANT:
    case DEMON:
    case DEVIL:
    case DRAGON:
    case BEAST:
    case MAGICALBEAST:
    case GOLEM:
    case ETHEREAL:
    case ASTRAL:
    case GASEOUS:
    case ENERGY:
    case FAERIE:
    case DEVA:
    case ELEMENTAL:
    case PUDDING:
    case SLIME:
    case UNDEAD:
 *
 */

// This should be called whenever a monster is put into the world or into a room
// If we have an ID of -1, grab the next ID from the server and assign it to this
// monster

void Monster::validateId() {
//  std::clog << "Validating ID for <" << getName() << ">" << std::endl;
    if(id.empty() || id == "-1") {
        setId(gServer->getNextMonsterId());
    }
}

//*********************************************************************
//                      pulseTick
//*********************************************************************

bool hearMobTick(std::shared_ptr<Socket> sock) {
    if(!sock->getPlayer() || !isCt(sock))
        return(false);
    return(!sock->getPlayer()->flagIsSet(P_NO_TICK_MSG));
}

void Monster::pulseTick(long t) {
    bool ill = false;
    //bool noTick = false;
    long mainTick = LT(this, LT_TICK);
    long secTick = LT(this, LT_TICK_SECONDARY);

    int hpTickAmt = 0;
    int mpTickAmt = 0;

    ill = isEffected("petrification") || (isPoisoned() && !immuneToPoison());
//  noTick = ill;

    //std::shared_ptr<BaseRoom> room = getRoom();
    // ****** Main Tick ******
    if(mainTick < t && !ill) {
        double power = 0.8;
        int hpTickTime = 60 - 5*bonus(constitution.getCur());

        if(flagIsSet(M_FAST_TICK) && !nearEnemy()) {
            hpTickTime = std::max(1,(15 - (2*bonus(constitution.getCur()))));
            power = 1.0;
        } else if(flagIsSet(M_REGENERATES) && !nearEnemy()) {
            hpTickTime /= 2;
            power = 0.9;
        } else {
            power = 0.8;
        }
        hpTickAmt = static_cast<int>( pow( static_cast<double>(hp.getMax())*0.2, power ) );
        hpTickAmt = hp.increase(hpTickAmt);

        lasttime[LT_TICK].ltime = t;
        lasttime[LT_TICK].interval = hpTickTime;

    }
    // ****** End Main Tick ******

    // ****** Secondary Tick ******
    if(secTick < t && !ill) {
        double power = 0.8;
        int mpTickTime = 60 - 5*bonus(piety.getCur());

        if(flagIsSet(M_FAST_TICK) && !nearEnemy()) {
            mpTickTime = std::max(1,(5 - (2*bonus(piety.getCur()))));
            power = 1.0;
        } else if(flagIsSet(M_REGENERATES) && !nearEnemy()) {
            mpTickTime = mpTickTime * 2 / 3;
            power = 0.9;
        } else {
            mpTickTime -= 5;
            power = 0.8;
        }

        mpTickAmt = static_cast<int>( pow( static_cast<double>(hp.getMax())*0.2, power ) );

        mpTickAmt = mp.increase(mpTickAmt);
        lasttime[LT_TICK_SECONDARY].ltime = t;
        lasttime[LT_TICK_SECONDARY].interval = mpTickTime;
    }
    // ****** End Secondary Tick ******


    if(flagIsSet(M_PERMENANT_MONSTER) && (hpTickAmt || mpTickAmt)) {
        broadcast(hearMobTick, "^y*** %M(L%d,R%s) just ticked. [%d/%dH](+%d) [%d/%dM](+%d)",
            this, level, currentLocation.room.displayStr().c_str(),
            std::min(hp.getCur(),hp.getMax()), hp.getMax(), hpTickAmt,
            std::min(mp.getCur(),mp.getMax()), mp.getMax(), mpTickAmt);
    }

    // reset hide when mob ticks to full and no enemies are in the room
    if(!nearEnemy(Containable::downcasted_shared_from_this<Creature>()) && hp.getCur() >= hp.getMax()/4) {
        if(flagIsSet(M_WAS_HIDDEN)) // Reset mob-hide
        {
            broadcast((std::shared_ptr<Socket> )nullptr, getRoomParent(), "%M hides in shadows.", this);
            setFlag(M_HIDDEN);
            clearFlag(M_WAS_HIDDEN);
        }
    }
    // Mobs shouldn't stay berserk after they tick full
    if(!nearEnemy(Containable::downcasted_shared_from_this<Creature>()) && hp.getCur() == hp.getMax()) {
        if(isEffected("berserk")) {
            broadcast((std::shared_ptr<Socket> )nullptr, getRoomParent(), "^r%M's rage diminishes!", this);
            removeEffect("berserk");
            setFlag(M_WILL_BERSERK);
        }
    }
}



//*********************************************************************
//                      beneficialCaster
//*********************************************************************
// The following function scans the room and casts various beneficial spells on
// any particular players which need them. It is to be used for monsters in
// clinics and such to make them more interactive. The chance to cast is random.

void Monster::beneficialCaster() {
    int     chance=0, heal=0;


    if(mp.getCur() < 2)
        return;

    for(const auto& pIt: getRoomParent()->players) {
        auto ply = pIt.lock();
        if(!ply) continue;

        if( ply->flagIsSet(P_DM_INVIS) ||
            ply->flagIsSet(P_HIDDEN) ||
            ply->isEffected("petrification") ||
            !canSee(ply)
        )
            continue;

        // mobs won'twont cast on people in combat
        if(ply->inCombat())
            continue;
        if(!Faction::willBeneCast(ply, primeFaction))
            continue;

        if(ply->flagIsSet(P_DOCTOR_KILLER)) {
            if(Random::get(1,100)<=2) {
                broadcast(ply->getSock(), ply->getRoomParent(), "%M spits, \"Doctor killer!\" at %N.", this, ply.get());
                ply->print("%M spits, \"Doctor killer!\" at you.\n", this);
            }
            continue;
        }

        if(spellIsKnown(S_SLOW_POISON)) {

            chance = Random::get(1,100);
            if((mp.getCur() >= 8) && (chance < 10) && !ply->isEffected("slow-poison") && ply->isPoisoned()) {
                if(ply->immuneToPoison())
                    continue;

                if(ply->isEffected("slow-poison"))
                    continue;


                ply->print("%M casts a slow-poison spell on you.\n", this);
                broadcast(ply->getSock(), ply->getSock(), getRoomParent(),
                    "%M casts a slow-poison spell on %N.", this, ply.get());
                mp.decrease(8);

                ply->addPermEffect("slow-poison");
                continue;
            }
        }

        if(spellIsKnown(S_CURE_DISEASE)) {

            chance = Random::get(1,100);
            if((mp.getCur() >= 12) && (chance < 10) && (ply->isDiseased())) {
                if(ply->immuneToDisease())
                    continue;

                ply->print("%M casts a cure-disease spell on you.\n", this);

                broadcast(ply->getSock(), ply->getSock(), getRoomParent(), "%M casts a cure-disease spell on %N.", this, ply.get());
                mp.decrease(12);

                ply->cureDisease();
                continue;
            }
        }

        if(ply->getClass() == CreatureClass::LICH)
            continue;
        if(ply->flagIsSet(P_POISONED_BY_PLAYER) && Random::get(1,100) >= 3)
            continue;

        if(spellIsKnown(S_VIGOR)) {
            chance = Random::get(1,100);
            if((mp.getCur() >= 2) && (chance <= 5) && (ply->hp.getCur() < ply->hp.getMax())) {

                ply->print("%M casts a vigor spell on you.\n", this);

                broadcast(ply->getSock(), ply->getSock(), getRoomParent(), "%M casts a vigor spell on %N.", this, ply.get());
                mp.decrease(2);
                heal = Random::get(1,6) + bonus((int) piety.getCur());
                heal = std::max(1, heal);

                if(cClass == CreatureClass::CLERIC && clan != 12) {
                    heal *= 2;
                    heal += (level / 2);
                }
                if( (cClass == CreatureClass::CLERIC && clan == 11) ||
                    (cClass == CreatureClass::PALADIN)
                )
                    heal += Random::get(1,4);
                ply->hp.increase(heal);
                heal = 0;
                continue;
            }
        }

        if(spellIsKnown(S_MEND_WOUNDS)) {

            chance = Random::get(1,100);
            if((mp.getCur() >= 4) && (chance <= 5) && (ply->hp.getCur() < (ply->hp.getMax()/2))) {

                ply->print("%M casts a mend-wounds spell on you.\n", this);

                broadcast(ply->getSock(), ply->getSock(), getRoomParent(), "%M casts a mend-wounds spell on %N.", this, ply.get());
                mp.decrease(4);
                heal = Random::get(6,9) + bonus((int) piety.getCur());
                heal = std::max(1, heal);
                if(cClass == CreatureClass::CLERIC && clan != 12) {
                    heal *= 2;
                    heal += (level / 2);
                }
                if( (cClass == CreatureClass::CLERIC && clan == 11) ||
                    (cClass == CreatureClass::PALADIN)
                )
                    heal += Random::get(1,4);
                ply->hp.increase(heal);
                heal = 0;
                continue;
            }
        }

        if(spellIsKnown(S_HEAL)) {

            chance = Random::get(1,100);
            if((mp.getCur() >= 25) && (chance < 3) && (cClass == CreatureClass::CLERIC && clan != 12) &&
                    (ply->hp.getCur() < (ply->hp.getMax()/4)) && (ply->getLevel() >= 5)) {

                ply->print("%M casts a heal spell on you.\n", this);

                broadcast(ply->getSock(), ply->getSock(), getRoomParent(), "%M casts a heal spell on %N.", this, ply.get());
                mp.decrease(25);

                ply->hp.restore();
                continue;
            }
        }
    }
}


//*********************************************************************
//                      mobDeathScream
//*********************************************************************

int Monster::mobDeathScream() {
    long    i=0, t=0, n=0;
    int     death=0;
    std::shared_ptr<Creature> target=nullptr;

    i = lasttime[LT_BERSERK].ltime;
    t = time(nullptr);

    if(t - i < 30L) // Mob has to wait 30 seconds.
        return(0);


    lasttime[LT_DEATH_SCREAM].ltime = t;
    lasttime[LT_DEATH_SCREAM].interval = 30L;

    if(hp.getCur() <= (hp.getMax()/5)) {
        clearFlag(M_DEATH_SCREAM);          // Mobs will not scream at lower then 20% hp.
        return(0);
    }


    broadcast((std::shared_ptr<Socket> )nullptr, getRoomParent(), "%M bellows out a deadly ear-splitting scream!!", this);

    auto room = getRoomParent();
    auto pIt = room->players.begin();
    auto pEnd = room->players.end();
    while(pIt != pEnd) {
        target = (*pIt++).lock();
        if(!target) continue;
        n = Random::get(1,15) + level; // Damage done.

        if(target->isEffected("resist-magic"))
            n /= 2;

        target->printColor("^yHarmful vibrations double you over in pain for ^Y%d^y damage!\n", n);

        if(n > (level+11))
            target->stun(Random::get(4, 6));


        if(target->isEffected("berserk")) {
            n = n - (n / 5);
            n = std::max(1L, n);
        }

        if(isUndead() && target->isEffected("undead-ward"))
            n /= 2;

        if(target->flagIsSet(P_DM_INVIS))
            n=0;

        if(doDamage(target, n, CHECK_DIE)) {
            death = 1;
            continue;
        }

    }

    return death != 0;
}


//*********************************************************************
//                      possibleEnemy
//*********************************************************************
// This function looks for possible enemies in a room. It is specifically
// designed for fast wandering, hunting mobs....like inquisitors. -- TC

bool Monster::possibleEnemy() {
    bool    possible=false, nodetect=false;

    if( !flagIsSet(M_STEAL_ALWAYS) &&
        !flagIsSet(M_AGGRESSIVE) &&
        !flagIsSet(M_AGGRESSIVE_GOOD) &&
        !flagIsSet(M_AGGRESSIVE_EVIL)
    )
        return(false);

    for(const auto& pIt: getRoomParent()->players) {
        auto ply = pIt.lock();
        if(!ply) continue;
        if(flagIsSet(M_AGGRESSIVE))
            possible = true;
        if(flagIsSet(M_STEAL_ALWAYS))
            possible = true;
        if(flagIsSet(M_AGGRESSIVE_GOOD) && ply->getAdjustedAlignment() > NEUTRAL)
            possible = true;
        if(flagIsSet(M_AGGRESSIVE_EVIL) && ply->getAdjustedAlignment() < NEUTRAL)
            possible = true;

        if(ply->flagIsSet(P_HIDDEN))
            nodetect = true;
        if(!canSee(ply))
            nodetect = true;

        if(nodetect)
            possible = false;

        if(possible)
            return(true);
    }

    return(possible);
}

//*********************************************************************
//                      willCastHolds
//*********************************************************************
// This functions determine whether a monster that wants to cast a hold spell
// will cast it or not
bool Monster::willCastHolds(const std::shared_ptr<Creature>& target, int splNo) {

    if (!target)
        return(false);

    //Mobs will only cast hold spells on ptesters and staff for now
    if (target->isPlayer() && !target->flagIsSet(P_PTESTER) && !target->isStaff())
        return(false);

    if(target->isMagicallyHeld(false))
        return(false);

    if (target->isEffected("free-action")) {
        if (intelligence.getCur() >= 160 && Random::get(1,100) > 20)
            return(false);
    }

    if(intelligence.getCur() >= 200 && target->checkResistEnchantments(this->getAsCreature(), get_spell_name(splNo), false))
        return(false);

    // Players are all humanoids, so these spells are pointless
    if(target->isPlayer() && 
        (splNo == S_HOLD_ANIMAL || splNo == S_HOLD_PLANT ||
            splNo == S_HOLD_ELEMENTAL || splNo == S_HOLD_FEY))
        return(false);

    if(splNo != S_HOLD_UNDEAD && target->isUndead())
        return(false);

    switch (splNo) {
    case S_HOLD_PERSON:
        if(target->isHumanoidLike()) 
            return(true);
        break;
    case S_HOLD_MONSTER:
        if(target->isMonster() && monType::isFey(target->getType()))
            return(false);
        else
            return(true);
        break;
    case S_HOLD_UNDEAD:
        if(target->isUndead())
            return(true);
        break;
    case S_HOLD_ANIMAL:
        if(monType::isAnimal(target->getType()))
            return(true);
        break;
    case S_HOLD_PLANT:
        if(monType::isPlant(target->getType()))
            return(true);
        break;
    case S_HOLD_ELEMENTAL:
        if(monType::isElemental(target->getType()))
            return(true);
        break;
    case S_HOLD_FEY:
        if(monType::isFey(target->getType()))
            return(true);
        break;
    default:
        break;
    }

    return(false);

}

//*********************************************************************
//                      castSpell
//*********************************************************************
// This function allows monsters to cast spells at players.

extern int spllist_size;
int Monster::castSpell(const std::shared_ptr<Creature>&target) {
    std::shared_ptr<Creature>temp_ptr=nullptr;
    cmd     cmnd;
    int     i=0, spl=0, c=0;
    int     known[20], knowctr=0;
    int     (*fn)(SpellFn);
    std::string enemy;
    int     realm=0, n=0;

    zero(known, sizeof(known));
    for(i=0; i<MAXSPELL; i++) {
        if(knowctr > 19)
            break;
        if(spellIsKnown(i) && i != S_ILLUSION)
            known[knowctr++] = i;
    }

    if(!knowctr)
        return(0);

    spl = known[Random::get<int>(0, knowctr-1)];

    // pets are smarter
    if(isPet()) {
        // Pets only cast offensive spells in combat
        if((int(*)(SpellFn, const char*, osp_t*)) get_spell_function(spl) != splOffensive) {
            realm = proficiency[1];
            if (realm <= NO_REALM || realm >= MAX_REALM) {
                // Non casting pet class if realm is zero
                return (0);
            }
            if(realm == CONJUREBARD || realm == CONJUREMAGE || realm == CONJUREANIM) {
                realm = getRandomRealm();
            }
            spl = ospell[(realm-1)].splno;
        }

        if( (int(*)(SpellFn, const char*, osp_t*)) get_spell_function(spl) == splOffensive &&
            get_spell_lvl(spl) < 5) {
            // They will cast higher level offensive spells if they have the mana
            for(i=1;i <knowctr; i++) {
                if( get_spell_lvl(known[i-1]) > get_spell_lvl(spl) &&
                    (int(*)(SpellFn, const char*, osp_t*))get_spell_function(known[i-1]) == splOffensive
                ) {
                    for(c=0; ospell[c].splno != get_spell_num(known[i-1]); c++)
                        if(ospell[c].splno == -1) {
                            return(13);
                        }
                    if(mp.getCur() >= ospell[c].mp)
                        spl = known[i-1];
                }
            }
        }
    }

    int splNo = get_spell_num(spl);
    if( (int(*)(SpellFn, const char*, osp_t*))get_spell_function(spl) == splOffensive ||
        splNo == S_TELEPORT ||
        splNo == S_STUN ||
        splNo == S_WORD_OF_RECALL ||
        splNo == S_DRAIN_EXP ||
        splNo == S_BLINDNESS ||
        splNo == S_SILENCE ||
        splNo == S_FEAR ||
        splNo == S_DISPEL_MAGIC ||
        splNo == S_CANCEL_MAGIC ||
        splNo == S_ANNUL_MAGIC ||
        splNo == S_SCARE ||
        splNo == S_ENTANGLE ||
        splNo == S_ETHREAL_TRAVEL ||
        splNo == S_DISINTEGRATE ||
        splNo == S_MAGIC_MISSILE ||
        splNo == S_SLOW ||
        splNo == S_WEAKNESS ||
        splNo == S_DAMNATION ||
        splNo == S_FEEBLEMIND ||
        splNo == S_ENFEEBLEMENT ||
        (splNo == S_HEAL && target->getClass() == CreatureClass::LICH) ||
        (splNo == S_DISPEL_EVIL && target->getAdjustedAlignment() < NEUTRAL) ||
        (splNo == S_DISPEL_GOOD && target->getAdjustedAlignment() > NEUTRAL) ||
        splNo == S_JUDGEMENT ||
        splNo == S_DEAFNESS ||
        splNo == S_SAP_LIFE ||
        splNo == S_LIFETAP ||
        splNo == S_LIFEDRAW ||
        splNo == S_DRAW_SPIRIT ||
        splNo == S_SIPHON_LIFE ||
        splNo == S_SPIRIT_STRIKE ||
        splNo == S_SOULSTEAL ||
        splNo == S_TOUCH_OF_KESH ||
        (splNo == S_HOLD_PERSON && willCastHolds(target, splNo)) ||
        (splNo == S_HOLD_MONSTER && willCastHolds(target, splNo)) ||
        (splNo == S_HOLD_UNDEAD && willCastHolds(target, splNo)) ||
        (splNo == S_HOLD_ANIMAL && willCastHolds(target, splNo)) ||
        (splNo == S_HOLD_PLANT && willCastHolds(target, splNo)) ||
        (splNo == S_HOLD_ELEMENTAL && willCastHolds(target, splNo)) ||
        (splNo == S_HOLD_FEY && willCastHolds(target, splNo))
    ) {
        // we have the exact pointer
        enemy = target->getName();
        n = 1;
        do {
            temp_ptr = getRoomParent()->findCreature(Containable::downcasted_shared_from_this<Monster>(), enemy, n, false);
            if(!temp_ptr) {
                // something is seriously wrong here
                return(12);
            }

            // move to the next
            n++;

            // keep going till we find the exact match
        } while( target != temp_ptr );

        // we were one too many here
        n--;

        // at this point we know which number is the exact match
        strcpy(cmnd.str[2], target->getCName());
        cmnd.val[2] = n;
        cmnd.num = 3;
    } else {
        // cast on self
        cmnd.num = 2;
    }
    fn = get_spell_function(spl);

    target->printColor("^r");

    SpellData data;
    data.set(CastType::CAST, get_spell_school(spl), get_spell_domain(spl), nullptr, Containable::downcasted_shared_from_this<Monster>());
    data.splno = spl;

    if((int(*)(SpellFn, const char*, osp_t*))fn == splOffensive) {
        for(c=0; ospell[c].splno != get_spell_num(spl); c++)
            if(ospell[c].splno == -1)
                return(13);
        i = ((int(*)(SpellFn, const char*, osp_t*))*fn)(Containable::downcasted_shared_from_this<Monster>(), &cmnd, &data, get_spell_name(spl), &ospell[c]);
    } else {
        i = ((int(*)(SpellFn))*fn)(Containable::downcasted_shared_from_this<Monster>(), &cmnd, &data);
    }

    castDelay(PET_CAST_DELAY);
    return(i);
}

//*********************************************************************
//                      petCaster
//*********************************************************************

bool Monster::petCaster() {
    std::shared_ptr<Player>master = getPlayerMaster();
    int     heal=0;
    long    i=0, t = time(nullptr);

    if(!isPet() || !master || !inSameRoom(master))
        return(false);

    i = LT(this, LT_SPELL);
    if(t < i)
        return(false);

    if(mp.getCur() < 2 || Random::get(1,100) > 33 || getRoomParent()->flagIsSet(R_NO_MAGIC))
        return(false);

    if( spellIsKnown(S_HEAL) &&
        master->hp.getCur() <= (master->hp.getMax()/8) &&
        mp.getCur() >= 25 &&
        Random::get(1,100) > 50
    ) {
        mp.decrease(25);

        master->print("%M casts a heal spell on you.\n", this);
        broadcast(master->getSock(), getRoomParent(),
            "%M casts a heal spell on %N.", this, master.get());
        master->hp.restore();
        castDelay(PET_CAST_DELAY);
        return(true);
    }

    if( spellIsKnown(S_MEND_WOUNDS) &&
        master->hp.getCur() <= (master->hp.getMax()/2) &&
        mp.getCur() >= 4 &&
        Random::get(1,100) > 25
    ) {
        master->print("%M casts a mend-wounds spell on you.\n", this);
        broadcast((std::shared_ptr<Socket> )nullptr, master->getSock(), getRoomParent(), "%M casts a mend-wounds spell on %N.", this, master.get());

        mp.decrease(4);

        heal = std::max(bonus((int) intelligence.getCur()), bonus((int) piety.getCur())) +
               ((cClass == CreatureClass::CLERIC) ?
                level + Random::get(1, 1 + level / 2) : 0) +
               (cClass == CreatureClass::PALADIN ?
                level / 2 + Random::get(1, 1 + level / 3) : 0) +
               ((isEffected("vampirism") || cClass == CreatureClass::BARD || cClass == CreatureClass::DEATHKNIGHT) ?
                Random::get(1, 1 + level / 5) + 2 : 0) +
               ((isEffected("lycanthropy") || cClass == CreatureClass::MONK) ? level / 5 +
                Random::get(1, 1 + level / 7) + 2 : 0) +
               dice(2, 7, 0);

                this->doHeal(master, heal);
        castDelay(PET_CAST_DELAY);
        return(true);
    }

    if( spellIsKnown(S_VIGOR) &&
        master->hp.getCur() < master->hp.getMax() &&
        mp.getCur() >= 2
    ) {
        master->print("%M casts a vigor spell on you.\n", this);
        broadcast((std::shared_ptr<Socket> )nullptr, master->getSock(), getRoomParent(), "%M casts a vigor spell on %N.", this, master.get());
        mp.decrease(2);

        heal = std::max(bonus((int) intelligence.getCur()),bonus((int) piety.getCur())) +
               ((cClass == CreatureClass::CLERIC) ? level / 2 +
                Random::get(1, 1 + level / 2) : 0) +
               (cClass == CreatureClass::PALADIN ?
                level / 3 + Random::get(1, 1 + level / 4) : 0) +
               ((isEffected("vampirism") || cClass == CreatureClass::BARD || cClass == CreatureClass::DEATHKNIGHT) ?
                Random::get(1, 1 + level / 5) : 0) +
               ((isEffected("lycanthropy") || cClass == CreatureClass::MONK) ? level/5 +
                Random::get(1,1+level/7) : 0)
               + Random::get(1, 8);

                this->doHeal(master, heal);
        castDelay(PET_CAST_DELAY);
        return(true);
    }

    // After all possible spells have been checked on the players...check themselves
    if( spellIsKnown(S_MEND_WOUNDS) &&
        hp.getCur() <= (hp.getMax()/2) &&
        mp.getCur() >= 4 &&
        Random::get(1,100) > 25
    ) {
        broadcast((std::shared_ptr<Socket> )nullptr, getRoomParent(), "%M casts a mend-wounds spell on %sself.", this, himHer());
        mp.decrease(4);

        heal = std::max(bonus((int) intelligence.getCur()), bonus((int) piety.getCur())) +
               ((cClass == CreatureClass::CLERIC) ?
                level + Random::get(1, 1 + level / 2) : 0) +
               (cClass == CreatureClass::PALADIN ?
                level / 2 + Random::get(1, 1 + level / 3) : 0) +
               ((isEffected("vampirism") || cClass == CreatureClass::BARD || cClass == CreatureClass::DEATHKNIGHT) ?
                Random::get(1, 1 + level / 5) + 2 : 0) +
               ((isEffected("lycanthropy") || cClass == CreatureClass::MONK) ? level / 5 +
                                                                               Random::get(1, 1 + level / 7) + 2 : 0) +
               dice(2, 7, 0);

        doHeal(Containable::downcasted_shared_from_this<Monster>(), heal);
        castDelay(PET_CAST_DELAY);
        return(true);
    }

    if( spellIsKnown(S_VIGOR) &&
        hp.getCur() < hp.getMax() &&
        mp.getCur() >= 2
    ) {

        broadcast((std::shared_ptr<Socket> )nullptr, getRoomParent(), "%M casts a vigor spell on %sself.", this, himHer());
        mp.decrease(2);

        heal = std::max(bonus((int) intelligence.getCur()),bonus((int) piety.getCur())) +
               ((cClass == CreatureClass::CLERIC) ? level / 2 +
                Random::get(1, 1 + level / 2) : 0) +
               (cClass == CreatureClass::PALADIN ?
                level / 3 + Random::get(1, 1 + level / 4) : 0) +
               ((isEffected("vampirism") || cClass == CreatureClass::BARD || cClass == CreatureClass::DEATHKNIGHT) ?
                Random::get(1, 1 + level / 5) : 0) +
               ((isEffected("lycanthropy") || cClass == CreatureClass::MONK) ? level/5 +
                Random::get(1,1+level/7) : 0)
               + Random::get(1, 8);

        doHeal(Containable::downcasted_shared_from_this<Monster>(), heal);
        castDelay(PET_CAST_DELAY);
        return(true);
    }

    return(false);
}


//*********************************************************************
//                      checkAssist
//*********************************************************************
// See if this monster will assist another monster,
// or if they will be assisted by someone else

bool hearMobAggro(std::shared_ptr<Socket> sock) {
    if(!sock->getPlayer() || !isCt(sock))
        return(false);
    return(!sock->getPlayer()->flagIsSet(P_NO_AGGRO_MSG));
}

bool Monster::checkAssist() {
    int i=0, assist=0;

    std::shared_ptr<Creature>crt=nullptr;

    for(i=0; i<NUM_ASSIST_MOB; i++) {
        if(assist_mob[i].id) {
            assist = 1;
            break;
        }
    }

    if(assist) {
        for(const auto& mons : getRoomParent()->monsters) {
            if(mons.get() == this)
                continue;

            if(mons->hasEnemy() && willAssist(mons)) {
                crt = mons->getTarget(false);

                crt = enm_in_group(crt);

                if(crt && inSameRoom(crt) && !isEnemy(crt)) {
                    // if assisting, don't let them swing right away
                    updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
                    addEnemy(crt, true);

                    broadcast(hearMobAggro, "^y*** %s(R:%s) added %s to %s attack list (same room).",
                        getCName(), getRoomParent()->fullName().c_str(), crt->getCName(), hisHer());

                    setFlag(M_ALWAYS_ACTIVE);
                    crt = nullptr;
                }
            }
        }
    }


    if(flagIsSet(M_WILL_BE_ASSISTED) || !primeFaction.empty()) {
        std::shared_ptr<Creature> target=nullptr;
        if(hasEnemy()) {
            target = getTarget();
            if(target)
                target = target->getAsPlayer();
            if(target) {
                for(const auto& mons : target->getRoomParent()->monsters) {
                    if(mons.get() == this)
                        continue;
                    if(mons->hasEnemy() || !mons->canSee(target))
                        continue;

                    if( (mons->flagIsSet(M_WILL_ASSIST) && flagIsSet(M_WILL_BE_ASSISTED)) ||
                        (mons->flagIsSet(M_FACTION_ASSIST) && primeFaction == mons->getPrimeFaction()))
                    {
                        target->printColor("^r%M quickly assists %N!\n", mons.get(), this);

                        mons->setFlag(M_ALWAYS_ACTIVE);

                        mons->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
                        mons->getAsMonster()->addEnemy(enm_in_group(target), true);
                    }
                }
            }
        }
    }
    return(true);
}

//*********************************************************************
//                      willAssist
//*********************************************************************

bool Monster::willAssist(const std::shared_ptr<Monster> victim) const {

    if(!victim->info.id)
        return(false);

    for(const auto & i : assist_mob) {
        if(i == victim->info)
            return(true);
    }

    return(false);
}

//*********************************************************************
//                      checkScavange
//*********************************************************************

void Monster::checkScavange(long t) {
    std::shared_ptr<BaseRoom> room = getRoomParent();
    std::shared_ptr<Object>  object=nullptr;
    long i=0;

    if(room->flagIsSet(R_SHOP_STORAGE))
        return;

    if(isMagicallyHeld())
        return;

    if(flagIsSet(M_SCAVANGER)) {
        i = lasttime[LT_MON_SCAVANGE].ltime;
        if( t - i > 20 &&
            Random::get<bool>(0.15) &&
            !room->objects.empty() &&
            canScavange((*room->objects.begin())) &&
            !((*room->objects.begin())->getType() == ObjectType::WEAPON && flagIsSet(M_WILL_WIELD)))
        {
            object = (*room->objects.begin());
            object->deleteFromRoom();

            setFlag(M_HAS_SCAVANGED);
            broadcast((std::shared_ptr<Socket> )nullptr, room, "%M picked up %1P.", this, object.get());

            // Object is gold
            if(!object->info.id) {
                coins.add(object->value);
                object.reset();
            } else
                addObj(object);
        }
        if(t - i > 20)
            lasttime[LT_MON_SCAVANGE].ltime = t;
    }

    // thief code
    if(flagIsSet(M_TAKE_LOOT) || flagIsSet(M_STREET_SWEEPER)) {
        i = lasttime[LT_MOB_THIEF].ltime;
        if(t - i > 5) {
            std::weak_ptr<Object> hide_obj;
            std::ostringstream oStr;
            lasttime[LT_MOB_THIEF].ltime = t;
            ObjectSet::iterator it;
            for(it = room->objects.begin() ; it != room->objects.end() ; ) {
                object = (*it++);
                if((flagIsSet(M_STREET_SWEEPER) ||
                    (object->getType() == ObjectType::MONEY || object->value[GOLD] >= 100)) &&
                        canScavange(object) )
                {
                    if(getWeight() + object->getActualWeight() > maxWeight()) {
                        if(hide_obj.expired() && !flagIsSet(M_STREET_SWEEPER)) {
                            hide_obj = object;
                        }
                        continue;
                    }
                    oStr << object->getName() << "^x, ";
                    object->deleteFromRoom();
                    if(object->getType() == ObjectType::MONEY || !object->info.id) {
                        coins.add(object->value);
                        object.reset();
                    } else {
                        addObj(object);
                    }
                }
            }
            auto str = oStr.str();
            if(!str.empty()) {
                str = str.substr(0, str.length() - 2);
                broadcast((std::shared_ptr<Socket> )nullptr, room, "%M picked up %s.", this, str.c_str());
            }
            if(auto obj = hide_obj.lock()) {
                broadcast(getSock(), room, "%M attempts to hide %1P.", this, obj.get());
                if(Random::get(1, 100) <= level * 10) {
                    obj->setFlag(O_HIDDEN);
                }
            }
        }
    }

}

//*********************************************************************
//                      canScavange
//*********************************************************************

bool Monster::canScavange(const std::shared_ptr<Object>&  object) {
    return( !object->flagIsSet(O_NO_TAKE) &&
            !object->flagIsSet(O_SCENERY) &&
            !object->flagIsSet(O_HIDDEN) &&
            !object->flagIsSet(O_PERM_INV_ITEM) &&
            !object->flagIsSet(O_PERM_ITEM) &&
            !Unique::is(object)
    );
}

// TODO: Put this in the config/server class
static int Mobilechance = 30;

//*********************************************************************
//                      checkWander
//*********************************************************************
// See if the monster will go mobile or wander away
// Return Value: 1 - Move to the next monster
//               2 - Free this monster and start again at start of active list

int Monster::checkWander(long t) {
    int n=0, wanderchance=0;
    std::shared_ptr<BaseRoom> room = getRoomParent();
    long i = lasttime[LT_MON_WANDER].ltime;

            // If we're a mobile monster
    if( flagIsSet(M_MOBILE_MONSTER) &&
        // if fast wander, mobile even if perm.
        (!flagIsSet(M_PERMENANT_MONSTER) || flagIsSet(M_FAST_WANDER)) &&
        // can fast-wander if enemies are not nearby
        (flagIsSet(M_FAST_WANDER) ? !nearEnemy() :
            // if not fast-wander, can mobile if no enemies
            !hasEnemy()) &&
        // can't wander if following someone
        !flagIsSet(M_DM_FOLLOW) && !getMaster() &&
        // or if they're set no wander
        !flagIsSet(M_NO_WANDER) && !isMagicallyHeld())
    {
        // Fast wander can wander once a second
        if((flagIsSet(M_FAST_WANDER) && t - i > 1) ||
            // Non fast wander has a 20% chance after 20 seconds
            (t - i > 20 && Random::get(1, 100) > 20))
        {
            // If we're a fast wanderer, see if there's anyone in the room we
            // might want to stick around and attack. If there isn't we can
            // wander, otherwise don't move.
            if(!flagIsSet(M_FAST_WANDER) || (flagIsSet(M_FAST_WANDER) && !possibleEnemy()))
                n = mobileCrt();
            else
                n = 0;

            if(!n)
                clearFlag(M_MOBILE_MONSTER);

            if(n)
                return(1);
        }
        return(0);
    }


    // If we're chasing a shoplifter, we don't have an enemy in the room,
    // and sufficient time has passed
    if( flagIsSet(M_ATTACKING_SHOPLIFTER) &&
        !nearEnemy() &&
        (t - i > 60 && Random::get(1, 100) <= (!flagIsSet(M_PERMENANT_MONSTER) ? 40:30))
    ) {
        // If we have a mobile chance or we're a fast wanderer
        if((Random::get(1, 100) < Mobilechance || flagIsSet(M_FAST_WANDER))) {
            // We can go looking for the guy
            setFlag(M_MOBILE_MONSTER);
        } else if(!flagIsSet(M_CHASING_SOMEONE)) {
            // If we're nto chasing someone
            // Then we might start chasing them or let our guard down

            if(!flagIsSet(M_PERMENANT_MONSTER)) {
                broadcast((std::shared_ptr<Socket> )nullptr, room, "%1M mutters obscenities under %s breath.", this, hisHer());
                // If we're not a perm, become a fast wander and go looking for the guy
                setFlag(M_FAST_WANDER);
                // Clear any aggressive flags so we don't go hit someone who
                // doesn't deserve it!  Chase the shoplifter down
                clearFlag(M_AGGRESSIVE_GOOD);
                clearFlag(M_AGGRESSIVE_EVIL);
                clearFlag(M_WILL_BE_AGGRESSIVE);
                clearFlag(M_WILL_ASSIST);
                setFlag(M_CHASING_SOMEONE);
                setFlag(M_OUTLAW_AGGRO);
            } else if(flagIsSet(M_ATTACKING_SHOPLIFTER)) {
                broadcast((std::shared_ptr<Socket> )nullptr, room, "%1M lets down %s guard.", this, hisHer());
                clearFlag(M_ATTACKING_SHOPLIFTER);
                clearEnemyList();
            }
        }

        // If we're chasing someone we have a chance to wander away
        if(flagIsSet(M_CHASING_SOMEONE) && Random::get(1,100) < 10) {
            std::shared_ptr<Creature> target = getTarget(false);
            if(target)
                broadcast((std::shared_ptr<Socket> )nullptr, room, "%1M gives up %s search for %s and wanders away.", this, hisHer(), target->getCName());
            else
                broadcast((std::shared_ptr<Socket> )nullptr, room, "%1M gives up %s search and wanders away.", this, hisHer());
            return(2);
        }
    }

    // Special case for aggressive monsters
    // If we're an aggressive non perm and haven't had any combat in 20 minutes
    const std::string moveString = Move::getString(Containable::downcasted_shared_from_this<Monster>());
    if(flagIsSet(M_AGGRESSIVE) &&
        t - lasttime[LT_AGGRO_ACTION].ltime > 1200 &&
       !flagIsSet(M_PERMENANT_MONSTER)
    ) {
        // Then we've got a chance to wander away
        if(Random::get<bool>(0.05)) {
            if(race || monType::isIntelligent(type))
                broadcast((std::shared_ptr<Socket> )nullptr, room, "%1M %s away in search of someone to bully.", this, moveString.c_str());
            else
                broadcast((std::shared_ptr<Socket> )nullptr, room, "%1M just %s away.", this, moveString.c_str());
            return(2);
        }
    }


    // Now see if we'll either wander away or if we can become a wanderer
          // No wandering away if we have scavanged
    if( (!flagIsSet(M_HAS_SCAVANGED) &&
        // if fast wander, wander even if perm.
        (!flagIsSet(M_PERMENANT_MONSTER) || flagIsSet(M_FAST_WANDER)) &&
        // pets and other followers don't wander
        !isPet() && !flagIsSet(M_DM_FOLLOW)) &&
        // will not wander if magically held - since they can't move
        (!isMagicallyHeld()))
    {
        if( // If it's time to wander, and we have no enemey
            (t - i > 60 && (inUniqueRoom() && Random::get(1, 100) <= getUniqueRoomParent()->wander.getTraffic()) && !hasEnemy()) ||
            // or we fast-wander if enemies are not nearby
            (flagIsSet(M_FAST_WANDER) && !nearEnemy()))
        {
            lasttime[LT_MON_WANDER].ltime = t;

            // Special rules if we fast wander
            if(flagIsSet(M_FAST_WANDER)) {
                // We always get the mobile creature flag set
                setFlag(M_MOBILE_MONSTER);

                // Now we've got a small chance here to wander away
                if(hasEnemy())
                    wanderchance = 1;
                else
                    wanderchance = 5;
                    // If we're not a permenant monster
                if( !flagIsSet(M_PERMENANT_MONSTER) &&
                    // and there is no dminvis person in the room
                    !room->dmInRoom() &&
                    // and we've got a wander chance
                    (Random::get(1,100) <= wanderchance)
                ) {
                    broadcast((std::shared_ptr<Socket> )nullptr, room, "%1M just %s away.", this, moveString.c_str());
                    return(2);
                }
            }

            // If we're not a fast wanderer
            if(!flagIsSet(M_FAST_WANDER)) {
                // And we've got a mobile chance
                if(Random::get(1, 100) < Mobilechance) {
                    // Then we can wander next round
                    setFlag(M_MOBILE_MONSTER);
                }
                // Otherwise if we have no enemy and we're not a perm, and there's no
                // dm invis person in the room, we can wander away
                else if(!room->dmInRoom() && !hasEnemy() && !flagIsSet(M_PERMENANT_MONSTER)) {
                    // Time to wander away
                    broadcast((std::shared_ptr<Socket> )nullptr, room, "%1M just %s away.", this, moveString.c_str());
                    return(2);
                }
            }
        } else if(t - i > 60)
            lasttime[LT_MON_WANDER].ltime = t;
    }
    return(0);
}

//*********************************************************************
//                      checkEnemyMobs
//*********************************************************************
// If we're not currently fighthing someone, see if we hate any of the
// monsters in the room and attack them!

bool Monster::checkEnemyMobs() {
    int i=0, aggroSize=0;
    std::shared_ptr<BaseRoom> room = getRoomParent();

    for(i = 0 ; i<NUM_ENEMY_MOB ;i++) {
        if(enemy_mob[i].id != 0) {
            aggroSize++;
        }
    }
    // Nothing to aggro, null room, or we have a nearby enemy
    if(aggroSize == 0 || !room || nearEnemy())
        return(false);

    // 25% chance each update cycle to aggro an enemy monster
    if(Random::get(1,100) > 25)
        return(false);

    // We know we have some enemies, now lets see if we have any
    // enemies in the room with us to attack!

    for(const auto& mons : room->monsters) {
        if(mons.get() != this && isEnemyMob(mons)) {
            broadcast((std::shared_ptr<Socket> )nullptr, room, "%M attacks %N!", this, mons.get());
            addEnemy(mons);
            updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
            return(true);
        }
    }

    return(false);
}

//*********************************************************************
//                      isEnemyMob
//*********************************************************************

bool Monster::isEnemyMob(const std::shared_ptr<Monster>  target) const {
    if(target->info.id == 0)
        return(false);
    for(const auto & i : enemy_mob) {
        if(i == target->info)
            return(true);
    }
    return(false);
}


//*********************************************************************
//                      toJail
//*********************************************************************
// Sends a guy off to jail

int Monster::toJail(std::shared_ptr<Player> player) {
    std::shared_ptr<UniqueRoom> room=nullptr;
    CatRef jailroom;
    jailroom.id = 2650;
    long jailtime=0;

    if(player->isStaff())
        return(0);

    if(jail.id)
        jailroom = jail;

    if(!loadRoom(jailroom, room)) {
        player->print("%M gets very confused.\n", this);
        return(-1);
    }

    logn("log.jail", "%s was hauled off to jail (%s) by %s.\n", player->getCName(), jailroom.displayStr().c_str(), getCName());
    player->deleteFromRoom();
    player->addToRoom(room);
    player->doPetFollow();

    if(room->flagIsSet(R_MOB_JAIL)) {
        jailtime = (std::max(5,(player->getLevel())*30));

        if(player->isCt())
            jailtime = 15L;

        player->lasttime[LT_MOB_JAILED].ltime = time(nullptr);
        player->lasttime[LT_MOB_JAILED].interval = jailtime;

        player->print("You can get out in around %ld day%s!\n",
              (((jailtime / 60)/24 > 1) ? ((jailtime / 60)/24): 1),
              (((jailtime / 60)/24 > 1) ? "s":""));
    }

    return(1);
}

//*********************************************************************
//                      grabCoins
//*********************************************************************
// steals someone's gold

int Monster::grabCoins(const std::shared_ptr<Player>& player) {
    if(!player->coins[GOLD])
        return(0);

    unsigned long grab = Random::get<unsigned long>(1, player->coins[GOLD]), num = player->coins[GOLD];
    // never steal all gold if they have more than 100k
    grab = std::min(grab, 100000UL);

    Server::logGold(GOLD_OUT, player, Money(grab, GOLD), Containable::downcasted_shared_from_this<Monster>(), "Mugging");
    coins.add(grab, GOLD);
    player->coins.sub(grab, GOLD);

    logn("log.mug", "%s was mugged by %s for %d gold.\n", player->getCName(), getCName(), grab);

    if(grab == num) {
        player->print("%M grabs all your coins!\n", this);
        return(2);
    } else {
        player->print("%M takes %d gold coins from you!\n", this, grab);
        return(1);
    }
}

void spawnMonsters(const std::string &roomId, const std::list<std::string> &monsters) {
    std::shared_ptr<UniqueRoom> room = nullptr;
    std::shared_ptr<Monster>  monster = nullptr;
    CatRef  cr;

    getCatRef(roomId, cr, nullptr);
    if(!loadRoom(cr, room)) {
        broadcast(isDm, "Error loading room %s", roomId.c_str());
        return;
    }

    for(const auto & mId : monsters) {
        getCatRef(mId, cr, nullptr);
        if(!loadMonster(cr, monster)) {
            broadcast(isDm, "^GError loading monster %s in room %s^x", mId.c_str(), roomId.c_str());
            continue;
        }

        if ( ((room->countCrt()+1) > (room->getMaxMobs()>0?room->getMaxMobs():MAX_MOBS_IN_ROOM)) ) {
            broadcast(isDm, "^GHooK:spawnMonsters: ^gtried to exceed room maxMobs (%d)\n--Room: %s(%s^g), Monster: %s(%s^g)^x", 
                                                    (room->getMaxMobs()>0?room->getMaxMobs():MAX_MOBS_IN_ROOM), roomId.c_str(), 
                                                                            room->getName().c_str(), mId.c_str(), monster->getCName());
            continue;
        }
        
        if (monster->flagIsSet(M_PERMENANT_MONSTER)) {
            broadcast(isDm, "^GHook:spawnMonsters: ^gtried to spawn PERM monster\n--Monster: %s(%s^g), Room: %s(%s^g)^x", 
                                        mId.c_str(), monster->getCName(), roomId.c_str(), room->getName().c_str());
            continue;
        }

        monster->initMonster();

        if (!monster->flagIsSet(M_CUSTOM))
            monster->validateAc();

        if (!monster->flagIsSet(M_NO_ADJUST))
            monster->adjust(-1);

        broadcast(isDm, "^GHook:spawnMonsters: ^gspawned monster %s(%s^g) in room %s(%s^g)^x", 
                                            mId.c_str(), monster->getCName(), roomId.c_str(), room->getName().c_str());
        monster->addToRoom(room);
        gServer->addActive(monster);

    }
}