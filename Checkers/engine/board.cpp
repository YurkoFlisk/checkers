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

// board.cpp, version 1.7

#include "board.h"
#include "misc.h"
#include <random>
#include <sstream>

Board::Board(game_rules rule) noexcept
{
	init_zobrist();
	restart(rule);
}

Board::~Board(void) noexcept = default;

void Board::init_zobrist(void) noexcept
{
	auto gen = std::linear_congruential_engine<uint64_t, 172687568145817, 2949769574,
		0xffffffffffffffff>();
	for (int i = 0; i < 64; ++i)
	{
		zobrist_hash[WHITE_SIMPLE][i] = gen();
		zobrist_hash[WHITE_QUEEN][i] = gen();
		zobrist_hash[BLACK_SIMPLE][i] = gen();
		zobrist_hash[BLACK_QUEEN][i] = gen();
	}
}

void Board::_clear_board(void)
{
	cur_hash = 0;
	all_piece_count = 0;
	for (int i = 0; i < PT_COUNT; ++i)
		piece_count[i] = 0;
	for (int i = 0; i < 8; ++i)
		for (int j = 0; j < 8; ++j)
			board[i][j] = Piece(PT_EMPTY);
}

void Board::restart(game_rules rule, bool mis) noexcept
{
	_clear_board();
	for (int i = 0; i < 8; i += 2)
	{
		_put_piece(Position(0, i), Piece(WHITE_SIMPLE));
		_put_piece(Position(2, i), Piece(WHITE_SIMPLE));
		_put_piece(Position(6, i), Piece(BLACK_SIMPLE));
	}
	for (int i = 1; i < 8; i += 2)
	{
		_put_piece(Position(1, i), Piece(WHITE_SIMPLE));
		_put_piece(Position(5, i), Piece(BLACK_SIMPLE));
		_put_piece(Position(7, i), Piece(BLACK_SIMPLE));
	}
	rules = rule;
	white_turn = true;
	misere = mis;
	cur_ply = 0;
	state = GAME_CONTINUE;
	(decltype(_position_count)()).swap(_position_count);
	consecutiveQM.resize(1);
	consecutiveQM[0] = 0;
	switch (rules)
	{
	case RULES_DEFAULT:
		move_gen = std::make_unique<MoveGenDefault>(*this);
		break;
	case RULES_ENGLISH:
		move_gen = std::make_unique<MoveGenEnglish>(*this);
		break;
	}
}

bool Board::legal_move(Move& move) const
{
	std::vector<Move> moves;
	get_all_moves(moves);
	auto move_it = std::find(moves.begin(), moves.end(), move);
	if (move_it == moves.end())
		return false;
	move.set_info_from(*move_it);
	return true;
}

void Board::_update_game_state(void)
{
	std::vector<Move> vec;
	get_all_moves(vec);
	if (vec.size() == 0)
		state = no_moves_state();
	else
	{
		bool draw = consecutiveQM[cur_ply] >=
			2 * DRAW_CONSECUTIVE_QUEEN_MOVES; // Because consequtiveQM really counts half-moves(plies)
		if(!draw)
			for (const auto& pos : _position_count)
				if (pos.second >= DRAW_REPEATED_POS_COUNT)
				{
					draw = true;
					break;
				}
		state = (draw ? DRAW : GAME_CONTINUE);
	}
}

void Board::_proceed(Move& m)
{
	white_turn = !white_turn;
	++cur_ply;
	++_position_count[get_hash()];
	if (consecutiveQM.size() <= cur_ply)
		consecutiveQM.resize(cur_ply + 1);
	if (m.get_original().is_queen())
		consecutiveQM[cur_ply] = consecutiveQM[cur_ply - 1] + 1;
	else
		consecutiveQM[cur_ply] = 0;
}

void Board::_retreat(Move& m)
{
	white_turn = !white_turn;
	--cur_ply;
	if((--_position_count[get_hash()]) == 0)
		_position_count.erase(get_hash());
}

void Board::_put_piece(Position pos, Piece piece)
{
	++all_piece_count;
	index[pos.get_row()][pos.get_column()] = piece_count[piece.get_type()]++;
	piece_list[piece.get_type()][index[pos.get_row()][pos.get_column()]] = pos;
	board[pos.get_row()][pos.get_column()] = piece;
	cur_hash ^= zobrist_hash[piece.get_type()][pos_idx(pos)];
}

void Board::_remove_piece(Position pos)
{
	Piece& cur = board[pos.get_row()][pos.get_column()];
	cur_hash ^= zobrist_hash[cur.get_type()][pos_idx(pos)];
	Position last_pos = piece_list[cur.get_type()][--piece_count[cur.get_type()]];
	std::swap(piece_list[cur.get_type()][index[pos.get_row()][pos.get_column()]],
		piece_list[cur.get_type()][piece_count[cur.get_type()]]);
	index[last_pos.get_row()][last_pos.get_column()] = index[pos.get_row()][pos.get_column()];
	board[pos.get_row()][pos.get_column()] = Piece(PT_EMPTY);
	--all_piece_count;
}

void Board::_do_move(const Move& move)
{
	_remove_piece(move.old_pos());
	_put_piece(move.new_pos(), move.get_become());
	for (size_t i = 0; i < move.capt_size(); ++i)
		_remove_piece(move.get_captured()[i].first);
}

void Board::_undo_move(const Move& undo)
{
	_remove_piece(undo.new_pos());
	_put_piece(undo.old_pos(), undo.get_original());
	for (size_t i = 0; i < undo.capt_size(); ++i)
		_put_piece(undo.get_captured()[i].first, undo.get_captured()[i].second);
}

bool Board::read_pos(std::istream& istr, Position& pos)
{
	char col;
	int row;
	if (!(istr >> col >> row))
		return false;
	--row, col -= 'a';
	if (row < 0 || row > 7 || col < 0 || col > 7 || (row & 1) != (col & 1))
		throw(checkers_error("Position is illegal"));
	pos.set_row(row);
	pos.set_column(col);
	return true;
}

void Board::write_pos(std::ostream& ostr, Position pos)
{
	ostr << (char)('a' + pos.get_column()) << pos.get_row() + 1;
}

bool Board::read_move(std::istream& istr, Move& move)
{
	char delim = '-';
	Position pos;
	std::string str_move;
	move = Move();
	if (!(istr >> str_move))
		return false;
	std::stringstream ss_move(str_move);
	for (int cur_pos = 1; cur_pos == 1 || (ss_move >> delim); ++cur_pos)
	{
		if (delim != '-' && delim != ':')
			throw(checkers_error("Move has wrong format"));
		try
		{
			read_pos(ss_move, pos);
		}
		catch (const checkers_error& err)
		{
			throw(checkers_error("Position " + std::to_string(cur_pos) + ": " + err.what()));
		}
		move.add_step(pos);
	}
	if (move.size() == 1)
		throw(checkers_error("Move cannot consist of only one position"));
	return true;
}

void Board::write_move(std::ostream& ostr, const Move& move)
{
	if (move.size() == 0)
		return;
	const char delim = (move.capt_size() == 0 ? '-' : ':');
	write_pos(ostr, move[0]);
	for (size_t i = 1; i < move.size(); ++i)
	{
		ostr << delim;
		write_pos(ostr, move[i]);
	}
}