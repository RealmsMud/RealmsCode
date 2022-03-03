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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <list>                                     // for list, operator==
#include <map>                                      // for operator==, map
#include <ostream>                                  // for basic_ostream::op...
#include <set>                                      // for set
#include <string>                                   // for allocator, string
#include <utility>                                  // for pair

#include "area.hpp"                                 // for MapMarker
#include "catRef.hpp"                               // for CatRef
#include "config.hpp"                               // for Config, gConfig
#include "dice.hpp"                                 // for Dice
#include "enums/loadType.hpp"                       // for LoadType, LoadTyp...
#include "flags.hpp"                                // for MAX_MONSTER_FLAGS
#include "global.hpp"                               // for CreatureClass
#include "hooks.hpp"                                // for Hooks
#include "lasttime.hpp"                             // for crlasttime, lasttime
#include "location.hpp"                             // for Location
#include "magic.hpp"                                // for MAXSPELL, S_SAP_LIFE
#include "money.hpp"                                // for Money
#include "mud.hpp"                                  // for TOTAL_LTS, DAILYLAST
#include "mudObjects/areaRooms.hpp"                 // for AreaRoom
#include "mudObjects/container.hpp"                 // for MonsterSet
#include "mudObjects/creatures.hpp"                 // for Creature, SkillMap
#include "mudObjects/monsters.hpp"                  // for Monster
#include "mudObjects/mudObject.hpp"                 // for MudObject
#include "mudObjects/players.hpp"                   // for Player
#include "mudObjects/rooms.hpp"                     // for NUM_PERM_SLOTS
#include "mudObjects/uniqueRooms.hpp"               // for UniqueRoom
#include "os.hpp"                                   // for merror
#include "proto.hpp"                                // for mprofic, zero
#include "raceData.hpp"                             // for RaceData
#include "realm.hpp"                                // for MAX_REALM, COLD
#include "server.hpp"                               // for Server, gServer
#include "size.hpp"                                 // for whatSize, NO_SIZE
#include "skills.hpp"                               // for Skill, SkillInfo
#include "specials.hpp"                             // for SpecialAttack
#include "stats.hpp"                                // for Stat
#include "structs.hpp"                              // for daily, saves
#include "utils.hpp"                                // for MAX
#include "xml.hpp"                                  // for newStringChild

