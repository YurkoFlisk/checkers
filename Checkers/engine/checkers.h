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

// checkers.h, version 1.5

#pragma once
#ifndef _CHECKERS_H
#define _CHECKERS_H
#include <iostream>
#include <vector>
#include <list>
#include <stack>
#include <algorithm>
#include "misc.h"
#include "board.h"

enum step_result : int8_t { STEP_ILLEGAL, STEP_ILLEGAL_NEW, STEP_PROCEED, STEP_FINISH };
enum tt_bound : int8_t { TTBOUND_EXACT, TTBOUND_LOWER, TTBOUND_UPPER };

struct TT_Entry
{
	int16_t value;
	int8_t depth;
	tt_bound bound_type;
	Position best_move_from;
	Position best_move_to;
};

typedef std::unordered_map<uint64_t, TT_Entry> TranspositionTable;

class Checkers
	: public Board
{
public:
	static constexpr int16_t MAX_SCORE = 30000; // Max score(absolute value), which indicates special situations(win/loss, initial value etc)
	static constexpr int16_t MAX_LOSE_SCORE = -MAX_SCORE + 1000; // Maximum score for loosing player
	static constexpr int16_t MIN_WIN_SCORE = MAX_SCORE - 1000; // Minimum score for winning player
	static constexpr int MAX_KILLERS = 3; // Maximum numbers of killers for killer heuristic(AI)
	static constexpr int16_t NORMAL_WEIGHT = 100; // Weight of non-queen piece in score function
	static constexpr int16_t QUEEN_WEIGHT = 300; // Weight of queen piece in score function
	static constexpr int16_t DELTA_PRUNING_MARGIN = 500; // Safety margin for delta pruning in quiscence search
	static constexpr int16_t LT_PRUNING_MARGIN = 300; // Safety margin for LT pruning
	static constexpr int16_t STAND_PAT_MARGIN = 300; // Safety margin for stand-pat pruning
#if defined _DEBUG || defined DEBUG
	static constexpr int8_t MAX_SEARCH_DEPTH = 8; // Maximum search depth of AI
#else
	static constexpr int8_t MAX_SEARCH_DEPTH = 12; // Maximum search depth of AI
#endif
	static const int8_t MIN_HISTORY_MAKE_DEPTH[MAX_SEARCH_DEPTH + 1]; // Maximum depth where history values will be stored
	static const int HISTORY_VALUE[MAX_SEARCH_DEPTH + 1]; // Value added for move which was searched with depth, equal to index
	// Constructor
	Checkers(bool = false, bool = true) noexcept;
	// Destructor
	~Checkers(void) noexcept;
	// Public member functions
	inline int8_t get_search_depth(void) const noexcept;
	inline const Move& get_part_move(void) const noexcept;
	inline size_t get_part_move_size(void) const noexcept;
	inline const std::vector<Move>& get_part_possible_moves(void) const;
	inline const Move& get_last_move(void) const;
	inline colour current_turn_colour(void) const noexcept;
	inline const Piece* operator[](size_t) const;
	inline void set_search_depth(int8_t) noexcept;
	bool move(Move&); // Function for inputing player's move
	// Function for inputing player's move step-by-step. Returns
	// STEP_ILLEGAL and discards information about move if the move is illegal,
	// STEP_ILLEGAL_NEW if the move is illegal, but given position is a start for new(valid) move
	// STEP_PROCEED if given step is valid(there is legal move beginning with inputted step sequence),
	// STEP_FINISH if given step is valid and the whole move is finished
	step_result step(const Position&);
	// AI. Outputs computer's move to a first parameter and position score to a second.
	// Returns false if computer doesn't have move(lost) and true otherwise. Uses minimax algorithm with alpha-beta pruning
	inline bool get_computer_move(Move&, int&);
	inline bool get_computer_move(Move&);
	void part_undo(void); // Undoing of current unfinished part move(inputted with step function)
	void undo_move(void); // Undo last move
	void redo_move(void); // Redo last undone move
	void perform_computer_move(void); // AI. Performs computer move
	void restart(bool = false, bool = true) noexcept override; // Restarts game(resets board and state)
	void load_board(std::istream&);
	void save_board(std::ostream&) const;
	void load_game(std::istream&); // Loads a game from given stream in text format
	void save_game(std::ostream&) const; // Outputs current game to given stream in text format
	// Returns score of the current game position(FOR WHITE AS MAXIMIZER)
	inline int16_t score(void) const noexcept;
	// Returns score of the current game position using quiescence search(FOR CURRENT TURN AS MAXIMIZER)
	template<colour>
	int16_t evaluate(int16_t, int16_t);
protected:
	// This function is only for using in step function(and company). It only sets specified cell, without updating other stuff
	inline void _set_cell(int, int, Piece);
	inline void _set_cell(Position, Piece);
	// Initialization of piece-square tables
	void init_psq(void);
	// Overridden Board functions
	inline void _put_piece(Position, Piece) override;
	inline void _remove_piece(Position) override;
	// Functions for writing and reading values from transposition tables(needed for ply-adjustment of mate scores)
	static inline int16_t value_from_tt(const TT_Entry&, int16_t);
	static inline int16_t value_to_tt(int16_t, int16_t);
	// Misc
	static inline bool _legal_position(Position) noexcept;
	static inline int16_t lose_score(int16_t) noexcept;
	static inline int16_t win_score(int16_t) noexcept;
	inline int16_t no_moves_score(int16_t) const noexcept;
	inline int _history_move_score(const Move&) const;
	inline void _update_possible_moves(void);
	// Internal logic of AI(principal variation search)
	template<colour>
	int16_t _pvs(int8_t, int16_t, int16_t);
	// Internal logic of AI
	template<colour>
	bool get_computer_move(Move&, int&);
	// Members
	int8_t search_depth; // Depth of minimax search
	int16_t _score; // Internal member for get_computer_move function(for storing results of recursive calls)
	int16_t inc_score; // Position score that is evaluated incrementally(for white as maximizer)
	std::vector<Move> undos; // Stack for information about undoing moves
	std::stack<Move> redos; // Stack for information about redoing undone moves
	Move _cur_move; // Internal member for step function
	std::vector<Move> _cur_possible_moves; // Internal member for step function
	TranspositionTable _transtable[2]; // Scores for some of already computed positions where 0 is white's turn and 1 is black's
	std::vector<std::list<Move> > killers; // Killers for killer heuristic in AI(indexed by ply)
	int history[64][64]; // History table for history heuristic in AI
};

