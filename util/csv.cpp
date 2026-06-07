/*
 * csv.cpp
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

#include "csv.hpp"

#include <exception>
#include <boost/tokenizer.hpp>

namespace util {

std::vector<std::vector<std::string>> parseCsv(std::istream& in) {
    std::vector<std::vector<std::string>> rows;
    // empty escape set: treat backslashes literally; comma separator; double-quote quoting
    const boost::escaped_list_separator<char> sep("", ",", "\"");

    std::string line;
    while(std::getline(in, line)) {
        if(!line.empty() && line.back() == '\r')
            line.pop_back();
        if(line.empty())
            continue;
        try {
            boost::tokenizer<boost::escaped_list_separator<char>> tok(line, sep);
            rows.emplace_back(tok.begin(), tok.end());
        } catch(const std::exception&) {
            // skip a malformed line rather than abort the whole parse
        }
    }
    return rows;
}

}
