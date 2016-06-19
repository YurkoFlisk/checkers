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

// checkers.h, version 1.4

#pragma once
#ifndef _CHECKERS_H
#define _CHECKERS_H
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <bitset>
#include <stack>
#include <unordered_map>
#include <algorithm>
#include "misc.h"
#include "move.h"

enum game_state : std::int8_t { GAME_CONTINUE = 0, DRAW, WHITE_WIN, BLACK_WIN };
enum step_result : std::int8_t { STEP_ILLEGAL, STEP_ILLEGAL_NEW, STEP_PROCEED, STEP_FINISH };

typedef Piece Board[8][8];
typedef std::bitset<sizeof(Board) * CHAR_BIT> RawBoard;

class Checkers
{
public:
	static constexpr int DRAW_REPEATED_POS_COUNT = 3; // Counted of repeated positions during the game for declaring draw
	static constexpr int MAX_SCORE = 2000000000; // Max score(absolute value), which indicates special situations(win/loss, initial value etc)
	static constexpr int MAX_KILLERS = 3; // Maximum numbers of killers for killer heuristic(AI)
	static constexpr int NORMAL_WEIGHT = 100; // Weight of non-queen piece in score function
	static constexpr int QUEEN_WEIGHT = 300; // Weight of queen piece in score function
	static const int NORMAL_COLUMN_WEIGHT[8]; // Weight of column for pieces in score functions
	static const int NORMAL_ROW_WEIGHT_WHITE[8]; // Weight of row for white non-queen piece in score function
	static const int NORMAL_ROW_WEIGHT_BLACK[8]; // Weight of row for black non-queen piece in score function
#if defined _DEBUG || defined DEBUG
	static constexpr int MAX_SEARCH_DEPTH = 8; // Maximum search depth of AI
#else
	static constexpr int MAX_SEARCH_DEPTH = 12; // Maximum search depth of AI
#endif
	static const int MAX_TRANSTABLE_MAKE_DEPTH[MAX_SEARCH_DEPTH + 1]; // Maximum depth where transposition table will be made
	static const int MIN_TRANSTABLE_USE_DEPTH[MAX_SEARCH_DEPTH + 1]; // Minimum depth where transposition table can be used
	static const int MAX_HISTORY_MAKE_DEPTH[MAX_SEARCH_DEPTH + 1]; // Maximum depth where history values will be stored
	static const int HISTORY_VALUE[MAX_SEARCH_DEPTH + 1]; // Value added for move which was searched with depth, equal to index
	// Constructor
	Checkers(bool = false, bool = true) noexcept;
	// Destructor
	~Checkers(void) noexcept;
	// Public member functions
	inline bool get_misere(void) const noexcept;
	inline bool get_white_turn(void) const noexcept;
	inline int get_search_depth(void) const noexcept;
	inline game_state get_state(void) const noexcept;
	inline const Move& get_part_move(void) const noexcept;
	inline size_t get_part_move_size(void) const noexcept;
	inline const std::vector<Move>& get_part_possible_moves(void) const;
	inline const Move& get_last_move(void) const;
	inline colour current_turn_colour(void) const noexcept;
	inline const Piece* operator[](size_t) const;
	inline void set_search_depth(int) noexcept;
	template<colour = EMPTY>
	void get_all_moves(std::vector<Move>&) const; // Outputs to given vector all possible moves
	bool legal_move(Move&) const; // Returns whether given move is legal
	bool move(Move&); // Function for inputing player's move
	// Function for inputing player's move step-by-step. Returns
	// STEP_ILLEGAL and discards information about move if the move is illegal,
	// STEP_ILLEGAL_NEW if the move is illegal, but given position is a start for new(valid) move
	// STEP_PROCEED if given step is valid(there is legal move beginning with inputted step sequence),
	// STEP_FINISH if given step is valid and the whole move is finished
	step_result step(const Position&);
	// AI. Outputs computer's move to a first parameter and position score to a second.
	// Returns false if computer doesn't have move(lost) and true otherwise. Uses minimax algorithm with alpha-beta pruning
	bool get_computer_move(Move&, int&);
	inline bool get_computer_move(Move&);
	void part_undo(void); // Undoing of current unfinished part move(inputted with step function)
	void undo_move(void); // Undo last move
	void redo_move(void); // Redo last undone move
	void perform_computer_move(void); // AI. Performs computer move
	void restart(bool = false, bool = true) noexcept; // Restarts game(resets board and state)
	static bool read_move(std::istream&, Move&); // Reads move from given string
	static void write_move(std::ostream&, const Move&);
	void load_game(std::istream&); // Loads a game from given stream in text format
	void save_game(std::ostream&) const; // Outputs current game to given stream in text format
	int score(void) const noexcept; // Returns score of the current game position(FOR WHITE AS MAXIMIZER)
	int evaluate(int, int, int); // Returns score of the current game position using quiescence search(FOR CURRENT TURN AS MAXIMIZER)
private:
	// Private member functions
	inline int no_moves_score(int) const;
	inline game_state no_moves_state(void) const;
	inline Piece& between(const Position&, const Position&);
	inline const Piece& between(const Position&, const Position&) const;
	inline bool _legal_position(const Position&) const noexcept;
	inline int _history_move_score(const Move&) const;
	inline void _update_possible_moves(void);
	void _undo_move(const Move&); // Performs undoing of a move using information from move_info structure
	void _do_move(const Move&); // Performs a move using information from move_info structure
	void _update_game_state(void); // Updates current game state after a player's move
	// Helper function for finding all capture-moves that can be done by a piece with given coordinates
	template<colour>
	void _find_deep_capture(std::vector<Move>&, Move&, int, int, bool(&)[8][8]) const;
	// Same but for queen pieces
	template<colour>
	void _find_deep_capture_queen(std::vector<Move>&, Move&, int, int, bool(&)[8][8]) const;
	void _white_computer_move(size_t, int, int); // Helper function for get_computer_move
	void _black_computer_move(size_t, int, int); // Helper function for get_computer_move
	// Members
	union
	{
		Board board; // Game board
		RawBoard _raw_board; // For easier hashing
	};
	bool white_turn; // Whether current turn is white's
	bool misere; // Whether the game is misere(winner is the loser)
	game_state state; // Whether game is end and, if not, who won it
	std::vector<Move> undos; // Stack for information about undoing moves
	std::stack<Move> redos; // Stack for information about redoing undone moves
	int search_depth; // Depth of minimax search
	int _last_ai_use_ply; // Internal member for get_computer_move function(last ply at which ai was used)
	int _score; // Internal member for get_computer_move function
	Move _cur_move; // Internal member for step function
	std::vector<Move> _cur_possible_moves; // Internal member for step function
	std::unordered_map<RawBoard, int> _position_count; // How many times each position occured throughout the game(for detecting draws)
	std::unordered_map<RawBoard, int> _transtable_white; // Scores for some of already computed positions where it's white's turn
	std::unordered_map<RawBoard, int> _transtable_black; // Scores for some of already computed positions where it's black's turn
	std::list<Move> killers[MAX_SEARCH_DEPTH]; // Killers for killer heuristic in AI
	int history[64][64]; // History table for history heuristic in AI
};

