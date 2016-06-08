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

// checkers.cpp, version 1.3

#include "checkers.h"

const int Checkers::NORMAL_ROW_WEIGHT_WHITE[8] = {
	0, 3, 5, 10, 20, 25, 30, -1
}; // -1 - unused
const int Checkers::NORMAL_ROW_WEIGHT_BLACK[8] = {
	-1, 30, 25, 20, 10, 5, 3, 0
}; // -1 - unused
const int Checkers::MAX_TRANSTABLE_MAKE_DEPTH[Checkers::MAX_SEARCH_DEPTH + 1] = {
	-1, 0, 1, 2, 2, 2, 3, 3, 4
#if !defined _DEBUG && !defined DEBUG
	, 5, 6, 6, 7
#endif
}; // -1 - unused
const int Checkers::MIN_TRANSTABLE_USE_DEPTH[Checkers::MAX_SEARCH_DEPTH + 1] = {
	-1, 0, 1, 1, 2, 2, 2, 3, 3
#if !defined _DEBUG && !defined DEBUG
	, 4, 5, 6, 6
#endif
}; // -1 - unused

Piece::Piece(void) noexcept
	: queen(false), color(EMPTY)
{}

Piece::Piece(colour c, bool q) noexcept
	: queen(q), color(c)
{}

bool Piece::operator==(const Piece& rhs) const noexcept
{
	return color == rhs.color && queen == rhs.queen;
}

bool Piece::operator!=(const Piece& rhs) const noexcept
{
	return color != rhs.color || queen != rhs.queen;
}

position::position(void) noexcept
	: row(0), column(0)
{}

position::position(int r, int c) noexcept
	: row(r), column(c)
{}

bool position::operator==(const position& rhs) const noexcept
{
	return row == rhs.row && column == rhs.column;
}

bool position::operator!=(const position& rhs) const noexcept
{
	return row != rhs.row || column != rhs.column;
}

Checkers::Checkers(bool white) noexcept
	: search_depth(MAX_SEARCH_DEPTH)
{
	restart(white);
}

Checkers::~Checkers(void) noexcept
{}

void Checkers::restart(bool white) noexcept
{
	board[3][1] = board[3][3] = board[3][5] = board[3][7] =
		board[4][0] = board[4][2] = board[4][4] = board[4][6] = Piece(EMPTY);
	board[0][0] = board[0][2] = board[0][4] = board[0][6] =
		board[1][1] = board[1][3] = board[1][5] = board[1][7] =
		board[2][0] = board[2][2] = board[2][4] = board[2][6] = Piece(WHITE);
	board[5][1] = board[5][3] = board[5][5] = board[5][7] =
		board[6][0] = board[6][2] = board[6][4] = board[6][6] =
		board[7][1] = board[7][3] = board[7][5] = board[7][7] = Piece(BLACK);
	white_turn = white;
	state = GAME_CONTINUE;
	_last_ai_use_ply = 0;
	_cur_possible_moves.clear();
	_cur_move.clear();
	_position_count.clear();
	_transtable_white.clear();
	_transtable_black.clear();
	undos.swap(decltype(undos)());
	redos.swap(decltype(redos)());
}

int Checkers::score(void) const noexcept
{
	int sc(0);
	for (size_t row = 0; row < 8; ++row)
		for (size_t column = row & 1; column < 8; column += 2)
			if (board[row][column].color == WHITE)
				sc += (board[row][column].queen ? QUEEN_WEIGHT : (NORMAL_WEIGHT + NORMAL_ROW_WEIGHT_WHITE[row]));
			else if (board[row][column].color == BLACK)
				sc -= (board[row][column].queen ? QUEEN_WEIGHT : (NORMAL_WEIGHT + NORMAL_ROW_WEIGHT_BLACK[row]));
	return sc;
}

