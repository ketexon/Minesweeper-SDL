#include "Solver.h"

#include "State.h"

#include <stdio.h>
#include <stdlib.h>

#include "Matrix.h"

bool UpdateSurroundingTiles(SolveState* state, int x, int y);

bool FlagTileAtIndex(SolveState* state, int index){
	if(state->tiles[index].flagged) return false;
	if(state->log) printf("Flagging %d %d\n", index % state->w, index / state->w);
	state->tiles[index].flagged = true;
	--state->nMinesLeft;
	UpdateSurroundingTiles(state, index % state->w, index / state->w);
	return true;
}

bool ClearTile(SolveState* state, int x, int y) {
	int index = x + y * state->w;
	SolveStateTile* tile = &state->tiles[index];

	if(tile->uncovered) return false;

	tile->uncovered = true;
	if(tile->surroundingMines == 0){
		for(int newX = x - 1; newX <= x + 1; ++newX){
			for(int newY = y - 1; newY <= y + 1; ++newY){
				if((newX == x && newY == y) || newX < 0 || newY < 0 || newX >= state->w || newY >= state->h) continue;
				ClearTile(state, newX, newY);
			}
		}
	}
	return true;
}

bool ClearTileAtIndex(SolveState* state, int index){
	return ClearTile(state, index % state->w, index / state->w);
}

#define REQUIRE 1
#define DISCLUDE 0
#define ANY -1

int CountSurroundingTiles(SolveState* state, int x, int y, int includeFlagged, int includeUncovered);

bool ClearSurroundingTiles(SolveState* state, int x, int y) {
	bool madeChanges = false;
	SolveStateTile* tile = &state->tiles[x + y * state->w];

	if(state->log) printf("Checking tiles around %d %d to clear...\n", x, y);

	if(CountSurroundingTiles(state, x, y, REQUIRE, DISCLUDE) < tile->surroundingMines) return false;

	if(state->log) printf("Clearing tiles around %d %d\n", x, y);

	for(int newX = x - 1; newX <= x + 1; ++newX){
		for(int newY = y - 1; newY <= y + 1; ++newY){
			if((newX == x && newY == y) || newX < 0 || newY < 0 || newX >= state->w || newY >= state->h) continue;
			SolveStateTile* newTile = &state->tiles[newX + newY * state->w];
			if(newTile->flagged || newTile->uncovered) {
				continue;
			};
			madeChanges = ClearTile(state, newX, newY) || madeChanges;
			madeChanges = ClearSurroundingTiles(state, newX, newY) || madeChanges;
		}
	}
	return madeChanges;
}

bool UpdateSurroundingTiles(SolveState* state, int x, int y) {
	bool madeChanges = false;
	SolveStateTile* tile = &state->tiles[x + y * state->w];

	if(state->log) printf("Trying to update tiles around %d %d\n", x, y);

	for(int newX = x - 1; newX <= x + 1; ++newX){
		for(int newY = y - 1; newY <= y + 1; ++newY){
			if((newX == x && newY == y) || newX < 0 || newY < 0 || newX >= state->w || newY >= state->h) continue;

			SolveStateTile* newTile = &state->tiles[newX + newY * state->w];
			if(!newTile->uncovered) {
				continue;
			};

			ClearSurroundingTiles(state, newX, newY);
		}
	}
	return madeChanges;
}

int CountSurroundingTiles(SolveState* state, int x, int y, int includeFlagged, int includeUncovered) {
	int n = 0;
	for(int newX = x - 1; newX <= x + 1; ++newX){
		for(int newY = y - 1; newY <= y + 1; ++newY){
			if((newX == x && newY == y) || newX < 0 || newY < 0 || newX >= state->w || newY >= state->h) continue;
			SolveStateTile* tile = &state->tiles[newX + newY * state->w];

			if(includeFlagged == REQUIRE && !tile->flagged) continue;
			if(includeFlagged == DISCLUDE && tile->flagged) continue;
			if(includeUncovered == REQUIRE && !tile->uncovered) continue;
			if(includeUncovered == DISCLUDE && tile->uncovered) continue;

			++n;
		}
	}
	return n;
}

void PrintSolveState(SolveState* state){
	for(int y = 0; y < state->h; ++y){
		for(int x = 0; x < state->w; ++x){
			SolveStateTile* tile = &state->tiles[x + y * state->w];
			if(tile->uncovered){
				printf("%c", '0' + tile->surroundingMines);
			}
			else if(tile->flagged){
				printf("x");
			}
			else{
				printf("_");
			}
		}
		printf("\n");
	}
	printf("\n");
}

void SetInsert(int* set, size_t* size, int newValue) {
	int start = 0, end = *size - 1;
	while(true){
		int middle = (end + start)/2;

		// 2 elements left
		// manually search at this point
		if(middle == start || middle == end){
			if(set[start] == newValue || set[end] == newValue) return;
			int insertPos = set[end] < newValue ? end + 1 : set[start] < newValue ? start + 1 : start;

			for(int i = *size; i > insertPos; --i){
				set[i] = set[i - 1];
			}
			set[insertPos] = newValue;
			++(*size);
			return;
		}

		if(set[middle] > newValue){
			end = middle - 1;
		}
		else if(set[middle] < newValue){
			start = middle + 1;
		}
		else {
			return;
		}
	}
}

