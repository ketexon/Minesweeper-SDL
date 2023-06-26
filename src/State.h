#pragma once

#include <stdbool.h>

#include <SDL.h>
#include <yoga/Yoga.h>

typedef enum TileState {
	TILE_STATE_UNINITIALIZED	= 0b000,
	TILE_STATE_INITIALIZED 		= 0b001,
	TILE_STATE_MINE 			= 0b010,
	TILE_STATE_FLAG				= 0b100,
} TileState;

typedef struct Tile {
	TileState state;
} Tile;

typedef struct State {
	struct {
		bool init;
		SDL_Window* window;
		SDL_Renderer* renderer;

		bool drewFirstFrame;
	} sdl;

	struct {
		YGNodeRef root;
		YGNodeRef minesweeper;
	} yoga;

	bool drewFirstFrame;

	struct {
		size_t width, height;
		Tile* data;
		SDL_FRect* rects;
	} board;
} State;

bool State_Init(State*);

void State_InitBoard(State*);
void State_RecalculateBoardLayout(State*);

void State_RecalculateLayout(State*);

void State_HandleEvent(State*, SDL_Event*);
void State_Update(State*);

void State_Destroy(State*);