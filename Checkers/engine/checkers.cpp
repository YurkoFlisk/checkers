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
	0, 0, 0x1, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20
#if !defined _DEBUG && !defined DEBUG
	, 0x40, 0x80, 0x100, 0x200
#endif
};
int16_t PSQ_TABLE[PT_COUNT][8][8] = { // Only for left columns, right are filled symmetrically in init_psq function
	{ // PT_EMPTY
	},{ // PT_SHADOW
	},{ // WHITE_SIMPLE
		{  0,  1,  3,  5 },
		{  3,  4,  5,  8 },
		{  5,  7, 10, 14 },
		{ 13, 15, 16, 16 },
		{ 14, 15, 20, 20 },
		{ 17, 18, 20, 20 },
		{ 25, 27, 29, 30 },
		{ -1, -1, -1, -1 } // unused
	},{ // BLACK_SIMPLE(filled symmetrically to WHITE_SIMPLE in init_psq function)
	},{ // unused
	},{ // unused
	},{ // WHITE_QUEEN
		{ -15, -10, -10, -10 },
		{ -10,   5,   0,   0 },
		{ -10,   0,   5,   0 },
		{ -10,   0,   0,  10 },
		{ -10,   0,   0,  10 },
		{ -10,   0,   5,   0 },
		{ -10,	 5,   0,   0 },
		{ -15, -10, -10, -10 }
	},{ // BLACK_QUEEN(filled symmetrically to WHITE_QUEEN in init_psq function)
	}
};
const int8_t Checkers::MIN_HISTORY_MAKE_DEPTH[Checkers::MAX_SEARCH_DEPTH + 1] = {
	0, 1, 1, 1, 1, 2, 2, 2, 2
#if !defined _DEBUG && !defined DEBUG
	, 2, 3, 3, 3
#endif
};

Checkers::Checkers(bool mis, bool white) noexcept
	: search_depth(MAX_SEARCH_DEPTH)
{
	init_psq();
	restart(mis, white);
}

Checkers::~Checkers(void) noexcept = default;

void Checkers::init_psq(void)
{
	// Fill right columns symmetrically to left
	for (auto& row : PSQ_TABLE[WHITE_SIMPLE])
		for (int i = 0; i < 4; ++i)
			row[7 - i] = row[i];
	for (auto& row : PSQ_TABLE[WHITE_QUEEN])
		for (int i = 0; i < 4; ++i)
			row[7 - i] = row[i];
	// Fill black's PSQs symmetrically to white's
	for (int row = 0; row < 8; ++row)
	{
		std::transform(PSQ_TABLE[WHITE_SIMPLE][row], PSQ_TABLE[WHITE_SIMPLE][row] + 8, PSQ_TABLE[BLACK_SIMPLE][7 - row],
			[](int16_t num) {return -num; });
		std::transform(PSQ_TABLE[WHITE_QUEEN][row], PSQ_TABLE[WHITE_QUEEN][row] + 8, PSQ_TABLE[BLACK_QUEEN][7 - row],
			[](int16_t num) {return -num; });
	}
}

