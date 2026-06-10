/*
 * gmcp.cpp
 *   Pure GMCP (telnet opt 201) conversion layer
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

#include "gmcp.hpp"

#include <cctype>

#include "socket.hpp"      // MSDP_* delimiters

namespace gmcp {

const std::vector<GmcpMapping>& mappings() {
    static const std::vector<GmcpMapping> table = {
        {"HEALTH",             "Char.Vitals", "hp",           true,  false},
        {"HEALTH_MAX",         "Char.Vitals", "maxhp",        true,  false},
        {"MANA",               "Char.Vitals", "mp",           true,  false},
        {"MANA_MAX",           "Char.Vitals", "maxmp",        true,  false},
        {"EXPERIENCE",         "Char.Vitals", "xp",           true,  false},
        {"EXPERIENCE_MAX",     "Char.Vitals", "maxxp",        true,  false},
        {"EXPERIENCE_TNL",     "Char.Vitals", "tnl",          true,  false},
        {"EXPERIENCE_TNL_MAX", "Char.Vitals", "maxtnl",       true,  false},
        {"LEVEL",              "Char.Status", "level",        true,  false},
        {"CLASS",              "Char.Status", "class",        false, false},
        {"RACE",               "Char.Status", "race",         false, false},
        {"WIMPY",              "Char.Status", "wimpy",        true,  false},
        {"MONEY",              "Char.Status", "gold",         false, false},
        {"BANK",               "Char.Status", "bank",         false, false},
        {"ARMOR",              "Char.Status", "armor",        true,  false},
        {"ARMOR_ABSORB",       "Char.Status", "armor_absorb", true,  false},
        {"TARGET",             "Char.Status", "target",       false, false},
        {"TARGET_ID",          "Char.Status", "target_id",    false, false},
        {"TARGET_HEALTH",      "Char.Status", "target_hp",    true,  false},
        {"TARGET_HEALTH_MAX",  "Char.Status", "target_maxhp", true,  false},
        {"CHARACTER_NAME",     "Char.Name",   "name",         false, false},
        {"ROOM",               "Room.Info",   "",             false, true},
        {"GROUP",              "Char.Group",  "",             false, true},
    };
    return table;
}

std::string packageForVar(const std::string& msdpVar) {
    for (const auto& m : mappings())
        if (m.msdpVar == msdpVar)
            return m.package;
    return "";
}

std::vector<std::string> varsForPackage(const std::string& package) {
    std::vector<std::string> out;
    for (const auto& m : mappings())
        if (m.package == package)
            out.push_back(m.msdpVar);
    return out;
}

std::string stripSupportsVersion(const std::string& token) {
    auto pos = token.find_last_of(' ');
    if (pos == std::string::npos || pos + 1 >= token.size())
        return token;
    for (size_t i = pos + 1; i < token.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(token[i])))
            return token;
    return token.substr(0, pos);
}

nlohmann::json buildPackageBody(const std::vector<GmcpField>& fields) {
    nlohmann::json body = nlohmann::json::object();
    for (const auto& f : fields) {
        if (f.numeric) {
            try {
                size_t pos = 0;
                long long n = std::stoll(f.value, &pos);
                if (pos == f.value.size()) {
                    body[f.key] = n;
                    continue;
                }
            } catch (const std::exception&) {}
        }
        body[f.key] = f.value;
    }
    return body;
}

ParsedMessage parseMessage(std::string_view payload) {
    ParsedMessage m;
    auto sp = payload.find(' ');
    if (sp == std::string_view::npos) {
        m.package = std::string(payload);
        m.data = nullptr;
        return m;
    }
    m.package = std::string(payload.substr(0, sp));
    std::string_view body = payload.substr(sp + 1);
    m.data = body.empty() ? nlohmann::json(nullptr) : nlohmann::json::parse(body);
    return m;
}

std::set<std::string> parseSupports(const nlohmann::json& arr) {
    std::set<std::string> out;
    if (arr.is_array())
        for (const auto& e : arr)
            if (e.is_string())
                out.insert(stripSupportsVersion(e.get<std::string>()));
    return out;
}

namespace {
    bool isMsdpDelim(unsigned char c) {
        return c >= MSDP_VAR && c <= MSDP_ARRAY_CLOSE;
    }

    nlohmann::json parseValue(std::string_view s, size_t& i);

    std::string parseScalar(std::string_view s, size_t& i) {
        std::string out;
        while (i < s.size() && !isMsdpDelim(static_cast<unsigned char>(s[i])))
            out.push_back(s[i++]);
        return out;
    }

    // i is positioned just past MSDP_TABLE_OPEN.
    nlohmann::json parseTable(std::string_view s, size_t& i) {
        nlohmann::json obj = nlohmann::json::object();
        while (i < s.size() && static_cast<unsigned char>(s[i]) != MSDP_TABLE_CLOSE) {
            if (static_cast<unsigned char>(s[i]) == MSDP_VAR) {
                ++i;
                std::string key = parseScalar(s, i);
                if (i < s.size() && static_cast<unsigned char>(s[i]) == MSDP_VAL) {
                    ++i;
                    obj[key] = parseValue(s, i);
                } else {
                    obj[key] = std::string();
                }
            } else {
                ++i;
            }
        }
        if (i < s.size())
            ++i;
        return obj;
    }

    // i is positioned just past MSDP_ARRAY_OPEN.
    nlohmann::json parseArray(std::string_view s, size_t& i) {
        nlohmann::json arr = nlohmann::json::array();
        while (i < s.size() && static_cast<unsigned char>(s[i]) != MSDP_ARRAY_CLOSE) {
            if (static_cast<unsigned char>(s[i]) == MSDP_VAL) {
                ++i;
                arr.push_back(parseValue(s, i));
            } else {
                ++i;
            }
        }
        if (i < s.size())
            ++i;
        return arr;
    }

    nlohmann::json parseValue(std::string_view s, size_t& i) {
        if (i >= s.size())
            return std::string();
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == MSDP_TABLE_OPEN) { ++i; return parseTable(s, i); }
        if (c == MSDP_ARRAY_OPEN) { ++i; return parseArray(s, i); }
        return parseScalar(s, i);
    }
}

nlohmann::json msdpTableToJson(std::string_view msdp) {
    size_t i = 0;
    return parseValue(msdp, i);
}

nlohmann::json msdpValueToJson(std::string_view raw) {
    if (!raw.empty() && (static_cast<unsigned char>(raw[0]) == MSDP_TABLE_OPEN || static_cast<unsigned char>(raw[0]) == MSDP_ARRAY_OPEN))
        return msdpTableToJson(raw);
    return std::string(raw);
}

nlohmann::json charStatusVars() {
    return {
        {"level",        "Level"},
        {"class",        "Class"},
        {"race",         "Race"},
        {"wimpy",        "Wimpy"},
        {"gold",         "Gold"},
        {"bank",         "Bank"},
        {"armor",        "Armor"},
        {"armor_absorb", "Armor Absorb"},
        {"target",       "Target"},
        {"target_id",    "Target ID"},
        {"target_hp",    "Target Health"},
        {"target_maxhp", "Target Max Health"},
    };
}

}
