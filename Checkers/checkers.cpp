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

// checkers.cpp, version 1.4

#include "checkers.h"

const int Checkers::NORMAL_COLUMN_WEIGHT[8] = {
	0, 2, 4, 9, 9, 4, 2, 0
};
const int Checkers::NORMAL_ROW_WEIGHT_WHITE[8] = {
	0, 3, 6, 13, 20, 25, 35, -1
}; // -1 - unused
const int Checkers::NORMAL_ROW_WEIGHT_BLACK[8] = {
	-1, 35, 25, 20, 13, 6, 3, 0
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
const int Checkers::MAX_HISTORY_MAKE_DEPTH[Checkers::MAX_SEARCH_DEPTH + 1] = {
	0, 0, 0, 0, 1, 2, 3, 4, 5
#if !defined _DEBUG && !defined DEBUG
	, 6, 7, 8, 9
#endif
};
const int Checkers::HISTORY_VALUE[Checkers::MAX_SEARCH_DEPTH + 1] = {
	0, 0, 0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20
#if !defined _DEBUG && !defined DEBUG
	, 0x40, 0x80, 0x100, 0x200
#endif
};

Checkers::Checkers(bool mis, bool white) noexcept
	: search_depth(MAX_SEARCH_DEPTH)
{
	restart(mis, white);
}

Checkers::~Checkers(void) noexcept
{}

void Checkers::restart(bool mis, bool white) noexcept
{
	board[3][1] = board[3][3] = board[3][5] = board[3][7] =
		board[4][0] = board[4][2] = board[4][4] = board[4][6] = Piece(PT_EMPTY);
	board[0][0] = board[0][2] = board[0][4] = board[0][6] =
		board[1][1] = board[1][3] = board[1][5] = board[1][7] =
		board[2][0] = board[2][2] = board[2][4] = board[2][6] = Piece(WHITE_SIMPLE);
	board[5][1] = board[5][3] = board[5][5] = board[5][7] =
		board[6][0] = board[6][2] = board[6][4] = board[6][6] =
		board[7][1] = board[7][3] = board[7][5] = board[7][7] = Piece(BLACK_SIMPLE);
	white_turn = white;
	misere = mis;
	state = GAME_CONTINUE;
	_last_ai_use_ply = 0;
	_position_count.clear();
	_transtable_white.clear();
	_transtable_black.clear();
	undos.swap(decltype(undos)());
	redos.swap(decltype(redos)());
	_update_possible_moves();
	for (auto killer : killers)
		killer.clear();
	for (int i = 0; i < 64; ++i)
		for (int j = 0; j < 64; ++j)
			history[i][j] = 0;
}

int Checkers::score(void) const noexcept
{
	int sc(0);
	for (size_t row = 0; row < 8; ++row)
		for (size_t column = row & 1; column < 8; column += 2)
			if (board[row][column].get_colour() == WHITE)
				sc += (board[row][column].is_queen() ? QUEEN_WEIGHT : (NORMAL_WEIGHT +
					NORMAL_ROW_WEIGHT_WHITE[row] + NORMAL_COLUMN_WEIGHT[column]));
			else if (board[row][column].get_colour() == BLACK)
				sc -= (board[row][column].is_queen() ? QUEEN_WEIGHT : (NORMAL_WEIGHT +
					NORMAL_ROW_WEIGHT_BLACK[row] + NORMAL_COLUMN_WEIGHT[column]));
	return misere ? -sc : sc;
}

int Checkers::evaluate(int depth, int alpha, int beta)
{
	std::vector<Move> moves;
	get_all_moves(moves);
	if (moves.empty())
		return no_moves_score(depth);
	if (moves[0].get_captured().size() == 0) // Available moves are non-capture
		return (white_turn ? score() : -score()); // Because score() is computed for white as maximizer
	int sc;
	white_turn = !white_turn; // Change turn for correct work of recursion(_get_all_moves functions uses this flag)
	for (const auto& move : moves)
	{
		_do_move(move);
		sc = -evaluate(depth + 1, -beta, -alpha);
		_undo_move(move);
		if(sc > alpha)
			alpha = sc;
		if (alpha >= beta)
			break;
	}
	white_turn = !white_turn; // Change turn back
	return alpha;
}

void Checkers::_undo_move(const Move& undo)
{
	const Position& oldpos = undo.old_pos(), newpos = undo.new_pos();
	board[newpos.get_row()][newpos.get_column()] = std::move(Piece());
	board[oldpos.get_row()][oldpos.get_column()] = undo.get_original();
	for (size_t i = 0; i < undo.get_captured().size(); ++i)
		board[undo.get_captured()[i].first.get_row()][
			undo.get_captured()[i].first.get_column()] = undo.get_captured()[i].second;
}

void Checkers::_do_move(const Move& move)
{
	const Position& oldpos = move.old_pos(), newpos = move.new_pos();
	board[oldpos.get_row()][oldpos.get_column()] = std::move(Piece());
	board[newpos.get_row()][newpos.get_column()] = move.get_become();
	for (size_t i = 0; i < move.get_captured().size(); ++i)
		board[move.get_captured()[i].first.get_row()][
			move.get_captured()[i].first.get_column()] = std::move(Piece());
}

bool Checkers::legal_move(Move& move) const
{
	std::vector<Move> moves;
	get_all_moves(moves);
	auto move_it = std::find(moves.begin(), moves.end(), move);
	if (move_it == moves.end())
		return false;
	move.set_info_from(*move_it);
	return true;
}

bool Checkers::move(Move& m)
{
	part_undo();
	if (state != GAME_CONTINUE || !legal_move(m))
		return false;
	_do_move(m);
	undos.push_back(m);
	redos.swap(decltype(redos)());
	++_position_count[_raw_board];
	white_turn = !white_turn; // Change turn of move
	_update_game_state();
	_update_possible_moves();
	return true;
}

void Checkers::_update_game_state(void)
{
	std::vector<Move> vec;
	get_all_moves(vec);
	if (vec.size() == 0)
		state = no_moves_state();
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

step_result Checkers::step(const Position& pos)
{
	if (state != GAME_CONTINUE)
		return STEP_ILLEGAL;
	_cur_possible_moves.erase(
		std::remove_if(_cur_possible_moves.begin(), _cur_possible_moves.end(), [&](const Move& m) {
		return m.size() <= _cur_move.size() || m[_cur_move.size()] != pos;
	}), _cur_possible_moves.end());
	if (_cur_possible_moves.empty())
	{
		if (_cur_move.size() > 0)
		{
			part_undo();
			return (step(pos) == STEP_PROCEED ? STEP_ILLEGAL_NEW : STEP_ILLEGAL);
		}
		else
			part_undo();
		return STEP_ILLEGAL;
	}
	_cur_move.add_step(pos);
	if (_cur_move.size() > 1)
	{
		const int cur_row = _cur_move.new_pos().get_row(), cur_col = _cur_move.new_pos().get_column(),
			prev_row = _cur_move[_cur_move.size() - 2].get_row(), prev_col = _cur_move[_cur_move.size() - 2].get_column();
		const int d_col = (cur_col - prev_col < 0 ? -1 : 1), d_row = (cur_row - prev_row < 0 ? -1 : 1);
		board[cur_row][cur_col] = board[prev_row][prev_col];
		board[prev_row][prev_col] = Piece();
		if (!board[cur_row][cur_col].is_queen() && cur_row == (white_turn ? 7 : 0))
			board[cur_row][cur_col] = Piece(white_turn ? Piece(WHITE_QUEEN) : Piece(BLACK_QUEEN));
		for (int row = prev_row, col = prev_col; row != cur_row; row += d_row, col += d_col)
			if (board[row][col].get_type() != PT_EMPTY)
			{
				_cur_move.add_capture(std::make_pair(Position(row, col), board[row][col]));
				board[row][col] = Piece(PT_SHADOW);
				break; // We can't capture more than one piece in one step in legal move
			}
	}
	else
		_cur_move.set_original(board[_cur_move[0].get_row()][_cur_move[0].get_column()]);
	_cur_move.set_become(board[pos.get_row()][pos.get_column()]);
	if (_cur_possible_moves.size() == 1 && _cur_possible_moves[0].size() == _cur_move.size())
	{
		for (const auto& destroyed : _cur_move.get_captured())
			board[destroyed.first.get_row()][destroyed.first.get_column()] = Piece(PT_EMPTY);
		undos.push_back(_cur_move);
		redos.swap(decltype(redos)());
		_cur_possible_moves.clear();
		_cur_move = Move();
		++_position_count[_raw_board];
		white_turn = !white_turn;
		_update_game_state();
		_update_possible_moves();
		return STEP_FINISH;
	}
	return STEP_PROCEED;
}

void Checkers::part_undo(void)
{
	if (_cur_move.size() > 0)
	{
		_undo_move(_cur_move);
		_cur_move = Move();
	}
	_update_possible_moves();
}

void Checkers::undo_move(void)
{
	part_undo();
	if (undos.empty())
		return;
	--_position_count[_raw_board];
	_undo_move(undos.back());
	redos.push(undos.back());
	undos.pop_back();
	white_turn = !white_turn;
	_update_game_state();
	_update_possible_moves();
}

void Checkers::redo_move(void)
{
	part_undo();
	if (redos.empty())
		return;
	_do_move(redos.top());
	undos.push_back(redos.top());
	redos.pop();
	++_position_count[_raw_board];
	white_turn = !white_turn;
	_update_game_state();
	_update_possible_moves();
}

template<colour TURN>
void Checkers::get_all_moves(std::vector<Move>& vec) const
{
	// Search for capture-moves first
	for (size_t row = 0; row < 8; ++row)
		for (size_t column = row & 1; column < 8; column += 2)
			if (board[row][column].get_colour() == TURN)
			{
				Move move(Path(1, Position(row, column)));
				move.set_original(board[row][column]);
				bool capture[8][8] = {}, queen = board[row][column].is_queen();
				tmp_assign<Piece> move_begin(const_cast<Piece&>
					(board[row][column]), Piece()); // Because this position is empty when we move from it
				if (queen)
					_find_deep_capture_queen<TURN>(vec, move, row, column, capture);
				else
					_find_deep_capture<TURN>(vec, move, row, column, capture);
			}
	// Capture-move is mandatory, so we need to check non-capture moves only when we don't have any capture-moves
	if (vec.empty())
		for (size_t row = 0; row < 8; ++row)
			for (size_t column = row & 1; column < 8; column += 2)
				if (board[row][column].get_colour() == TURN)
				{
					if (board[row][column].is_queen())
					{
						constexpr int dx[4] = { 1, 1, -1, -1 }, dy[4] = { 1, -1, 1, -1 };
						for (size_t dir = 0; dir < 4; ++dir)
							for (int r = row + dy[dir], c = column + dx[dir]; r < 8 && c < 8 && r >= 0 && c >= 0
								&& board[r][c].get_type() == PT_EMPTY; r += dy[dir], c += dx[dir])
							{
								Move move(Path(1, Position(row, column)));
								move.add_step(Position(r, c));
								move.set_original(board[row][column]);
								move.set_become(TURN == WHITE ? Piece(WHITE_QUEEN) : Piece(BLACK_QUEEN));
								vec.push_back(std::move(move));
							}
					}
					else if (TURN == WHITE)
					{
						if (column < 7 && board[row + 1][column + 1].get_type() == PT_EMPTY)
						{
							Move move(Path(1, Position(row, column)));
							move.set_original(board[row][column]);
							move.add_step(Position(row + 1, column + 1));
							move.set_become(row == 6 ? Piece(WHITE_QUEEN) : Piece(WHITE_SIMPLE));
							vec.push_back(std::move(move));
						}
						if (column > 0 && board[row + 1][column - 1].get_type() == PT_EMPTY)
						{
							Move move(Path(1, Position(row, column)));
							move.set_original(board[row][column]);
							move.add_step(Position(row + 1, column - 1));
							move.set_become(row == 6 ? Piece(WHITE_QUEEN) : Piece(WHITE_SIMPLE));
							vec.push_back(std::move(move));
						}
					}
					else
					{
						if (column < 7 && board[row - 1][column + 1].get_type() == PT_EMPTY)
						{
							Move move(Path(1, Position(row, column)));
							move.set_original(board[row][column]);
							move.add_step(Position(row - 1, column + 1));
							move.set_become(std::move(row == 1 ? Piece(BLACK_QUEEN) : Piece(BLACK_SIMPLE)));
							vec.push_back(std::move(move));
						}
						if (column > 0 && board[row - 1][column - 1].get_type() == PT_EMPTY)
						{
							Move move(Path(1, Position(row, column)));
							move.set_original(board[row][column]);
							move.add_step(Position(row - 1, column - 1));
							move.set_become(std::move(row == 1 ? Piece(BLACK_QUEEN) : Piece(BLACK_SIMPLE)));
							vec.push_back(std::move(move));
						}
					}
				}
}

template<colour TURN>
void Checkers::_find_deep_capture(std::vector<Move>& vec, Move& move, int row, int column, bool(&captured)[8][8]) const
{
	static constexpr int d_row[4] = { 1, 1, -1, -1 }, d_column[4] = { 1, -1, 1, -1 };
	for (size_t dir = 0; dir < 4; ++dir)
	{
		const int r1 = row + d_row[dir], c1 = column + d_column[dir], r2 = r1 + d_row[dir], c2 = c1 + d_column[dir];
		if (r2 < 0 || c2 < 0 || r2 > 7 || c2 > 7 || board[r2][c2].get_type() != PT_EMPTY
			|| board[r1][c1].get_colour() != opposite(TURN) || captured[r1][c1])
			continue;
		move.add_step(Position(r2, c2)); // Correct capture-move
		move.add_capture(std::make_pair(Position(r1, c1), board[r1][c1]));
		const size_t old = vec.size();
		captured[r1][c1] = true; // For preventing 'recapturing' piece at (r1; c1) in moves produced by recursive call to this function(next 4 lines)
		if (TURN == WHITE) // We can become queen at this move
			if (r2 == 7)
			{
				_find_deep_capture_queen<WHITE>(vec, move, r2, c2, captured);
				if (old == vec.size()) // If in recursive call we haven't found any move, then 'move' is a final capture and is one of possible captures
				{
					move.set_become(Piece(WHITE_QUEEN));
					vec.push_back(move);
				}
			}
			else
			{
				_find_deep_capture<WHITE>(vec, move, r2, c2, captured);
				if (old == vec.size()) // If in recursive call we haven't found any move, then 'move' is a final capture and is one of possible captures
				{
					move.set_become(Piece(WHITE_SIMPLE));
					vec.push_back(move);
				}
			}
		else
			if (r2 == 0)
			{
				_find_deep_capture_queen<BLACK>(vec, move, r2, c2, captured);
				if (old == vec.size()) // If in recursive call we haven't found any move, then 'move' is a final capture and is one of possible captures
				{
					move.set_become(Piece(BLACK_QUEEN));
					vec.push_back(move);
				}
			}
			else
			{
				_find_deep_capture<BLACK>(vec, move, r2, c2, captured);
				if (old == vec.size()) // If in recursive call we haven't found any move, then 'move' is a final capture and is one of possible captures
				{
					move.set_become(Piece(BLACK_SIMPLE));
					vec.push_back(move);
				}
			}
		captured[r1][c1] = false; // For correct work on next cycles we should clear changes to 'captured' and 'move' variables
		move.pop_step();
		move.pop_capture();
	}
}

template<colour TURN>
void Checkers::_find_deep_capture_queen(std::vector<Move>& vec, Move& move, int row, int column, bool(&captured)[8][8]) const
{
	static constexpr int d_row[4] = { 1, 1, -1, -1 }, d_column[4] = { 1, -1, 1, -1 };
	for (size_t dir = 0; dir < 4; ++dir)
	{
		bool capture_found = false;
		int r1 = row + d_row[dir], c1 = column + d_column[dir];
		for (; r1 < 7 && c1 < 7 && r1 > 0 && c1 > 0 && board[r1][c1].get_colour() != TURN &&
			!captured[r1][c1]; r1 += d_row[dir], c1 += d_column[dir])
			if (board[r1][c1].get_colour() == opposite(TURN))
			{
				capture_found = true;
				break;
			}
		if (!capture_found)
			continue;
		captured[r1][c1] = true;
		const size_t old = vec.size();
		for (int r2 = r1 + d_row[dir], c2 = c1 + d_column[dir]; r2 < 8 && c2 < 8 && r2 >= 0 && c2 >= 0 &&
			board[r2][c2].get_type() == PT_EMPTY; r2 += d_row[dir], c2 += d_column[dir])
		{
			move.add_step(Position(r2, c2));
			move.add_capture(std::make_pair(Position(r1, c1), board[r1][c1]));
			_find_deep_capture_queen<TURN>(vec, move, r2, c2, captured);
			move.pop_step();
			move.pop_capture();
		}
		if (old == vec.size()) // If in recursive calls we haven't found any move, then any move is a final capture in this direction and is one of possible captures
			if(TURN == WHITE)
				for (int r2 = r1 + d_row[dir], c2 = c1 + d_column[dir]; r2 < 8 && c2 < 8 && r2 >= 0 && c2 >= 0 &&
					board[r2][c2].get_type() == PT_EMPTY; r2 += d_row[dir], c2 += d_column[dir])
				{
					move.add_step(Position(r2, c2));
					move.add_capture(std::make_pair(Position(r1, c1), board[r1][c1]));
					move.set_become(Piece(WHITE_QUEEN));
					vec.push_back(move);
					move.pop_step();
					move.pop_capture();
				}
			else
				for (int r2 = r1 + d_row[dir], c2 = c1 + d_column[dir]; r2 < 8 && c2 < 8 && r2 >= 0 && c2 >= 0 &&
					board[r2][c2].get_type() == PT_EMPTY; r2 += d_row[dir], c2 += d_column[dir])
				{
					move.add_step(Position(r2, c2));
					move.add_capture(std::make_pair(Position(r1, c1), board[r1][c1]));
					move.set_become(Piece(BLACK_QUEEN));
					vec.push_back(move);
					move.pop_step();
					move.pop_capture();
				}
		captured[r1][c1] = false;
	}
}

bool Checkers::get_computer_move(Move& out, int& out_score)
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
	get_all_moves(moves);
	if (moves.empty())
	{
		state = no_moves_state();
		out_score = white_turn ? no_moves_score(0) : -no_moves_score(0);
		return false;
	}
	// Minimax with alpha-beta pruning
	if (white_turn)
	{
		std::pair<std::vector<Move>::iterator, int> best_move = std::move(std::make_pair(moves.end(), -MAX_SCORE));
		white_turn = !white_turn; // Change turn for correct work of recursion(_get_all_moves functions uses this flag)
		for (auto it = moves.begin(); it != moves.end(); ++it)
		{
			_do_move(*it);
			_black_computer_move(1, alpha, beta);
			_undo_move(*it);
			if (_score > best_move.second)
				best_move.second = _score, best_move.first = it;
			if (_score > alpha)
				alpha = _score;
		}
		white_turn = !white_turn; // Change turn back
		out = *best_move.first, out_score = best_move.second;
		// Add this position evaluation to transposition table
		if (0 <= MAX_TRANSTABLE_MAKE_DEPTH[search_depth])
			_transtable_white[_raw_board] = best_move.second;
	}
	else
	{
		std::pair<std::vector<Move>::iterator, int> best_move = std::move(std::make_pair(moves.end(), MAX_SCORE));
		white_turn = !white_turn; // Change turn for correct work of recursion(_get_all_moves functions uses this flag)
		for (auto it = moves.begin(); it != moves.end(); ++it)
		{
			_do_move(*it);
			_white_computer_move(1, alpha, beta);
			_undo_move(*it);
			if (_score < best_move.second)
				best_move.second = _score, best_move.first = it;
			if (_score < beta)
				beta = _score;
		}
		white_turn = !white_turn; // Change turn back
		out = *best_move.first, out_score = best_move.second;
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
		_score = evaluate(depth, alpha, beta);
		return;
	}
	// Get all moves for current position
	std::vector<Move> moves;
	get_all_moves<WHITE>(moves);
	if (moves.empty())
	{
		_score = no_moves_score(depth);
		return;
	}
	// Killer heuristic
	size_t killers_found = 0;
	for (const auto& killer : killers[depth])
		for (size_t i = killers_found; i < moves.size(); ++i)
			if (moves[i] == killer)
			{
				swap(moves[i], moves[killers_found++]);
				break;
			}
	// History heuristic
	sort(moves.begin() + killers_found, moves.end(), [&](const Move& lhs, const Move& rhs){
		return _history_move_score(lhs) > _history_move_score(rhs);
	});
	// Minimax with alpha-beta pruning
	int best_move = -MAX_SCORE + depth; // For black to always choose the shortest winning path
	white_turn = !white_turn; // Change turn for correct work of recursion(_get_all_moves functions uses this flag)
	for (size_t i = 0; i < moves.size(); ++i)
	{
		Move& cur_move = moves[i];
		_do_move(cur_move);
		_black_computer_move(depth + 1, alpha, beta);
		_undo_move(cur_move);
		if (_score > best_move)
			best_move = _score;
		if (_score > alpha)
			alpha = _score;
		if (alpha >= beta)
		{
			// History heuristic
			if (depth <= MAX_HISTORY_MAKE_DEPTH[search_depth])
				history[(cur_move.old_pos().get_row() << 3) + cur_move.old_pos().get_column()]
				[(cur_move.new_pos().get_row() << 3) + cur_move.new_pos().get_column()] += HISTORY_VALUE[search_depth - depth];
			// Killer heuristic
			if (i >= killers_found) // Update killers if current move is not in killers
			{
				killers[depth].push_front(std::move(cur_move)); // std::move ?
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
		_transtable_white[_raw_board] = _score;
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
		_score = -evaluate(depth, -beta, -alpha); // Because evaluate() will maximize black 
		return;
	}
	// Get all moves for current position
	std::vector<Move> moves;
	get_all_moves<BLACK>(moves);
	if (moves.empty())
	{
		_score = -no_moves_score(depth);
		return;
	}
	// Killer heuristic
	size_t killers_found = 0;
	for (const auto& killer : killers[depth])
		for (size_t i = killers_found; i < moves.size(); ++i)
			if (moves[i] == killer)
			{
				swap(moves[i], moves[killers_found++]);
				break;
			}
	// History heuristic
	sort(moves.begin() + killers_found, moves.end(), [&](const Move& lhs, const Move& rhs) {
		return _history_move_score(lhs) > _history_move_score(rhs);
	});
	// Minimax with alpha-beta pruning
	int best_move = MAX_SCORE - depth; // For white to always choose the shortest winning path
	white_turn = !white_turn; // Change turn for correct work of recursion(_get_all_moves functions uses this flag)
	for (size_t i = 0; i < moves.size(); ++i)
	{
		Move& cur_move = moves[i];
		_do_move(cur_move);
		_white_computer_move(depth + 1, alpha, beta);
		_undo_move(cur_move);
		if (_score < best_move)
			best_move = _score;
		if (_score < beta)
			beta = _score;
		if (alpha >= beta)
		{
			// History heuristic
			if (depth <= MAX_HISTORY_MAKE_DEPTH[search_depth])
				history[(cur_move.old_pos().get_row() << 3) + cur_move.old_pos().get_column()]
				[(cur_move.new_pos().get_row() << 3) + cur_move.new_pos().get_column()] += HISTORY_VALUE[search_depth - depth];
			// Killer heuristic
			if (i >= killers_found) // Update killers if current move is not in killers
			{
				killers[depth].push_front(std::move(cur_move));
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
		_transtable_black[_raw_board] = _score;
}

void Checkers::perform_computer_move(void)
{
	part_undo();
	if (state != GAME_CONTINUE)
		return;
	Move move;
	if (!get_computer_move(move))
		return;
	_do_move(move);
	undos.push_back(move);
	redos.swap(decltype(redos)());
	++_position_count[_raw_board];
	white_turn = !white_turn; // Change turn of move
	_update_game_state();
	_update_possible_moves();
}

bool Checkers::read_move(std::istream& istr, Move& move)
{
	char delim, col;
	int row;
	std::string str_move;
	move = Move();
	if (!(istr >> str_move))
		return false;
	std::stringstream ss_move(str_move);
	ss_move >> col >> row;
	--row, col -= 'a';
	if (row < 0 || row > 7 || col < 0 || col > 7 || (row & 1) != (col & 1))
		throw(checkers_error("Position 1 is illegal"));
	move.add_step(Position(row, col));
	for (int cur_pos = 2; ss_move >> delim; ++cur_pos)
	{
		if (delim != '-' && delim != ':')
			throw(checkers_error("Move has wrong format"));
		ss_move >> col >> row;
		--row, col -= 'a';
		if (row < 0 || row > 7 || col < 0 || col > 7 || (row & 1) != (col & 1))
			throw(checkers_error("Position " + std::to_string(cur_pos) + "is illegal"));
		move.add_step(Position(row, col));
	}
	if (move.size() == 1)
		throw(checkers_error("Move cannot consist of only one position"));
	return true;
}

void Checkers::write_move(std::ostream& ostr, const Move& move)
{
	char delim = (move.get_captured().empty() ? '-' : ':');
	ostr << (char)('a' + move[0].get_column()) << move[0].get_row() + 1;
	for (size_t i = 1; i < move.size(); ++i)
		ostr << delim << (char)('a' + move[i].get_column()) << move[i].get_row() + 1;
}

void Checkers::load_game(std::istream& istr)
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
		write_move(ostr, cur);
		ostr << '\n';
	}
}

// Explicit template instantiations
template void Checkers::_find_deep_capture<WHITE>(std::vector<Move>&, Move&, int, int, bool(&)[8][8]) const;
template void Checkers::_find_deep_capture<BLACK>(std::vector<Move>&, Move&, int, int, bool(&)[8][8]) const;
template void Checkers::_find_deep_capture_queen<WHITE>(std::vector<Move>&, Move&, int, int, bool(&)[8][8]) const;
template void Checkers::_find_deep_capture_queen<BLACK>(std::vector<Move>&, Move&, int, int, bool(&)[8][8]) const;
template void Checkers::get_all_moves<WHITE>(std::vector<Move>&) const;
template void Checkers::get_all_moves<BLACK>(std::vector<Move>&) const;