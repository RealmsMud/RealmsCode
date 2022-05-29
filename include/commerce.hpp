/*
 * commerce.hpp
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

#pragma once

#include <memory>


#define TAX .06

class Object;
class Player;
class Property;
class UniqueRoom;

const char* cannotUseMarker(const std::shared_ptr<Player>& player, const std::shared_ptr<Object>&  object);

bool doFilter(const std::shared_ptr<Object>&  object, std::string_view filter);
std::string objShopName(const std::shared_ptr<Object>&  object, int m, int flags, int pad);

void playerShopList(const std::shared_ptr<Player>& player, Property* p, std::string& filter, std::shared_ptr<UniqueRoom>& storage);
void shopList(const std::shared_ptr<Player>& player, Property* p, std::string& filter, std::shared_ptr<UniqueRoom>& storage);
void playerShopBuy(const std::shared_ptr<Player>& player, cmd* cmnd, Property* p, std::shared_ptr<UniqueRoom>& storage);
void shopBuy(const std::shared_ptr<Player>& player, cmd* cmnd, Property* p, std::shared_ptr<UniqueRoom>& storage);
