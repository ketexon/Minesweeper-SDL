#pragma once

#include <stdbool.h>

#include <SDL.h>
#include <yoga/Yoga.h>

typedef enum TileState {
	TILE_STATE_UNINITIALIZED	= 0b0000,
	TILE_STATE_INITIALIZED 		= 0b0001,
	TILE_STATE_MINE 			= 0b0010,
	TILE_STATE_FLAG				= 0b0100,
	TILE_STATE_PRESSED			= 0b1000
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

		struct {
			bool init;
		} image;
	} sdl;

	struct {
		YGNodeRef root;
		YGNodeRef minesweeper;
	} yoga;

	bool drewFirstFrame;

	struct {
		size_t width, height;
		Tile* tiles;
		char* surroundingTiles;
		SDL_FRect* rects;
	} board;

	struct {
		struct {
			SDL_Texture* texture;

			SDL_Rect normal;
			SDL_Rect pressed;
			SDL_Rect flaged;
			SDL_Rect bomb;
			SDL_Rect bombRed;
			SDL_Rect digit[8];
		} tilesheet;

	} images;
} State;

bool State_Init(State*);

void State_InitBoard(State*);
void State_InitLayout(State*);
void State_RecalculateBoardLayout(State*);

void State_RecalculateLayout(State*, int width, int height);

void State_HandleEvent(State*, SDL_Event*);
void State_Update(State*);

void State_Destroy(State*);