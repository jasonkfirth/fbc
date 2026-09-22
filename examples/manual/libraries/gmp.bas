'' examples/manual/libraries/gmp.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'gmp, The GNU Multiple Precision Arithmetic Library'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibgmp
'' --------
''
'' Ownership:
''
'' The example owns the __mpz_struct buffer allocated for bignum and the
'' string allocated by mpz_get_str().  Each is released on its matching
'' successful allocation path.

#include Once "gmp.bi"

Dim As mpz_ptr bignum = Allocate(SizeOf(__mpz_struct))
If bignum = 0 Then
	Print "Unable to allocate the GMP number."
Else
	mpz_init_set_si(bignum, 2)
	mpz_pow_ui(bignum, bignum, 65536)

	Print "2^65536 = ";
	Dim As ZString Ptr s = mpz_get_str(0, 10, bignum)
	If s <> 0 Then
		Print *s;
		Deallocate(s)
		s = 0
	Else
		Print "<conversion failed>";
	End If
	Print

	mpz_clear(bignum)
	Deallocate(bignum)
	bignum = 0
End If

'' end of examples/manual/libraries/gmp.bas
