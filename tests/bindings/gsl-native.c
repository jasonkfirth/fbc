/* FreeBASIC GSL native qualification
 * File: gsl-native.c
 * Purpose: Provide C layouts and actual matrix results.
 * Responsibilities: Owning allocation, row-major indexing and transpose cleanup.
 * This file contains no substitute matrix implementation.
 */
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_math.h>
#include <stddef.h>
#include <stdio.h>

int main(void)
{
    size_t i, j;
    gsl_matrix *matrix;
    printf("ABI %zu %zu %zu %zu %zu %zu %zu\n", sizeof(size_t), sizeof(gsl_matrix),
           offsetof(gsl_matrix, size1), offsetof(gsl_matrix, tda),
           offsetof(gsl_matrix, data), offsetof(gsl_matrix, block), offsetof(gsl_matrix, owner));
    matrix = gsl_matrix_alloc(3, 3);
    if (!matrix) return 1;
    for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) gsl_matrix_set(matrix, i, j, 0.23 + 100 * i + j);
    if (gsl_matrix_transpose(matrix)) { gsl_matrix_free(matrix); return 2; }
    for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) {
        if (fabs(gsl_matrix_get(matrix, i, j) - (0.23 + 100 * j + i)) > 0.00001) {
            gsl_matrix_free(matrix);
            return 3;
        }
    }
    gsl_matrix_free(matrix);
    if (gsl_hypot(3, 4) != 5 || fabs(gsl_pow_4(3.141) - 97.335607906161) > 0.0000000001) return 4;
    puts("GSL_NATIVE=PASS transpose=9 hypot=5 cleanup=1");
    return 0;
}
/* end of gsl-native.c */
