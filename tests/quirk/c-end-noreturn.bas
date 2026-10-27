'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC C backend regression tests
'' File: c-end-noreturn.bas
'' Purpose: Preserve termination and alternate entry paths in emitted C.
'' Responsibilities: Exercise END/SYSTEM/STOP, live forward/backward labels,
''     jump-table dispatch, dead tails and ordinary library returns.
'' This file intentionally does NOT dereference null on an executable path.
#lang "fb"

#ifndef TERMINATION_KIND
	#define TERMINATION_KIND 0
#endif

#ifdef TERMINATION_LIBRARY

function termination_library_value( byval value as long ) as long
	return value + 1
end function

#else

private function guarded_value( byval value as const long ptr ) as long
	if( value = 0 ) then
		#if TERMINATION_KIND = 0
			end 37
		#elseif TERMINATION_KIND = 1
			system 37
		#else
			stop 37
		#endif
	end if
	return *value
end function

private function forward_target( byval take_branch as long ) as long
	if( take_branch <> 0 ) then goto live_target
	end 91
live_target:
	return 42
end function

private function backward_target( byval take_branch as long ) as long
	goto dispatch
live_target:
	return 43
	end 92
dispatch:
	if( take_branch <> 0 ) then goto live_target
	end 39
end function

private function table_target( byval selector as long ) as long
	select case as const selector
	case 2
		return 44
	case 4
		end 93
	case else
		return 45
	end select
end function

private sub terminate_with_tail( )
	end 38
unused_target:
	'' An unreferenced label cannot revive this application-termination tail.
	end 94
end sub

dim as long value = 42
if( guarded_value( @value ) <> 42 ) then end 1
if( forward_target( 1 ) <> 42 ) then end 2
if( backward_target( 1 ) <> 43 ) then end 3
if( table_target( 2 ) <> 44 ) then end 4
if( table_target( 3 ) <> 45 ) then end 5

select case command( 1 )
case "end"
	value = guarded_value( 0 )
	end 6
case "tail"
	terminate_with_tail( )
	end 7
case "branch"
	value = backward_target( 0 )
	end 8
end select

#ifndef TERMINATION_IMPLICIT_MAIN
	end 0
#endif

#endif

'' end of c-end-noreturn.bas
