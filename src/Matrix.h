#pragma once

#include <stdlib.h>

typedef struct Matrix {
	size_t r, c;
	int* data;
} Matrix;

Matrix* Matrix_New(size_t r, size_t c);

int* Matrix_Get(Matrix*, size_t r, size_t c);

void Matrix_Print(Matrix*);

void Matrix_RREF(Matrix*);

void Matrix_SwapRows(Matrix*, int r1, int r2);
// void Matrix_ScaleRow(Matrix*, int r, int scale);

// r1 = r1 - r1
void Matrix_SubRows(Matrix*, int r1, int r2, int scale);


void Matrix_Free(Matrix*);