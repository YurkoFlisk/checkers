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

// tt.cpp, version 1.7

#include "tt.h"

TT_Bucket::TT_Bucket(void) noexcept
	: size(0)
{}

TT_Bucket::~TT_Bucket(void) noexcept = default;

TranspositionTable::TranspositionTable(void) noexcept = default;

TranspositionTable::~TranspositionTable(void) noexcept = default;

const TT_Entry* TT_Bucket::find(uint64_t key) const
{
	for (int i = 0; i < size; ++i)
		if (entries[i].key == key)
			return entries + i;
	return nullptr;
}

void TT_Bucket::store(uint64_t k, int16_t val, int16_t ag, int8_t d, tt_bound bt, PseudoMove pseudo_bm)
{
	if (size < MAX_ENTRY_COUNT)
	{
		for (int i = 0; i < size; ++i)
			if (entries[i].key == k)
			{
				if (entries[i].depth < d || (entries[i].depth == d
					&& tt_bound_better(bt, entries[i].bound_type)))
					entries[i].store(k, val, ag, d, bt, pseudo_bm);
				/*if (entries[i].bound_type == TTBOUND_EXACT ?
					bt == TTBOUND_EXACT && entries[i].depth < d : entries[i].depth <= d)
					entries[i].store(k, val, ag, d, bt, pseudo_bm);*/
				return;
			}
		entries[size++].store(k, val, ag, d, bt, pseudo_bm);
	}
	else
	{
		TT_Entry* replace = entries;
		for (int i = 0; i < MAX_ENTRY_COUNT; ++i)
			if (entries[i].key == k)
			{
				if (entries[i].depth < d || (entries[i].depth == d
					&& tt_bound_better(bt, entries[i].bound_type)))
					entries[i].store(k, val, ag, d, bt, pseudo_bm);
				/*if (entries[i].bound_type == TTBOUND_EXACT ?
					bt == TTBOUND_EXACT && entries[i].depth < d : entries[i].depth <= d)
					entries[i].store(k, val, ag, d, bt, pseudo_bm);*/
				return;
			}
			else if (i > 0 && (entries[i].age < replace->age || (entries[i].age == replace->age
				&& (entries[i].depth < replace->depth || (entries[i].depth == replace->depth
					&& tt_bound_better(replace->bound_type, entries[i].bound_type))))))
				replace = entries + i;
			/*else if (i > 0 && (entries[i].age < replace->age || (entries[i].age == replace->age
				&& (entries[i].bound_type == TTBOUND_EXACT ?
					replace->bound_type == TTBOUND_EXACT && entries[i].depth < replace->depth :
					entries[i].depth <= replace->depth))))
				replace = entries + i;*/
		replace->store(k, val, ag, d, bt, pseudo_bm);
	}
}