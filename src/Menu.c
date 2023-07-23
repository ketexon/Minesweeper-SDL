#include "Win.h"
#include "State.h"
#include "Constants.h"
#include "Resources.h"

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

void State_UncheckDifficulty(State* state){
	CheckMenuItem(state->menu, IDM_EASY, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(state->menu, IDM_MEDIUM, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(state->menu, IDM_HARD, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(state->menu, IDM_CUSTOM, MF_BYCOMMAND | MF_UNCHECKED);

	MENUITEMINFOW info = {
		.cbSize = sizeof(MENUITEMINFOW),
		.fMask = MIIM_STRING,
		.dwTypeData = MENU_DEFAULT_CUSTOM_DIFFICULTY_TEXT,
		.cch = sizeof(MENU_DEFAULT_CUSTOM_DIFFICULTY_TEXT)/sizeof(*MENU_DEFAULT_CUSTOM_DIFFICULTY_TEXT) - 1
	};

	SetMenuItemInfoW(
		state->menu,
		IDM_CUSTOM,
		false,
		&info
	);
}

void State_UncheckTheme(State* state){
	CheckMenuItem(state->menu, IDM_THEME_DEFAULT, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(state->menu, IDM_THEME_CUTE, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(state->menu, IDM_THEME_CUSTOM, MF_BYCOMMAND | MF_UNCHECKED);

	MENUITEMINFOW info = {
		.cbSize = sizeof(MENUITEMINFOW),
		.fMask = MIIM_STRING,
		.dwTypeData = MENU_DEFAULT_CUSTOM_THEME_TEXT,
		.cch = sizeof(MENU_DEFAULT_CUSTOM_THEME_TEXT)/sizeof(*MENU_DEFAULT_CUSTOM_THEME_TEXT) - 1
	};

	SetMenuItemInfoW(
		state->menu,
		IDM_THEME_CUSTOM,
		false,
		&info
	);
}

LPWSTR State_CreateCustomThemeFileDialog(State* state){
	LPWSTR filePathOut = NULL;

	// state->images.tilesheet.texture = state->images.tilesheet.sourceTextures.original;
	// FILE DIALOGUE
	IFileOpenDialog* fileDialog = NULL;
	HRESULT hr = CoCreateInstance(
		&CLSID_FileOpenDialog,
		NULL,
		CLSCTX_INPROC_SERVER,
		&IID_IFileOpenDialog,
		&(void*) fileDialog
	);
	if(SUCCEEDED(hr)){
		FILEOPENDIALOGOPTIONS options;
		hr = fileDialog->lpVtbl->GetOptions(fileDialog, &options);
		if(SUCCEEDED(hr)){
			hr = fileDialog->lpVtbl->SetOptions(fileDialog, options | FOS_FORCEFILESYSTEM);
			if(SUCCEEDED(hr)){
				COMDLG_FILTERSPEC filters[] = {
					{
						.pszName = L"Image",
						.pszSpec = L"*.png",
					},
				};
				hr = fileDialog->lpVtbl->SetFileTypes(fileDialog, sizeof(filters) / sizeof(*filters), filters);
				if(SUCCEEDED(hr)){
					hr = fileDialog->lpVtbl->SetFileTypeIndex(fileDialog, 1);
					if(SUCCEEDED(hr)){
						hr = fileDialog->lpVtbl->SetDefaultExtension(fileDialog, L"png");
						if(SUCCEEDED(hr)){
							hr = fileDialog->lpVtbl->Show(fileDialog, NULL);

							if (SUCCEEDED(hr)){
								IShellItem* shellItem = NULL;
								hr = fileDialog->lpVtbl->GetResult(fileDialog, &shellItem);
								if(SUCCEEDED(hr)){
									PWSTR filePath = NULL;
									hr = shellItem->lpVtbl->GetDisplayName(shellItem, SIGDN_FILESYSPATH, &filePath);
									if(SUCCEEDED(hr)){
										filePathOut = filePath;
									}
									else{
										fprintf(stderr, "Could not get filepath\n");
									}
								}
								else{
									fprintf(stderr, "Could not get result\n");
								}
							}
							// else: cancelled
						}
						else{
							fprintf(stderr, "Could not set default extension\n");
						}
					}
					else{
						fprintf(stderr, "Could not set file type index\n");
					}
				}
				else{
					fprintf(stderr, "Could not set file types\n");
				}
			}
			else{
				fprintf(stderr, "Could not set options\n");
			}
		}
		else{
			fprintf(stderr, "Could not get options\n");
		}

		fileDialog->lpVtbl->Release(fileDialog);
	}
	else{
		fprintf(stderr, "Could not create IFileDialog\n");
	}

	return filePathOut;
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
	} else if(id == IDM_THEME_DEFAULT){
		state->images.tilesheet.texture = state->images.tilesheet.sourceTextures.original;
		State_UpdateBackgroundColor(state);

		State_UncheckTheme(state);
		CheckMenuItem(
			state->menu,
			IDM_THEME_DEFAULT,
			MF_BYCOMMAND | MF_CHECKED
		);
	} else if(id == IDM_THEME_CUTE){
		state->images.tilesheet.texture = state->images.tilesheet.sourceTextures.cute;
		State_UpdateBackgroundColor(state);

		State_UncheckTheme(state);
		CheckMenuItem(
			state->menu,
			IDM_THEME_CUTE,
			MF_BYCOMMAND | MF_CHECKED
		);
	} else if(id == IDM_THEME_CUSTOM){
		LPWSTR filePath = State_CreateCustomThemeFileDialog(state);

		if(filePath != NULL){
			int mbBufferSize = WideCharToMultiByte(
				CP_UTF8, 0,
				filePath, -1,
				NULL, 0,
				NULL, NULL
			);
			char* filePathMB = malloc(mbBufferSize);

			mbBufferSize = WideCharToMultiByte(
				CP_UTF8, 0,
				filePath, -1,
				filePathMB, mbBufferSize,
				NULL, NULL
			);

			if(mbBufferSize != 0){
				SDL_Texture* newCustomTexture = State_LoadTextureFromPath(state, filePathMB);
				if(state->images.tilesheet.sourceTextures.custom != NULL) {
					SDL_DestroyTexture(state->images.tilesheet.sourceTextures.custom);
				}
				state->images.tilesheet.sourceTextures.custom = newCustomTexture;
				state->images.tilesheet.texture = newCustomTexture;

				State_UpdateBackgroundColor(state);

				LPWSTR fileName = PathFindFileNameW(filePath);

				State_UncheckTheme(state);
				CheckMenuItem(
					state->menu,
					IDM_THEME_CUSTOM,
					MF_BYCOMMAND | MF_CHECKED
				);

				size_t newMenuTextLength = sizeof(MENU_DEFAULT_CUSTOM_THEME_TEXT)/sizeof(*MENU_DEFAULT_CUSTOM_THEME_TEXT) // Default
					+ 1 // \t
					+ wcslen(fileName) // filename
					+ 2;// quotes

				LPWSTR newMenuText = malloc((newMenuTextLength + 1) * sizeof(*newMenuText));
				swprintf_s(newMenuText, newMenuTextLength + 1, L"%s\t\"%s\"", MENU_DEFAULT_CUSTOM_THEME_TEXT, fileName);

				MENUITEMINFOW info = {
					.cbSize = sizeof(MENUITEMINFOW),
					.fMask = MIIM_STRING,
					.dwTypeData = newMenuText,
					.cch = newMenuTextLength
				};

				SetMenuItemInfoW(
					state->menu,
					IDM_THEME_CUSTOM,
					false,
					&info
				);

				free(newMenuText);
			}
			else{
				printf("Could not convert wstr to mbstr\n");
			}

			CoTaskMemFree(filePath);
			free(filePathMB);
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
