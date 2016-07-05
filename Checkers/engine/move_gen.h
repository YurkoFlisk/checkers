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

// move_gen.h, version 1.5

#pragma once
#include "move.h"

enum move_type : int8_t { ALL, CAPTURE, NON_CAPTURE };

class MoveGen
{
public:
	MoveGen(const Board& b)
		: board(b)
	{};
	template<colour, move_type = ALL>
	inline void get_all_moves(std::vector<Move>&) const; // Outputs to given vector all possible moves
protected:
	virtual void get_all_moves_WHITE_ALL(std::vector<Move>&) const = 0;
	virtual void get_all_moves_WHITE_CAPTURE(std::vector<Move>&) const = 0;
	virtual void get_all_moves_WHITE_NON_CAPTURE(std::vector<Move>&) const = 0;
	virtual void get_all_moves_BLACK_ALL(std::vector<Move>&) const = 0;
	virtual void get_all_moves_BLACK_CAPTURE(std::vector<Move>&) const = 0;
	virtual void get_all_moves_BLACK_NON_CAPTURE(std::vector<Move>&) const = 0;
	const Board& board;
};

class MoveGenDefault
	: public MoveGen
{
public:
	MoveGenDefault(const Board& b)
		: MoveGen(b)
	{};
protected:
	virtual inline void get_all_moves_WHITE_ALL(std::vector<Move>&) const override;
	virtual inline void get_all_moves_WHITE_CAPTURE(std::vector<Move>&) const override;
	virtual inline void get_all_moves_WHITE_NON_CAPTURE(std::vector<Move>&) const override;
	virtual inline void get_all_moves_BLACK_ALL(std::vector<Move>&) const override;
	virtual inline void get_all_moves_BLACK_CAPTURE(std::vector<Move>&) const override;
	virtual inline void get_all_moves_BLACK_NON_CAPTURE(std::vector<Move>&) const override;
	// Helper function for finding all capture-moves that can be done by a piece with given coordinates
	template<colour>
	void _find_deep_capture(std::vector<Move>&, Move&, int8_t, int8_t, bool(&)[8][8]) const;
	// Same but for queen pieces
	template<colour>
	void _find_deep_capture_queen(std::vector<Move>&, Move&, int8_t, int8_t, bool(&)[8][8]) const;
	// Main generating function
	template<colour, move_type>
	void _get_all_moves(std::vector<Move>&) const;
};

template<colour TURN, move_type MT>
inline void MoveGen::get_all_moves(std::vector<Move>& vec) const
{
	static_assert(TURN == WHITE || TURN == BLACK, "TURN must be either WHITE or BLACK");
	if (TURN == WHITE)
		if (MT == ALL)
			get_all_moves_WHITE_ALL(vec);
		else if (MT == CAPTURE)
			get_all_moves_WHITE_CAPTURE(vec);
		else
			get_all_moves_WHITE_NON_CAPTURE(vec);
	else
		if (MT == ALL)
			get_all_moves_BLACK_ALL(vec);
		else if (MT == CAPTURE)
			get_all_moves_BLACK_CAPTURE(vec);
		else
			get_all_moves_BLACK_NON_CAPTURE(vec);
}

inline void MoveGenDefault::get_all_moves_WHITE_ALL(std::vector<Move>& vec) const
{
	_get_all_moves<WHITE, ALL>(vec);
}

inline void MoveGenDefault::get_all_moves_WHITE_CAPTURE(std::vector<Move>& vec) const
{
	_get_all_moves<WHITE, CAPTURE>(vec);
}

inline void MoveGenDefault::get_all_moves_WHITE_NON_CAPTURE(std::vector<Move>& vec) const
{
	_get_all_moves<WHITE, NON_CAPTURE>(vec);
}

inline void MoveGenDefault::get_all_moves_BLACK_ALL(std::vector<Move>& vec) const
{
	_get_all_moves<BLACK, ALL>(vec);
}

inline void MoveGenDefault::get_all_moves_BLACK_CAPTURE(std::vector<Move>& vec) const
{
	_get_all_moves<BLACK, CAPTURE>(vec);
}

inline void MoveGenDefault::get_all_moves_BLACK_NON_CAPTURE(std::vector<Move>& vec) const
{
	_get_all_moves<BLACK, NON_CAPTURE>(vec);
}