bool SolveIter(SolveState* state, bool phase2){
	if(state->log) printf("===Phase%d===\n", phase2 ? 2 : 1);

	if(state->log) PrintSolveState(state);

	bool madeChanges = false;

	SolveStateTile* tiles = state->tiles;

	// find unresolved tiles
	size_t unresolvedTilesSize = 0;
	size_t unresolvedTilesCap = 64;
	int* unresolvedTiles = malloc(unresolvedTilesCap * sizeof(*unresolvedTiles));

	if(state->log) printf("Finding unresolved tiles...\n");
	for(int x = 0; x < state->w; ++x){
		for(int y = 0; y < state->h; ++y){
			int index = x + y * state->w;

			// if it has mines around it
			if(tiles[index].uncovered && tiles[index].surroundingMines > 0){
				// if we havent flagged all the mines around a tile
				if(tiles[index].surroundingMines != CountSurroundingTiles(state, x, y, REQUIRE, DISCLUDE)) {
					if(unresolvedTilesSize == unresolvedTilesCap){
						unresolvedTilesCap *= 2;
						unresolvedTiles = realloc(unresolvedTiles, sizeof(*unresolvedTiles) * unresolvedTilesCap);
					}
					SetInsert(unresolvedTiles, &unresolvedTilesSize, index);
					if(state->log) printf("\tFound: %d %d\n", x, y);
				}
			}
		}
	}


	// find candidate mine spots
	size_t candidatesSize = 0;
	size_t candidatesCap = 64;
	int* candidates = malloc(candidatesCap * sizeof(*candidates));
	if(state->log) printf("Finding candidate tiles...\n");
	if(!phase2){
		// for every tile
		for(int i = 0; i < unresolvedTilesSize; ++i){
			int index = unresolvedTiles[i];
			int x = index % state->w;
			int y = index / state->w;

			for(int sx = x - 1; sx <= x + 1; ++sx){
				for(int sy = y - 1; sy <= y + 1; ++sy){
					if((sx == x && sy == y) || sx < 0 || sy < 0 || sx >= state->w || sy >= state->h) continue;
					int sIndex = sx + sy * state->w;
					if(!tiles[sIndex].uncovered && !tiles[sIndex].flagged) {
						if(candidatesCap == candidatesSize){
							candidatesCap *= 2;
							candidates = realloc(candidates, candidatesCap * sizeof(*candidates));
						}
						SetInsert(candidates, &candidatesSize, sIndex);
						if(state->log) printf("\tFound: %d %d\n", sx, sy);
					}
				}
			}
		}
	}
	else {
		// find candidate mine spots (all covered tiles)
		// for every tile
		for(int i = 0; i < state->w * state->h; ++i){
			if(!state->tiles[i].uncovered && !state->tiles[i].flagged) {
				if(candidatesCap == candidatesSize){
					candidatesCap *= 2;
					candidates = realloc(candidates, candidatesCap * sizeof(*candidates));
				}

				candidates[candidatesSize++] = i;
				if(state->log) printf("\tFound: %d %d\n", i % state->w, i / state->w);
			}
		}
	}

	// create matrix for rref
	Matrix* mat = Matrix_New(unresolvedTilesSize + (phase2 ? 1 : 0), candidatesSize + 1);

	for(int r = 0; r < unresolvedTilesSize; ++r){
		int unresolvedTileIndex = unresolvedTiles[r];
		int ux = unresolvedTileIndex % state->w;
		int uy = unresolvedTileIndex / state->w;

		for(int c = 0; c < candidatesSize; ++c){
			int candidateIndex = candidates[c];
			int cx = candidateIndex % state->w;
			int cy = candidateIndex / state->w;

			if(abs(cx - ux) <= 1 && abs(cy - uy) <= 1){
				*Matrix_Get(mat, r, c) = 1;
			}
		}
		*Matrix_Get(mat, r, candidatesSize) = state->tiles[unresolvedTileIndex].surroundingMines - CountSurroundingTiles(state, ux, uy, REQUIRE, DISCLUDE);
	}

	if(phase2) {
		// sum of all uncleared tiles must be equal to mines left
		for(int c = 0; c < candidatesSize; ++c){
			*Matrix_Get(mat, unresolvedTilesSize, c) = 1;
		}
		*Matrix_Get(mat, unresolvedTilesSize, candidatesSize) = state->nMinesLeft;
	}

	if(state->log) printf("Original matrix:\n");
	if(state->log) Matrix_Print(mat);

	Matrix_RREF(mat);

	if(state->log) printf("RREF matrix:\n");
	if(state->log) Matrix_Print(mat);

	// for each row, the upper bound is the sum of the positive entries and lower is sum of negative entries
	for(int r = 0; r < mat->r; ++r){
		int lowerBound = 0, upperBound = 0;
		bool allSameSign = true;
		int sign = 0;
		for(int c = 0; c < mat->c - 1; ++c){
			int v = *Matrix_Get(mat, r, c);
			if(v > 0) {
				upperBound += v;
				if(sign < 0) allSameSign = false;
				else if(sign == 0){
					sign = 1;
				}
			}
			else if(v < 0) {
				lowerBound += v;
				if(sign > 0) allSameSign = false;
				else if(sign == 0){
					sign = -1;
				}
			}
		}
		if(state->log) printf("Bounds for row %d: L: %d, U: %d\n", r, lowerBound, upperBound);
		int numMines = *Matrix_Get(mat, r, mat->c - 1);
		// number of mines = lower bound -> all negative entries are mines
		if(numMines == lowerBound){
			for(int c = 0; c < mat->c - 1; ++c){
				int v = *Matrix_Get(mat, r, c);
				if(v < 0) {
					madeChanges = true;
					FlagTileAtIndex(state, candidates[c]);
				}
			}
		}
		// number of mines = upper bound -> all positive entries are mines
		else if(numMines == upperBound){
			for(int c = 0; c < mat->c - 1; ++c){
				int v = *Matrix_Get(mat, r, c);
				if(v > 0) {
					madeChanges = true;
					FlagTileAtIndex(state, candidates[c]);
				}
			}
		}
		if(numMines == 0 && allSameSign){
			for(int c = 0; c < mat->c - 1; ++c){
				int v = *Matrix_Get(mat, r, c);
				if(v != 0) {
					madeChanges = true;
					ClearTileAtIndex(state, candidates[c]);
				}
			}
		}
	}

	if(state->log) printf("Flag result:\n");
	if(state->log) PrintSolveState(state);

	bool clearedTiles;
	do {
		clearedTiles = false;

		// now we clear tiles based on flags
		// no mines left: clear all tiles
		if(state->nMinesLeft == 0){
			for(int i = 0; i < state->w * state->h; ++i){
				int x = i % state->w;
				int y = i / state->w;
				SolveStateTile* tile = &state->tiles[i];
				if(!tile->flagged && !tile->uncovered){
					clearedTiles = ClearTile(state, x, y) || clearedTiles;
				}
			}
		}
		else{
			// now we clear tiles based on flags
			for(int i = 0; i < unresolvedTilesSize; ++i){
				int index = unresolvedTiles[i];
				int x = index % state->w;
				int y = index / state->w;

				clearedTiles = ClearSurroundingTiles(state, x, y) || clearedTiles;
			}
		}
	} while(clearedTiles);

	madeChanges = madeChanges || clearedTiles;

	if(state->log) printf("Clear result:\n");
	if(state->log) PrintSolveState(state);

	Matrix_Free(mat);

	free(candidates);
	free(unresolvedTiles);

	return madeChanges;
}

