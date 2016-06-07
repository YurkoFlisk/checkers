// Checkers
// By Yurko Prokopets(aka YurkoFlisk)
// constants.h
// Version 1.3

#pragma once
#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#define CONSTEXPR constexpr

CONSTEXPR const char* CLASS_NAME = "Checkers";
CONSTEXPR const char* MAIN_TITLE = "Checkers";
CONSTEXPR const char* LAST_GAME_FILE = "last_game.txt";
CONSTEXPR size_t MAIN_WIDTH = 480;
CONSTEXPR size_t MAIN_HEIGHT = 520;
CONSTEXPR size_t CELL_SIZE = 50;
CONSTEXPR size_t BOARD_LEFT = 30;
CONSTEXPR size_t BOARD_TOP = 30;
CONSTEXPR size_t BOARD_RIGHT = BOARD_LEFT + 8 * CELL_SIZE;
CONSTEXPR size_t BOARD_BOTTOM = BOARD_TOP + 8 * CELL_SIZE;
CONSTEXPR size_t D_QUEEN = 15;
CONSTEXPR size_t D_ROW = 15;
CONSTEXPR size_t D_COLUMN = 15;
CONSTEXPR size_t D_TURN = 20;
CONSTEXPR size_t STEP_SLEEP_TIME = 250;
#if defined _DEBUG || defined DEBUG
 CONSTEXPR size_t LEVELS_COUNT = 8;
 CONSTEXPR size_t HINT_DEPTH = 8;
#else
 CONSTEXPR size_t LEVELS_COUNT = 12;
 CONSTEXPR size_t HINT_DEPTH = 12;
#endif
CONSTEXPR size_t MAX_OFN_CHARS = 256;
CONSTEXPR size_t LEVELS[LEVELS_COUNT] = {
	1, 2, 3, 4, 5, 6, 7, 8
#if !defined _DEBUG && !defined DEBUG
	, 9, 10, 11, 12
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
	, "9", "10", "11", "12"
#endif
}; // levels in combo box in new game with cpu dialog box(as strings)

#endif