inline int8_t Checkers::get_search_depth(void) const noexcept
{
	return search_depth;
}

inline const Move& Checkers::get_part_move(void) const noexcept
{
	return _cur_move;
}

inline size_t Checkers::get_part_move_size(void) const noexcept
{
	return _cur_move.get_path().size();
}

// Get all possible moves which begin with current inputed(by step function) part(if it's empty now, all possible moves are returned)
inline const std::vector<Move>& Checkers::get_part_possible_moves(void) const
{
	return _cur_possible_moves;
}

inline const Move& Checkers::get_last_move(void) const
{
	return undos.back();
}

inline colour Checkers::current_turn_colour(void) const noexcept
{
	return white_turn ? WHITE : BLACK;
}

inline const Piece* Checkers::operator[](size_t idx) const
{
	return board[idx];
}

// Set search depth. Should be [1; MAX_SEARCH_DEPTH]
inline void Checkers::set_search_depth(int8_t depth) noexcept
{
	if ((search_depth = std::max<int8_t>(depth, 1)) > MAX_SEARCH_DEPTH)
		search_depth = MAX_SEARCH_DEPTH;
}

// Updates currently possible moves
inline void Checkers::_update_possible_moves(void)
{
	_cur_possible_moves.clear();
	get_all_moves(_cur_possible_moves);
}

inline void Checkers::_set_cell(int row, int col, Piece piece)
{
	board[row][col] = piece;
}

inline void Checkers::_set_cell(Position pos, Piece piece)
{
	board[pos.get_row()][pos.get_column()] = piece;
}

inline int16_t Checkers::value_from_tt(const TT_Entry& entry, int16_t ply)
{
	return
		entry.value < MAX_LOSE_SCORE ? entry.value + ply :
		entry.value > MIN_WIN_SCORE ? entry.value - ply :
		entry.value;
}

inline int16_t Checkers::value_to_tt(int16_t value, int16_t ply)
{
	return
		value < MAX_LOSE_SCORE ? value - ply :
		value > MIN_WIN_SCORE ? value + ply :
		value;
}

// Score for maximizer, if it loses(parameter is current ply)
inline int16_t Checkers::lose_score(int16_t ply) noexcept
{
	return -MAX_SCORE + ply;
}

// Score for maximizer, if it is winning(parameter is current ply)
inline int16_t Checkers::win_score(int16_t ply) noexcept
{
	return MAX_SCORE - ply;
}

inline int16_t Checkers::no_moves_score(int16_t ply) const noexcept
{
	return get_misere() ? win_score(ply) : lose_score(ply);
}

inline int Checkers::_history_move_score(const Move& m) const
{
	return history[pos_idx(m.old_pos())][pos_idx(m.new_pos())];
}

// Returns whether given position is valid
inline bool Checkers::_legal_position(Position pos) noexcept
{
	return pos.get_row() > 0 && pos.get_column() > 0 && pos.get_row() < 8 && pos.get_column() < 8 &&
		(pos.get_row() & 1) == (pos.get_column() & 1);
}

inline bool Checkers::get_computer_move(Move& m, int& sc)
{
	if (white_turn)
		return get_computer_move<WHITE>(m, sc);
	else
		return get_computer_move<BLACK>(m, sc);
}

inline bool Checkers::get_computer_move(Move& out)
{
	int tmp;
	return get_computer_move(out, tmp);
}

#endif