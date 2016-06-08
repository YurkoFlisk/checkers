// Checkers benchmarker
// Launches computer vs computer game and writes it's log to ai_log.txt
// Copyright(c) 2016 Yurko Prokopets(aka YurkoFlisk)
// main.cpp, version 1.3

#include "..\Checkers\checkers.h"
#include <iostream>
#include <chrono>

using namespace std;
CONSTEXPR const char* AI_LOG_FILE = "ai_log.txt";

int main(void)
{
	Checkers white_ai, black_ai;
	int white_level, black_level;
	Move move;
	cout << "Enter white level: ";
	cin >> white_level;
	cout << "Enter black level: ";
	cin >> black_level;
	white_ai.set_search_depth(white_level);
	black_ai.set_search_depth(black_level);
	std::ofstream ai_log(AI_LOG_FILE);
	for (bool white_turn = true; ; white_turn = !white_turn)
	{
		auto start_time = std::chrono::high_resolution_clock::now();
		(white_turn ? white_ai : black_ai).get_computer_move(move);
		auto end_time = std::chrono::high_resolution_clock::now();
		white_ai.move(move);
		black_ai.move(move);
		ai_log << (white_turn ? "White" : "Black") << " move ";
		Checkers::write_move(ai_log, move);
		ai_log << ". " << std::chrono::duration_cast<
			std::chrono::milliseconds>(end_time - start_time).count() << "ms elapsed\n";
		if (white_ai.get_state() != GAME_CONTINUE)
		{
			if (white_ai.get_state() == DRAW)
				ai_log << "DRAW!\n";
			else
				ai_log << (white_ai.get_state() == WHITE_WIN ? "WHITE WON!" : "BLACK WON!") << '\n';
			break;
		}
	}
	ai_log.close();
	return 0;
}