void Checkers::_perform_move(const Move& move, move_info& undo)
{
	undo.oldpos = move[0], undo.newpos = move.back();
	undo.original = board[move[0].row][move[0].column];
	board[move[0].row][move[0].column] = Piece();
	bool queen = undo.original.queen;
	for (size_t i = 1; i < move.size(); ++i)
	{
		if (queen == false) // If piece is simple
		{
			if (abs(move[i].row - move[i - 1].row) == 2) // If capture-move
			{
				undo.eaten.push_back(std::make_pair(position((move[i - 1].row + move[i].row) >> 1,
					(move[i - 1].column + move[i].column) >> 1), between(move[i - 1], move[i])));
				between(move[i - 1], move[i]) = Piece();
			}
		}
		else // If piece is queen
		{
			const int d_row = (move[i].row < move[i - 1].row ? -1 : 1), d_column = (move[i].column < move[i - 1].column ? -1 : 1);
			for (int row = move[i - 1].row + d_row, column = move[i - 1].column + d_column; row != move[i].row; row += d_row, column += d_column)
				if (board[row][column].color != EMPTY)
				{
					undo.eaten.push_back(std::make_pair(position(row, column), board[row][column]));
					board[row][column] = Piece();
					break;
				}
		}
		if (move[i].row == (undo.original.color == WHITE ? 7 : 0)) // If reaches last row, piece become queen
			queen = true;
	}
	board[move.back().row][move.back().column].color = undo.original.color;
	board[move.back().row][move.back().column].queen = queen;
	undo.become = board[move.back().row][move.back().column];
}

void Checkers::_undo_move(const move_info& undo)
{
	board[undo.newpos.row][undo.newpos.column] = Piece();
	board[undo.oldpos.row][undo.oldpos.column] = undo.original;
	for (size_t i = 0; i < undo.eaten.size(); ++i)
		board[undo.eaten[i].first.row][undo.eaten[i].first.column] = undo.eaten[i].second;
}

void Checkers::_redo_move(const move_info& redo)
{
	for (size_t i = 0; i < redo.eaten.size(); ++i)
		board[redo.eaten[i].first.row][redo.eaten[i].first.column] = Piece();
	board[redo.oldpos.row][redo.oldpos.column] = Piece();
	board[redo.newpos.row][redo.newpos.column] = redo.become;
}

bool Checkers::_legal_move(const Move& move) const
{
	std::vector<Move> moves;
	_get_all_moves(moves);
	return std::find(moves.begin(), moves.end(), move) != moves.end();
}

void Checkers::set_search_depth(int depth) noexcept
{
	if ((search_depth = std::max(depth, 1)) > MAX_SEARCH_DEPTH)
		search_depth = MAX_SEARCH_DEPTH;
}

bool Checkers::move(const Move& m, move_info& info)
{
	if (state != GAME_CONTINUE || !_legal_move(m))
		return false;
	_perform_move(m, info);
	undos.push_back(make_pair(m, info));
	redos.swap(decltype(redos)());
	++_position_count[_raw_board];
	white_turn = !white_turn; // Change turn of move
	_update_game_state();
	return true;
}

void Checkers::_update_game_state(void)
{
	std::vector<Move> vec;
	_get_all_moves(vec);
	if (vec.size() == 0)
		state = white_turn ? BLACK_WIN : WHITE_WIN;
	else
	{
		bool draw = false;
		for (const auto& pos : _position_count)
			if (pos.second >= DRAW_REPEATED_POS_COUNT)
			{
				draw = true;
				break;
			}
		state = (draw ? DRAW : GAME_CONTINUE);
	}
}

bool Checkers::part_move(const position& pos)
{
	if (state != GAME_CONTINUE)
		return false;
	if (_cur_move.empty())
		_get_all_moves(_cur_possible_moves);
	_cur_possible_moves.erase(
		std::remove_if(_cur_possible_moves.begin(), _cur_possible_moves.end(), [&](const Move& m) {
		return m.size() <= _cur_move.size() || m[_cur_move.size()] != pos;
	}), _cur_possible_moves.end());
	if (_cur_possible_moves.empty())
	{
		if (_cur_move.size() > 0)
		{
			part_undo();
			if (board[pos.row][pos.column].color == (white_turn ? WHITE : BLACK))
				part_move(pos);
		}
		else
			_cur_move_info = move_info();
		return false;
	}
	_cur_move.push_back(pos);
	if (_cur_move.size() > 1)
	{
		move_info _cur_part;
		_perform_move({ _cur_move[_cur_move.size() - 2], _cur_move.back() }, _cur_part);
		_cur_move_info.eaten.insert(_cur_move_info.eaten.end(), _cur_part.eaten.begin(), _cur_part.eaten.end());
		for (const auto& destroyed : _cur_part.eaten)
			board[destroyed.first.row][destroyed.first.column] = Piece(SHADOW);
	}
	else
		_cur_move_info.oldpos = _cur_move[0], _cur_move_info.original = board[_cur_move[0].row][_cur_move[0].column];
	_cur_move_info.become = board[pos.row][pos.column];
	if (_cur_possible_moves.size() == 1 && _cur_possible_moves[0].size() == _cur_move.size())
	{
		_cur_move_info.newpos = _cur_move.back();
		for (const auto& destroyed : _cur_move_info.eaten)
			board[destroyed.first.row][destroyed.first.column] = Piece(EMPTY);
		undos.push_back(make_pair(_cur_move, _cur_move_info));
		redos.swap(decltype(redos)());
		_cur_possible_moves.clear();
		_cur_move.clear(), _cur_move_info = move_info();
		++_position_count[_raw_board];
		white_turn = !white_turn;
		_update_game_state();
		return true;
	}
	return false;
}

