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

#include "Lua.h"
#include "Solver.h"

bool State_StartGame(State* state, int tileX, int tileY);

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

	HRESULT hr = CoInitializeEx(NULL, 0);
	if(FAILED(hr)){
		fprintf(stderr, "Could not initialize COM.");
		return false;
	}

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

void State_ClearBoard(State* state){
	for(int i = 0; i < state->board.width * state->board.height; ++i) {
		state->board.tiles[i].state = TILE_STATE_UNINITIALIZED;
		state->board.tiles[i].surroundingMines = 0;
	}
}

void State_CreateBoard(State* state){
	size_t nTiles = state->board.width * state->board.height;

	state->board.tiles = malloc(sizeof(Tile) * nTiles);
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
	if(state->board.rects) free(state->board.rects);
}

void State_ResetBoard(State* state){
	State_DestroyBoard(state);
	State_CreateBoard(state);

	int windowWidth, windowHeight;
	SDL_GetWindowSizeInPixels(state->sdl.window, &windowWidth, &windowHeight);
	State_RecalculateLayout(state, windowWidth, windowHeight);
}

void State_GenerateFlagsDefault(State* state);

/**
 * @param solveStateTilesBuffer Buffer to be used for solve state to hold tiles
 * @param problematicTiles Tiles that could not be solved
 */
bool State_HasSolution(State* state, SolveStateTile* solveStateTilesBuffer, int tileX, int tileY, TilePosition** problematicTiles, size_t* nProblematicTiles){
	for(int i = 0; i < state->board.width * state->board.height; ++i){
		solveStateTilesBuffer[i].flagged = state->board.tiles[i].state & TILE_STATE_FLAG;
		solveStateTilesBuffer[i].uncovered = state->board.tiles[i].state & TILE_STATE_UNCOVERED;
		solveStateTilesBuffer[i].surroundingMines = state->board.tiles[i].surroundingMines;
	}

	SolveState solveState = {
		.w = state->board.width,
		.h = state->board.height,
		.nMinesLeft = state->board.nMines - state->board.minesFlagged,
		.tiles = solveStateTilesBuffer,
		.log = false,
	};

	SolveParams solveParams = {
		.state = &solveState,
		.tileClicked = { tileX, tileY },
		.maxIters = state->board.nMines / 2,
	};

	return HasSolution(&solveParams, problematicTiles, nProblematicTiles);
}

bool State_EnsureSolvableDefault(State* state, int tileX, int tileY){
	SolveStateTile* sstBuffer = malloc(state->board.width * state->board.height * sizeof(*sstBuffer));

	TilePosition* problematicTiles;
	size_t nProblematicTiles;
	bool hasSolution = State_HasSolution(state, sstBuffer, tileX, tileY, &problematicTiles, &nProblematicTiles);

	TilePosition* candidateBuffer = malloc((SOLVER_MAX_PERTURBATION_DISTANCE * 2 + 1) * (SOLVER_MAX_PERTURBATION_DISTANCE * 2 + 1) * sizeof(*candidateBuffer));

	int nPerturbations = 0;
	while(!hasSolution && problematicTiles && nPerturbations++ < SOLVER_MAX_PERTURBATIONS){
		// printf("Perturbation: %d\n", nPerturbations);
		// Perturb problematic tiles

		// iterate problematic tiles
		for(int i = 0; i < nProblematicTiles; ++i){
			TilePosition tilePosition = problematicTiles[i];
			int x = tilePosition.x, y = tilePosition.y;
			Tile* tile = &state->board.tiles[x + y * state->board.width];

			// if its a mine, relocate
			if(tile->state & TILE_STATE_MINE){
				// count potential surrounding tiles
				int nCandidates = 0;
				for(int j = 0; j < (SOLVER_MAX_PERTURBATION_DISTANCE * 2 + 1) * (SOLVER_MAX_PERTURBATION_DISTANCE * 2 + 1); ++j){
					int dx = j % (SOLVER_MAX_PERTURBATION_DISTANCE * 2 + 1) - SOLVER_MAX_PERTURBATION_DISTANCE;
					int dy = j / (SOLVER_MAX_PERTURBATION_DISTANCE * 2 + 1) - SOLVER_MAX_PERTURBATION_DISTANCE;

					if(abs(dx) < SOLVER_MIN_PERTURBATION_DISTANCE || abs(dy) < SOLVER_MIN_PERTURBATION_DISTANCE) continue;

					int newX = x + dx;
					int newY = y + dy;
					if(
						newX < 0 || newX >= state->board.width
						|| newY < 0 || newY >= state->board.height
						|| newX > tileX - BOARD_CLICK_SAFE_AREA && newX < tileX + BOARD_CLICK_SAFE_AREA
						|| newY > tileY - BOARD_CLICK_SAFE_AREA && newY < tileX + BOARD_CLICK_SAFE_AREA
					){
						continue;
					}

					// dont move to where there's already a mine
					if(state->board.tiles[newX + newY * state->board.width].state & TILE_STATE_MINE) continue;

					bool isProblematic = false;
					// dont move to a problematic tile
					for(int k = 0; k < nProblematicTiles; ++k){
						if(problematicTiles[k].x == x + dx && problematicTiles[k].y == y + dy) {
							isProblematic = true;
							break;
						}
					}
					if(isProblematic) continue;

					candidateBuffer[nCandidates++] = (TilePosition) { .x = newX, .y = newY };
				}

				if(nCandidates != 0){
					int positionIndex = (int)((double) rand() / (RAND_MAX + 1) * nCandidates);
					TilePosition position = candidateBuffer[positionIndex];
					state->board.tiles[position.x + position.y * state->board.width].state |= TILE_STATE_MINE;
					state->board.tiles[x + y * state->board.width].state &= ~TILE_STATE_MINE;
				}
			}
		}

		free(problematicTiles);

		// recalculate flags
		State_GenerateFlagsDefault(state);
		hasSolution = State_HasSolution(state, sstBuffer, tileX, tileY, &problematicTiles, &nProblematicTiles);
	}

	free(candidateBuffer);

	free(sstBuffer);
	if(problematicTiles) free(problematicTiles);

	return hasSolution;
}

