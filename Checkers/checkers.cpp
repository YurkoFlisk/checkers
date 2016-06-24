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

// checkers.cpp, version 1.5

#include "checkers.h"

const int Checkers::HISTORY_VALUE[Checkers::MAX_SEARCH_DEPTH + 1] = {
	0, 0, 0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20
#if !defined _DEBUG && !defined DEBUG
	, 0x40, 0x80, 0x100, 0x200
#endif
};
const int16_t Checkers::NORMAL_COLUMN_WEIGHT[8] = {
	0, 2, 4, 9, 9, 4, 2, 0
};
const int16_t Checkers::NORMAL_ROW_WEIGHT_WHITE[8] = {
	0, 3, 6, 13, 20, 25, 35, -1
}; // -1 - unused
const int16_t Checkers::NORMAL_ROW_WEIGHT_BLACK[8] = {
	-1, 35, 25, 20, 13, 6, 3, 0
}; // -1 - unused
const int16_t Checkers::PSQ_TABLE[PT_COUNT][8][8] = {
	{ // PT_EMPTY
	},{ // PT_SHADOW
	},{ // WHITE_SIMPLE
		{  0,  2,  4,  9,  9,  4,  2,  0 },
		{  3,  5,  7, 12, 12,  7,  5,  3 },
		{  6,  8, 10, 15, 15, 10,  8,  6 },
		{ 13, 15, 17, 22, 22, 17, 15, 13 },
		{ 20, 22, 24, 29, 29, 24, 22, 20 },
		{ 25, 27, 29, 34, 34, 29, 27, 25 },
		{ 35, 37, 39, 44, 44, 39, 37, 35 },
		{ -1, -1, -1, -1, -1, -1, -1, -1 } // unused
	},{ // BLACK_SIMPLE
		{ -1, -1, -1, -1, -1, -1, -1, -1 }, // unused
		{ 35, 37, 39, 44, 44, 39, 37, 35 },
		{ 25, 27, 29, 34, 34, 29, 27, 25 },
		{ 20, 22, 24, 29, 29, 24, 22, 20 },
		{ 13, 15, 17, 22, 22, 17, 15, 13 },
		{  6,  8, 10, 15, 15, 10,  8,  6 },
		{  3,  5,  7, 12, 12,  7,  5,  3 },
		{  0,  2,  4,  9,  9,  4,  2,  0 },
	},{ // unused
	},{ // unused
	},{ // WHITE_QUEEN
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},{ // BLACK_QUEEN
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	}
};
const int8_t Checkers::MIN_HISTORY_MAKE_DEPTH[Checkers::MAX_SEARCH_DEPTH + 1] = {
	0, 1, 1, 1, 1, 2, 2, 2, 2
#if !defined _DEBUG && !defined DEBUG
	, 2, 2, 2, 2
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
	inc_score = 0; // it is important that it is cleared before calling Board's restart
	Board::restart(mis, white);
	_transtable[0].clear();
	_transtable[1].clear();
	(decltype(undos)()).swap(undos);
	(decltype(redos)()).swap(redos);
	_update_possible_moves();
	for (auto killer : killers)
		killer.clear();
	for (int i = 0; i < 64; ++i)
		for (int j = 0; j < 64; ++j)
			history[i][j] = 0;
}

void Checkers::_put_piece(const Position& pos, Piece piece)
{
	Board::_put_piece(pos, piece);
	inc_score += PSQ_TABLE[piece.get_type()][pos.get_row()][pos.get_column()];
}

void Checkers::_remove_piece(const Position& pos)
{
	inc_score -= PSQ_TABLE[board[pos.get_row()][pos.get_column()].get_type()][pos.get_row()][pos.get_column()];
	Board::_remove_piece(pos);
}

int16_t Checkers::score(void) const noexcept
{
	int16_t sc(inc_score);
	sc += NORMAL_WEIGHT*(piece_count[WHITE_SIMPLE] - piece_count[BLACK_SIMPLE]);
	sc += QUEEN_WEIGHT*(piece_count[WHITE_QUEEN] - piece_count[BLACK_QUEEN]);
	return get_misere() ? -sc : sc;
}

