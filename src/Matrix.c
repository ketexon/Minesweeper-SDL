#include "Matrix.h"

#include <stdio.h>

Matrix* Matrix_New(size_t r, size_t c) {
	Matrix* matrix = malloc(sizeof(*matrix));

	matrix->r = r;
	matrix->c = c;
	matrix->data = calloc(r * c, sizeof(int));

	return matrix;
}

int* Matrix_Get(Matrix* matrix, size_t r, size_t c) {
	return matrix->data + c + r * matrix->c;
}

//https://www.cfm.brown.edu/people/dobrush/cs52/Mathematica/Part1/rref.html
void Matrix_RREF(Matrix* mat){
    // lead = 0
    // rowCount = len(M)
    // columnCount = len(M[0])
    // for r in range(rowCount):
    //     if lead >= columnCount:
    //         return
    //     i = r
    //     while M[i][lead] == 0:
    //         i += 1
    //         if i == rowCount:
    //             i = r
    //             lead += 1
    //             if columnCount == lead:
    //                 return
    //     M[i],M[r] = M[r],M[i]
    //     lv = M[r][lead]
    //     M[r] = [ mrx / float(lv) for mrx in M[r]]
    //     for i in range(rowCount):
    //         if i != r:
    //             lv = M[i][lead]
    //             M[i] = [ iv - lv*rv for rv,iv in zip(M[r],M[i])]
    //     lead += 1
	int lead = 0;

	for(int r = 0; r < mat->r; ++r){
		if(lead >= mat->c) return;
		int i = r;
		while(*Matrix_Get(mat, i, lead) == 0){
			++i;
			if(i == mat->r) {
				i = r;
				++lead;
				if(lead == mat->c) return;
			}
		}
		Matrix_SwapRows(mat, i, r);
		// we don't need to scale because all entries are 1
		for(i = 0; i < mat->r; ++i){
			if (i != r){
				Matrix_SubRows(mat, i, r, *Matrix_Get(mat, i, lead));
			}
		}
		++lead;
	}
}

void Matrix_SwapRows(Matrix* mat, int r1, int r2){
	for(int c = 0; c < mat->c; ++c){
		int tmp = *Matrix_Get(mat, r1, c);
		*Matrix_Get(mat, r1, c) = *Matrix_Get(mat, r2, c);
		*Matrix_Get(mat, r2, c) = tmp;
	}
}

void Matrix_SubRows(Matrix* mat, int r1, int r2, int scale) {
	for(int c = 0; c < mat->c; ++c){
		*Matrix_Get(mat, r1, c) = *Matrix_Get(mat, r1, c) - *Matrix_Get(mat, r2, c) * scale;
	}
}


void Matrix_Free(Matrix* matrix) {
	free(matrix->data);
	free(matrix);
}

void Matrix_Print(Matrix* matrix){
	printf("[\n");
	for(int r = 0; r < matrix->r; ++r){
		printf("\t[");
		for(int c = 0; c < matrix->c; ++c){
			printf("%d, ", *Matrix_Get(matrix, r, c));
		}
		printf("],\n");
	}
	printf("]\n");

}