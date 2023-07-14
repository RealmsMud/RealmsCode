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

#pragma once

#include <filesystem>

namespace fs = std::filesystem;

class CatRef;

struct Path {
    static inline const fs::path BasePath = "/home/realms/realms";
    static inline const fs::path Bin = BasePath / "bin";
    static inline const fs::path Log = BasePath / "log";
    static inline const fs::path BugLog = BasePath / "log/bug";
    static inline const fs::path StaffLog = BasePath / "log/staff";
    static inline const fs::path BankLog = BasePath / "log/bank";
    static inline const fs::path GuildBankLog = BasePath / "log/guildbank";

    static inline const fs::path UniqueRoom = BasePath / "rooms";
    static inline const fs::path AreaRoom = BasePath / "rooms/area";
    static inline const fs::path Monster = BasePath / "monsters";
    static inline const fs::path Object = BasePath / "objects";
    static inline const fs::path Player = BasePath / "player";
    static inline const fs::path PlayerBackup = BasePath / "player/backup";

    static inline const fs::path Config = BasePath / "config";

    static inline const fs::path Code = Config / "code";
// First check the docker install path; then the code directory, and finally fall back to the old place
    static inline const fs::path Python = "/build/pythonLib/:" + BasePath.string() + "/RealmsCode/pythonLib:" + BasePath.string() + "/config/code/python/";
    static inline const fs::path Game = Config / "game";
    static inline const fs::path AreaData = Game / "area";
    static inline const fs::path Talk = Game / "talk";
    static inline const fs::path Desc = Game / "ddesc";
    static inline const fs::path Sign = Game / "signs";

    static inline const fs::path PlayerData = Config / "player";
    static inline const fs::path Bank = PlayerData / "bank";
    static inline const fs::path GuildBank = PlayerData / "guildbank";
    static inline const fs::path History = PlayerData / "history";
    static inline const fs::path Post = PlayerData / "post";

    static inline const fs::path BaseHelp = BasePath / "help";
    static inline const fs::path Help = BaseHelp / "help";
    static inline const fs::path CreateHelp = BaseHelp / "create";
    static inline const fs::path Wiki = BaseHelp / "wiki";
    static inline const fs::path DMHelp = BaseHelp / "dmhelp";
    static inline const fs::path BuilderHelp = BaseHelp / "bhelp";
    static inline const fs::path HelpTemplate = BaseHelp / "template";

    static inline const fs::path Zone = BasePath / "zones";

    static bool checkDirExists(const fs::path& path);
    static bool checkDirExists(const std::string &area, fs::path (*fn)(const CatRef &cr));

    static bool checkPaths();

    static fs::path objectPath(const CatRef& cr);
    static fs::path monsterPath(const CatRef& cr);
    static fs::path roomPath(const CatRef& cr);
    static fs::path roomBackupPath(const CatRef& cr);
};

