#include <SDL.h>

#include <stdbool.h>

#ifdef KET_DEBUG
#include <stdio.h>
#endif

#include "State.h"
#include "Solver.h"

#include "Win.h"
#include <conio.h>

#include "Lua.h"

int main(int argc, char* argv[]){
#ifdef KET_DEBUG
	if(AllocConsole()){
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	}
#endif

	// Tile tiles[] = {
	// 	{1, 0}, {1, 0}, {1, 0},
	// 	{1, 1}, {1, 2}, {1, 2},
	// 	{1, 2}, {TILE_STATE_MINE, 0}, {TILE_STATE_MINE, 0},
	// 	{TILE_STATE_MINE, 0}, {1, 4}, {TILE_STATE_MINE, 0},
	// };
	// SolveParams solveParams = {
	// 	.width = 3,
	// 	.height = 4,
	// 	.mines = 4,
	// 	.tileClicked = { 0, 0 },
	// 	.tiles = tiles
	// };
	// Solve(&solveParams);

	/*
		_10
		_20
		_20
		_20
		_10

		110
		x20
		x20
		220
		x10
	*/

	// SolveStateTile tiles[] = {
	// 	{false, false, 1}, {false, false, 1}, {false, false, 0},
	// 	{false, false, 2}, {false, false, 2}, {false, false, 0},
	// 	{false, false, 2}, {false, false, 2}, {false, false, 0},
	// 	{false, false, 2}, {false, false, 2}, {false, false, 0},
	// 	{false, false, 1}, {false, false, 1}, {false, false, 0},
	// };

	/*
		2x2
		___
	*/
	// SolveStateTile tiles[] = {
	// 	{true, false, 2}, {false, true, 1}, {true, false, 2},
	// 	{false, false, 2}, {false, false, 2}, {false, false, 2},
	// };
	// SolveState solveState = {
	// 	.w = 3,
	// 	.h = 2,
	// 	.tiles = tiles,
	// 	.nMinesLeft = 1,
	// 	.log = true,
	// };
	// SolveParams solveParams = {
	// 	.state = &solveState,
	// 	.tileClicked = { -1, -1 },
	// };
	// printf("RESULT: %d\n", HasSolution(&solveParams));

	// PrintSolveState(&solveState);

	// _getch();
	// return 0;

	State state;
	State* statePtr = &state;
	if(!State_Init(statePtr)){
		State_Destroy(statePtr);
		return 1;
	}

	bool shouldQuit = false;
	while(!shouldQuit && !statePtr->shouldQuit) {
		SDL_Event event;
		while(SDL_PollEvent(&event)){
			if(event.type == SDL_QUIT) {
				shouldQuit = 1;
				break;
			}
			else{
				State_HandleEvent(statePtr, &event);
			}
		}

		State_Update(statePtr);
	}

	State_Destroy(statePtr);

#ifdef KET_DEBUG
	printf("Press any key to continue . . . ");
	getch();
#endif

	return 0;
}