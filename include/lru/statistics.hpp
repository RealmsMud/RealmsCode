/*
 * statistics.h
 *   LRU Cache - Statistics
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
#include <sstream>
#include <unordered_map>

#include "lru/key-statistics.hpp"

namespace LRU {

template <typename key_t>
class Statistics {
public:
	Statistics(bool monitor_key_stats): _monitor_key_stats(monitor_key_stats), _total_hits(0), _total_accesses(0) {};

	void start_monitoring() {
		_monitor_key_stats = true;
	}

	void stop_monitoring() {
		_monitor_key_stats = false;
	}

	void register_hit(const key_t& key) {
		_total_accesses++;
		_total_hits++;
		if (_monitor_key_stats) {
			_hit_map[key].hits++;
		}
	}

	void register_miss(const key_t& key) {
		_total_accesses++;
		if (_monitor_key_stats) {
			_hit_map[key].misses++;
		}
	}

	size_t total_accesses() const noexcept {
		return _total_accesses;
	}

	size_t total_hits() const noexcept {
		return _total_hits;
	}

	double hit_rate() const noexcept {
		return static_cast<double>(total_hits()) / total_accesses();
	}

	double miss_rate() const noexcept {
		return 1 - hit_rate();
	}

	std::string detail_status() {
	    std::ostringstream oStr;
	    for(auto it : _hit_map) {
	    	oStr << it.first.str() << ":" << it.second.hits << "/" << it.second.accesses() << '|';
	    }
	    oStr << std::endl;
	    return(oStr.str());

	}

private:
	using HitMap = std::unordered_map<key_t, KeyStatistics>;

	bool _monitor_key_stats;
	size_t _total_hits;
	size_t _total_accesses;
	HitMap _hit_map;
};


} // Namespace LRU