template<colour TURN>
int16_t Checkers::evaluate(int16_t alpha, int16_t beta)
{
	std::vector<Move> moves;
	get_all_moves<TURN>(moves);
	if (moves.empty())
		return lose_score(cur_ply);
	if (moves[0].get_captured().size() == 0) // Available moves are non-capture
		return (TURN == WHITE ? score() : -score()); // Because score() is computed for white as maximizer
	++cur_ply;
	for (const auto& move : moves)
	{
		_do_move(move);
		_score = -evaluate<opposite(TURN)>(-beta, -alpha);
		_undo_move(move);
		if (_score > alpha)
			alpha = _score;
		if (alpha >= beta)
			break;
	}
	--cur_ply;
	return alpha;
}

bool Checkers::move(Move& m)
{
	part_undo();
	if (get_state() != GAME_CONTINUE || !legal_move(m))
		return false;
	_do_move(m);
	undos.push_back(m);
	(decltype(redos)()).swap(redos);
	_proceed();
	_update_game_state();
	_update_possible_moves();
	return true;
}

step_result Checkers::step(const Position& pos)
{
	if (get_state() != GAME_CONTINUE)
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
		set_cell(cur_row, cur_col, board[prev_row][prev_col]);
		set_cell(prev_row, prev_col, Piece());
		if (!board[cur_row][cur_col].is_queen() && cur_row == (get_white_turn() ? 7 : 0))
			set_cell(cur_row, cur_col, get_white_turn() ? Piece(WHITE_QUEEN) : Piece(BLACK_QUEEN));
		for (int row = prev_row, col = prev_col; row != cur_row; row += d_row, col += d_col)
			if (board[row][col].get_type() != PT_EMPTY)
			{
				_cur_move.add_capture(std::make_pair(Position(row, col), board[row][col]));
				set_cell(row, col, Piece(PT_SHADOW));
				break; // We can't capture more than one piece in one step in legal move
			}
	}
	else
		_cur_move.set_original(board[_cur_move[0].get_row()][_cur_move[0].get_column()]);
	_cur_move.set_become(board[pos.get_row()][pos.get_column()]);
	if (_cur_possible_moves.size() == 1 && _cur_possible_moves[0].size() == _cur_move.size())
	{
		Move move = _cur_move;
		part_undo();
		_do_move(move);
		undos.push_back(move);
		(decltype(redos)()).swap(redos);
		_cur_possible_moves.clear();
		_proceed();
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
		set_cell(_cur_move.new_pos(), Piece());
		set_cell(_cur_move.old_pos(), _cur_move.get_original());
		for (const auto& destroyed : _cur_move.get_captured())
			set_cell(destroyed.first, destroyed.second);
		_cur_move = Move();
	}
	_update_possible_moves();
}

void Checkers::undo_move(void)
{
	part_undo();
	if (undos.empty())
		return;
	_retreat();
	_undo_move(undos.back());
	redos.push(undos.back());
	undos.pop_back();
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
	_proceed();
	_update_game_state();
	_update_possible_moves();
}

template<colour TURN>
bool Checkers::get_computer_move(Move& out, int& out_score)
{
	if (get_state() != GAME_CONTINUE)
		return false;
	int16_t alpha = -MAX_SCORE - 1, beta = MAX_SCORE + 1;
	// Get all moves for current position
	std::vector<Move> moves;
	get_all_moves<TURN>(moves);
	if (moves.empty())
	{
		state = no_moves_state();
		out_score = lose_score(cur_ply);
		return false;
	}
	// Make sure killers size is sufficient
	if ((++cur_ply) + MAX_SEARCH_DEPTH > killers.size())
		killers.resize(cur_ply + MAX_SEARCH_DEPTH);
	// Principal variation search
	auto best_move = moves.end();
	int raised_alpha_cnt = 0, move_idx = 0;
	for (; move_idx < moves.size(); ++move_idx)
	{
		auto it = moves.begin() + move_idx;
		_do_move(*it);
		if (raised_alpha_cnt < 2)
			_score = -_pvs<opposite(TURN)>(search_depth - 1, -beta, -alpha);
		else
		{
			_score = -_pvs<opposite(TURN)>(search_depth - 1, -alpha - 1, -alpha);
			if (beta > _score && _score > alpha)
				_score = -_pvs<opposite(TURN)>(search_depth - 1, -beta, -_score);
		}
		_undo_move(*it);
		if (_score > alpha)
			alpha = _score, best_move = it, ++raised_alpha_cnt;
	}
	--cur_ply, out = *best_move, out_score = alpha;
	// Add this position evaluation to transposition table
	TT_Entry& entry = _transtable[TURN - WHITE][get_raw()];
	entry.value = out_score;
	entry.depth = 0;
	entry.bound_type = TTBOUND_EXACT;
	return true;
}

