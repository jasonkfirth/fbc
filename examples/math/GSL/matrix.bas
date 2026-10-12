'' FreeBASIC GSL matrix example
'' File: matrix.bas
'' Purpose: Create, print and transpose a small GSL matrix.
'' Responsibilities: Check allocation and release the owned matrix.
'' This example contains no matrix views or shared backing storage.
#include "gsl/gsl_matrix.bi"

'' gsl uses the c-style row major order, unlike VB or Fortran
? "A 3x3 matrix"
Dim As gsl_matrix Ptr m = gsl_matrix_alloc(3, 3)
If m = 0 Then
    Print "Could not allocate the GSL matrix"
    End 1
End If
For i As Integer = 0 To 2
    For j As Integer = 0 To 2
        gsl_matrix_set (m, i, j, 0.23D0 + 100*i + j)
    Next
next

For i As Integer = 0 To 2
    For j As Integer = 0 To 2
        Print "m(";i;",";j;") = "; gsl_matrix_get (m, i, j)
    Next
next
?

If gsl_matrix_transpose(m) <> GSL_SUCCESS Then
    gsl_matrix_free(m)
    Print "Could not transpose the GSL matrix"
    End 1
End If

? "And its transpose"
For i As Integer = 0 To 2
    For j As Integer = 0 To 2
        Print "m(";i;",";j;") = "; gsl_matrix_get (m, i, j)
    Next
next

' The owning allocation is released before waiting for the final keypress.
gsl_matrix_free(m)
Sleep
'' end of matrix.bas