inline bool Checkers::get_misere(void) const noexcept
{
	return misere;
}

inline bool Checkers::get_white_turn(void) const noexcept
{
	return white_turn;
}

inline int Checkers::get_search_depth(void) const noexcept
{
	return search_depth;
}

inline game_state Checkers::get_state(void) const noexcept
{
	return state;
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
inline void Checkers::set_search_depth(int depth) noexcept
{
	if ((search_depth = std::max(depth, 1)) > MAX_SEARCH_DEPTH)
		search_depth = MAX_SEARCH_DEPTH;
}

template<>
inline void Checkers::get_all_moves<EMPTY>(std::vector<Move>& vec) const
{
	if (white_turn)
		get_all_moves<WHITE>(vec);
	else
		get_all_moves<BLACK>(vec);
}

template<>
inline void Checkers::get_all_moves<SHADOW>(std::vector<Move>& vec) const
{
	get_all_moves<EMPTY>(vec);
}

// Updates currently possible moves
inline void Checkers::_update_possible_moves(void)
{
	_cur_possible_moves.clear();
	get_all_moves(_cur_possible_moves);
}

inline game_state Checkers::no_moves_state(void) const
{
	return (white_turn ^ misere) ? BLACK_WIN : WHITE_WIN;
}

// Score for maximizer, if it does not have move
inline int Checkers::no_moves_score(int depth) const
{
	return misere ? MAX_SCORE - depth : -MAX_SCORE + depth;
}

inline Piece& Checkers::between(const Position& p1, const Position& p2)
{
	return board[(p1.get_row() + p2.get_row()) >> 1][(p1.get_column() + p2.get_column()) >> 1];
}

inline const Piece& Checkers::between(const Position& p1, const Position& p2) const
{
	return board[(p1.get_row() + p2.get_row()) >> 1][(p1.get_column() + p2.get_column()) >> 1];
}

inline int Checkers::_history_move_score(const Move& m) const
{
	return history[(m.old_pos().get_row() << 3) + m.old_pos().get_column()]
		[(m.new_pos().get_row() << 3) + m.new_pos().get_column()];
}

// Returns whether given position is valid
inline bool Checkers::_legal_position(const Position& pos) const noexcept
{
	return pos.get_row() > 0 && pos.get_column() > 0 && pos.get_row() < 8 && pos.get_column() < 8 &&
		(pos.get_row() & 1) == (pos.get_column() & 1);
}

inline bool Checkers::get_computer_move(Move& out)
{
	int tmp;
	return get_computer_move(out, tmp);
}

#endif