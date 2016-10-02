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

// board.h, version 1.6

#pragma once
#ifndef _BOARD_H
#define _BOARD_H
#include <unordered_map>
#include <memory>
#include "move_gen.h"

enum game_state : int8_t { GAME_CONTINUE, DRAW, WHITE_WIN, BLACK_WIN };
enum game_rules : int8_t { RULES_DEFAULT, RULES_ENGLISH };

class Board
{
	friend class MoveGenDefault;
	friend class MoveGenEnglish;
public:
	static constexpr int DRAW_REPEATED_POS_COUNT = 3; // Count of repeated positions during the game for declaring draw
	static constexpr int DRAW_CONSECUTIVE_QUEEN_MOVES = 15; // Count of consecutive queen-only moves(from both players) for declaring draw
	// Constructor
	Board(game_rules = RULES_DEFAULT) noexcept;
	// Destructor
	virtual ~Board(void) noexcept;
	// Public member functions
	inline uint64_t get_hash(void) const noexcept;
	inline game_rules get_rules(void) const noexcept;
	inline bool get_white_turn(void) const noexcept;
	inline bool get_misere(void) const noexcept;
	inline game_state get_state(void) const noexcept;
	inline int16_t get_current_ply(void) const noexcept;
	inline int get_piece_count(piece_type) const noexcept;
	inline int get_all_piece_count(void) const noexcept;
	inline const Piece* operator[](size_t) const;
	inline Piece get_cell(int, int) const;
	inline Piece get_cell(const Position&) const;
	virtual void restart(game_rules = RULES_DEFAULT, bool = false) noexcept; // Restarts game(resets board and state)
	bool legal_move(Move&) const; // Returns whether given move is legal
	template<colour, move_type = ALL>
	inline void get_all_moves(std::vector<Move>&) const; // Outputs to given vector all possible moves
	template<colour TURN = EMPTY, move_type = ALL>
	inline std::enable_if_t<TURN == EMPTY || TURN == SHADOW,
		void> get_all_moves(std::vector<Move>&) const;
	// Stream IO functions
	static bool read_pos(std::istream&, Position&); // Reads position from given stream
	static void write_pos(std::ostream&, Position); // Outputs move to given stream
	static bool read_move(std::istream&, Move&); // Reads move from given stream
	static void write_move(std::ostream&, const Move&); // Outputs move to given stream
protected:
	inline game_state no_moves_state(void) const noexcept;
	void _update_game_state(void); // Updates current game state after a player's move
	void _proceed(Move&); // Performs updating board information when performing given move
	void _retreat(Move&); // Performs updating board information when undoing given move
	// Clear the board
	void _clear_board(void);
	// Initialization of Zobrist keys
	void init_zobrist(void) noexcept;
	// Putting and removing pieces
	virtual void _put_piece(Position, Piece);
	virtual void _remove_piece(Position);
	void _do_move(const Move&); // Performs a move using information from move_info structure
	void _undo_move(const Move&); // Performs undoing of a move using information from move_info structure
	bool white_turn; // Whether current turn is white's
	bool misere; // Whether the game is misere(winner is the loser)
	game_rules rules; // Game rules used now
	game_state state; // Whether game is end and, if yes, who won it
	Piece board[8][8]; // Board
	Position piece_list[PT_COUNT][12]; // Piece list
	int16_t cur_ply; // Current ply
	int all_piece_count; // Overall piece count
	int piece_count[PT_COUNT]; // Count of each piece
	int index[8][8]; // Index of position in piece_list[{ position's piece type }] array
	std::unique_ptr<MoveGen> move_gen;
	uint64_t cur_hash; // Hash of current position
	uint64_t zobrist_hash[PT_COUNT][64]; // Zobrist keys
	std::vector<int> consecutiveQM; // Consequtive queen moves up to given ply
	std::unordered_map<uint64_t, int> _position_count; // How many times each position occured throughout the game(for detecting draws)
};

inline uint64_t Board::get_hash(void) const noexcept
{
	return cur_hash;
}

inline game_rules Board::get_rules(void) const noexcept
{
	return rules;
}

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

inline int Board::get_piece_count(piece_type pt) const noexcept
{
	if (pt < 0 || pt >= PT_COUNT)
		return 0;
	return piece_count[pt];
}

inline int Board::get_all_piece_count(void) const noexcept
{
	return all_piece_count;
}

inline game_state Board::no_moves_state(void) const noexcept
{
	return (white_turn ^ misere) ? BLACK_WIN : WHITE_WIN;
}

template<colour TURN, move_type MT>
inline void Board::get_all_moves(std::vector<Move>& vec) const
{
	move_gen->get_all_moves<TURN, MT>(vec);
}

template<colour TURN, move_type MT>
inline std::enable_if_t<TURN == EMPTY || TURN == SHADOW,
	void> Board::get_all_moves(std::vector<Move>& vec) const
{
	if (white_turn)
		get_all_moves<WHITE, MT>(vec);
	else
		get_all_moves<BLACK, MT>(vec);
}

inline const Piece* Board::operator[](size_t idx) const
{
	return board[idx];
}

inline Piece Board::get_cell(int row, int col) const
{
	return board[row][col];
}

inline Piece Board::get_cell(const Position& pos) const
{
	return board[pos.row][pos.column];
}

#endif