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
#include <boost/utility.hpp>      // for next
#include <cstdlib>                // for atoi, atol
#include <cstring>                // for strncmp, strcmp, strlen, strcpy
#include <ctime>                  // for time

#include "bank.hpp"               // for guildLog
#include "carry.hpp"              // for Carry
#include "catRef.hpp"             // for CatRef
#include "color.hpp"              // for padColor
#include "cmd.hpp"                // for cmd
#include "commands.hpp"           // for getFullstrText, cmdHelp, cmdProperties
#include "config.hpp"             // for Config, gConfig
#include "container.hpp"          // for ObjectSet, Container
#include "creatureStreams.hpp"    // for Streamable, ColorOff, ColorOn
#include "creatures.hpp"          // for Player, Creature, Monster
#include "dm.hpp"                 // for findRoomsWithFlag
#include "exits.hpp"              // for Exit
#include "factions.hpp"           // for Faction, Faction::INDIFFERENT
#include "flags.hpp"              // for P_AFK, R_SHOP, O_PERM_ITEM, R_BUILD...
#include "global.hpp"             // for CreatureClass, MAG, PROP_SHOP, BUY
#include "guilds.hpp"             // for Guild, shopStaysWithGuild
#include "hooks.hpp"              // for Hooks
#include "money.hpp"              // for Money, GOLD
#include "mud.hpp"                // for MINSHOPLEVEL, GUILD_MASTER
#include "objects.hpp"            // for Object, ObjectType, ObjectType::LOT...
#include "os.hpp"                 // for merror
#include "property.hpp"           // for Property, PartialOwner
#include "proto.hpp"              // for broadcast, logn, link_rom, needUniq...
#include "random.hpp"             // for Random
#include "rooms.hpp"              // for UniqueRoom, BaseRoom, ExitList
#include "server.hpp"             // for Server, gServer, GOLD_OUT, GOLD_IN
#include "unique.hpp"             // for Lore, addOwner, isLimited, Unique
#include "utils.hpp"              // for MAX, MIN
#include "xml.hpp"                // for loadRoom, loadObject, loadPlayer

#define TAX .06


//*********************************************************************
//                      buyAmount
//*********************************************************************

Money buyAmount(const Player* player, const Monster *monster, const Object* object, bool sell) {
    Money cost = Faction::adjustPrice(player, monster->getPrimeFaction(), object->value, true);
    cost.set(MAX<unsigned long>(10,cost[GOLD]), GOLD);
    return(cost);
}

Money buyAmount(const Player* player, const UniqueRoom *room, const Object* object, bool sell) {
    Money cost = Faction::adjustPrice(player, room->getFaction(), object->value, true);
    cost.set(MAX<unsigned long>(10,cost[GOLD]), GOLD);
    return(cost);
}

//*********************************************************************
//                      sellAmount
//*********************************************************************

Money sellAmount(const Player* player, const UniqueRoom *room, const Object* object, bool sell) {
    Money value = Faction::adjustPrice(player, room->getFaction(), object->value, sell);
    value.set(MIN<unsigned long>(value[GOLD] / 2, MAXPAWN), GOLD);
    return(value);
}

//*********************************************************************
//                      bailCost
//*********************************************************************

Money bailCost(const Player* player) {
    Money cost;
    cost.set(2000*player->getLevel(), GOLD);
    return(cost);
}

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

long shopProfit(Object* object) {
    return((long)(object->getShopValue() - (TAX * object->getShopValue())));
}


//*********************************************************************
//                      getCondition
//*********************************************************************

