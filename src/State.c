#include "State.h"
#include "Constants.h"

#include <stdio.h>
#include <stdio.h>

float aspectRatio = 1.0f;

YGSize SquareMeasureFunc(
	YGNodeRef node,
	float w, YGMeasureMode widthMode,
	float h, YGMeasureMode heightMode
) {
	if(aspectRatio * h > w) return (YGSize) { w, w / aspectRatio };
	else return (YGSize) { h * aspectRatio, h };
}

bool State_Init(State* state){
	memset(state, 0, sizeof(*state));

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0){
		fprintf(stderr, "Could not init SDL.");
		return false;
	}

	state->sdl.init = true;

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

	state->yoga.root = YGNodeNew();
	YGNodeRef root = state->yoga.root;
	YGNodeStyleSetPadding(root, YGEdgeAll, 16);

	YGNodeStyleSetDisplay(root, YGDisplayFlex);
	YGNodeStyleSetFlexDirection(root, YGFlexDirectionRow);
	YGNodeStyleSetJustifyContent(root, YGJustifyCenter);
	YGNodeStyleSetAlignItems(root, YGAlignCenter);

	state->yoga.minesweeper = YGNodeNew();
	YGNodeRef minesweeperNode = state->yoga.minesweeper;
	YGNodeInsertChild(root, minesweeperNode, 0);

	YGNodeSetMeasureFunc(minesweeperNode, SquareMeasureFunc);

	State_InitBoard(state);

	State_RecalculateLayout(state, WINDOW_WIDTH, WINDOW_HEIGHT);

	return true;
}

void State_InitBoard(State* state){
	state->board.width = BOARD_WIDTH;
	state->board.height = BOARD_HEIGHT;

	size_t nTiles = state->board.width * state->board.height;

	state->board.data = malloc(sizeof(Tile) * nTiles);
	state->board.rects = malloc(sizeof(SDL_FRect) * nTiles);

	memset(state->board.data, (Tile) {
		.state = TILE_STATE_UNINITIALIZED
	}, nTiles);

	State_RecalculateBoardLayout(state);
}

void State_RecalculateBoardLayout(State* state) {
	float boardLeft = YGNodeLayoutGetLeft(state->yoga.minesweeper);
	float boardTop = YGNodeLayoutGetTop(state->yoga.minesweeper);
	float boardWidth = YGNodeLayoutGetWidth(state->yoga.minesweeper);
	float boardHeight = YGNodeLayoutGetHeight(state->yoga.minesweeper);

	float tileWidth = boardWidth / state->board.width;
	float tileHeight = boardHeight / state->board.height;

	for(size_t x = 0; x < state->board.width; ++x){
		for(size_t y = 0; y < state->board.height; ++y){
			size_t i = x + state->board.width * y;

			SDL_FRect* rect = &state->board.rects[i];
			rect->w = tileWidth;
			rect->h = tileHeight;

			rect->x = x * tileWidth + boardLeft;
			rect->y = y * tileWidth + boardTop;
		}
	}
}

void State_RecalculateLayout(State* state, int windowWidth, int windowHeight){
	YGNodeStyleSetWidth(state->yoga.root, (float) windowWidth);
	YGNodeStyleSetHeight(state->yoga.root, (float) windowHeight);
	YGNodeCalculateLayout(state->yoga.root, YGUndefined, YGUndefined, YGDirectionLTR);

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

	SDL_RenderPresent(state->sdl.renderer);
	if(!state->drewFirstFrame){
		state->drewFirstFrame = true;
		SDL_ShowWindow(state->sdl.window);
	}
}

void State_Destroy(State* state){
	if(state->sdl.renderer) SDL_DestroyRenderer(state->sdl.renderer);
	if(state->sdl.window) SDL_DestroyWindow(state->sdl.window);
	if(state->sdl.init) SDL_Quit();

	if(state->yoga.root) YGNodeFreeRecursive(state->yoga.root);

	if(state->board.data) free(state->board.data);
	if(state->board.rects) free(state->board.rects);
}