void Checkers::part_undo(void)
{
	if (_cur_move.size() > 0)
	{
		_cur_move_info.newpos = _cur_move.back();
		_undo_move(_cur_move_info);
		_cur_move.clear(), _cur_move_info = move_info();
		_cur_possible_moves.clear();
	}
}

void Checkers::undo_move(void)
{
	part_undo();
	if (undos.empty())
		return;
	--_position_count[_raw_board];
	_undo_move(undos.back().second);
	redos.push(undos.back());
	undos.pop_back();
	white_turn = !white_turn;
	_update_game_state();
}

void Checkers::redo_move(void)
{
	part_undo();
	if (redos.empty())
		return;
	_redo_move(redos.top().second);
	undos.push_back(redos.top());
	redos.pop();
	++_position_count[_raw_board];
	white_turn = !white_turn;
	_update_game_state();
}

void Checkers::_get_all_moves(std::vector<Move>& vec) const
{
	// Search for capture-moves first
	for (size_t row = 0; row < 8; ++row)
		for (size_t column = row & 1; column < 8; column += 2)
			if (board[row][column].color == current_turn_color())
			{
				Move move(1, position(row, column));
				bool capture[8][8] = {}, queen = board[row][column].queen;
				tmp_assign<Piece> move_begin(const_cast<Piece&>
					(board[row][column]), Piece()); // Because this position is empty when we move from it
				if (queen)
					_find_deep_capture_queen(vec, move, row, column, capture);
				else
					_find_deep_capture(vec, move, row, column, capture);
			}
	// Capture-move is mandatory, so we need to check non-capture moves only when we don't have any capture-moves
	if (vec.empty())
		for (size_t row = 0; row < 8; ++row)
			for (size_t column = row & 1; column < 8; column += 2)
				if (board[row][column].color == current_turn_color())
				{
					if (board[row][column].queen)
					{
						const int dx[4] = { 1, 1, -1, -1 }, dy[4] = { 1, -1, 1, -1 };
						for (size_t dir = 0; dir < 4; ++dir)
							for (int r = row + dy[dir], c = column + dx[dir]; r < 8 && c < 8 && r >= 0 && c >= 0
								&& board[r][c].color == EMPTY; r += dy[dir], c += dx[dir])
							{
								Move move(1, position(row, column));
								move.push_back(position(r, c));
								vec.push_back(move);
							}
					}
					else
					{
						if (column < 7 && board[white_turn ? row + 1 : row - 1][column + 1].color == EMPTY)
						{
							Move move(1, position(row, column));
							move.push_back(position(white_turn ? row + 1 : row - 1, column + 1));
							vec.push_back(move);
						}
						if (column > 0 && board[white_turn ? row + 1 : row - 1][column - 1].color == EMPTY)
						{
							Move move(1, position(row, column));
							move.push_back(position(white_turn ? row + 1 : row - 1, column - 1));
							vec.push_back(move);
						}
					}
				}
}

