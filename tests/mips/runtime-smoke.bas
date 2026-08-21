'' FreeBASIC MIPS Linux runtime smoke test
'' ---------------------------------------
''
'' File: tests/mips/runtime-smoke.bas
''
'' Purpose:
''
''     Exercise a linked FreeBASIC runtime under each emulated MIPS Linux ABI.
''
'' Responsibilities:
''
''     - verify integer byte order at run time
''     - verify pointer width agrees with the compiler target
''     - verify libffi-backed THREADCALL and pthread integration
''
'' This file intentionally does NOT contain:
''
''     - emulator startup policy
''     - toolchain paths
''     - fbctests orchestration

#include once "crt/mem.bi"

sub set_result( byval result as integer ptr )
	*result = 42
end sub

'' ULONG remains 32 bits on every FreeBASIC target, so the expected leading
'' byte does not change when this test moves from MIPS32 to MIPS64.
dim value as ulong = &h01020304
dim first_byte as ubyte

'' A byte copy observes the target's in-memory representation.  A direct cast
'' and dereference can be folded by the C backend into a numeric conversion,
'' which always selects the low-order value rather than the first memory byte.
memcpy( @first_byte, @value, sizeof( first_byte ) )

#if defined( __FB_BIGENDIAN__ )
	if( first_byte <> &h01 ) then
		end 1
	end if
#else
	if( first_byte <> &h04 ) then
		end 1
	end if
#endif

#if defined( __FB_64BIT__ )
	if( sizeof( any ptr ) <> 8 ) then
		end 2
	end if
#else
	if( sizeof( any ptr ) <> 4 ) then
		end 2
	end if
#endif

dim result as integer
dim worker as any ptr = threadcall set_result( @result )
if( worker = cast( any ptr, 0 ) ) then
	end 3
end if

threadwait worker
if( result <> 42 ) then
	end 4
end if

print "mips runtime ok"

'' end of tests/mips/runtime-smoke.bas
