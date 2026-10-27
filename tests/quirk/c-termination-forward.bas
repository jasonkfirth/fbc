'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC C backend regression tests
'' File: c-termination-forward.bas
'' Purpose: Keep later non-returning definitions consistent with earlier callers.
'' Responsibilities: Exercise public forward calls, transitive termination and
''     a public function with both returning and terminating paths.
'' This file intentionally does NOT change runtime error or exception handling.
#lang "fb"

declare sub dispatch_exit( byval status as long )

public sub public_forward_exit( byval status as long )
	dispatch_exit( status )
	end 96
end sub

public function mixed_public( byval value as long ) as long
	if( value <> 0 ) then return value + 1
	public_forward_exit( 37 )
	end 97
end function

sub dispatch_exit( byval status as long )
	end status
end sub

if( mixed_public( 41 ) <> 42 ) then end 1
select case command( 1 )
case "end"
	dim as long result = mixed_public( 0 )
	end result
case "tail"
	public_forward_exit( 38 )
	end 98
case "branch"
	dispatch_exit( 39 )
	end 99
end select
end 0

'' end of c-termination-forward.bas
