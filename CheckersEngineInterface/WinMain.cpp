// Checkers engine interface
// Used by CheckersEngTester for accessing different version of engine with the same interface
// Copyright (c) 2016-2017 Yurko Prokopets (aka YurkoFlisk)
// main.cpp, version 1.7

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fstream>
#include "../Checkers/engine/checkers.h"

// User messages
#define CM_MOVE WM_USER
#define CM_CPUMOVE WM_USER + 1
#define CM_SETDEPTH WM_USER + 2
#define CM_SETTIMELIMIT WM_USER + 3

// Constants
const char* CLASS_NAME = "Checkers engine interface";

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	MSG msg;
	WNDCLASSEX wndClass = {};
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.hInstance = hInstance;
	wndClass.lpfnWndProc = WndProc;
	wndClass.lpszClassName = CLASS_NAME;
	if(!RegisterClassEx(&wndClass))
		return FALSE;
	HWND hWnd = CreateWindow(CLASS_NAME, NULL, NULL, NULL, NULL, NULL, NULL,
		HWND_MESSAGE, NULL, hInstance, NULL);
	if (!hWnd)
		return FALSE;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

// Callback function for handling messages from CheckersEngTester
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static Checkers game;
	static Move move;
	static int8_t depth;
	static std::ifstream in;
	static std::ofstream out;
	switch (msg)
	{
	case CM_MOVE:
		in.open("text.txt");
		Board::read_move(in, move);
		in.close();
		game.move(move);
		return game.get_state();
	case CM_CPUMOVE:
		depth = game.get_computer_move(move);
		game.move(move);
		out.open("text.txt");
		Board::write_move(out, move);
		out.close();
		return depth;
	case CM_SETDEPTH:
		game.set_search_depth(wParam);
		break;
	case CM_SETTIMELIMIT:
		game.set_time_limit(wParam);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}