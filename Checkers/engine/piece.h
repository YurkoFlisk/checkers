/*
========================================================================
Copyright (c) 2016-2017 Yurko Prokopets(aka YurkoFlisk)

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

// piece.h, version 1.7

#pragma once
#ifndef _PIECE_H
#define _PIECE_H
#include <cstdint>

enum colour : int8_t { EMPTY = 0, SHADOW, WHITE, BLACK };
enum piece_type : int8_t { PT_EMPTY = 0, PT_SHADOW, WHITE_SIMPLE, BLACK_SIMPLE, WHITE_QUEEN = 6, BLACK_QUEEN };
constexpr int8_t PT_COUNT = BLACK_QUEEN + 1;
constexpr int8_t PT_QUEEN = 1 << 2;
constexpr int8_t PT_COLOUR = (1 << 2) - 1;

constexpr inline colour opposite(colour c)
{
	return c == WHITE ? BLACK : WHITE;
}

constexpr inline piece_type turn_simple(colour c)
{
	return c == WHITE ? WHITE_SIMPLE : BLACK_SIMPLE;
}

constexpr inline piece_type turn_queen(colour c)
{
	return c == WHITE ? WHITE_QUEEN : BLACK_QUEEN;
}

constexpr inline colour get_colour(piece_type pt) noexcept
{
	return static_cast<colour>(pt & PT_COLOUR);
}

constexpr inline bool is_queen(piece_type pt) noexcept
{
	return pt & PT_QUEEN;
}

class Piece
{
public:
	Piece(void) noexcept {}
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
		return ::is_queen(type);
	}
	constexpr inline colour get_colour(void) const noexcept
	{
		return ::get_colour(type);
	}
private:
	piece_type type;
};

#endif