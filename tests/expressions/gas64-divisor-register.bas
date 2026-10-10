/'
    Project: FreeBASIC compiler regression tests
    File: gas64-divisor-register.bas
    Purpose: Exercise integer division while preparing a variadic call.
    Responsibilities: Check register-width spelling and argument values.
    This file intentionally does NOT contain: SDL declarations or file I/O.
'/

' TEST_MODE : COMPILE_AND_RUN_OK

#if defined( __FB_X86__ ) and defined( __FB_64BIT__ ) and not defined( __FB_DARWIN__ )
	#cmdline "-gen gas64"
	#include once "crt/stdio.bi"

	'' A long argument list keeps the usual division temporaries occupied.
	'' The 32-bit division path must use edi/esi when it selects rdi/rsi.
	#define major_part(v) clng((v) \ 1000000L)
	#define minor_part(v) clng(((v) \ 1000L) mod 1000L)
	#define micro_part(v) clng((v) mod 1000L)

	sub check_versions()
		dim as long compiled = 3002000, linked = 3002000
		dim as long core_compiled = 3004018, core_linked = 3004018
		dim result as zstring * 128

		snprintf(@result, sizeof(result), "%d.%d.%d %d.%d.%d %d.%d.%d %d.%d.%d", _
		         major_part(compiled), minor_part(compiled), micro_part(compiled), _
		         major_part(linked), minor_part(linked), micro_part(linked), _
		         major_part(core_compiled), minor_part(core_compiled), micro_part(core_compiled), _
		         major_part(core_linked), minor_part(core_linked), micro_part(core_linked))
		if result <> "3.2.0 3.2.0 3.4.18 3.4.18" then end 1
	end sub

	check_versions()
#endif

/' end of gas64-divisor-register.bas '/