void Checkers::_find_deep_capture(std::vector<Move>& vec, Move& move, int row, int column, bool(&captured)[8][8]) const
{
	static CONSTEXPR int d_row[4] = { 1, 1, -1, -1 }, d_column[4] = { 1, -1, 1, -1 };
	for (size_t dir = 0; dir < 4; ++dir)
	{
		const int r1 = row + d_row[dir], c1 = column + d_column[dir], r2 = r1 + d_row[dir], c2 = c1 + d_column[dir];
		if (r2 < 0 || c2 < 0 || r2 > 7 || c2 > 7 || board[r2][c2].color != EMPTY
			|| board[r1][c1].color != opposite(current_turn_color()) || captured[r1][c1])
			continue;
		move.push_back(position(r2, c2)); // Correct capture-move
		const size_t old = vec.size();
		captured[r1][c1] = true; // For preventing 'recapturing' piece at (r1; c1) in moves produced by recursive call to this function(next 4 lines)
		if (white_turn) // We can become queen at this move
			r2 == 7 ? _find_deep_capture_queen(vec, move, r2, c2, captured) : _find_deep_capture(vec, move, r2, c2, captured);
		else
			r2 == 0 ? _find_deep_capture_queen(vec, move, r2, c2, captured) : _find_deep_capture(vec, move, r2, c2, captured);
		if (old == vec.size()) // If in recursive call we haven't found any move, then 'move' is a final capture and is one of possible captures
			vec.push_back(move);
		captured[r1][c1] = false; // For correct work on next cycles we should clear changes to 'captured' and 'move' variables
		move.pop_back();
	}
}

void Checkers::_find_deep_capture_queen(std::vector<Move>& vec, Move& move, int row, int column, bool(&captured)[8][8]) const
{
	static CONSTEXPR int d_row[4] = { 1, 1, -1, -1 }, d_column[4] = { 1, -1, 1, -1 };
	for (size_t dir = 0; dir < 4; ++dir)
	{
		bool capture_found = false;
		int r1 = row + d_row[dir], c1 = column + d_column[dir];
		for (; r1 < 7 && c1 < 7 && r1 > 0 && c1 > 0 && board[r1][c1].color != current_turn_color() &&
			!captured[r1][c1]; r1 += d_row[dir], c1 += d_column[dir])
			if (board[r1][c1].color == opposite(current_turn_color()))
			{
				capture_found = true;
				break;
			}
		if (!capture_found)
			continue;
		captured[r1][c1] = true;
		const size_t old = vec.size();
		for (int r2 = r1 + d_row[dir], c2 = c1 + d_column[dir]; r2 < 8 && c2 < 8 && r2 >= 0 && c2 >= 0 &&
			board[r2][c2].color == EMPTY && !captured[r2][c2]; r2 += d_row[dir], c2 += d_column[dir])
		{
			move.push_back(position(r2, c2));
			_find_deep_capture_queen(vec, move, r2, c2, captured);
			move.pop_back();
		}
		if (old == vec.size()) // If in recursive calls we haven't found any move, then any move is a final capture in this direction and is one of possible captures
			for (int r2 = r1 + d_row[dir], c2 = c1 + d_column[dir]; r2 < 8 && c2 < 8 && r2 >= 0 && c2 >= 0 &&
				board[r2][c2].color == EMPTY && !captured[r2][c2]; r2 += d_row[dir], c2 += d_column[dir])
			{
				move.push_back(position(r2, c2));
				vec.push_back(move);
				move.pop_back();
			}
		captured[r1][c1] = false;
	}
}

