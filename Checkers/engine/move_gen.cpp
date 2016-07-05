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

// move_gen.cpp, version 1.5

#include "move_gen.h"
#include "board.h"
#include "misc.h"

template<colour TURN, move_type MT>
void MoveGenDefault::_get_all_moves(std::vector<Move>& vec) const
{
	// Search for capture-moves first
	if (MT != NON_CAPTURE)
	{
		for (int i = 0; i < board.piece_count[turn_simple(TURN)]; ++i)
		{
			const auto& pos = board.piece_list[turn_simple(TURN)][i];
			Move move(Path(1, pos));
			move.set_original(board[pos.row][pos.column]);
			bool capture[8][8] = {};
			tmp_assign<Piece> move_begin(const_cast<Piece&>
				(board[pos.row][pos.column]), Piece()); // Because this position is empty when we move from it
			_find_deep_capture<TURN>(vec, move, pos.row, pos.column, capture);
		}
		for (int i = 0; i < board.piece_count[turn_queen(TURN)]; ++i)
		{
			const auto& pos = board.piece_list[turn_queen(TURN)][i];
			Move move(Path(1, pos));
			move.set_original(board[pos.row][pos.column]);
			bool capture[8][8] = {};
			tmp_assign<Piece> move_begin(const_cast<Piece&>
				(board[pos.row][pos.column]), Piece()); // Because this position is empty when we move from it
			_find_deep_capture_queen<TURN>(vec, move, pos.row, pos.column, capture);
		}
	}
	// Capture-move is mandatory, so we need to check non-capture moves only when we don't have any capture-moves
	if (MT == CAPTURE || !vec.empty())
		return;
	for (int i = 0; i < board.piece_count[turn_simple(TURN)]; ++i)
	{
		const auto& pos = board.piece_list[turn_simple(TURN)][i];
		if (pos.column < 7 && board[TURN == WHITE ? pos.row + 1 : pos.row - 1][pos.column + 1].get_type() == PT_EMPTY)
		{
			Move move(Path(1, pos));
			move.set_original(board[pos.row][pos.column]);
			if (TURN == WHITE)
			{
				move.add_step(Position(pos.row + 1, pos.column + 1));
				move.set_become(pos.row == 6 ? Piece(WHITE_QUEEN) : Piece(WHITE_SIMPLE));
			}
			else
			{
				move.add_step(Position(pos.row - 1, pos.column + 1));
				move.set_become(pos.row == 1 ? Piece(BLACK_QUEEN) : Piece(BLACK_SIMPLE));
			}
			vec.push_back(std::move(move));
		}
		if (pos.column > 0 && board[TURN == WHITE ? pos.row + 1 : pos.row - 1][pos.column - 1].get_type() == PT_EMPTY)
		{
			Move move(Path(1, pos));
			move.set_original(board[pos.row][pos.column]);
			if (TURN == WHITE)
			{
				move.add_step(Position(pos.row + 1, pos.column - 1));
				move.set_become(pos.row == 6 ? Piece(WHITE_QUEEN) : Piece(WHITE_SIMPLE));
			}
			else
			{
				move.add_step(Position(pos.row - 1, pos.column - 1));
				move.set_become(pos.row == 1 ? Piece(BLACK_QUEEN) : Piece(BLACK_SIMPLE));
			}
			vec.push_back(std::move(move));
		}
	}
	for (int i = 0; i < board.piece_count[turn_queen(TURN)]; ++i)
	{
		const auto& pos = board.piece_list[turn_queen(TURN)][i];
		constexpr int dx[4] = { 1, 1, -1, -1 }, dy[4] = { 1, -1, 1, -1 };
		for (size_t dir = 0; dir < 4; ++dir)
			for (int r = pos.row + dy[dir], c = pos.column + dx[dir]; r < 8 && c < 8 && r >= 0 && c >= 0
				&& board[r][c].get_type() == PT_EMPTY; r += dy[dir], c += dx[dir])
			{
				Move move(Path(1, pos));
				move.add_step(Position(r, c));
				move.set_original(board[pos.row][pos.column]);
				move.set_become(TURN == WHITE ? Piece(WHITE_QUEEN) : Piece(BLACK_QUEEN));
				vec.push_back(std::move(move));
			}
	}
}

template<colour TURN>
void MoveGenDefault::_find_deep_capture(std::vector<Move>& vec, Move& move, int8_t row, int8_t column, bool(&captured)[8][8]) const
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
		if (TURN == WHITE ? (r2 == 7) : (r2 == 0)) // We can become queen at this move
		{
			_find_deep_capture_queen<TURN>(vec, move, r2, c2, captured);
			if (old == vec.size()) // If in recursive call we haven't found any move, then 'move' is a final capture and is one of possible captures
			{
				move.set_become(Piece(TURN == WHITE ? WHITE_QUEEN : BLACK_QUEEN));
				vec.push_back(move);
			}
		}
		else
		{
			_find_deep_capture<TURN>(vec, move, r2, c2, captured);
			if (old == vec.size()) // If in recursive call we haven't found any move, then 'move' is a final capture and is one of possible captures
			{
				move.set_become(Piece(TURN == WHITE ? WHITE_SIMPLE : BLACK_SIMPLE));
				vec.push_back(move);
			}
		}
		captured[r1][c1] = false; // For correct work on next cycles we should clear changes to 'captured' and 'move' variables
		move.pop_step();
		move.pop_capture();
	}
}

template<colour TURN>
void MoveGenDefault::_find_deep_capture_queen(std::vector<Move>& vec, Move& move, int8_t row, int8_t column, bool(&captured)[8][8]) const
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
			for (int r2 = r1 + d_row[dir], c2 = c1 + d_column[dir]; r2 < 8 && c2 < 8 && r2 >= 0 && c2 >= 0 &&
				board[r2][c2].get_type() == PT_EMPTY; r2 += d_row[dir], c2 += d_column[dir])
			{
				move.add_step(Position(r2, c2));
				move.add_capture(std::make_pair(Position(r1, c1), board[r1][c1]));
				move.set_become(Piece(TURN == WHITE ? WHITE_QUEEN : BLACK_QUEEN));
				vec.push_back(move);
				move.pop_step();
				move.pop_capture();
			}
		captured[r1][c1] = false;
	}
}

// Explicit template instantiations
template void	MoveGenDefault::_find_deep_capture<WHITE>(std::vector<Move>&, Move&, int8_t, int8_t, bool(&)[8][8]) const;
template void	MoveGenDefault::_find_deep_capture<BLACK>(std::vector<Move>&, Move&, int8_t, int8_t, bool(&)[8][8]) const;
template void	MoveGenDefault::_find_deep_capture_queen<WHITE>(std::vector<Move>&, Move&, int8_t, int8_t, bool(&)[8][8]) const;
template void	MoveGenDefault::_find_deep_capture_queen<BLACK>(std::vector<Move>&, Move&, int8_t, int8_t, bool(&)[8][8]) const;
template void	MoveGenDefault::_get_all_moves<WHITE, ALL>(std::vector<Move>&) const;
template void	MoveGenDefault::_get_all_moves<WHITE, CAPTURE>(std::vector<Move>&) const;
template void	MoveGenDefault::_get_all_moves<WHITE, NON_CAPTURE>(std::vector<Move>&) const;
template void	MoveGenDefault::_get_all_moves<BLACK, ALL>(std::vector<Move>&) const;
template void	MoveGenDefault::_get_all_moves<BLACK, CAPTURE>(std::vector<Move>&) const;
template void	MoveGenDefault::_get_all_moves<BLACK, NON_CAPTURE>(std::vector<Move>&) const;