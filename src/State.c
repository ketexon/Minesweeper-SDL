#include "State.h"
#include "Constants.h"
#include "Win.h"

#include "Resources.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <SDL_image.h>
#include <SDL_syswm.h>

#ifdef KET_DEBUG
void DrawLayoutOutlineV2(State* state){
	SDL_SetRenderDrawColor(state->sdl.renderer, 0, 255, 0, 255);
	SDL_RenderDrawRectsF(state->sdl.renderer, (SDL_FRect*)&state->layoutv2, sizeof(state->layoutv2)/sizeof(SDL_FRect));
}
#endif

SDL_FRect RectToFRect(const SDL_Rect* rect){
	return (SDL_FRect) {
		.x = rect->x,
		.y = rect->y,
		.w = rect->w,
		.h = rect->h,
	};
}

void MousePosToTile(State* state, int mouseX, int mouseY, int* tilePosX, int* tilePosY){
	SDL_FRect boardRect = state->layoutv2.board;

	float tileWidth = boardRect.w / state->board.width;
	float tileHeight = boardRect.h / state->board.height;

	// tile tx, ty is the rect:
	//	{
	//		tx * tileWidth + left, ty * tileHeight + top,
	//		tileWidth, tileHeight
	// }
	//
	// ie: tx * tileWidth + left <= mx < (tx + 1) * tileWidth + left
	//			tx * tileWidth <= mx - left < (tx + 1) * tileWidth
	//			tx <= (mx - left)/tileWidth < tx + 1
	// and since tx is an integer, this means that tx = (mx - left)/tileWidth
	// similarly, ty = (my - top)/tileHeight;

	// note: we cannot just do int -> float cast because it rounds towards 0, so -0.5 -0.5 rounds to 0 0
	int tx = floorf((mouseX - boardRect.x)/tileWidth);
	int ty = floorf((mouseY - boardRect.y)/tileHeight);

	// if tile pos not valid, set to -1, -1
	if(tx < 0 || tx >= state->board.width || ty < 0 || ty >= state->board.height){
		tx = -1;
		ty = -1;
	}
	*tilePosX = tx;
	*tilePosY = ty;
}

bool State_Init(State* state){
	memset(state, 0, sizeof(*state));

	state->shouldQuit = false;

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0){
		fprintf(stderr, "Could not init SDL.");
		return false;
	}
	state->sdl.init = true;

	int sdlImageFlags = IMG_INIT_PNG;
	if(!(IMG_Init(sdlImageFlags) & sdlImageFlags)){
		fprintf(stderr, "Could not init SDL_image.");
		return false;
	}
	state->sdl.image.init = true;

	state->sdl.window = SDL_CreateWindow(
		WINDOW_TITLE,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE
	);

	if(state->sdl.window == NULL){
		fprintf(stderr, "Could not create window.");
		return false;
	}

	state->sdl.renderer = SDL_CreateRenderer(
		state->sdl.window,
		-1,
		SDL_RENDERER_ACCELERATED
	);

	if(state->sdl.renderer == NULL){
		fprintf(stderr, "Could not create renderer.");
		return false;
	}

	State_InitBoard(state);
	State_InitLayout(state);

	State_LoadResources(state);

	state->mouse.tileHoverX = -1;
	state->mouse.tileHoverY = -1;

	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
	State_CreateMenu(state);

	return true;
}

void State_InitBoard(State* state){
	state->board.width = BOARD_WIDTH_MEDIUM;
	state->board.height = BOARD_HEIGHT_MEDIUM;

	state->board.nMines = BOARD_N_MINES_MEDIUM;

	State_CreateBoard(state);
}

void State_CreateBoard(State* state){
	size_t nTiles = state->board.width * state->board.height;

	state->board.tiles = malloc(sizeof(Tile) * nTiles);
	state->board.surroundingMines = malloc(sizeof(char) * nTiles);
	state->board.rects = malloc(sizeof(SDL_FRect) * nTiles);

	for(size_t i = 0; i < nTiles; ++i){
		state->board.tiles[i] = (Tile) {
			.state = TILE_STATE_UNINITIALIZED
		};
	}

	state->board.tilesLeft = nTiles;
	state->board.minesFlagged = 0;

	state->gameStarted = false;
	state->gameOver = false;
}

