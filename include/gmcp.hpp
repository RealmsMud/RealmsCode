/*
 * gmcp.hpp
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

#pragma once

#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace gmcp {
    struct GmcpMapping {
        std::string msdpVar;
        std::string package;
        std::string key;
        bool numeric;
        bool structured;
    };
    const std::vector<GmcpMapping>& mappings();

    struct GmcpField {
        std::string key;
        std::string value;
        bool numeric;
    };

    std::string packageForVar(const std::string& msdpVar);
    std::vector<std::string> varsForPackage(const std::string& package);
    std::string stripSupportsVersion(const std::string& token);

    nlohmann::json buildPackageBody(const std::vector<GmcpField>& fields);

    struct ParsedMessage {
        std::string package;
        nlohmann::json data;
    };
    ParsedMessage parseMessage(std::string_view payload);
    std::set<std::string> parseSupports(const nlohmann::json& arr);
    nlohmann::json msdpTableToJson(std::string_view msdp);
    nlohmann::json msdpValueToJson(std::string_view raw);
    nlohmann::json charStatusVars();
}
