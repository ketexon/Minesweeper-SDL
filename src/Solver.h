#pragma once

// https://informatika.stei.itb.ac.id/~rinaldi.munir/Stmik/2021-2022/Makalah/Makalah-IF2211-Stima-2022-K2%20(30).pdf

#include <stdbool.h>
#include <stdint.h>

#include "State.h"

// https://massaioli.wordpress.com/2013/01/12/solving-minesweeper-with-matricies/

typedef struct SolveStateTile {
	bool uncovered;
	bool flagged;
	uint8_t surroundingMines;
} SolveStateTile;

typedef struct SolveState {
	int w, h;
	SolveStateTile* tiles;
	int nMinesLeft;
	bool log;
} SolveState;

typedef struct SolveParams {
	SolveState* state;
	struct {
		int x, y;
	} tileClicked;
	int maxIters;
} SolveParams;

void PrintSolveState(SolveState* state);

bool HasSolution(SolveParams*);