void State_DestroyBoard(State* state){
	if(state->board.tiles) free(state->board.tiles);
	if(state->board.surroundingMines) free(state->board.surroundingMines);
	if(state->board.rects) free(state->board.rects);
}

void State_ResetBoard(State* state){
	State_DestroyBoard(state);
	State_CreateBoard(state);

	int windowWidth, windowHeight;
	SDL_GetWindowSizeInPixels(state->sdl.window, &windowWidth, &windowHeight);
	State_RecalculateLayout(state, windowWidth, windowHeight);
}

// tileX,Y is the tile clicked to start the game
// we must guarentee that there are no mines within 3x3 of that tile
void State_InitMines(State* state, int tileX, int tileY){
	// generate mines
	int nTiles = state->board.width * state->board.height;
	int tilesLeft = nTiles - (BOARD_CLICK_SAFE_AREA * 2 + 1) * (BOARD_CLICK_SAFE_AREA * 2 + 1);
	int minesLeft = state->board.nMines;

	for(int x = 0; x < state->board.width; ++x){
		for(int y = 0; y < state->board.height; ++y){
			// skip over tiles within safe area of clicked tile
			if(
				x > tileX - BOARD_CLICK_SAFE_AREA && x < tileX + BOARD_CLICK_SAFE_AREA
				&& y > tileY - BOARD_CLICK_SAFE_AREA && y < tileY + BOARD_CLICK_SAFE_AREA
			) {
				continue;
			}

			float probability = (float)(minesLeft) / (tilesLeft--);
			if((float)rand() / RAND_MAX < probability){
				state->board.tiles[x + y * state->board.width].state |= TILE_STATE_MINE;
				minesLeft -= 1;
			}


			if(minesLeft == 0) break;
		}

		if(minesLeft == 0) break;
	}

	// generate adjacent mine counts
	// also set init flag
	for(int tx = 0; tx < state->board.width; ++tx){
		for(int ty = 0; ty < state->board.height; ++ty){
			int index = tx + ty * state->board.width;
			int minesSurroundingTile = 0;

			for(int x = tx - 1; x <= tx + 1; ++x){
				for(int y = ty - 1; y <= ty + 1; ++y){
					if((x == tx && y == ty) || x < 0 || x >= state->board.width || y < 0 || y >= state->board.height) {
						continue;
					}
					if(state->board.tiles[x + y * state->board.width].state & TILE_STATE_MINE) {
						++minesSurroundingTile;
					}
				}

				state->board.surroundingMines[index] = minesSurroundingTile;

				state->board.tiles[index].state |= TILE_STATE_INITIALIZED;
			}
		}
	}
}

void State_StartGame(State* state, int tileX, int tileY){
	//srand(time(NULL));

	State_InitMines(state, tileX, tileY);
	state->ticksStarted = SDL_GetTicks64();
	state->gameStarted = true;
}

void State_LoseGame(State* state){
	state->ticksEnded = SDL_GetTicks64();
	state->gameOver = true;
	state->gameWon = false;
}

void State_WinGame(State* state){
	state->ticksEnded = SDL_GetTicks64();
	state->gameOver = true;
	state->gameWon = true;
}

void State_UncoverTile(State* state, int tileX, int tileY){
	int index = tileX + tileY * state->board.width;
	Tile* tile = &state->board.tiles[index];
	// uncovering an already uncovered tile -> nothing to do
	if(tile->state & TILE_STATE_UNCOVERED || tile->state & TILE_STATE_FLAG) return;

	--state->board.tilesLeft;
	tile->state |= TILE_STATE_UNCOVERED;

	uint8_t surroundingMines = state->board.surroundingMines[index];
	if(surroundingMines == 0){
		for(int x = tileX - 1; x <= tileX + 1; ++x){
			for(int y = tileY - 1; y <= tileY + 1; ++y){
				if((x == tileX && y == tileY) || x < 0 || x >= state->board.width || y < 0 || y >= state->board.height) {
					continue;
				}

				if(!(state->board.tiles[x + y * state->board.width].state & TILE_STATE_UNCOVERED)){
					State_UncoverTile(state, x, y);
				}
			}
		}
	}
}

void State_FlagTile(State* state, int tileX, int tileY) {
	int tileIndex = tileX + tileY * state->board.width;

	Tile* tile = &state->board.tiles[tileIndex];
	if(!(tile->state & TILE_STATE_UNCOVERED)) {
		// toggle flag
		tile->state ^= TILE_STATE_FLAG;
		state->board.minesFlagged += (tile->state & TILE_STATE_FLAG) ? 1 : -1;
	}
}

