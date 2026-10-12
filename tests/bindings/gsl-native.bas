'' FreeBASIC GSL native qualification
'' File: gsl-native.bas
'' Purpose: Compare C layouts and call the actual matrix and math interfaces.
'' Responsibilities: Owning allocation, row-major indexing and transpose cleanup.
'' This file contains no substitute matrix implementation.

#lang "fb"
#include once "gsl/gsl_matrix.bi"
#include once "gsl/gsl_math.bi"

Print "ABI "; SizeOf(UInteger); " "; SizeOf(gsl_matrix); " "; OffsetOf(gsl_matrix, size1); " "; _
	OffsetOf(gsl_matrix, tda); " "; OffsetOf(gsl_matrix, data); " "; OffsetOf(gsl_matrix, block); " "; OffsetOf(gsl_matrix, owner)
Dim As gsl_matrix Ptr matrix = gsl_matrix_alloc(3, 3)
If matrix = 0 Then End 1
For i As UInteger = 0 To 2
	For j As UInteger = 0 To 2
		gsl_matrix_set(matrix, i, j, 0.23D0 + 100 * i + j)
	Next
Next
Dim As Integer status = 0
If gsl_matrix_transpose(matrix) <> GSL_SUCCESS Then
	status = 2
Else
	For i As UInteger = 0 To 2
		For j As UInteger = 0 To 2
			If Abs(gsl_matrix_get(matrix, i, j) - (0.23D0 + 100 * j + i)) > 0.00001D0 Then status = 3
		Next
	Next
End If
gsl_matrix_free(matrix)
If status <> 0 Then End status
If gsl_hypot(3, 4) <> 5 OrElse Abs(gsl_pow_4(3.141D0) - 97.335607906161D0) > 0.0000000001D0 Then End 4
Print "GSL_NATIVE=PASS transpose=9 hypot=5 cleanup=1"
'' end of gsl-native.bas
