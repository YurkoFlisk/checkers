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

// move.h, version 1.7

#pragma once
#ifndef _MOVE_H
#define _MOVE_H
#include "piece.h"
#include "position.h"
#include "svector.h"

typedef SVector<Position, 12> Path;
typedef SVector<std::pair<Position, Piece>, 11> CaptureList;

class Move
{
	friend class MoveGenDefault;
	friend class MoveGenEnglish;
	friend class Board;
	friend class Checkers;
public:
	Move(void) noexcept {}
	Move(Position p) noexcept : path(1, p) {};
	Move(const Move& m) noexcept : original(m.original), become(m.become),
		path(m.path), captured(m.captured) {}
	~Move(void) noexcept {}
	inline Move& operator=(const Move& m)
	{
		original = m.original;
		become = m.become;
		captured = m.captured;
		path = m.path;
		return *this;
	}
	inline bool operator==(const Move& rhs) const noexcept
	{
		return path == rhs.path;
	}
	inline bool operator!=(const Move& rhs) const noexcept
	{
		return path != rhs.path;
	}
	inline const Position& operator[](int idx) const
	{
		return path[idx];
	}
	inline int size(void) const noexcept
	{
		return path.size();
	}
	inline int capt_size(void) const noexcept
	{
		return captured.size();
	}
	inline const Path& get_path(void) const noexcept
	{
		return path;
	}
	inline const Position& old_pos(void) const
	{
		return path.front();
	}
	inline const Position& new_pos(void) const
	{
		return path.back();
	}
	inline void add_step(const Position& pos)
	{
		path.add(pos);
	}
	inline void pop_step(void)
	{
		path.pop();
	}
	inline void swap(Move& rhs)
	{
		std::swap(original, rhs.original);
		std::swap(become, rhs.become);
		::swap(path, rhs.path);
		::swap(captured, rhs.captured);
	}
	inline std::pair<Position, Piece> get_last_captured(void) const
	{
		return captured.back();
	}
	// Should not be used until move information for current path is set (It's set by legal_move
	// function in Checkers class. For moves returned from Checkers object it's set automatically)
	inline Piece get_original(void) const noexcept
	{
		return original;
	}
	// Should not be used until move information for current path is set (It's set by legal_move
	// function in Checkers class. For moves returned from Checkers object it's set automatically)
	inline Piece get_become(void) const noexcept
	{
		return become;
	}
	// Should not be used until move information for current path is set (It's set by legal_move
	// function in Checkers class. For moves returned from Checkers object it's set automatically)
	inline const CaptureList& get_captured(void) const noexcept
	{
		return captured;
	}
private:
	inline void set_info_from(const Move& rhs) noexcept
	{
		original = rhs.original;
		become = rhs.become;
		captured = rhs.captured;
	}
	inline void set_original(Piece orig) noexcept
	{
		original = orig;
	}
	inline void set_become(Piece bec) noexcept
	{
		become = bec;
	}
	inline void add_capture(const std::pair<Position, Piece>& cap)
	{
		captured.add(cap);
	}
	inline void pop_capture(void)
	{
		captured.pop();
	}
	Piece original;
	Piece become;
	Path path;
	CaptureList captured;
};

inline void swap(Move& lhs, Move& rhs)
{
	lhs.swap(rhs);
}

#endif