bool Checkers::get_computer_move(Move& out)
{
	if (state != GAME_CONTINUE)
		return false;
	int alpha = -MAX_SCORE - 1, beta = MAX_SCORE + 1;
	// Prepare for killer heuristic(we also use information about previously computed killers)
	if (undos.size() > _last_ai_use_ply)
	{
		const int d_ply = undos.size() - _last_ai_use_ply;
		for (int i = 0; i < search_depth - d_ply; ++i)
			killers[i] = killers[i + d_ply];
		for (int i = std::max(search_depth - d_ply, 0); i < search_depth; ++i)
			killers[i].clear();
	}
	else if (undos.size() < _last_ai_use_ply)
	{
		const int d_ply = _last_ai_use_ply - undos.size();
		for (int i = search_depth - 1; i >= d_ply; --i)
			killers[i] = killers[i - d_ply];
		for (int i = 0; i < std::min(d_ply, search_depth); ++i)
			killers[i].clear();
	}
	_last_ai_use_ply = undos.size();
	// Get all moves for current position
	std::vector<Move> moves;
	_get_all_moves(moves);
	// Minimax with alpha-beta pruning
	if (white_turn)
	{
		if (moves.empty())
		{
			state = BLACK_WIN;
			return false;
		}
		std::pair<std::vector<Move>::iterator, int> best_move = std::make_pair(moves.end(), -MAX_SCORE);
		white_turn = !white_turn; // Change turn for correct work of recursion(_get_all_moves functions uses this flag)
		for (auto it = moves.begin(); it != moves.end(); ++it)
		{
			move_info undo;
			_perform_move(*it, undo);
			_black_computer_move(1, alpha, beta);
			_undo_move(undo);
			if (_score > best_move.second)
				best_move.second = _score, best_move.first = it;
			if (_score > alpha)
				alpha = _score;
		}
		white_turn = !white_turn; // Change turn back
		out = *best_move.first;
		// Add this position evaluation to transposition table
		if (0 <= MAX_TRANSTABLE_MAKE_DEPTH[search_depth])
			_transtable_white[_raw_board] = best_move.second;
	}
	else
	{
		if (moves.empty())
		{
			state = WHITE_WIN;
			return false;
		}
		std::pair<std::vector<Move>::iterator, int> best_move = std::make_pair(moves.end(), MAX_SCORE);
		white_turn = !white_turn; // Change turn for correct work of recursion(_get_all_moves functions uses this flag)
		for (auto it = moves.begin(); it != moves.end(); ++it)
		{
			move_info undo;
			_perform_move(*it, undo);
			_white_computer_move(1, alpha, beta);
			_undo_move(undo);
			if (_score < best_move.second)
				best_move.second = _score, best_move.first = it;
			if (_score < beta)
				beta = _score;
		}
		white_turn = !white_turn; // Change turn back
		out = *best_move.first;
		// Add this position evaluation to transposition table
		if (0 <= MAX_TRANSTABLE_MAKE_DEPTH[search_depth])
			_transtable_black[_raw_board] = best_move.second;
	}
	return true;
}

void Checkers::_white_computer_move(size_t depth, int alpha, int beta)
{
	// Use transposition table
	if (depth >= MIN_TRANSTABLE_USE_DEPTH[search_depth])
	{
		auto it = _transtable_white.find(_raw_board);
		if (it != _transtable_white.end())
		{
			_score = it->second;
			return;
		}
	}
	// Reached desired depth, so evaluate this position score
	if (depth == search_depth)
	{
		_score = score();
		return;
	}
	// Get all moves for current position
	std::vector<Move> moves;
	_get_all_moves(moves);
	// Killer heuristic
	size_t killers_found = 0;
	for (const auto& killer : killers[depth])
		for (size_t i = killers_found; i < moves.size(); ++i)
			if (moves[i] == killer)
			{
				swap(moves[i], moves[killers_found++]);
				break;
			}
	// Minimax with alpha-beta pruning
	int best_move = -MAX_SCORE + depth; // For black to always choose the shortest winning path
	white_turn = !white_turn; // Change turn for correct work of recursion(_get_all_moves functions uses this flag)
	for (size_t i = 0; i < moves.size(); ++i)
	{
		move_info undo;
		_perform_move(moves[i], undo);
		_black_computer_move(depth + 1, alpha, beta);
		_undo_move(undo);
		if (_score > best_move)
			best_move = _score;
		if (_score > alpha)
			alpha = _score;
		if (alpha >= beta)
		{
			// Killer heuristic
			if (i >= killers_found) // Update killers if current move is not in killers
			{
				killers[depth].push_front(moves[i]);
				if (killers[depth].size() > MAX_KILLERS)
					killers[depth].pop_back();
			}
			break;
		}
	}
	white_turn = !white_turn; // Change turn back
	_score = best_move;
	// Add this position evaluation to transposition table
	if (depth <= MAX_TRANSTABLE_MAKE_DEPTH[search_depth])
		_transtable_white[_raw_board] = best_move;
}

