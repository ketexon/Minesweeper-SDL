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
void DrawLayoutNodeOutline(State* state, YGNodeRef node){
	SDL_FRect rect = Node_GetNodeScreenRect(node);

	SDL_SetRenderDrawColor(state->sdl.renderer, 255, 0, 0, 255);
	SDL_RenderDrawRectF(state->sdl.renderer, &rect);
}

void DrawLayoutNodeOutlineRecursive(State* state, YGNodeRef root){
	int nChildren = YGNodeGetChildCount(root);
	for(int i = 0; i < nChildren; ++i){
		DrawLayoutNodeOutlineRecursive(state, YGNodeGetChild(root, i));
	}

	DrawLayoutNodeOutline(state, root);
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
	YGNodeRef node = state->layout.board;
	SDL_FRect nodeRect = Node_GetNodeScreenRect(node);

	float tileWidth = nodeRect.w / state->board.width;
	float tileHeight = nodeRect.h / state->board.height;

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
	int tx = floorf((mouseX - nodeRect.x)/tileWidth);
	int ty = floorf((mouseY - nodeRect.y)/tileHeight);

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

	State_RecalculateBoardLayout(state);
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
	State_RecalculateBoardLayout(state);
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

				SDL_FRect smileyRect = Node_GetNodeScreenRect(state->layout.smiley);
				SDL_FRect mouseRect = (SDL_FRect) {
					.x = event->button.x,
					.y = event->button.y,
					.w = 1, .h = 1,
				};
				if(SDL_HasIntersectionF(&smileyRect, &mouseRect)){
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

			SDL_FRect smileyRect = Node_GetNodeScreenRect(state->layout.smiley);
			SDL_FRect mouseRect = (SDL_FRect) {
				.x = event->button.x,
				.y = event->button.y,
				.w = 1, .h = 1,
			};
			state->mouse.smileyHovered = SDL_HasIntersectionF(&smileyRect, &mouseRect);

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
	SDL_FRect fRect;

	fRect = Node_GetNodeScreenRect(state->layout.smiley);
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
		smileyRect = state->images.tilesheet.smiley.normal;
	}

	SDL_RenderCopyF(
		state->sdl.renderer,
		state->images.tilesheet.texture,
		&smileyRect,
		&fRect
	);



	fRect = Node_GetNodeScreenRect(state->layout.minesLeft);
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


	fRect = Node_GetNodeScreenRect(state->layout.time);
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
		SDL_FRect* pRect = &state->board.rects[i];

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
			pRect
		);
	}

#ifdef KET_DEBUG
	DrawLayoutNodeOutlineRecursive(state, state->layout.root);
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

	if(state->layout.root) YGNodeFreeRecursive(state->layout.root);

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