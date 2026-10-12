'' examples/manual/libraries/gmp.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'gmp, The GNU Multiple Precision Arithmetic Library'
'' Purpose: Compute and print 2^65536 through the native GMP library.
'' Responsibilities: Own the number buffer and decimal output storage.
'' This example contains no custom GMP allocator or limb arithmetic.
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibgmp
'' --------
''
'' Ownership:
''
'' The example owns the __mpz_struct buffer allocated for bignum and the
'' output string supplied to mpz_get_str().  Each is released on its matching
'' successful allocation path.

#include Once "gmp.bi"

Dim As Integer status = 0
Dim As mpz_ptr bignum = Allocate(SizeOf(__mpz_struct))
If bignum = 0 Then
	Print "Unable to allocate the GMP number."
	status = 1
Else
	mpz_init_set_si(bignum, 2)
	mpz_pow_ui(bignum, bignum, 65536)

	Print "2^65536 = ";
	' GMP can use a custom allocator. Supply our own output buffer so that
	' Deallocate() never receives memory owned by the library's allocator.
	' Two extra bytes cover a possible sign and the terminating zero.
	Dim As UInteger digits = mpz_sizeinbase(bignum, 10)
	Dim As ZString Ptr s
	If digits > 0 AndAlso digits <= CUInt(-1) - 2 Then s = Allocate(digits + 2)
	If s <> 0 Then
		If mpz_get_str(s, 10, bignum) <> 0 Then
			Print *s;
		Else
			Print "<conversion failed>";
			status = 1
		End If
		Deallocate(s)
		s = 0
	Else
		Print "<output allocation failed>";
		status = 1
	End If
	Print

	mpz_clear(bignum)
	Deallocate(bignum)
	bignum = 0
End If

End status
'' end of examples/manual/libraries/gmp.bas