template<colour TURN>
int16_t Checkers::_pvs(int8_t depth, int16_t alpha, int16_t beta)
{
	int16_t old_alpha = alpha;
	// Use transposition table
	auto tt_it = _transtable[TURN - WHITE].find(get_raw());
	if (tt_it != _transtable[TURN - WHITE].end() && tt_it->second.depth >= depth)
		switch (tt_it->second.bound_type)
		{
		case TTBOUND_EXACT:
			return tt_it->second.value;
		case TTBOUND_LOWER:
			alpha = std::max(alpha, tt_it->second.value);
			break;
		case TTBOUND_UPPER:
			beta = std::min(beta, tt_it->second.value);
			break;
		}
	// Mate distance pruning
	alpha = std::max(alpha, lose_score(cur_ply));
	beta = std::min(beta, win_score(cur_ply + 1));
	if (alpha >= beta)
		return alpha;
	// Reached desired depth, so evaluate this position score
	if (depth == 0)
		return evaluate<TURN>(alpha, beta);
	// Get all moves for current position
	std::vector<Move> moves;
	get_all_moves<TURN>(moves);
	if (moves.empty())
		return lose_score(cur_ply);
	// Killer heuristic
	size_t killers_found = 0, move_idx = 0;
	for (const auto& killer : killers[cur_ply])
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
	// Principal variation search
	++cur_ply;
	for (bool pv_search = true; move_idx < moves.size(); ++move_idx)
	{
		Move& cur_move = moves[move_idx];
		_do_move(cur_move);
		if (pv_search)
			_score = -_pvs<opposite(TURN)>(depth - 1, -beta, -alpha);
		else
		{
			_score = -_pvs<opposite(TURN)>(depth - 1, -alpha - 1, -alpha);
			if (beta > _score && _score > alpha)
				_score = -_pvs<opposite(TURN)>(depth - 1, -beta, -_score);
		}
		_undo_move(cur_move);
		if (_score > alpha)
			alpha = _score, pv_search = false; // pv_search = false here?
		if (alpha >= beta)
		{
			// History heuristic
			if (depth >= MIN_HISTORY_MAKE_DEPTH[search_depth])
				history[pos_idx(cur_move.old_pos())][pos_idx(cur_move.new_pos())] += HISTORY_VALUE[depth];
			// Killer heuristic
			if (move_idx >= killers_found) // Update killers if current move is not in killers
			{
				killers[cur_ply].push_front(std::move(cur_move));
				if (killers[cur_ply].size() > MAX_KILLERS)
					killers[cur_ply].pop_back();
			}
			break;
		}
	}
	--cur_ply;
	// Add this position evaluation to transposition table
	if (tt_it == _transtable[TURN - WHITE].end() || depth > tt_it->second.depth)
	{
		TT_Entry& entry = (tt_it == _transtable[TURN - WHITE].end() ?
			_transtable[TURN - WHITE][get_raw()] : tt_it->second);
		entry.value = alpha;
		entry.depth = depth;
		if (alpha <= old_alpha)
			entry.bound_type = TTBOUND_UPPER;
		else if (alpha < beta)
			entry.bound_type = TTBOUND_EXACT;
		else
			entry.bound_type = TTBOUND_LOWER;
	}
	return alpha;
}

void Checkers::perform_computer_move(void)
{
	part_undo();
	if (get_state() != GAME_CONTINUE)
		return;
	Move move;
	if (!get_computer_move(move))
		return;
	_do_move(move);
	undos.push_back(move);
	(decltype(redos)()).swap(redos);
	_proceed();
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
		catch (const checkers_error& err)
		{
			restart();
			throw(checkers_error("Error in move "
				+ std::to_string(cur_move) + ": " + err.what()));
		}
		if (get_state() != GAME_CONTINUE)
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
template int16_t	Checkers::_pvs<WHITE>(int8_t, int16_t, int16_t);
template int16_t	Checkers::_pvs<BLACK>(int8_t, int16_t, int16_t);
template bool		Checkers::get_computer_move<WHITE>(Move&, int&);
template bool		Checkers::get_computer_move<BLACK>(Move&, int&);
template int16_t	Checkers::evaluate<WHITE>(int16_t, int16_t);
template int16_t	Checkers::evaluate<BLACK>(int16_t, int16_t);