void State_ClickTile(State* state, int tileX, int tileY) {
	int tileIndex = tileX + tileY * state->board.width;

	Tile* tile = &state->board.tiles[tileIndex];

	// cant click a flagged tile
	if(tile->state & TILE_STATE_FLAG) return;

	if(!(tile->state & TILE_STATE_INITIALIZED)) {
		State_StartGame(state, tileX, tileY);
	}

	if(tile->state & TILE_STATE_MINE) {
		tile->state |= TILE_STATE_UNCOVERED;
		State_LoseGame(state);
	}
	else {
		State_UncoverTile(state, tileX, tileY);
	}

	if(state->board.tilesLeft == state->board.nMines){
		State_WinGame(state);
	}
}

void State_ClickSmiley(State* state){
	State_ResetBoard(state);
}

void State_UncheckDifficulty(State* state){
	CheckMenuItem(
		state->menu,
		IDM_EASY,
		MF_BYCOMMAND | MF_UNCHECKED
	);

	CheckMenuItem(
		state->menu,
		IDM_MEDIUM,
		MF_BYCOMMAND | MF_UNCHECKED
	);

	CheckMenuItem(
		state->menu,
		IDM_HARD,
		MF_BYCOMMAND | MF_UNCHECKED
	);

	CheckMenuItem(
		state->menu,
		IDM_CUSTOM,
		MF_BYCOMMAND | MF_UNCHECKED
	);

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

typedef struct CustomDifficultyResult {
	uint32_t width, height;
	uint32_t nMines;
} CustomDifficultyResult;

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

				wchar_t buffer[4] = { 0 };

				*((WORD*) buffer) = 3;
				SendDlgItemMessageW(hwnd, IDE_WIDTH, EM_GETLINE, 0, (LPARAM) buffer);
				WORD width = wcstoul(buffer, &end, 10);

				*((WORD*) buffer) = 3;
				SendDlgItemMessageW(hwnd, IDE_WIDTH, EM_GETLINE, 0, (LPARAM) buffer);
				WORD height = wcstoul(buffer, &end, 10);

				*((WORD*) buffer) = 3;
				SendDlgItemMessageW(hwnd, IDE_MINES, EM_GETLINE, 0, (LPARAM) buffer);
				WORD mines = wcstoul(buffer, &end, 10);



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

void State_HandleEvent(State* state, SDL_Event* event){
	switch(event->type){
		case SDL_WINDOWEVENT: {
			switch(event->window.event){
				case SDL_WINDOWEVENT_RESIZED:
					State_RecalculateLayout(state, event->window.data1, event->window.data2);
					break;
			}
			break;
		}
		case SDL_MOUSEBUTTONDOWN: {
			if(event->button.button == SDL_BUTTON_LEFT){
				if(!state->gameOver) {
					int tx, ty;
					MousePosToTile(state, event->button.x, event->button.y, &tx, &ty);

					// if clicked on a tile
					if(tx != -1){
						state->mouse.down = true;

						int index = tx + ty * state->board.width;
						// mark tile as pressed
						state->board.tiles[index].state |= TILE_STATE_PRESSED;
					}
				}

				SDL_FRect mouseRect = (SDL_FRect) {
					.x = event->button.x,
					.y = event->button.y,
					.w = 1, .h = 1,
				};
				if(SDL_HasIntersectionF(&state->layoutv2.smiley, &mouseRect)){
					state->mouse.smileyDown = true;
				}
			}

			break;
		}
		case SDL_MOUSEMOTION: {
			int tx, ty;
			MousePosToTile(state, event->motion.x, event->motion.y, &tx, &ty);

			// if we are dragging
			if(state->mouse.down && !state->gameOver){
				// if we started hovering a new tile
				if(state->mouse.tileHoverX != -1 && (state->mouse.tileHoverX != tx || state->mouse.tileHoverY != ty)){
					int oldIndex = state->mouse.tileHoverX + state->mouse.tileHoverY * state->board.width;
					// mark old tile as unpressed
					state->board.tiles[oldIndex].state &= ~TILE_STATE_PRESSED;
				}
				if(tx != -1){
					int index = tx + ty * state->board.width;
					// mark new tile as pressed
					state->board.tiles[index].state |= TILE_STATE_PRESSED;
				}
			}

			// update hovered tile
			state->mouse.tileHoverX = tx;
			state->mouse.tileHoverY = ty;

			SDL_FRect mouseRect = (SDL_FRect) {
				.x = event->button.x,
				.y = event->button.y,
				.w = 1, .h = 1,
			};
			state->mouse.smileyHovered = SDL_HasIntersectionF(&state->layoutv2.smiley, &mouseRect);

			break;
		}

		case SDL_MOUSEBUTTONUP: {
			if(event->button.button == SDL_BUTTON_LEFT) {
				// if we clicked a tile
				if(!state->gameOver && state->mouse.tileHoverX != -1 && state->mouse.down){
					State_ClickTile(state, state->mouse.tileHoverX, state->mouse.tileHoverY);

					int index = state->mouse.tileHoverX + state->mouse.tileHoverY * state->board.width;
					state->board.tiles[index].state &= ~TILE_STATE_PRESSED;
				}

				state->mouse.down = false;

				if(state->mouse.smileyDown && state->mouse.smileyHovered){
					State_ClickSmiley(state);
				}

				state->mouse.smileyDown = false;
			}

			// RIGHT CLICKED TILE
			if(event->button.button == SDL_BUTTON_RIGHT){
				if(!state->gameOver){
					if(state->mouse.tileHoverX != -1){
						State_FlagTile(state, state->mouse.tileHoverX, state->mouse.tileHoverY);
					}
				}
			}
			break;
		}

		case SDL_SYSWMEVENT: {
			UINT msg = event->syswm.msg->msg.win.msg;
			LPARAM lParam = event->syswm.msg->msg.win.lParam;
			WPARAM wParam = event->syswm.msg->msg.win.wParam;
			HWND hwnd = event->syswm.msg->msg.win.hwnd;

			if(msg == WM_COMMAND && HIWORD(wParam) == 0){
				WORD id = LOWORD(wParam);
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
					}
				}
			}
			break;
		}
	}
}

