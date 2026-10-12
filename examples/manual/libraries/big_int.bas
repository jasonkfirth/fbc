'' examples/manual/libraries/big_int.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'big_int'
'' Purpose: Calculate and display a large integer using the native library.
'' Responsibilities: Check failures and release owned numbers and strings.
'' This file contains no arbitrary precision arithmetic implementation.
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibbigint
'' --------

#include Once "big_int/big_int_full.bi"

Function print_num(ByVal num As big_int Ptr) As Boolean
	Dim As big_int_str Ptr s = big_int_str_create(1)
	If (s = 0) Then
		Return False
	End If

	If (big_int_to_str(num, 10, s) <> 0) Then
		big_int_str_destroy(s)
		Return False
	End If

	Print *s->Str;

	big_int_str_destroy(s)
	Return True
End Function

	Dim As big_int Ptr bignum = big_int_create(1)
	If bignum = 0 Then End 1

	If big_int_from_int(2, bignum) <> 0 OrElse big_int_pow(bignum, 65536, bignum) <> 0 Then
		big_int_destroy(bignum)
		End 1
	End If

	Print "2^65536 = ";
	Dim As Boolean printed = print_num(bignum)
	Print

	big_int_destroy(bignum)
	If Not printed Then End 1

'' end of big_int.bas
