/*
 * .h
 *   Header file for login and character creation.
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
#include <string>

class Player;
class Socket;
class Account;

#define PASSWORD_MIN_LENGTH     5
#define PASSWORD_MAX_LENGTH     35

typedef enum {
    // Connected States
    CON_START,
    CON_PLAYING,
    CON_DISCONNECTING,
    CON_END,

    // For stat changing
    CON_STATS_START,
    CON_CHANGING_STATS,
    CON_CHANGING_STATS_CALCULATE,
    CON_CHANGING_STATS_CONFIRM,
    CON_CHANGING_STATS_RAISE,
    CON_CHANGING_STATS_LOWER,
    CON_STATS_END,

    CON_CHOSING_WEAPONS,

    // Viewing a file
    CON_VIEWING_FILE_REVERSE,

    // Post edit etc
    CON_SENDING_MAIL,
    CON_EDIT_HISTORY,
    CON_EDIT_PROPERTY,

    // Surname and Title
    CON_CONFIRM_SURNAME,
    CON_CONFIRM_TITLE,

    // Change password
    CON_PASSWORD_START,
    CON_CHANGE_PASSWORD,
    CON_CHANGE_PASSWORD_GET_NEW,
    CON_CHANGE_PASSWORD_FINISH,
    CON_PASSWORD_END,

    // LOGIN_START/END bound setState's range routing; PAUSE_SCREEN has its own handler
    LOGIN_START,
    LOGIN_DNS_LOOKUP,
    LOGIN_PAUSE_SCREEN,
    LOGIN_ACCOUNT,
    LOGIN_END,

    // CREATE_START/END bound setState range routing
    CREATE_START,
    CREATE_CHARACTER,
    CREATE_END,

    // gambling
    BLACKJACK_START,
    BLACKJACK_DEAL,
    BLACKJACK_PLAY,
    BLACKJACK_PLAYER_TURN,
    BLACKJACK_DEALER_TURN,
    BLACKJACK_END,
    SLOTS_START,
    SLOTS_END,

    UNUSED_STATE
} connectedStates;

typedef enum {
    NO_COLOR = 0,
    ANSI_COLOR
} colorTypes;


//*********************************************************************
//                      Create
//*********************************************************************
// createPlayer decodes the telnet letter, calls Flow::advance, then renders the next step;
// the render* below are print-only and set the wizard cursor.
namespace Create {
    struct InitialSkills;

    void addStartingItem(const std::shared_ptr<Player>& player, const std::string &area, int id, bool wear=true, bool skipUseCheck=false, int num=1);
    void addStartingWeapon(const std::shared_ptr<Player>& player, const std::string &weapon);

    void renderSex(const std::shared_ptr<Socket>& sock);
    void renderRace(const std::shared_ptr<Socket>& sock);
    void renderSubRace(const std::shared_ptr<Socket>& sock);
    void finishRace(const std::shared_ptr<Socket>& sock);
    void renderClass(const std::shared_ptr<Socket>& sock);
    void renderDeity(const std::shared_ptr<Socket>& sock);
    void renderLocation(const std::shared_ptr<Socket>& sock);
    void renderStatsChoice(const std::shared_ptr<Socket>& sock);
    void renderStats(const std::shared_ptr<Socket>& sock);
    void finishStats(const std::shared_ptr<Socket>& sock);
    void renderBonusStat(const std::shared_ptr<Socket>& sock);
    void renderPenaltyStat(const std::shared_ptr<Socket>& sock);
    void renderProf(const std::shared_ptr<Socket>& sock, const InitialSkills& granted);
    void renderSecondProf(const std::shared_ptr<Socket>& sock);
    void renderName(const std::shared_ptr<Socket>& sock);
    void renderDmPassword(const std::shared_ptr<Socket>& sock);
    void renderDone(const std::shared_ptr<Socket>& sock);
    void processDone(const std::shared_ptr<Socket>& sock, const std::string& str);
}

// Various login functions
void convertNewWeaponSkills(const std::shared_ptr<Socket>& sock, const std::string& str);
void login(const std::shared_ptr<Socket>& sock, const std::string& inStr);
void createPlayer(const std::shared_ptr<Socket>& sock, const std::string& str);
void startCharacterCreation(const std::shared_ptr<Socket>& sock);
void showAccountMenu(const std::shared_ptr<Socket>& sock, std::shared_ptr<Account> account);
void doSurname(const std::shared_ptr<Socket>& sock, const std::string& str);
void doTitle(const std::shared_ptr<Socket>& sock, const std::string& str);
