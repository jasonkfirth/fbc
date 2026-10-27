/'
    Project: FreeBASIC C backend regression tests
    File: const-nested-arg-casts.bas
    Purpose: Preserve nested pointer qualifiers when casting call arguments.
    Responsibilities: Check ordinary calls, callback calls and const record reads.
    This file intentionally does NOT require an external C or graphics library.
'/
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fb"

type ConstSink as sub cdecl( byval text as const zstring const ptr ptr )

sub consume cdecl( byval text as const zstring const ptr ptr )

	if( text = 0 ) then end 1
	if( *text = 0 ) then end 2
	if( **text <> "hello" ) then end 3

end sub

type RecordValue

	value as integer

end type

sub consumeRecord( byref value as const RecordValue )

	if( value.value <> 42 ) then end 4

end sub

dim as ConstSink callback = @consume
dim as zstring ptr text = strptr( "hello" )
dim as RecordValue value = ( 42 )

consume( @text )
callback( @text )
consumeRecord( value )

'' Constant aggregates still need writable storage while their initializers
'' run. Their pointer elements must retain the pointed-to type's qualifiers.
dim as const ulong initial(0 to 2) = { 1ul, 2ul, 3ul }
dim as const RecordValue constantValue = ( 42 )
dim as const integer const ptr pointers(0 to 1) = { @value.value, @constantValue.value }
if( initial(0) <> 1ul or initial(2) <> 3ul ) then end 7
if( constantValue.value <> 42 ) then end 8
if( *pointers(0) <> 42 or *pointers(1) <> 42 ) then end 9

'' FreeBASIC accepts const pointers for memory release and resizing. The C
'' library takes mutable void pointers, so those calls need its ABI spelling.
dim as const integer ptr allocation = allocate( sizeof( integer ) )
if( allocation = 0 ) then end 5
dim as const integer ptr resized = reallocate( allocation, 2 * sizeof( integer ) )
if( resized = 0 ) then
	deallocate( allocation )
	end 6
end if
deallocate( resized )

' end of const-nested-arg-casts.bas
