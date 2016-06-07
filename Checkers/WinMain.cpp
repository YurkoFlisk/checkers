// Checkers
// By Yurko Prokopets(aka YurkoFlisk)
// WinMain.cpp
// Version 1.3

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <fstream>
#include <thread>
#include <mutex>
#include "targetver.h"
#include "resource.h"
#include "checkers.h"
#include "constants.h"
#if defined _DEBUG || defined DEBUG // Memory leak detection
 #define _CRTDBG_MAP_ALLOC
 #include <cstdlib>
 #include <crtdbg.h>
#endif
// Messages
#define CM_CPUMOVE WM_USER // Checkers' message - computer move
// Colors
CONSTEXPR DWORD C_LTGRAY = RGB(192, 192, 192);
CONSTEXPR DWORD C_GRAY = RGB(160, 160, 160);
CONSTEXPR DWORD C_BROWN = RGB(139, 69, 19);
CONSTEXPR DWORD C_BLACK = RGB(0, 0, 0);
CONSTEXPR DWORD C_GREEN = RGB(20, 255, 20);
CONSTEXPR DWORD C_SHADOW = RGB(77, 162, 49);
// Global variables
HINSTANCE hInst; // program's instance
Checkers checkers; // game logic
std::ifstream in; // for file operations
std::ofstream out; // ..
bool pvp; // true if a game is pvp(player versus player), false if it is with computer
bool board_whitedown; // true if board is turned so that 1-st row is lower than 8-th, false otherwise
bool player_white; // true if player plays white(unused if pvp == true)
bool repaint_board; // true if we need to repaint board in a next processing of WM_PAINT message
bool computers_move; // indicates whether ai is thinking at the moment
// Forward declarations
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
bool CreateMainWindow(HINSTANCE, int);
void FinishGame(HWND hWnd);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK CpuGame(HWND, UINT, WPARAM, LPARAM);
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
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
	wndClass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
	if (!RegisterClassEx(&wndClass))
		return false;
	HWND hWnd = CreateWindow(CLASS_NAME, MAIN_TITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		MAIN_WIDTH, MAIN_HEIGHT, NULL, NULL, hInst, NULL);
	if (!hWnd)
		return false;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return true;
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
// Callback function for main window
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static char ofn_buffer[MAX_OFN_CHARS];
	static OPENFILENAMEA ofn;
	static size_t sx, sy;
	static size_t tmp_x, tmp_y, x_pos, y_pos;
	static int d_ycenter; // distance between base line and y-center of the font
	static HBRUSH hWhiteCellBrush;
	static HBRUSH hBlackCellBrush;
	static HBRUSH hWhitePieceBrush;
	static HBRUSH hBlackPieceBrush;
	static HBRUSH hShadowPieceBrush;
	static HBRUSH hCrownBrush;
	static TEXTMETRIC tm;
	static Move cpuMove; // shared between two threads
	static std::mutex mut;
	static std::thread ai; // thread for ai computations
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
	HDC hDC;
	PAINTSTRUCT ps;
	int wmId, wmEvent;
	switch (msg)
	{
	case WM_CREATE:
		ofn.lStructSize = sizeof(ofn);
		ofn.nMaxFile = MAX_OFN_CHARS;
		ofn.lpstrFile = ofn_buffer;
		ofn.lpstrFilter = "All files\0*.*";
		ofn.hInstance = hInst;
		ofn.lpstrInitialDir = ".\\";
		hWhiteCellBrush = CreateSolidBrush(C_LTGRAY);
		hBlackCellBrush = CreateSolidBrush(C_BROWN);
		hWhitePieceBrush = CreateSolidBrush(C_GRAY);
		hBlackPieceBrush = CreateSolidBrush(C_BLACK);
		hShadowPieceBrush = CreateSolidBrush(C_SHADOW);
		hCrownBrush = CreateSolidBrush(C_GREEN);
		hDC = GetDC(hWnd);
		GetTextMetrics(hDC, &tm);
		d_ycenter = (tm.tmAscent - tm.tmDescent) / 2;
		player_white = true;
		pvp = false;
		board_whitedown = true;
		repaint_board = true;
		computers_move = false;
		break;
	case WM_SIZE:
		sx = LOWORD(lParam);
		sy = HIWORD(lParam);
		repaint_board = true;
		break;
	case WM_MOVE:
		repaint_board = true;
		break;
	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);
		if (checkers.current_turn_color() == WHITE)
			SelectObject(hDC, (HGDIOBJ)hWhitePieceBrush);
		else
			SelectObject(hDC, (HGDIOBJ)hBlackPieceBrush);
		Ellipse(hDC, BOARD_RIGHT, BOARD_TOP - D_TURN, BOARD_RIGHT + D_TURN, BOARD_TOP);
		if (computers_move)
		{
			EndPaint(hWnd, &ps);
			break;
		}
		if (repaint_board)
		{
			SetTextAlign(hDC, TA_BASELINE | TA_CENTER);
			for (size_t i = 0; i < 8; ++i)
				TextOut(hDC, BOARD_LEFT + i*CELL_SIZE + CELL_SIZE / 2, BOARD_TOP - D_COLUMN + d_ycenter, COLUMN_LR[board_whitedown ? i : 7 - i], 1);
			for (size_t i = 0; i < 8; ++i)
				TextOut(hDC, BOARD_LEFT - D_ROW, BOARD_TOP + i*CELL_SIZE + CELL_SIZE / 2 + d_ycenter, ROW_TD[board_whitedown ? i : 7 - i], 1);
			SelectObject(hDC, (HGDIOBJ)hWhiteCellBrush);
			for (size_t x = BOARD_LEFT, n = 0; x < BOARD_RIGHT; x += CELL_SIZE, ++n)
				for (size_t y = BOARD_TOP + CELL_SIZE*(n & 1); y < BOARD_BOTTOM; y += 2 * CELL_SIZE)
					Rectangle(hDC, x, y, x + CELL_SIZE, y + CELL_SIZE);
			repaint_board = false;
		}
		SelectObject(hDC, (HGDIOBJ)hBlackCellBrush);
		for (size_t x = BOARD_LEFT, n = 0; x < BOARD_RIGHT; x += CELL_SIZE, ++n)
			for (size_t y = BOARD_TOP + CELL_SIZE*(1 - (n & 1)); y < BOARD_BOTTOM; y += 2 * CELL_SIZE)
				Rectangle(hDC, x, y, x + CELL_SIZE, y + CELL_SIZE);
		SelectObject(hDC, (HGDIOBJ)hShadowPieceBrush);
		for (size_t i = 0; i < 8; ++i)
			for (size_t j = i & 1; j < 8; j += 2)
				if (checkers[i][j].color == SHADOW)
					if (board_whitedown)
						Ellipse(hDC, BOARD_LEFT + j*CELL_SIZE, BOARD_BOTTOM - (i + 1)*CELL_SIZE, BOARD_LEFT + (j + 1)*CELL_SIZE, BOARD_BOTTOM - i*CELL_SIZE);
					else
						Ellipse(hDC, BOARD_RIGHT - (j + 1)*CELL_SIZE, BOARD_TOP + i*CELL_SIZE, BOARD_RIGHT - j*CELL_SIZE, BOARD_TOP + (i + 1)*CELL_SIZE);
		SelectObject(hDC, (HGDIOBJ)hWhitePieceBrush);
		for (size_t i = 0; i < 8; ++i)
			for (size_t j = i & 1; j < 8; j += 2)
				if (checkers[i][j].color == WHITE)
					if (board_whitedown)
						Ellipse(hDC, BOARD_LEFT + j*CELL_SIZE, BOARD_BOTTOM - (i + 1)*CELL_SIZE, BOARD_LEFT + (j + 1)*CELL_SIZE, BOARD_BOTTOM - i*CELL_SIZE);
					else
						Ellipse(hDC, BOARD_RIGHT - (j + 1)*CELL_SIZE, BOARD_TOP + i*CELL_SIZE, BOARD_RIGHT - j*CELL_SIZE, BOARD_TOP + (i + 1)*CELL_SIZE);
		SelectObject(hDC, (HGDIOBJ)hBlackPieceBrush);
		for (size_t i = 0; i < 8; ++i)
			for (size_t j = i & 1; j < 8; j += 2)
				if (checkers[i][j].color == BLACK)
					if (board_whitedown)
						Ellipse(hDC, BOARD_LEFT + j*CELL_SIZE, BOARD_BOTTOM - (i + 1)*CELL_SIZE, BOARD_LEFT + (j + 1)*CELL_SIZE, BOARD_BOTTOM - i*CELL_SIZE);
					else
						Ellipse(hDC, BOARD_RIGHT - (j + 1)*CELL_SIZE, BOARD_TOP + i*CELL_SIZE, BOARD_RIGHT - j*CELL_SIZE, BOARD_TOP + (i + 1)*CELL_SIZE);
		SelectObject(hDC, (HGDIOBJ)hCrownBrush);
		for (size_t i = 0; i < 8; ++i)
			for (size_t j = i & 1; j < 8; j += 2)
				if (checkers[i][j].queen)
					if (board_whitedown)
						Ellipse(hDC, BOARD_LEFT + j*CELL_SIZE + D_QUEEN, BOARD_BOTTOM - (i + 1)*CELL_SIZE + D_QUEEN,
							BOARD_LEFT + (j + 1)*CELL_SIZE - D_QUEEN, BOARD_BOTTOM - i*CELL_SIZE - D_QUEEN);
					else
						Ellipse(hDC, BOARD_RIGHT - (j + 1)*CELL_SIZE + D_QUEEN, BOARD_TOP + i*CELL_SIZE + D_QUEEN,
							BOARD_RIGHT - j*CELL_SIZE - D_QUEEN, BOARD_TOP + (i + 1)*CELL_SIZE - D_QUEEN);
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
		if (checkers.part_move(position(y_pos, x_pos)))
		{
			InvalidateRect(hWnd, NULL, FALSE);
			UpdateWindow(hWnd);
			if (!pvp && checkers.get_state() == GAME_CONTINUE)
			{
				computers_move = true; // From this moment separate thread for ai is launched
				ai = std::thread(ai_move, hWnd); // Launching new thread for ai
				ai.detach();
			}
			else if (checkers.get_state() != GAME_CONTINUE)
				FinishGame(hWnd);
		}
		else
			InvalidateRect(hWnd, NULL, FALSE);
		break;
	case CM_CPUMOVE:
		// Performing and animating computer move
		computers_move = false;
		for (size_t i = 0; i < cpuMove.size(); ++i)
		{
			checkers.part_move(cpuMove[i]);
			InvalidateRect(hWnd, NULL, FALSE);
			UpdateWindow(hWnd);
			Sleep(STEP_SLEEP_TIME);
		}
		InvalidateRect(hWnd, NULL, FALSE);
		UpdateWindow(hWnd);
		if (checkers.get_state() != GAME_CONTINUE)
			FinishGame(hWnd);
		else if (!pvp && checkers.get_white_turn() != player_white)
		{
			computers_move = true;
			ai = std::thread(ai_move, hWnd);
			ai.detach();
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDM_NEW_GAME_CPU:
			if (!computers_move)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_CPUGAME), hWnd, CpuGame);
			break;
		case IDM_NEW_GAME_PVP:
			if (computers_move)
				break;
			checkers.restart();
			pvp = true;
			board_whitedown = true;
			repaint_board = true;
			computers_move = false;
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		case IDM_REVERSE:
			board_whitedown = !board_whitedown;
			repaint_board = true;
			if (!computers_move) // If computer is now thinking, then the board will be repainted during animation of it's move
				InvalidateRect(hWnd, NULL, TRUE);
			break;
		case IDM_UNDO_MOVE:
			if (computers_move)
				break;
			checkers.undo_move();
			if (!pvp && checkers.get_white_turn() != player_white)
				checkers.undo_move();
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case IDM_REDO_MOVE:
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
			if (computers_move || checkers.get_state() != GAME_CONTINUE)
				break;
			computers_move = true;
			ai = std::thread(ai_hint_move, hWnd); // Launching new thread for ai
			ai.detach();
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
INT_PTR CALLBACK CpuGame(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static size_t idx;
	static HWND hLevelCBWnd;
	switch (message)
	{
	case WM_INITDIALOG:
		CheckRadioButton(hDlg, IDC_WHITE, IDC_BLACK, IDC_WHITE);
		hLevelCBWnd = GetDlgItem(hDlg, IDC_LEVEL);
		for (size_t i = 0; i < LEVELS_COUNT; ++i)
			SendMessage(hLevelCBWnd, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(LEVELS_STR[i]));
		SendMessage(hLevelCBWnd, CB_SETCURSEL, LEVELS_COUNT - 1, NULL);
		return(INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			computers_move = false;
			repaint_board = true;
			pvp = false;
			idx = SendMessage(GetDlgItem(hDlg, IDC_LEVEL), CB_GETCURSEL, NULL, NULL);
			checkers.set_search_depth(LEVELS[idx]);
			checkers.restart();
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
			InvalidateRect(GetParent(hDlg), NULL, TRUE);
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