void State_Update(State* state){
	// c0c0c0
	SDL_SetRenderDrawColor(state->sdl.renderer, 0xC0, 0xC0, 0xC0, 0xFF);
	SDL_RenderClear(state->sdl.renderer);

	// Draw UI
	SDL_FRect* pFRect;
	SDL_FRect fRect;

	pFRect = &state->layoutv2.smiley;
	SDL_Rect smileyRect;

	if(state->mouse.smileyDown && state->mouse.smileyHovered){
		smileyRect = state->images.tilesheet.smiley.pressed;
	}
	else if(state->gameOver){
		if(state->gameWon){
			smileyRect = state->images.tilesheet.smiley.win;
		}
		else{
			smileyRect = state->images.tilesheet.smiley.lose;
		}
	}
	else {
		if(state->mouse.down && state->mouse.tileHoverX != -1){
			smileyRect = state->images.tilesheet.smiley.surprise;
		}
		else{
			smileyRect = state->images.tilesheet.smiley.normal;
		}
	}

	SDL_RenderCopyF(
		state->sdl.renderer,
		state->images.tilesheet.texture,
		&smileyRect,
		pFRect
	);



	fRect = state->layoutv2.minesRemaining;
	fRect.w /= 3.0f;

	int minesLeft = state->board.nMines - state->board.minesFlagged;
	int place = 100;
	for(int i = 0; i < 3; ++i){
		if(minesLeft < 0 && i == 0){
			SDL_RenderCopyF(
				state->sdl.renderer,
				state->images.tilesheet.texture,
				&state->images.tilesheet.digitMinus,
				&fRect
			);
		}
		else {
			SDL_RenderCopyF(
				state->sdl.renderer,
				state->images.tilesheet.texture,
				&state->images.tilesheet.digit[(abs(minesLeft) / place) % 10],
				&fRect
			);
		}

		place /= 10;
		fRect.x += fRect.w;
	}


	fRect = state->layoutv2.time;
	fRect.w /= 3.0f;

	uint64_t time;
	if(state->gameOver){
		time = (state->ticksEnded - state->ticksStarted)/1000;
	}
	else if (state->gameStarted){
		time = (SDL_GetTicks64() - state->ticksStarted)/1000;
	}
	else{
		time = 0;
	}

	place = 100;
	for(int i = 0; i < 3; ++i){
		SDL_RenderCopyF(
			state->sdl.renderer,
			state->images.tilesheet.texture,
			&state->images.tilesheet.digit[(time / place) % 10],
			&fRect
		);

		place /= 10;
		fRect.x += fRect.w;
	}


	// Draw Tiles
	for(size_t i = 0; i < state->board.width * state->board.height; ++i){
		Tile* tile = &state->board.tiles[i];
		pFRect = &state->board.rects[i];

		SDL_Rect* tilesheetRect = NULL;

		if(tile->state & TILE_STATE_UNCOVERED){
			if(tile->state & TILE_STATE_MINE){
				tilesheetRect = &state->images.tilesheet.mineRed;
			}
			else {
				uint8_t surroundingMines = state->board.surroundingMines[i];
				if(surroundingMines == 0){
					tilesheetRect = &state->images.tilesheet.pressed;
				}
				else {
					tilesheetRect = &state->images.tilesheet.tileDigit[surroundingMines - 1];
				}
			}
		}
		else if(tile->state & TILE_STATE_FLAG){
			tilesheetRect = &state->images.tilesheet.flaged;
		}
		else if(tile->state & TILE_STATE_PRESSED){
			tilesheetRect = &state->images.tilesheet.pressed;
		}
		else{
			tilesheetRect = &state->images.tilesheet.normal;
			if(state->gameOver && tile->state & TILE_STATE_MINE){
				tilesheetRect = &state->images.tilesheet.mine;
			}
		}

		SDL_RenderCopyF(
			state->sdl.renderer,
			state->images.tilesheet.texture,
			tilesheetRect,
			pFRect
		);
	}

	struct Border {
		SDL_Rect* src;
		SDL_FRect* dst;
	} borders[] = {
		{ &state->images.tilesheet.border.lr, &state->layoutv2.border.uiLeft },
		{ &state->images.tilesheet.border.lr, &state->layoutv2.border.uiRight },
		{ &state->images.tilesheet.border.lr, &state->layoutv2.border.boardLeft },
		{ &state->images.tilesheet.border.lr, &state->layoutv2.border.boardRight },

		{ &state->images.tilesheet.border.ud, &state->layoutv2.border.uiTop },
		{ &state->images.tilesheet.border.ud, &state->layoutv2.border.uiBottom },
		{ &state->images.tilesheet.border.ud, &state->layoutv2.border.boardBottom },

		{ &state->images.tilesheet.border.ul, &state->layoutv2.border.uiTopLeft },
		{ &state->images.tilesheet.border.ur, &state->layoutv2.border.uiTopRight },
		{ &state->images.tilesheet.border.dl, &state->layoutv2.border.boardBottomLeft },
		{ &state->images.tilesheet.border.dr, &state->layoutv2.border.boardBottomRight },

		{ &state->images.tilesheet.border.urd, &state->layoutv2.border.uiBottomLeft },
		{ &state->images.tilesheet.border.uld, &state->layoutv2.border.uiBottomRight },
	};

	SDL_Rect r = state->images.tilesheet.border.ud;

	// draw borders
	for (int i = 0; i < sizeof(borders)/sizeof(*borders); ++i){
		struct Border* border = &borders[i];
		SDL_RenderCopyF(
			state->sdl.renderer,
			state->images.tilesheet.texture,
			border->src,
			border->dst
		);
	}


#ifdef KET_DEBUG
	//DrawLayoutOutlineV2(state);
#endif

	SDL_RenderPresent(state->sdl.renderer);
	if(!state->drewFirstFrame){
		state->drewFirstFrame = true;
		SDL_ShowWindow(state->sdl.window);
	}
}

void State_Destroy(State* state){
	if(state->sdl.renderer) SDL_DestroyRenderer(state->sdl.renderer);
	if(state->sdl.window) SDL_DestroyWindow(state->sdl.window);

	if(state->images.tilesheet.texture) SDL_DestroyTexture(state->images.tilesheet.texture);

	if(state->sdl.image.init) IMG_Quit();
	if(state->sdl.init) SDL_Quit();

	if(state->board.tiles) free(state->board.tiles);
	if(state->board.rects) free(state->board.rects);

	if(state->menu) DestroyMenu(state->menu);
}



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