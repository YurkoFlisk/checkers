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

// position.h, version 1.4

#pragma once
#ifndef _POSITION_H
#define _POSITION_H

class Position
{
	friend class Board;
public:
	constexpr Position(void) noexcept
		: row(0), column(0) {}
	constexpr Position(const Position& p) noexcept
		: row(p.row), column(p.column) {}
	constexpr Position(int r, int c) noexcept
		: row(r), column(c) {}
	inline Position& operator=(const Position& p)
	{
		row = p.row;
		column = p.column;
		return *this;
	}
	constexpr inline bool operator==(const Position& rhs) const noexcept
	{
		return row == rhs.row && column == rhs.column;
	}
	constexpr inline bool operator!=(const Position& rhs) const noexcept
	{
		return row != rhs.row || column != rhs.column;
	}
	constexpr inline int get_row(void) const noexcept
	{
		return row;
	}
	constexpr inline int get_column(void) const noexcept
	{
		return column;
	}
	inline void set_row(int r) noexcept
	{
		row = r;
	}
	inline void set_column(int c) noexcept
	{
		column = c;
	}
private:
	int row;
	int column;
};

inline int pos_idx(const Position& pos) noexcept
{
	return (pos.get_row() << 3) + pos.get_column();
}

#endif