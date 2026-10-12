'' FreeBASIC serialization interoperability test
'' File: interop.bas
'' Purpose: Exchange the same logical payload between actual target executables.
'' Responsibilities: Exercise native INTEGER widths, descriptors and nested arrays.
'' This file intentionally contains no codec implementation.

#include once "fbserialize.bi"

type ChildState
    text_value as string
    number as integer
end type
type PortableState
    leading_byte as ubyte
    text_value as string
    number as integer
    unsigned_number as uinteger
    ratio as double
    values(any, any) as ChildState
    fixed_values(-1 to 1) as ChildState
    flag as boolean
end type

dim as PortableState state
dim as string payload
if( command(1) = "write" ) then
    state.leading_byte = 250
    state.text_value = "portable" & chr(0) & "payload"
    state.number = -2147483647
    state.unsigned_number = 4294967295ull
    state.ratio = 0.125
    state.flag = true
    redim state.values(-2 to 0, 3 to 4)
    state.values(0,4).text_value = "owned"
    state.values(0,4).number = -12345
    state.fixed_values(-1).text_value = "inline"
    payload = Serialize(state)
    if( len(payload) = 0 or err <> 0 ) then end 1
    open command(2) for binary access write as #1
    put #1, , payload
    close #1
elseif( command(1) = "read" ) then
    open command(2) for binary access read as #1
    payload = space(lof(1))
    get #1, , payload
    close #1
    if( Deserialize(payload, state) = 0 ) then end 2
    if( state.leading_byte <> 250 or state.text_value <> "portable" & chr(0) & "payload" ) then end 3
    if( state.number <> -2147483647 or state.unsigned_number <> 4294967295ull ) then end 4
    if( state.ratio <> 0.125 or state.flag <> true ) then end 5
    if( lbound(state.values,1) <> -2 or ubound(state.values,1) <> 0 ) then end 6
    if( lbound(state.values,2) <> 3 or ubound(state.values,2) <> 4 ) then end 7
    if( state.values(0,4).text_value <> "owned" or state.values(0,4).number <> -12345 ) then end 8
    if( state.fixed_values(-1).text_value <> "inline" ) then end 9
    if( Serialize(state) <> payload ) then end 10
else
    end 11
end if
print "SERIALIZATION_INTEROP=PASS BITS=" & str(sizeof(any ptr) * 8)
'' end of interop.bas
