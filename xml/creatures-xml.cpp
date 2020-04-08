/*
 * creatures-xml.cpp
 *   Creatures xml
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

#include "config.hpp"
#include "creatures.hpp"
#include "mud.hpp"
#include "proto.hpp"
#include "raceData.hpp"
#include "rooms.hpp"
#include "server.hpp"
#include "socials.hpp"
#include "specials.hpp"
#include "xml.hpp"


int convertProf(Creature* player, Realm realm) {
    int skill = player->getLevel()*7 + player->getLevel()*3 * mprofic(player, realm) / 100 - 5;
    skill = MAX(0, skill);
    return(skill);
}

int Creature::readFromXml(xmlNodePtr rootNode, bool offline) {
    xmlNodePtr curNode;
    int i;
    CreatureClass c = CreatureClass::NONE;

    Player *pPlayer = getAsPlayer();
    Monster *mMonster = getAsMonster();

    if(mMonster) {
        mMonster->info.load(rootNode);
        mMonster->info.id = (short)xml::getIntProp(rootNode, "Num");
    }
    xml::copyPropToBString(version, rootNode, "Version");

    curNode = rootNode->children;
    // Start reading stuff in!

    while(curNode) {
        // Name will only be loaded for Monsters
        if(NODE_NAME(curNode, "Name")) setName(xml::getBString(curNode));
        else if(NODE_NAME(curNode, "Id")) setId(xml::getBString(curNode));
        else if(NODE_NAME(curNode, "Description")) xml::copyToBString(description, curNode);
        else if(NODE_NAME(curNode, "Keys")) {
            loadStringArray(curNode, key, CRT_KEY_LENGTH, "Key", 3);
        }
        else if(NODE_NAME(curNode, "MoveTypes")) {
            loadStringArray(curNode, movetype, CRT_MOVETYPE_LENGTH, "MoveType", 3);
        }
        else if(NODE_NAME(curNode, "Level")) setLevel(xml::toNum<unsigned short>(curNode), true);
        else if(NODE_NAME(curNode, "Type")) setType(xml::toNum<unsigned short>(curNode));
        else if(NODE_NAME(curNode, "RoomNum")&& getVersion() < "2.34" ) xml::copyToNum(currentLocation.room.id, curNode);
        else if(NODE_NAME(curNode, "Room")) currentLocation.room.load(curNode);

        else if(NODE_NAME(curNode, "Race")) setRace(xml::toNum<unsigned short>(curNode));
        else if(NODE_NAME(curNode, "Class")) c = xml::toNum<CreatureClass>(curNode);
        else if(NODE_NAME(curNode, "AttackPower")) setAttackPower(xml::toNum<unsigned int>(curNode));
        else if(NODE_NAME(curNode, "DefenseSkill")) {
            if(mMonster) {
                mMonster->setDefenseSkill(xml::toNum<int>(curNode));
            }
        }
        else if(NODE_NAME(curNode, "WeaponSkill")) {
            if(mMonster) {
                mMonster->setWeaponSkill(xml::toNum<int>(curNode));
            }
        }
        else if(NODE_NAME(curNode, "Class2")) {
            if(pPlayer) {
                pPlayer->setSecondClass(xml::toNum<CreatureClass>(curNode));
            } else if(mMonster) {
                // TODO: Dom: for compatability, remove when possible
                mMonster->setMobTrade(xml::toNum<unsigned short>(curNode));
            }
        }
        else if(NODE_NAME(curNode, "Alignment")) setAlignment(xml::toNum<short>(curNode));
        else if(NODE_NAME(curNode, "Armor")) setArmor(xml::toNum<unsigned int>(curNode));
        else if(NODE_NAME(curNode, "Experience")) setExperience(xml::toNum<unsigned long>(curNode));

        else if(NODE_NAME(curNode, "Deity")) setDeity(xml::toNum<unsigned short>(curNode));

        else if(NODE_NAME(curNode, "Clan")) setClan(xml::toNum<unsigned short>(curNode));
        else if(NODE_NAME(curNode, "PoisonDuration")) setPoisonDuration(xml::toNum<unsigned short>(curNode));
        else if(NODE_NAME(curNode, "PoisonDamage")) setPoisonDamage(xml::toNum<unsigned short>(curNode));
        else if(NODE_NAME(curNode, "CurrentLanguage")) xml::copyToNum(current_language, curNode);
        else if(NODE_NAME(curNode, "Coins")) coins.load(curNode);
        else if(NODE_NAME(curNode, "Realms")) {
            xml::loadNumArray<unsigned long>(curNode, realm, "Realm", MAX_REALM-1);
        }
        else if(NODE_NAME(curNode, "Proficiencies")) {
            long proficiency[6];
            zero(proficiency, sizeof(proficiency));
            xml::loadNumArray<long>(curNode, proficiency, "Proficiency", 6);

            // monster jail rooms shouldn't be stored here
            if(mMonster && mMonster->getVersion() < "2.42b") {
                mMonster->setCastChance((short)proficiency[0]);
                mMonster->rescue[0].setArea("misc");
                mMonster->rescue[0].id = (short)proficiency[1];
                mMonster->rescue[1].setArea("misc");
                mMonster->rescue[1].id = (short)proficiency[2];
                mMonster->setMaxLevel((unsigned short)proficiency[3]);
                mMonster->jail.setArea("misc");
                mMonster->jail.id = (short)proficiency[4];
            }
        }
        else if(NODE_NAME(curNode, "Dice")) damage.load(curNode);
        else if(NODE_NAME(curNode, "Skills")) {
            loadSkills(curNode);
        }
        else if(NODE_NAME(curNode, "Factions")) {
            loadFactions(curNode);
        }
        else if(NODE_NAME(curNode, "Effects")) {
            effects.load(curNode, this);
        }
        else if(NODE_NAME(curNode, "SpecialAttacks")) {
            loadAttacks(curNode);
        }
        else if(NODE_NAME(curNode, "Stats")) {
            loadStats(curNode);
        }
        else if(NODE_NAME(curNode, "Flags")) {
            // Clear flags before loading incase we're loading a reference creature
            for(i=0; i<CRT_FLAG_ARRAY_SIZE; i++)
                flags[i] = 0;
            loadBits(curNode, flags);
        }
        else if(NODE_NAME(curNode, "Spells")) {
            loadBits(curNode, spells);
        }
        else if(NODE_NAME(curNode, "Quests")) {
            loadBits(curNode, old_quests);
        }
        else if(NODE_NAME(curNode, "Languages")) {
            loadBits(curNode, languages);
        }
        else if(NODE_NAME(curNode, "DailyTimers")) {
            loadDailys(curNode, daily);
        }
        else if(NODE_NAME(curNode, "LastTimes")) {
            loadLastTimes(curNode, lasttime);
        }
        else if(NODE_NAME(curNode, "SavingThrows")) {
            loadSavingThrows(curNode, saves);
        }
        else if(NODE_NAME(curNode, "Inventory")) {
            readObjects(curNode, offline);
        }
        else if(NODE_NAME(curNode, "Pets")) {
            readCreatures(curNode, offline);
        }
        else if(NODE_NAME(curNode, "AreaRoom")) gServer->areaInit(this, curNode);
        else if(NODE_NAME(curNode, "Size")) setSize(whatSize(xml::toNum<int>(curNode)));
        else if(NODE_NAME(curNode, "Hooks")) hooks.load(curNode);


            // code for only players
        else if(pPlayer) pPlayer->readXml(curNode, offline);

            // code for only monsters
        else if(mMonster) mMonster->readXml(curNode, offline);

        curNode = curNode->next;
    }

    // run this here so effects are added properly
    setClass(c);

    convertOldEffects();

    if(getVersion() < "2.46l") {
        upgradeStats();
    }

    if(isPlayer()) {
        if(getVersion() < "2.47b") {
            pPlayer->recordLevelInfo();
        }
    }
    if(isPlayer()) {
        if(getVersion() < "2.47b") {
#define P_OLD_MISTED                55       // Player is in mist form
            if(flagIsSet(P_OLD_MISTED)) {
                addEffect("mist", -1);
                clearFlag(P_OLD_MISTED);
            }
#define P_OLD_INCOGNITO                 104      // DM/CT is incognito
            if(flagIsSet(P_OLD_INCOGNITO)) {
                addEffect("incognito", -1);
                clearFlag(P_OLD_INCOGNITO);
            }
        }
        if(getVersion() < "2.47a") {
            // Update weapon skills
            SkillMap::iterator skIt;
            for(skIt = skills.begin() ; skIt != skills.end() ; ) {
                Skill* skill = (*skIt++).second;
                SkillInfo* parentSkill = skill->getSkillInfo();
                if(!parentSkill)
                    continue;
                bstring group = parentSkill->getGroup();
                if(group.left(7) == "weapons" && group.length() > 6) {
                    bstring weaponSkillName = group.right(group.length() - 8);
                    Skill* weaponSkill = getSkill(weaponSkillName, false);
                    if(!weaponSkill) {
                        addSkill(weaponSkillName, skill->getGained());
                    } else {
                        if(weaponSkill->getGained() < skill->getGained()) {
                            weaponSkill->setGained(skill->getGained());
                        }
                    }
                }

            }
        }


        if(getVersion() < "2.46k" && knowsSkill("endurance")) {
            remSkill("endurance");
#define P_RUNNING_OLD 56
            clearFlag(P_RUNNING_OLD);
        }

        if(getVersion() < "2.43" && getClass() !=  CreatureClass::BERSERKER) {
            int skill = level;
            if(isPureCaster() || isHybridCaster() || isStaff()) {
                skill *= 10;
            } else {
                skill *= 7;
            }
            skill -= 5;
            skill = MAX(0, skill);

            addSkill("abjuration", skill);
            addSkill("conjuration", skill);
            addSkill("divination", skill);
            addSkill("enchantment", skill);
            addSkill("evocation", skill);
            addSkill("illusion", skill);
            addSkill("necromancy", skill);
            addSkill("translocation", skill);
            addSkill("transmutation", skill);

            addSkill("fire", convertProf(this, FIRE));
            addSkill("water", convertProf(this, WATER));
            addSkill("earth", convertProf(this, EARTH));
            addSkill("air", convertProf(this, WIND));
            addSkill("cold", convertProf(this, COLD));
            addSkill("electric", convertProf(this, ELEC));
        }
        if(getVersion() < "2.45c" && getCastingType() == Divine) {
            int skill = level;
            if(isPureCaster() || isHybridCaster() || isStaff()) {
                skill *= 10;
            } else {
                skill *= 7;
            }
            skill -= 5;
            skill = MAX(0, skill);

            addSkill("healing", skill);
            addSkill("destruction", (int)getSkillGained("evocation"));
            addSkill("evil", (int)getSkillGained("necromancy"));
            addSkill("knowledge", (int)getSkillGained("divination"));
            addSkill("protection", (int)getSkillGained("abjuration"));
            addSkill("augmentation", (int)getSkillGained("transmutation"));
            addSkill("travel", (int)getSkillGained("translocation"));
            addSkill("creation", (int)getSkillGained("conjuration"));
            addSkill("trickery", (int)getSkillGained("illusion"));
            addSkill("good", skill);
            addSkill("nature", skill);

            remSkill("abjuration");
            remSkill("conjuration");
            remSkill("divination");
            remSkill("enchantment");
            remSkill("evocation");
            remSkill("illusion");
            remSkill("necromancy");
            remSkill("translocation");
            remSkill("transmutation");
        }
        if(getClass() == CreatureClass::LICH && getVersion() < "2.43b" && !spellIsKnown(S_SAP_LIFE))
            learnSpell(S_SAP_LIFE);
    }

    setVersion();


    if(size == NO_SIZE && race)
        size = gConfig->getRace(race)->getSize();

    escapeText();
    return(0);
}


//*********************************************************************
//                      loadFaction
//*********************************************************************

bool Creature::loadFaction(xmlNodePtr rootNode) {
    bstring name = "";
    int         regard=0;
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "Name")) {
            xml::copyToBString(name, curNode);
        } else if(NODE_NAME(curNode, "Regard")) {
            xml::copyToNum(regard, curNode);
        }
        curNode = curNode->next;
    }
    // players; load faction even if 0
    if(!name.empty() && (regard != 0 || isPlayer())) {
        factions[name] = regard;
        return(true);
    }
    return(false);
}

//*********************************************************************
//                      loadSkills
//*********************************************************************

void Creature::loadSkills(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "Skill")) {
            try {
                auto *skill = new Skill(curNode);
                skills.insert(SkillMap::value_type(skill->getName(), skill));
            } catch(...) {
                std::clog << "Error loading skill for " << getName() << std::endl;
            }
        }
        curNode = curNode->next;
    }
}

//*********************************************************************
//                      loadFactions
//*********************************************************************

void Creature::loadFactions(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "Faction")) {
            loadFaction(curNode);
        }
        curNode = curNode->next;
    }
}


//*********************************************************************
//                      readCreatures
//*********************************************************************
// Reads in creatures and crts refs and adds them to the parent

void MudObject::readCreatures(xmlNodePtr curNode, bool offline) {
    xmlNodePtr childNode = curNode->children;
    Monster *mob=nullptr;
    CatRef  cr;

    Creature* cParent = getAsCreature();
//  Monster* mParent = parent->getMonster();
//  Player* pParent = parent->getPlayer();
    UniqueRoom* rParent = getAsUniqueRoom();
    Object* oParent = getAsObject();

    while(childNode) {
        mob = nullptr;
        if(NODE_NAME( childNode , "Creature")) {
            // If it's a full creature, read it in
            mob = new Monster;
            if(!mob)
                merror("loadCreaturesXml", FATAL);
            mob->readFromXml(childNode);
        } else if(NODE_NAME( childNode, "CrtRef")) {
            // If it's an creature reference, we want to first load the parent creature
            // and then use readCreatureXml to update any fields that may have changed
            cr.load(childNode);
            cr.id = xml::getIntProp( childNode, "Num" );

            if(loadMonster(cr, &mob)) {
                mob->readFromXml(childNode);
            } else {
                //printf("Unable to load creature %d\n", num);
            }
        }
        if(mob != nullptr) {
            // Add it to the appropriate parent
            if(cParent) {
                // If we have a creature parent, we must be a pet
                cParent->addPet(mob);
            } else if(rParent) {
                mob->addToRoom(rParent, 0);
            } else if(oParent) {
                // TODO: Not currently implemented
            }
        }
        childNode = childNode->next;
    }
}


//*********************************************************************
//                      loadCrLastTime
//*********************************************************************
// Loads a single crlasttime into the given lasttime

void loadCrLastTime(xmlNodePtr curNode, struct crlasttime* pCrLastTime) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Interval")) xml::copyToNum(pCrLastTime->interval, childNode);
        else if(NODE_NAME(childNode, "LastTime")) xml::copyToNum(pCrLastTime->ltime, childNode);
        else if(NODE_NAME(childNode, "Misc")) pCrLastTime->cr.load(childNode);

        childNode = childNode->next;
    }
}

//*********************************************************************
//                      loadStats
//*********************************************************************
// Loads all stats (strength, intelligence, hp, etc) into the given creature

void Creature::loadStats(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    bstring statName = "";

    while(childNode) {
        if(NODE_NAME(childNode, "Stat")) {
            statName = xml::getProp(childNode, "Name");

            if(statName.empty())
                continue;

            if(statName == "Strength") strength.load(childNode, statName);
            else if(statName == "Dexterity") dexterity.load(childNode, statName);
            else if(statName == "Constitution") constitution.load(childNode, statName);
            else if(statName == "Intelligence") intelligence.load(childNode, statName);
            else if(statName == "Piety") piety.load(childNode, statName);
            else if(statName == "Hp") hp.load(childNode, statName);
            else if(statName == "Mp") mp.load(childNode, statName);
            else if(statName == "Focus") {
                Player* player = getAsPlayer();
                if(player)
                    player->focus.load(childNode, statName);
            }
        }

        childNode = childNode->next;
    }
}

//*********************************************************************
//                      loadAttacks
//*********************************************************************

void Creature::loadAttacks(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "SpecialAttack")) {
            auto* newAttack = new SpecialAttack(curNode);
            specials.push_back(newAttack);
        }
        curNode = curNode->next;
    }
}



//*********************************************************************
//                      loadDailys
//*********************************************************************
// This function will load all daily timers into the given dailys array

void loadDailys(xmlNodePtr curNode, struct daily* pDailys) {
    xmlNodePtr childNode = curNode->children;
    int i=0;

    while(childNode) {
        if(NODE_NAME(childNode, "Daily")) {
            i = xml::getIntProp(childNode, "Num");
            // TODO: Sanity check i
            loadDaily(childNode, &pDailys[i]);
        }
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      loadDaily
//*********************************************************************
// This loads an individual daily into the provided pointer

void loadDaily(xmlNodePtr curNode, struct daily* pDaily) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Max")) xml::copyToNum(pDaily->max, childNode);
        else if(NODE_NAME(childNode, "Current")) xml::copyToNum(pDaily->cur, childNode);
        else if(NODE_NAME(childNode, "LastTime")) xml::copyToNum(pDaily->ltime, childNode);

        childNode = childNode->next;
    }
}



//*********************************************************************
//                      loadSavingThrows
//*********************************************************************
// Loads all saving throws into the given array of saving throws

void loadSavingThrows(xmlNodePtr curNode, struct saves* pSavingThrows) {
    xmlNodePtr childNode = curNode->children;
    int i=0;

    while(childNode) {
        if(NODE_NAME(childNode, "SavingThrow")) {
            i = xml::getIntProp(childNode, "Num");
            // TODO: Sanity check i
            loadSavingThrow(childNode, &pSavingThrows[i]);
        }
        childNode = childNode->next;
    }
}

//*********************************************************************
//                      loadSavingThrows
//*********************************************************************
// Loads an individual saving throw

void loadSavingThrow(xmlNodePtr curNode, struct saves* pSavingThrow) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Chance")) xml::copyToNum(pSavingThrow->chance, childNode);
        else if(NODE_NAME(childNode, "Gained")) xml::copyToNum(pSavingThrow->gained, childNode);
        else if(NODE_NAME(childNode, "Misc")) xml::copyToNum(pSavingThrow->misc, childNode);

        childNode = childNode->next;
    }
}




