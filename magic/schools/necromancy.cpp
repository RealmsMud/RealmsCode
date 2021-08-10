/*
 * necromancy.cpp
 *   Necromancy spells
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <cstring>         // for strlen, strncmp
#include <ctime>           // for time, time_t

#include "bstring.hpp"     // for bstring
#include "cmd.hpp"         // for cmd
#include "config.hpp"      // for Config, gConfig
   // for Container
#include "creatures.hpp"   // for Creature, Monster, Player, NO_CHECK
#include "deityData.hpp"   // for DeityData
#include "flags.hpp"       // for M_PERMENANT_MONSTER, M_PET, M_UNDEAD, P_LI...
#include "global.hpp"      // for CastType, CastType::CAST, CreatureClass
    // for lasttime
#include "magic.hpp"       // for SpellData, doOffensive, getPetTitle, petTa...
#include "mud.hpp"         // for LT_ANIMATE, ospell, LT_SPELL, DL_HARM, LT
#include "objects.hpp"     // for Object
#include "proto.hpp"       // for broadcast, dice, bonus, dec_daily, get_spe...
#include "random.hpp"      // for Random
#include "rooms.hpp"       // for BaseRoom
#include "server.hpp"      // for Server, gServer
  // for Statistics
       // for Stat
#include "structs.hpp"     // for osp_t, daily
#include "utils.hpp"       // for MIN, MAX
#include "xml.hpp"         // for loadMonster


//*********************************************************************
//                      splHarm
//*********************************************************************

int splHarm(Creature* player, cmd* cmnd, SpellData* spellData) {
    Player  *pPlayer = player->getAsPlayer();
    Creature* target=nullptr;
    int wrongdiety=0, multi=0, roll=0, dmg=0, bns=0, saved=0;


    if(player->getClass() !=  CreatureClass::CLERIC && !player->isCt() && spellData->how == CastType::CAST) {
        player->print("Your class prohibits you from casting that spell.\n");
        return(0);
    }

    if(pPlayer && pPlayer->hasSecondClass())
        multi=1;

    switch(player->getDeity()) {
    case ENOCH:
    case CERIS:
    case JAKAR:
    case GRADIUS:
    case ARES:
    case KAMIRA:
    case LINOTHAN:
        wrongdiety = 1;
        break;
    }

    if( (wrongdiety || multi) &&
        player->getClass() == CreatureClass::CLERIC &&
        !player->isStaff() &&
        spellData->how != CastType::POTION
    ) {
        player->print("You are not allowed to cast that spell.\n");
        if(wrongdiety)
            player->print("That magic is forbidden by %s.\n", gConfig->getDeity(player->getDeity())->getName().c_str());
        if(multi)
            player->print("Only those that embrace Aramon purely are granted that power.\n");

        return(0);
    }



    // Harm self
    if(cmnd->num == 2) {

        if(spellData->how != CastType::POTION) {
            player->print("You can't do that!\n");
            return(0);
        } else {
            if(spellData->how == CastType::CAST && player->isPlayer())
                player->getAsPlayer()->statistics.offensiveCast();
            player->hp.setCur(MIN<int>(player->hp.getCur(), Random::get(1,10)));
            player->print("Your lifeforce is nearly sucked away by deadly magic.\n");
            return(0);
        }

    // Cast harm on another player or monster
    } else {
        if(player->noPotion( spellData))
            return(0);

        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            player->print("That person is not here.\n");
            return(0);
        }

        if(spellData->how != CastType::CAST && spellData->how != CastType::SCROLL && spellData->how != CastType::WAND) {
            player->print("Nothing happens.\n");
            return(0);
        }

        if(!player->canAttack(target))
            return(0);

        if(pPlayer && target->isMonster() && !target->isPet()
                && target->getAsMonster()->nearEnemy() && !target->getAsMonster()->isEnemy(player)) {
            player->print("Not while %N is in combat with someone.\n", target);
            return(0);
        }


        if(target->pFlagIsSet(P_LINKDEAD) && target->getClass() !=  CreatureClass::LICH) {
            player->print("%M is immune to that right now.\n", target);
            return(0);
        }

        if (pPlayer && target->isMonster() && !player->isCT() && target->mFlagIsSet(M_NO_HARM_SPELL)) {
            player->print("Your spell has no effect on %M.\n", target);
            return(0);
        }


        if( !dec_daily(&player->daily[DL_HARM]) &&
            spellData->how == CastType::CAST &&
            !player->isCt() &&
            player->isPlayer()
        ) {
            player->print("You have been granted that spell enough times for today.\n");
            return(0);
        }

        if(target->isPlayer() && target->getClass() !=  CreatureClass::LICH)
            target->wake("Terrible nightmares disturb your sleep!");
        player->print("You cast a harm spell on %N.\n", target);
        target->print("%M casts a harm spell on you!\n", player);
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts a harm spell on %N!", player, target);
        if(target->isMonster())
            target->getAsMonster()->addEnemy(player);

        if(spellData->how == CastType::CAST && player->isPlayer())
            player->getAsPlayer()->statistics.offensiveCast();

        if(target->isPlayer() && target->getClass() == CreatureClass::LICH) {
            target->hp.restore();

            player->print("Your harm spell completely heals %s.\n", target->getCName());
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M's harm spell completely heals %N.", player, target);
            target->print("Your life force is completely regenerated by %N's harm spell.\n", player);
            return(1);
        }


        bns = 10 * ((int)target->getLevel() - (int)player->getLevel());

        if(target->mFlagIsSet(M_PERMENANT_MONSTER))
            bns += 40;

        saved = target->chkSave(DEA, player, bns);



        if(!saved || player->isCt()) {
            roll = Random::get(1,10);
            dmg = target->hp.getCur() - roll;

            target->hp.setCur(MIN<int>(target->hp.getCur(), roll));
            target->print("Your lifeforce is nearly sucked away by deadly magic.\n");
            player->print("Your harm spell nearly sucks the life out of %N!\n", target);
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M's harm spell sucks away %N's life.", player, target);
            if(target->isMonster())
                target->getAsMonster()->adjustThreat(player, dmg);

        } else {

            dmg = target->hp.getCur()/4;

            player->print("Your harm spell doubles %N over in pain.\n", target);
            target->print("Part of your lifeforce is sucked away by deadly magic.\n");
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M's harm wracks %N with pain.", player, target);
            player->doDamage(target, dmg, NO_CHECK);
        }
    }

    return(1);
}

//********************************************************************
//                      drain_exp
//*********************************************************************
// The spell drain_exp causes a player to lose a selected amout of
// exp.  When a player loses exp, the player's magical realm and
// weapon procifiency will reflect the change.  This spell is not
// intended to be learned or casted by a player.

int drain_exp(Creature* player, cmd* cmnd, SpellData* spellData) {
    Creature* target=nullptr;

    unsigned long loss=0;

    if(spellData->how == CastType::CAST && !player->isCt()) {
        player->print("You may not cast that spell.\n");
        return(0);
    }

    if(spellData->how == CastType::SCROLL) {
        player->print("You may not cast that spell.\n");
        return(0);
    }

    // drain exp on self
    if(cmnd->num == 2) {

        if(spellData->how == CastType::POTION || spellData->how == CastType::WAND)
            loss = dice(player->getLevel(), player->getLevel(), (player->getLevel()) * 10);

        else if(spellData->how == CastType::CAST)
            loss = dice(player->getLevel(), player->getLevel(), 1);

        loss = MIN(loss, player->getExperience());

        if(spellData->how == CastType::CAST || spellData->how == CastType::WAND) {
            player->print("You cast an energy drain spell on yourself.\n");
            player->print("You lose %d experience.\n", loss);
            broadcast(player->getSock(), player->getParent(), "%M casts energy drain on %sself.", player, player->himHer());
        } else if(spellData->how == CastType::POTION) {
            player->print("You feel your experience slipping away.\n");
            player->print("You lose %d experience.\n", loss);
        }
        player->subExperience(loss);

    } else {
        // energy drain a monster or player
        if(player->noPotion( spellData))
            return(0);

        loss = dice(player->getLevel(), player->getLevel(), 1);

        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findCreature(player, cmnd->str[2], cmnd->val[2], false);

        if(!target) {
            player->print("That's not here.\n");
            return(0);
        }


        loss = MIN(loss, target->getExperience());
        if(spellData->how == CastType::CAST || spellData->how == CastType::WAND) {
            player->print("You cast energy drain on %N.\n", target);
            broadcast(player->getSock(), target->getSock(), player->getParent(), "%M casts energy drain on %N.", player, target);
            target->print(
                  "%M casts energy drain on you.\nYou feel your experience slipping away.\n",
                  player);
            target->print("You lose %d experience.\n", loss);
            player->print("%M loses %d experience.\n", target, loss);
        }

        target->subExperience(loss);
    }

    return(1);
}

//*********************************************************************
//                      animate_dead
//*********************************************************************
// This function allows clerics of Aramon to invoke undead targets.

int animateDeadCmd(Player* player, cmd* cmnd) {
    SpellData data;
    data.set(CastType::SKILL, NECROMANCY, EVIL, nullptr, player);
    if(!data.check(player))
        return(0);
    return(animate_dead(player, cmnd, &data));
}

int animate_dead(Creature* player, cmd* cmnd, SpellData* spellData) {
    Player* pPlayer = player->getAsPlayer();
    if(!pPlayer)
        return(0);

    Monster *target=nullptr;
    int     title=0, mp=0, shocked=0, level=0, skLevel=0;
    int     crt_num=0, buff=0, interval=0;
    time_t  t, i;


    if(!player->ableToDoCommand())
        return(0);

    if(!strncmp(cmnd->str[0], "animate", strlen(cmnd->str[0])))
        spellData->how = CastType::SKILL;

    if(player->noPotion( spellData))
        return(0);

    if(spellData->how == CastType::SKILL && !player->knowsSkill("animate")) {
        player->print("You lack the ability to call forth the dead.\n");
        return(0);
    }

    if(spellData->how == CastType::CAST) {
        if(!(player->getClass() == CreatureClass::CLERIC && player->getDeity() == ARAMON) &&
            !player->isCt())
        {
            player->print("Only Clerics of %s may cast that spell.\n", gConfig->getDeity(ARAMON)->getName().c_str());
            return(0);
        }
    }



    if(!player->isCt() &&
        (spellData->how == CastType::CAST || spellData->how == CastType::SKILL) &&
        player->getAdjustedAlignment() > REDDISH)
    {
        player->print("You are not evil enough to do that!\n");
        shocked = Random::get(5,10);

        player->print("You are shocked for %d damage by %s's wrath!\n", shocked, gConfig->getDeity(player->getDeity())->getName().c_str());

        if(player->hp.getCur() > 10)
            player->hp.decrease(shocked);
        else
            player->hp.setCur(1);

        return(0);
    }

    if(player->hasPet() && !player->checkStaff("You may only animate one undead creature at a time!\n"))
        return(0);

    /*
    if(spellData->how == CastType::SKILL) {
        skLevel=(int)player->getSkillLevel("animate");
    } else {
        skLevel = player->getLevel();
    }
    */
    // TODO: conjure/animate is no longer a skill until progression
    // has been fixed
    skLevel = player->getLevel();


    if(player->getClass() == CreatureClass::CLERIC && pPlayer->getSecondClass() == CreatureClass::ASSASSIN)
        skLevel = MAX(1, skLevel-3);

    if(spellData->object) {
        level = spellData->object->getQuality()/10;
    } else {
        level = skLevel;
    }


    title = getPetTitle(spellData->how, level, spellData->how == CastType::WAND && player->getClass() !=  CreatureClass::CLERIC && player->getDeity() != ARAMON, true);
    mp = 4 * title;

    if(spellData->how == CastType::CAST && !player->checkMp(mp))
        return(0);

    t = time(nullptr);
    i = LT(player, LT_ANIMATE);
    if(i > t && !player->isCt() && spellData->how != CastType::WAND) {
        player->pleaseWait(i-t);
        return(0);
    }
    if(spellData->how == CastType::CAST) {
        if(player->spellFail( spellData->how)) {
            player->subMp(mp);
            return(0);
        }
    }


    crt_num = MAX(BASE_UNDEAD, BASE_UNDEAD + 3*(title-1));
    // 0 = weak, 1 = normal, 2 = buff
    buff = Random::get(1,3) - 1;

    if(spellData->object) {
        level = spellData->object->getQuality()/10;
    } else {
        level = skLevel;
    }

    switch(buff) {
    case 0:
        // medium undead, add 1
        crt_num++;
        level -=- 3;
        break;
    case 1:
        // weak undead
        level -= 2;
        break;
    case 2:
        // Buff undead, add 2
        level -= Random::get(0,1);
        crt_num += 2;
        break;
    default:
        player->print("Something's wrong.\n");
        break;
    }

    level = MAX(MIN(MAXALVL, level), 1);

    //  player->print("Title-1: %d     crt_num: %d        buff: %d\n", title-1, crt_num, buff);
    // load the undead Creature
    if(!loadMonster(crt_num, &target)) {
        player->print("Error loading undead target (%d)!\n", crt_num);
        return(0);
    }

    player->print("A %s crawls forth from the earth to aid you.\n", target->getCName());
    player->checkImprove("animate", true);
    broadcast(pPlayer->getSock(), player->getRoomParent(), "A %s crawls forth from the earth to aid %N.",
        target->getCName(), player);

    // add mob to the room, make it active, and make it follow the summoner
    target->updateAttackTimer();
    target->addToRoom(player->getRoomParent());
    gServer->addActive(target);

    player->addPet(target);

    petTalkDesc(target, player);
    target->setFlag(M_PET);
    target->setFlag(M_UNDEAD);
    // pets use LT_SPELL
    target->lasttime[LT_SPELL].ltime = t;
    target->lasttime[LT_SPELL].interval = 3;
    target->proficiency[1] = CONJUREANIM;


    target->setLevel(level);

    // This will be adjusted in 2.50, for now just str*1.75
    target->setAttackPower((unsigned int)(target->strength.getCur()*1.75));

    target->setDefenseSkill(((target->getLevel()-1) * 10) + (buff*5));
    target->setWeaponSkill(((target->getLevel()-1) * 10) + (buff*5));


    // find out how long it's going to last and create all the timeouts
    //interval = (60L*Random::get(2,4)) + (bonus((int) player->piety.getCur())*60L >= 0 ? bonus((int) pPlayer->piety.getCur())*60L : 0);      /* + 60 * title; */

    interval = ((60L*Random::get(3,5)) + (60L*bonus((int) player->piety.getCur())));
    target->lasttime[LT_ANIMATE].ltime = t;
    target->lasttime[LT_ANIMATE].interval = interval;
    player->lasttime[LT_ANIMATE].ltime = t;
    player->lasttime[LT_ANIMATE].interval = 900L;
    if(player->isCt())
        player->lasttime[LT_ANIMATE].interval = 6L;

    if(spellData->how == CastType::CAST)
        player->mp.decrease(mp);

    if(spellData->how == CastType::SKILL)
        return(PROMPT);
    return(1);
}

