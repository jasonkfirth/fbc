''
'' FreeBASIC macOS CRT tests
'' ------------------------
''
'' File: setjmp-darwin-smoke.bas
''
'' Purpose:
''
''     Verify the opaque setjmp storage required by the Darwin C ABI.
''
'' Responsibilities:
''
''     - check architecture-specific jmp_buf and sigjmp_buf sizes
''     - exercise setjmp/longjmp and their signal-preserving counterparts
''     - exercise Darwin's underscore-prefixed non-signal entry points
''
'' This file intentionally does NOT contain:
''
''     - jumps across FreeBASIC object lifetimes
''     - signal-handler installation
''     - assumptions about the opaque saved-register layout
''

#include once "crt/setjmp.bi"

const SMOKE_OK = 0
const SMOKE_SETJMP_FAILED = 1
const SMOKE_UNDERSCORE_SETJMP_FAILED = 2
const SMOKE_SIGSETJMP_FAILED = 3

#if defined( __FB_64BIT__ ) and defined( __FB_ARM__ )
	#assert sizeof( jmp_buf ) = 192
	#assert sizeof( sigjmp_buf ) = 196
#elseif defined( __FB_ARM__ )
	#assert sizeof( jmp_buf ) = 112
	#assert sizeof( sigjmp_buf ) = 116
#elseif defined( __FB_64BIT__ )
	#assert sizeof( jmp_buf ) = 148
	#assert sizeof( sigjmp_buf ) = 152
#else
	#assert sizeof( jmp_buf ) = 72
	#assert sizeof( sigjmp_buf ) = 76
#endif

dim as jmp_buf regular_environment
dim as long jump_result = setjmp( @regular_environment )

if( jump_result = 0 ) then
	longjmp @regular_environment, 23
end if

if( jump_result <> 23 ) then
	end SMOKE_SETJMP_FAILED
end if

dim as jmp_buf underscore_environment
jump_result = _setjmp( @underscore_environment )

if( jump_result = 0 ) then
	_longjmp @underscore_environment, 29
end if

if( jump_result <> 29 ) then
	end SMOKE_UNDERSCORE_SETJMP_FAILED
end if

dim as sigjmp_buf signal_environment
jump_result = sigsetjmp( @signal_environment, 1 )

if( jump_result = 0 ) then
	siglongjmp @signal_environment, 31
end if

if( jump_result <> 31 ) then
	end SMOKE_SIGSETJMP_FAILED
end if

end SMOKE_OK

'' end of setjmp-darwin-smoke.bas
