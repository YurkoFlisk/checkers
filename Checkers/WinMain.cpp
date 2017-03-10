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

// WinMain.cpp, version 1.5

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <algorithm>
namespace Gdiplus
{
	using std::min;
	using std::max;
}
#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <fstream>
#include <future>
#include <mutex>
#include "targetver.h"
#include "resource.h"
#include "engine/checkers.h"
#if defined _DEBUG || defined DEBUG // Memory leak detection
 #define _CRTDBG_MAP_ALLOC
 #include <cstdlib>
 #include <crtdbg.h>
#endif
#include <objidl.h>
#include <gdiplus.h>
#include "constants.h"
using namespace Gdiplus;
// Messages
#define CM_CPUMOVE WM_USER // Checkers' message - computer move
// Global variables
HINSTANCE hInst; // program's instance
RECT updateRect;
HDC hDC, hDCMem;
Checkers checkers; // game logic
std::ifstream in; // for file operations
std::ofstream out; // ..
bool pvp; // true if a game is pvp(player versus player), false if it is with computer
bool board_whitedown; // true if board is turned so that 1-st row is lower than 8-th, false otherwise
bool player_white; // true if player plays white(unused if pvp == true)
bool repaint_board; // true if we need to repaint board in a next processing of WM_PAINT message
bool computers_move; // indicates whether ai is thinking at the moment
int d_ycenter; // distance between base line and y-center of the font
int selected_x, selected_y; // coordinates of selected piece(both -1 if no one is selected)
std::vector<Position> selected_part_moves; // possible part moves for selected piece
// Pens
Pen* blackPen;
Pen* selectedPen;
Pen* possiblePen;
// Brushes
HBRUSH hBackground = (HBRUSH)(COLOR_WINDOW + 1);
SolidBrush* whiteCellBrush;
SolidBrush* blackCellBrush;
SolidBrush* whitePieceBrush;
SolidBrush* blackPieceBrush;
SolidBrush* shadowPieceBrush;
SolidBrush* crownBrush;
// Forward declarations
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
bool CreateMainWindow(HINSTANCE, int);
void FinishGame(HWND);
void GetPositionRect(const Position&, RECT*);
void InvalidatePos(HWND hWnd, const Position& pos);
void OnPaint(HDC);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgNewGame(HWND, UINT, WPARAM, LPARAM);
// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
#if defined _DEBUG || defined DEBUG // Memory leak detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	MSG msg;
	HACCEL hAccelTable;
	GdiplusStartupInput gpStartupInput;
	ULONG_PTR gpToken;
	GdiplusStartup(&gpToken, &gpStartupInput, NULL);
	if (!CreateMainWindow(hInstance, nCmdShow))
		return FALSE;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHECKERS));
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	GdiplusShutdown(gpToken);
	return msg.wParam;
}
// Function for creating main window
bool CreateMainWindow(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wndClass;
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.hInstance = hInst = hInstance;
	wndClass.cbClsExtra = wndClass.cbWndExtra = 0;
	wndClass.lpfnWndProc = WndProc;
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpszClassName = CLASS_NAME;
	wndClass.lpszMenuName = MAKEINTRESOURCE(IDC_CHECKERS);
	wndClass.hbrBackground = hBackground;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
	wndClass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
	if (!RegisterClassEx(&wndClass))
		return false;
	HWND hWnd = CreateWindow(CLASS_NAME, MAIN_TITLE, WS_OVERLAPPEDWINDOW & (~WS_THICKFRAME),
		CW_USEDEFAULT, CW_USEDEFAULT, MAIN_WIDTH, MAIN_HEIGHT, NULL, NULL, hInst, NULL);
	if (!hWnd)
		return false;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return true;
}
// Function for getting bounding rect of given position
void GetPositionRect(const Position& pos, RECT* rect)
{
	if(board_whitedown)
		rect->left = BOARD_LEFT + pos.get_column()*CELL_SIZE,
		rect->top = BOARD_BOTTOM - (pos.get_row() + 1)*CELL_SIZE;
	else
		rect->left = BOARD_RIGHT - (pos.get_column() + 1)*CELL_SIZE,
		rect->top = BOARD_TOP + pos.get_row()*CELL_SIZE;
	rect->right = rect->left + CELL_SIZE, rect->bottom = rect->top + CELL_SIZE;
}
// Invalidating given position
void InvalidatePos(HWND hWnd, const Position& pos)
{
	GetPositionRect(pos, &updateRect);
	InvalidateRect(hWnd, &updateRect, FALSE);
}
// Function for finishing game
void FinishGame(HWND hWnd)
{
	out.open(LAST_GAME_FILE);
	checkers.save_game(out);
	out.close();
	if (checkers.get_state() == DRAW)
		MessageBox(hWnd, "Draw!", "Game result", MB_ICONINFORMATION);
	else
		MessageBox(hWnd, checkers.get_state() == WHITE_WIN ? "White won this game!" :
			"Black won this game!", "Game result", MB_ICONINFORMATION);
}
// Function for drawing
void OnPaint(HDC hDC)
{
	if (computers_move)
		return;
	Graphics graphics(hDC);
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);
	if (repaint_board)
	{
		updateRect.left = updateRect.top = 0, updateRect.right = MAIN_WIDTH, updateRect.bottom = MAIN_HEIGHT;
		FillRect(hDCMem, &updateRect, hBackground);
	}
	if (checkers.current_turn_colour() == WHITE)
		graphics.FillEllipse(whitePieceBrush, BOARD_RIGHT, BOARD_TOP - D_TURN, D_TURN, D_TURN);
	else
		graphics.FillEllipse(blackPieceBrush, BOARD_RIGHT, BOARD_TOP - D_TURN, D_TURN, D_TURN);
	if (repaint_board)
	{
		SetTextAlign(hDC, TA_BASELINE | TA_CENTER);
		for (int i = 0; i < 8; ++i)
			TextOut(hDC, BOARD_LEFT + i*CELL_SIZE + CELL_SIZE / 2, BOARD_TOP - D_COLUMN + d_ycenter, COLUMN_LR[board_whitedown ? i : 7 - i], 1);
		for (int i = 0; i < 8; ++i)
			TextOut(hDC, BOARD_LEFT - D_ROW, BOARD_TOP + i*CELL_SIZE + CELL_SIZE / 2 + d_ycenter, ROW_TD[board_whitedown ? i : 7 - i], 1);
		for (int x = BOARD_LEFT, n = 0; x < BOARD_RIGHT; x += CELL_SIZE, ++n)
			for (int y = BOARD_TOP + CELL_SIZE*(n & 1); y < BOARD_BOTTOM; y += 2 * CELL_SIZE)
			{
				graphics.DrawRectangle(blackPen, x, y, CELL_SIZE, CELL_SIZE);
				graphics.FillRectangle(whiteCellBrush, x, y, CELL_SIZE, CELL_SIZE);
			}
		repaint_board = false;
	}
	for (int x = BOARD_LEFT, n = 0; x < BOARD_RIGHT; x += CELL_SIZE, ++n)
		for (int y = BOARD_TOP + CELL_SIZE*(1 - (n & 1)); y < BOARD_BOTTOM; y += 2 * CELL_SIZE)
		{
			graphics.DrawRectangle(blackPen, x, y, CELL_SIZE, CELL_SIZE);
			graphics.FillRectangle(blackCellBrush, x, y, CELL_SIZE, CELL_SIZE);
		}
	for (int i = 0; i < 8; ++i)
		for (int j = i & 1; j < 8; j += 2)
		{
			if (checkers[i][j].get_colour() == SHADOW)
				if (board_whitedown)
					graphics.FillEllipse(shadowPieceBrush, BOARD_LEFT + j*CELL_SIZE + D_PIECE, BOARD_BOTTOM - (i + 1)*CELL_SIZE + D_PIECE,
						PIECE_SIZE, PIECE_SIZE);
				else
					graphics.FillEllipse(shadowPieceBrush, BOARD_RIGHT - (j + 1)*CELL_SIZE + D_PIECE, BOARD_TOP + i*CELL_SIZE + D_PIECE,
						PIECE_SIZE, PIECE_SIZE);
			else if (checkers[i][j].get_colour() == WHITE)
				if (board_whitedown)
					graphics.FillEllipse(whitePieceBrush, BOARD_LEFT + j*CELL_SIZE + D_PIECE, BOARD_BOTTOM - (i + 1)*CELL_SIZE + D_PIECE,
						PIECE_SIZE, PIECE_SIZE);
				else
					graphics.FillEllipse(whitePieceBrush, BOARD_RIGHT - (j + 1)*CELL_SIZE + D_PIECE, BOARD_TOP + i*CELL_SIZE + D_PIECE,
						PIECE_SIZE, PIECE_SIZE);
			else if (checkers[i][j].get_colour() == BLACK)
				if (board_whitedown)
					graphics.FillEllipse(blackPieceBrush, BOARD_LEFT + j*CELL_SIZE + D_PIECE, BOARD_BOTTOM - (i + 1)*CELL_SIZE + D_PIECE,
						PIECE_SIZE, PIECE_SIZE);
				else
					graphics.FillEllipse(blackPieceBrush, BOARD_RIGHT - (j + 1)*CELL_SIZE + D_PIECE, BOARD_TOP + i*CELL_SIZE + D_PIECE,
						PIECE_SIZE, PIECE_SIZE);
			if (checkers[i][j].is_queen())
				if (board_whitedown)
					graphics.FillEllipse(crownBrush, BOARD_LEFT + j*CELL_SIZE + D_QUEEN, BOARD_BOTTOM - (i + 1)*CELL_SIZE + D_QUEEN,
						CROWN_SIZE, CROWN_SIZE);
				else
					graphics.FillEllipse(crownBrush, BOARD_RIGHT - (j + 1)*CELL_SIZE + D_QUEEN, BOARD_TOP + i*CELL_SIZE + D_QUEEN,
						CROWN_SIZE, CROWN_SIZE);
		}
	if (selected_x != -1)
	{
		if (board_whitedown)
			graphics.DrawEllipse(selectedPen, BOARD_LEFT + selected_x*CELL_SIZE + D_SELECTED,
				BOARD_BOTTOM - (selected_y + 1)*CELL_SIZE + D_SELECTED, SELECTED_SIZE, SELECTED_SIZE);
		else
			graphics.DrawEllipse(selectedPen, BOARD_RIGHT - (selected_x + 1)*CELL_SIZE + D_SELECTED,
				BOARD_TOP + selected_y*CELL_SIZE + D_SELECTED, SELECTED_SIZE, SELECTED_SIZE);
		for (const auto& pos : selected_part_moves)
			if (board_whitedown)
				graphics.DrawEllipse(possiblePen, BOARD_LEFT + pos.get_column()*CELL_SIZE + D_POSSIBLE,
					BOARD_BOTTOM - (pos.get_row() + 1)*CELL_SIZE + D_POSSIBLE, POSSIBLE_SIZE, POSSIBLE_SIZE);
			else
				graphics.DrawEllipse(possiblePen, BOARD_RIGHT - (pos.get_column() + 1)*CELL_SIZE + D_POSSIBLE,
					BOARD_TOP + pos.get_row()*CELL_SIZE + D_POSSIBLE, POSSIBLE_SIZE, POSSIBLE_SIZE);
	}
}
// Callback function for main window
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static char ofn_buffer[MAX_OFN_CHARS];
	static OPENFILENAMEA ofn;
	static TEXTMETRIC tm;
	static HBITMAP hBMMem;
	static HANDLE hOld;
	static PAINTSTRUCT ps;
	static int wmId, wmEvent;
	static size_t tmp_x, tmp_y, x_pos, y_pos;
	static step_result pm_result;
	static std::vector<Position> cur_move_used_pos;
	static Move cpuMove; // shared between two threads
	static std::mutex mut;
	static std::future<void> fut;
	static const auto ai_move = [](HWND hWnd) { // function for ai thread
		std::lock_guard<std::mutex> lock(mut);
		checkers.get_computer_move(cpuMove);
		PostMessage(hWnd, CM_CPUMOVE, NULL, NULL);
	};
	static const auto ai_hint_move = [](HWND hWnd) { // function for ai thread
		std::lock_guard<std::mutex> lock(mut);
		int prev_depth = checkers.get_search_depth();
		checkers.set_search_depth(HINT_DEPTH);
		checkers.get_computer_move(cpuMove);
		checkers.set_search_depth(prev_depth);
		PostMessage(hWnd, CM_CPUMOVE, NULL, NULL);
	};
	switch (msg)
	{
	case WM_CREATE:
		ofn.lStructSize = sizeof(ofn);
		ofn.nMaxFile = MAX_OFN_CHARS;
		ofn.lpstrFile = ofn_buffer;
		ofn.lpstrFilter = "All files\0*.*";
		ofn.hInstance = hInst;
		ofn.lpstrInitialDir = ".\\";
		blackPen = new Pen(C_BLACK);
		selectedPen = new Pen(C_YELLOW, SELECTED_PEN_WIDTH);
		possiblePen = new Pen(C_BLUE, POSSIBLE_PEN_WIDTH);
		whiteCellBrush = new SolidBrush(C_LTGRAY);
		blackCellBrush = new SolidBrush(C_BROWN);
		whitePieceBrush = new SolidBrush(C_GRAY);
		blackPieceBrush = new SolidBrush(C_BLACK);
		shadowPieceBrush = new SolidBrush(C_SHADOW);
		crownBrush = new SolidBrush(C_GREEN);
		hDC = GetDC(hWnd);
		hDCMem = CreateCompatibleDC(hDC);
		hBMMem = CreateCompatibleBitmap(hDC, MAIN_WIDTH, MAIN_HEIGHT);
		hOld = SelectObject(hDCMem, hBMMem);
		GetTextMetrics(hDC, &tm);
		d_ycenter = (tm.tmAscent - tm.tmDescent) / 2;
		player_white = true;
		pvp = false;
		board_whitedown = true;
		repaint_board = true;
		computers_move = false;
		selected_x = selected_y = -1;
		break;
	case WM_MOVE:
		repaint_board = true;
		break;
	case WM_PAINT:
		updateRect.left = BOARD_RIGHT, updateRect.top = BOARD_TOP - D_TURN,
			updateRect.right = BOARD_RIGHT + D_TURN, updateRect.bottom = BOARD_TOP;
		InvalidateRect(hWnd, &updateRect, FALSE);
		if (repaint_board)
			InvalidateRect(hWnd, NULL, FALSE);
		hDC = BeginPaint(hWnd, &ps);
		OnPaint(hDCMem);
		BitBlt(hDC, 0, 0, MAIN_WIDTH, MAIN_HEIGHT, hDCMem, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		break;
	case WM_LBUTTONDOWN:
		if (computers_move)
			break; // Do not react on attempts to move while ai is thinking
		tmp_x = GET_X_LPARAM(lParam);
		tmp_y = GET_Y_LPARAM(lParam);
		if (tmp_x < BOARD_LEFT || tmp_x >= BOARD_RIGHT || tmp_y <= BOARD_TOP || tmp_y > BOARD_BOTTOM)
			break;
		if (board_whitedown)
			x_pos = (tmp_x - BOARD_LEFT) / CELL_SIZE, y_pos = (BOARD_BOTTOM - tmp_y) / CELL_SIZE;
		else
			x_pos = 7 - (tmp_x - BOARD_LEFT) / CELL_SIZE, y_pos = 7 - (BOARD_BOTTOM - tmp_y) / CELL_SIZE;
		for (const auto& pos : selected_part_moves)
			InvalidatePos(hWnd, pos);
		selected_part_moves.clear();
		pm_result = checkers.step(Position(y_pos, x_pos));
		if (pm_result == STEP_FINISH)
		{
			cur_move_used_pos.clear();
			InvalidatePos(hWnd, Position(selected_y, selected_x));
			InvalidatePos(hWnd, Position(y_pos, x_pos));
			for(const auto& capt : checkers.get_last_move().get_captured())
				InvalidatePos(hWnd, capt.first);
			selected_x = selected_y = -1;
			UpdateWindow(hWnd);
			if (!pvp && checkers.get_state() == GAME_CONTINUE)
			{
				computers_move = true; // From this moment separate thread for ai is launched
				fut = std::async(ai_move, hWnd);
			}
			else if (checkers.get_state() != GAME_CONTINUE)
				FinishGame(hWnd);
		}
		else if (checkers[y_pos][x_pos].get_colour() == checkers.current_turn_colour())
		{
			cur_move_used_pos.push_back(Position(selected_y, selected_x));
			InvalidatePos(hWnd, cur_move_used_pos.back());
			if (!(checkers.get_part_move().capt_size() == 0))
			{
				cur_move_used_pos.push_back(checkers.get_part_move().get_last_captured().first);
				InvalidatePos(hWnd, cur_move_used_pos.back());
			}
			else if (pm_result == STEP_ILLEGAL_NEW)
			{
				for (auto pos : cur_move_used_pos)
					InvalidatePos(hWnd, pos);
				cur_move_used_pos.clear();
			}
			InvalidatePos(hWnd, Position(y_pos, x_pos));
			selected_x = x_pos, selected_y = y_pos;
			if (pm_result != STEP_ILLEGAL)
				for (const auto& move : checkers.get_part_possible_moves())
				{
					selected_part_moves.push_back(move[checkers.get_part_move_size()]);
					InvalidatePos(hWnd, selected_part_moves.back());
				}
		}
		else if(selected_x != -1)
		{
			for (const auto& pos : cur_move_used_pos)
				InvalidatePos(hWnd, pos);
			InvalidatePos(hWnd, Position(selected_y, selected_x));
			selected_x = selected_y = -1;
		}
		break;
	case CM_CPUMOVE:
		// Performing and animating computer move
		computers_move = false;
		for (size_t i = 0; i < cpuMove.size(); ++i)
		{
			checkers.step(cpuMove[i]);
			if (i > 0)
			{
				InvalidatePos(hWnd, cpuMove[i - 1]);
				InvalidatePos(hWnd, cpuMove[i]);
				if (cpuMove.capt_size())
					InvalidatePos(hWnd, cpuMove.get_captured()[i - 1].first);
				UpdateWindow(hWnd);
				if (i != cpuMove.size() - 1)
					Sleep(STEP_SLEEP_TIME);
			}
		}
		for (const auto& capt : checkers.get_last_move().get_captured())
			InvalidatePos(hWnd, capt.first);
		if (checkers.get_state() != GAME_CONTINUE)
			FinishGame(hWnd);
		else if (!pvp && checkers.get_white_turn() != player_white)
		{
			computers_move = true;
			fut = std::async(ai_move, hWnd);
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDM_NEW_GAME:
			selected_x = selected_y = -1;
			selected_part_moves.clear();
			if (!computers_move)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_NEWGAME), hWnd, DlgNewGame);
			break;
		case IDM_REVERSE:
			board_whitedown = !board_whitedown;
			repaint_board = true;
			if (!computers_move) // If computer is now thinking, then the board will be repainted during animation of it's move
				InvalidateRect(hWnd, NULL, FALSE);
			break;
		case IDM_UNDO_MOVE:
			selected_x = selected_y = -1;
			selected_part_moves.clear();
			if (computers_move)
				break;
			checkers.undo_move();
			if (!pvp && checkers.get_white_turn() != player_white)
				checkers.undo_move();
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case IDM_REDO_MOVE:
			selected_x = selected_y = -1;
			selected_part_moves.clear();
			if (computers_move || checkers.get_state() != GAME_CONTINUE)
				break;
			checkers.redo_move();
			if (!pvp && checkers.get_state() == GAME_CONTINUE)
				checkers.redo_move();
			InvalidateRect(hWnd, NULL, FALSE);
			if (checkers.get_state() != GAME_CONTINUE)
				FinishGame(hWnd);
			break;
		case IDM_HINT:
			selected_x = selected_y = -1;
			selected_part_moves.clear();
			if (computers_move || checkers.get_state() != GAME_CONTINUE)
				break;
			checkers.part_undo();
			InvalidateRect(hWnd, NULL, FALSE);
			UpdateWindow(hWnd);
			computers_move = true;
			fut = std::async(ai_hint_move, hWnd);
			break;
		case IDM_OPENGAME:
			if (computers_move)
				break;
			ofn.Flags = OFN_HIDEREADONLY;
			ofn.lpstrTitle = "Open game";
			if (!GetOpenFileName(&ofn))
				break;
			in.open(ofn_buffer);
			try
			{
				checkers.load_game(in);
				MessageBox(hWnd, "Game succesfully loaded!", "Load result", MB_ICONINFORMATION);
			}
			catch (const checkers_error& error)
			{
				if (error.get_error_type() == error_type::WARNING)
					MessageBox(hWnd, error.what(), "Warning", MB_ICONWARNING);
				else
					MessageBox(hWnd, error.what(), "Error", MB_ICONERROR);
			}
			in.close();
			pvp = true;
			selected_x = selected_y = -1;
			selected_part_moves.clear();
			cur_move_used_pos.clear();
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case IDM_SAVEGAME:
			if (computers_move)
				break;
			ofn.Flags = OFN_NOTESTFILECREATE;
			ofn.lpstrTitle = "Save game";
			if (!GetSaveFileName(&ofn))
				break;
			out.open(ofn_buffer);
			checkers.save_game(out);
			out.close();
			MessageBox(hWnd, "Game succesfully saved!", "Save result", MB_ICONINFORMATION);
			break;
		case IDM_OPENBOARD:
			if (computers_move)
				break;
			ofn.Flags = OFN_HIDEREADONLY;
			ofn.lpstrTitle = "Open position";
			if (!GetOpenFileName(&ofn))
				break;
			in.open(ofn_buffer);
			try
			{
				checkers.load_board(in);
				MessageBox(hWnd, "Position succesfully loaded!", "Load result", MB_ICONINFORMATION);
			}
			catch (const checkers_error& error)
			{
				if (error.get_error_type() == error_type::WARNING)
					MessageBox(hWnd, error.what(), "Warning", MB_ICONWARNING);
				else
					MessageBox(hWnd, error.what(), "Error", MB_ICONERROR);
			}
			in.close();
			pvp = true;
			selected_x = selected_y = -1;
			selected_part_moves.clear();
			cur_move_used_pos.clear();
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case IDM_SAVEBOARD:
			if (computers_move)
				break;
			ofn.Flags = OFN_NOTESTFILECREATE;
			ofn.lpstrTitle = "Save position";
			if (!GetSaveFileName(&ofn))
				break;
			out.open(ofn_buffer);
			checkers.save_board(out);
			out.close();
			MessageBox(hWnd, "Position succesfully saved!", "Save result", MB_ICONINFORMATION);
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		delete blackPen;
		delete selectedPen;
		delete possiblePen;
		delete whiteCellBrush;
		delete blackCellBrush;
		delete whitePieceBrush;
		delete blackPieceBrush;
		delete shadowPieceBrush;
		delete crownBrush;
		DeleteObject(hBackground);
		DeleteObject(hBMMem);
		SelectObject(hDCMem, hOld);
		DeleteDC(hDCMem);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
// Callback function for about dialog box
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
// Callback function for new game with computer dialog box
INT_PTR CALLBACK DlgNewGame(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static game_rules rules;
	static bool misere;
	static size_t idx;
	static HWND hCBWnd;
	switch (message)
	{
	case WM_INITDIALOG:
		CheckRadioButton(hDlg, IDC_WHITE, IDC_BLACK, IDC_WHITE);
		hCBWnd = GetDlgItem(hDlg, IDC_LEVEL);
		for (size_t i = 0; i < LEVELS_COUNT + 1; ++i)
			SendMessage(hCBWnd, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(LEVELS_STR[i]));
		SendMessage(hCBWnd, CB_SETCURSEL, DEFAULT_LEVEL_IDX, NULL);
		hCBWnd = GetDlgItem(hDlg, IDC_RULES);
		for (size_t i = 0; i < RULES_COUNT; ++i)
			SendMessage(hCBWnd, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(RULES_STR[i]));
		SendMessage(hCBWnd, CB_SETCURSEL, DEFAULT_RULES_IDX, NULL);
		hCBWnd = GetDlgItem(hDlg, IDC_TIMELIMIT);
		for (size_t i = 0; i < TIMELIMITS_COUNT; ++i)
			SendMessage(hCBWnd, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(TIMELIMITS_STR[i]));
		SendMessage(hCBWnd, CB_SETCURSEL, DEFAULT_TIMELIMIT_IDX, NULL);
		return(INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			computers_move = false;
			repaint_board = true;
			pvp = IsDlgButtonChecked(hDlg, IDC_PVP);
			misere = IsDlgButtonChecked(hDlg, IDC_MISERE);
			idx = SendMessage(GetDlgItem(hDlg, IDC_RULES), CB_GETCURSEL, NULL, NULL);
			rules = RULES[idx];
			if (pvp)
			{
				board_whitedown = true;
				checkers.restart(rules, misere);
			}
			else
			{
				idx = SendMessage(GetDlgItem(hDlg, IDC_LEVEL), CB_GETCURSEL, NULL, NULL);
				if (idx == LEVELS_COUNT)
					checkers.set_search_depth(Checkers::UNBOUNDED_DEPTH);
				else
					checkers.set_search_depth(LEVELS[idx]);
				idx = SendMessage(GetDlgItem(hDlg, IDC_TIMELIMIT), CB_GETCURSEL, NULL, NULL);
				checkers.set_time_limit(TIMELIMITS[idx]);
				checkers.restart(rules, misere);
				if (IsDlgButtonChecked(hDlg, IDC_WHITE) == BST_CHECKED)
				{
					player_white = true;
					board_whitedown = true;
				}
				else
				{
					player_white = false;
					board_whitedown = false;
					checkers.perform_computer_move();
				}
			}
			InvalidateRect(GetParent(hDlg), NULL, TRUE);
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		case IDC_PVP:
			pvp = IsDlgButtonChecked(hDlg, IDC_PVP);
			EnableWindow(GetDlgItem(hDlg, IDC_LEVEL), !pvp);
			EnableWindow(GetDlgItem(hDlg, IDC_WHITE), !pvp);
			EnableWindow(GetDlgItem(hDlg, IDC_BLACK), !pvp);
			break;
		}
		break;
	}
	return (INT_PTR)FALSE;
}