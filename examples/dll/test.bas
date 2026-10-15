''
'' Project: FreeBASIC compiler examples
'' File: test.bas
''
'' Purpose: Call mydll's function and print the result.
'' Responsibilities: Select the platform's import library and exercise AddNumbers.
'' This file intentionally does NOT contain mydll's implementation or load DXE files dynamically.
''
''
'' Compile as: fbc test.bas
'' (mydll must be compiled first)
''

#include "mydll.bi"

#if defined( __FB_DOS__ )
	#inclib "mydll_il"
#else
	#inclib "mydll"
#endif

randomize( timer( ) )

dim as integer x = rnd( ) * 10
dim as integer y = rnd( ) * 10

print x; " +"; y; " ="; AddNumbers( x, y )

'' end of test.bas
