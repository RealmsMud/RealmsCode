/*
 * startlocs.cpp
 *   Player starting location code.
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
#include <fmt/format.h>                // for format
#include <list>                        // for list, operator==, list<>::iter...
#include <map>                         // for map, operator==, map<>::const_...
#include <memory>                      // for allocator, allocator_traits<>:...
#include <ostream>                     // for operator<<, basic_ostream, cha...
#include <string>                      // for string, operator==, basic_string
#include <utility>                     // for pair

#include "area.hpp"                    // for MapMarker
#include "catRef.hpp"                  // for CatRef
#include "cmd.hpp"                     // for cmd
#include "commands.hpp"                // for dmStartLocs
#include "config.hpp"                  // for Config, gConfig
#include "flags.hpp"                   // for P_CHAOTIC, P_CHOSEN_ALIGNMENT
#include "global.hpp"                  // for CreatureClass, CreatureClass::...
#include "location.hpp"                // for Location
#include "login.hpp"                   // for addStartingItem
#include "login/characterCreationService.hpp"  // for Create::startingLocationChoices
#include "magic.hpp"                   // for checkRefusingMagic, SpellData
#include "mud.hpp"                     // for DL_TELEP
#include "mudObjects/areaRooms.hpp"    // for AreaRoom
#include "mudObjects/container.hpp"    // for Container
#include "mudObjects/creatures.hpp"    // for Creature
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "proto.hpp"                   // for up, broadcast, dec_daily, low
#include "raceData.hpp"                // for RaceData
#include "startlocs.hpp"               // for StartLoc
#include "structs.hpp"                 // for daily



//*********************************************************************
//                      initialBind
//*********************************************************************
// setup the player for the first time they are bound

void initialBind(std::shared_ptr<Player> player, const std::string &str) {
    const StartLoc* location = gConfig->getStartLoc(str);
    if(!location) {
        broadcast(isCt, fmt::format("Invalid start location: {}", str).c_str());
        return;
    }
    CatRef cr = location->getStartingGuide();

    if(str == "oceancrest") {
        player->setFlag(P_HARDCORE);
        player->setFlag(P_CHAOTIC);
        player->setFlag(P_CHOSEN_ALIGNMENT);
    }

    player->bind(location);
    player->currentLocation.room = player->bound.room;
    player->currentLocation.room = player->getRecallRoom().room;

    if(cr.id)
        Create::addStartingItem(player, cr.area, cr.id, false, true);
}

//*********************************************************************
//                      startingLocationChoices
//*********************************************************************
// shared by the telnet menu and the headless flow; no Player touched
std::vector<Create::StartLocOption> Create::startingLocationChoices(short raceId, CreatureClass cClass, int deity) {
    auto make = [](const std::string& key) {
        std::string label = key;
        if(!label.empty())
            label[0] = up(label[0]);
        return Create::StartLocOption{key, label};
    };

    // if only one start location is defined, our choices are easy!
    if (gConfig->start.size() == 1)
        return { make(gConfig->start.begin()->first) };

    std::list<std::string> options;
    int race = raceId;

    // set race equal to their parent race, use raceId if checking for subraces
    const RaceData *r = gConfig->getRace(race);
    if (r && r->getParentRace())
        race = r->getParentRace();

    if (cClass == CreatureClass::DRUID) {

        // druidic order overrides all other starting locations
        options.emplace_back("druidwood");
    } else if (cClass == CreatureClass::LICH) {

        //for now, all liches start in Highport
        options.emplace_back("highport");

    } else if (deity == ENOCH || raceId == SERAPH) {

        // religious states
        options.emplace_back("sigil");

    } else if (deity == ARAMON || raceId == CAMBION) {

        // religious states
        options.emplace_back("caladon");

    } else if (race == HUMAN && cClass == CreatureClass::CLERIC) {

        // all other human clerics have to start in HP because
        // Sigil and Caladon are very religious
        options.emplace_back("highport");

    } else if (race == GNOME || race == HALFLING) {

        options.emplace_back("gnomebarrow");

    } else if (race == HALFGIANT) {

        options.emplace_back("schnai");
        options.emplace_back("highport");

    } else if (race == BARBARIAN) {

        options.emplace_back("schnai");

    } else if (race == DWARF) {

        options.emplace_back("highport");

    } else if (race == DARKELF) {

        options.emplace_back("oakspire");
        options.emplace_back("highport");

    } else if (race == TIEFLING) {

        options.emplace_back("highport");
        options.emplace_back("caladon");

    } else if (race == MINOTAUR) {

        options.emplace_back("ruhrdan");

    } else if (race == KATARAN) {

        options.emplace_back("kataran");

    } else if (race == KENKU) {

        options.emplace_back("Tokoki Peak");
        options.emplace_back("highport");

    } else if (race == HALFORC) {

        // half-breeds can start in more places
        options.emplace_back("highport");
        options.emplace_back("caladon");
        options.emplace_back("orc");

    } else if (race == ORC) {

        options.emplace_back("orc");

    } else if (race == ELF || deity == LINOTHAN) {

        options.emplace_back("eldinwood");

    } else if (race == HALFELF || race == HUMAN) {

        // humans and half-breeds can start in more places
        options.emplace_back("highport");
        options.emplace_back("sigil");

        if (race == HUMAN)
            options.emplace_back("caladon");
        else if (race == HALFELF)
            options.emplace_back("eldinwood");


    }

    switch (cClass) {
        case CreatureClass::RANGER:
            options.emplace_back("druidwood");
            break;
            // even seraphs of these classes cannot start in Sigil
        case CreatureClass::ASSASSIN:
        case CreatureClass::LICH:
        case CreatureClass::THIEF:
        case CreatureClass::DEATHKNIGHT:
        case CreatureClass::PUREBLOOD:
        case CreatureClass::ROGUE:
        case CreatureClass::WEREWOLF:
            options.remove("sigil");
            break;
        default:
            break;
    }

    // needed:
    // troll, goblin, kobold, ogre

    // these areas arent finished yet
    options.remove("caladon");
    options.remove("meadhil");
    options.remove("orc");
    options.remove("schnai");
    options.remove("ironguard");
    options.remove("kataran");

    // if the areas aren't open, give them the default starting location,
    // which is the first one on the list
    if (options.empty()) {
        if (const auto def = gConfig->getDefaultStartLoc())
            options.push_back(def->getName());
        else
            return {};   // no startlocs configured: degenerate, Location step rejects all input
    }

    std::vector<Create::StartLocOption> out;
    out.reserve(options.size());
    for (const auto& key : options)
        out.push_back(make(key));
    return out;
}

//*********************************************************************
//                      dmStartLocs
//*********************************************************************

int dmStartLocs(const std::shared_ptr<Player>& player, cmd* cmnd) {
    player->print("Starting Locations:\n");

    std::map<std::string, StartLoc*>::iterator it;
    for(it = gConfig->start.begin(); it != gConfig->start.end() ; it++) {

        player->print("%s\n", (*it).first.c_str());

    }
    return(0);
}

//*********************************************************************
//                      splBind
//*********************************************************************
// This function will change the player's bound room. If cast as a spell,
// it must belong to a list of rooms. If used from the floor, it can point
// to any room

int splBind(const std::shared_ptr<Creature>& player, cmd* cmnd, SpellData* spellData) {
    std::shared_ptr<Creature> creature=nullptr;
    std::shared_ptr<Player> target=nullptr, pPlayer = player->getAsPlayer();
    const StartLoc* location=nullptr;

    if(!pPlayer)
        return(0);

    if(spellData->how == CastType::CAST) {

        if(!pPlayer->isCt()) {
            if( pPlayer->getClass() !=  CreatureClass::MAGE &&
                pPlayer->getClass() !=  CreatureClass::LICH &&
                pPlayer->getClass() !=  CreatureClass::CLERIC &&
                pPlayer->getClass() !=  CreatureClass::DRUID
            ) {
                pPlayer->print("Your class prevents you from casting that spell.\n");
                return(0);
            }
            if(pPlayer->getLevel() < 13) {
                pPlayer->print("You are not yet powerful enough to cast that spell.\n");
                return(0);
            }
        }

        if(cmnd->num < 2) {
            pPlayer->print("Syntax: cast bind [target]%s.\n", pPlayer->isCt() ? " <location>" : "");
            return(0);
        }


        if(cmnd->num == 2)
            target = pPlayer;
        else {

            cmnd->str[2][0] = up(cmnd->str[2][0]);

            creature = pPlayer->getParent()->findCreature(pPlayer, cmnd->str[2], cmnd->val[2], false);
            if(creature)
                target = creature->getAsPlayer();

            if(!target) {
                pPlayer->print("You don't see that person here.\n");
                return(0);
            }
            if(checkRefusingMagic(player, target))
                return(0);
        }


        if(pPlayer->isCt() && cmnd->num == 4)
            location = gConfig->getStartLoc(cmnd->str[3]);
        else if(pPlayer->inUniqueRoom() && pPlayer->getUniqueRoomParent()->info.id)
            location = gConfig->getStartLocByReq(pPlayer->getUniqueRoomParent()->info);

        if( !location ||
            !location->getRequired().getId() ||
            !location->getBind().getId()
        ) {
            pPlayer->print("%s is not a valid location to bind someone!\n",
                pPlayer->isCt() && cmnd->num == 4 ? "That" : "This");
            return(0);
        }

        if(!pPlayer->isCt()) {
            if( (pPlayer->inAreaRoom() && pPlayer->getAreaRoomParent()->mapmarker != location->getRequired().mapmarker) ||
                (pPlayer->inUniqueRoom() && pPlayer->getUniqueRoomParent()->info != location->getRequired().room)
            ) {
                pPlayer->print("To bind to this location, you must be at %s.\n", location->getRequiredName().c_str());
                return(0);
            }
            if(!dec_daily(&pPlayer->daily[DL_TELEP])) {
                pPlayer->print("You are too weak to bind again today.\n");
                return(0);
            }
        }

        if(pPlayer == target) {

            pPlayer->print("Bind spell cast.\nYou are now bound to %s.\n", location->getBindName().c_str());
            broadcast(pPlayer->getSock(), pPlayer->getRoomParent(), "%M casts a bind spell on %sself.", pPlayer.get(), pPlayer->himHer());

        } else {

            // nosummon flag
            if( target->flagIsSet(P_NO_SUMMON) &&
                !pPlayer->checkStaff("The spell fizzles.\n%M's summon flag is not set.\n", target.get()))
            {
                target->print("%s tried to bind you to this room!\nIf you wish to be bound, type \"set summon\".\n", pPlayer->getCName());
                return(0);
            }

            pPlayer->print("Bind cast on %s.\n%s is now bound to %s.\n",
                target->getCName(), target->getCName(), location->getBindName().c_str());
            target->print("%M casts a bind spell on you.\nYou are now bound to %s.\n",
                pPlayer.get(), location->getBindName().c_str());
            broadcast(player->getSock(), target->getSock(), pPlayer->getRoomParent(),
                "%M casts a bind spell on %N.", pPlayer.get(), target.get());

        }

        target->bind(location);

    } else {

        if(spellData->how != CastType::WAND) {
            pPlayer->print("Nothing happens.\n");
            return(0);
        } else {

            // we need more info from the item!
            pPlayer->print("Nothing happens (for now).\n");
            return(0);

        }
    }

    return(1);
}


//*********************************************************************
//                      StartLoc
//*********************************************************************

StartLoc::StartLoc() {
    name = requiredName = bindName = "";
    primary = false;
}


std::string StartLoc::getName() const { return(name); }
std::string StartLoc::getBindName() const { return(bindName); }
std::string StartLoc::getRequiredName() const { return(requiredName); }
Location StartLoc::getBind() const { return(bind); }
Location StartLoc::getRequired() const { return(required); }
CatRef StartLoc::getStartingGuide() const { return(startingGuide); }
bool StartLoc::isDefault() const { return(primary); }
void StartLoc::setDefault() { primary = true; }

//*********************************************************************
//                      bind
//*********************************************************************

void Player::bind(const StartLoc* location) {
    bound = location->getBind();
}


//*********************************************************************
//                      getStartLoc
//*********************************************************************

const StartLoc *Config::getStartLoc(const std::string &id) const {
    auto it = start.find(id);

    if(it == start.end())
        return(nullptr);

    return((*it).second);
}


//*********************************************************************
//                      getDefaultStartLoc
//*********************************************************************

const StartLoc *Config::getDefaultStartLoc() const {
    std::map<std::string, StartLoc*>::const_iterator it;
    for(it = start.begin(); it != start.end() ; it++) {
        if((*it).second->isDefault())
            return((*it).second);
    }
    return(nullptr);
}


//*********************************************************************
//                      getStartLocByReq
//*********************************************************************

const StartLoc *Config::getStartLocByReq(const CatRef& cr) const {
    std::map<std::string, StartLoc*>::const_iterator it;
    StartLoc* s=nullptr;

    for(it = start.begin() ; it != start.end() ; it++) {
        s = (*it).second;
        if(cr == s->getRequired().room)
            return(s);
    }
    return(nullptr);
}


//*********************************************************************
//                      clearStartLoc
//*********************************************************************

void Config::clearStartLoc() {
    std::map<std::string, StartLoc*>::iterator it;

    for(it = start.begin() ; it != start.end() ; it++) {
        StartLoc* s = (*it).second;
        delete s;
    }
    start.clear();
}
