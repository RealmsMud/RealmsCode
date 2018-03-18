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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include <math.h>

#include "creatures.hpp"
#include "mud.hpp"
#include "factions.hpp"
#include "move.hpp"
#include "rooms.hpp"
#include "unique.hpp"
#include "server.hpp"
#include "socket.hpp"
#include "xml.hpp"

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
    if(id.empty() || id.equals("-1")) {
        setId(gServer->getNextMonsterId());
    }
}

//*********************************************************************
//                      pulseTick
//*********************************************************************

bool hearMobTick(Socket* sock) {
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

    //BaseRoom* room = getRoom();
    // ****** Main Tick ******
    if(mainTick < t && !ill) {
        double power = 0.8;
        int hpTickTime = 60 - 5*bonus(constitution.getCur());

        if(flagIsSet(M_FAST_TICK) && !nearEnemy()) {
            hpTickTime = MAX(1,(15 - (2*bonus(constitution.getCur()))));
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
            mpTickTime = MAX(1,(5 - (2*bonus(piety.getCur()))));
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
            this, level, currentLocation.room.str().c_str(),
            MIN(hp.getCur(),hp.getMax()), hp.getMax(), hpTickAmt,
            MIN(mp.getCur(),mp.getMax()), mp.getMax(), mpTickAmt);
    }

    // reset hide when mob ticks to full and no enemies are in the room
    if(!nearEnemy(this) && hp.getCur() >= hp.getMax()/4) {
        if(flagIsSet(M_WAS_HIDDEN)) // Reset mob-hide
        {
            broadcast(nullptr, getRoomParent(), "%M hides in shadows.", this);
            setFlag(M_HIDDEN);
            clearFlag(M_WAS_HIDDEN);
        }
    }
    // Mobs shouldn't stay berserk after they tick full
    if(!nearEnemy(this) && hp.getCur() == hp.getMax()) {
        if(isEffected("berserk")) {
            broadcast(nullptr, getRoomParent(), "^r%M's rage diminishes!", this);
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

    for(Player* ply : getRoomParent()->players) {

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
            if(mrand(1,100)<=2) {
                broadcast(ply->getSock(), ply->getRoomParent(), "%M spits, \"Doctor killer!\" at %N.", this, ply);
                ply->print("%M spits, \"Doctor killer!\" at you.\n", this);
            }
            continue;
        }

        if(spellIsKnown(S_SLOW_POISON)) {

            chance = mrand(1,100);
            if((mp.getCur() >= 8) && (chance < 10) && !ply->isEffected("slow-poison") && ply->isPoisoned()) {
                if(ply->immuneToPoison())
                    continue;

                if(ply->isEffected("slow-poison"))
                    continue;


                ply->print("%M casts a slow-poison spell on you.\n", this);
                broadcast(ply->getSock(), ply->getSock(), getRoomParent(),
                    "%M casts a slow-poison spell on %N.", this, ply);
                mp.decrease(8);

                ply->addPermEffect("slow-poison");
                continue;
            }
        }

        if(spellIsKnown(S_CURE_DISEASE)) {

            chance = mrand(1,100);
            if((mp.getCur() >= 12) && (chance < 10) && (ply->isDiseased())) {
                if(ply->immuneToDisease())
                    continue;

                ply->print("%M casts a cure-disease spell on you.\n", this);

                broadcast(ply->getSock(), ply->getSock(), getRoomParent(), "%M casts a cure-disease spell on %N.",
                    this, ply);
                mp.decrease(12);

                ply->cureDisease();
                continue;
            }
        }

        if(ply->getClass() == CreatureClass::LICH)
            continue;
        if(ply->flagIsSet(P_POISONED_BY_PLAYER) && mrand(1,100) >= 3)
            continue;

        if(spellIsKnown(S_VIGOR)) {
            chance = mrand(1,100);
            if((mp.getCur() >= 2) && (chance <= 5) && (ply->hp.getCur() < ply->hp.getMax())) {

                ply->print("%M casts a vigor spell on you.\n", this);

                broadcast(ply->getSock(), ply->getSock(), getRoomParent(), "%M casts a vigor spell on %N.",
                    this, ply);
                mp.decrease(2);
                heal = mrand(1,6) + bonus((int) piety.getCur());
                heal = MAX(1, heal);

                if(cClass == CreatureClass::CLERIC && clan != 12) {
                    heal *= 2;
                    heal += (level / 2);
                }
                if( (cClass == CreatureClass::CLERIC && clan == 11) ||
                    (cClass == CreatureClass::PALADIN)
                )
                    heal += mrand(1,4);
                ply->hp.increase(heal);
                heal = 0;
                continue;
            }
        }

        if(spellIsKnown(S_MEND_WOUNDS)) {

            chance = mrand(1,100);
            if((mp.getCur() >= 4) && (chance <= 5) && (ply->hp.getCur() < (ply->hp.getMax()/2))) {

                ply->print("%M casts a mend-wounds spell on you.\n", this);

                broadcast(ply->getSock(), ply->getSock(), getRoomParent(), "%M casts a mend-wounds spell on %N.",
                    this, ply);
                mp.decrease(4);
                heal = mrand(6,9) + bonus((int) piety.getCur());
                heal = MAX(1, heal);
                if(cClass == CreatureClass::CLERIC && clan != 12) {
                    heal *= 2;
                    heal += (level / 2);
                }
                if( (cClass == CreatureClass::CLERIC && clan == 11) ||
                    (cClass == CreatureClass::PALADIN)
                )
                    heal += mrand(1,4);
                ply->hp.increase(heal);
                heal = 0;
                continue;
            }
        }

        if(spellIsKnown(S_HEAL)) {

            chance = mrand(1,100);
            if((mp.getCur() >= 25) && (chance < 3) && (cClass == CreatureClass::CLERIC && clan != 12) &&
                    (ply->hp.getCur() < (ply->hp.getMax()/4)) && (ply->getLevel() >= 5)) {

                ply->print("%M casts a heal spell on you.\n", this);

                broadcast(ply->getSock(), ply->getSock(), getRoomParent(), "%M casts a heal spell on %N.", this, ply);
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
    Creature* target=0;

    i = lasttime[LT_BERSERK].ltime;
    t = time(0);

    if(t - i < 30L) // Mob has to wait 30 seconds.
        return(0);


    lasttime[LT_DEATH_SCREAM].ltime = t;
    lasttime[LT_DEATH_SCREAM].interval = 30L;

    if(hp.getCur() <= (hp.getMax()/5)) {
        clearFlag(M_DEATH_SCREAM);          // Mobs will not scream at lower then 20% hp.
        return(0);
    }


    broadcast(nullptr, getRoomParent(), "%M bellows out a deadly ear-splitting scream!!", this);

    PlayerSet::iterator pIt = getRoomParent()->players.begin();
    PlayerSet::iterator pEnd = getRoomParent()->players.end();
    while(pIt != pEnd) {
        target = (*pIt++);
        n = mrand(1,15) + level; // Damage done.

        if(target->isEffected("resist-magic"))
            n /= 2;

        target->printColor("^yHarmful vibrations double you over in pain for ^Y%d^y damage!\n", n);

        if(n > (level+11))
            target->stun(mrand(4, 6));


        if(target->isEffected("berserk")) {
            n = n - (n / 5);
            n = MAX(1L, n);
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

    return(!!death);
}


//*********************************************************************
//                      possibleEnemy
//*********************************************************************
// This function looks for possible enemies in a room. It is specifically
// designed for fast wandering, hunting mobs....like inquisitors. -- TC

bool Monster::possibleEnemy() {
    bool    possible=0, nodetect=0;

    ASSERTLOG(this);

    if( !flagIsSet(M_STEAL_ALWAYS) &&
        !flagIsSet(M_AGGRESSIVE) &&
        !flagIsSet(M_AGGRESSIVE_GOOD) &&
        !flagIsSet(M_AGGRESSIVE_EVIL)
    )
        return(0);

    for(Player* ply : getRoomParent()->players) {
        if(flagIsSet(M_AGGRESSIVE))
            possible = true;
        if(flagIsSet(M_STEAL_ALWAYS))
            possible = true;
        if(flagIsSet(M_AGGRESSIVE_GOOD) && ply->getAlignment() > 99)
            possible = true;
        if(flagIsSet(M_AGGRESSIVE_EVIL) && ply->getAlignment() < -99)
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
//                      castSpell
//*********************************************************************
// This function allows monsters to cast spells at players.

int Monster::castSpell(Creature *target) {
    Creature *temp_ptr=0;
    cmd     cmnd;
    int     i=0, spl=0, c=0;
    int     known[20], knowctr=0;
    int     (*fn)(SpellFn);
    bstring enemy;
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

    spl = known[mrand(0, knowctr-1)];

    // pets are smarter
    if(isPet()) {
        // Pets only cast offensive spells in combat
        if((int(*)(SpellFn, char*, osp_t*)) get_spell_function(spl) != splOffensive) {
            realm = proficiency[1];
            if(realm == CONJUREBARD || realm == CONJUREMAGE || realm == CONJUREANIM)
                realm = getRandomRealm();
            spl = ospell[(realm-1)].splno;
        }

        if( (int(*)(SpellFn, char*, osp_t*)) get_spell_function(spl) == splOffensive &&
            get_spell_lvl(spl) < 5) {
            // They will cast higher level offensive spells if they have the mana
            for(i=1;i <knowctr; i++) {
                if( get_spell_lvl(known[i-1]) > get_spell_lvl(spl) &&
                    (int(*)(SpellFn, char*, osp_t*))get_spell_function(known[i-1]) == splOffensive
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
    if( (int(*)(SpellFn, char*, osp_t*))get_spell_function(spl) == splOffensive ||
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
        splNo == S_HOLD_PERSON ||
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
        splNo == S_TOUCH_OF_KESH
    ) {
        // we have the exact pointer
        enemy = target->getName();
        n = 1;
        do {
            temp_ptr = getRoomParent()->findCreature(this, enemy, n, false);
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
    data.set(CastType::CAST, get_spell_school(spl), get_spell_domain(spl), 0, this);
    data.splno = spl;

    if((int(*)(SpellFn, char*, osp_t*))fn == splOffensive) {
        for(c=0; ospell[c].splno != get_spell_num(spl); c++)
            if(ospell[c].splno == -1)
                return(13);
        i = ((int(*)(SpellFn, const char*, osp_t*))*fn)(this, &cmnd, &data, get_spell_name(spl), &ospell[c]);
    } else {
        i = ((int(*)(SpellFn))*fn)(this, &cmnd, &data);
    }

    castDelay(PET_CAST_DELAY);
    return(i);
}

//*********************************************************************
//                      petCaster
//*********************************************************************

bool Monster::petCaster() {
    Player *master = getPlayerMaster();
    int     heal=0;
    long    i=0, t = time(0);

    if(!isPet() || !master || !inSameRoom(master))
        return(false);

    i = LT(this, LT_SPELL);
    if(t < i)
        return(false);

    if(mp.getCur() < 2 || mrand(1,100) > 33 || getRoomParent()->flagIsSet(R_NO_MAGIC))
        return(false);

    if( spellIsKnown(S_HEAL) &&
        master->hp.getCur() <= (master->hp.getMax()/8) &&
        mp.getCur() >= 25 &&
        mrand(1,100) > 50
    ) {
        mp.decrease(25);

        master->print("%M casts a heal spell on you.\n", this);
        broadcast(master->getSock(), getRoomParent(),
            "%M casts a heal spell on %N.", this, master);
        master->hp.restore();
        castDelay(PET_CAST_DELAY);
        return(true);
    }

    if( spellIsKnown(S_MEND_WOUNDS) &&
        master->hp.getCur() <= (master->hp.getMax()/2) &&
        mp.getCur() >= 4 &&
        mrand(1,100) > 25
    ) {
        master->print("%M casts a mend-wounds spell on you.\n", this);
        broadcast((Socket*)nullptr, master->getSock(), getRoomParent(),
            "%M casts a mend-wounds spell on %N.", this, master);

        mp.decrease(4);

        heal = MAX(bonus((int) intelligence.getCur()), bonus((int) piety.getCur())) +
               ((cClass == CreatureClass::CLERIC) ?
                level + mrand(1, 1 + level / 2) : 0) +
               (cClass == CreatureClass::PALADIN ?
                level / 2 + mrand(1, 1 + level / 3) : 0) +
               ((isEffected("vampirism") || cClass == CreatureClass::BARD || cClass == CreatureClass::DEATHKNIGHT) ?
                mrand(1, 1 + level / 5) + 2 : 0) +
               ((isEffected("lycanthropy") || cClass == CreatureClass::MONK) ? level / 5 +
                mrand(1, 1 + level / 7) + 2 : 0) +
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
        broadcast((Socket*)nullptr, master->getSock(), getRoomParent(),
            "%M casts a vigor spell on %N.", this, master);
        mp.decrease(2);

        heal = MAX(bonus((int) intelligence.getCur()),bonus((int) piety.getCur())) +
               ((cClass == CreatureClass::CLERIC) ? level / 2 +
                mrand(1, 1 + level / 2) : 0) +
               (cClass == CreatureClass::PALADIN ?
                level / 3 + mrand(1, 1 + level / 4) : 0) +
               ((isEffected("vampirism") || cClass == CreatureClass::BARD || cClass == CreatureClass::DEATHKNIGHT) ?
                mrand(1, 1 + level / 5) : 0) +
               ((isEffected("lycanthropy") || cClass == CreatureClass::MONK) ? level/5 +
                mrand(1,1+level/7) : 0)
               + mrand(1, 8);

                this->doHeal(master, heal);
        castDelay(PET_CAST_DELAY);
        return(true);
    }

    // After all possible spells have been checked on the players...check themselves
    if( spellIsKnown(S_MEND_WOUNDS) &&
        hp.getCur() <= (hp.getMax()/2) &&
        mp.getCur() >= 4 &&
        mrand(1,100) > 25
    ) {
        broadcast(nullptr, getRoomParent(), "%M casts a mend-wounds spell on %sself.", this, himHer());
        mp.decrease(4);

        heal = MAX(bonus((int) intelligence.getCur()), bonus((int) piety.getCur())) +
               ((cClass == CreatureClass::CLERIC) ?
                level + mrand(1, 1 + level / 2) : 0) +
               (cClass == CreatureClass::PALADIN ?
                level / 2 + mrand(1, 1 + level / 3) : 0) +
               ((isEffected("vampirism") || cClass == CreatureClass::BARD || cClass == CreatureClass::DEATHKNIGHT) ?
                mrand(1, 1 + level / 5) + 2 : 0) +
               ((isEffected("lycanthropy") || cClass == CreatureClass::MONK) ? level / 5 +
                mrand(1, 1 + level / 7) + 2 : 0) +
               dice(2, 7, 0);

                doHeal(this, heal);
        castDelay(PET_CAST_DELAY);
        return(true);
    }

    if( spellIsKnown(S_VIGOR) &&
        hp.getCur() < hp.getMax() &&
        mp.getCur() >= 2
    ) {

        broadcast(nullptr, getRoomParent(), "%M casts a vigor spell on %sself.", this, himHer());
        mp.decrease(2);

        heal = MAX(bonus((int) intelligence.getCur()),bonus((int) piety.getCur())) +
               ((cClass == CreatureClass::CLERIC) ? level / 2 +
                mrand(1, 1 + level / 2) : 0) +
               (cClass == CreatureClass::PALADIN ?
                level / 3 + mrand(1, 1 + level / 4) : 0) +
               ((isEffected("vampirism") || cClass == CreatureClass::BARD || cClass == CreatureClass::DEATHKNIGHT) ?
                mrand(1, 1 + level / 5) : 0) +
               ((isEffected("lycanthropy") || cClass == CreatureClass::MONK) ? level/5 +
                mrand(1,1+level/7) : 0)
               + mrand(1, 8);

        doHeal(this, heal);
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

bool hearMobAggro(Socket* sock) {
    if(!sock->getPlayer() || !isCt(sock))
        return(false);
    return(!sock->getPlayer()->flagIsSet(P_NO_AGGRO_MSG));
}

bool Monster::checkAssist() {
    int i=0, assist=0;

    Creature *crt=0;

    for(i=0; i<NUM_ASSIST_MOB; i++) {
        if(assist_mob[i].id) {
            assist = 1;
            break;
        }
    }

    if(assist) {
        for(Monster* mons : getRoomParent()->monsters) {
            if(mons == this)
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
                    crt = 0;
                }
            }
        }
    }


    if(flagIsSet(M_WILL_BE_ASSISTED) || primeFaction != "") {
        Creature* target=0;
        if(hasEnemy()) {
            target = getTarget();
            if(target)
                target = target->getAsPlayer();
            if(target) {
                for(Monster* mons : target->getRoomParent()->monsters) {
                    if(mons == this)
                        continue;
                    if(mons->hasEnemy() || !mons->canSee(target))
                        continue;

                    if( (mons->flagIsSet(M_WILL_ASSIST) && flagIsSet(M_WILL_BE_ASSISTED)) ||
                        (mons->flagIsSet(M_FACTION_ASSIST) && primeFaction == mons->getPrimeFaction()))
                    {
                        target->printColor("^r%M quickly assists %N!\n", mons, this);

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

bool Monster::willAssist(const Monster *victim) const {

    if(!victim->info.id)
        return(false);

    for(int i=0; i<NUM_ASSIST_MOB; i++) {
        if(assist_mob[i] == victim->info)
            return(true);
    }

    return(false);
}

//*********************************************************************
//                      checkScavange
//*********************************************************************

void Monster::checkScavange(long t) {
    BaseRoom* room = getRoomParent();
    Object* object=0;
    long i=0;

    if(room->flagIsSet(R_SHOP_STORAGE))
        return;

    if(flagIsSet(M_SCAVANGER)) {
        i = lasttime[LT_MON_SCAVANGE].ltime;
        if( t - i > 20 &&
            mrand(1, 100) <= 15 &&
            !room->objects.empty() &&
            canScavange((*room->objects.begin())) &&
            !((*room->objects.begin())->getType() == ObjectType::WEAPON && flagIsSet(M_WILL_WIELD)))
        {
            object = (*room->objects.begin());
            object->deleteFromRoom();

            setFlag(M_HAS_SCAVANGED);
            broadcast(nullptr, room, "%M picked up %1P.", this, object);

            // Object is gold
            if(!object->info.id) {
                coins.add(object->value);
                delete object;
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
            Object  *hide_obj = nullptr;
            char    buf[2048], *s = buf;
            int     buflen = 0;
            int     namelen;


            lasttime[LT_MOB_THIEF].ltime = t;
            buf[0] = 0;
            ObjectSet::iterator it;
            for(it = room->objects.begin() ; it != room->objects.end() ; ) {
                object = (*it++);
                if((flagIsSet(M_STREET_SWEEPER) ||
                    (object->getType() == ObjectType::MONEY || object->value[GOLD] >= 100)) &&
                        canScavange(object) )
                {
                    if(getWeight() + object->getActualWeight() > maxWeight()) {
                        if(!hide_obj && !flagIsSet(M_STREET_SWEEPER)) {
                            hide_obj = object;
                        }
                        continue;
                    }
                    namelen = object->getName().length();
                    if(buflen + namelen + 2 >= 2048)
                        break;
                    strcpy(s, object->getCName());
                    s += namelen;
                    strcpy(s, "^x, ");
                    s += 4;
                    buflen += namelen + 4;
                    object->deleteFromRoom();
                    if(object->getType() == ObjectType::MONEY || !object->info.id) {
                        coins.add(object->value);
                        delete object;
                    } else {
                        addObj(object);
                    }
                }
            }
            if(buflen) {
                buf[buflen - 2] = 0;
                broadcast(nullptr, room, "%M picked up %s.", this, buf);
            }
            if(hide_obj) {
                object = hide_obj;
                broadcast(getSock(), room, "%M attempts to hide %1P.", this, object);
                if(mrand(1, 100) <= level * 10) {
                    object->setFlag(O_HIDDEN);
                }
            }
        }
    }

}

//*********************************************************************
//                      canScavange
//*********************************************************************

bool Monster::canScavange(Object* object) {
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
    BaseRoom* room = getRoomParent();
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
        !flagIsSet(M_NO_WANDER))
    {
        // Fast wander can wander once a second
        if((flagIsSet(M_FAST_WANDER) && t - i > 1) ||
            // Non fast wander has a 20% chance after 20 seconds
            (t - i > 20 && mrand(1, 100) > 20))
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
        (t - i > 60 && mrand(1, 100) <= (!flagIsSet(M_PERMENANT_MONSTER) ? 40:30))
    ) {
        // If we have a mobile chance or we're a fast wanderer
        if((mrand(1, 100) < Mobilechance || flagIsSet(M_FAST_WANDER))) {
            // We can go looking for the guy
            setFlag(M_MOBILE_MONSTER);
        } else if(!flagIsSet(M_CHASING_SOMEONE)) {
            // If we're nto chasing someone
            // Then we might start chasing them or let our guard down

            if(!flagIsSet(M_PERMENANT_MONSTER)) {
                broadcast(nullptr, room, "%1M mutters obscenities under %s breath.", this, hisHer());
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
                broadcast(nullptr, room, "%1M lets down %s guard.", this, hisHer());
                clearFlag(M_ATTACKING_SHOPLIFTER);
                clearEnemyList();
            }
        }

        // If we're chasing someone we have a chance to wander away
        if(flagIsSet(M_CHASING_SOMEONE) && mrand(1,100) < 10) {
            Creature* target = getTarget(false);
            if(target)
                broadcast(nullptr, room, "%1M gives up %s search for %s and wanders away.", this, hisHer(), target->getCName());
            else
                broadcast(nullptr, room, "%1M gives up %s search and wanders away.", this, hisHer());
            return(2);
        }
    }

    // Special case for aggressive monsters
    // If we're an aggressive non perm and haven't had any combat in 20 minutes
    if( flagIsSet(M_AGGRESSIVE) &&
        t - lasttime[LT_AGGRO_ACTION].ltime > 1200 &&
        !flagIsSet(M_PERMENANT_MONSTER)
    ) {
        // Then we've got a chance to wander away
        if(mrand(1, 100) < 5) {
            if(race || monType::isIntelligent(type))
                broadcast(nullptr, room, "%1M %s away in search of someone to bully.", this, Move::getString(this).c_str());
            else
                broadcast(nullptr, room, "%1M just %s away.", this, Move::getString(this).c_str());
            return(2);
        }
    }


    // Now see if we'll either wander away or if we can become a wanderer
          // No wandering away if we have scavanged
    if( (!flagIsSet(M_HAS_SCAVANGED) &&
        // if fast wander, wander even if perm.
        (!flagIsSet(M_PERMENANT_MONSTER) || flagIsSet(M_FAST_WANDER)) &&
        // pets and other followers don't wander
        !isPet() && !flagIsSet(M_DM_FOLLOW)))
    {
        if( // If it's time to wander, and we have no enemey
            (t - i > 60 && (inUniqueRoom() && mrand(1, 100) <= getUniqueRoomParent()->wander.getTraffic()) && !hasEnemy()) ||
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
                    (mrand(1,100) <= wanderchance)
                ) {
                    broadcast(nullptr, room, "%1M just %s away.", this, Move::getString(this).c_str());
                    return(2);
                }
            }

            // If we're not a fast wanderer
            if(!flagIsSet(M_FAST_WANDER)) {
                // And we've got a mobile chance
                if(mrand(1, 100) < Mobilechance) {
                    // Then we can wander next round
                    setFlag(M_MOBILE_MONSTER);
                }
                // Otherwise if we have no enemy and we're not a perm, and there's no
                // dm invis person in the room, we can wander away
                else if(!room->dmInRoom() && !hasEnemy() && !flagIsSet(M_PERMENANT_MONSTER)) {
                    // Time to wander away
                    broadcast(nullptr, room, "%1M just %s away.", this, Move::getString(this).c_str());
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
    BaseRoom* room = getRoomParent();

    for(i = 0 ; i<NUM_ENEMY_MOB ;i++) {
        if(enemy_mob[i].id != 0) {
            aggroSize++;
        }
    }
    // Nothing to aggro, null room, or we have a nearby enemy
    if(aggroSize == 0 || !room || nearEnemy())
        return(false);

    // 25% chance each update cycle to aggro an enemy monster
    if(mrand(1,100) > 25)
        return(false);

    // We know we have some enemies, now lets see if we have any
    // enemies in the room with us to attack!

    for(Monster* mons : room->monsters) {
        if(mons != this && isEnemyMob(mons)) {
            broadcast(nullptr, room, "%M yells, \"Die!\"\n%M attacks %N!", this, this, mons);
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

bool Monster::isEnemyMob(const Monster* target) const {
    if(target->info.id == 0)
        return(false);
    for(int i=0 ; i<NUM_ENEMY_MOB ; i++) {
        if(enemy_mob[i] == target->info)
            return(true);
    }
    return(false);
}


//*********************************************************************
//                      toJail
//*********************************************************************
// Sends a guy off to jail

int Monster::toJail(Player* player) {
    UniqueRoom* room=0;
    CatRef jailroom;
    jailroom.id = 2650;
    long jailtime=0;

    if(player->isStaff())
        return(0);

    if(jail.id)
        jailroom = jail;

    if(!loadRoom(jailroom, &room)) {
        player->print("%M gets very confused.\n", this);
        return(-1);
    }

    logn("log.jail", "%s was hauled off to jail (%s) by %s.\n", player->getCName(), jailroom.str().c_str(), getCName());
    player->deleteFromRoom();
    player->addToRoom(room);
    player->doPetFollow();

    if(room->flagIsSet(R_MOB_JAIL)) {
        jailtime = (8 + 2*player->getLevel())*60;

        if(player->isCt())
            jailtime = 15L;

        player->lasttime[LT_MOB_JAILED].ltime = time(0);
        player->lasttime[LT_MOB_JAILED].interval = jailtime;

        player->print("You are sentenced to around %ld day%s hard labor!\n",
              (((jailtime / 60)/24 > 1) ? ((jailtime / 60)/24): 1),
              (((jailtime / 60)/24 > 1) ? "s":""));
    }

    return(1);
}

//*********************************************************************
//                      grabCoins
//*********************************************************************
// steals someone's gold

int Monster::grabCoins(Player* player) {
    if(!player->coins[GOLD])
        return(0);

    unsigned long grab = mrand(1, player->coins[GOLD]), num = player->coins[GOLD];
    // never steal all gold if they have more than 100k
    grab = MIN(grab, 100000UL);

    gServer->logGold(GOLD_OUT, player, Money(grab, GOLD), this, "Mugging");
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
