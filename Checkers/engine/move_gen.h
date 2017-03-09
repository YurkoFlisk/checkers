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

// move_gen.h, version 1.7

#pragma once
#ifndef _MOVEGEN_H
#define _MOVEGEN_H
#include "move.h"

constexpr int MAX_MOVES_COUNT = 100;
enum move_type : int8_t { ALL, CAPTURE, NON_CAPTURE };

struct MLNode
{
	Move move;
	int score;
};

inline bool operator>(const MLNode& ml1, const MLNode& ml2)
{
	return ml1.score > ml2.score;
}

typedef SVector<MLNode, MAX_MOVES_COUNT> MoveList;

class MoveGen
{
public:
	MoveGen(const Board& b)
		: board(b)
	{}
	template<colour, move_type = ALL>
	inline void get_all_moves(MoveList&) const; // Outputs to given vector all possible moves
protected:
	virtual void get_all_moves_WHITE_ALL(MoveList&) const = 0;
	virtual void get_all_moves_WHITE_CAPTURE(MoveList&) const = 0;
	virtual void get_all_moves_WHITE_NON_CAPTURE(MoveList&) const = 0;
	virtual void get_all_moves_BLACK_ALL(MoveList&) const = 0;
	virtual void get_all_moves_BLACK_CAPTURE(MoveList&) const = 0;
	virtual void get_all_moves_BLACK_NON_CAPTURE(MoveList&) const = 0;
	const Board& board;
};

class MoveGenDefault
	: public MoveGen
{
public:
	MoveGenDefault(const Board& b)
		: MoveGen(b)
	{}
protected:
	virtual inline void get_all_moves_WHITE_ALL(MoveList&) const override;
	virtual inline void get_all_moves_WHITE_CAPTURE(MoveList&) const override;
	virtual inline void get_all_moves_WHITE_NON_CAPTURE(MoveList&) const override;
	virtual inline void get_all_moves_BLACK_ALL(MoveList&) const override;
	virtual inline void get_all_moves_BLACK_CAPTURE(MoveList&) const override;
	virtual inline void get_all_moves_BLACK_NON_CAPTURE(MoveList&) const override;
	// Helper function for finding all capture-moves that can be done by a piece with given coordinates
	template<colour>
	void _find_deep_capture(MoveList&, Move&, int8_t, int8_t, bool(&)[8][8]) const;
	// Same but for queen pieces
	template<colour>
	void _find_deep_capture_queen(MoveList&, Move&, int8_t, int8_t, bool(&)[8][8]) const;
	// Main generating function
	template<colour, move_type>
	void _get_all_moves(MoveList&) const;
};

class MoveGenEnglish
	: public MoveGen
{
public:
	MoveGenEnglish(const Board& b)
		: MoveGen(b)
	{}
protected:
	virtual inline void get_all_moves_WHITE_ALL(MoveList&) const override;
	virtual inline void get_all_moves_WHITE_CAPTURE(MoveList&) const override;
	virtual inline void get_all_moves_WHITE_NON_CAPTURE(MoveList&) const override;
	virtual inline void get_all_moves_BLACK_ALL(MoveList&) const override;
	virtual inline void get_all_moves_BLACK_CAPTURE(MoveList&) const override;
	virtual inline void get_all_moves_BLACK_NON_CAPTURE(MoveList&) const override;
	// Helper function for finding all capture-moves that can be done by a piece with given coordinates
	template<colour>
	void _find_deep_capture(MoveList&, Move&, int8_t, int8_t) const;
	// Same but for queen pieces
	template<colour>
	void _find_deep_capture_queen(MoveList&, Move&, int8_t, int8_t, bool(&)[8][8]) const;
	// Main generating function
	template<colour, move_type>
	void _get_all_moves(MoveList&) const;
};

template<colour TURN, move_type MT>
inline void MoveGen::get_all_moves(MoveList& vec) const
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

inline void MoveGenDefault::get_all_moves_WHITE_ALL(MoveList& vec) const
{
	_get_all_moves<WHITE, ALL>(vec);
}

inline void MoveGenDefault::get_all_moves_WHITE_CAPTURE(MoveList& vec) const
{
	_get_all_moves<WHITE, CAPTURE>(vec);
}

inline void MoveGenDefault::get_all_moves_WHITE_NON_CAPTURE(MoveList& vec) const
{
	_get_all_moves<WHITE, NON_CAPTURE>(vec);
}

inline void MoveGenDefault::get_all_moves_BLACK_ALL(MoveList& vec) const
{
	_get_all_moves<BLACK, ALL>(vec);
}

inline void MoveGenDefault::get_all_moves_BLACK_CAPTURE(MoveList& vec) const
{
	_get_all_moves<BLACK, CAPTURE>(vec);
}

inline void MoveGenDefault::get_all_moves_BLACK_NON_CAPTURE(MoveList& vec) const
{
	_get_all_moves<BLACK, NON_CAPTURE>(vec);
}

inline void MoveGenEnglish::get_all_moves_WHITE_ALL(MoveList& vec) const
{
	_get_all_moves<WHITE, ALL>(vec);
}

inline void MoveGenEnglish::get_all_moves_WHITE_CAPTURE(MoveList& vec) const
{
	_get_all_moves<WHITE, CAPTURE>(vec);
}

inline void MoveGenEnglish::get_all_moves_WHITE_NON_CAPTURE(MoveList& vec) const
{
	_get_all_moves<WHITE, NON_CAPTURE>(vec);
}

inline void MoveGenEnglish::get_all_moves_BLACK_ALL(MoveList& vec) const
{
	_get_all_moves<BLACK, ALL>(vec);
}

inline void MoveGenEnglish::get_all_moves_BLACK_CAPTURE(MoveList& vec) const
{
	_get_all_moves<BLACK, CAPTURE>(vec);
}

inline void MoveGenEnglish::get_all_moves_BLACK_NON_CAPTURE(MoveList& vec) const
{
	_get_all_moves<BLACK, NON_CAPTURE>(vec);
}

#endif