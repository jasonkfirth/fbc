'' Project: FreeBASIC compiler tests
'' File: c-branch-exits.bas
'' Purpose: Exercise C label reachability through returning branches.
'' Responsibilities: Cover structured exits, dead cycles and backwards entries.
'' This file intentionally does NOT contain platform I/O or assembler targets.
'' TEST_MODE : COMPILE_AND_RUN_OK
#lang "fb"

private function choose_by_if( byval selector as long ) as long

	if( selector = 0 ) then
		return 11
	else
		return 12
	end if

end function

private function choose_by_select( byval selector as long ) as long

	select case( selector )
	case 0
		return 21
	case else
		return 22
	end select

end function

private function dead_cycle( ) as long

	goto entry

dead_a:
	goto dead_b
dead_b:
	goto dead_a

entry:
	return 31

end function

private function backwards_entries( ) as long

	goto dispatch

first_entry:
	return 42
second_entry:
	goto first_entry
dispatch:
	goto second_entry

end function

if( choose_by_if( 0 ) <> 11 ) then end 1
if( choose_by_if( 1 ) <> 12 ) then end 2
if( choose_by_select( 0 ) <> 21 ) then end 3
if( choose_by_select( 1 ) <> 22 ) then end 4
if( dead_cycle( ) <> 31 ) then end 5
if( backwards_entries( ) <> 42 ) then end 6
print "c-branch-exits-ok"
end 0

'' end of c-branch-exits.bas