void Checkers::_black_computer_move(size_t depth, int alpha, int beta)
{
	// Use transposition table
	if (depth >= MIN_TRANSTABLE_USE_DEPTH[search_depth])
	{
		auto it = _transtable_black.find(_raw_board);
		if (it != _transtable_black.end())
		{
			_score = it->second;
			return;
		}
	}
	// Reached desired depth, so evaluate this position score
	if (depth == search_depth)
	{
		_score = score();
		return;
	}
	// Get all moves for current position
	std::vector<Move> moves;
	_get_all_moves(moves);
	// Killer heuristic
	size_t killers_found = 0;
	for (const auto& killer : killers[depth])
		for (size_t i = killers_found; i < moves.size(); ++i)
			if (moves[i] == killer)
			{
				swap(moves[i], moves[killers_found++]);
				break;
			}
	// Minimax with alpha-beta pruning
	int best_move = MAX_SCORE - depth; // For white to always choose the shortest winning path
	white_turn = !white_turn; // Change turn for correct work of recursion(_get_all_moves functions uses this flag)
	for (size_t i = 0; i < moves.size(); ++i)
	{
		move_info undo;
		_perform_move(moves[i], undo);
		_white_computer_move(depth + 1, alpha, beta);
		_undo_move(undo);
		if (_score < best_move)
			best_move = _score;
		if (_score < beta)
			beta = _score;
		if (alpha >= beta)
		{
			// Killer heuristic
			if (i >= killers_found) // Update killers if current move is not in killers
			{
				killers[depth].push_front(moves[i]);
				if (killers[depth].size() > MAX_KILLERS)
					killers[depth].pop_back();
			}
			break;
		}
	}
	white_turn = !white_turn; // Change turn back
	_score = best_move;
	// Add this position evaluation to transposition table
	if (depth <= MAX_TRANSTABLE_MAKE_DEPTH[search_depth])
		_transtable_black[_raw_board] = best_move;
}

void Checkers::perform_computer_move(void)
{
	if (state != GAME_CONTINUE)
		return;
	Move move;
	if (!get_computer_move(move))
		return;
	move_info info;
	_perform_move(move, info);
	undos.push_back(make_pair(move, info));
	redos.swap(decltype(redos)());
	++_position_count[_raw_board];
	white_turn = !white_turn; // Change turn of move
	_update_game_state();
}

bool Checkers::read_move(std::istream& istr, Move& move) throw(checkers_error)
{
	char delim, col;
	int row;
	std::string str_move;
	if (!(istr >> str_move))
		return false;
	std::stringstream ss_move(str_move);
	ss_move >> col >> row;
	--row, col -= 'a';
	if (row < 0 || row > 7 || col < 0 || col > 7 || (row & 1) != (col & 1))
		throw(checkers_error("Position 1 is illegal"));
	move.push_back(position(row, col));
	for (int cur_pos = 2; ss_move >> delim; ++cur_pos)
	{
		if (delim != '-')
			throw(checkers_error("Move has wrong format"));
		ss_move >> col >> row;
		--row, col -= 'a';
		if (row < 0 || row > 7 || col < 0 || col > 7 || (row & 1) != (col & 1))
			throw(checkers_error("Position " + std::to_string(cur_pos) + "is illegal"));
		move.push_back(position(row, col));
	}
	if (move.size() == 1)
		throw(checkers_error("Move cannot consist of only one position"));
	return true;
}

void Checkers::write_move(std::ostream& ostr, const Move& move)
{
	ostr << (char)('a' + move[0].column) << move[0].row + 1;
	for (size_t i = 1; i < move.size(); ++i)
		ostr << '-' << (char)('a' + move[i].column) << move[i].row + 1;
}

void Checkers::load_game(std::istream& istr) throw(checkers_error)
{
	restart();
	Move move;
	for (int cur_move = 1; ; ++cur_move)
	{
		try
		{
			if (!read_move(istr, move))
				break;
		}
		catch(const checkers_error& err)
		{
			restart();
			throw(checkers_error("Error in move "
				+ std::to_string(cur_move) + ": " + err.what()));
		}
		if (state != GAME_CONTINUE)
			throw(checkers_error(
				"Moves are present after the end of a game. They are not played", error_type::WARNING));
		if (!this->move(move))
		{
			restart();
			throw(checkers_error("Error in move " + std::to_string(cur_move)
				+ ": Move is illegal"));
		}
	}
}

void Checkers::save_game(std::ostream& ostr) const
{
	for (const auto& cur : undos)
	{
		write_move(ostr, cur.first);
		ostr << '\n';
	}
}