class Object;

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
    xml::copyPropToString(version, rootNode, "Version");

    curNode = rootNode->children;
    // Start reading stuff in!

    while(curNode) {
        // Name will only be loaded for Monsters
        if(NODE_NAME(curNode, "Name")) setName(xml::getString(curNode));
        else if(NODE_NAME(curNode, "Id")) setId(xml::getString(curNode));
        else if(NODE_NAME(curNode, "Description")) xml::copyToString(description, curNode);
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
        else if(NODE_NAME(curNode, "Class")) c = static_cast<CreatureClass>(xml::toNum<short>(curNode));
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
                pPlayer->setSecondClass(static_cast<CreatureClass>(xml::toNum<short>(curNode)));
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
            for (auto const& [skillId, skill] : skills) {
                const SkillInfo* parentSkill = skill->getSkillInfo();
                if(!parentSkill)
                    continue;
                std::string skillGroup = parentSkill->getGroup();
                if(skillGroup.starts_with("weapons") && skillGroup.length() > 8) {
                    std::string weaponSkillName = skillGroup.substr(8);
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

        if(getVersion() < "2.47l") {
            if(getClass() == CreatureClass::FIGHTER ||
            getClass() == CreatureClass::PALADIN ||
            getClass() == CreatureClass::DEATHKNIGHT ||
            getClass() == CreatureClass::RANGER ||
            (getClass() == CreatureClass::CLERIC && (getDeity() == ENOCH ||
            getDeity() == ARES || getDeity() == GRADIUS)))
            {
                int initialSkill = (int)getSkillGained("chain");
                initialSkill = MAX(1,initialSkill-(initialSkill/10));
                addSkill("scale",initialSkill);
                addSkill("ring",initialSkill);
            }
        }

         if(isPlayer()) {   

            if(getVersion() < "2.52") {
                #define P_NO_LONG_DESCRIPTION_OLD      4
                #define P_NO_SHORT_DESCRIPTION_OLD     5
                clearFlag(P_NO_LONG_DESCRIPTION_OLD);
                clearFlag(P_NO_SHORT_DESCRIPTION_OLD);
            }
             if(getVersion() < "2.52b") {
                #define P_CAN_PROXY_OLD 156
                clearFlag(P_CAN_PROXY_OLD);
             }
        }
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
    std::string name;
    int         regard=0;
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
        if(NODE_NAME(curNode, "Name")) {
            xml::copyToString(name, curNode);
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
    std::string statName = "";

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



//*********************************************************************
//                      saveToXml
//*********************************************************************
// This will save the entire creature with anything non zero to the given
// node which should be a creature or player node
//
// Keep in mind, we're saving a copy of the creature - so make sure the Copy
// constructor has copied whatever you're expecting to save

int Creature::saveToXml(xmlNodePtr rootNode, int permOnly, LoadType saveType, bool saveID) const {
//  xmlNodePtr  rootNode;
    xmlNodePtr      curNode;
    xmlNodePtr      childNode;
    int i;

    if(getName()[0] == '\0' || rootNode == nullptr)
        return(-1);

    const Player    *pPlayer = getAsConstPlayer();
    const Monster   *mMonster = getAsConstMonster();

    if(pPlayer) {
        xml::newProp(rootNode, "Name", pPlayer->getName());
        xml::newProp(rootNode, "Password", pPlayer->getPassword());
        xml::newNumProp(rootNode, "LastLogin", pPlayer->getLastLogin());
    } else if(mMonster) {
        // Saved for LoadType::LS_REF and LoadType::LS_FULL
        xml::newNumProp(rootNode, "Num", mMonster->info.id);
        xml::saveNonNullString(rootNode, "Name", mMonster->getName());
        xml::newProp(rootNode, "Area", mMonster->info.area);
    }

    if(saveID)
        xml::saveNonNullString(rootNode, "Id", getId());
    else
        xml::newProp(rootNode, "ID", "-1");

    // For the future, when we change things, can read them in based on the version they
    // were saved in before or do modifications based on the new version
    xml::newProp(rootNode, "Version", gConfig->getVersion());

    if(pPlayer) {
        if(pPlayer->inAreaRoom()) {
            curNode = xml::newStringChild(rootNode, "AreaRoom");
            pPlayer->getConstAreaRoomParent()->mapmarker.save(curNode);
            //pPlayer->currentLocation.mapmarker.save(curNode);
        } else
            pPlayer->currentLocation.room.save(rootNode, "Room", true);
    }

    // Saved for LoadType::LS_REF and LoadType::LS_FULL
    xml::saveNonZeroNum(rootNode, "Race", race);
    xml::saveNonZeroNum(rootNode, "Class", static_cast<int>(cClass));
    if(pPlayer) {
        xml::saveNonZeroNum(rootNode, "Class2", static_cast<int>(pPlayer->getSecondClass()));
    } else if(mMonster) {
        // TODO: Dom: for compatability, remove when possible
        xml::saveNonZeroNum(rootNode, "Class2", mMonster->getMobTrade());
        xml::saveNonZeroNum(rootNode, "MobTrade", mMonster->getMobTrade());
    }

    xml::saveNonZeroNum(rootNode, "Level", level);
    xml::saveNonZeroNum(rootNode, "Type", (int)type);
    xml::saveNonZeroNum(rootNode, "Experience", experience);
    coins.save("Coins", rootNode);
    xml::saveNonZeroNum(rootNode, "Alignment", alignment);

    curNode = xml::newStringChild(rootNode, "Stats");
    strength.save(curNode, "Strength");
    dexterity.save(curNode, "Dexterity");
    constitution.save(curNode, "Constitution");
    intelligence.save(curNode, "Intelligence");
    piety.save(curNode, "Piety");
    hp.save(curNode, "Hp");
    mp.save(curNode, "Mp");

    if(pPlayer)
        pPlayer->focus.save(curNode, "Focus");

    // Only saved for LoadType::LS_FULL saves, or player saves
    if(saveType == LoadType::LS_FULL || pPlayer) {
        xml::saveNonNullString(rootNode, "Description", description);

        // Keys
        curNode = xml::newStringChild(rootNode, "Keys");
        for(i=0; i<3; i++) {
            if(key[i][0] == 0)
                continue;
            childNode = xml::newStringChild(curNode, "Key", key[i]);
            xml::newNumProp(childNode, "Num", i);
        }

        // MoveTypes
        curNode = xml::newStringChild(rootNode, "MoveTypes");
        for(i=0; i<3; i++) {
            if(movetype[i][0] == 0)
                continue;
            childNode = xml::newStringChild(curNode, "MoveType", movetype[i]);
            xml::newNumProp(childNode, "Num", i);
        }


        xml::saveNonZeroNum(rootNode, "Armor", armor);
        xml::saveNonZeroNum(rootNode, "Deity", deity);


        saveULongArray(rootNode, "Realms", "Realm", realm, MAX_REALM-1);
        saveLongArray(rootNode, "Proficiencies", "Proficiency", proficiency, 6);

        damage.save(rootNode, "Dice");

        xml::saveNonZeroNum(rootNode, "Clan", clan);
        xml::saveNonZeroNum(rootNode, "PoisonDuration", poison_dur);
        xml::saveNonZeroNum(rootNode, "PoisonDamage", poison_dmg);
        xml::saveNonZeroNum(rootNode, "Size", size);

        if(pPlayer) pPlayer->saveXml(rootNode);
        else if(mMonster) mMonster->saveXml(rootNode);

        saveFactions(rootNode);
        saveSkills(rootNode);

        hooks.save(rootNode, "Hooks");

        effects.save(rootNode, "Effects");
        saveAttacks(rootNode);

        if(!minions.empty()) {
            curNode = xml::newStringChild(rootNode, "Minions");
            std::list<std::string>::const_iterator mIt;
            for(mIt = minions.begin() ; mIt != minions.end() ; mIt++) {
                childNode = xml::newStringChild(curNode, "Minion", (*mIt));
            }
        }

        //      saveShortIntArray(rootNode, "Factions", "Faction", faction, MAX_FACTION);

        // Save dailys
        curNode = xml::newStringChild(rootNode, "DailyTimers");
        for(i=0; i<DAILYLAST+1; i++)
            saveDaily(curNode, i, daily[i]);

        // Save saving throws
        curNode = xml::newStringChild(rootNode, "SavingThrows");
        for(i=0; i<MAX_SAVE; i++)
            saveSavingThrow(curNode, i, saves[i]);

        // Perhaps change this into saveInt/CharArray
        saveBits(rootNode, "Spells", MAXSPELL, spells);
        saveBits(rootNode, "Quests", MAXSPELL, old_quests);
        xml::saveNonZeroNum(rootNode, "CurrentLanguage", current_language);
        saveBits(rootNode, "Languages", LANGUAGE_COUNT, languages);
    }

    // Saved for LoadType::LS_FULL and LoadType::LS_REF
    saveBits(rootNode, "Flags", pPlayer ? MAX_PLAYER_FLAGS : MAX_MONSTER_FLAGS, flags);

    // Save lasttimes
    for(i=0; i<TOTAL_LTS; i++) {
        // this nested loop means we won't create an xml node if we don't have to
        if(lasttime[i].interval || lasttime[i].ltime || lasttime[i].misc) {
            curNode = xml::newStringChild(rootNode, "LastTimes");
            for(; i<TOTAL_LTS; i++)
                saveLastTime(curNode, i, lasttime[i]);
        }
    }

    curNode = xml::newStringChild(rootNode, "Inventory");
    saveObjectsXml(curNode, objects, permOnly);

    // We want quests saved after inventory so when they are loaded we can calculate
    // if the quest is complete or not and store it in the appropriate variables
    if(pPlayer)
        pPlayer->saveQuests(rootNode);

    return(0);
}



//*********************************************************************
//                      saveFactions
//*********************************************************************

void Creature::saveFactions(xmlNodePtr rootNode) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "Factions");
    xmlNodePtr factionNode;
    std::map<std::string, long>::const_iterator fIt;
    for(fIt = factions.begin() ; fIt != factions.end() ; fIt++) {
        factionNode = xml::newStringChild(curNode, "Faction");
        xml::newStringChild(factionNode, "Name", (*fIt).first);
        xml::newNumChild(factionNode, "Regard", (*fIt).second);
    }
}

//*********************************************************************
//                      saveSkills
//*********************************************************************

void Creature::saveSkills(xmlNodePtr rootNode) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "Skills");
    std::map<std::string, Skill*>::const_iterator sIt;
    for(sIt = skills.begin() ; sIt != skills.end() ; sIt++) {
        (*sIt).second->save(curNode);
    }
}



