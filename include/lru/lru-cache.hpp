/*
 * lru_cache.h
 *   LRU Cache
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

#ifndef INCLUDE_LRU_CACHE_HPP_
#define INCLUDE_LRU_CACHE_HPP_

#include <unordered_map>
#include <list>
#include <vector>

#include "lru/statistics.hpp"

const bool MONITOR_STATS = true;

namespace LRU {

template < class T >
struct CanCleanupFn {
	bool operator()( const T *x ) { return true; }
};

template < class T >
struct CleanUpFn {
		void operator()( const T *x ) { delete x; }
};

template< class key_t, class data_t, class clean_up_fn = CleanUpFn< data_t >, class can_clean_up_fn = CanCleanupFn< data_t > > class lru_cache {
public:
	using list_t = std::list< std::pair< key_t, data_t* > >;       // Main cache storage typedef
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
	bool   _is_reference;				// Are we storing a reference, or a copy, of the data
	LRU::Statistics<key_t> _stats;		// Cache hit/miss & keys

public:

	// Constructor/Deconstructor
	lru_cache(size_t max_size, bool is_reference): _max_size(max_size), _is_reference(is_reference), _stats(MONITOR_STATS) {}
	~lru_cache() { clear(); }


	// Clear the list and index
	void clear(void) {
		for(auto m_iter : _items_map) {
			clean_up_fn()(m_iter.second->second);
		}
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
		map_iter_t m_iter = _items_map.find(key);

		if(m_iter != _items_map.end()) {
			if(touch)
				_touch(m_iter->second);
			_register_hit(key);
		} else {
			_register_miss(key);
		}

		return m_iter;
	}

	map_citer_t find(const key_t &key, bool touch = true) const {
		auto m_iter = _items_map.find(key);

		if(m_iter != _items_map.end()) {
			if(touch)
				_touch(m_iter->second);
			_register_hit(key);
		} else {
			_register_miss(key);
		}

		return m_iter;
	}

	inline void touch( const key_t &key ) {
		_touch(key);
	}

	// Fetch a copy of the data
	inline data_t *fetch(const key_t &key, bool touch = true ) {
		map_iter_t m_iter = find(key, touch);
		if(m_iter == end())
			return nullptr;
		return m_iter->second->second;
	}

	// Fetch data and return whether it was found
	inline bool fetch(const key_t &key, data_t **data, bool touch=true ) {
		map_iter_t m_iter = find(key, touch);
		if(m_iter == end()) {
			return false;
		}
		if (_is_reference) {
			*data = m_iter->second->second;
		} else {
			**data = *m_iter->second->second;
		}
		return true;
	}

	// Insert a new (key, data) pair
	inline void insert(const key_t &key, data_t **data) {
		// Touch the key, if it exists, then replace the content.
		map_iter_t m_iter = _touch(key);
		if(m_iter != _items_map.end())
			_remove(m_iter);
		data_t *to_insert;

		if (_is_reference) {
			to_insert = *data;
		} else {
			to_insert = new data_t;
			*to_insert = **data;
		}

		// Ok, do the actual insert at the head of the list
		_items_list.push_front(std::make_pair(key, to_insert));
		list_iter_t l_iter = _items_list.begin();

		// Store the index
		_items_map.insert(std::make_pair(key, l_iter));

		// Check to see if we need to remove an element due to exceeding max_size
		int sanity_check = 0;
		while(_items_map.size() > _max_size) {
			// Remove the last element.
			l_iter = _items_list.end();
			l_iter--;
			if (can_clean_up_fn()(l_iter->second)) {
				_remove(l_iter->first);
			} else {
				sanity_check++;
				if(sanity_check > _max_size / 2) {
					throw std::range_error("Couldn't clean up more than half of the cache.");
				}
				_touch(l_iter);
			}
		}
	}

	// Get a list of all keys - Mainly for debugging
	inline const key_list_t get_all_keys( void ) {
		key_list_t ret;
		for( list_citer_t l_iter : _items_list)
			ret.push_back(l_iter->first);
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
		map_iter_t m_iter = _items_map.find(key);
		if(m_iter == _items_map.end())
			return m_iter;
		_touch(m_iter->second);
		return m_iter;
	}

	// Touch a list iterator
	void _touch(list_iter_t l_iter) {
		// Move the found node to the head of the list.
		_items_list.splice( _items_list.begin(), _items_list, l_iter );
	}

	// Remove a key
	inline void _remove( const key_t &key ) {
		map_iter_t m_iter = _items_map.find(key);
		_remove(m_iter);
	}

	// Remove an items map iterator and associated items
	inline void _remove( const map_iter_t &m_iter ) {
		data_t* data = m_iter->second->second;
		_items_list.erase(m_iter->second);
		_items_map.erase(m_iter);
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
