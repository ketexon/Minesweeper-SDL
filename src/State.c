#include "State.h"
#include "Constants.h"

#include <stdio.h>
#include "Win.h"

#include <SDL_image.h>

SDL_Rect* LoadRectResource(const wchar_t* name){
	HRSRC src = FindResourceW(NULL, name, RC_TYPE_RECT);
	HGLOBAL data = LoadResource(NULL, src);
	void* mem = LockResource(data);
	return (SDL_Rect*) mem;
}

SDL_FRect RectToFRect(const SDL_Rect* rect){
	return (SDL_FRect) {
		.x = rect->x,
		.y = rect->y,
		.w = rect->w,
		.h = rect->h,
	};
}

bool State_Init(State* state){
	memset(state, 0, sizeof(*state));

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

	HRSRC tilesheetSrc = FindResourceW(NULL, RC_TILESHEET, RC_TYPE_IMAGE);
	HGLOBAL tilesheetData = LoadResource(NULL, tilesheetSrc);
	void* tilesheetMem = LockResource(tilesheetData);
	SDL_RWops* tilesheetRW = SDL_RWFromMem(tilesheetMem, SizeofResource(NULL, tilesheetSrc));

	SDL_Texture* tilesheet = IMG_LoadTextureTyped_RW(
		state->sdl.renderer,
		tilesheetRW,
		true,
		"PNG"
	);

	state->images.tilesheet.texture = tilesheet;
	state->images.tilesheet.normal = *LoadRectResource(RC_TILE_NORMAL);
	state->images.tilesheet.flaged = *LoadRectResource(RC_TILE_FLAGGED);
	state->images.tilesheet.bomb = *LoadRectResource(RC_TILE_BOMB);
	state->images.tilesheet.bombRed = *LoadRectResource(RC_TILE_BOMB_RED);
	state->images.tilesheet.digit[0] = *LoadRectResource(RC_TILE_1);
	state->images.tilesheet.digit[1] = *LoadRectResource(RC_TILE_2);
	state->images.tilesheet.digit[2] = *LoadRectResource(RC_TILE_3);
	state->images.tilesheet.digit[3] = *LoadRectResource(RC_TILE_4);
	state->images.tilesheet.digit[4] = *LoadRectResource(RC_TILE_5);
	state->images.tilesheet.digit[5] = *LoadRectResource(RC_TILE_6);
	state->images.tilesheet.digit[6] = *LoadRectResource(RC_TILE_7);
	state->images.tilesheet.digit[7] = *LoadRectResource(RC_TILE_8);

	return true;
}

void State_InitBoard(State* state){
	state->board.width = BOARD_WIDTH;
	state->board.height = BOARD_HEIGHT;

	size_t nTiles = state->board.width * state->board.height;

	state->board.tiles = malloc(sizeof(Tile) * nTiles);
	state->board.surroundingTiles = malloc(sizeof(char) * nTiles);
	state->board.rects = malloc(sizeof(SDL_FRect) * nTiles);

	for(size_t i = 0; i < nTiles; ++i){
		state->board.tiles[i] = (Tile) {
			.state = TILE_STATE_UNINITIALIZED
		};
	}

	State_RecalculateBoardLayout(state);
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

			break;
		}
		case SDL_MOUSEMOTION: {
			break;
		}
	}
}

void State_Update(State* state){
	SDL_SetRenderDrawColor(state->sdl.renderer, 255, 255, 255, 255);
	SDL_RenderClear(state->sdl.renderer);

	SDL_FRect rect;

	float offsetLeft = YGNodeLayoutGetLeft(state->yoga.minesweeper);
	float offsetTop = YGNodeLayoutGetTop(state->yoga.minesweeper);
	float width = YGNodeLayoutGetWidth(state->yoga.minesweeper);
	float height = YGNodeLayoutGetHeight(state->yoga.minesweeper);

	rect = (SDL_FRect) {
		.x = offsetLeft,
		.y = offsetTop,
		.w = width,
		.h = height
	};

	SDL_SetRenderDrawColor(state->sdl.renderer, 0, 0, 0, 255);
	SDL_RenderDrawRectF(state->sdl.renderer, &rect);

	float tileWidth = width / state->board.width;
	float tileHeight = height / state->board.height;

	rect.w = tileWidth;
	rect.h = tileHeight;

	SDL_RenderDrawRectsF(
		state->sdl.renderer,
		state->board.rects,
		state->board.width * state->board.height
	);

	for(size_t i = 0; i < state->board.width * state->board.height; ++i){
		Tile* tile = &state->board.tiles[i];
		SDL_FRect* pRect = &state->board.rects[i];

		SDL_Rect* tilesheetRect = NULL;
		if(tile->state & TILE_STATE_PRESSED){

		}
		else{

		}

		SDL_RenderCopyF(
			state->sdl.renderer,
			state->images.tilesheet.texture,
			&state->images.tilesheet.normal,
			pRect
		);
	}

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

	if(state->yoga.root) YGNodeFreeRecursive(state->yoga.root);

	if(state->board.tiles) free(state->board.tiles);
	if(state->board.rects) free(state->board.rects);
}