//*********************************************************************
//                      saveAttacks
//*********************************************************************

void Creature::saveAttacks(xmlNodePtr rootNode) const {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "SpecialAttacks");
    std::list<SpecialAttack*>::const_iterator eIt;
    for(eIt = specials.begin() ; eIt != specials.end() ; eIt++) {
        (*eIt)->save(curNode);
    }
}


//*********************************************************************
//                      saveCreaturesXml
//*********************************************************************

int saveCreaturesXml(xmlNodePtr parentNode, const MonsterSet& set, int permOnly) {
    xmlNodePtr curNode;
    for(const Monster* mons : set) {
        if( mons && mons->isMonster() &&
            (   (permOnly == ALLITEMS && !mons->isPet()) ||
                (permOnly == PERMONLY && mons->flagIsSet(M_PERMENANT_MONSTER))
            ) )
        {
            if(mons->flagIsSet(M_SAVE_FULL)) {
                // Save a fully copy of the mob to the node
                curNode = xml::newStringChild(parentNode, "Creature");
                mons->saveToXml(curNode, permOnly, LoadType::LS_FULL);
            } else {
                // Just save a reference
                curNode = xml::newStringChild(parentNode, "CrtRef");
                mons->saveToXml(curNode, permOnly, LoadType::LS_REF);
            }
        }

    }
    return(0);
}



