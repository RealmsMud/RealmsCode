/*
 * traps.cpp
 *   Trap routines.
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

#include <math.h>                      // for abs
#include <cstdlib>                     // for abs
#include <ctime>                       // for time
#include <set>                         // for operator==, _Rb_tree_const_ite...
#include <string>                      // for allocator, string
#include <type_traits>                 // for enable_if<>::type

#include "catRef.hpp"                  // for CatRef
#include "commands.hpp"                // for lose_all
#include "damage.hpp"                  // for Damage
#include "flags.hpp"                   // for P_PREPARED, O_RESIST_DISOLVE
#include "global.hpp"                  // for DEA, CreatureClass, MAXWEAR, BRE
#include "lasttime.hpp"                // for lasttime
#include "location.hpp"                // for Location
#include "mud.hpp"                     // for LT_SPELL
#include "mudObjects/container.hpp"    // for ObjectSet, PlayerSet, MonsterSet
#include "mudObjects/creatures.hpp"    // for Creature, NO_CHECK
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/objects.hpp"      // for Object, ObjectType, ObjectType...
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "proto.hpp"                   // for broadcast, logn, standardPoiso...
#include "random.hpp"                  // for Random
#include "server.hpp"                  // for Server, gServer
#include "stats.hpp"                   // for Stat
#include "traps.hpp"                   // for TRAP_ALARM, TRAP_BONEAV, TRAP_...
#include "xml.hpp"                     // for loadRoom

//*********************************************************************
//                      teleport_trap
//*********************************************************************
// This functions allows a player to be teleported via a trap.

void Player::teleportTrap() {
    std::shared_ptr<BaseRoom> newRoom;

    if(isStaff())
        return;

    newRoom = teleportWhere();

    deleteFromRoom();
    addToRoom(newRoom);
    doPetFollow();

    print("Your body is violently pulled in all directions.\n");

    doDamage(Containable::downcasted_shared_from_this<Player>(), hp.getMax()/2, NO_CHECK);
    hp.setCur(std::max<int>(5, hp.getCur()));
    // This is all done so a teleport trap will not be abused for exploring.
    smashInvis();

    lasttime[LT_SPELL].ltime = time(nullptr);
    lasttime[LT_SPELL].interval = 120L;

    stun(Random::get(20,95));
}

//*********************************************************************
//                      rock_slide
//*********************************************************************

void Player::rockSlide() {
    std::shared_ptr<Player> target;
    Damage dmg;

    auto room = getRoomParent();
    auto pIt = room->players.begin();
    auto pEnd = room->players.end();
    while(pIt != pEnd) {
        target = (*pIt++).lock();

        if(!target)
            continue;

        dmg.set(Random::get(10, 20));

        if(target->getClass() == CreatureClass::LICH)
            dmg.set(dmg.get()*2);

        if(target->chkSave(LCK, target, 0))
            dmg.set(dmg.get()/2);

        target->applyMagicalArmor(dmg,PHYSICAL_DMG);

        *target << ColorOn << "Falling rocks crush you for " << target->customColorize("*CC:DAMAGE*").c_str() << dmg.get() << " ^xdamage.\n" << ColorOff;
        target->doDamage(target, dmg.get(), NO_CHECK);
        if(target->hp.getCur() < 1) {
            *target << "You are crushed to death by falling rocks.\n";
            target->die(ROCKS);
        }

    }

}

//*********************************************************************
//                      doCheckTraps
//*********************************************************************

// This function checks a room to see if there are any traps and whether
// the player pointed to by the first parameter fell into any of them.


int Player::doCheckTraps(const std::shared_ptr<UniqueRoom>& room) {
    auto pThis = Containable::downcasted_shared_from_this<Player>();
    
    std::shared_ptr<Player> ply=nullptr;
    std::shared_ptr<BaseRoom> newRoom=nullptr;
    std::shared_ptr<UniqueRoom> uRoom=nullptr;
    std::shared_ptr<Creature> target=nullptr;
    int     toHit=0, a=0, num=0, roll=0, saved=0;
    Location l;
    Damage trapDamage;


    if(isCt()) {
        *pThis << ColorOn << "^MTrap passed.\n" << ColorOff;
        return(0);
    }
    if(isEffected("mist"))
        return(0);


    if(room->getTrapExit().id)
        l.room = room->getTrapExit();
    else
        l = bound;

    switch(room->getTrap()) {
    case TRAP_PIT:
    case TRAP_DART:
    case TRAP_BLOCK:
    case TRAP_NAKED:
    case TRAP_ALARM:
    case TRAP_ARROW:
    case TRAP_SPIKED_PIT:
    case TRAP_WORD:
    case TRAP_FIRE:
    case TRAP_FROST:
    case TRAP_ELEC:
    case TRAP_ACID:
    case TRAP_ROCKS:
    case TRAP_ICE:
    case TRAP_SPEAR:
    case TRAP_CROSSBOW:
    case TRAP_GASP:
    case TRAP_GASB:
    case TRAP_GASS:
    case TRAP_MUD:
    case TRAP_WEB:
    case TRAP_BONEAV:
    case TRAP_PIERCER:
    case TRAP_CHUTE:
        if(flagIsSet(P_PREPARED) && Random::get(1,20) < dexterity.getCur()/10) {
            clearFlag(P_PREPARED);
            return(0);
        }
        clearFlag(P_PREPARED);
        if(Random::get(1,100) < dexterity.getCur()/10)
            return(0);

        break;
    case TRAP_FALL:
        if(flagIsSet(P_PREPARED) && Random::get(1,30) < dexterity.getCur()/20) {
            clearFlag(P_PREPARED);
            return(0);
        }
        clearFlag(P_PREPARED);
        if(Random::get(1,100) < dexterity.getCur()/20)
            return(0);
        break;

    case TRAP_DISP:
        if(cClass == CreatureClass::MAGE || cClass == CreatureClass::LICH || cClass == CreatureClass::CLERIC || cClass == CreatureClass::DRUID) {
            if(flagIsSet(P_PREPARED) && Random::get(1,30) < ((intelligence.getCur()/10 + piety.getCur()/10)/2)) {
                clearFlag(P_PREPARED);
                return(0);
            }
            clearFlag(P_PREPARED);
            if(Random::get(1,100) < intelligence.getCur()/10)
                return(0);
        } else if(flagIsSet(P_PREPARED) && (Random::get(1,100)<=5)) {
            clearFlag(P_PREPARED);
            return(0);
        }
        break;
    case TRAP_MPDAM:
    case TRAP_RMSPL:
        if(flagIsSet(P_PREPARED) && Random::get(1,30) < ((intelligence.getCur()/10 + piety.getCur()/10)/2)) {
            clearFlag(P_PREPARED);
            return(0);
        }
        clearFlag(P_PREPARED);
        if(Random::get(1,100) < intelligence.getCur()/10)
            return(0);
        break;
    case TRAP_TPORT:
    case TRAP_ETHEREAL_TRAVEL:
        if(flagIsSet(P_PREPARED) && Random::get(1,50) < (level/2+(intelligence.getCur()/10 + piety.getCur()/10)/2)) {
            clearFlag(P_PREPARED);
            return(0);
        }
        clearFlag(P_PREPARED);
        if(Random::get(1,100) < intelligence.getCur()/10)
            return(0);
        break;

    default:
        return(0);
    }


    switch(room->getTrap()) {
    case TRAP_PIT:
        if(!isEffected("levitate")) {
            newRoom = l.loadRoom(pThis);
            if(!newRoom)
                return(0);

            *pThis << "You fell into a pit trap!\n";
            broadcast(getSock(), getRoomParent(), "%M fell into a pit trap!", this);

            deleteFromRoom();
            addToRoom(newRoom);
            doPetFollow();

            trapDamage.set(Random::get(1,15));

            if(chkSave(DEA, pThis, 0)) {
                *pThis << "You managed to control your fall.\n";
                trapDamage.set(trapDamage.get() / 2);
            }
            applyMagicalArmor(trapDamage,PHYSICAL_DMG);
            *pThis << "You lost " << trapDamage.get() << " hit points.\n";
            hp.decrease(trapDamage.get());

            if(hp.getCur() < 1)
                die(PIT);
            else
                checkTraps(newRoom->getAsUniqueRoom());
        }
        break;

    case TRAP_FALL:
        if(!isEffected("levitate") && !isEffected("fly")) {
            newRoom = l.loadRoom(pThis);
            if(!newRoom)
                return(0);

            *pThis << ColorOn << "^#You tumble downward uncontrollably!\n" << ColorOff;
            broadcast(getSock(), getRoomParent(), "%M tumbles downward uncontrollably!", this);
            deleteFromRoom();
            addToRoom(newRoom);
            doPetFollow();
            trapDamage.set(Random::get(1,20));

            if(chkSave(DEA, pThis, -25)) {
                *pThis << "You managed slow your fall.\n";
                trapDamage.set(trapDamage.get() / 2);
            }
            applyMagicalArmor(trapDamage,PHYSICAL_DMG);
            *pThis << "You lost " << trapDamage.get() << " hit points.\n";
            hp.decrease(trapDamage.get());
            if(hp.getCur() < 1)
                die(SPLAT);
            else
                checkTraps(newRoom->getAsUniqueRoom());
        }
        break;

    case TRAP_DISP:

        if(chkSave(LCK, pThis, -10))
            return(0);
        newRoom = l.loadRoom(pThis);
        if(!newRoom)
            return(0);


        *pThis << ColorOn << "^#You are overcome by vertigo!\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "%M vanishes!", this);
        deleteFromRoom();
        addToRoom(newRoom);
        doPetFollow();
        checkTraps(newRoom->getAsUniqueRoom());
        break;

    case TRAP_CHUTE:
        newRoom = l.loadRoom(pThis);
        if(!newRoom)
            return(0);

        *pThis << ColorOn << "^#You fall down a chute!\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "%M fell down a chute!", this);
        deleteFromRoom();
        addToRoom(newRoom);
        doPetFollow();
        checkTraps(newRoom->getAsUniqueRoom());

        break;

    case TRAP_ROCKS:
        if(!isEffected("fly")) {
            *pThis << ColorOn << "^#You have triggered a rock slide!\n" << ColorOff;
            broadcast(getSock(), getRoomParent(), "%M triggers a rock slide!", this);

            rockSlide();
        }
        break;

    case TRAP_BONEAV:
        newRoom = l.loadRoom(pThis);
        if(!newRoom)
            return(0);

        *pThis << ColorOn << "^#You have triggered an avalanche of bones!\nYou are knocked down by its immense weight!\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "%M was crushed by an avalanche of bones!", this);
        deleteFromRoom();
        addToRoom(newRoom);
        doPetFollow();
        trapDamage.set(Random::get(hp.getMax() / 4, hp.getMax() / 2));

        if(chkSave(DEA, pThis, -15)) {
            *pThis << "You manage to only be partially buried.\n";
            trapDamage.set(trapDamage.get() / 2);
        }
        applyMagicalArmor(trapDamage,PHYSICAL_DMG);
        *pThis << "You lost " << trapDamage.get() << " hit points.\n";
        hp.decrease(trapDamage.get());
        if(hp.getCur() < 1)
            die(BONES);
        else
            checkTraps(newRoom->getAsUniqueRoom());

        break;

    case TRAP_SPIKED_PIT:
        if(!isEffected("levitate")) {
            newRoom = l.loadRoom(pThis);
            if(!newRoom)
                return(0);

            *pThis << ColorOn << "^#You fell into a pit with spikes!" << ColorOff;
            broadcast(getSock(), getRoomParent(), "%M fell into a pit!\nIt has spikes at the bottom!", this);
            deleteFromRoom();
            addToRoom(newRoom);
            doPetFollow();
            trapDamage.set(Random::get(15,30));

            if(chkSave(DEA,pThis,0)) {
                *pThis << "The spikes barely graze you.\n";
                trapDamage.set(trapDamage.get() / 2);
            }
            if(isBrittle())
                trapDamage.set(trapDamage.get() * 3 / 2);
            applyMagicalArmor(trapDamage,PHYSICAL_DMG);
            *pThis << "You lost " << trapDamage.get() << " hit points.\n";
            hp.decrease(trapDamage.get());
            if(hp.getCur() < 1)
                die(SPIKED_PIT);
            else
                checkTraps(newRoom->getAsUniqueRoom());
        }
        break;

    case TRAP_DART:
        *pThis << ColorOn << "^#You triggered a hidden dart!\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "%M gets hit by a hidden poisoned dart!", this);
        trapDamage.set(Random::get(1,10));
        if(chkSave(DEA,pThis,-1)) {
            *pThis << "The dart barely scratches you.\n";
            trapDamage.set(trapDamage.get() / 2);
        }
        applyMagicalArmor(trapDamage,PHYSICAL_DMG);
        *pThis << "You lost " << trapDamage.get() << " hit points.\n";
        if(!immuneToPoison() && !chkSave(POI, pThis, -1)) {
            num = room->getTrapStrength() ? room->getTrapStrength() : 2;

            poison(nullptr, num, standardPoisonDuration(num, constitution.getCur()));
            poisonedBy = "a poisoned dart";
        }
        hp.decrease(trapDamage.get());
        if(hp.getCur() < 1)
            die(DART);
        break;

    case TRAP_ARROW:
        *pThis << ColorOn << "^#You triggered an arrow trap!\nA flight of arrows peppers you!\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "%M gets hit by a flight of arrows!", this);
        trapDamage.set(Random::get(15,20));

        if(chkSave(DEA,pThis,0)) {
            *pThis << "You avoided several of the arrows by dodging chaotically!\n";
            trapDamage.set(trapDamage.get() / 2);
        }
        applyMagicalArmor(trapDamage,PHYSICAL_DMG);
        *pThis << "You lost " << trapDamage.get() << " hit points.\n";

        hp.decrease(trapDamage.get());

        if(hp.getCur() < 1)
            die(ARROW);
        break;

    case TRAP_SPEAR:
        *pThis << ColorOn << "You triggered a spear trap!\nA giant spear flies towards you!" << ColorOff;
        broadcast(getSock(), getRoomParent(), "%M gets hit by a giant spear!", this);
        trapDamage.set(Random::get(10, 15));

        if(chkSave(DEA, pThis, 0)) {
            *pThis << "The glances off of you.\n";
            trapDamage.set(trapDamage.get() / 2);
        }
        applyMagicalArmor(trapDamage,PHYSICAL_DMG);
        *pThis << "You lost " << trapDamage.get() << " hit points.\n";

        hp.decrease(trapDamage.get());

        if(hp.getCur() < 1)
            die(SPEAR);
        break;

    case TRAP_CROSSBOW:
        *pThis << ColorOn << "^#You triggered a crossbow trap!\nA giant crossbow bolt hits you in the chest!\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "%M gets hit by a giant crossbow bolt!", this);
        trapDamage.set(Random::get(20, 25));

        if(chkSave(DEA, pThis, 0)) {
            *pThis << "The bolt didn't penetrate too deeply.\n";
            trapDamage.set(trapDamage.get() / 2);
        }
        // 50% chance the crossbow bolt is poisoned
        if (Random::get(1,100) <= 50) {
            *pThis << ColorOn << "^G^#The crossbow bolt is poisoned!\n" << ColorOff;
            if(!immuneToPoison() && !chkSave(POI, pThis, 0)) {
                num = room->getTrapStrength() ? room->getTrapStrength() : 7;

                poison(nullptr, num, standardPoisonDuration(num, constitution.getCur()));
                poisonedBy = "a poisoned crossbow bolt";
            }
         }
        applyMagicalArmor(trapDamage,PHYSICAL_DMG);
        *pThis << "You lost " << trapDamage.get() << " hit points.\n";

        hp.decrease(trapDamage.get());

        if(hp.getCur() < 1)
            die(CROSSBOW_TRAP);
        break;

    case TRAP_GASP:
        *pThis << ColorOn << "^G^#You triggered a gas trap!\nGas rapidly fills the room!\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "Gas rapidly fills the room!");

        if(!immuneToPoison()) {
            if(!chkSave(POI, pThis, -15)) {
                *pThis << ColorOn << "^G^#The billowing poisonous green cloud surrounds you!\n" << ColorOff;
                num = room->getTrapStrength() ? room->getTrapStrength() : 15;
                poison(nullptr, num, standardPoisonDuration(num, constitution.getCur()));
                poisonedBy = "poison gas";
            } else {
                    *pThis << "You managed to hold your breath in time.\n";
            }
        } 
        else {
            *pThis << "The billowing cloud of green gas has no effect on you.\n";
        }

        break;

    case TRAP_GASB:
        *pThis << ColorOn << "^rYou triggered a gas trap!\nReddish gas rapidly fills the room!\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "Reddish gas rapidly fills the room!");

        if(!chkSave(DEA, pThis, 0)) {
            *pThis << ColorOn << "^rThe billowing red cloud blinds your eyes!\n" << ColorOff;
            addEffect("blindness", 120 - (constitution.getCur()/10), 1);
        }
        break;

    case TRAP_GASS:
        *pThis << ColorOn << "^#You triggered a gas trap!\nGas rapidly fills the room!\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "Gas rapidly fills the room!");

        for(const auto& pIt : getRoomParent()->players) {
            if(auto fPly = pIt.lock()) {
                if (!fPly->chkSave(DEA, fPly, 0)) {
                    if (!fPly->flagIsSet(P_RESIST_STUN)) {
                        *fPly << "Billowing white clouds surrounds you!\n";
                        *fPly << "You are stunned!\n";
                        fPly->stun(Random::get(10, 18));
                    } else {
                        *fPly << "The billowing cloud of white gas has no effect on you.\n";
                    }
                } else
                    *fPly << "The billowing cloud of white gas has no effect on you.\n";
            }
        }
        break;

    case TRAP_MUD:
    case TRAP_WEB:

        if(room->getTrap() == TRAP_MUD) {
            *pThis << ColorOn << "^#You sink to your chest in stinking mud!\nYou are stuck!\n" << ColorOff;
            broadcast(getSock(), getRoomParent(), "%M sank chest deep into some mud!", this);
        } else if(room->getTrap() == TRAP_WEB) {
            *pThis << ColorOn << "^#You brush against some large spider webs!\nYou are stuck!\n" << ColorOff;
            broadcast(getSock(), getRoomParent(), "%M stuck to some large spider webs!", this);
        }

        if(chkSave(DEA, pThis , -1))
            stun(Random::get(5,10));
        else
            stun(Random::get(20,30));
        break;

    case TRAP_ETHEREAL_TRAVEL:

        if(checkDimensionalAnchor()) {
            *pThis << ColorOn << "^yYour dimensional-anchor protected you from an ethereal-travel trap!\n" << ColorOff;
        } else if(!chkSave(SPL, pThis, -25)) {
            *pThis << ColorOn << "^c^#You become tranlucent and fade away!\nYou've been transported to the ethereal plane!\n" << ColorOff;
            broadcast(getSock(), getRoomParent(), "^B%N becomes translucent and fades away!", this);
            etherealTravel();
        } else {
            *pThis << ColorOn << "^cYou get a tingling warm feeling all over.\n" << ColorOff;
        }
        break;


    case TRAP_PIERCER:
        num = std::max(1,std::min(8,Random::get((room->getTrapStrength()+1)/2, room->getTrapStrength()+1)));
        for(a=0;a<num;a++) {
            target = getRandomPlayer(room);
            if(!target)
                target = getRandomMonster(room);
            if(!target)
                continue;

            ply = target->getAsPlayer();

            if(ply && ply->flagIsSet(P_DM_INVIS))
                continue;

            trapDamage.reset();
            trapDamage.set(Random::get(target->hp.getMax()/20, target->hp.getMax()/10));
            target->modifyDamage(nullptr, PHYSICAL_DMG, trapDamage);

            toHit = 14 - (trapDamage.get()/10);
            if(target->isInvisible() && target->isPlayer())
                toHit= 20 - (trapDamage.get()/10);
            roll = Random::get(1,20);

            if(roll >= toHit) {
                broadcast(target->getSock(), room, "A living stalagtite falls upon %N from above!", target.get());
                *target << "A living stalagtite falls on you from above!\n";
                saved = chkSave(DEA, pThis, 0);
                if(saved)
                    *target << "It struck a glancing blow.\n";

                target->applyMagicalArmor(trapDamage,PHYSICAL_DMG);
                
                *target << ColorOn << "You are hit for "<< target->customColorize("*CC:DAMAGE*").c_str() << trapDamage.get() << " ^xdamage.\n" << ColorOff;

                target->hp.decrease(trapDamage.get());
                if(target->hp.getCur() < 1) {
                    if(ply) {
                        ply->die(PIERCER);
                        continue;
                    } else {
                        std::shared_ptr<Monster>  mon = target->getAsMonster();
                        broadcast((std::shared_ptr<Socket> )nullptr,  room, "%M was killed!", mon.get());
                        if(mon->flagIsSet(M_PERMANENT_MONSTER))
                            mon->diePermCrt();
                        mon->deleteFromRoom();

                        continue;
                    }
                }
                continue;
            } else {
                *target << "A falling stalagtite almost hit you!\n";
                broadcast(target->getSock(), room, "A falling stalagtite almost hits %N!", target.get());
                continue;
            }

        }
        break;

    case TRAP_FIRE:
        *pThis << ColorOn << "^R^#You set off a fire trap!\nFlames engulf you!\n" << ColorOff;
        if (isEffected("immune-fire")) {
            *pThis << "The flames do nothing to you.\n";
            break;
        }
        broadcast(getSock(), getRoomParent(), "%M is engulfed by flames!", this);
        trapDamage.set(Random::get(20,40));

        if(chkSave(BRE, pThis, 0)) {
            trapDamage.set(trapDamage.get() / 2);
            *pThis << "You managed to partially escape the flames.\n";
        }

        if(isEffected("heat-protection") || isEffected("alwayswarm"))
            trapDamage.set(trapDamage.get() / 2);
        if(isEffected("resist-fire"))
            trapDamage.set(trapDamage.get() / 2);

        *pThis << ColorOn << "You are burned for " << customColorize("*CC:DAMAGE*").c_str() << trapDamage.get() << " ^xdamage.\n" << ColorOff;

        hp.decrease(trapDamage.get());

        if(hp.getCur() < 1)
            die(FIRE_TRAP);
        break;

    case TRAP_FROST:
        *pThis << ColorOn << "^C^#You are hit by a blast of ice!\nFrost envelopes you!\n" << ColorOff;
        if (isEffected("immune-cold")) {
            *pThis << "The frost does nothing to you.\n";
            break;
        }
        broadcast(getSock(), getRoomParent(), "%M is engulfed by a cloud of frost!", this);
        trapDamage.set(Random::get(20,40));

        if(chkSave(BRE, pThis, 0)) {
            trapDamage.set(trapDamage.get() / 2);
            *pThis << "You are only partially frostbitten.\n";
        }

        if(isEffected("warmth") || isEffected("alwayscold"))
            trapDamage.set(trapDamage.get() / 2);
        if(isEffected("resist-cold"))
            trapDamage.set(trapDamage.get() / 2);

        *pThis << ColorOn << "You are frozen for " << customColorize("*CC:DAMAGE*").c_str() << trapDamage.get() << " ^xdamage.\n" << ColorOff;

        hp.decrease(trapDamage.get());

        if(hp.getCur() < 1)
            die(FROST);
        break;

    case TRAP_ELEC:
        *pThis << ColorOn << "^B^#You are hit by a crackling blue bolt!\nElectricity pulses through your body!\n" << ColorOff;
        if (isEffected("immune-electric")) {
            *pThis << "The electrical bolt does nothing but tickle.\n";
            break;
        }
        broadcast(getSock(), getRoomParent(), "%M is surrounded by crackling blue arcs!", this);
        trapDamage.set(Random::get(20,40));

        if(chkSave(BRE, pThis, 0)) {
            trapDamage.set(trapDamage.get() / 2);
            *pThis << "The electricity only half engulfed you.\n";
        }

        if(isEffected("resist-electric"))
            trapDamage.set(trapDamage.get() / 2);

        *pThis << ColorOn << "You are electrocuted for " << customColorize("*CC:DAMAGE*").c_str() << trapDamage.get() << " ^xdamage.\n" << ColorOff;

        hp.decrease(trapDamage.get());

        if(hp.getCur() < 1)
            die(ELECTRICITY);
        break;

    case TRAP_ACID:
        *pThis << ColorOn << "^g^#You are blasted by a jet of acid!\nYou are immersed in dissolving liquid.\n" << ColorOff;
        
        broadcast(getSock(), getRoomParent(), "%M is immersed in acid!", this);
        trapDamage.set(Random::get(20,30));


        if(chkSave(DEA, pThis, 0)) {
            trapDamage.set(trapDamage.get() / 2);
            *pThis << "The acid only splashed you lightly.\n";
        } else {
            *pThis << "Your skin dissolves to the bone.\n";
            loseAcid();
        }

        *pThis << ColorOn << "You are burned for " << customColorize("*CC:DAMAGE*").c_str() << trapDamage.get() << " ^xdamage.\n" << ColorOff;

        hp.decrease(trapDamage.get());

        if(hp.getCur() < 1)
            die(ACID);
        break;

    case TRAP_BLOCK:
        *pThis << ColorOn << "^W^#You triggered a falling block!\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "A large block falls on %N.", this);
        trapDamage.set(hp.getMax() / 2);

        if(chkSave(DEA, pThis, -5)) {
            trapDamage.set(trapDamage.get() / 2);
            *pThis << "The block hits you in a glancing blow\nas you quickly dive out of the way.\n";
        }
        applyMagicalArmor(trapDamage,PHYSICAL_DMG);
        *pThis << "You lost " << trapDamage.get() << " hit points.\n";
        hp.decrease(trapDamage.get());
        if(hp.getCur() < 1)
            die(BLOCK);
        break;

    case TRAP_ICE:
        *pThis << ColorOn << "^C^#A giant icicle falls from above!\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "%N's vibrations caused a giant icicle to drop!", this);
        trapDamage.set(hp.getMax() / 2);

        if(chkSave(DEA, pThis, -5)) {
            trapDamage.set(trapDamage.get() / 2);
            *pThis << "The icicle slashes you but doesn't impale you.\n";
        } else {
            *pThis << "You are impaled!\n";
            broadcast(getSock(), getRoomParent(), "%N is impaled by a huge icicle!", this);
        }
        applyMagicalArmor(trapDamage,PHYSICAL_DMG);
        *pThis << "You lost " << trapDamage.get() << " hit points.\n";
        hp.decrease(trapDamage.get());
        if(hp.getCur() < 1)
            die(ICICLE_TRAP);
        break;

    case TRAP_MPDAM:
        if(mp.getCur() <= 0)
            break;
        *pThis << ColorOn << "^m^#You are hit by an explosive force of negative energy!\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "An energy bolt strikes %N.", this);

        if(chkSave(MEN, pThis,0)) {
            trapDamage.set(std::min(mp.getCur(),mp.getMax() / 2));
            *pThis << "You lost " << trapDamage.get() << " magic points.\n";
        } else {
            trapDamage.set(mp.getCur());
            *pThis << "You feel the magical energy sucked away from you.\n";
        }

        mp.decrease(trapDamage.get());
        break;

    case TRAP_RMSPL:
        *pThis << ColorOn << "^D^#A foul smelling charcoal cloud surrounds you.\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "A charcoal cloud surrounds %N.", this);
        if(!chkSave(SPL, pThis,0)) {
            doDispelMagic();
            *pThis << "Your magic begins to dissolve.\n";
        }
        break;

    case TRAP_NAKED:
        *pThis << ColorOn << "^g^#You are covered in oozing green slime.\n" << ColorOff;
        if(chkSave(DEA, pThis, 0)) {
            broadcast(getSock(), getRoomParent(), "^gA foul smelling and oozing green slime envelops %N.", this);
            stun(Random::get(1,10));
        } else {
            *pThis << "Your possessions begin to dissolve!\n";
            loseAll(false, "slime-trap");
            stun(Random::get(5,20));
        }
        break;

    case TRAP_TPORT:

        if(checkDimensionalAnchor()) {
            *pThis << ColorOn << "^yYour dimensional-anchor protected you from a teleportation trap!^w\n" << ColorOff;
        } else if(!chkSave(SPL, pThis, 0)) {
            *pThis << ColorOn << "^yThe world changes rapidly around you!\nYou've been teleported!\n" << ColorOff;
            broadcast(getSock(), getRoomParent(), "%N disappears!", this);
            teleportTrap();
        } else {
            *pThis << ColorOn << "^yYou feel a warm tingling all over.\n" << ColorOff;
        }
        break;



    case TRAP_WORD:

        if(checkDimensionalAnchor()) {
            *pThis << ColorOn << "^yYour dimensional-anchor protected you from a word-of-recall trap!^w\n" << ColorOff;
        } else {
            *pThis << ColorOn << "^cYou phase in and out of existence.\n" << ColorOff;
            broadcast(getSock(), getRoomParent(), "%N disappears.", this);

            newRoom = getRecallRoom().loadRoom(pThis);
            if(!newRoom)
                break;
            deleteFromRoom();
            addToRoom(newRoom);
            doPetFollow();
            break;
        }

    case TRAP_ALARM:
        *pThis << ColorOn << "^W^#You set off an alarm!\n\n" << ColorOff;
        broadcast(getSock(), getRoomParent(), "%M sets off an alarm!\n", this);

        CatRef  acr;

        if(room->getTrapExit().id)
            acr = room->getTrapExit();
        else {
            acr = room->info;
            acr.id++;
        }

        if(!loadRoom(acr, uRoom))
            return(0);

        uRoom->addPermCrt();

        std::shared_ptr<Monster> tmp_crt=nullptr;
        auto mIt = uRoom->monsters.begin();
        while(mIt != uRoom->monsters.end()) {
            tmp_crt = (*mIt++);
            if(tmp_crt->flagIsSet(M_PERMANENT_MONSTER)) {
                if(!uRoom->players.empty())
                    broadcast(tmp_crt->getSock(), tmp_crt->getRoomParent(),
                        "%M hears an alarm and leaves to investigate.",tmp_crt.get());
                else
                    gServer->addActive(tmp_crt);
                tmp_crt->clearFlag(M_PERMANENT_MONSTER);
                tmp_crt->setFlag(M_AGGRESSIVE);
                tmp_crt->diePermCrt();
                tmp_crt->deleteFromRoom();
                tmp_crt->addToRoom(room);
                broadcast(tmp_crt->getSock(), tmp_crt->getRoomParent(),
                    "%M comes to investigate the alarm.", tmp_crt.get());
            }
        }
        break;
    }
    return(2);
}

//*********************************************************************
//                      checkTraps
//*********************************************************************
// A wrapper for do_check_traps so we can have a trap spring
// on everyone in the room. self is whether or not they moved themselves
// into a room - followers of a leader ALL springing weight traps would be bad.

int Player::checkTraps(std::shared_ptr<UniqueRoom> room, bool self, bool isEnter) {
    if(!room)
        return(0);

    std::shared_ptr<Player> target=nullptr;

    if(!room->getTrap()) {
        clearFlag(P_PREPARED);
        return(0);
    }

    if(isEnter && room->flagIsSet(R_NO_SPRING_TRAP_ON_ENTER))
        return(0);

    // if it's not weight-based, continue normally
    if(!room->getTrapWeight()) {
        return(doCheckTraps(room));
    } else if(
        self &&
        countForWeightTrap() &&
        room->getWeight() >= room->getTrapWeight()
    ) {
        // some traps already do area-of-effect; if we do them in
        // this loop, it'd be bad. run them normally.
        switch(room->getTrap()) {
            case TRAP_ROCKS:
            case TRAP_GASS:
            case TRAP_PIERCER:
            case TRAP_ALARM:
            case TRAP_BONEAV:
                return(doCheckTraps(room));
            default:
                break;
        }

        // we loop through everyone in the room!

        auto pIt = room->players.begin();
        auto pEnd = room->players.end();
        while(pIt != pEnd) {
            target = (*pIt++).lock();
            if(target)
                target->doCheckTraps(room);
        }
    }
    return(0);
}

//*********************************************************************
//                      countForWeightTrap
//*********************************************************************

bool Creature::countForWeightTrap() const {
    if(isMonster())
        return(!isEffected("levitate") && !isEffected("fly"));
    return(!isStaff() && !isEffected("mist") && !isEffected("levitate") && !isEffected("fly"));
}

//*********************************************************************
//                      resistLose
//*********************************************************************

bool resistLose(std::shared_ptr<Object>  object) {
    if(object->flagIsSet(O_RESIST_DISOLVE) || object->getQuestnum())
        return(true);

    int r = 15 + abs(object->getAdjustment()) * 5;

    // containers with lots of items are more resistant,
    // since it would suck to lose a lot of items
    if(object->getType() == ObjectType::CONTAINER)
        r += object->getShotsCur() * 5;

    return(Random::get(1,100) < r);
}

//*********************************************************************
//                      lose_all
//*********************************************************************

void Player::loseAll(bool destroyAll, std::string lostTo) {
    auto pThis = Containable::downcasted_shared_from_this<Player>();
    std::shared_ptr<Object>  object=nullptr;
    int     i=0;

    // if we send !destroyAll, there are some scenarios where
    // we don't destroy everything

    // remove all equipped items
    for(i=0; i<MAXWEAR; i++) {
        if(ready[i]) {
            if(!destroyAll && resistLose(ready[i])) {
                printColor("%O resisted destruction.\n", ready[i].get());
                continue;
            }

            logn("log.dissolve", "%s(L%d) lost %s to %s in room %s.\n",
                getCName(), getLevel(), ready[i]->getCName(), lostTo.c_str(), getRoomParent()->fullName().c_str());
            ready[i]->popBag(pThis, true, false, false, false, true);
            unequip(i+1, UNEQUIP_DELETE);
        }
    }

    computeAC();
    computeAttackPower();

    // delete all possessions
    ObjectSet::iterator it;
    for( it = objects.begin() ; it != objects.end() ; ) {
        object = (*it++);

        if(!destroyAll && resistLose(object)) {
            printColor("%O resisted destruction.\n", object.get());
            continue;
        }

        logn("log.dissolve", "%s(L%d) lost %s to %s in room %s.\n",
            getCName(), getLevel(), object->getCName(), lostTo.c_str(), getRoomParent()->fullName().c_str());
        object->popBag(pThis, true, false, false, false, true);
        delObj(object, true, false, true, false);
        object.reset();
    }
    checkDarkness();
}

//*********************************************************************
//                      dissolveItem
//*********************************************************************
// dissolve_item will randomly select one equipted (including held or
// wield) items on the given player and then delete it. The player
// receives a message that the item was destroyed as well as who is
// responsible for the deed.

void Player::dissolveItem(std::shared_ptr<Creature> creature) {
    char    checklist[MAXWEAR];
    int     numwear=0, i=0, n=0;

    for(i=0; i<MAXWEAR; i++) {
        checklist[i] = 0;
        // if(i==WIELD-1 || i==HELD-1) continue;
        if(ready[i])
            checklist[numwear++] = i+1;
    }

    if(!numwear)
        n = 0;
    else {
        i = Random::get(0, numwear-1);
        n = (int) checklist[i];
    }
    if(n) {
        if(ready[n-1]) {
            if(ready[n - 1]->flagIsSet(O_RESIST_DISOLVE)) {
                printColor("%M tried to dissolve your %s.\n", creature.get(), ready[n-1]->getCName());
                n = 0;
                return;
            }
        }
    }


    if(n) {
        broadcast(getSock(), getRoomParent(),"%M destroys %N's %s.", creature.get(), this, ready[n-1]->getCName());
        printColor("%M destroys your %s.\n",creature.get(), ready[n-1]->getCName());
        logn("log.dissolve", "%s(L%d) lost %s to acid in room %s.\n",
                getCName(), level, ready[n-1]->getCName(), getRoomParent()->fullName().c_str());
        // Unequip it and don't add it to the inventory, delete it
        unequip(n, UNEQUIP_DELETE);
        computeAC();
    }
}

//*********************************************************************
//                      loseAcid
//*********************************************************************
// This function goes through a players possessions worn AND carried
// and determines whether it was fragged due to dissolving acid.

void Player::loseAcid() {
    std::shared_ptr<Object>  object=nullptr;
    int     i=0;

    // Check all equipped items
    for(i=0; i<MAXWEAR; i++) {
        if(ready[i]) {
            if( !ready[i]->flagIsSet(O_RESIST_DISOLVE) &&
                (Random::get(1,100) <= 3 - abs(ready[i]->getAdjustment()))
            ) {
                if(ready[i]->flagIsSet(O_NO_PREFIX)) {
                    printColor("^y%s %s dissolved by acid!\n", ready[i]->getCName(), ready[i]->flagIsSet(O_SOME_PREFIX) ? "were":"was");
                } else {
                    printColor("^yYour %s %s dissolved by acid!\n", ready[i]->getCName(),ready[i]->flagIsSet(O_SOME_PREFIX) ? "were":"was");
                }

                logn("log.dissolve", "%s(L%d) lost %s to acid trap in room %s.\n",
                        getCName(), level, ready[i]->getCName(), getRoomParent()->fullName().c_str());
                unequip(i+1, UNEQUIP_DELETE);
            }
        }
    }


    computeAC();
    computeAttackPower();

    // 10% chance to attempt dissolving inventory possessions, reduced by higher dexterity bonus.
    if (Random::get(1,100) <= std::max(1,(10 - bonus(dexterity.getCur()))) )
    {
        ObjectSet::iterator it;
        for( it = objects.begin() ; it != objects.end() ; ) {
            object = (*it++);
            // Bags have a much much rarer chance to attempt dissolve. (approx 25/10000 == .25%)
            if(object->getType() == ObjectType::CONTAINER && (Random::get(1,10000) <= 25))
                continue;
            // Anything not r-dissolve with a magical adjustment is more resistant. Anything +-4 or better/worse is immune.
            if( !object->flagIsSet(O_RESIST_DISOLVE) &&
                (Random::get(1,100) <= (4 - abs(object->getAdjustment() )) ))
            {
                if(object->flagIsSet(O_NO_PREFIX)) {
                    printColor("^y%s %s dissolved by acid!\n", object->getCName(), object->flagIsSet(O_SOME_PREFIX) ? "were":"was");
                } else {
                    printColor("^yYour %s %s dissolved by acid!\n", object->getCName(), object->flagIsSet(O_SOME_PREFIX) ? "were":"was");
                }
                logn("log.dissolve", "%s(L%d) lost %s to acid trap in room %s.\n",
                        getCName(), level, object->getCName(), getRoomParent()->fullName().c_str());
                delObj(object, true, false, true, false);
            }
            // Check if acid loses potency and stops dissolving inv items...20% chance + dexterity bonus
            if (Random::get(1,100) <= (std::max(1,20+bonus(dexterity.getCur()))))
                break;
        }
    }
   
    checkDarkness();
}
