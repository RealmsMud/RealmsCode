/*
 * identify.cpp
 *   Bard identify
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

#include <cstdlib>                 // for atoi
#include <cstring>                 // for strcpy
#include <ctime>                   // for time
#include <string>                  // for string, allocator

#include "cmd.hpp"                 // for cmd
#include "commands.hpp"            // for cmdNoAuth, cmdIdentify
#include "config.hpp"              // for Config, gConfig
#include "dice.hpp"                // for Dice
#include "effects.hpp"             // for Effect
#include "flags.hpp"               // for O_CUSTOM_OBJ, O_EQUIPPING_BESTOWS_...
#include "global.hpp"              // for FINGER, CreatureClass, ARMS, BELT
#include "lasttime.hpp"            // for lasttime
#include "money.hpp"               // for Money
#include "mud.hpp"                 // for LT_IDENTIFY
#include "mudObjects/objects.hpp"  // for Object, ObjectType, ObjectType::CO...
#include "mudObjects/players.hpp"  // for Player
#include "objIncrease.hpp"         // for ObjIncrease, LanguageIncrease, Ski...
#include "proto.hpp"               // for broadcast, bonus, get_language_adj
#include "random.hpp"              // for Random
#include "size.hpp"                // for getSizeName
#include "skills.hpp"              // for SkillInfo
#include "stats.hpp"               // for Stat
#include "toNum.hpp"



//**********************************************************************
//                      cmdIdentify
//**********************************************************************
// Gives a bard info about an item if sucessfull

int cmdIdentify(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Object> object = nullptr;
    int chance = 0, avgbns = 0, ac = 0, wear = 0, jakar = 0;
    long i = 0, t = 0;
    std::string desc;
    std::string output;

    if(!player->ableToDoCommand())
        return(0);
    if(player->getClass() == CreatureClass::BUILDER && !player->canBuildObjects())
        return(cmdNoAuth(player));

    player->clearFlag(P_AFK);

    if(!player->isStaff()) {
        if(!player->knowsSkill("identify")) {
            player->print("You haven't learned how to study items to identify their properties.\n");
            return(0);
        }

        if(player->getClass() == CreatureClass::CLERIC && player->getDeity() == JAKAR)
            jakar=1;
        if(player->isBlind()) {
            player->printColor("^CYou're blind!\n");
            return(0);
        }
    }

    if(cmnd->num < 2) {
        player->print("What do you wish to identify?\n");
        return(0);
    }
    object = player->findObject(player, cmnd, 1);

    if(!object) {
        player->print("You don't have that object in your inventory.\n");
        return(0);
    }

    if(!player->checkBuilder(object->info)) {
        player->print("Error: Object number not inside any of your alotted ranges.\n");
        return(0);
    }

    if(!player->isStaff()) {
        i = player->lasttime[LT_IDENTIFY].ltime + player->lasttime[LT_IDENTIFY].interval;
        t = time(nullptr);
        if(i > t) {
            player->pleaseWait(i-t);
            return(0);
        }

        if(object->flagIsSet(O_CUSTOM_OBJ)) {
            player->printColor("%P was made by the gods. It is beyond you to understand their ways.\n", object.get());
            return(0);
        }

        if(object->getLevel() > player->getLevel()) {
            broadcast(player->getSock(), player->getParent(), "%M is totally puzzled by %P.", player.get(), object.get());
            player->printColor("You do not have enough experience to identify %P.\n", object.get());
            return(0);
        }

    }
    
    if(!jakar)
        avgbns = (player->intelligence.getCur() + player->piety.getCur())/2;
    else
        avgbns = player->piety.getCur();

    chance = (int)(5*player->getSkillLevel("identify")) + (bonus(avgbns)) - object->getLevel();
    chance = std::min(90, chance);

    if(player->isStaff())
        chance = 101;
    if(player->isCt())
        player->print("Chance: %d%\n", chance);

    if(!player->isStaff() && Random::get(1,100) > chance) {
        player->printColor("You need to study %P more to surmise its qualities.\n", object.get());
        player->checkImprove("identify", false);
        broadcast(player->getSock(), player->getParent(), "%M carefully studies %P.",player.get(), object.get());
        player->lasttime[LT_IDENTIFY].ltime = t;
        player->lasttime[LT_IDENTIFY].interval = 45L;
        return(0);
    } else {
        broadcast(player->getSock(), player->getParent(), "%M carefully studies %P.", player.get(), object.get());
        broadcast(player->getSock(), player->getParent(), "%s successfully identifies it!", player->upHeShe());
        player->printColor("You carefully study %P.\n",object.get());
        player->printColor("You manage to learn about %P!\n", object.get());
        player->checkImprove("identify", true);

        object->clearFlag(O_JUST_BOUGHT);

        player->lasttime[LT_IDENTIFY].ltime = t;
        player->lasttime[LT_IDENTIFY].interval = 45L;

        if(object->increase) {
            
            if(object->increase->type == SkillIncrease) {
                const SkillInfo* skill = gConfig->getSkill(object->increase->increase);
                if(!skill)
                    player->printColor("^rThe skill set on this object is not a valid skill.\n");
                else {
                    output = skill->getDisplayName();
                    player->printColor("%O can increase your knowledge of %s.\n", object.get(), output.c_str());
                }
            } else if(object->increase->type == LanguageIncrease) {
                int lang = toNum<int>(object->increase->increase);
                if(lang < 1 || lang > LANGUAGE_COUNT)
                    player->printColor("^rThe language set on this object is not a valid language.\n");
                else
                    player->printColor("%O can teach you %s.\n", object.get(), get_language_adj(lang-1));
            } else {
                player->printColor("^rThe increase type set on this object is not a valid increase type.\n");
            }

        } else if(object->getType() == ObjectType::WEAPON) {
            player->printColor("%O is a %s, with an average damage of %d.\n", object.get(), object->getTypeName().c_str(),
                  std::max(1, object->damage.average() + object->getAdjustment()));
        } else if(object->getType() == ObjectType::POISON) {
            player->printColor("%O is a poison.\n", object.get());
            player->print("It has a maximum duration of %d seconds.\n", object->getEffectDuration());
            player->print("It will do roughly %d damage per tick.\n", object->getEffectStrength());
        } else if(object->getType() == ObjectType::ARMOR && object->getWearflag() != HELD && object->getWearflag() != WIELD) {
            ac = object->getArmor();
            wear = object->getWearflag();

            if(wear > FINGER && wear <= FINGER8)
                wear = FINGER;
            if(wear == BODY) {
                if(ac <= 44)
                    desc = "weak" ;
                else if(ac > 44 && ac <= 84)
                    desc = "decent";
                else if(ac > 84 && ac <= 97)
                    desc = "admirable";
                else if(ac > 97 && ac <= 106)
                    desc = "splendid";
                else if(ac > 106 && ac <= 119)
                    desc = "excellent";
                else if(ac > 119 && ac <= 128)
                    desc = "exemplary";
                else if(ac > 128 && ac <= 141)
                    desc = "superb";
                else if(ac > 141)
                    desc = "amazing";
            } else if(wear == ARMS) {
                if(ac <= 13)
                    desc = "weak" ;
                else if(ac > 13 && ac <= 22)
                    desc = "decent";
                else if(ac > 22 && ac <= 26)
                    desc = "admirable";
                else if(ac > 26 && ac <= 48)
                    desc = "splendid";
                else if(ac > 48 && ac <= 62)
                    desc = "excellent";
                else if(ac > 62 && ac <= 81)
                    desc = "exemplary";
                else if(ac > 81 && ac <= 88)
                    desc = "superb";
                else if(ac > 88)
                    desc = "amazing";
            } else if(wear == LEGS) {
                if(ac <= 13)
                    desc = "weak" ;
                else if(ac > 13 && ac <= 22)
                    desc = "decent";
                else if(ac > 22 && ac <= 26)
                    desc = "admirable";
                else if(ac > 26 && ac <= 48)
                    desc = "splendid";
                else if(ac > 48 && ac <= 62)
                    desc = "excellent";
                else if(ac > 62 && ac <= 81)
                    desc = "exemplary";
                else if(ac > 81 && ac <= 88)
                    desc = "superb";
                else if(ac > 88)
                    desc = "amazing";
            } else if(wear == NECK) {
                if(ac <= 9)
                    desc = "weak" ;
                else if(ac > 9 && ac <= 18)
                    desc = "decent";
                else if(ac > 18 && ac <= 26)
                    desc = "admirable";
                else if(ac > 26 && ac <= 35)
                    desc = "splendid";
                else if(ac > 35 && ac <= 44)
                    desc = "excellent";
                else if(ac > 44 && ac <= 53)
                    desc = "exemplary";
                else if(ac > 53 && ac <= 62)
                    desc = "superb";
                else if(ac > 62)
                    desc = "amazing";
            } else if(wear == BELT) {
                if(ac <= 9)
                    desc = "weak" ;
                else if(ac > 9 && ac <= 18)
                    desc = "decent";
                else if(ac > 18 && ac <= 26)
                    desc = "admirable";
                else if(ac > 26 && ac <= 35)
                    desc = "splendid";
                else if(ac > 35 && ac <= 44)
                    desc = "excellent";
                else if(ac > 44 && ac <= 53)
                    desc = "exemplary";
                else if(ac > 53 && ac <= 62)
                    desc = "superb";
                else if(ac > 62)
                    desc = "amazing";
            } else if(wear == HANDS) {
                if(ac <= 9)
                    desc = "weak" ;
                else if(ac > 9 && ac <= 18)
                    desc = "decent";
                else if(ac > 18 && ac <= 26)
                    desc = "admirable";
                else if(ac > 26 && ac <= 35)
                    desc = "splendid";
                else if(ac > 35 && ac <= 44)
                    desc = "excellent";
                else if(ac > 44 && ac <= 53)
                    desc = "exemplary";
                else if(ac > 53 && ac <= 62)
                    desc = "superb";
                else if(ac > 62)
                    desc = "amazing";
            } else if(wear == HEAD) {
                if(ac <= 9)
                    desc = "weak" ;
                else if(ac > 9 && ac <= 18)
                    desc = "decent";
                else if(ac > 18 && ac <= 26)
                    desc = "admirable";
                else if(ac > 26 && ac <= 35)
                    desc = "splendid";
                else if(ac > 35 && ac <= 44)
                    desc = "excellent";
                else if(ac > 44 && ac <= 53)
                    desc = "exemplary";
                else if(ac > 53 && ac <= 62)
                    desc = "superb";
                else if(ac > 62)
                    desc = "amazing";
            } else if(wear == FEET) {
                if(ac <= 9)
                    desc = "weak" ;
                else if(ac > 9 && ac <= 18)
                    desc = "decent";
                else if(ac > 18 && ac <= 26)
                    desc = "admirable";
                else if(ac > 26 && ac <= 35)
                    desc = "splendid";
                else if(ac > 35 && ac <= 44)
                    desc = "excellent";
                else if(ac > 44 && ac <= 53)
                    desc = "exemplary";
                else if(ac > 53 && ac <= 62)
                    desc = "superb";
                else if(ac > 62)
                    desc = "amazing";
            } else if(wear == FINGER) {
                if(ac < 1)
                    desc = "no" ;
                else if(ac == 1)
                    desc = "decent";
                else if(ac == 9)
                    desc = "excellent";
                else if(ac == 13)
                    desc = "extraordinary";
                else if(ac == 18)
                    desc = "superb";
                else if(ac >= 22)
                    desc = "amazing";
            } else if(wear == SHIELD) {
                if(ac <= 18)
                    desc = "weak" ;
                else if(ac > 18 && ac <= 26)
                    desc = "decent";
                else if(ac > 26 && ac <= 40)
                    desc = "admirable";
                else if(ac > 40 && ac <= 53)
                    desc = "splendid";
                else if(ac > 53 && ac <= 62)
                    desc = "excellent";
                else if(ac > 62 && ac <= 71)
                    desc = "exemplary";
                else if(ac > 72 && ac <= 84)
                    desc = "superb";
                else if(ac > 84)
                    desc = "amazing";
            } else if(wear == FACE) {
                if(ac <= 9)
                    desc = "weak" ;
                else if(ac > 9 && ac <= 18)
                    desc = "decent";
                else if(ac > 18 && ac <= 26)
                    desc = "admirable";
                else if(ac > 26 && ac <= 35)
                    desc = "splendid";
                else if(ac > 35 && ac <= 44)
                    desc = "excellent";
                else if(ac > 44 && ac <= 53)
                    desc = "exemplary";
                else if(ac > 53 && ac <= 62)
                    desc = "superb";
                else if(ac > 62)
                    desc = "amazing";
            }

            player->printColor("%O is a %s.\n", object.get(), object->getTypeName().c_str());
            if(desc[0])
                player->printColor("It will offer %s protection for where it's worn.\n", desc.c_str());
        } else if(object->getType() == ObjectType::SONGSCROLL) {
            player->printColor("%O is a %s, enscribed with the Song of %s.\n",
                               object.get(), object->getTypeName().c_str(), get_song_name(object->getMagicpower() - 1));
        } else if(object->getType() == ObjectType::WAND || object->getType() == ObjectType::SCROLL || object->getType() == ObjectType::POTION) {
            player->printColor("%O is a %s, with the %s spell.\n", object.get(), object->getTypeName().c_str(),
                get_spell_name(object->getMagicpower()-1));
        } else if(object->getType() == ObjectType::KEY) {
            player->printColor("%O is a %s %s.\n", object.get(),
                  (object->getShotsCur() < 1 ? "broken" : (object->getShotsCur() > 2 ? "sturdy" : "weak")),
                               object->getTypeName().c_str());
        } else if(object->getType() == ObjectType::MONEY) {
            player->printColor("%O is a %s.\n", object.get(), object->getTypeName().c_str());
        } else if(object->getType() == ObjectType::CONTAINER) {
            player->printColor("%O is a %s.\n", object.get(), object->getTypeName().c_str());
            if(object->getSize())
                player->print("It can hold %s items.\n", getSizeName(object->getSize()).c_str());
        } else {
            player->printColor("%O is a %s.\n", object.get(), object->getTypeName().c_str());
        }

        if(object->flagIsSet(O_EQUIPPING_BESTOWS_EFFECT) && Effect::objectCanBestowEffect(object->getEffect())) {
            auto* effect = gConfig->getEffect(object->getEffect());
            if(effect)
                player->printColor("Equipping this item will give you the %s^x effect.\n", effect->getDisplay().c_str());
        }

        output = object->value.str();
        player->print("It is worth %s", output.c_str());
        if(object->getType() != ObjectType::CONTAINER && object->getType() != ObjectType::MONEY) {
            player->print(", and is ", object.get());
            if(object->getShotsCur() >= object->getShotsMax() * .99)
                player->print("brand new");
            else if(object->getShotsCur() >= object->getShotsMax() * .90)
                player->print("almost brand new");
            else if(object->getShotsCur() >= object->getShotsMax() * .75)
                player->print("in good condition");
            else if(object->getShotsCur() >= object->getShotsMax() * .50)
                player->print("almost half used up");
            else if(object->getShotsCur() >= object->getShotsMax() * .25)
                player->print("in fair condition");
            else if(object->getShotsCur() >= object->getShotsMax() * .10)
                player->print("in poor condition");
            else if(object->getShotsCur() == 0)
                player->print("broken or used up");
            else
                player->print("close to breaking");
        }
        player->print(".\n");
        return(0);
    }
}
