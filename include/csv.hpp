/*
 * csv.hpp
 *   Minimal, robust CSV reader (quoted fields, RFC-ish)
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

#include <istream>
#include <string>
#include <vector>

namespace util {

// Parse CSV into rows of fields. Quoted fields (") may contain commas; a trailing CR
// (CRLF input) is tolerated; blank and unparseable lines are skipped. Header handling
// is the caller's concern.
std::vector<std::vector<std::string>> parseCsv(std::istream& in);

}
