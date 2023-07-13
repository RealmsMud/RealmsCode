/*
 * key-statistics.h
 *   LRU Cache - Key specific statistics
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
/*
 * LRU Cache:
 *  Inspiration from:
 *    https://github.com/paudley/lru_cache
 *    https://github.com/goldsborough/lru-cache
 */

#pragma once

#include <cstddef>

namespace LRU {

class KeyStatistics {
public:
	size_t hits;
	size_t misses;

	KeyStatistics(size_t hits_=0, size_t misses_=0): hits(hits_), misses(misses_) {}

	size_t accesses() const {
		return hits + misses;
	}

	void reset() {
		hits = misses = 0;
	}
};

} // Namespace LRU
