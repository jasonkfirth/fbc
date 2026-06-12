' TEST_MODE : COMPILE_AND_RUN_OK

'
' C long prototype regression test
'
' clong and culong must emit as C long/unsigned long in prototypes.  GCC checks
' builtin declarations against those exact C types.
'

#cmdline "-gen gcc"
#cmdline "-Wc -Wno-unknown-warning-option"
#cmdline "-Wc -Werror=builtin-declaration-mismatch"

#include once "../../inc/crt/long.bi"

extern "C"
	declare function fb_test_expect_clong cdecl alias "__builtin_expect" ( byval as clong, byval as clong ) as clong
	declare function fb_test_popcountl cdecl alias "__builtin_popcountl" ( byval as culong ) as long
end extern

if( fb_test_expect_clong( 1, 1 ) = 0 ) then
	end 1
end if

if( fb_test_popcountl( cast( culong, &hfull ) ) <> 4 ) then
	end 1
end if

' end of clong-prototype.bas
