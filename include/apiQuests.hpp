/*
 * apiQuests.hpp
 *   REST-side accessors for the on-disk per-zone JSON quest store
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
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "paths.hpp"

// The REST API reads quests straight from the per-zone JSON store (Path::Zone/<zone>/quests/),
// independent of the in-game XML store (gConfig->quests). `base` is injectable for tests.

std::filesystem::path apiQuestJsonPath(const std::string& zone, int id, const std::filesystem::path& base = Path::Zone);

std::vector<int> apiScanQuestIds(const std::string& zone, const std::filesystem::path& base = Path::Zone);

std::optional<nlohmann::json> apiReadQuestJson(const std::string& zone, int id, const std::filesystem::path& base = Path::Zone);