std::string getCondition(Object* object) {
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

void setupShop(Property *p, Player* player, const Guild* guild, UniqueRoom* shop, UniqueRoom* storage) {
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

void shopAssignGuildMessage(Property* p, const Player* player, const Guild* guild) {
    player->print("This property is now associated with the guild %s.\n", guild->getName().c_str());
    p->appendLog("", "This property is now associated with the guild %s.", guild->getName().c_str());
    broadcastGuild(guild->getNum(), 1, "### %s has assigned the shop located at %s as partially run by the guild.",
        player->getCName(), p->getLocation().c_str());
}

//*********************************************************************
//                      shopAssignGuild
//*********************************************************************

bool shopAssignGuild(Property *p, Player* player, const Guild* guild, UniqueRoom* shop, UniqueRoom* storage, bool showMessage) {
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

void shopRemoveGuild(Property *p, Player* player, UniqueRoom* shop, UniqueRoom* storage) {
    p->setGuild(0);
    p->appendLog("", "This property is no longer associated with a guild.");

    if(!shop || !storage) {
        CatRef cr = p->ranges.front().low;
        if(loadRoom(cr, &shop)) {
            cr.id++;
            if(!loadRoom(cr, &storage))
                return;
        }
    }

    setupShop(p, player, nullptr, shop, storage);
}



//*********************************************************************
//                      shopStorageRoom
//*********************************************************************

CatRef shopStorageRoom(const UniqueRoom *shop) {
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

bool playerShopSame(Player* player, Object* obj1, Object* obj2) {
    return( obj1->showAsSame(player, obj2) &&
            getCondition(obj1) == getCondition(obj2) &&
            obj1->getShopValue() == obj2->getShopValue()
    );
}

//*********************************************************************
//                      objShopName
//*********************************************************************

std::string objShopName(Object* object, int m, int flags, int pad) {
    std::string name = object->getObjStr(nullptr, flags, m);
    return padColor(name, pad);
}

//*********************************************************************
//                      tooManyItemsInShop
//*********************************************************************

bool tooManyItemsInShop(const Player* player, const UniqueRoom* storage) {
    int numObjects=0, numLines=0;
    Object *obj;
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

bool canBuildShop(const Player* player, const UniqueRoom* room) {
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
            for(Exit* exit : room->exits) {
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

int cmdShop(Player* player, cmd* cmnd) {
    UniqueRoom* room = player->getUniqueRoomParent(), *storage=nullptr;
    Object  *deed=nullptr;
    int     action=0;
    int     flags = 0, len = strlen(cmnd->str[1]);
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
            for(Exit* exit : room->exits) {
                if(!exit->target.room.isArea("guild")) {
                    cr = exit->target.room;
                    break;
                }
            }
        }

        for(Object* obj : player->objects ) {
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

    if(!loadRoom(shopStorageRoom(room), &storage)) {
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
        UniqueRoom *shop=nullptr;
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

        logn("log.shop", "%s just founded a shop! (%s - %s).\n", player->getCName(), xname.c_str(), cr.str().c_str());

        shop = new UniqueRoom;
        shop->info = cr;

        link_rom(shop, room->info, "out");
        link_rom(room, shop->info, xname.c_str());
        shop->setFlag(R_SHOP);
        shop->setFlag(R_INDOORS);

        storage = new UniqueRoom;
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
            player->getCName(), room->info.str().c_str(), xname.c_str(), shop->info.str().c_str());
        broadcast(player->getSock(), player->getParent(), "%M just opened a shop!", player );
        if(!player->flagIsSet(P_DM_INVIS))
            broadcast("### %s just opened a shop! It's located at: %s.", player->getCName(), room->getCName());

        if(room->info.isArea("guild"))
            shopAssignGuildMessage(p, player, guild);

        delete shop;
        delete storage;
        player->delObj(deed, true);
        delete deed;
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
        Object *obj = player->findObject(player, cmnd, 2);
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
        broadcast(player->getSock(), player->getParent(), "%M just stocked something in this store.", player);
        // obj->shopValue
        if(limited)
            player->save(true);
        storage->saveToFile(0);
    } else if(action == SHOP_PRICE) {
        Object *obj = storage->findObject(player, cmnd, 2);
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
        broadcast(player->getSock(), player->getParent(), "%M just updated the prices in this store.", player);
    } else if(action == SHOP_REMOVE) {
        Object *obj = storage->findObject(player, cmnd, 2);

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
        broadcast(player->getSock(), player->getParent(), "%M just removed something from this store.", player);

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
            player->getCName(), player->getUniqueRoomParent()->info.str().c_str(), player->getUniqueRoomParent()->getCName());
        player->print("Shop renamed to '%s'.\n", player->getUniqueRoomParent()->getCName());
        player->getUniqueRoomParent()->saveToFile(0);

        return(0);
    } else {
        player->printColor(shopSyntax);
    }

    return(0);
}

//*********************************************************************
//                      doFilter
//*********************************************************************

bool doFilter(const Object* object, std::string_view filter) {
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

bool isValidShop(const UniqueRoom* shop, const UniqueRoom* storage) {
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

const char* cannotUseMarker(Player* player, Object* object) {
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

int cmdList(Player* player, cmd* cmnd) {
    UniqueRoom* room = player->getUniqueRoomParent(), *storage=nullptr;
    Object* object=nullptr;
    int     n=0;
    std::string filter = "";

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

    if(!loadRoom(shopStorageRoom(room), &storage)) {
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
        Money cost;

        if(!Faction::willDoBusinessWith(player, player->getUniqueRoomParent()->getFaction())) {
            *player << "The shopkeeper refuses to do business with you.\n";
            return(0);
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
    } else {
        // We've got a player run shop here...different formating them
        int num = 0, flags = 0, m=0;
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

    return(0);
}

//*********************************************************************
//                      cmdPurchase
//*********************************************************************
// purchase allows a player to buy an item from a monster.  The
// purchase item flag must be set, and the monster must have an
// object to sell.  The object for sale is determined by the first
// object listed in carried items.

int cmdPurchase(Player* player, cmd* cmnd) {
    Monster *creature=nullptr;
    Object  *object=nullptr;
    int     maxitem=0;
    CatRef  obj_num[10];
    Money cost;
    int     i=0, j=0, found=0, match=0;


    if(cmnd->num == 2)
        return(cmdBuy(player, cmnd));


    player->clearFlag(P_AFK);
    if(!player->ableToDoCommand())
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
        player->print("%M refuses to do business with you.\n", creature);
        return(0);
    }

    if(!creature->flagIsSet(M_CAN_PURCHASE_FROM)) {
        player->print("You cannot purchase objects from %N.\n",creature);
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
        player->print("%M has nothing to sell.\n",creature);
        return(0);
    }

    found = 0;
    for(i=0; i<maxitem; i++) {
        if(loadObject(creature->carry[i].info, &object)) {
            if(player->canSee(object) && keyTxtEqual(object, cmnd->str[1])) {
                match++;
                if(match == cmnd->val[1]) {
                    found = 1;
                    break;
                } else
                    delete object;
            } else
                delete object;
        }
    }

    if(!found) {
        player->print("%M says, \"Sorry, I don't have any of those to sell.\"\n", creature);
        return(0);
    }

    object->init();

    if(player->getWeight() + object->getActualWeight() > player->maxWeight()) {
        *player << "That would be too much for you to carry.\n";
        delete object;
        return(0);
    }

    if(player->tooBulky(object->getActualBulk())) {
        *player << "That would be too bulky.\nClean your inventory first.\n";
        delete object;
        return(0);
    }


    if(!Unique::canGet(player, object)) {
        *player << "You are unable to purchase that limited item at this time.\n";
        delete object;
        return(0);
    }
    if(!Lore::canHave(player, object, false)) {
        *player << "You cannot purchased that item.\nIt is a limited item of which you cannot carry any more.\n";
        delete object;
        return(0);
    }
    if(!Lore::canHave(player, object, true)) {
        *player << "You cannot purchased that item.\nIt contains a limited item that you cannot carry.\n";
        delete object;
        return(0);
    }



    cost = buyAmount(player, creature, object, true);

    if(player->coins[GOLD] < cost[GOLD]) {
        player->print("%M says \"The price is %s, and not a copper piece less!\"\n", creature, cost.str().c_str());
        delete object;
    } else {

        player->print("You pay %N %s gold pieces.\n", creature, cost.str().c_str());
        broadcast(player->getSock(), player->getParent(), "%M pays %N %s for %P.\n", player, creature, cost.str().c_str(), object);
        player->coins.sub(cost);

        player->doHaggling(creature, object, BUY);
        Server::logGold(GOLD_OUT, player, object->refund, object, "MobPurchase");

        if(object->hooks.executeWithReturn("afterPurchase", player, creature->getId())) {
            // A return value of true means the object still exists
            player->printColor("%M says, \"%sHere is your %s.\"\n", creature,
                    Faction::getAttitude(player->getFactionStanding(creature->getPrimeFaction())) < Faction::INDIFFERENT ? "Hrmph. " : "Thank you very much. ",
                    object->getCName());
            Limited::addOwner(player, object);
            object->setDroppedBy(creature, "MobPurchase");

            player->addObj(object);

        } else {
            // A return value of false means the object doesn't exist, and as such the spell was cast, so delete it
            delete object;

        }

    }

    return(0);
}


//*********************************************************************
//                      cmdSelection
//*********************************************************************
// The selection command lists all the items  a monster is selling.
// The monster needs the M_CAN_PURCHASE_FROM flag set to denote it can sell.

int cmdSelection(Player* player, cmd* cmnd) {
    Monster *creature=nullptr;
    Object  *object=nullptr;
    CatRef  obj_list[10];
    int     i=0, j=0, found=0, maxitem=0;
    std::string filter = "";

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
        player->print("%M refuses to do business with you.\n", creature);
        return(0);
    }

    if(!creature->flagIsSet(M_CAN_PURCHASE_FROM)) {
        player->print("%M is not selling anything.\n",creature);
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
        player->print("%M has nothing to sell.\n", creature);
        return(0);
    }

    player->print("%M is currently selling:", creature);
    if(!filter.empty())
        player->printColor(" ^Y(filtering on \"%s\")", filter.c_str());
    *player << "\n";

    Money cost;
    for(i=0; i<maxitem; i++) {
        if(!creature->carry[i].info.id || !loadObject(creature->carry[i].info, &object)) {
            player->print("%d) Out of stock.\n", i+1);
        } else {
            if(!doFilter(object, filter)) {
                cost = buyAmount(player, creature, object, true);

                player->printColor("%d) %s    %s\n", i+1, objShopName(object, 1, 0, 35).c_str(),
                    cost.str().c_str());
            }
            delete object;
        }
    }

    *player << "\n";
    return(0);
}

//*********************************************************************
//                      cmdBuy
//*********************************************************************
// This function allows a player to buy something from a shop.

int cmdBuy(Player* player, cmd* cmnd) {
    UniqueRoom* room = player->getUniqueRoomParent(), *storage=nullptr;
    Object  *object=nullptr, *object2=nullptr;
    int     num=0, n=1;

    if(cmnd->num == 3)
        return(cmdPurchase(player, cmnd));

    if(!needUniqueRoom(player))
        return(0);


    if(!player->ableToDoCommand())
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

    if(!loadRoom(shopStorageRoom(room), &storage)) {
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
        Player* owner=nullptr;
        Guild*  guild=nullptr;
        int     flags=0;
        long    deposit=0;
        bool    online=false;

        if(p->isOwner(player->getName())) {
            player->print("Why would you want to buy stuff from your own shop?\n");
            return(0);
        }
        flags |= MAG;

        if(cmnd->str[1][0] != '#' || !cmnd->str[1][1]) {
            player->print("What item would you like to buy?\n");
            return(0);
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
            return(0);
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
            return(0);
        }

        if(player->getWeight() + object->getActualWeight() > player->maxWeight()) {
            *player << "You couldn't possibly carry anymore.\n";
            return(0);
        }

        if(player->tooBulky(object->getActualBulk()) ) {
            *player << "No more will fit in your inventory right now.\n";
            return(0);
        }


        if(!Unique::canGet(player, object, true)) {
            *player << "You are unable to purchase that limited item at this time.\n";
            return(0);
        }
        if(!Lore::canHave(player, object, false)) {
            *player << "You cannot purchased that item.\nIt is a limited item of which you cannot carry any more.\n";
            return(0);
        }
        if(!Lore::canHave(player, object, true)) {
            *player << "You cannot purchased that item.\nIt contains a limited item that you cannot carry.\n";
            return(0);
        }



        player->printColor("You buy a %s.\n", object->getObjStr(nullptr, flags, 1).c_str());
        broadcast(player->getSock(), player->getParent(), "%M just bought %1P.", player, object);
        object->clearFlag(O_PERM_INV_ITEM);
        object->clearFlag(O_PERM_ITEM);
        object->clearFlag(O_TEMP_PERM);

        player->coins.sub(object->getShopValue(), GOLD);

        if(object->getType() == ObjectType::LOTTERYTICKET || object->getType() == ObjectType::BANDAGE)
            object->setShopValue(0);


        owner = gServer->findPlayer(p->getOwner().c_str());
        if(!owner) {
            if(!loadPlayer(p->getOwner().c_str(), &owner)) {
                loge("Shop (%s) without a valid owner (%s).\n",
                    room->info.str().c_str(), p->getOwner().c_str());
                player->save(true);
                return(0);
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
            free_crt(owner);
        player->save(true);
        return(0);
        //*********************************************************************
    } else {
        // Not a player run shop
        object = storage->findObject(player, cmnd, 1);

        if(!object) {
            *player << "That's not being sold.\n";
            return(0);
        }

        if(!Faction::willDoBusinessWith(player, player->getUniqueRoomParent()->getFaction())) {
            *player << "The shopkeeper refuses to do business with you.\n";
            return(0);
        }

        if(object->getName() == "storage room") {
            if(!room->flagIsSet(R_STORAGE_ROOM_SALE)) {
                *player << "You can't buy storage rooms here.\n";
                return(0);
            }

            CatRef  cr = gConfig->getSingleProperty(player, PROP_STORAGE);
            if(cr.id) {
                *player << "You are already affiliated with a storage room.\nOnly one is allowed per player.\n";
                return(0);
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
                return(0);
            }
        } else {

            if(player->coins[GOLD] < cost[GOLD]) {
                *player << "You don't have enough gold.\n";
                return(0);
            }

            if( player->getWeight() + object->getActualWeight() > player->maxWeight() &&
                !player->checkStaff("You can't carry anymore.\n") )
                return(0);

            if(player->tooBulky(object->getActualBulk()) ) {
                *player << "No more will fit in your inventory right now.\n";
                return(0);
            }

        }

        // Not buying a storage room or bail
        if(object->getName() != "storage room" && object->getName() != "bail") {

            player->unhide();
            if(object->getType() == ObjectType::LOTTERYTICKET) {
                if(gConfig->getLotteryEnabled() == 0) {
                    player->print("Sorry, lottery tickets are not currently being sold.\n");
                    return(0);
                }

                if(createLotteryTicket(&object2, player->getCName()) < 0) {
                    player->print("Sorry, lottery tickets are not currently being sold.\n");
                    return(0);
                }
            // Not a lottery Ticket
            } else {
                object2 = new Object;
                if(!object2)
                    merror("buy", FATAL);

                *object2 = *object;
                object2->clearFlag(O_PERM_INV_ITEM);
                object2->clearFlag(O_PERM_ITEM);
                object2->clearFlag(O_TEMP_PERM);
            }

            if(object2)
                object2->init();

            if(!Unique::canGet(player, object)) {
                *player << "You are unable to purchase that limited item at this time.\n";
                delete object2;
                return(0);
            }
            if(!Lore::canHave(player, object, false)) {
                *player << "You cannot purchased that item.\nIt is a limited item of which you cannot carry any more.\n";
                delete object2;
                return(0);
            }
            if(!Lore::canHave(player, object, true)) {
                *player << "You cannot purchased that item.\nIt contains a limited item that you cannot carry.\n";
                delete object2;
                return(0);
            }

            bool isDeed = object2->deed.low.id || object2->deed.high;

            // buying a deed
            if(isDeed && player->getLevel() < MINSHOPLEVEL) {
                player->print("You must be at least level %d to buy a shop deed.\n", MINSHOPLEVEL);
                delete object2;
                return(0);
            }

            player->printColor("You bought %1P.\n", object2);

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

            broadcast(player->getSock(), player->getParent(), "%M bought %1P.", player, object2);

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
                delete object2;
            }

        } else if(object->getName() == "storage room") {

            CatRef  cr = gConfig->getAvailableProperty(PROP_STORAGE, 1);
            if(cr.id < 1) {
                player->print("The shopkeeper says, \"Sorry, I don't have any more rooms available!\"\n");
                return(0);
            }

            player->coins.sub(cost);
            Server::logGold(GOLD_OUT, player, cost, nullptr, "PurchaseStorage");

            player->print("The shopkeeper says, \"Congratulations, you are now the proud owner of a new storage room. Don't forget, to get in you must also buy a key.\"\n");
            player->print("You have %s left.\n", player->coins.str().c_str());
            broadcast(player->getSock(), player->getParent(), "%M just bought a storage room.", player);
            logn("log.storage", "%s bought storage room %s.\n",
                player->getCName(), cr.str().c_str());

            createStorage(cr, player);

        } else {
            // Buying bail to get outta jail!
            Exit    *exit=nullptr;

            // Find the cell door
            exit = findExit(player, "cell", 1);
            player->unhide();

            if(!exit || !exit->flagIsSet(X_LOCKED)) {
                player->print("You are in no need of bail!\n");
                return(0);
            }

            exit->clearFlag(X_LOCKED);
            exit->ltime.ltime = time(nullptr);
            player->coins.sub(cost);
            Server::logGold(GOLD_OUT, player, cost, exit, "Bail");
            player->print("You bail yourself out for %s gold.\n", cost.str().c_str());
            broadcast("### %M just bailed %sself out of jail!", player, player->himHer());
        }
    }
    return(0);
}


//*********************************************************************
//                      cmdSell
//*********************************************************************
// This function will allow a player to sell an object in a pawn shop

int cmdSell(Player* player, cmd* cmnd) {
    Object  *object=nullptr;
    bool    poorquality=false;
    Money value;


    if(player->getClass() == CreatureClass::BUILDER)
        return(0);

    if(!player->ableToDoCommand())
        return(0);

    if(!player->getRoomParent()->flagIsSet(R_PAWN_SHOP)) {
        *player << "This is not a pawn shop.\n";
        return(0);
    }

    if(cmnd->num < 2) {
        player->print("Sell what?\n");
        return(0);
    }

    player->unhide();
    object = player->findObject(player, cmnd, 1);

    if(!object) {
        *player << "You don't have that.\n";
        return(0);
    }

    if(object->flagIsSet(O_KEEP)) {
        player->printColor("%O is currently in safe keeping. Unkeep it to sell it.\n", object);
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

    player->computeLuck();
    // Luck for sale of items
    //  gold = ((Ply[fd].extr->getLuck()*gold)/100);

    if((object->getType() == ObjectType::WEAPON || object->getType() == ObjectType::ARMOR) && object->getShotsCur() <= object->getShotsMax()/8)
        poorquality = true;

    if((object->getType() == ObjectType::WAND || object->getType() > ObjectType::MISC || object->getType() == ObjectType::KEY) && object->getShotsCur() < 1)
        poorquality = true;

    if (value[GOLD] < 20 || poorquality || object->getType() == ObjectType::SCROLL ||
    // objects flagged as eatable/drinkable can be sold
            (object->getType() == ObjectType::POTION && !object->flagIsSet(O_EATABLE)
                    && !object->flagIsSet(O_DRINKABLE)) || object->getType() == ObjectType::SONGSCROLL
            || object->flagIsSet(O_STARTING)) {
        switch(Random::get(1,5)) {
            case 1:
                player->print("The shopkeep says, \"I'm not interested in that.\"\n");
                break;
            case 2:
                player->print("The shopkeep says, \"I don't want that.\"\n");
                break;
            case 3:
                player->print("The shopkeep says, \"I don't have any use for that.\"\n");
                break;
            case 4:
                player->print("The shopkeep says, \"You must be joking; that's worthless!\"\n");
                break;
            default:
                player->print("The shopkeep says, \"I won't buy that crap from you.\"\n");
                break;
        }
        return(0);
    }

    // This prevents high level thieves from possibly cashing in on shoplifting.
    if(object->flagIsSet(O_WAS_SHOPLIFTED)) {
        player->print("The shopkeep says, \"That was stolen from a shop somewhere. I won't buy it!\"\n");
        return(0);
    }

    if(object->flagIsSet(O_NO_PAWN)) {
        player->print("The shopkeep says, \"I refuse to buy that! Get out!\"\n");
        return(0);
    }

    player->printColor("The shopkeep gives you %s for %P.\n", value.str().c_str(), object);
    broadcast(player->getSock(), player->getParent(), "%M sells %1P.", player, object);

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

int cmdValue(Player* player, cmd* cmnd) {
    Object  *object=nullptr;
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

    object = player->findObject(player, cmnd, 1);

    if(!object) {
        *player << "You don't have that.\n";
        return(0);
    }

    if(!Faction::willDoBusinessWith(player, player->getUniqueRoomParent()->getFaction())) {
        *player << "The shopkeeper refuses to do business with you.\n";
        return(0);
    }

    value = sellAmount(player, player->getConstUniqueRoomParent(), object, false);

    player->printColor("The shopkeep says, \"%O's worth %s.\"\n", object, value.str().c_str());

    broadcast(player->getSock(), player->getParent(), "%M gets %P appraised.", player, object);

    return(0);
}



//*********************************************************************
//                      cmdRefund
//*********************************************************************

int cmdRefund(Player* player, cmd* cmnd) {
    Object  *object=nullptr;


    if(player->getClass() == CreatureClass::BUILDER)
        return(0);

    if(!player->ableToDoCommand())
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
    broadcast(player->getSock(), player->getParent(), "%M refunds %P.", player, object);
    Server::logGold(GOLD_IN, player, object->refund, object, "Refund");
    player->delObj(object, true);
    // No further haggling allowed in this store until they buy a full priced item and have left the room
    player->setFlag(P_JUST_REFUNDED);
    if(player->getRoomParent() && player->getRoomParent()->getAsUniqueRoom()) {
        player->addRefundedInStore(player->getRoomParent()->getAsUniqueRoom()->info);
    }

    delete object;
    return(0);
}

//*********************************************************************
//                      failTrade
//*********************************************************************

void failTrade(const Player* player, const Object* object, const Monster* target, Object* trade=nullptr, bool useHeShe=true) {
    if(useHeShe)
        player->printColor("%s gives you back %P.\n", target->upHeShe(), object);
    else
        player->printColor("%M gives you back %P.\n", target, object);
    broadcast(player->getSock(), player->getParent(), "%M tried to trade %P with %N.",
        player, object, target);
    delete trade;
}

//*********************************************************************
//                      canReceiveObject
//*********************************************************************
// Determine if the player can receive the object (as a result of a trade
// or completing a quest. This code does NOT handle bulk/weight, as that
// is tracked on a broader scale, rather than for each item.

bool canReceiveObject(const Player* player, Object* object, const Monster* monster, bool isTrade, bool doQuestOwner, bool maybeTryAgainMsg) {
    std::string tryAgain = "";
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
            player->print("%M cannot trade for that right now.\n", monster);
        else
            player->print("You cannot complete that quest right now.\nThe unique item reward is already in the game.\n");
        if(!tryAgain.empty())
            player->print(tryAgain.c_str());
        return(false);
    }

    // check for lore items
    if(!Lore::canHave(player, object, false)) {
        if(isTrade)
            player->printColor("%M cannot trade for that right now, as you already have %P.\n", monster, object);
        else
            player->printColor("You cannot complete that quest right now, as you already have %P.\n", object);
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
//                      abortItemList
//*********************************************************************
// If we cannot give them the objects for whatever reason, we must
// delete all the objects we have already loaded.

void abortItemList(std::list<Object*> &objects) {
    Object* object=nullptr;

    while(!objects.empty()) {
        object = objects.front();
        delete object;
        objects.pop_front();
    }

    objects.clear();
}

//*********************************************************************
//                      prepareItemList
//*********************************************************************
// When attempting to trade or complete a quest, we first check to see if they are
// able to receive all of the items as a result. If they cannot, we prevent them
// from completing the trade or quest, give them a reason why and return false. If
// they are able to receive all the items, we return a list of objects they will
// receive in the "objects" parameter.

bool prepareItemList(const Player* player, std::list<Object*> &objects, Object* object, const Monster* monster, bool isTrade, bool doQuestOwner, int totalBulk) {
    std::list<CatRef>::const_iterator it;
    Object* toGive=nullptr;
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
        abortItemList(objects);
        delete object;
        return(false);
    }

    if(object->flagIsSet(O_LOAD_ALL) && !object->randomObjects.empty()) {
        // in this case, we give them a lot of items
        for(it = object->randomObjects.begin(); it != object->randomObjects.end(); it++) {
            if(!loadObject(*it, &toGive))
                continue;
            objects.push_back(toGive);
            totalBulk += toGive->getActualBulk();
            // if they can't get this object, we need to delete everything
            // we've already loaded so far
            if(!canReceiveObject(player, toGive, monster, isTrade, doQuestOwner, false)) {
                abortItemList(objects);
                delete object;
                return(false);
            }
        }
        // they aren't actually getting the "object" item
        delete object;
    } else {
        // the simple, common scenario
        objects.push_back(object);
        totalBulk += object->getActualBulk();
    }

    // check total bulk: if they can't get the objects, we need to delete everything
    // we've already loaded so far
    if(player->tooBulky(totalBulk)) {
        if(isTrade || doQuestOwner)
            player->print("What %N wants to give you is too bulky for you.\n", monster);
        abortItemList(objects);
        return(false);
    }

    return(true);
}

//*********************************************************************
//                      cmdTrade
//*********************************************************************

int cmdTrade(Player* player, cmd* cmnd) {
    Monster *creature=nullptr;
    Object  *object=nullptr, *trade=nullptr;
    Carry   invTrade, invResult;
    int     i=0, numTrade = cmnd->val[0];
    bool    badNumTrade=false, found=false, hasTrade=false, doQuestOwner=false;
    int     flags = player->displayFlags();

    player->clearFlag(P_AFK);
    if(!player->ableToDoCommand())
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
        player->print("You can't trade with %N.\n", creature);
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
        player->print("%M refuses to do business with you.\n", creature);
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
        player->print("%M has nothing to trade.\n", creature);
        return(0);
    }

    if(!found || !object->info.id || ((object->getShotsCur() <= object->getShotsMax()/10) && object->getType() != ObjectType::MISC)) {
        player->print("%M says, \"I don't want that!\"\n", creature);
        failTrade(player, object, creature);
        return(0);
    }

    if(badNumTrade) {
        player->print("%M says, \"That's now how many I want!\"\n", creature);
        failTrade(player, object, creature);
        return(0);
    }

    if(!object->isQuestOwner(player)) {
        player->print("%M says, \"That doesn't belong to you!\"\n", creature);
        failTrade(player, object, creature);
        return(0);
    }

    if(!loadObject((invResult.info), &trade)) {
        player->print("%M says, \"Sorry, I don't have anything for you in return for that. Keep it.\"\n", creature);
        failTrade(player, object, creature);
        return(0);
    }

    std::list<Object*> objects;
    std::list<Object*>::const_iterator oIt;

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
        Object *obj = nullptr;

        // find the first occurance of the object in their inventory
        for( it = player->objects.begin() ; it != player->objects.end() ; ) {
            obj = (*it);
            if(obj->info == object->info)
                break;
            else
                it++;
        }

        if(it == player->objects.end()) {
            player->print("%M gets confused and cannot trade for that right now.\n", creature);
            failTrade(player, object, creature, trade);
            return(0);
        }
        while( it != player->objects.end() && numObjects != numTrade && (*it)->info == object->info) {
            obj = (*it);
            numObjects++;
            if(!obj->isQuestOwner(player)) {
                player->print("%M says, \"That doesn't belong to you!\"\n", creature);
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
        for(Object* toDelete : toDelSet) {
            player->delObj(toDelete, true);
            creature->addObj(toDelete);
            numObjects--;
        }
    } else {
        // the simple, common scenario
        player->delObj(object, true);
        creature->addObj(object);
    }

    player->printColor("%M says, \"Thank you for retrieving %s for me.\n", creature, object->getObjStr(player, flags, numTrade).c_str());
    player->printColor("For it, I will reward you.\"\n");

    broadcast(player->getSock(), player->getParent(), "%M trades %P to %N.", player,object,creature);


    for(oIt = objects.begin(); oIt != objects.end(); oIt++) {
        player->printColor("%M gives you %P in trade.\n", creature, *oIt);
        (*oIt)->setDroppedBy(creature, "MonsterTrade");
        doGetObject(*oIt, player);
    }

    if(creature->ttalk[0])
        player->print("%M says, \"%s\"\n", creature, creature->ttalk);

    return(0);
}


//*********************************************************************
//                      cmdAuction
//*********************************************************************

int cmdAuction(Player* player, cmd* cmnd) {
    int         i=0, flags=0;
    long        amnt=0, batch=1, each=0;
    Object      *object=nullptr;

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
            batch = atoi(&cmnd->str[2][1]);

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



    Player* target=nullptr;
    for(const auto& p : gServer->players) {
        target = p.second;

        if(!target->isConnected())
            continue;
        if(target->flagIsSet(P_NO_AUCTIONS))
            continue;

        if(!strcmp(cmnd->str[1], "self")) {
            target->printColor("*** %M is auctioning %sself for %ldgp.\n", player, player->himHer(), amnt);
            continue;
        }
        target->printColor("*** %M is auctioning %s for %ldgp%s.\n",
            player, object->getObjStr(nullptr, flags, batch).c_str(), amnt, each ? each==1 ? " each" : " total" : "");
    }

    return(0);
}

//*********************************************************************
//                      setLastPawn
//*********************************************************************

void Player::setLastPawn(Object* object) {
    // uniques and quests cannot be reclaimed
    if(object && (object->flagIsSet(O_UNIQUE) || object->getQuestnum())) {
        delete object;
        return;
    }
    if(lastPawn) {
        delete lastPawn;
        lastPawn = nullptr;
    }
    lastPawn = object;
}

//*********************************************************************
//                      restoreLastPawn
//*********************************************************************

bool Player::restoreLastPawn() {
    if(!getRoomParent()->flagIsSet(R_PAWN_SHOP) || !getRoomParent()->flagIsSet(R_DUMP_ROOM)) {
        print("You must be in a pawn shop to reclaim items.\n");
        return(false);
    }
    if(!lastPawn) {
        print("You have no items to reclaim.\n");
        return(false);
    }
    printColor("You negotiate with the shopkeeper to reclaim %P.\n", lastPawn);
    if(lastPawn->refund[GOLD] > coins[GOLD]) {
        print("You don't have enough money to reclaim that item!\n");
        return(false);
    }
    if(!Unique::canGet(this, lastPawn) || !Lore::canHave(this, lastPawn)) {
        print("You currently cannot reclaim that limited object.\n");
        return(false);
    }
    coins.sub(lastPawn->refund);
    printColor("You give the shopkeeper %d gold and reclaim %P.\n", lastPawn->refund[GOLD], lastPawn);
    Server::logGold(GOLD_OUT, this, lastPawn->refund, lastPawn, "Reclaim");

    lastPawn->setFlag(O_RECLAIMED);
    lastPawn->refund.zero();
    doGetObject(lastPawn, this, true, true);
    lastPawn = nullptr;
    return(true);
}

//*********************************************************************
//                      cmdReclaim
//*********************************************************************

int cmdReclaim(Player* player, cmd* cmnd) {
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

void Creature::doHaggling(Creature *vendor, Object* object, int trans) {
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
        chance = MIN(95,(getLevel()*2 + saves[LCK].chance/10));

    // If this item was sold and reclaimed, no further haggling
    if(trans == SELL && object->flagIsSet(O_RECLAIMED)) {
        chance = 0;
    }


    // If we've refunded an item in this shop, no haggling until having bought a full priced item again

    if(trans == BUY) {
        if(getAsPlayer() && flagIsSet(P_JUST_REFUNDED))
            chance = 0;
        if(chance > 0 && getRoomParent() && getRoomParent()->getAsUniqueRoom() ) {
            Player* player = getAsPlayer();
            if( player && !player->storesRefunded.empty()) {
                if(player->hasRefundedInStore(getRoomParent()->getAsUniqueRoom()->info)) {
                    chance = 0;
                }
            }
        }
    }
    if(trans == SELL)
        val = MIN<unsigned long>(MAXPAWN,object->value[GOLD]/2);
    else
        val = object->value[GOLD]/2;

    roll = Random::get(1,100);

    if(isCt()) {
        print("haggle chance: %d\n", chance);
        print("roll: %d\n", roll);
    }
    if(roll <= chance) {

        discount = MAX(1,Random::get(getLevel()/2, 1+getLevel()));

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
                broadcast(getSock(), getRoomParent(), "%M haggles over prices with %N.", this, vendor);
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