void Checkers::restart(bool mis, bool white) noexcept
{
	part_undo();
	inc_score = 0; // It is important that it is cleared before calling Board's restart
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

inline void Checkers::_put_piece(Position pos, Piece piece)
{
	Board::_put_piece(pos, piece);
	inc_score += PSQ_TABLE[piece.get_type()][pos.get_row()][pos.get_column()];
}

inline void Checkers::_remove_piece(Position pos)
{
	inc_score -= PSQ_TABLE[board[pos.get_row()][pos.get_column()].get_type()][pos.get_row()][pos.get_column()];
	Board::_remove_piece(pos);
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
		_set_cell(cur_row, cur_col, board[prev_row][prev_col]);
		_set_cell(prev_row, prev_col, Piece());
		if (!board[cur_row][cur_col].is_queen() && cur_row == (get_white_turn() ? 7 : 0))
			_set_cell(cur_row, cur_col, get_white_turn() ? Piece(WHITE_QUEEN) : Piece(BLACK_QUEEN));
		for (int row = prev_row, col = prev_col; row != cur_row; row += d_row, col += d_col)
			if (board[row][col].get_type() != PT_EMPTY)
			{
				_cur_move.add_capture(std::make_pair(Position(row, col), board[row][col]));
				_set_cell(row, col, Piece(PT_SHADOW));
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
		_set_cell(_cur_move.new_pos(), Piece());
		_set_cell(_cur_move.old_pos(), _cur_move.get_original());
		for (const auto& destroyed : _cur_move.get_captured())
			_set_cell(destroyed.first, destroyed.second);
		_cur_move = Move();
	}
	_update_possible_moves();
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

inline int16_t Checkers::score(void) const noexcept
{
	int16_t sc(inc_score);
	sc += NORMAL_WEIGHT*(piece_count[WHITE_SIMPLE] - piece_count[BLACK_SIMPLE]);
	sc += QUEEN_WEIGHT*(piece_count[WHITE_QUEEN] - piece_count[BLACK_QUEEN]);
	return get_misere() ? -sc : sc;
}

template<colour TURN>
int16_t Checkers::evaluate(int16_t alpha, int16_t beta)
{
	// Mate distance pruning
	alpha = std::max(alpha, lose_score(cur_ply));
	beta = std::min(beta, win_score(cur_ply + 1));
	if (alpha >= beta)
		return alpha;
	// Stand pat
	const int16_t stand_pat = (TURN == WHITE ? score() : -score()); // Because score() is computed for white as maximizer
	if (stand_pat >= beta + STAND_PAT_MARGIN)
		return stand_pat;
	// Delta pruning
	else if (stand_pat + DELTA_PRUNING_MARGIN < alpha)
		return stand_pat;
	// Get all capture moves
	std::vector<Move> moves;
	get_all_moves<TURN, CAPTURE>(moves);
	// If no capture moves, check whether there are non-capture moves
	if (moves.empty())
	{
		get_all_moves<TURN, NON_CAPTURE>(moves);
		return moves.empty() ? lose_score(cur_ply) : stand_pat;
	}
	// Quiescent search
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

template<colour TURN>
bool Checkers::get_computer_move(Move& out, int& out_score)
{
	if (get_state() != GAME_CONTINUE)
		return false;
	// Get all moves for current position
	std::vector<Move> moves;
	get_all_moves<TURN>(moves);
	if (moves.empty())
	{
		state = no_moves_state();
		out_score = no_moves_score(cur_ply);
		return false;
	}
	// Make sure killers size is sufficient
	if ((++cur_ply) + MAX_SEARCH_DEPTH > killers.size())
		killers.resize(cur_ply + MAX_SEARCH_DEPTH);
	// History heuristic
	sort(moves.begin(), moves.end(), [&](const Move& lhs, const Move& rhs) {
		return _history_move_score(lhs) > _history_move_score(rhs);
	});
	// Main iterative deepening loop
	out_score = 0;
	for (int depth = 1; depth <= search_depth; ++depth)
	{
		// Principal variation search with aspiration windows
		int16_t delta = 21, best_score;
		int16_t alpha = std::max(out_score - delta, -MAX_SCORE),
			beta = std::min(out_score + delta, +MAX_SCORE);
		auto best_move = moves.end();
		while (true)
		{
			best_score = alpha;
			int raised_alpha_cnt = 0, move_idx = 0;
			for (; move_idx < moves.size(); ++move_idx)
			{
				auto it = moves.begin() + move_idx;
				_do_move(*it);
				if (raised_alpha_cnt < 2)
					_score = -_pvs<opposite(TURN)>(depth - 1, -beta, -best_score);
				else
				{
					_score = -_pvs<opposite(TURN)>(depth - 1, -best_score - 1, -best_score);
					if (beta > _score && _score > best_score)
						_score = -_pvs<opposite(TURN)>(depth - 1, -beta, -_score);
				}
				_undo_move(*it);
				if (_score > best_score)
					best_score = _score, best_move = it, ++raised_alpha_cnt;
				if (best_score >= beta)
					break;
			}
			// Update delta
			delta += delta / 3;
			// Update alpha and beta
			if (best_score <= alpha) // fail-low
			{
				beta = (best_score + beta) / 2;
				alpha = std::max(best_score - delta, -MAX_SCORE);
			}
			else if (best_score >= beta) // fail-high
			{
				alpha = (alpha + best_score) / 2;
				beta = std::min(best_score + delta, +MAX_SCORE);
			}
			else
				break;
		}
		// Update moves list
		// In next iteration current best move will be examined first
		swap(*best_move, moves.front());
		// History heuristic
		sort(moves.begin() + 1, moves.end(), [&](const Move& lhs, const Move& rhs) {
			return _history_move_score(lhs) > _history_move_score(rhs);
		});
		// Set out score
		out_score = best_score;
	}
	--cur_ply, out = moves.front();
	// Add this position evaluation to transposition table
	TT_Entry& entry = _transtable[TURN - WHITE][get_hash()];
	entry.value = value_to_tt(out_score, cur_ply);
	entry.depth = 0;
	entry.best_move_from = out.old_pos();
	entry.best_move_to = out.new_pos();
	entry.bound_type = TTBOUND_EXACT;
	return true;
}

template<colour TURN>
int16_t Checkers::_pvs(int8_t depth, int16_t alpha, int16_t beta)
{
	// Reached desired depth, so evaluate this position score
	if (depth == 0)
		return evaluate<TURN>(alpha, beta);
	// Mate distance pruning
	alpha = std::max(alpha, lose_score(cur_ply));
	beta = std::min(beta, win_score(cur_ply + 1));
	if (alpha >= beta)
		return alpha;
	// Use transposition table
	bool pv_search = true;
	int16_t old_alpha = alpha, best_score = lose_score(cur_ply);
	std::vector<Move> moves;
	size_t killers_last, move_idx = 0;
	auto tt_it = _transtable[TURN - WHITE].find(get_hash());
	if (tt_it != _transtable[TURN - WHITE].end())
	{
		if (tt_it->second.depth >= depth)
			switch (tt_it->second.bound_type)
			{
			case TTBOUND_EXACT:
				return value_from_tt(tt_it->second, cur_ply);
			case TTBOUND_LOWER:
				alpha = std::max(alpha, value_from_tt(tt_it->second, cur_ply));
				break;
			case TTBOUND_UPPER:
				beta = std::min(beta, value_from_tt(tt_it->second, cur_ply));
				break;
			}
		// EXPERIMENTAL AUTHOR's HEURISTIC(Lesser transposition pruning)
		else if (tt_it->second.depth + 2 >= depth)
		{
			if (tt_it->second.bound_type == TTBOUND_LOWER || tt_it->second.bound_type == TTBOUND_EXACT)
				alpha = std::max<int16_t>(alpha, value_from_tt(tt_it->second, cur_ply) - LT_PRUNING_MARGIN);
			if (tt_it->second.bound_type == TTBOUND_UPPER || tt_it->second.bound_type == TTBOUND_EXACT)
				beta = std::min<int16_t>(beta, value_from_tt(tt_it->second, cur_ply) + LT_PRUNING_MARGIN);
		}
		if (alpha >= beta)
			return alpha;
		killers_last = 1;
		// Get all moves for current position
		get_all_moves<TURN>(moves);
		for (auto& cur_move : moves)
			if (cur_move.old_pos() == tt_it->second.best_move_from
				&& cur_move.new_pos() == tt_it->second.best_move_to)
			{
				swap(moves.front(), cur_move);
				break;
			}
	}
	else
	{
		killers_last = 0;
		// Get all moves for current position
		get_all_moves<TURN>(moves);
		if (moves.empty())
			return no_moves_score(cur_ply);
	}
	// Killer heuristic
	for (const auto& killer : killers[cur_ply])
		for (size_t i = killers_last; i < moves.size(); ++i)
			if (moves[i] == killer)
			{
				swap(moves[i], moves[killers_last++]);
				break;
			}
	// History heuristic
	sort(moves.begin() + killers_last, moves.end(), [&](const Move& lhs, const Move& rhs) {
		return _history_move_score(lhs) > _history_move_score(rhs);
	});
	// Principal variation search
	auto best_move = moves.end();
	for (++cur_ply; move_idx < moves.size(); ++move_idx)
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
		if (_score > best_score)
			best_score = _score, best_move = moves.begin() + move_idx;
		if (_score > alpha)
			alpha = _score, pv_search = false; // pv_search = false here?
		if (alpha >= beta)
		{
			// History heuristic
			if (depth >= MIN_HISTORY_MAKE_DEPTH[search_depth])
				history[pos_idx(cur_move.old_pos())][pos_idx(cur_move.new_pos())] += HISTORY_VALUE[depth];
			// Killer heuristic
			if (move_idx >= killers_last) // Update killers if current move is not in killers
			{
				killers[cur_ply].push_front(cur_move);
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
			_transtable[TURN - WHITE][get_hash()] : tt_it->second);
		entry.value = value_to_tt(alpha, cur_ply);
		entry.depth = depth;
		entry.best_move_from = best_move->old_pos();
		entry.best_move_to = best_move->new_pos();
		if (alpha <= old_alpha)
			entry.bound_type = TTBOUND_UPPER;
		else if (alpha < beta)
			entry.bound_type = TTBOUND_EXACT;
		else
			entry.bound_type = TTBOUND_LOWER;
	}
	return alpha;
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

void Checkers::load_board(std::istream& istr)
{
	std::string str;
	bool mis, wt;
	istr >> str;
	if (str == "NORMAL_GAME")
		mis = false;
	else if (str == "MISERE_GAME")
		mis = true;
	else
		throw(checkers_error("Game type should be either NORMAL_GAME or MISERE_GAME"));
	istr >> str;
	if (str == "WHITE_TURN")
		wt = true;
	else if (str == "BLACK_TURN")
		wt = false;
	else
		throw(checkers_error("Turn must be either WHITE_TURN or BLACK_TURN"));
	restart(mis, wt);
	_clear_board();
	Piece piece;
	Position pos;
	for (int entry = 1; istr >> str; ++entry) try
	{
		if (str == "WS")
			piece = Piece(WHITE_SIMPLE);
		else if (str == "WQ")
			piece = Piece(WHITE_QUEEN);
		else if (str == "BS")
			piece = Piece(BLACK_SIMPLE);
		else if (str == "BQ")
			piece = Piece(BLACK_QUEEN);
		else
			throw(checkers_error("Piece is wrong"));
		if (!read_pos(istr, pos))
			throw(checkers_error("Position is absent"));
		_put_piece(pos, piece);
	}
	catch (const checkers_error& err)
	{
		restart();
		throw(checkers_error("Error in entry " +
			std::to_string(entry) + ": " + err.what()));
	}
	_update_game_state();
	_update_possible_moves();
}

void Checkers::save_board(std::ostream& ostr) const
{
	ostr << (misere ? "MISERE_GAME " : "NORMAL_GAME ")
		<< (white_turn ? "WHITE_TURN\n" : "BLACK_TURN\n");
	for (auto pt : { WHITE_SIMPLE, WHITE_QUEEN, BLACK_SIMPLE, BLACK_QUEEN })
		for (int i = 0; i < piece_count[pt]; ++i)
		{
			ostr << (get_colour(pt) == WHITE ? "W" : "B")
				<< (is_queen(pt) ? "Q" : "S") << ' ';
			write_pos(ostr, piece_list[pt][i]);
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