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

// board.h, version 1.5

#pragma once
#ifndef _BOARD_H
#define _BOARD_H
#include <bitset>
#include <unordered_map>
#include "piece.h"
#include "position.h"
#include "move.h"

enum game_state : int8_t { GAME_CONTINUE, DRAW, WHITE_WIN, BLACK_WIN };

typedef std::bitset<sizeof(Piece) * 64 * CHAR_BIT> RawBoard;

class Board
{
public:
	static constexpr int DRAW_REPEATED_POS_COUNT = 3; // Counted of repeated positions during the game for declaring draw
	Board(void) noexcept;
	~Board(void) noexcept;
	inline bool get_white_turn(void) const noexcept;
	inline bool get_misere(void) const noexcept;
	inline game_state get_state(void) const noexcept;
	inline int16_t get_current_ply(void) const noexcept;
	inline const RawBoard& get_raw(void) const noexcept;
	inline const Piece* operator[](size_t) const;
	inline Piece get_cell(int, int) const;
	inline Piece get_cell(const Position&) const;
	inline void set_cell(int, int, Piece);
	inline void set_cell(const Position&, Piece);
	void restart(bool = false, bool = true) noexcept; // Restarts game(resets board and state)
	bool legal_move(Move&) const; // Returns whether given move is legal
	template<colour = EMPTY>
	void get_all_moves(std::vector<Move>&) const; // Outputs to given vector all possible moves
protected:
	inline game_state no_moves_state(void) const noexcept;
	void _update_game_state(void); // Updates current game state after a player's move
	void _proceed(void); // Performs updating board information when proceeding to a next ply
	void _retreat(void); // Performs updating board information when retreating to a previous ply
	// Helper function for finding all capture-moves that can be done by a piece with given coordinates
	template<colour>
	void _find_deep_capture(std::vector<Move>&, Move&, int8_t, int8_t, bool(&)[8][8]) const;
	// Same but for queen pieces
	template<colour>
	void _find_deep_capture_queen(std::vector<Move>&, Move&, int8_t, int8_t, bool(&)[8][8]) const;
	void _put_piece(const Position&, Piece);
	void _remove_piece(const Position&);
	void _do_move(const Move&); // Performs a move using information from move_info structure
	void _undo_move(const Move&); // Performs undoing of a move using information from move_info structure
	bool white_turn; // Whether current turn is white's
	bool misere; // Whether the game is misere(winner is the loser)
	game_state state; // Whether game is end and, if not, who won it
	int16_t cur_ply; // Current ply
	union
	{
		Piece board[8][8]; // Board
		RawBoard _raw_board; // For easier hashing
	};
	int piece_count[PT_COUNT];
	int index[8][8];
	Position piece_list[PT_COUNT][12];
	std::unordered_map<RawBoard, int> _position_count; // How many times each position occured throughout the game(for detecting draws)
};

inline bool Board::get_white_turn(void) const noexcept
{
	return white_turn;
}

inline bool Board::get_misere(void) const noexcept
{
	return misere;
}

inline game_state Board::get_state(void) const noexcept
{
	return state;
}

inline int16_t Board::get_current_ply(void) const noexcept
{
	return cur_ply;
}

inline const RawBoard& Board::get_raw(void) const noexcept
{
	return _raw_board;
}

inline game_state Board::no_moves_state(void) const noexcept
{
	return (white_turn ^ misere) ? BLACK_WIN : WHITE_WIN;
}

template<>
inline void Board::get_all_moves<EMPTY>(std::vector<Move>& vec) const
{
	if (white_turn)
		get_all_moves<WHITE>(vec);
	else
		get_all_moves<BLACK>(vec);
}

template<>
inline void Board::get_all_moves<SHADOW>(std::vector<Move>& vec) const
{
	get_all_moves<EMPTY>(vec);
}

inline const Piece* Board::operator[](size_t idx) const
{
	return board[idx];
}

inline Piece Board::get_cell(int row, int col) const
{
	return board[row][col];
}

inline void Board::set_cell(int row, int col, Piece piece)
{
	board[row][col] = piece;
}

inline Piece Board::get_cell(const Position& pos) const
{
	return board[pos.row][pos.column];
}

inline void Board::set_cell(const Position& pos, Piece piece)
{
	board[pos.row][pos.column] = piece;
}

#endif