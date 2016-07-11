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

// constants.h, version 1.4

#pragma once
#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#define CONSTEXPR constexpr

CONSTEXPR const char* CLASS_NAME = "Checkers";
CONSTEXPR const char* MAIN_TITLE = "Checkers";
CONSTEXPR const char* LAST_GAME_FILE = "last_game.txt";
CONSTEXPR int MAIN_WIDTH = 480;
CONSTEXPR int MAIN_HEIGHT = 520;
CONSTEXPR int D_PIECE = 1;
CONSTEXPR int D_QUEEN = 15;
CONSTEXPR int D_ROW = 15;
CONSTEXPR int D_COLUMN = 15;
CONSTEXPR int D_TURN = 20;
CONSTEXPR int D_SELECTED = 2;
CONSTEXPR int D_POSSIBLE = 2;
CONSTEXPR int SELECTED_PEN_WIDTH = 2;
CONSTEXPR int POSSIBLE_PEN_WIDTH = 2;
CONSTEXPR int CELL_SIZE = 50;
CONSTEXPR int SELECTED_SIZE = CELL_SIZE - 2 * D_SELECTED;
CONSTEXPR int POSSIBLE_SIZE = CELL_SIZE - 2 * D_POSSIBLE;
CONSTEXPR int PIECE_SIZE = CELL_SIZE - 2 * D_PIECE;
CONSTEXPR int CROWN_SIZE = CELL_SIZE - 2 * D_QUEEN;
CONSTEXPR int BOARD_LEFT = 30;
CONSTEXPR int BOARD_TOP = 30;
CONSTEXPR int BOARD_RIGHT = BOARD_LEFT + 8 * CELL_SIZE;
CONSTEXPR int BOARD_BOTTOM = BOARD_TOP + 8 * CELL_SIZE;
CONSTEXPR int STEP_SLEEP_TIME = 250;
CONST Gdiplus::Color C_LTGRAY = Gdiplus::Color(192, 192, 192);
CONST Gdiplus::Color C_GRAY = Gdiplus::Color(160, 160, 160);
CONST Gdiplus::Color C_BROWN = Gdiplus::Color(139, 69, 19);
CONST Gdiplus::Color C_BLACK = Gdiplus::Color(0, 0, 0);
CONST Gdiplus::Color C_GREEN = Gdiplus::Color(20, 255, 20);
CONST Gdiplus::Color C_BLUE = Gdiplus::Color(20, 20, 255);
CONST Gdiplus::Color C_YELLOW = Gdiplus::Color(216, 216, 20);
CONST Gdiplus::Color C_SHADOW = Gdiplus::Color(87, 77, 162, 49);
#if defined _DEBUG || defined DEBUG
 CONSTEXPR size_t LEVELS_COUNT = 8;
 CONSTEXPR size_t HINT_DEPTH = 8;
#else
 CONSTEXPR size_t LEVELS_COUNT = 16;
 CONSTEXPR size_t HINT_DEPTH = 16;
#endif
CONSTEXPR size_t MAX_OFN_CHARS = 256;
CONSTEXPR size_t LEVELS[LEVELS_COUNT] = {
	1, 2, 3, 4, 5, 6, 7, 8
#if !defined _DEBUG && !defined DEBUG
	, 9, 10, 11, 12, 13, 14, 15, 16
#endif
}; // levels in combo box in new game with cpu dialog box(as numbers)
const char* const COLUMN_LR[8] = {
	"a", "b", "c", "d", "e", "f", "g", "h"
}; // displayed columns from left to right
const char* const ROW_TD[8] = {
	"8", "7", "6", "5", "4", "3", "2", "1"
}; // displayed rows from top to down
const char* const LEVELS_STR[LEVELS_COUNT] = {
	"1", "2", "3", "4", "5", "6", "7", "8"
#if !defined _DEBUG && !defined DEBUG
	, "9", "10", "11", "12", "13", "14", "15", "16"
#endif
}; // levels in combo box in new game with cpu dialog box(as strings)

#endif