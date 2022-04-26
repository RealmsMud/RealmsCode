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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef PATHS_H_
#define PATHS_H_

#include <filesystem>

namespace fs = std::filesystem;

class CatRef;

namespace Path {
    inline fs::path Bin = "/home/realms/realms/bin/";
    inline fs::path Log = "/home/realms/realms/log/";
    inline fs::path BugLog = "/home/realms/realms/log/bug/";
    inline fs::path StaffLog = "/home/realms/realms/log/staff/";
    inline fs::path BankLog = "/home/realms/realms/log/bank/";
    inline fs::path GuildBankLog = "/home/realms/realms/log/guildbank/";

    inline fs::path UniqueRoom = "/home/realms/realms/rooms/";
    inline fs::path AreaRoom = "/home/realms/realms/rooms/area/";
    inline fs::path Monster = "/home/realms/realms/monsters/";
    inline fs::path Object = "/home/realms/realms/objects/";
    inline fs::path Player = "/home/realms/realms/player/";
    inline fs::path PlayerBackup = "/home/realms/realms/player/backup/";

    inline fs::path Config = "/home/realms/realms/config/";

    inline fs::path Code = "/home/realms/realms/config/code/";
    // First check the docker install path; then the code directory, and finally fall back to the old place
    inline fs::path Python = "/build/pythonLib/:/home/realms/realms/RealmsCode/pythonLib:/home/realms/realms/config/code/python/";
    inline fs::path Game = "/home/realms/realms/config/game/";
    inline fs::path AreaData = "/home/realms/realms/config/game/area/";
    inline fs::path Talk = "/home/realms/realms/config/game/talk/";
    inline fs::path Desc = "/home/realms/realms/config/game/ddesc/";
    inline fs::path Sign = "/home/realms/realms/config/game/signs/";

    inline fs::path PlayerData = "/home/realms/realms/config/player/";
    inline fs::path Bank = "/home/realms/realms/config/player/bank/";
    inline fs::path GuildBank = "/home/realms/realms/config/player/guildbank/";
    inline fs::path History = "/home/realms/realms/config/player/history/";
    inline fs::path Post = "/home/realms/realms/config/player/post/";

    inline fs::path BaseHelp = "/home/realms/realms/help/";
    inline fs::path Help = "/home/realms/realms/help/help/";
    inline fs::path CreateHelp = "/home/realms/realms/help/create/";
    inline fs::path Wiki = "/home/realms/realms/help/wiki/";
    inline fs::path DMHelp = "/home/realms/realms/help/dmhelp/";
    inline fs::path BuilderHelp = "/home/realms/realms/help/bhelp/";
    inline fs::path HelpTemplate = "/home/realms/realms/help/template/";

    bool checkDirExists(const char* filename);
    bool checkDirExists(const std::string &area, char* (*fn)(const CatRef &cr));

    bool checkPaths();
}


#endif /*PATHS_H_*/

