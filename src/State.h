#pragma once

#include <stdbool.h>

#include <SDL.h>
#include <yoga/Yoga.h>

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
} State;

bool State_Init(State*);

void State_HandleEvent(State*, SDL_Event*);
void State_Update(State*);

void State_Destroy(State*);