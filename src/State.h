#pragma once

#include "Win.h"

#include <stdbool.h>

#include <SDL.h>

typedef struct TilePosition {
	int x, y;
} TilePosition;

typedef enum TileState {
	TILE_STATE_UNINITIALIZED	= 0b00000,
	TILE_STATE_INITIALIZED 		= 0b00001,
	TILE_STATE_MINE 			= 0b00010,
	TILE_STATE_FLAG				= 0b00100,
	TILE_STATE_UNCOVERED		= 0b01000,
	TILE_STATE_PRESSED			= 0b10000,
	TILE_STATE_ANY				= 0b11111
} TileState;

typedef struct Tile {
	TileState state;
	uint8_t surroundingMines;
} Tile;

typedef struct State {
	bool shouldQuit;

	bool gameOver;
	bool gameWon;
	uint64_t ticksEnded;

	bool gameStarted;
	uint64_t ticksStarted;

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
		SDL_FRect content;
		SDL_FRect game;
		SDL_FRect board;
		SDL_FRect ui;
		SDL_FRect minesRemaining;
		SDL_FRect smiley;
		SDL_FRect time;
		struct {
			SDL_FRect uiBottom;
			SDL_FRect uiBottomRight;
			SDL_FRect uiRight;
			SDL_FRect uiTopRight;
			SDL_FRect uiTop;
			SDL_FRect uiTopLeft;
			SDL_FRect uiLeft;
			SDL_FRect uiBottomLeft;


			SDL_FRect boardLeft;
			SDL_FRect boardBottomLeft;
			SDL_FRect boardBottom;
			SDL_FRect boardBottomRight;
			SDL_FRect boardRight;
		} border;
	} layoutv2;

	bool drewFirstFrame;

	struct {
		size_t width, height;
		Tile* tiles;
		SDL_FRect* rects;

		int nMines;
		int tilesLeft;
		int minesFlagged;
	} board;

	struct {
		struct {
			struct {
				SDL_Texture* original;
				SDL_Texture* cute;
				SDL_Texture* custom;
			} sourceTextures;
			SDL_Texture* texture;

			SDL_Rect normal;
			SDL_Rect pressed;
			SDL_Rect flaged;
			SDL_Rect mine;
			SDL_Rect mineRed;
			SDL_Rect tileDigit[8];
			SDL_Rect digit[10];
			SDL_Rect digitMinus;

			struct {
				SDL_Rect normal;
				SDL_Rect pressed;
				SDL_Rect win;
				SDL_Rect lose;
				SDL_Rect surprise;
			} smiley;

			struct {
				SDL_Rect ur;
				SDL_Rect ul;
				SDL_Rect dr;
				SDL_Rect dl;

				SDL_Rect lr;
				SDL_Rect ud;

				SDL_Rect uld;
				SDL_Rect urd;
			} border;

			SDL_Rect background;
		} tilesheet;

	} images;

	SDL_Color backgroundColor;

	struct {
		bool down;
		int tileHoverX, tileHoverY;
		bool smileyDown;
		bool smileyHovered;
	} mouse;

	HMENU menu;
} State;

bool State_Init(State*);

void State_CreateMenu(State*);

void State_InitBoard(State*);
void State_InitLayout(State*);

void State_CreateBoard(State*);

void State_RecalculateBoardLayout(State*);
void State_RecalculateLayout(State*, int width, int height);

void State_HandleEvent(State*, SDL_Event*);
void State_HandleMenuEvent(State*, HWND, WORD id);
void State_Update(State*);

void State_DestroyBoard(State*);

void State_Destroy(State*);