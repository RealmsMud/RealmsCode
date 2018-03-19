/*
 * lru_cache.h
 *   LRU Cache - Inspired from https://github.com/paudley/lru_cache
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef INCLUDE_LRU_CACHE_HPP_
#define INCLUDE_LRU_CACHE_HPP_

#include <unordered_map>
#include <list>
#include <vector>

#include "lru/statistics.hpp"

const bool MONITOR_STATS = true;

namespace LRU {

template < class T >
struct CleanUpFn {
		void operator()( const T &x ) { delete x; }
};

template< class key_t, class data_t, class clean_up_fn = CleanUpFn< data_t > > class lru_cache {
public:
	using list_t = std::list< std::pair< key_t, data_t > >;       // Main cache storage typedef
	using list_iter_t = typename list_t::iterator;                // Main cache iterator
	using list_citer_t = typename list_t::const_iterator;         // Main cache iterator (const)
	using key_list_t = std::vector< key_t >;                      // List of keys
	using key_list_iter_t = typename key_list_t::iterator;        // Main cache iterator
	using key_list_citer_t = typename key_list_t::const_iterator; // Main cache iterator (const)
	using map_t = std::unordered_map< key_t, list_iter_t >;       // Index typedef
	using pair_t = std::pair< key_t, list_iter_t >;               // Pair of map elements
	using map_iter_t = typename map_t::iterator;			      // Index iterator
	using map_citer_t = typename map_t::const_iterator;           // Index iterator (const)

private:
	list_t _items_list;               	// Main cache storage
	map_t  _items_map;                	// Cache storage index
	size_t _max_size;  				  	// Maximum abstract size of the cache
	LRU::Statistics<key_t> _stats;		// Cache hit/miss & keys

public:

	// Constructor/Deconstructor
	lru_cache(size_t max_size): _max_size(max_size), _stats(MONITOR_STATS)  {}
	~lru_cache() { clear(); }


	// Clear the list and index
	void clear(void) {
		_items_list.clear();
		_items_map.clear();
	};

	// Does the cache contain this key?
	//  - Records cache hit/miss
	inline bool contains(const key_t &key) {
		return find(key, true) != end();
	}

	// Remove this key from the cache
	// - Does not record cache hit/miss
	inline void remove(const key_t &key) {
		map_iter_t m_iter = _items_map.find(key);

		if(m_iter == _items_map.end())
			return;

		_remove(m_iter);
	}

	// How big is the cache?
	size_t size() const noexcept {
		return _items_map.size();
	}

	size_t capacity() const noexcept {
		return _max_size;
	}

	double utilization() const noexcept {
		return static_cast<double>(size()) / capacity();
	}

	//******************************************************************
	// Iterators
	//******************************************************************

	map_iter_t end() noexcept {
		return _items_map.end();
	}

	map_citer_t end() const noexcept {
		return _items_map.cend();
	}

	map_iter_t begin() noexcept {
		return _items_map.begin();
	}

	map_citer_t begin() const noexcept {
		return _items_map.cbegin();
	}

	map_iter_t find(const key_t &key, bool touch = true) {
		map_iter_t miter = _items_map.find(key);

		if(miter != _items_map.end()) {
			if(touch)
				_touch(miter->second);
			_register_hit(key);
		} else {
			_register_miss(key);
		}

		return miter;
	}

	map_citer_t find(const key_t &key, bool touch = true) const {
		auto miter = _items_map.find(key);

		if(miter != _items_map.end()) {
			if(touch)
				_touch(miter->second);
			_register_hit(key);
		} else {
			_register_miss(key);
		}

		return miter;
	}

	inline void touch( const key_t &key ) {
		_touch(key);
	}

	// Fetch a pointer to the data
	inline data_t *fetch_ptr(const key_t &key, bool touch = true ) {
		map_iter_t miter = find(key, touch);
		if (miter == end()) {
			return nullptr;
		}
		return &(miter->second->second);
	}

	// Fetch a copy of the data
	inline data_t fetch(const key_t &key, bool touch = true ) {
		map_iter_t miter = find(key, touch);
		if(miter == end())
			return nullptr;
		return miter->second->second;
	}

	// Fetch data and return whether it was found
	inline bool fetch(const key_t &key, data_t &data, bool touch=true ) {
		map_iter_t miter = find(key, touch);
		if(miter == end()) {
			return false;
		}
		data = miter->second->second;
		return true;
	}

	// Insert a new (key, data) pair
	inline void insert(const key_t &key, const data_t &data) {
		// Touch the key, if it exists, then replace the content.
		map_iter_t miter = _touch(key);
		if(miter != _items_map.end())
			_remove(miter);

		// Ok, do the actual insert at the head of the list
		_items_list.push_front(std::make_pair(key, data));
		list_iter_t liter = _items_list.begin();

		// Store the index
		_items_map.insert(std::make_pair(key, liter));

		// Check to see if we need to remove an element due to exceeding max_size
		while(_items_map.size() > _max_size) {
			// Remove the last element.
			liter = _items_list.end();
			liter--;
			_remove(liter->first);
		}
	}

	// Get a list of all keys - Mainly for debugging
	inline const key_list_t get_all_keys( void ) {
		key_list_t ret;
		for( list_citer_t liter : _items_list)
			ret.push_back(liter->first);
		return ret;
	}

	std::string get_stat_info(bool extended=false) {
	    std::ostringstream oStr;
	    oStr << "Size: " << size() << "/" << capacity() << " - " << utilization()*100.0 << "%" << std::endl;;
	    oStr << "Hit Rate: " << _stats.hit_rate() << "%  Miss Rate: " << _stats.miss_rate() << "%" << std::endl;
	    if(extended) {
	    	oStr << _stats.detail_status();
	    }
	    return(oStr.str());
	}

private:
	// Touch a key (move it to the front)
	inline map_iter_t _touch(const key_t &key) {
		map_iter_t miter = _items_map.find(key);
		if(miter == _items_map.end())
			return miter;
		_touch(miter->second);
		return miter;
	}

	// Touch a list iterator
	void _touch(list_iter_t liter) {
		// Move the found node to the head of the list.
		_items_list.splice( _items_list.begin(), _items_list, liter );
	}

	// Remove a key
	inline void _remove( const key_t &key ) {
		map_iter_t miter = _items_map.find(key);
		_remove(miter);
	}

	// Remove an items map iterator and associated items
	inline void _remove( const map_iter_t &miter ) {
		data_t data = miter->second->second;
		_items_list.erase(miter->second);
		_items_map.erase(miter);
		clean_up_fn()(data);
	}

	inline void _register_miss(const key_t &key) {
		_stats.register_miss(key);
	}

	inline void _register_hit(const key_t &key) {
		_stats.register_hit(key);
	}
};

} // Namespace LRU

#endif /* INCLUDE_LRU_CACHE_HPP_ */
