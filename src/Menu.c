#include "Win.h"
#include "State.h"
#include "Constants.h"

#include <SDL2/SDL.h>
#include <SDL_syswm.h>

#include <stdio.h>

typedef struct CustomDifficultyResult {
	uint32_t width, height;
	uint32_t nMines;
} CustomDifficultyResult;

INT_PTR CustomDifficultyDialogProc(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam
);

void State_CreateMenu(State* state){
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(state->sdl.window, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;

	state->menu = LoadMenuW(
		(HINSTANCE) GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
		RC_MENU
	);

	if(state->menu) SetMenu(hwnd, state->menu);
}

void State_HandleMenuEvent(State* state, HWND hwnd, WORD id){
	if(id == IDM_EXIT){
		state->shouldQuit = true;
	}
	else if(id == IDM_EASY){
		state->board.nMines = BOARD_N_MINES_EASY;
		state->board.width = BOARD_WIDTH_EASY;
		state->board.height = BOARD_HEIGHT_EASY;
		State_ResetBoard(state);

		State_UncheckDifficulty(state);
		CheckMenuItem(
			state->menu,
			IDM_EASY,
			MF_BYCOMMAND | MF_CHECKED
		);
	}
	else if(id == IDM_MEDIUM){
		state->board.nMines = BOARD_N_MINES_MEDIUM;
		state->board.width = BOARD_WIDTH_MEDIUM;
		state->board.height = BOARD_HEIGHT_MEDIUM;
		State_ResetBoard(state);

		State_UncheckDifficulty(state);
		CheckMenuItem(
			state->menu,
			IDM_MEDIUM,
			MF_BYCOMMAND | MF_CHECKED
		);
	}
	else if(id == IDM_HARD){
		state->board.nMines = BOARD_N_MINES_HARD;
		state->board.width = BOARD_WIDTH_HARD;
		state->board.height = BOARD_HEIGHT_HARD;
		State_ResetBoard(state);

		State_UncheckDifficulty(state);
		CheckMenuItem(
			state->menu,
			IDM_HARD,
			MF_BYCOMMAND | MF_CHECKED
		);
	}
	else if(id == IDM_CUSTOM){
		CustomDifficultyResult* res = (CustomDifficultyResult*) DialogBoxParamW(
			NULL,
			RC_CUSTOM_DIFFICULTY_DIALOG,
			hwnd,
			&CustomDifficultyDialogProc,
			0
		);
		if(res != NULL){
			state->board.nMines = res->nMines;
			state->board.width = res->width;
			state->board.height = res->height;
			State_ResetBoard(state);

			State_UncheckDifficulty(state);
			CheckMenuItem(
				state->menu,
				IDM_CUSTOM,
				MF_BYCOMMAND | MF_CHECKED
			);

			wchar_t newCustomDifficultyText[sizeof("000x000, 000 mines") + 1];
			swprintf(
				newCustomDifficultyText,
				sizeof(newCustomDifficultyText)/sizeof(*newCustomDifficultyText) - 1,
				L"%ux%u, %u mines",
				res->width,
				res->height,
				res->nMines
			);

			MENUITEMINFOW info = {
				.cbSize = sizeof(MENUITEMINFOW),
				.fMask = MIIM_STRING,
				.dwTypeData = newCustomDifficultyText,
				.cch = wcslen(newCustomDifficultyText),
			};

			SetMenuItemInfoW(
				state->menu,
				IDM_CUSTOM,
				false,
				&info
			);

			free(res);
		}
	}
}

INT_PTR CustomDifficultyDialogProc(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam
){
	const int msgresult = 0; // for some reason DWL_MSGRESULT is undeclared?
	switch(msg){
		case WM_INITDIALOG: {
			// Set max number of characters to 3
			SendDlgItemMessageW(
				hwnd,
				IDE_WIDTH,
				EM_SETLIMITTEXT,
				3, 0
			);

			SendDlgItemMessageW(
				hwnd,
				IDE_HEIGHT,
				EM_SETLIMITTEXT,
				3, 0
			);

			break;
		}
		case WM_COMMAND: {
			WORD id = LOWORD(wParam);
			if(id == IDOK){
				wchar_t* end;

				wchar_t buffer[4];

				memset(buffer, 0, sizeof(buffer)/sizeof(*buffer));
				*((WORD*) buffer) = 3;
				SendDlgItemMessageW(hwnd, IDE_WIDTH, EM_GETLINE, 0, (LPARAM) buffer);
				unsigned long width = wcstoul(buffer, &end, 10);

				memset(buffer, 0, sizeof(buffer)/sizeof(*buffer));
				*((WORD*) buffer) = 3;
				SendDlgItemMessageW(hwnd, IDE_HEIGHT, EM_GETLINE, 0, (LPARAM) buffer);
				unsigned long height = wcstoul(buffer, &end, 10);

				memset(buffer, 0, sizeof(buffer)/sizeof(*buffer));
				*((WORD*) buffer) = 3;
				SendDlgItemMessageW(hwnd, IDE_MINES, EM_GETLINE, 0, (LPARAM) buffer);
				unsigned long mines = wcstoul(buffer, &end, 10);

				if(width <= 0 || height <= 0 || mines <= 0) {
					MessageBoxW(hwnd, L"Width, height, and mines must be greater than 0", L"Input Error", MB_OK | MB_ICONERROR);
					return TRUE;
				}

				if(width > 999 || height > 999) {
					return TRUE;
				}

				if(mines >= width * height) {
					MessageBoxW(hwnd, L"Too many mines", L"Input Error", MB_OK | MB_ICONERROR);
					return TRUE;
				}

				CustomDifficultyResult* res = malloc(sizeof(CustomDifficultyResult));
				res->width = width;
				res->height = height;
				res->nMines = mines;
				EndDialog(hwnd, (INT_PTR) res);
				return TRUE;
			}
			else if(id == IDE_WIDTH || id == IDE_HEIGHT || id == IDE_MINES){
				WORD msg = HIWORD(wParam);
				if(msg == EN_UPDATE) {
					WORD length = (WORD) SendDlgItemMessageW(
						hwnd,
						id,
                        EM_LINELENGTH,
						0, 0
					);
					if(length == 0) break;

					wchar_t buffer[4] = { 0 };
					// required for
					*((WORD*) buffer) = 3;

					SendDlgItemMessageW(
						hwnd,
						id,
                        EM_GETLINE,
						0, (LPARAM) buffer
					);

					wchar_t newBuffer[4] = { 0 };

					bool textChanged = false;
					int j = 0;
					for(int i = 0; buffer[i] != 0; ++i){
						if(iswdigit(buffer[i])){
							newBuffer[j++] = buffer[i];
						}
						else {
							textChanged = true;
						}
					}

					if(textChanged){
						WORD oldSel = LOWORD(SendDlgItemMessageW(hwnd, id, EM_GETSEL, (WPARAM) NULL, (LPARAM) NULL));
						SendDlgItemMessageW(hwnd, id, WM_SETTEXT, 0, (LPARAM) newBuffer);
						SendDlgItemMessageW(hwnd, id, EM_SETSEL, (WPARAM) oldSel - 1, (LPARAM) oldSel - 1);
					}
				}
			}
			break;
		}
		case WM_CLOSE: {
			EndDialog(hwnd, 0);
			return TRUE;
		}
	}
	return FALSE;
}