//*********************************************************************
//                      splNecroDrain
//*********************************************************************

int splNecroDrain(Creature* player, cmd* cmnd, SpellData* spellData) {
    Creature* target=nullptr;
    bstring spell = "";
    int tier=0, c=0;
    osp_t osp;

    for(c=0; ospell[c].splno != get_spell_num(spellData->splno); c++)
        if(ospell[c].splno == -1)
            return(0);
    osp = ospell[c];

    switch(spellData->splno) {
    case S_SAP_LIFE:
        tier = 1;
        spell = "sap life";
        break;
    case S_LIFETAP:
        tier = 2;
        spell = "lifetap";
        break;
    case S_LIFEDRAW:
        tier = 3;
        spell = "lifedraw";
        break;
    case S_DRAW_SPIRIT:
        spell = "draw spirit";
        tier = 4;
        break;
    case S_SIPHON_LIFE:
        tier = 5;
        spell = "siphon life";
        break;
    case S_SPIRIT_STRIKE:
        tier = 6;
        spell = "spirit strike";
        break;
    case S_SOULSTEAL:
        tier = 7;
        spell = "soulsteal";
        break;
    case S_TOUCH_OF_KESH:
        tier = 8;
        spell = "touch of Kesh";
        break;
    default:
        player->print("Your spell fails unexpectedly.\n");
        return(0);
    }

    if((target = player->findMagicVictim(cmnd->str[2], cmnd->val[2], spellData, true, false, "Cast on what?\n", "You don't see that here.\n")) == nullptr)
        return(0);

    return(doOffensive(player, target, spellData, spell.c_str(), &osp));
}
