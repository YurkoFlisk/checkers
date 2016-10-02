/*
========================================================================
Copyright (c) 2016 Yurko Prokopets(aka YurkoFlisk)

This file is part of Checkers source code

Checkers is free software : you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Checkers is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Checkers.If not, see <http://www.gnu.org/licenses/>
========================================================================
*/

// tt.h, version 1.6

#pragma once
#include "move.h"

enum tt_bound : int8_t { TTBOUND_EXACT, TTBOUND_LOWER, TTBOUND_UPPER };
constexpr int TT_INDEX_BITS = 19;
constexpr int TT_SIZE = 1 << TT_INDEX_BITS;
constexpr int TT_INDEX_MASK = TT_SIZE - 1;

struct TT_Entry
{
	int16_t value;
	int8_t depth;
	tt_bound bound_type;
	Position best_move_from;
	Position best_move_to;
	uint64_t key;
	// Stores an info to entry
	inline void store(uint64_t, int16_t, int8_t, tt_bound, Position, Position);
};

class TT_Bucket
{
public:
	static constexpr int MAX_ENTRY_COUNT = 3;
	// Constructor
	TT_Bucket(void) noexcept;
	// Destructor
	~TT_Bucket(void) noexcept;
	// Finds entry corresponding to given key. If there is no such entry, returns nullptr
	const TT_Entry* find(uint64_t) const;
	// Stores an entry with given key
	void store(uint64_t, int16_t, int8_t, tt_bound, Position, Position);
	// Cleares the bucket
	inline void clear(void);
private:
	TT_Entry entries[MAX_ENTRY_COUNT];
	int16_t size;
};

class TranspositionTable
{
public:
	// Constructor
	TranspositionTable(void) noexcept;
	// Destructor
	~TranspositionTable(void) noexcept;
	// Finds entry corresponding to given key. If there is no such entry, returns nullptr
	inline const TT_Entry* find(uint64_t) const;
	// Stores an entry with given key
	inline void store(uint64_t, int16_t, int8_t, tt_bound, Position, Position);
	// Cleares the table
	inline void clear(void);
protected:
	TT_Bucket table[TT_SIZE];
};

inline void TT_Entry::store(uint64_t k, int16_t val, int8_t d, tt_bound bt, Position bm_from, Position bm_to)
{
	key = k, value = val, depth = d, bound_type = bt, best_move_from = bm_from, best_move_to = bm_to;
}

inline void TT_Bucket::clear(void)
{
	size = 0;
}

inline const TT_Entry* TranspositionTable::find(uint64_t key) const
{
	return table[key & TT_INDEX_MASK].find(key);
}

inline void TranspositionTable::store(uint64_t k, int16_t val, int8_t d, tt_bound bt, Position bm_from, Position bm_to)
{
	table[k & TT_INDEX_MASK].store(k, val, d, bt, bm_from, bm_to);
}

inline void TranspositionTable::clear(void)
{
	for (auto& bucket : table)
		bucket.clear();
}