'' Project: FreeBASIC C-emitter tests
'' File: order-and-packed.bas
'' Purpose: Preserve evaluation order, lexical identity and packed pointer storage.
'' Responsibilities: Check sibling locals and an unaligned named pointer field.
'' This file intentionally does NOT require an observer or a replacement runtime.

'' TEST_MODE : COMPILE_AND_RUN_OK

dim shared events as integer

function mark( byval value as integer ) as integer
	events = events * 10 + value
	return value
end function

function calculate( byval take_first as integer ) as integer
	dim result as integer = mark( 1 )
	if( take_first ) then
		dim value as integer = mark( 2 )
		result += value
	else
		dim value as integer = mark( 3 )
		result += value
	end if
	dim tail as integer = mark( 4 )
	return result + tail
end function

if( calculate( -1 ) <> 7 or events <> 124 ) then end 1
events = 0
if( calculate( 0 ) <> 8 or events <> 134 ) then end 2

type PackedRow field = 1
	guard as ubyte
	name as const zstring ptr
	tail as long
end type

dim storage(0 to sizeof(PackedRow) + 1) as ubyte
dim row as PackedRow ptr = cptr(PackedRow ptr, @storage(1))
row->guard = 93
row->name = @"packed"
row->tail = 117
if( row->guard <> 93 or row->tail <> 117 ) then end 3
if( *row->name <> "packed" ) then end 4
if( storage(0) <> 0 or storage(ubound(storage)) <> 0 ) then end 5
end 0

'' end of order-and-packed.bas
