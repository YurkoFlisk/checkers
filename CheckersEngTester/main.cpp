// Checkers tester
// Launches engine vs engine game for automatical testing
// different versions of engine against each other
// Copyright(c) 2016 Yurko Prokopets(aka YurkoFlisk)
// main.cpp, version 1.6

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include "../Checkers/engine/board.h"
#define CM_MOVE WM_USER
#define CM_CPUMOVE WM_USER + 1
#define CM_SETDEPTH WM_USER + 2

using namespace std;

DWORD whitePID, blackPID, pID;
HWND hWndFound = NULL, hWndMsgWhite = NULL, hWndMsgBlack = NULL;
HANDLE hWhite, hBlack;

BOOL CloseHandler(DWORD dwCtrlType)
{
	if (dwCtrlType == CTRL_CLOSE_EVENT)
	{
		TerminateProcess(hWhite, 0);
		TerminateProcess(hBlack, 0);
	}
	return TRUE;
}

int main(void)
{
	game_state gs;
	Move move;
	string eng_white, eng_black;
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CloseHandler, TRUE);
	cout << "Enter white engine: ";
	getline(cin, eng_white);
	cout << "Enter black engine: ";
	getline(cin, eng_black);
	PROCESS_INFORMATION pi;
	STARTUPINFO si = {};
	si.cb = sizeof(si);
	if (!CreateProcess(eng_white.data(), NULL, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi))
		return 0;
	whitePID = pi.dwThreadId, hWhite = pi.hProcess;
	if (!CreateProcess(eng_black.data(), NULL, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi))
	{
		TerminateProcess(hWhite, 0);
		return 0;
	}
	blackPID = pi.dwThreadId, hBlack = pi.hProcess;
	WaitForInputIdle(hWhite, INFINITE);
	WaitForInputIdle(hBlack, INFINITE);
	while (hWndFound = FindWindowEx(HWND_MESSAGE, hWndFound, "Checkers engine interface", NULL))
	{
		pID = GetWindowThreadProcessId(hWndFound, NULL);
		if (pID == whitePID)
			hWndMsgWhite = hWndFound;
		else if (pID == blackPID)
			hWndMsgBlack = hWndFound;
	}
	if (!hWndMsgWhite || !hWndMsgBlack)
	{
		TerminateProcess(hWhite, 0);
		TerminateProcess(hBlack, 0);
		return 0;
	}
	int wDepth, bDepth, timer;
	cout << "Enter white depth: ";
	cin >> wDepth;
	cout << "Enter black depth: ";
	cin >> bDepth;
	SendMessage(hWndMsgWhite, CM_SETDEPTH, wDepth, NULL);
	SendMessage(hWndMsgBlack, CM_SETDEPTH, bDepth, NULL);
	std::ofstream log("cet_log.txt");
	for (bool white_turn = true; ; white_turn = !white_turn)
	{
		auto start = chrono::high_resolution_clock::now();
		SendMessage(white_turn ? hWndMsgWhite : hWndMsgBlack, CM_CPUMOVE, NULL, NULL);
		auto end = chrono::high_resolution_clock::now();
		timer = chrono::duration_cast<chrono::milliseconds>(end - start).count();
		std::ifstream in("text.txt");
		Board::read_move(in, move);
		in.close();
		gs = (game_state)(SendMessage(white_turn ? hWndMsgBlack : hWndMsgWhite, CM_MOVE, NULL, NULL));
		cout << (white_turn ? "White move: " : "Black move: ");
		Board::write_move(cout, move);
		cout << ' ' << timer << "ms elapsed\n";
		Board::write_move(log, move);
		log << '\n';
		if (gs != GAME_CONTINUE)
		{
			cout << (gs == WHITE_WIN ? "White won this game!\n" :
				gs == BLACK_WIN ? "Black won this game!\n" :
				"Draw!\n");
			SendMessage(hWndMsgWhite, WM_CLOSE, NULL, NULL);
			SendMessage(hWndMsgBlack, WM_CLOSE, NULL, NULL);
			break;
		}
	}
	log.close();
	system("pause");
	return 0;
}