// https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
int RoundUpToPowerOf2(int value){
	unsigned int v = value;

	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	return v;
}

bool HasSolution(SolveParams* params, TilePosition** unsolvableTilesOut, size_t* unsolvableTilesLenOut) {
	SolveState* state = params->state;
	SolveStateTile* tiles = state->tiles;

	if(params->tileClicked.x != -1){
		ClearTile(state, params->tileClicked.x, params->tileClicked.y);
	}


	int i = 0;
	while(true){
		bool madeChanges = SolveIter(state, false);

		if(!madeChanges) {
			madeChanges = SolveIter(state, true);
		}
		if(!madeChanges) break;
		if(params->maxIters > 0 && i++ > params->maxIters) break;
	}

	if(state->nMinesLeft != 0){
		// unsolvable
		// fill unsolvableTiles
		size_t cap = (size_t) RoundUpToPowerOf2(state->nMinesLeft * 2);
		if(cap < 16) cap = 16;
		size_t len = 0;
		TilePosition* unsolvableTiles = malloc(cap * sizeof(*unsolvableTiles));
		for(int y = 0; y < state->h; ++y){
			for(int x = 0; x < state->w; ++x){
				int index = x + y * state->w;
				if(!tiles[index].uncovered) {
					// check if the tile is near an uncovered tile
					bool nearUncovered = false;
					for(int i = 0; i < 9; ++i){
						int dx = i % 3 - 2;
						int dy = i % 3 - 2;
						int newX = x + dx, newY = y + dy;
						if((dx == 0 && dy == 0) || newX < 0 || newY < 0 || newX >= state->w || newY >= state->h){
							continue;
						}
						if(state->tiles[newX + state->w * newY].uncovered) {
							nearUncovered = true;
							break;
						}
					}

					if(nearUncovered){
						if(len >= cap){
							cap *= 2;
							unsolvableTiles = realloc(unsolvableTiles, cap * sizeof(*unsolvableTiles));
						}
						unsolvableTiles[len++] = (TilePosition) { x, y };
					}
				}
			}
		}

		*unsolvableTilesOut = unsolvableTiles;
		*unsolvableTilesLenOut = len;

		return false;
	}
	else{
		*unsolvableTilesOut = NULL;
		*unsolvableTilesLenOut = 0;
		return true;
	}
}