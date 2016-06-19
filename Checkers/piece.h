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

// piece.h, version 1.4

#pragma once
#ifndef _PIECE_H
#define _PIECE_H

enum colour : char { EMPTY = 0, SHADOW, WHITE, BLACK };
enum piece_type : char { PT_EMPTY = 0, PT_SHADOW, WHITE_SIMPLE, BLACK_SIMPLE, WHITE_QUEEN = 6, BLACK_QUEEN };

constexpr inline colour opposite(colour c)
{
	return c == WHITE ? BLACK : WHITE;
}

constexpr char PT_QUEEN = 1 << 2;
constexpr char PT_COLOUR = (1 << 2) - 1;

class Piece
{
public:
	constexpr Piece(void) noexcept
		: type(PT_EMPTY) {}
	constexpr Piece(const Piece& p) noexcept
		: type(p.type) {}
	constexpr Piece(colour c, bool q = false) noexcept
		: type((piece_type)(q ? (c < 2 ? c : ((q << 2) | c)) : c)) {}
	constexpr Piece(piece_type t) noexcept
		: type(t) {}
	inline Piece& operator=(const Piece& p)
	{
		type = p.type;
		return *this;
	}
	constexpr inline bool operator==(Piece rhs) const noexcept
	{
		return type == rhs.type;
	}
	constexpr inline bool operator!=(Piece rhs) const noexcept
	{
		return type != rhs.type;
	}
	constexpr inline piece_type get_type(void) const noexcept
	{
		return type;
	}
	constexpr inline bool is_queen(void) const noexcept
	{
		return type & PT_QUEEN;
	}
	constexpr inline colour get_colour(void) const noexcept
	{
		return static_cast<colour>(type & PT_COLOUR);
	}
private:
	piece_type type;
};

#endif