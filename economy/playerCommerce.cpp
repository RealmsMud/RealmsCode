/*
 * playerCommerce.cpp
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
#include "commerce.hpp"
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
#include "utils.hpp"                   // for MAX, MIN
#include "xml.hpp"                     // for loadRoom, loadObject, loadPlayer


const int SHOP_FOUND = 1,
        SHOP_STOCK = 2,
        SHOP_PRICE = 3,
        SHOP_REMOVE = 4,
        SHOP_NAME = 5,
        SHOP_GUILD = 6;

const char *shopSyntax =
        "Syntax: shop ^e<^xfound^e> <^cexit name^e>^x\n"
        "             ^e<^xstock^e> <^citem^e> [^c$price^e]^x\n"
        "             ^e<^xprice^e> <^citem^e> <^c$price^e>^x\n"
        "             ^e<^xremove^e> <(^citem^e>^x\n"
        "             ^e<^xname^e> <^cshop name^e>^x\n"
        "             ^e<^xguild^e> <^xassign^e|^xremove^e>^x\n"
        "             ^e<^xsurvey^e> [^call^e]^x\n"
        "             ^e<^xhelp^e>^x\n"
        "\n"
        "             ^e<^xhire^e> <^cplayer^e>       -^x add a partial owner\n"
        "             ^e<^xfire^e> <^cplayer^e>       -^x remove a partial owner\n"
        "             ^e<^xflags^e>^x               ^e-^x view / set flags on partial owners\n"
        "             ^e<^xlog^e>^x ^e<^xdelete^e|^xrecord^e>^x ^e-^x view / delete / set log type\n"
        "             ^e<^xdestroy^e>^x             ^e-^x close this shop down\n"
        "\n";

//*********************************************************************
//                      shopProfit
//*********************************************************************

long shopProfit(const std::shared_ptr<Object>&  object) {
    return((long)(object->getShopValue() - (TAX * object->getShopValue())));
}


//*********************************************************************
//                      getCondition
//*********************************************************************

std::string getCondition(const std::shared_ptr<Object>&  object) {
    int percent = -1;

    // possible division by 0
    if(object->getShotsCur() > 0 && object->getShotsMax() > 0)
        percent = 100 * object->getShotsCur() / object->getShotsMax();

    if( object->getType() == ObjectType::WEAPON ||
        object->getType() == ObjectType::ARMOR ||
        object->getType() == ObjectType::LIGHTSOURCE ||
        object->getType() == ObjectType::WAND ||
        object->getType() == ObjectType::KEY ||
        object->getType() == ObjectType::POISON
            ) {
        if(percent >= 90)
            return("Pristine");
        else if(percent >= 75)
            return("Excellent");
        else if(percent >= 60)
            return("Good");
        else if(percent >= 45)
            return("Slightly Scratched");
        else if(percent >= 30)
            return("Badly Dented");
        else if(percent >= 10)
            return("Horrible");
        else if(percent >= 1)
            return("Terrible");
        else if(object->getShotsCur() < 1)
            return("Broken");
    }
    return("Pristine");
}

//*********************************************************************
//                      setupShop
//*********************************************************************

void setupShop(Property *p, const std::shared_ptr<Player>& player, const Guild* guild, const std::shared_ptr<UniqueRoom>&shop, const std::shared_ptr<UniqueRoom>& storage) {
    const char* shopSuffix = "'s Shop of Miscellaneous Wares";
    const char* shopStorageSuffix = "'s Shop Storage Room";
    if(guild)
        shop->setName(guild->getName() + shopSuffix);
    else
        shop->setName(player->getName() + shopSuffix);

    // handle shop description
    shop->setShortDescription("You are in ");
    if(guild)
        shop->appendShortDescription(guild->getName());
    else
        shop->appendShortDescription(player->getName());
    shop->appendShortDescription("'s shop of magnificent wonders.");

    // switch to long description
    shop->setLongDescription("In ");
    if(guild)
        shop->appendLongDescription(guild->getName());
    else
        shop->appendLongDescription(player->getName());

    shop->appendLongDescription("'s shop, you may find many things which you need.\n");
    if(guild)
        shop->appendLongDescription("The guild");
    else
        shop->appendLongDescription(player->upHeShe());

    shop->appendLongDescription(" should be keeping it well stocked in order to maintain\ngood business. Should you have any complaints, you should go to\nthe nearest post office and mudmail the ");
    if(guild) {
        shop->appendLongDescription("the guild contact, ");
        shop->appendLongDescription(player->getName());
        shop->appendLongDescription(".");
    } else
        shop->appendLongDescription("proprietor.");

    if(guild)
        storage->setName(guild->getName() + shopStorageSuffix);
    else
        storage->setName(player->getName() + shopStorageSuffix);

    shop->saveToFile(0);
    storage->saveToFile(0);

    if(p)
        p->setName(shop->getName());
}

//*********************************************************************
//                      shopAssignGuildMessage
//*********************************************************************

void shopAssignGuildMessage(Property* p, const std::shared_ptr<Player>& player, const Guild* guild) {
    player->print("This property is now associated with the guild %s.\n", guild->getName().c_str());
    p->appendLog("", "This property is now associated with the guild %s.", guild->getName().c_str());
    broadcastGuild(guild->getNum(), 1, "### %s has assigned the shop located at %s as partially run by the guild.",
                   player->getCName(), p->getLocation().c_str());
}

//*********************************************************************
//                      shopAssignGuild
//*********************************************************************

bool shopAssignGuild(Property *p, const std::shared_ptr<Player>& player, const Guild* guild, const std::shared_ptr<UniqueRoom>& shop, const std::shared_ptr<UniqueRoom>& storage, bool showMessage) {
    if(!player->getGuild()) {
        *player << "You are not part of a guild.\n";
        return(false);
    }
    if(p->getGuild() == player->getGuild()) {
        *player << "This property is already assigned to your guild.\n";
        return(false);
    }
    if(!guild) {
        *player << "There was an error loading your guild.\n";
        return(false);
    }

    p->setGuild(guild->getNum());
    setupShop(p, player, guild, shop, storage);

    if(showMessage)
        shopAssignGuildMessage(p, player, guild);
    return(true);
}

//*********************************************************************
//                      shopRemoveGuild
//*********************************************************************

void shopRemoveGuild(Property *p, std::shared_ptr<Player> player, std::shared_ptr<UniqueRoom> shop, std::shared_ptr<UniqueRoom> storage) {
    p->setGuild(0);
    p->appendLog("", "This property is no longer associated with a guild.");

    if(!shop || !storage) {
        CatRef cr = p->ranges.front().low;
        if(loadRoom(cr, shop)) {
            cr.id++;
            if(!loadRoom(cr, storage))
                return;
        }
    }

    setupShop(p, player, nullptr, shop, storage);
}



//*********************************************************************
//                      shopStorageRoom
//*********************************************************************

CatRef shopStorageRoom(const std::shared_ptr<const UniqueRoom>& shop) {
    CatRef cr;
    if(shop->getTrapExit().id && !shop->flagIsSet(R_LOG_INTO_TRAP_ROOM))
        cr = shop->getTrapExit();
    else {
        cr = shop->info;
        cr.id++;
    }
    return(cr);
}

//*********************************************************************
//                      playerShopSame
//*********************************************************************

bool playerShopSame(std::shared_ptr<Player> player, const std::shared_ptr<Object>&  obj1, const std::shared_ptr<Object>&  obj2) {
    return( obj1->showAsSame(player, obj2) &&
            getCondition(obj1) == getCondition(obj2) &&
            obj1->getShopValue() == obj2->getShopValue()
    );
}


//*********************************************************************
//                      tooManyItemsInShop
//*********************************************************************

bool tooManyItemsInShop(const std::shared_ptr<Player>& player, const std::shared_ptr<UniqueRoom>& storage) {
    int numObjects=0, numLines=0;
    std::shared_ptr<Object>obj;
    ObjectSet::iterator it;
    for(it = storage->objects.begin() ; it != storage->objects.end() ; ) {
        obj = (*it++);
        numObjects++;
        numLines++;


        while(it != storage->objects.end()) {
            if(playerShopSame(nullptr, obj, (*it))) {
                numObjects++;
                it++;
            } else
                break;
        }

        if(numObjects >= gConfig->getShopNumObjects()) {
            player->print("There are too many items in this shop (%d/%d).\n", numObjects, gConfig->getShopNumObjects());
            player->print("Please remove some before continuing.\n");
            return(true);
        } else if ( numLines >= gConfig->getShopNumLines()) {
            player->print("The number of items in your shop take up too many lines: (%d/%d).\n", numLines, gConfig->getShopNumLines());
            player->print("Please remove some before continuing.\n");
            return(true);

        }
    }

    return(false);
}

//*********************************************************************
//                      cmdShop
//*********************************************************************

bool canBuildShop(const std::shared_ptr<Player>& player, const std::shared_ptr<UniqueRoom>& room) {
    // works for most people
    if(room->flagIsSet(R_BUILD_SHOP))
        return(true);

    // guild foyers can accomodate shops, but only for members of the guild
    if( player->getGuild() &&
        player->getGuildRank() == GUILD_MASTER &&
        room->flagIsSet(R_GUILD_OPEN_ACCESS) &&
        room->info.isArea("guild"))
    {
        Property* p = gConfig->getProperty(room->info);
        if(p->getType() == PROP_GUILDHALL && player->getGuild() == p->getGuild()) {
            // already hosts a shop?
            for(const auto& exit : room->exits) {
                if(exit->target.room.isArea("shop"))
                    return(false);
            }

            // If we pass these checks, too, then we can build a shop here
            return(true);
        }
    }

    return(false);
}

//*********************************************************************
//                      cmdShop
//*********************************************************************

int cmdShop(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<UniqueRoom> room = player->getUniqueRoomParent(), storage = nullptr;
    std::shared_ptr<Object> deed = nullptr;
    int action = 0;
    int flags = 0;
    size_t len = strlen(cmnd->str[1]);

    const Guild* guild=nullptr;
    if(player->getGuild())
        guild = gConfig->getGuild(player->getGuild());

    if(!len) {
        player->printColor(shopSyntax);
        return(0);
    }

    if(!strncmp(cmnd->str[1], "help", len)) {
        strcpy(cmnd->str[1], "shops");
        return(cmdHelp(player, cmnd));
    } else if(
            !strcmp(cmnd->str[1], "hire") ||
            !strcmp(cmnd->str[1], "fire") ||
            !strcmp(cmnd->str[1], "flags") ||
            !strcmp(cmnd->str[1], "log") ||
            !strcmp(cmnd->str[1], "destroy")
            ) {
        // give them a cross-command shortcut
        return(cmdProperties(player, cmnd));
    }

    if(!needUniqueRoom(player))
        return(0);

    if(!strncmp(cmnd->str[1], "survey", len)) {
        if(!strcmp(cmnd->str[2], "all")) {
            *player << "Searching for suitable shop locations in this city.\n";
            findRoomsWithFlag(player, player->getUniqueRoomParent()->info, R_BUILD_SHOP);
        } else {
            if(!canBuildShop(player, room))
                *player << "You are unable to build a shop here.\n";
            else
                *player << "This site is open for shop construction.\n";
        }
        return(0);
    }

    if(cmnd->num < 3) {
        player->printColor(shopSyntax);
        return(0);
    }

    flags |= MAG;

    if(!strncmp(cmnd->str[1], "found", len)) {
        if(player->getLevel() < MINSHOPLEVEL && !player->isDm()) {
            player->print("You must be level %d to found a shop!\n", MINSHOPLEVEL);
            return(0);
        }
        if(!canBuildShop(player, room)) {
            *player << "This section of town is not zoned for shops.\n";
            return(0);
        }

        // See if the deed can be built in this room
        CatRef cr = room->info;
        // If we're inside a guild, check the out exit instead
        if(room->info.isArea("guild")) {
            for(const auto& exit : room->exits) {
                if(!exit->target.room.isArea("guild")) {
                    cr = exit->target.room;
                    break;
                }
            }
        }

        for(const auto& obj : player->objects ) {
            if(obj->getName() == "shop deed")
                if(obj->deed.belongs(cr)) {
                    deed = obj;
                    break; // We got one!
                }
        }
        if(!deed) {
            player->print("You need a shop deed to build a shop in this town!\nVisit the local real estate office.\n");
            return(0);
        }
        action = SHOP_FOUND;
    } else if(  !strncmp(cmnd->str[1], "stock", len) ||
                !strncmp(cmnd->str[1], "add", len) ||
                !strcmp(cmnd->str[1], "sell")
            )
        action = SHOP_STOCK;
    else if(!strncmp(cmnd->str[1], "price", len) || !strncmp(cmnd->str[1], "value", len))
        action = SHOP_PRICE;
    else if(!strncmp(cmnd->str[1], "remove", len))
        action = SHOP_REMOVE;
    else if(!strncmp(cmnd->str[1], "name", len))
        action = SHOP_NAME;
    else if(!strncmp(cmnd->str[1], "guild", len))
        action = SHOP_GUILD;


    Property* p = gConfig->getProperty(room->info);

    if(action != SHOP_FOUND) {
        if( !p ||
            p->getType() != PROP_SHOP ||
            (   !player->isDm() &&
                !p->isOwner(player->getName()) &&
                !p->isPartialOwner(player->getName())
            )
                ) {
            player->print("This is not your shop!\n");
            return(0);
        }
    }

    PartialOwner partial;
    PartialOwner* po=nullptr;
    if(p && !p->isOwner(player->getName())) {
        po = p->getPartialOwner(player->getName());
        // get default flags if not on the list
        if(!po) {
            partial.defaultFlags(p->getType());
            po = &partial;
        }
    }

    if(!loadRoom(shopStorageRoom(room), storage)) {
        if( action == SHOP_STOCK ||
            action == SHOP_REMOVE ||
            action == SHOP_PRICE ||
            action == SHOP_NAME
                ) {
            player->print("The storage room to this shop could not be loaded!\n");
            return(0);
        }
    }

    // see if they have the authorization to do the command they want to do
    if(po) {
        if(action == SHOP_NAME || action == SHOP_GUILD) {
            *player << "Only the owner of this shop may perform that action.\n";
            return(0);
        }
        if( (action == SHOP_STOCK && !po->flagIsSet(PROP_SHOP_CAN_STOCK)) ||
            (action == SHOP_REMOVE && !po->flagIsSet(PROP_SHOP_CAN_REMOVE)) ||
            (action == SHOP_PRICE && (!po->flagIsSet(PROP_SHOP_CAN_STOCK) || !po->flagIsSet(PROP_SHOP_CAN_REMOVE)))
                ) {
            if(!player->checkStaff("You don't have the proper authorization to perform that action.\n"))
                return(0);
        }
    }

    bool limited=false;
    if(action == SHOP_FOUND) {
        std::shared_ptr<UniqueRoom> shop=nullptr;
        storage = nullptr;

        std::string xname = getFullstrText(cmnd->fullstr, 2);
        std::replace(xname.begin(), xname.end(), '_', ' ');

        if(!Property::goodExit(player, room, "Shop", xname))
            return(0);

        CatRef  cr = gConfig->getAvailableProperty(PROP_SHOP, 2);
        if(cr.id < 1) {
            player->print("Sorry, you can't found new shops right now, try again later!\n");
            return(0);
        }

        logn("log.shop", "%s just founded a shop! (%s - %s).\n", player->getCName(), xname.c_str(), cr.displayStr().c_str());

        shop = std::make_shared<UniqueRoom>();
        shop->info = cr;

        link_rom(shop, room->info, "out");
        link_rom(room, shop->info, xname.c_str());
        shop->setFlag(R_SHOP);
        shop->setFlag(R_INDOORS);

        storage = std::make_shared<UniqueRoom>();
        storage->info = shop->info;
        storage->info.id++;

        link_rom(storage, shop->info, "out");
        storage->setFlag(R_SHOP_STORAGE);
        storage->setTrapExit(shop->info);

        // everything to do with the player's name
        // sned in a guild if they're inside a guild hall

        room->saveToFile(0);

        p = new Property;
        p->found(player, PROP_SHOP);

        p->addRange(shop->info.area, shop->info.id, shop->info.id+1);

        if(!room->info.isArea("guild")) {
            setupShop(p, player, nullptr, shop, storage);
        } else {
            shopAssignGuild(p, player, guild, shop, storage, false);
        }

        gConfig->addProperty(p);

        player->print("Congratulations! You are now the owner of a brand new shop.\n");
        logn("log.shops", "*** %s just built a shop! (%s - %s) (Shop %s).\n",
             player->getCName(), room->info.displayStr().c_str(), xname.c_str(), shop->info.displayStr().c_str());
        broadcast(player->getSock(), player->getParent(), "%M just opened a shop!", player.get() );
        if(!player->flagIsSet(P_DM_INVIS))
            broadcast("### %s just opened a shop! It's located at: %s.", player->getCName(), room->getCName());

        if(room->info.isArea("guild"))
            shopAssignGuildMessage(p, player, guild);

        shop.reset();
        storage.reset();
        player->delObj(deed, true);
        deed.reset();
        player->getUniqueRoomParent()->clearFlag(R_BUILD_SHOP);
        player->getUniqueRoomParent()->setFlag(R_WAS_BUILD_SHOP);
        player->getUniqueRoomParent()->saveToFile(0);
        return(0);
    } else if(action == SHOP_GUILD) {
        if(!strncmp(cmnd->str[2], "assign", strlen(cmnd->str[2]))) {
            if(!shopAssignGuild(p, player, guild, player->getUniqueRoomParent(), storage, true))
                return(0);
        } else if(!strncmp(cmnd->str[2], "remove", strlen(cmnd->str[2]))) {
            if(!p->getGuild()) {
                *player << "This property is not assigned to a guild.\n";
                return(0);
            }
            if(shopStaysWithGuild(player->getUniqueRoomParent())) {
                *player << "This property was constructed inside the guild hall.\nIt cannot be removed from the guild.\n";
                return(0);
            }
            broadcastGuild(p->getGuild(), 1, "### %s's shop located at %s is no longer partially run by the guild.",
                           player->getCName(), p->getLocation().c_str());
            shopRemoveGuild(p, player, player->getUniqueRoomParent(), storage);
            *player << "This property is no longer associated with a guild.\n";
        } else {
            *player << "Command not understood.\n";
            player->printColor("You may ^Wassign^x this shop to a guild or ^Wremove^x it from a guild.\n");
            return(0);
        }

        gConfig->saveProperties();

    } else if(action == SHOP_STOCK) {
        std::shared_ptr<Object>obj = player->findObject(player, cmnd, 2);
        int value=0;

        if(!obj) {
            *player << "You don't have any object with that name.\n";
            return(0);
        }

        if(obj->flagIsSet(O_NO_DROP) || obj->getType() == ObjectType::LOTTERYTICKET) {
            *player << "That item cannot be stocked.\n";
            return(0);
        }

        if(!obj->objects.empty()) {
            *player << "You need to empty it before it can be stocked.\n";
            return(0);
        }

        if(obj->flagIsSet(O_STARTING)) {
            *player << "Starting items cannot be stocked.\n";
            return(0);
        }

        limited = Limited::isLimited(obj);
        if(limited && (!p || !p->isOwner(player->getCName()))) {
            *player << "You may only handle limited items in shops where you are the primary owner.\n";
            return(0);
        }

        if(obj->getShopValue()) {
            player->print("Why would you want to sell second hand trash?\n");
            return(0);
        }
        if(obj->getShotsCur() < 1 && obj->getShotsCur() != obj->getShotsMax() && obj->getType() != ObjectType::CONTAINER) {
            player->print("Why would you want to sell such trash in your shop?\n");
            return(0);
        }

        if(tooManyItemsInShop(player, storage))
            return(0);

        if(cmnd->num > 3 && cmnd->str[3][0] == '$') {
            value = atoi(cmnd->str[3]+1);
            value = MAX(0, value);
        }
        if(!value)
            value = obj->value[GOLD];
        obj->setShopValue(value);
        obj->setFlag(O_PERM_ITEM);

        player->delObj(obj);
        obj->addToRoom(storage);
        p->appendLog(player->getName(), "%s stocked %s for $%d.", player->getCName(), obj->getObjStr(nullptr, flags, 1).c_str(), obj->getShopValue());
        player->printColor("You stock %s in the store for $%d.\n", obj->getObjStr(nullptr, flags, 1).c_str(), obj->getShopValue());
        broadcast(player->getSock(), player->getParent(), "%M just stocked something in this store.", player.get());
        // obj->shopValue
        if(limited)
            player->save(true);
        storage->saveToFile(0);
    } else if(action == SHOP_PRICE) {
        std::shared_ptr<Object>obj = storage->findObject(player, cmnd, 2);
        int value=0;

        if(!obj) {
            *player << "You're not selling anything with that name.\n";
            return(0);
        }

        if(cmnd->str[3][0] == '$') {
            value = atoi(cmnd->str[3]+1);
            value = MAX(0, value);
        }
        if(cmnd->str[3][0] != '$' || !value) {
            player->printColor(shopSyntax);
            return(0);
        }
        obj->setShopValue(value);
        p->appendLog(player->getName(), "%s set the price for %s to $%d.", player->getCName(), obj->getObjStr(nullptr, flags, 1).c_str(), obj->getShopValue());
        player->printColor("You set the price for %s to $%d.\n", obj->getObjStr(nullptr, flags, 1).c_str(), obj->getShopValue());
        broadcast(player->getSock(), player->getParent(), "%M just updated the prices in this store.", player.get());
    } else if(action == SHOP_REMOVE) {
        std::shared_ptr<Object>obj = storage->findObject(player, cmnd, 2);

        if(!obj) {
            *player << "You're not selling anything with that name.\n";
            return(0);
        }

        if(player->getWeight() + obj->getActualWeight() > player->maxWeight()) {
            *player << "That'd be too heavy for you to carry right now.\n";
            return(0);
        }

        if(player->tooBulky(obj->getActualBulk())) {
            *player << "That would be too bulky for you to carry.\n";
            return(0);
        }

        limited = Limited::isLimited(obj);
        if(limited && (!p || !p->isOwner(player->getName()))) {
            *player << "You may only handle unique items in shops where you are the primary owner.\n";
            return(0);
        }

        obj->deleteFromRoom();
        player->addObj(obj);
        p->appendLog(player->getName(), "%s removed %s.", player->getCName(), obj->getObjStr(nullptr, flags, 1).c_str());
        player->printColor("You remove %s from your store.\n", obj->getObjStr(nullptr, flags, 1).c_str());
        broadcast(player->getSock(), player->getParent(), "%M just removed something from this store.", player.get());

        obj->clearFlag(O_PERM_ITEM);
        obj->setShopValue(0);
        if(limited)
            player->save(true);
        storage->saveToFile(0);
    } else if(action == SHOP_NAME) {
        std::string name = getFullstrText(cmnd->fullstr, 2);

        if(!Property::goodNameDesc(player, name, "Rename shop to what?", "room name"))
            return(0);

        if(!p->getGuild()) {
            if(name.find(player->getName()) == std::string::npos) {
                *player << "Your shop name must contain your name.\n";
                return(0);
            }
        } else {
            if(!guild) {
                *player << "Error loading guild.\n";
                return(0);
            }
            if(name.find(guild->getName()) == std::string::npos) {
                *player << "Your shop name must contain your guild's name.\n";
                return(0);
            }
        }
        player->getUniqueRoomParent()->setName(name.c_str());
        p->setName(name);
        p->appendLog(player->getName(), "%s renamed the shop to %s.", player->getCName(), player->getUniqueRoomParent()->getCName());
        logn("log.shops", "%s renamed shop %s to %s.\n",
             player->getCName(), player->getUniqueRoomParent()->info.displayStr().c_str(), player->getUniqueRoomParent()->getCName());
        player->print("Shop renamed to '%s'.\n", player->getUniqueRoomParent()->getCName());
        player->getUniqueRoomParent()->saveToFile(0);

        return(0);
    } else {
        player->printColor(shopSyntax);
    }

    return(0);
}

void playerShopList(const std::shared_ptr<Player>& player, Property* p, std::string& filter, std::shared_ptr<UniqueRoom>& storage) {
    std::shared_ptr<Object>  object;
    int num = 0, flags = 0, m, n=0;
    bool owner=false;

    flags |= CAP;
    flags |= MAG;

    if(p->isOwner(player->getName())) {
        player->printPaged("You are selling:");
        owner = true;
    } else {
        std::string ownerName = p->getOwner();
        if(p->getGuild()) {
            const Guild* guild = gConfig->getGuild(p->getGuild());
            ownerName = guild->getName();
        }
        player->printPaged(fmt::format("{} is selling:", ownerName));
        if(p->isPartialOwner(player->getName()))
            owner = true;
    }

    if(!filter.empty())
        player->printPaged(fmt::format(" ^Y(filtering on \"{}\")", filter));

    ObjectSet::iterator it;
    for( it = storage->objects.begin() ; it != storage->objects.end() ; ) {
        object = (*it++);
        n++;
        m=1;

        while(it != storage->objects.end()) {
            if(playerShopSame(player, object, (*it))) {
                m++;
                object = (*it++);
            } else
                break;
        }
        num++;

        if(doFilter(object, filter)) continue;

        if(n == 1)
            player->printPaged("^WNum         Item                                       Price      Condition");
        player->printPaged(fmt::format("{:2}> {} ${:<9} {:<12} {}", num, objShopName(object, m, flags, 50), object->getShopValue(),
                                       getCondition(object), cannotUseMarker(player, object), owner ? fmt::format(" Profit: {}", shopProfit(object)) : ""));
    }
    if(!n)
        player->printPaged("Absolutely nothing!");
    player->donePaging();
}

void playerShopBuy(const std::shared_ptr<Player>& player, cmd* cmnd, Property* p, std::shared_ptr<UniqueRoom>& storage) {
    std::shared_ptr<UniqueRoom> room = player->getUniqueRoomParent();
    std::shared_ptr<Player> owner=nullptr;
    std::shared_ptr<Object> object=nullptr, object2=nullptr;

    Guild*  guild=nullptr;
    int     flags=0, num=0, n=1;
    long    deposit=0;
    bool    online=false;

    if(p->isOwner(player->getName())) {
        player->print("Why would you want to buy stuff from your own shop?\n");
        return;
    }
    flags |= MAG;

    if(cmnd->str[1][0] != '#' || !cmnd->str[1][1]) {
        player->print("What item would you like to buy?\n");
        return;
    }
    num = atoi(cmnd->str[1]+1);
    ObjectSet::iterator it, next;
    for( it = storage->objects.begin() ; it != storage->objects.end() && num != n; ) {
        while(it != storage->objects.end()) {
            next = std::next(it);
            if(next != storage->objects.end() && (playerShopSame(player, (*it), *(next)))) {
                it = next;
            } else
                break;
        }
        n++;
        it++;
    }
    if(it == storage->objects.end()) {
        *player << "That isn't being sold here.\n";
        return;
    }
    object = (*it);

    // load the guild a little bit earlier
    if(p->getGuild())
        guild = gConfig->getGuild(p->getGuild());

    if(player->coins[GOLD] < object->getShopValue()) {
        if(guild)
            player->print("You can't afford %s's outrageous prices!\n", guild->getName().c_str());
        else
            player->print("You can't afford %s's outrageous prices!\n", p->getOwner().c_str());
        return;
    }

    if(player->getWeight() + object->getActualWeight() > player->maxWeight()) {
        *player << "You couldn't possibly carry anymore.\n";
        return;
    }

    if(player->tooBulky(object->getActualBulk()) ) {
        *player << "No more will fit in your inventory right now.\n";
        return;
    }


    if(!Unique::canGet(player, object, true)) {
        *player << "You are unable to purchase that limited item at this time.\n";
        return;
    }
    if(!Lore::canHave(player, object, false)) {
        *player << "You cannot purchased that item.\nIt is a limited item of which you cannot carry any more.\n";
        return;
    }
    if(!Lore::canHave(player, object, true)) {
        *player << "You cannot purchased that item.\nIt contains a limited item that you cannot carry.\n";
        return;
    }



    player->printColor("You buy a %s.\n", object->getObjStr(nullptr, flags, 1).c_str());
    broadcast(player->getSock(), player->getParent(), "%M just bought %1P.", player.get(), object.get());
    object->clearFlag(O_PERM_INV_ITEM);
    object->clearFlag(O_PERM_ITEM);
    object->clearFlag(O_TEMP_PERM);

    player->coins.sub(object->getShopValue(), GOLD);

    if(object->getType() == ObjectType::LOTTERYTICKET || object->getType() == ObjectType::BANDAGE)
        object->setShopValue(0);


    owner = gServer->findPlayer(p->getOwner().c_str());
    if(!owner) {
        if(!loadPlayer(p->getOwner().c_str(), owner)) {
            loge("Shop (%s) without a valid owner (%s).\n",
                 room->info.displayStr().c_str(), p->getOwner().c_str());
            player->save(true);
            return;
        }
    } else
        online = true;



    object->deleteFromRoom();
    Limited::transferOwner(owner, player, object);
    player->addObj(object);
    Server::logGold(GOLD_OUT, player, Money(object->getShopValue() * TAX, GOLD), object, "TAX");
    deposit = shopProfit(object);

    if(!guild) {
        owner->bank.add(deposit, GOLD);
        Bank::log(owner->getCName(), "PROFIT from sale of %s to %s: %ld [Balance: %s]\n",
                  object->getObjStr(nullptr, flags, 1).c_str(), player->getCName(), deposit, owner->bank.str().c_str());
        if(online && !owner->flagIsSet(P_DONT_SHOW_SHOP_PROFITS))
            owner->printColor("*** You made $%d profit from the sale of %s^x to %s.\n",
                              deposit, object->getObjStr(nullptr, flags, 1).c_str(), player->getCName());
        owner->save(online);
    } else {
        guild->bank.add(deposit, GOLD);
        Bank::guildLog(guild->getNum(), "PROFIT from sale of %s to %s: %ld [Balance: %s]\n",
                       object->getObjStr(nullptr, flags, 1).c_str(), player->getCName(), deposit, guild->bank.str().c_str());
        gConfig->saveGuilds();
    }

    if(!online)
        owner.reset();
    player->save(true);
    return;
    //*********************************************************************
}