// Checkers benchmarker
// Launches computer vs computer game and writes it's log to ai_log.txt
// Copyright(c) 2016-2017 Yurko Prokopets(aka YurkoFlisk)
// main.cpp, version 1.7

#include "engine/checkers.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>

using namespace std;
constexpr const char* AI_LOG_FILE = "ai_log.txt";
Checkers white_ai, black_ai;

int main(void)
{
	int white_level, black_level, timer, overall_time(0), game_length(0), depth;
	Move move;
	float limit;
	cout << "Enter time limit(in ms): ";
	cin >> limit;
	cout << "Enter white level: ";
	cin >> white_level;
	cout << "Enter black level: ";
	cin >> black_level;
	white_ai.set_time_limit(limit);
	black_ai.set_time_limit(limit);
	white_ai.set_search_depth(white_level);
	black_ai.set_search_depth(black_level);
	std::ofstream ai_log(AI_LOG_FILE);
	stringstream log_line;
	for (bool white_turn = true; ; white_turn = !white_turn)
	{
		++game_length;
		auto start_time = std::chrono::high_resolution_clock::now();
		depth = (white_turn ? white_ai : black_ai).get_computer_move(move);
		auto end_time = std::chrono::high_resolution_clock::now();
		overall_time += (timer = std::chrono::duration_cast<
			std::chrono::milliseconds>(end_time - start_time).count());
		white_ai.move(move);
		black_ai.move(move);
		log_line << (white_turn ? "White" : "Black") << " move ";
		Checkers::write_move(log_line, move);
		log_line << ". " << timer << " ms elapsed. Searched to depth " << depth << ".\n";
		if (white_ai.get_state() != GAME_CONTINUE)
		{
			if (white_ai.get_state() == DRAW)
				log_line << "DRAW!\n";
			else
				log_line << (white_ai.get_state() == WHITE_WIN ? "WHITE WON!" : "BLACK WON!") << '\n';
			break;
		}
		cout << log_line.str();
		ai_log << log_line.str();
		stringstream().swap(log_line);
	}
	log_line << "Game length: " << game_length << " plies\n";
	log_line << "Overall time elapsed: " << overall_time << " ms\n";
	log_line << "Average thinking time: " << (float)(overall_time) / game_length << " ms\n";
	cout << log_line.str();
	ai_log << log_line.str();
	ai_log.close();
	return 0;
}