//*********************************************************************
//                      savePets
//*********************************************************************

void Creature::savePets(xmlNodePtr parentNode) const {
    xmlNodePtr curNode;

    for(const Monster* pet : pets) {
        // Only save pets, not creatures just following
        if(!pet->isPet()) continue;
        curNode = xml::newStringChild(parentNode, "Creature");
        pet->saveToXml(curNode, ALLITEMS, LoadType::LS_FULL);
    }
}


//*********************************************************************
//                      saveDaily
//*********************************************************************

xmlNodePtr saveDaily(xmlNodePtr parentNode, int i, struct daily pDaily) {
    // Avoid writing un-used daily timers
    if(pDaily.max == 0 && pDaily.cur == 0 && pDaily.ltime == 0)
        return(nullptr);

    xmlNodePtr curNode = xml::newStringChild(parentNode, "Daily");
    xml::newNumProp(curNode, "Num", i);

    xml::newNumChild(curNode, "Max", pDaily.max);
    xml::newNumChild(curNode, "Current", pDaily.cur);
    xml::newNumChild(curNode, "LastTime", pDaily.ltime);
    return(curNode);
}

//*********************************************************************
//                      saveCrLastTime
//*********************************************************************

xmlNodePtr saveCrLastTime(xmlNodePtr parentNode, int i, const struct crlasttime& pCrLastTime) {
    // Avoid writing un-used last times
    if(!pCrLastTime.interval && !pCrLastTime.ltime && !pCrLastTime.cr.id)
        return(nullptr);

    if(i < 0 || i >= NUM_PERM_SLOTS)
        return(nullptr);

    xmlNodePtr curNode = xml::newStringChild(parentNode, "LastTime");
    xml::newNumProp(curNode, "Num", i);

    xml::newNumChild(curNode, "Interval", pCrLastTime.interval);
    xml::newNumChild(curNode, "LastTime", pCrLastTime.ltime);
    pCrLastTime.cr.save(curNode, "Misc", false);
    return(curNode);
}


//*********************************************************************
//                      saveSavingThrow
//*********************************************************************

xmlNodePtr saveSavingThrow(xmlNodePtr parentNode, int i, struct saves pSavingThrow) {
    xmlNodePtr curNode = xml::newStringChild(parentNode, "SavingThrow");
    xml::newNumProp(curNode, "Num", i);

    xml::newNumChild(curNode, "Chance", pSavingThrow.chance);
    xml::newNumChild(curNode, "Gained", pSavingThrow.gained);
    xml::newNumChild(curNode, "Misc", pSavingThrow.misc);
    return(curNode);
}


