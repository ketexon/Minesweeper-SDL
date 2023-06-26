#include "State.h"
#include "Constants.h"

#include <stdio.h>
#include <stdio.h>

bool State_Init(State* state){
	memset(state, 0, sizeof(*state));

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0){
		fprintf(stderr, "Could not init SDL.");
		return false;
	}

	state->sdl.init = true;

	state->sdl.window = SDL_CreateWindow(
		WINDOW_TITLE,
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
		NULL,
		SDL_RENDERER_ACCELERATED
	);

	if(state->sdl.renderer == NULL){
		fprintf(stderr, "Could not create renderer.");
		return false;
	}

	state->yoga.root = YGNodeNew();
	YGNodeRef root = state->yoga.root;
	YGNodeStyleSetWidth(root, WINDOW_WIDTH);
	YGNodeStyleSetHeight(root, WINDOW_HEIGHT);
	YGNodeStyleSetPadding(root, YGEdgeAll, 5.0f);

	YGNodeStyleSetDisplay(root, YGDisplayFlex);

	state->yoga.minesweeper = YGNodeNew();
	YGNodeRef minesweeperNode = state->yoga.minesweeper;
	YGNodeInsertChild(state->yoga.root, minesweeperNode, 0);

	YGNodeStyleSetFlex(minesweeperNode, 1.0f);
	YGNodeStyleSetAspectRatio(minesweeperNode, 1.0f);

	return true;
}

void State_HandleEvent(State* state, SDL_Event* event){
	switch(event->type){
		case SDL_EVENT_WINDOW_RESIZED:
			YGNodeStyleSetWidth(state->yoga.root, (float) event->window.data1);
			YGNodeStyleSetHeight(state->yoga.root, (float) event->window.data2);
			break;
	}
}

void State_Update(State* state){
	SDL_SetRenderDrawColor(state->sdl.renderer, 255, 255, 255, 255);
	SDL_RenderClear(state->sdl.renderer);

	SDL_SetRenderDrawColor(state->sdl.renderer, 0, 0, 0, 255);
	SDL_RenderDrawRect_renamed_SDL_RenderRect(
		(SDL_Rect) {
			.x = YGNodeLayoutGetLeft(state->yoga.minesweeper),
			.y = YGNodeLayoutGetTop(state->yoga.minesweeper),
			.w = YGNodeLayoutGetWidth(state->yoga.minesweeper),
			.h = YGNodeLayoutGetHeight(state->yoga.minesweeper),
		}
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
}