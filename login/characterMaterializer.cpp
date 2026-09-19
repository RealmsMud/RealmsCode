/*
 * characterMaterializer.cpp
 *   Build a level-1 Player from a CharacterDraft. See characterMaterializer.hpp.
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

#include <algorithm>
#include <ctime>
#include <list>
#include <map>
#include <memory>
#include <string>

#include "account.hpp"
#include "config.hpp"
#include "flags.hpp"
#include "global.hpp"
#include "login.hpp"            // Create::addStartingItem / addStartingWeapon
#include "login/characterCreationService.hpp"   // Create::computeFinalStats, classForDraft, kStatCount
#include "login/characterDraft.hpp"
#include "login/characterMaterializer.hpp"
#include "magic.hpp"
#include "mud.hpp"
#include "mudObjects/players.hpp"
#include "paths.hpp"            // Path::Player, fs
#include "playerClass.hpp"
#include "raceData.hpp"
#include "skillGain.hpp"
#include "skills.hpp"
#include "stats.hpp"
#include "structs.hpp"
// startlocs.hpp intentionally not included; initialBind is forward-declared below.

// from areas/startlocs.cpp -- full first-time bind (room, flags, starting guide)
void initialBind(std::shared_ptr<Player> player, const std::string& str);

namespace {
    // The five primary stats the draft carries, in CrtStats order.
    constexpr std::array<int, Create::kStatCount> kStats{STR, DEX, CON, INT, PTY};

    Stat& stat(const std::shared_ptr<Player>& player, int which) {
        switch(which) {
            case STR: return player->strength;
            case DEX: return player->dexterity;
            case CON: return player->constitution;
            case INT: return player->intelligence;
            default:  return player->piety;   // PTY
        }
    }

    // Menu-picked weapon skills start at 1; the race/class auto-grants start at their
    // configured gained value. The draft carries names only, so re-read the tables here.
    // Race wins on overlap, matching the old grant order (class skipped known skills).
    std::map<std::string, int> initialWeaponSkillGains(const CharacterDraft& draft, const RaceData* race) {
        std::map<std::string, int> gains;
        const auto collect = [&](std::list<SkillGain*>::const_iterator begin,
                                 std::list<SkillGain*>::const_iterator end, bool classMode) {
            for(auto it = begin; it != end; ++it) {
                SkillGain* sGain = *it;
                if(!sGain || sGain->getName().empty())
                    continue;
                const SkillInfo* skill = gConfig->getSkill(sGain->getName());
                if(!skill || !skill->getGroup().starts_with("weapons"))
                    continue;
                if(classMode && draft.deity && !sGain->deityIsAllowed(draft.deity))
                    continue;
                gains.emplace(sGain->getName(), sGain->getGained());
            }
        };
        collect(race->getSkillBegin(), race->getSkillEnd(), false);
        if(PlayerClass* pClass = Create::classForDraft(draft))
            collect(pClass->getSkillBegin(), pClass->getSkillEnd(), true);
        return gains;
    }
}

static void applyDraftChoices(const std::shared_ptr<Player>& player, const CharacterDraft& draft) {
    player->setRace(draft.race);
    const RaceData* race = gConfig->getRace(draft.race);
    player->setSize(race->getSize());
    player->initLanguages();
    for(const auto& effect : race->effects)
        player->addPermEffect(effect);

    player->setSex(static_cast<Sex>(draft.sex));
    player->setClass(draft.cClass);
    player->setSecondClass(draft.secondClass);
    player->setDeity(draft.deity);

    if(!draft.startLocation.empty())
        initialBind(player, draft.startLocation);

    const std::array<int, Create::kStatCount> finalStats = Create::computeFinalStats(draft, race);
    for(size_t i = 0; i < kStats.size(); i++)
        stat(player, kStats[i]).setInitial(finalStats[i]);

    const std::map<std::string, int> gains = initialWeaponSkillGains(draft, race);
    for(const auto& weapon : draft.weaponSkills) {
        const auto gain = gains.find(weapon);
        player->addSkill(weapon, gain != gains.end() ? gain->second : 1);
        Create::addStartingWeapon(player, weapon);
    }
}

// does not persist, register, or wire a socket
static void finishCharacter(const std::shared_ptr<Player>& player, const std::string& name, const Account& account) {
    const long t = time(nullptr);
    player->setBirthday();
    player->setCreated();
    player->setName(name);

    player->setAccountName(account.getName());
    player->setPassword("");                 // account handles auth
    player->setFlag(P_PASSWORD_CURRENT);

    player->lasttime[LT_AGE].interval = 0;
    for(int i = 0; i < MAX_LT; i++)
        player->lasttime[i].ltime = t;
    // bank interest accrues from lastLogin; leaving it at 0 would pay out since the epoch
    player->setLastLogin(t);

    // con affects a level-1's hp, so adjust before leveling
    player->adjustStats();
    player->upLevel();

    player->addSkill("defense", 1);
    // Give out parry & block
    switch(player->getClass()) {
        case CreatureClass::ASSASSIN:
        case CreatureClass::BARD:
        case CreatureClass::FIGHTER:
        case CreatureClass::RANGER:
        case CreatureClass::ROGUE:
        case CreatureClass::PALADIN:
        case CreatureClass::DEATHKNIGHT:
            player->addSkill("block", 1);
            // fall through for parry
        case CreatureClass::THIEF:
            player->addSkill("parry", 1);
            break;
        case CreatureClass::BERSERKER:
            player->addSkill("block", 1);
            // Zerkers are more brute force than finesse...no parry
            break;
        default:
            break;
    }
    // Give out armor skills here
    switch(player->getClass()) {
        case CreatureClass::BARD:
        case CreatureClass::BERSERKER:
        case CreatureClass::FIGHTER:
        case CreatureClass::PALADIN:
        case CreatureClass::DEATHKNIGHT:
        case CreatureClass::RANGER:
            player->addSkill("plate", 1);
            player->addSkill("chain", 1);
            player->addSkill("scale", 1);
            player->addSkill("ring", 1);
        case CreatureClass::ASSASSIN:
        case CreatureClass::THIEF:
        case CreatureClass::ROGUE:
        case CreatureClass::PUREBLOOD:
        case CreatureClass::DRUID:
        case CreatureClass::CLERIC:
            player->addSkill("leather", 1);
        case CreatureClass::MAGE:
        case CreatureClass::MONK:
        case CreatureClass::LICH:
        case CreatureClass::WEREWOLF:
        default:
            player->addSkill("cloth", 1);
            break;
    }

    // A few clerics get plate
    if(player->getClass() == CreatureClass::CLERIC && !player->hasSecondClass()) {
        if(player->getDeity() == ENOCH || player->getDeity() == ARES || player->getDeity() == GRADIUS) {
            player->addSkill("plate", 1);
            player->addSkill("chain", 1);
            player->addSkill("scale", 1);
            player->addSkill("ring", 1);
        }
        if(player->getDeity() == LINOTHAN || player->getDeity() == JAKAR || player->getDeity() == ARACHNUS) {
            player->addSkill("chain", 1);
            player->addSkill("scale", 1);
            player->addSkill("ring", 1);
        }
    }

    if(player->getClass() != CreatureClass::BERSERKER) {
        if(player->getCastingType() == Divine) {
            player->addSkill("healing", 1);
            player->addSkill("destruction", 1);
            player->addSkill("evil", 1);
            player->addSkill("knowledge", 1);
            player->addSkill("protection", 1);
            player->addSkill("augmentation", 1);
            player->addSkill("travel", 1);
            player->addSkill("creation", 1);
            player->addSkill("trickery", 1);
            player->addSkill("good", 1);
            player->addSkill("nature", 1);
        } else {
            player->addSkill("abjuration", 1);
            player->addSkill("conjuration", 1);
            player->addSkill("divination", 1);
            player->addSkill("enchantment", 1);
            player->addSkill("evocation", 1);
            player->addSkill("illusion", 1);
            player->addSkill("necromancy", 1);
            player->addSkill("translocation", 1);
            player->addSkill("transmutation", 1);
        }

        player->addSkill("fire", 1);
        player->addSkill("water", 1);
        player->addSkill("earth", 1);
        player->addSkill("air", 1);
        player->addSkill("cold", 1);
        player->addSkill("electric", 1);
    }

    if(player->getClass() == CreatureClass::LICH)
        player->learnSpell(S_SAP_LIFE);

    Create::addStartingItem(player, "tut", 32);
    Create::addStartingItem(player, "tut", 33);
    Create::addStartingItem(player, "tut", 34);
    Create::addStartingItem(player, "tut", 35);
    Create::addStartingItem(player, "tut", 36);
    Create::addStartingItem(player, "tut", 37);
    Create::addStartingItem(player, "tut", 38);
    Create::addStartingItem(player, "tut", 39);
    Create::addStartingItem(player, "tut", 40);
    Create::addStartingItem(player, "tut", 42, false, true, 3);

    player->setFlag(P_LAG_PROTECTION_SET);
    player->setFlag(P_CLEAR_TARGET_ON_FLEE);

    if(player->getClass() == CreatureClass::BARD)
        player->learnSong(SONG_HEAL);

    // Class/race/deity auto-granted spells, effects and languages. init() also runs
    // this on every login (idempotent); doing it here makes a headless-created
    // character complete on disk before it has ever logged in.
    player->initSpellsEffects();
}

std::shared_ptr<Player> Create::materialize(const CharacterDraft& draft, const Account& account) {
    auto player = std::make_shared<Player>();
    player->fd = -1;
    // Starting items record setDroppedBy(player) and the first save writes the id, so the
    // headless path has to mint it here; only Server::addPlayer does it on the telnet path.
    player->validateId();
    applyDraftChoices(player, draft);
    finishCharacter(player, draft.name, account);
    return player;
}

Create::PersistResult Create::persistNewCharacter(const std::shared_ptr<Player>& player, Account& account) {
    if(!player || player->getName().empty())
        return {false, "Invalid player."};

    // saveToFile, not save(): there is no socket, so Player::save can't resolve the account
    // (it would log an error) and this function saves the account itself below.
    if(player->saveToFile() != 0)
        return {false, "Failed to save the character file."};

    const fs::path playerFile = (Path::Player / player->getName()).replace_extension("xml");
    if(!account.addCharacter(player->getName())) {
        fs::remove(playerFile);
        return {false, "The account cannot hold another character."};
    }
    if(!account.save()) {
        account.removeCharacter(player->getName());
        fs::remove(playerFile);
        return {false, "Failed to save the account."};
    }
    return {true, ""};
}

Create::CommitResult Create::commitCharacter(const CharacterDraft& draft, Account& account) {
    CommitResult result;
    if(Player::exists(draft.name)) {
        result.nameTaken = true;
        result.error = "That character name is already taken.";
        return result;
    }
    result.player = Create::materialize(draft, account);
    const PersistResult persisted = persistNewCharacter(result.player, account);
    if(!persisted.ok) {
        result.error = persisted.error;
        result.player = nullptr;
        return result;
    }
    result.ok = true;
    return result;
}
