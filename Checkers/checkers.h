// Checkers
// By Yurko Prokopets(aka YurkoFlisk)
// checkers.h
// Version 1.3

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
#include "constants.h"
#include "misc.h"

enum colour : std::int8_t {EMPTY = 0, SHADOW, WHITE, BLACK};
enum game_state : std::int8_t {GAME_CONTINUE = 0, DRAW, WHITE_WIN, BLACK_WIN};

inline colour opposite(colour c)
{return c == WHITE ? BLACK : WHITE;}

struct Piece
{
	Piece(void) noexcept;
	Piece(colour, bool = false) noexcept;
	bool operator==(const Piece&) const noexcept;
	bool operator!=(const Piece&) const noexcept;
	bool queen;
	colour color;
};

struct position
{
	position(void) noexcept;
	position(int, int) noexcept;
	bool operator==(const position&) const noexcept;
	bool operator!=(const position&) const noexcept;
	int row;
	int column;
};

struct move_info
{
	position oldpos;
	position newpos;
	Piece original;
	Piece become;
	std::vector<std::pair<position, Piece>> eaten;
};

typedef std::vector<position> Move;
typedef Piece Board[8][8];
typedef std::bitset<sizeof(Board) * CHAR_BIT> RawBoard;

class Checkers
{
public:
	static CONSTEXPR int DRAW_REPEATED_POS_COUNT = 3; // Counted of repeated positions during the game for declaring draw
	static CONSTEXPR int MAX_SCORE = 2000000000; // Max score(absolute value), which indicates special situations(win/loss, initial value etc)
	static CONSTEXPR int MAX_KILLERS = 4; // Maximum numbers of killers for killer heuristic(AI)
	static CONSTEXPR int NORMAL_WEIGHT = 100; // Weight of non-queen piece in score function
	static CONSTEXPR int QUEEN_WEIGHT = 300; // Weight of queen piece in score function
	static const int NORMAL_ROW_WEIGHT_WHITE[8]; // Weight of row for white non-queen piece in score function
	static const int NORMAL_ROW_WEIGHT_BLACK[8]; // Weight of row for black non-queen piece in score function
#if defined _DEBUG || defined DEBUG
	static CONSTEXPR int MAX_SEARCH_DEPTH = 8; // Maximum search depth of AI
#else
	static CONSTEXPR int MAX_SEARCH_DEPTH = 12; // Maximum search depth of AI
#endif
	static const int MAX_TRANSTABLE_MAKE_DEPTH[MAX_SEARCH_DEPTH + 1]; // Maximum depth where transposition table will be made
	static const int MIN_TRANSTABLE_USE_DEPTH[MAX_SEARCH_DEPTH + 1]; // Minimum depth where transposition table can be used
	// Constructor
	Checkers(bool = true) noexcept;
	// Destructor
	~Checkers(void) noexcept;
	// Public member functions
	inline bool get_white_turn(void) const noexcept
	{return white_turn;}
	inline int get_search_depth(void) const noexcept
	{return search_depth;}
	inline game_state get_state(void) const noexcept
	{return state;}
	inline colour current_turn_color(void) const noexcept
	{return white_turn ? WHITE : BLACK;}
	inline const Piece* operator[](size_t idx) const noexcept
	{return board[idx];}
	void set_search_depth(int) noexcept; // Set search depth. Should be [1; MAX_SEARCH_DEPTH]
	bool move(const Move&, move_info& = move_info()); // Function for inputing player's move
	bool part_move(const position&); // Function for inputing player's move step-by-step. Returns true if the move is completed
	// AI. Outputs computer's move to a first parameter.
	// Returns false if computer doesn't have move(lost) and true otherwise. Uses minimax algorithm with alpha-beta pruning
	bool get_computer_move(Move&);
	void part_undo(void); // Undoing of current unfinished part move
	void undo_move(void); // Undo last move
	void redo_move(void); // Redo last undone move
	void perform_computer_move(void); // AI. Performs computer move
	void restart(bool = true) noexcept; // Restarts game(resets board and state)
	static bool read_move(std::istream&, Move&) throw(checkers_error); // Reads move from given string
	static void write_move(std::ostream&, const Move&);
	void load_game(std::istream&) throw(checkers_error); // Loads a game from given stream in text format
	void save_game(std::ostream&) const; // Outputs current game to given stream in text format
	int score(void) const noexcept; // Returns signed score of the current game position
private:
	// Private member functions
	inline Piece& between(const position& p1, const position& p2)
	{return board[(p1.row + p2.row) >> 1][(p1.column + p2.column) >> 1];} // Helper function
	inline const Piece& between(const position& p1, const position& p2) const
	{return board[(p1.row + p2.row) >> 1][(p1.column + p2.column) >> 1];} // Helper function
	inline bool _legal_position(const position& pos) const noexcept
	{return pos.row > 0 && pos.column > 0 && pos.row < 8 && pos.column < 8 &&
		(pos.row & 1) == (pos.column & 1);} // Returns whether given position is valid
	bool _legal_move(const Move&) const; // Returns whether given move is legal
	void _perform_move(const Move&, move_info& = move_info()); // Performs legal move. Checks are done outside this function
	void _undo_move(const move_info&); // Performs undoing of a move using information from move_info structure
	void _redo_move(const move_info&); // Performs redoing of a move using information from move_info structure
	void _update_game_state(void); // Updates current game state after a player's move
	void _get_all_moves(std::vector<Move>&) const; // Outputs to given vector all possible moves
	void _find_deep_capture(std::vector<Move>&, Move&, int, int, bool(&)[8][8]) const; // Helper function for finding all capture-moves
	// that can be done by a piece with given coordinates
	void _find_deep_capture_queen(std::vector<Move>&, Move&, int, int, bool(&)[8][8]) const; // Same but for queen pieces
	void _white_computer_move(size_t, int, int); // Helper function for get_computer_move
	void _black_computer_move(size_t, int, int); // Helper function for get_computer_move
	// Members
	union
	{
		Board board; // Game board
		RawBoard _raw_board; // For easier hashing
	};
	bool white_turn; // Whether current turn is white's
	game_state state; // Whether game is end and, if not, who won it
	std::vector<std::pair<Move, move_info> > undos; // Stack for information about undoing moves
	std::stack<std::pair<Move, move_info> > redos; // Stack for information about redoing undone moves
	int search_depth; // Depth of minimax search
	int _last_ai_use_ply; // Internal member for get_computer_move function(last ply at which ai was used)
	int _score; // Internal member for get_computer_move function
	move_info _cur_move_info; // Internal member for part_move function
	Move _cur_move; // Internal member for part_move function
	std::vector<Move> _cur_possible_moves; // Internal member for part_move function
	std::unordered_map<RawBoard, int> _position_count; // How many times each position occured throughout the game(for detecting draws)
	std::unordered_map<RawBoard, int> _transtable_white; // Scores for some of already computed positions where it's white's turn
	std::unordered_map<RawBoard, int> _transtable_black; // Scores for some of already computed positions where it's black's turn
	std::list<Move> killers[MAX_SEARCH_DEPTH]; // Killers for killer heuristic in AI
};

#endif
