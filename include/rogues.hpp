/*
 * rogues.h
 *   roguish functions
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
#ifndef ROGUES_H
#define ROGUES_H

#include "socket.hpp"               // for Socket
#include "mudObjects/players.hpp"   // for Player

int cmdGamble(Player* player, cmd* cmnd);
void playBlackjack(Socket* sock, const std::string& str);
void playSlots(Socket* sock, const std::string& str);

#endif