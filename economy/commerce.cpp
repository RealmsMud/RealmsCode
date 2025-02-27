/*
 * commerce.cpp
 *   Functions to handle buying/selling/trading from shops/monsters.
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
#include <algorithm>                   // for replace
#include <cstdlib>                     // for atoi, atol
#include <cstring>                     // for strncmp, strcmp, strlen, strcpy
#include <ctime>                       // for time
#include <iterator>                    // for next
#include <list>                        // for list, operator==, list<>::cons...
#include <map>                         // for operator==, _Rb_tree_const_ite...
#include <memory>                      // for allocator, allocator_traits<>:...
#include <set>                         // for set<>::iterator, set
#include <string>                      // for string, operator==, char_traits
#include <string_view>                 // for operator==, basic_string_view
#include <type_traits>                 // for enable_if<>::type
#include <utility>                     // for pair

#include "bank.hpp"                    // for guildLog
#include "carry.hpp"                   // for Carry
#include "catRef.hpp"                  // for CatRef
#include "cmd.hpp"                     // for cmd
#include "color.hpp"                   // for padColor
#include "commands.hpp"                // for getFullstrText, cmdHelp, cmdPr...
#include "config.hpp"                  // for Config, gConfig
#include "creatureStreams.hpp"         // for Streamable, ColorOff, ColorOn
#include "dm.hpp"                      // for findRoomsWithFlag
#include "factions.hpp"                // for Faction, Faction::INDIFFERENT
#include "flags.hpp"                   // for P_AFK, R_SHOP, O_PERM_ITEM
#include "global.hpp"                  // for CreatureClass, MAG, PROP_SHOP
#include "guilds.hpp"                  // for Guild, shopStaysWithGuild
#include "hooks.hpp"                   // for Hooks
#include "lasttime.hpp"                // for lasttime
#include "location.hpp"                // for Location
#include "money.hpp"                   // for Money, GOLD
#include "mud.hpp"                     // for MINSHOPLEVEL, GUILD_MASTER
#include "mudObjects/container.hpp"    // for ObjectSet, Container
#include "mudObjects/creatures.hpp"    // for Creature
#include "mudObjects/exits.hpp"        // for Exit
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/objects.hpp"      // for Object, ObjectType, ObjectType...
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom, ExitList
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "property.hpp"                // for Property, PartialOwner
#include "proto.hpp"                   // for broadcast, logn, link_rom, nee...
#include "random.hpp"                  // for Random
#include "range.hpp"                   // for Range
#include "server.hpp"                  // for Server, GOLD_OUT, GOLD_IN, gSe...
#include "structs.hpp"                 // for saves
#include "unique.hpp"                  // for Lore, addOwner, isLimited, Unique
#include "xml.hpp"                     // for loadRoom, loadObject, loadPlayer
#include "commerce.hpp"
#include "toNum.hpp"



//*********************************************************************
//                      buyAmount
//*********************************************************************

Money buyAmount(const std::shared_ptr<Player>&
player, const std::shared_ptr<Monster>& monster, const std::shared_ptr<Object>&  object, bool sell) {
    Money cost = Faction::adjustPrice(player, monster->getPrimeFaction(), object->value, true);
    cost.set(std::max<unsigned long>(10,cost[GOLD]), GOLD);
    return(cost);
}

Money buyAmount(const std::shared_ptr<Player>& player, const std::shared_ptr<const UniqueRoom>& room, const std::shared_ptr<Object>&  object, bool sell) {
    Money cost = Faction::adjustPrice(player, room->getFaction(), object->value, true);
    cost.set(std::max<unsigned long>(10,cost[GOLD]), GOLD);
    return(cost);
}

//*********************************************************************
//                      sellAmount
//*********************************************************************

Money sellAmount(const std::shared_ptr<Player>& player, const std::shared_ptr<const UniqueRoom>& room, const std::shared_ptr<Object>&  object, bool sell) {
    Money value = Faction::adjustPrice(player, room->getFaction(), object->value, sell);
    value.set(std::min<unsigned long>(value[GOLD] / 2, MAXPAWN), GOLD);
    return(value);
}

Money sellAmount(const std::shared_ptr<const Player>& player, const std::shared_ptr<const UniqueRoom>& room, const std::shared_ptr<Object>&  object, bool sell) {
    Money value = Faction::adjustPrice(player, room->getFaction(), object->value, sell);
    value.set(std::min<unsigned long>(value[GOLD] / 2, MAXPAWN), GOLD);
    return(value);
}

//*********************************************************************
//                      bailCost
//*********************************************************************

Money bailCost(const std::shared_ptr<Player>& player) {
    Money cost;
    cost.set(2000*player->getLevel(), GOLD);
    return(cost);
}

//*********************************************************************
//                      objShopName
//*********************************************************************

std::string objShopName(const std::shared_ptr<Object>&  object, int m, int flags, int pad) {
    std::string name = object->getObjStr(nullptr, flags, m);
    return padColor(name, pad);
}

//*********************************************************************
//                      doFilter
//*********************************************************************

bool doFilter(const std::shared_ptr<Object>&  object, std::string_view filter) {
    if(filter.empty())
        return(false);

    if(filter == object->getSubType())
        return(false);

    if( (filter == "weapon" && object->getType() == ObjectType::WEAPON) ||
        (filter == "armor" && object->getType() == ObjectType::ARMOR) ||
        (filter == "potion" && object->getType() == ObjectType::POTION) ||
        (filter == "scroll" && object->getType() == ObjectType::SCROLL) ||
        (filter == "wand" && object->getType() == ObjectType::WAND) ||
        (filter == "container" && object->getType() == ObjectType::CONTAINER) ||
        (filter == "key" && object->getType() == ObjectType::KEY) ||
        ((filter == "light" || filter == "lightsource") && object->getType() == ObjectType::LIGHTSOURCE) ||
        (filter == "song" && object->getType() == ObjectType::SONGSCROLL) ||
        (filter == "poison" && object->getType() == ObjectType::POISON) ||
        (filter == "bandage" && object->getType() == ObjectType::BANDAGE)
    )
        return(false);

    if(filter != "none" && filter == object->getWeaponCategory())
        return(false);

    return(true);
}

//*********************************************************************
//                      isValidShop
//*********************************************************************
// everything must be set up perfectly for the shop to work

bool isValidShop(const std::shared_ptr<UniqueRoom>& shop, const std::shared_ptr<UniqueRoom>& storage) {
    // both rooms must exist
    if(!shop || !storage)
        return(false);

    // flags must be set properly
    if(!shop->flagIsSet(R_SHOP))
        return(false);

    if(!storage->flagIsSet(R_SHOP_STORAGE))
        return(false);

    // cannot be itself
    if(shop->info == storage->info)
        return(false);

    // shop must point to storage room
    if(shopStorageRoom(shop) != storage->info)
        return(false);

    return(true);
}

//*********************************************************************
//                      cannotUseMarker
//*********************************************************************

const char* cannotUseMarker(const std::shared_ptr<Player>& player, const std::shared_ptr<Object>&  object) {
    if(object->getType() == ObjectType::WEAPON || object->getType() == ObjectType::ARMOR) {
        if(!player->canUse(object, true))
            return(" ^r(x)^x");
    }
    return("");
}


//*********************************************************************
//                      cmdList
//*********************************************************************
// This function allows a player to list the items for sale within a
// shop.

int cmdList(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<UniqueRoom> room = player->getUniqueRoomParent(), storage=nullptr;
    std::string filter;

    // interchange list and selection
    if(cmnd->num == 2 && cmnd->str[1][0] != '-')
        return(cmdSelection(player, cmnd));

    if(cmnd->num == 2)
        filter = &cmnd->str[1][1];

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(!needUniqueRoom(player))
        return(0);

    if(!room->flagIsSet(R_SHOP)) {
        *player << "This is not a shop.\n";
        return(0);
    }

    if(!loadRoom(shopStorageRoom(room), storage)) {
        *player << "Nothing to buy.\n";
        return(0);
    }

    // Everything must be set up perfectly
    if(!isValidShop(room, storage)) {
        *player << "This is not a shop.\n";
        return(0);
    }

    Property* p = gConfig->getProperty(room->info);

    // not a player run shop
    if(!p || p->getType() != PROP_SHOP) {
        shopList(player, p, filter, storage);
    } else {
        // We've got a player run shop here...different formating them
        playerShopList(player, p, filter, storage);

    }

    return(0);
}

void shopList(const std::shared_ptr<Player>& player, Property* p, std::string& filter, std::shared_ptr<UniqueRoom>& storage) {
    Money cost;
    std::shared_ptr<Object>  object;


    if(!Faction::willDoBusinessWith(player, player->getUniqueRoomParent()->getFaction())) {
        *player << "The shopkeeper refuses to do business with you.\n";
        return;
    }

    storage->addPermObj();
    ObjectSet::iterator it;
    if(!storage->objects.empty()) {
        *player << "You may buy:";
        if(!filter.empty())
            *player << ColorOn << " ^Y(filtering on \"" << filter << "\")" << ColorOff;
        *player << "\n";
        for(it = storage->objects.begin() ; it != storage->objects.end() ; ) {
            object = (*it++);
            if(doFilter(object, filter))
                continue;

            if(object->getName() == "bail")
                object->value = bailCost(player);

            cost = buyAmount(player, player->getUniqueRoomParent(), object, true);

            // even if they love you, lottery tickets are the same price
            if(object->getType() == ObjectType::LOTTERYTICKET) {
                object->value.set(gConfig->getLotteryTicketPrice(), GOLD);
                cost = object->value;
            }

            player->printColor("   %s   Cost: %s%s\n",
                               objShopName(object, 1, CAP | (player->isEffected("detect-magic") ? MAG : 0), 52).c_str(),
                               cost.str().c_str(), cannotUseMarker(player, object));

        }
        *player << "\n";
    }  else {
        *player << "There is nothing for sale.\n";
    }
}


//*********************************************************************
//                      cmdPurchase
//*********************************************************************
// purchase allows a player to buy an item from a monster.  The
// purchase item flag must be set, and the monster must have an
// object to sell.  The object for sale is determined by the first
// object listed in carried items.

int cmdPurchase(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Monster> creature=nullptr;
    std::shared_ptr<Object> object=nullptr;
    int     maxitem=0;
    CatRef  obj_num[10];
    Money cost;
    int     i=0, j=0, found=0, match=0;


    if(cmnd->num == 2)
        return(cmdBuy(player, cmnd));


    player->clearFlag(P_AFK);
    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(cmnd->num < 2) {
        player->print("Syntax: purchase <item> <monster>\n");
        return(0);
    }

    if(cmnd->num < 3 ) {
        player->print("Syntax: purchase <item> <monster>\n");
        return(0);
    }

    creature = player->getParent()->findMonster(player, cmnd, 2);
    if(!creature) {
        *player << "That is not here.\n";
        return(0);
    }

    if(!Faction::willDoBusinessWith(player, creature->getPrimeFaction())) {
        player->print("%M refuses to do business with you.\n", creature.get());
        return(0);
    }

    if(!creature->flagIsSet(M_CAN_PURCHASE_FROM)) {
        player->print("You cannot purchase objects from %N.\n",creature.get());
        return(0);
    }

    for(i=0; i<10; i++) {
        if(creature->carry[i].info.id > 0) {
            found = 0;
            for(j=0; j< maxitem; j++)
                if(creature->carry[i].info == obj_num[j])
                    found = 1;
            if(!found) {
                maxitem++;
                obj_num[i] = creature->carry[i].info;
            }
        }
    }

    if(!maxitem) {
        player->print("%M has nothing to sell.\n",creature.get());
        return(0);
    }

    found = 0;
    for(i=0; i<maxitem; i++) {
        if(loadObject(creature->carry[i].info, object)) {
            if(player->canSee(object) && keyTxtEqual(object, cmnd->str[1])) {
                match++;
                if(match == cmnd->val[1]) {
                    found = 1;
                    break;
                } else
                    object.reset();
            } else
                object.reset();
        }
    }

    if(!found) {
        player->print("%M says, \"Sorry, I don't have any of those to sell.\"\n", creature.get());
        return(0);
    }

    object->init();

    if(player->getWeight() + object->getActualWeight() > player->maxWeight()) {
        *player << "That would be too much for you to carry.\n";
        object.reset();
        return(0);
    }

    if(player->tooBulky(object->getActualBulk())) {
        *player << "That would be too bulky.\nClean your inventory first.\n";
        object.reset();
        return(0);
    }


    if(!Unique::canGet(player, object)) {
        *player << "You are unable to purchase that limited item at this time.\n";
        object.reset();
        return(0);
    }
    if(!Lore::canHave(player, object, false)) {
        *player << "You cannot purchased that item.\nIt is a limited item of which you cannot carry any more.\n";
        object.reset();
        return(0);
    }
    if(!Lore::canHave(player, object, true)) {
        *player << "You cannot purchased that item.\nIt contains a limited item that you cannot carry.\n";
        object.reset();
        return(0);
    }



    cost = buyAmount(player, creature, object, true);

    if(player->coins[GOLD] < cost[GOLD]) {
        player->print("%M says \"The price is %s, and not a copper piece less!\"\n", creature.get(), cost.str().c_str());
        object.reset();
    } else {

        player->print("You pay %N %s gold pieces.\n", creature.get(), cost.str().c_str());
        broadcast(player->getSock(), player->getParent(), "%M pays %N %s for %P.\n", player.get(), creature.get(), cost.str().c_str(), object.get());
        player->coins.sub(cost);

        player->doHaggling(creature, object, BUY);
        Server::logGold(GOLD_OUT, player, object->refund, object, "MobPurchase");

        if(object->hooks.executeWithReturn("afterPurchase", player, creature->getId())) {
            // A return value of true means the object still exists
            player->printColor("%M says, \"%sHere is your %s.\"\n", creature.get(),
                               Faction::getAttitude(player->getFactionStanding(creature->getPrimeFaction())) < Faction::INDIFFERENT ? "Hrmph. " : "Thank you very much. ",
                               object->getCName());
            Limited::addOwner(player, object);
            object->setDroppedBy(creature, "MobPurchase");

            player->addObj(object);

        } else {
            // A return value of false means the object doesn't exist, and as such the spell was cast, so delete it
            object.reset();

        }

    }

    return(0);
}


//*********************************************************************
//                      cmdSelection
//*********************************************************************
// The selection command lists all the items  a monster is selling.
// The monster needs the M_CAN_PURCHASE_FROM flag set to denote it can sell.

int cmdSelection(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Monster> creature=nullptr;
    std::shared_ptr<Object> object=nullptr;
    CatRef  obj_list[10];
    int     i=0, j=0, found=0, maxitem=0;
    std::string filter;

    // interchange list and selection
    if(cmnd->num == 1 || cmnd->str[1][0] == '-')
        return(cmdList(player, cmnd));

    if(cmnd->num == 3)
        filter = &cmnd->str[2][1];

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);


    creature = player->getParent()->findMonster(player, cmnd);
    if(!creature) {
        *player << "That is not here.\n";
        return(0);
    }

    if(!Faction::willDoBusinessWith(player, creature->getPrimeFaction())) {
        player->print("%M refuses to do business with you.\n", creature.get());
        return(0);
    }

    if(!creature->flagIsSet(M_CAN_PURCHASE_FROM)) {
        player->print("%M is not selling anything.\n",creature.get());
        return(0);
    }

    for(i=0; i<10; i++) {
        if(creature->carry[i].info.id > 0) {
            found = 0;
            for(j=0; j< maxitem; j++)
                if(creature->carry[i].info == obj_list[j])
                    found = 1;

            if(!found) {
                maxitem++;
                obj_list[i] = creature->carry[i].info;
            }
        }
    }

    if(!maxitem) {
        player->print("%M has nothing to sell.\n", creature.get());
        return(0);
    }

    player->print("%M is currently selling:", creature.get());
    if(!filter.empty())
        player->printColor(" ^Y(filtering on \"%s\")", filter.c_str());
    *player << "\n";

    Money cost;
    for(i=0; i<maxitem; i++) {
        if(!creature->carry[i].info.id || !loadObject(creature->carry[i].info, object)) {
            player->print("%d) Out of stock.\n", i+1);
        } else {
            if(!doFilter(object, filter)) {
                cost = buyAmount(player, creature, object, true);

                player->printColor("%d) %s    %s\n", i+1, objShopName(object, 1, 0, 35).c_str(),
                                   cost.str().c_str());
            }
            object.reset();
        }
    }

    *player << "\n";
    return(0);
}



//*********************************************************************
//                      cmdBuy
//*********************************************************************
// This function allows a player to buy something from a shop.

int cmdBuy(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<UniqueRoom> room = player->getUniqueRoomParent(), storage=nullptr;

    if(cmnd->num == 3)
        return(cmdPurchase(player, cmnd));

    if(!needUniqueRoom(player))
        return(0);


    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(player->getClass() == CreatureClass::BUILDER) {
        *player << "You may not buy things from shops.\n";
        return(0);
    }

    if(!room->flagIsSet(R_SHOP)) {
        *player << "This is not a shop.\n";
        return(0);
    }

    if(cmnd->num < 2) {
        player->print("Buy what?\n");
        return(0);
    }

    if(!loadRoom(shopStorageRoom(room), storage)) {
        *player << "Nothing to buy.\n";
        return(0);
    }

    if(!isValidShop(room, storage)) {
        *player << "This is not a shop.\n";
        return(0);
    }

    //*********************************************************************
    Property* p = gConfig->getProperty(room->info);

    if(p && p->getType() == PROP_SHOP) {
        // We have a player run shop here!!!
        playerShopBuy(player, cmnd, p, storage);
    } else {
        // Not a player run shop
        shopBuy(player, cmnd, p, storage);
    }
    return(0);
}

void shopBuy(const std::shared_ptr<Player>& player, cmd* cmnd, Property* p, std::shared_ptr<UniqueRoom>& storage) {
    std::shared_ptr<UniqueRoom> room = player->getUniqueRoomParent();
    std::shared_ptr<Object> object=storage->findObject(player, cmnd, 1);
    std::shared_ptr<Object> object2=nullptr;

    if(!object) {
        *player << "That's not being sold.\n";
        return;
    }

    if(!Faction::willDoBusinessWith(player, player->getUniqueRoomParent()->getFaction())) {
        *player << "The shopkeeper refuses to do business with you.\n";
        return;
    }

    if(object->getName() == "storage room") {
        if(!room->flagIsSet(R_STORAGE_ROOM_SALE)) {
            *player << "You can't buy storage rooms here.\n";
            return;
        }

        CatRef  cr = gConfig->getSingleProperty(player, PROP_STORAGE);
        if(cr.id) {
            *player << "You are already affiliated with a storage room.\nOnly one is allowed per player.\n";
            return;
        }
    }

    if(object->getName() == "bail")
        object->value = bailCost(player);

    Money cost = buyAmount(player, player->getUniqueRoomParent(), object, true);

    // even if they love you, lottery tickets are the same price
    if(object->getType() == ObjectType::LOTTERYTICKET) {
        object->value.set(gConfig->getLotteryTicketPrice(), GOLD);
        cost = object->value;
    }

    if(object->getName() ==  "bail") {
        // Depends on level for bail
        // 2k per lvl to get out
        if(player->coins[GOLD] < cost[GOLD]) {
            *player << "You don't have enough gold to post bail.\n";
            return;
        }
    } else {

        if(player->coins[GOLD] < cost[GOLD]) {
            *player << "You don't have enough gold.\n";
            return;
        }

        if( player->getWeight() + object->getActualWeight() > player->maxWeight() &&
            !player->checkStaff("You can't carry anymore.\n") )
            return;

        if(player->tooBulky(object->getActualBulk()) ) {
            *player << "No more will fit in your inventory right now.\n";
            return;
        }

    }

    // Not buying a storage room or bail
    if(object->getName() != "storage room" && object->getName() != "bail") {

        player->unhide();
        if(object->getType() == ObjectType::LOTTERYTICKET) {
            if(gConfig->getLotteryEnabled() == 0) {
                player->print("Sorry, lottery tickets are not currently being sold.\n");
                return;
            }

            if(createLotteryTicket(object2, player->getCName()) < 0) {
                player->print("Sorry, lottery tickets are not currently being sold.\n");
                return;
            }
            // Not a lottery Ticket
        } else {
            object2 = std::make_shared<Object>(*object);
            object2->clearFlag(O_PERM_INV_ITEM);
            object2->clearFlag(O_PERM_ITEM);
            object2->clearFlag(O_TEMP_PERM);
        }

        if(object2)
            object2->init();

        if(!Unique::canGet(player, object)) {
            *player << "You are unable to purchase that limited item at this time.\n";
            object2.reset();
            return;
        }
        if(!Lore::canHave(player, object, false)) {
            *player << "You cannot purchased that item.\nIt is a limited item of which you cannot carry any more.\n";
            object2.reset();
            return;
        }
        if(!Lore::canHave(player, object, true)) {
            *player << "You cannot purchased that item.\nIt contains a limited item that you cannot carry.\n";
            object2.reset();
            return;
        }

        bool isDeed = object2->deed.low.id || object2->deed.high;

        // buying a deed
        if(isDeed && player->getLevel() < MINSHOPLEVEL) {
            player->print("You must be at least level %d to buy a shop deed.\n", MINSHOPLEVEL);
            object2.reset();
            return;
        }

        player->printColor("You bought %1P.\n", object2.get());

        object2->refund.zero();
        object2->refund.set(cost);
        player->coins.sub(cost);

        // No haggling on deeds!
        if(!isDeed)
            player->doHaggling(nullptr, object2, BUY);

        // We just did a full priced purchase, we can now haggle again (unless they do another refund)
        if(player->getRoomParent() && player->getRoomParent()->getAsUniqueRoom())
            player->removeRefundedInStore(player->getRoomParent()->getAsUniqueRoom()->info);


        object2->setDroppedBy(room, "StoreBought");
        Server::logGold(GOLD_OUT, player, object2->refund, object2, "StoreBought");

        if(object2->getType() == ObjectType::LOTTERYTICKET || object2->getType() == ObjectType::BANDAGE)
            object2->value.zero();

        player->print("You have %ld gold left.", player->coins[GOLD]);
        if(object2->getType() == ObjectType::LOTTERYTICKET)
            player->print(" Your numbers are %02d %02d %02d %02d %02d  (%02d).", object2->getLotteryNumbers(0),
                          object2->getLotteryNumbers(1), object2->getLotteryNumbers(2),
                          object2->getLotteryNumbers(3), object2->getLotteryNumbers(4), object2->getLotteryNumbers(5));
        *player << "\n";

        if(object2->getName() != "storage room" && object2->getName() != "bail" && object2->getType() != ObjectType::LOTTERYTICKET)
            object2->setFlag(O_JUST_BOUGHT);

        broadcast(player->getSock(), player->getParent(), "%M bought %1P.", player.get(), object2.get());

        logn("log.commerce", "%s just bought %s for %s in room %s.\n",
             player->getCName(), object2->getCName(), cost.str().c_str(), player->getRoomParent()->fullName().c_str());

        if(isDeed) {
            int flag=0;
            std::string type;
            std::string name = object2->getName();

            if(name == "shop deed") {
                type = "shop";
                flag = R_BUILD_SHOP;
            } else if(name.starts_with("guildhall deed")) {
                type = "guildhall";
                flag = R_BUILD_GUILDHALL;

                // Purchase warnings for guildhalls
                if(!player->getGuild())
                    player->printColor("^YCaution:^x you must belong to a guild to use a guildhall deed.\n");
                else if(player->getGuildRank() != GUILD_MASTER)
                    player->printColor("^YCaution:^x you must be a guildmaster to use a guildhall deed.\n");
                else {
                    std::list<Property*>::iterator it;
                    for(it = gConfig->properties.begin() ; it != gConfig->properties.end() ; it++) {
                        if((*it)->getType() != PROP_GUILDHALL)
                            continue;
                        if(player->getGuild() != (*it)->getGuild())
                            continue;
                        if(player->inUniqueRoom() && !player->getUniqueRoomParent()->info.isArea((*it)->getArea()))
                            continue;
                        player->printColor("^YCaution:^x your guild already owns a guildhall in %s.\n", gConfig->catRefName((*it)->getArea()).c_str());
                    }
                }
            }

            if(flag)
                findRoomsWithFlag(player, object2->deed, flag);
        }

        if(object->hooks.executeWithReturn("afterPurchase", player)) {
            Limited::addOwner(player, object2);
            player->addObj(object2);
        } else {
            object2.reset();
        }

    } else if(object->getName() == "storage room") {

        CatRef  cr = gConfig->getAvailableProperty(PROP_STORAGE, 1);
        if(cr.id < 1) {
            player->print("The shopkeeper says, \"Sorry, I don't have any more rooms available!\"\n");
            return;
        }

        player->coins.sub(cost);
        Server::logGold(GOLD_OUT, player, cost, nullptr, "PurchaseStorage");

        player->print("The shopkeeper says, \"Congratulations, you are now the proud owner of a new storage room. Don't forget, to get in you must also buy a key.\"\n");
        player->print("You have %s left.\n", player->coins.str().c_str());
        broadcast(player->getSock(), player->getParent(), "%M just bought a storage room.", player.get());
        logn("log.storage", "%s bought storage room %s.\n",
             player->getCName(), cr.displayStr().c_str());

        createStorage(cr, player);

    } else {
        // Buying bail to get outta jail!
        std::shared_ptr<Exit> exit=nullptr;

        // Find the cell door
        exit = findExit(player, "cell", 1);
        player->unhide();

        if(!exit || !exit->flagIsSet(X_LOCKED)) {
            player->print("You are in no need of bail!\n");
            return;
        }

        exit->clearFlag(X_LOCKED);
        exit->ltime.ltime = time(nullptr);
        player->coins.sub(cost);
        Server::logGold(GOLD_OUT, player, cost, exit, "Bail");
        player->print("You bail yourself out for %s gold.\n", cost.str().c_str());
        broadcast("### %M just bailed %sself out of jail!", player.get(), player->himHer());
    }    
}

//*********************************************************************
//                      cmdSell
//*********************************************************************
// This function will allow a player to sell an object in a pawn shop

int cmdSell(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Object> object=nullptr;
    Money value;


    if(player->getClass() == CreatureClass::BUILDER)
        return(0);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    auto room = player->getRoomParent();
    if(!room->flagIsSet(R_PAWN_SHOP) && !room->flagIsSet(R_SHOP)) {
        *player << "This is not a pawn shop.\n";
        return(0);
    }

    if(cmnd->num < 2) {
        player->print("Sell what?\n");
        return(0);
    }

    player->unhide();
    object = player->findObject(player, cmnd, 1, true);

    if(!object) {
        *player << "You don't have that.\n";
        return(0);
    }

    if(object->flagIsSet(O_KEEP)) {
        player->printColor("%O is currently set for safe keeping.\nUnkeep it to sell it: ^Wunkeep %s^x\n", object.get(), object.get()->key[0]);
        return(0);
    }

    if(!object->objects.empty()) {
        player->print("You don't want to sell that!\nThere's something inside it!\n");
        return(0);
    }

    if(!Faction::willDoBusinessWith(player, player->getUniqueRoomParent()->getFaction())) {
        *player << "The shopkeeper refuses to do business with you.\n";
        return(0);
    }

    value = sellAmount(player, player->getConstUniqueRoomParent(), object, false);

    if (object->isTrashAtPawn(value)) {
        switch(Random::get(1,6)) {
            case 1:
                player->print("The shopkeep says, \"I'm not interested in that.\"\n");
                break;
            case 2:
                player->print("The shopkeep says, \"I don't want that.\"\n");
                break;
            case 3:
                player->print("The shopkeep says, \"Sorry, I can't sell that. I'm not buying it!\"\n");
                break;
            case 4:
                player->print("The shopkeep says, \"Sorry, there's no market for garbage like that.\"\n");
                break;
            case 5:
                player->print("The shopkeep says, \"You must be joking, right? I can't sell that!\"\n");
                break;
            default:
                player->print("The shopkeep says, \"I won't buy that crap from you.\"\n");
                break;
        }
        return(0);
    }

    // No pawning of hot items!
    if(object->flagIsSet(O_WAS_SHOPLIFTED)) {
        player->print("The shopkeep says, \"That's so hot that it's practically smoking! I won't buy it! Get out!\"\n");
        return(0);
    }

    if(object->flagIsSet(O_NO_PAWN)) {
        player->print("The shopkeep says, \"There's no way I could move such an item. That won't work for me. Sorry.\"\n");
        return(0);
    }

    player->printColor("The shopkeep gives you %s for %P.\n", value.str().c_str(), object.get());
    broadcast(player->getSock(), player->getParent(), "%M sells %1P.", player.get(), object.get());

    object->refund.zero();
    object->refund.set(value);
    player->doHaggling(nullptr, object, SELL);
    Server::logGold(GOLD_IN, player, object->refund, object, "Pawn");
    logn("log.commerce", "%s sold %s in room %s.\n", player->getCName(), object->getCName(), player->getRoomParent()->fullName().c_str());


    player->coins.add(value);
    player->delObj(object, true);
    player->setLastPawn(object);
    return(0);
}

//*********************************************************************
//                      cmdValue
//*********************************************************************
// This function allows a player to find out the pawn-shop value of an
// object, if they are in the pawn shop.

int cmdValue(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Object> object=nullptr;
    Money value;


    if(!player->ableToDoCommand())
        return(0);

    if(!player->getRoomParent()->flagIsSet(R_PAWN_SHOP)) {
        *player << "You must be in a pawn shop.\n";
        return(0);
    }

    if(cmnd->num < 2) {
        player->print("Value what?\n");
        return(0);
    }

    player->unhide();

    object = player->findObject(player, cmnd, 1, true);

    if(!object) {
        *player << "You don't have that.\n";
        return(0);
    }

    if(!Faction::willDoBusinessWith(player, player->getUniqueRoomParent()->getFaction())) {
        *player << "The shopkeeper refuses to do business with you.\n";
        return(0);
    }

    value = sellAmount(player, player->getConstUniqueRoomParent(), object, false);

    player->printColor("The shopkeep says, \"%O's worth %s.\"\n", object.get(), value.str().c_str());

    broadcast(player->getSock(), player->getParent(), "%M gets %P appraised.", player.get(), object.get());

    return(0);
}



//*********************************************************************
//                      cmdRefund
//*********************************************************************

int cmdRefund(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Object> object=nullptr;


    if(player->getClass() == CreatureClass::BUILDER)
        return(0);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(!player->getRoomParent()->flagIsSet(R_SHOP)) {
        *player << "This is not a shop.\n";
        return(0);
    }

    if(cmnd->num < 2) {
        player->print("Refund what?\n");
        return(0);
    }

    object = player->findObject(player, cmnd, 1);
    if(!object) {
        *player << "You don't have that in your inventory.\n";
        return(0);
    }

    if(!object->flagIsSet(O_JUST_BOUGHT) || (object->getShotsCur() < object->getShotsMax()) || object->getShopValue()) {
        player->print("The shopkeep says, \"I don't return money for used goods!\"\n");
        return(0);
    }

    if(object->getRecipe()) {
        *player << "You cannot return intellectual property.\n";
        return(0);
    }


    
    player->coins.add(object->refund);
    player->printColor("The shopkeep takes the %s from you and returns %s to you.\n", object->getCName(), object->refund.str().c_str());
    broadcast(player->getSock(), player->getParent(), "%M refunds %P.", player.get(), object.get());
    Server::logGold(GOLD_IN, player, object->refund, object, "Refund");
    player->delObj(object, true);
    // No further haggling allowed in this store until they buy a full priced item and have left the room
    player->setFlag(P_JUST_REFUNDED);
    if(player->getRoomParent() && player->getRoomParent()->getAsUniqueRoom()) {
        player->addRefundedInStore(player->getRoomParent()->getAsUniqueRoom()->info);
    }

    object.reset();
    return(0);
}

//*********************************************************************
//                      failTrade
//*********************************************************************

void failTrade(const std::shared_ptr<Player>& player, const std::shared_ptr<Object>&  object, const std::shared_ptr<Monster>&  target, std::shared_ptr<Object> trade=nullptr, bool useHeShe= true) {
    *player << ColorOn;
    if (useHeShe) *player << target->upHeShe();
    else          *player << target;
    *player << "gives you back " << object << ".\n" << ColorOff;

    broadcast(player->getSock(), player->getParent(), "%M tried to trade %P with %N.", player.get(), object.get(), target.get());
    if(trade) trade.reset();
}

//*********************************************************************
//                      canReceiveObject
//*********************************************************************
// Determine if the player can receive the object as a result of a trade
// or completing a quest. This code does NOT handle bulk/weight, as that
// is tracked on a broader scale, rather than for each item.

bool canReceiveObject(const std::shared_ptr<Player>& player, const std::shared_ptr<Object>&  object, const std::shared_ptr<Monster>&  monster, bool isTrade, bool doQuestOwner, bool maybeTryAgainMsg) {
    std::string tryAgain;
    // if the result of the quest/trade is a random item and we fail based on that random item,
    // they may be able to retry the quest/trade.
    if(maybeTryAgainMsg) {
        if(isTrade)
            tryAgain = "As the result of the trade is a random item, you may be able to attempt the trade again.\n";
        else
            tryAgain = "As the quest reward is a random item, you may be able to attempt to complete the quest again.\n";
    }

    // check for unique items
    if(!Unique::canGet(player, object)) {
        if(isTrade)
            player->print("%M cannot trade for that right now.\n", monster.get());
        else
            player->print("You cannot complete that quest right now.\nThe unique item reward is already in the game.\n");
        if(!tryAgain.empty())
            player->print(tryAgain.c_str());
        return(false);
    }

    // check for lore items
    if(!Lore::canHave(player, object, false)) {
        if(isTrade)
            player->printColor("%M cannot trade for that right now, as you already have %P.\n", monster.get(), object.get());
        else
            player->printColor("You cannot complete that quest right now, as you already have %P.\n", object.get());
        if(!tryAgain.empty())
            player->print(tryAgain.c_str());
        return(false);
    }

    // check for quest items
    if( object->getQuestnum() && player->questIsSet(object->getQuestnum()-1) &&
        !player->checkStaff("You have already fulfilled that quest.\n")
    ) {
        if(!tryAgain.empty())
            player->print(tryAgain.c_str());
        return(false);
    }

    // Quest results set player ownership, so this will always be true. If this is a trade,
    // this will be true if the object the player is giving the monster is owned by them.
    if(doQuestOwner)
        object->setQuestOwner(player);
    return(true);
}

//*********************************************************************
//                      prepareItemList
//*********************************************************************
// When attempting to trade or complete a quest, we first check to see if they are
// able to receive all of the items as a result. If they cannot, we prevent them
// from completing the trade or quest, give them a reason why and return false. If
// they are able to receive all the items, we return a list of objects they will
// receive in the "objects" parameter.

bool prepareItemList(const std::shared_ptr<Player>& player, std::list<std::shared_ptr<Object> > &objects, std::shared_ptr<Object> &object, const std::shared_ptr<Monster>&  monster, bool isTrade, bool doQuestOwner, int totalBulk) {
    std::list<CatRef>::const_iterator it;
    std::shared_ptr<Object>  toGive=nullptr;
    // if they're getting a random result of a quest or trade,
    // they might be able to try again should it fail
    bool maybeTryAgainMsg = !object->flagIsSet(O_LOAD_ALL) && !object->randomObjects.empty();

    // count the bulk we will be getting rid of if this transaction is successful
    if(totalBulk > 0)
        totalBulk *= -1;

    object->init();

    // if they can't get this object, we need to delete everything
    // we've already loaded so far
    if(!canReceiveObject(player, object, monster, isTrade, doQuestOwner, maybeTryAgainMsg)) {
        object.reset();
        return(false);
    }

    if(object->flagIsSet(O_LOAD_ALL) && !object->randomObjects.empty()) {
        // in this case, we give them a lot of items
        for(it = object->randomObjects.begin(); it != object->randomObjects.end(); it++) {
            if(!loadObject(*it, toGive))
                continue;
            objects.push_back(toGive);
            totalBulk += toGive->getActualBulk();
            // if they can't get this object, we need to delete everything
            // we've already loaded so far
            if(!canReceiveObject(player, toGive, monster, isTrade, doQuestOwner, false)) {
                object.reset();
                return(false);
            }
        }
        // they aren't actually getting the "object" item
        object.reset();
    } else {
        // the simple, common scenario
        objects.push_back(object);
        totalBulk += object->getActualBulk();
    }

    // check total bulk: if they can't get the objects, we need to delete everything
    // we've already loaded so far
    if(player->tooBulky(totalBulk)) {
        if(isTrade || doQuestOwner)
            player->print("What %N wants to give you is too bulky for you.\n", monster.get());
        return(false);
    }

    return(true);
}

//*********************************************************************
//                      cmdTrade
//*********************************************************************

int cmdTrade(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Monster> creature=nullptr;
    std::shared_ptr<Object> object=nullptr, trade=nullptr;
    Carry   invTrade, invResult;
    int     i=0, numTrade = cmnd->val[0];
    bool    badNumTrade=false, found=false, hasTrade=false, doQuestOwner=false;
    int     flags = player->displayFlags();

    player->clearFlag(P_AFK);
    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(cmnd->num < 2) {
        player->print("Trade what?\n");
        return(0);
    }

    if(cmnd->num < 3) {
        player->print("Syntax: trade [num] <item> <monster>\n");
        return(0);
    }

    creature = player->getParent()->findMonster(player, cmnd, 2);
    if(!creature) {
        *player << "That is not here.\n";
        return(0);
    }

    if(!creature->flagIsSet(M_TRADES)) {
        player->print("You can't trade with %N.\n", creature.get());
        return(0);
    }

    object = player->findObject(player, cmnd, 1);

    if(!object) {
        *player << "You don't have that.\n";
        return(0);
    }


    if(object->flagIsSet(O_BROKEN_BY_CMD)) {
        player->print("Who would want that? It's broken!\n");
        return(0);
    }

    if(!Faction::willDoBusinessWith(player, creature->getPrimeFaction())) {
        player->print("%M refuses to do business with you.\n", creature.get());
        return(0);
    }

    for(i=0; i<5; i++) {
        // See if they have ANY items to trade - we'll give them a special message if they don't
        if(!creature->carry[i].info.id)
            continue;
        hasTrade = true;
        // If this the item we want?
        if(creature->carry[i].info != object->info)
            continue;

        found = true;
        invTrade = creature->carry[i];
        invResult = creature->carry[i+5];

        if(invTrade.numTrade > 1 && invTrade.numTrade != numTrade) {
            // If we found a match, but it didnt have all the items we wanted,
            // flag this so we can print a message about it later.
            badNumTrade = true;
        } else {
            // If we found an exact match, stop now and ignore any previous partial matches
            badNumTrade = false;
            break;
        }
    }

    if(!hasTrade) {
        player->print("%M has nothing to trade.\n", creature.get());
        return(0);
    }

    if(!found || !object->info.id || ((object->getShotsCur() <= object->getShotsMax()/10) && object->getType() != ObjectType::MISC)) {
        player->print("%M says, \"I don't want that!\"\n", creature.get());
        failTrade(player, object, creature);
        return(0);
    }

    if(badNumTrade) {
        player->print("%M says, \"That's now how many I want!\"\n", creature.get());
        failTrade(player, object, creature);
        return(0);
    }

    if(!object->isQuestOwner(player)) {
        player->print("%M says, \"That doesn't belong to you!\"\n", creature.get());
        failTrade(player, object, creature);
        return(0);
    }

    if(!loadObject((invResult.info), trade)) {
        player->print("%M says, \"Sorry, I don't have anything for you in return for that. Keep it.\"\n", creature.get());
        failTrade(player, object, creature);
        return(0);
    }

    std::list<std::shared_ptr<Object> > objects;
    std::list<std::shared_ptr<Object> >::const_iterator oIt;

    // If the quest owner is set on the item we are giving, set the quest owner
    // on the item(s) we are receiving.
    doQuestOwner = (!object->getQuestOwner().empty());

    if(!prepareItemList(player, objects, trade, creature, true, doQuestOwner, object->getActualBulk())) {
        // in the event of failure, prepareItemList() will delete the trade object
        failTrade(player, object, creature);
        return(0);
    }

    // are they trying to trade multiple objects
    if(numTrade > 1) {
        int numObjects=0;

        ObjectSet toDelSet;
        ObjectSet::iterator it;
        std::shared_ptr<Object>obj = nullptr;

        // find the first occurance of the object in their inventory
        for( it = player->objects.begin() ; it != player->objects.end() ; ) {
            obj = (*it);
            if(obj->info == object->info)
                break;
            else
                it++;
        }

        if(it == player->objects.end()) {
            player->print("%M gets confused and cannot trade for that right now.\n", creature.get());
            failTrade(player, object, creature, trade);
            return(0);
        }
        while( it != player->objects.end() && numObjects != numTrade && (*it)->info == object->info) {
            obj = (*it);
            numObjects++;
            if(!obj->isQuestOwner(player)) {
                player->print("%M says, \"That doesn't belong to you!\"\n", creature.get());
                failTrade(player, object, creature, trade);
                return(0);
            }
            toDelSet.insert(obj);
            it++;
        }

        if(numObjects != numTrade) {
            *player << "You don't have that many items.\n";
            failTrade(player, object, creature, trade, false);
            return(0);
        }

        // if everything is successful, start removing objects
        for(const auto& toDelete : toDelSet) {
            player->delObj(toDelete, true);
            creature->addObj(toDelete);
            numObjects--;
        }
    } else {
        // the simple, common scenario
        player->delObj(object, true);
        creature->addObj(object);
    }

    player->printColor("%M says, \"Thank you for retrieving %s for me.\n", creature.get(), object->getObjStr(player, flags, numTrade).c_str());
    player->printColor("For it, I will reward you.\"\n");

    broadcast(player->getSock(), player->getParent(), "%M trades %P to %N.", player.get(),object.get(),creature.get());


    for(oIt = objects.begin(); oIt != objects.end(); oIt++) {
        player->printColor("%M gives you %P in trade.\n", creature.get(), (*oIt).get());
        (*oIt)->setDroppedBy(creature, "MonsterTrade");
        doGetObject(*oIt, player);
    }

    if(creature->ttalk[0])
        player->print("%M says, \"%s\"\n", creature.get(), creature->ttalk);

    return(0);
}


//*********************************************************************
//                      cmdAuction
//*********************************************************************

int cmdAuction(const std::shared_ptr<Player>& player, cmd* cmnd) {
    int         i=0, flags=0;
    long        amnt=0, batch=1, each=0;
    std::shared_ptr<Object> object=nullptr;

    player->clearFlag(P_AFK);

    if(player->getClass() == CreatureClass::BUILDER || player->inJail()) {
        *player << "You cannot do that.\n";
        return(PROMPT);
    }

    if(cmnd->num < 3) {
        player->print("Syntax: auction (item)[which|#num] $(amount) [each|total]\n");
        player->print("        auction self $(amount)\n");
        return(0);
    }

    if(player->isEffected("mist") || player->isInvisible()) {
        *player << "You must be visible to everyone in order to auction.\n";
        return(0);
    }
    if(player->getLevel() < 7 && !player->isCt()) {
        *player << "You must be at least level 7 to auction items.\n";
        return(0);
    }
    if(player->flagIsSet(P_CANT_BROADCAST)) {
        player->print("Due to abuse, you no longer have that privilege.\n");
        return(0);
    }

    // use i to point to where the money will be
    i = 2;

    if(strcmp(cmnd->str[1], "self") != 0) {
        object = player->findObject(player, cmnd, 1);
        if(!object) {
            *player << "That object is not in your inventory.\n";
            return(0);
        }

        // batch auction
        if(cmnd->str[2][0] == '#') {
            i = 3;
            batch = toNum<long>(&cmnd->str[2][1]);

            if(batch < 1) {
                *player << "You must sell at least 1 item.\n";
                return(0);
            }
            if(batch > 100) {
                player->print("That's too many!\n");
                return(0);
            }
            if(cmnd->num==5 && !strcmp(cmnd->str[4], "each"))
                each = 1;
            else
                each = -1;
        }
    }

    if(cmnd->str[i][0] != '$') {
        player->print("Syntax: auction (item)[which|#num] $(amount) [each|total]\n");
        player->print("        auction self $(amount)\n");
        return(0);
    }

    amnt = atol(&cmnd->str[i][1]);

    if(amnt <= 0) {
        *player << "A price must be specified to auction.\n";
        return(0);
    }
    if (amnt < 500 && !player->checkStaff("Items must be auctioned for at least 500 gold.\n"))
        return (0);


    if(player->isEffected("detect-magic"))
        flags |= MAG;



    std::shared_ptr<Player> target=nullptr;
    for(const auto& p : gServer->players) {
        target = p.second;

        if(!target->isConnected())
            continue;
        if(target->flagIsSet(P_NO_AUCTIONS))
            continue;

        if(!strcmp(cmnd->str[1], "self")) {
            target->printColor("*** %M is auctioning %sself for %ldgp.\n", player.get(), player->himHer(), amnt);
            continue;
        }
        target->printColor("*** %M is auctioning %s for %ldgp%s.\n", player.get(), object->getObjStr(nullptr, flags, batch).c_str(), amnt, each ? each==1 ? " each" : " total" : "");
    }

    return(0);
}

//*********************************************************************
//                      setLastPawn
//*********************************************************************

void Player::setLastPawn(const std::shared_ptr<Object>&  object) {
    // uniques and quests cannot be reclaimed
    if(object && (object->flagIsSet(O_UNIQUE) || object->getQuestnum())) {
        return;
    }
    lastPawn = object;
}

//*********************************************************************
//                      restoreLastPawn
//*********************************************************************

bool Player::restoreLastPawn() {
    if(!getRoomParent()->flagIsSet(R_PAWN_SHOP) || !getRoomParent()->flagIsSet(R_DUMP_ROOM)) {
        *this << "You must be in a pawn shop to reclaim items.\n";
        return(false);
    }
    if(!lastPawn) {
        *this << "You have no items to reclaim.\n";
        return(false);
    }
    auto pThis = Containable::downcasted_shared_from_this<Player>();
    printColor("You negotiate with the shopkeeper to reclaim %P.\n", lastPawn.get());
    if(lastPawn->refund[GOLD] > coins[GOLD]) {
        *this << "You don't have enough money to reclaim that item!\n";
        return(false);
    }
    if(!Unique::canGet(pThis, lastPawn) || !Lore::canHave(pThis, lastPawn)) {
        *this << "You currently cannot reclaim that limited object.\n";
        return(false);
    }
    coins.sub(lastPawn->refund);
    *this << ColorOn << "You give the shopkeeper " << lastPawn->refund[GOLD] << " gold and reclaim " << lastPawn << ".\n";
    Server::logGold(GOLD_OUT, pThis, lastPawn->refund, lastPawn, "Reclaim");

    lastPawn->setFlag(O_RECLAIMED);
    lastPawn->refund.zero();
    doGetObject(lastPawn, pThis, true, true);
    lastPawn = nullptr;
    return(true);
}

//*********************************************************************
//                      cmdReclaim
//*********************************************************************

int cmdReclaim(const std::shared_ptr<Player>& player, cmd* cmnd) {
    player->restoreLastPawn();
    return(0);
}


//********************************************************************
//                      doHaggling
//********************************************************************
// This function is called whenever money changes hands for a player. It is specifically
// written for clerics of the god Jakar, but is being designed so it can be applied to
// all players in general if desired. It will make a player haggle to get more money if
// selling, and pay less money if buying, up to 30% discount. -TC

void Creature::doHaggling(const std::shared_ptr<Creature>&vendor, const std::shared_ptr<Object>&  object, int trans) {
    int     chance=0, npcVendor=0, roll=0, discount=0, lck=0;
    float   amt=0.0, percent=0.0;
    long    modAmt = 0, val = 0;


    if(vendor)
        npcVendor = 1;

    if(trans != 0 && trans != 1) {
        broadcast(::isDm, "^g*** Invalid function call: doHaggle. No transaction type specified. Aborting.");
        return;
    }

    /* Remove this to allow all players to haggle */
    if( !(  isCt() ||
            getClass() == CreatureClass::BARD ||
            (getClass() == CreatureClass::CLERIC && getDeity() == JAKAR) ) )
        return;

    if(chkSave(LCK, nullptr, -1))
        lck = 25;
    else
        lck = 0;

    if(npcVendor) {
        chance = 50 + lck + 5*((int)vendor->getLevel() - (int)getLevel());

    } else
        chance = std::min(95,(getLevel()*2 + saves[LCK].chance/10));

    // If this item was sold and reclaimed, no further haggling
    if(trans == SELL && object->flagIsSet(O_RECLAIMED)) {
        chance = 0;
    }


    // If we've refunded an item in this shop, no haggling until having bought a full priced item again

    if(trans == BUY) {
        if(getAsPlayer() && flagIsSet(P_JUST_REFUNDED))
            chance = 0;
        if(chance > 0 && getRoomParent() && getRoomParent()->getAsUniqueRoom() ) {
            std::shared_ptr<Player> player = getAsPlayer();
            if( player && !player->storesRefunded.empty()) {
                if(player->hasRefundedInStore(getRoomParent()->getAsUniqueRoom()->info)) {
                    chance = 0;
                }
            }
        }
    }
    if(trans == SELL)
        val = std::min<unsigned long>(MAXPAWN,object->value[GOLD]/2);
    else
        val = object->value[GOLD]/2;

    roll = Random::get(1,100);

    if(isCt()) {
        print("haggle chance: %d\n", chance);
        print("roll: %d\n", roll);
    }
    if(roll <= chance) {

        discount = std::max(1,Random::get(getLevel()/2, 1+getLevel()));

        percent = (float)(discount);
        percent /= 100;

        amt = (float)val;
        amt *= percent;

        modAmt = (long)amt;

        if(modAmt >= 10000)
            modAmt += (Random::get(1,10000)-1);
        else if(modAmt >= 100)
            modAmt += (Random::get(1,1000)-1);
        else if(modAmt >= 100)
            modAmt += (Random::get(1,100)-1);
        else if(modAmt >= 10)
            modAmt += (Random::get(1,10)-1);


        if(isCt()) {
            print("discount: %d%\n", discount);
            print("percent: %f, amt: %f\n", percent, amt);
            print("modAmt: %ld\n", modAmt);
        }

        if(modAmt) {
            if(npcVendor)
                broadcast(getSock(), getRoomParent(), "%M haggles over prices with %N.", this, vendor.get());
            switch(trans) {
            case BUY:
                printColor("^yYour haggling skills saved you %ld gold coins.\n", modAmt);
                coins.add(modAmt, GOLD);
                object->refund.sub(modAmt, GOLD);
                break;
            case SELL:
                printColor("^yYour haggling skills gained you an extra %ld gold coins.\n", modAmt);
                coins.add(modAmt, GOLD);
                object->refund.add(modAmt, GOLD);
                break;
            default:
                break;
            }
        }

    }
}

bool Player::hasRefundedInStore(CatRef& store) {
    if( storesRefunded.empty())
        return(false);
    else {
        std::list<CatRef>::const_iterator it;
        for(it = storesRefunded.begin() ; it != storesRefunded.end() ; it++) {
            if( *it == store) {
                return(true);
            }
        }
    }
    return(false);
}
void Player::addRefundedInStore(CatRef& store) {
    if(hasRefundedInStore(store))
        return;
    else
        storesRefunded.push_back(store);
}
void Player::removeRefundedInStore(CatRef& store) {
    std::list<CatRef>::iterator it;
    for(it = storesRefunded.begin() ; it != storesRefunded.end() ; it++) {
        if( *it == store) {
            storesRefunded.erase(it);
            return;
        }
    }
}
