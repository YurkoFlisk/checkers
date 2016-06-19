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

// move.h, version 1.4

#pragma once
#ifndef _MOVE_H
#define _MOVE_H
#include <vector>
#include "piece.h"
#include "position.h"

typedef std::vector<Position> Path;
typedef std::vector<std::pair<Position, Piece>> CaptureList;

class Move
{
	friend class Checkers;
public:
	Move(void) noexcept {}
	Move(const Move& m) noexcept
		: path(m.path), captured(m.captured)
		, original(m.original), become(m.become) {}
	Move(Move&& m) noexcept
		: path(std::move(m.path)), captured(std::move(m.captured))
		, original(m.original), become(m.become) {}
	Move(const Path& p) noexcept
		: path(p) {}
	Move(Path&& p) noexcept
		: path(std::move(p)) {}
	~Move(void) noexcept {};
	inline Move& operator=(const Move& m)
	{
		path = m.path;
		captured = m.captured;
		original = m.original;
		become = m.become;
		return *this;
	}
	inline Move& operator=(Move&& m)
	{
		path = std::move(m.path);
		captured = std::move(m.captured);
		original = m.original;
		become = m.become;
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
	inline const Position& operator[](size_t idx) const
	{
		return path[idx];
	}
	inline size_t size(void) const noexcept
	{
		return path.size();
	}
	inline const Path& get_path(void) const noexcept
	{
		return path;
	}
	inline const Position& old_pos(void) const
	{
		return path[0];
	}
	inline const Position& new_pos(void) const
	{
		return path.back();
	}
	inline void add_step(const Position& pos)
	{
		path.push_back(pos);
	}
	inline void pop_step(void)
	{
		path.pop_back();
	}
	inline void swap(Move& rhs)
	{
		std::swap(path, rhs.path);
		std::swap(captured, rhs.captured);
		std::swap(original, rhs.original);
		std::swap(become, rhs.become);
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
		captured.push_back(cap);
	}
	inline void pop_capture(void)
	{
		captured.pop_back();
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