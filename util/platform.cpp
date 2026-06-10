/*
 * platform.cpp
 *   OS-specific helpers. The only translation unit carrying per-OS #ifdef guards.
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
#include "platform.hpp"

#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>            // for _NSGetExecutablePath
#include <vector>
#else
#include <climits>                  // for PATH_MAX
#include <unistd.h>                 // for readlink
#endif

namespace fs = std::filesystem;

fs::path platform::executableDir() {
#ifdef __APPLE__
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);   // first call reports the buffer size needed
    std::vector<char> buf(size);
    if(_NSGetExecutablePath(buf.data(), &size) != 0)
        return fs::current_path();
    return fs::weakly_canonical(fs::path(buf.data())).parent_path();
#else
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if(len == -1)
        return fs::current_path();
    buf[len] = '\0';
    return fs::path(buf).parent_path();
#endif
}
