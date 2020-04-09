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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef LOGIN_H
#define LOGIN_H

#include "common.hpp"

class Player;
class Socket;

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
    CON_VIEWING_FILE,
    CON_VIEWING_FILE_REVERSE,

    // Post edit etc
    CON_SENDING_MAIL,
    CON_EDIT_NOTE,
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

    // Login States
    LOGIN_START,
    LOGIN_DNS_LOOKUP,
    LOGIN_GET_COLOR,
    LOGIN_PAUSE_SCREEN,
    LOGIN_GET_LOCKOUT_PASSWORD,
    LOGIN_GET_NAME,
    LOGIN_CHECK_CREATE_NEW,
    LOGIN_GET_PASSWORD,
    LOGIN_GET_PROXY_PASSWORD,
    LOGIN_END,

    // Creation States
    CREATE_START,
    CREATE_NEW,
    CREATE_GET_DM_PASSWORD,
    CREATE_CHECK_LOCKED_OUT,
    CREATE_GET_SEX,
    CREATE_GET_RACE,
    CREATE_GET_SUBRACE,
    CREATE_GET_CLASS,
    CREATE_GET_DEITY,
    CREATE_START_LOC,
    CREATE_START_CUSTOM,
    CREATE_GET_STATS_CHOICE,
    CREATE_GET_STATS,
    CREATE_GET_PROF,
    CREATE_GET_ALIGNMENT,
    CREATE_GET_PASSWORD,
    CREATE_BONUS_STAT,
    CREATE_PENALTY_STAT,
    CREATE_SECOND_PROF,
    CREATE_DONE,

    CUSTOM_COMMUNITY,
    CUSTOM_FAMILY,
    CUSTOM_SOCIAL,
    CUSTOM_EDUCATION,
    CUSTOM_HEIGHT,
    CUSTOM_WEIGHT,

    CREATE_END,

    UNUSED_STATE
} connectedStates;

typedef enum {
    NO_COLOR = 0,
    ANSI_COLOR
} colorTypes;


//*********************************************************************
//                      Create
//*********************************************************************
// The work functions in this namespace return false if they fail to get
// the information they want and the user should not progress further.
//
// the createCharacter function should determine what order everything
// is called in, the individual functions should only do the work that
// belongs to them.
//
// each work function has 2 settings - doPrint and doWork.
//
namespace Create {
    const int doPrint=1;
    const int doWork=2;
    void addStartingItem(Player* player, const bstring& area, int id, bool wear=true, bool skipUseCheck=false, int num=1);
    void addStartingWeapon(Player* player, const bstring& weapon);

    // work functions
    bool getSex(Socket* sock, bstring str, int mode);
    bool getRace(Socket* sock, bstring str, int mode);
    bool getSubRace(Socket* sock, bstring str, int mode);
    void finishRace(Socket* sock);
    bool getClass(Socket* sock, bstring str, int mode);
    bool getDeity(Socket* sock, bstring str, int mode);
    bool getLocation(Socket* sock, const bstring& str, int mode);
    bool getStatsChoice(Socket* sock, bstring str, int mode);
    bool getStats(Socket* sock, bstring str, int mode);
    void finishStats(Socket* sock);
    bool getBonusStat(Socket* sock, bstring str, int mode);
    bool getPenaltyStat(Socket* sock, bstring str, int mode);
    bool handleWeapon(Socket* sock, int mode, char ch);
    bool getProf(Socket* sock, bstring str, int mode);
    bool getSecondProf(Socket* sock, bstring str, int mode);
    bool getPassword(Socket* sock, const bstring& str, int mode);
    void done(Socket* sock, const bstring& str, int mode);

    // character customization functions
    bool startCustom(Socket* sock, bstring str, int mode);
    bool getCommunity(Socket* sock, bstring str, int mode);
    bool getFamily(Socket* sock, bstring str, int mode);
    bool getSocial(Socket* sock, bstring str, int mode);
    bool getEducation(Socket* sock, bstring str, int mode);
    bool getHeight(Socket* sock, bstring str, int mode);
    bool getWeight(Socket* sock, bstring str, int mode);

    void doFamily(Player* player, int mode);
    int calcHeight(int race, int mode);
    int calcWeight(int race, int mode);
}

// Various login functions
void convertNewWeaponSkills(Socket* sock, const bstring& str);
void login(Socket* sock, const bstring& str);
void createPlayer(Socket* sock, const bstring& str);
void doSurname(Socket* sock, const bstring& str);
void doTitle(Socket* sock, const bstring& str);

#endif