void State_GenerateFlagsDefault(State* state){
	// generate adjacent mine counts
	// also set init flag
	for(int tx = 0; tx < state->board.width; ++tx){
		for(int ty = 0; ty < state->board.height; ++ty){
			int index = tx + ty * state->board.width;
			int minesSurroundingTile = 0;

			for(int x = tx - 1; x <= tx + 1; ++x){
				for(int y = ty - 1; y <= ty + 1; ++y){
					if(x < 0 || x >= state->board.width || y < 0 || y >= state->board.height) {
						continue;
					}
					if(state->board.tiles[x + y * state->board.width].state & TILE_STATE_MINE) {
						++minesSurroundingTile;
					}
				}

				state->board.tiles[index].surroundingMines = minesSurroundingTile;

				state->board.tiles[index].state |= TILE_STATE_INITIALIZED;
			}
		}
	}
}

void State_GenerateMinesDefault(State* state, int tileX, int tileY){
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
}

void State_CreateGameDefault(State* state, int tileX, int tileY){
	State_GenerateMinesDefault(state, tileX, tileY);
	State_GenerateFlagsDefault(state);

	if(!State_EnsureSolvableDefault(state, tileX, tileY)){
		State_ClearBoard(state);
		State_CreateGameDefault(state, tileX, tileY);
	}
}

bool State_CreateGameCustom(State* state, int tileX, int tileY){
	return State_Lua_GenerateBoard(state, tileX, tileY);
}

// tileX,Y is the tile clicked to start the game
// we must guarentee that there are no mines within 3x3 of that tile
bool State_CreateGame(State* state, int tileX, int tileY){
	if(state->game.mode == GAMEMODE_DEFAULT){
		State_CreateGameDefault(state, tileX, tileY);
		return true;
	}
	else {
		return State_CreateGameCustom(state, tileX, tileY);
	}
}

bool State_StartGame(State* state, int tileX, int tileY){
	srand(time(NULL));

	if(State_CreateGame(state, tileX, tileY)){
		state->ticksStarted = SDL_GetTicks64();
		state->gameStarted = true;
		return true;
	}
	else{
		return false;
	}
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

	uint8_t surroundingMines = state->board.tiles[index].surroundingMines;
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
		if(!State_StartGame(state, tileX, tileY)) return;
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
				State_HandleMenuEvent(state, hwnd, id);
			}
			break;
		}
	}
}

void State_Update(State* state){
	SDL_SetRenderDrawColor(
		state->sdl.renderer,
		state->backgroundColor.r,
		state->backgroundColor.g,
		state->backgroundColor.b,
		state->backgroundColor.a
	);
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
				uint8_t surroundingMines = state->board.tiles[i].surroundingMines;
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

	SDL_Texture** sourceTextures = &state->images.tilesheet.sourceTextures.original;
	size_t nSourceTextures = sizeof(state->images.tilesheet.sourceTextures) / sizeof(*sourceTextures);
	for(int i = 0; i < nSourceTextures; ++i){
		SDL_Texture* sourceTexture = sourceTextures[i];
		if(sourceTexture != NULL) SDL_DestroyTexture(sourceTexture);
	}

	if(state->sdl.image.init) IMG_Quit();
	if(state->sdl.init) SDL_Quit();

	if(state->board.tiles) free(state->board.tiles);
	if(state->board.rects) free(state->board.rects);

	if(state->menu) DestroyMenu(state->menu);

	State_DestroyLua(state);

	CoUninitialize();
}