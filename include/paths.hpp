/*
 * paths.h
 *   Operation system paths.
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
#ifndef PATHS_H_
#define PATHS_H_

#include "bstring.hpp"
#include "catRef.hpp"

namespace Path {
    extern const char* Bin;
    extern const char* Log;
    extern const char* BugLog;
    extern const char* StaffLog;
    extern const char* BankLog;
    extern const char* GuildBankLog;

    extern const char* UniqueRoom;
    extern const char* AreaRoom;
    extern const char* Monster;
    extern const char* Object;
    extern const char* Player;
    extern const char* PlayerBackup;

    extern const char* Config;

    extern const char* Code;
    extern const char* Python;
    extern const char* Game;
    extern const char* AreaData;
    extern const char* Talk;
    extern const char* Desc;
    extern const char* Sign;

    extern const char* PlayerData;
    extern const char* Bank;
    extern const char* GuildBank;
    extern const char* History;
    extern const char* Post;

    extern const char* BaseHelp;
    extern const char* Help;
    extern const char* CreateHelp;
    extern const char* Wiki;
    extern const char* DMHelp;
    extern const char* BuilderHelp;
    extern const char* HelpTemplate;

    bool checkDirExists(const char* filename);
    bool checkDirExists(const bstring& area, char* (*fn)(const CatRef &cr));

    bool checkPaths();
}


#endif /*PATHS_H_*/

