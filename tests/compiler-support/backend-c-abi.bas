'' Project: FreeBASIC compiler regression tests
'' -----------------------------------------
''
'' File: backend-c-abi.bas
''
'' Purpose:
''
''     Check large BYVAL aggregates at a real C library and callback boundary.
''
'' Responsibilities:
''
''     - exercise direct calls and procedure pointer calls to C
''     - preserve parameter copy semantics when C calls back into BASIC
''     - distinguish C's va_list array decay from aggregate value passing
''     - copy aggregates from packed fields without assuming source alignment
''
'' This file intentionally does NOT contain:
''
''     - ABI assumptions for Windows or non-x86 targets
''

#if defined(__FB_X86__) and defined(__FB_64BIT__) and _
    (not defined(__FB_WIN32__)) and (not defined(__FB_DOS__))

'' Use the compiler's platform va_list rather than the header's legacy
'' pointer typedef, so this exercises the System V array-decay boundary.
type va_list as cva_list
#define __crt_stdarg_bi__
#include once "crt/stdio.bi"

type aggregate
	a as longint
	b as longint
	c as longint
	d as longint
end type

type packed_aggregate field = 1
	tag as ubyte
	a as longint
	b as longint
	c as longint
end type

type packed_holder field = 1
	pad as ubyte
	value as packed_aggregate
end type

declare function c_aggregate cdecl alias "compiler_support_c_aggregate" _
	( byval value as aggregate ) as longint
declare function c_packed_aggregate cdecl alias "compiler_support_c_packed_aggregate" _
	( byval value as packed_aggregate ) as longint
declare function c_callback cdecl alias "compiler_support_c_callback" _
	( byval callback as function cdecl(byval as aggregate) as longint, _
	  byval value as aggregate ) as longint

function basic_callback cdecl( byval value as aggregate ) as longint
	value.a += 10
	return value.a + value.b * 10 + value.c * 100 + value.d * 1000
end function

sub format_values cdecl( byval buffer as zstring ptr, byval fmt as zstring ptr, ... )
	dim as cva_list args = any
	cva_start( args, fmt )
	vsprintf( buffer, fmt, args )
	cva_end( args )
end sub

dim as aggregate value = (1, 2, 3, 4)
if( c_aggregate(value) <> 4321 ) then end 1
dim as function cdecl(byval as aggregate) as longint fn = @c_aggregate
if( fn(value) <> 4321 ) then end 2
if( c_callback(@basic_callback, value) <> 4331 ) then end 3
if( value.a <> 1 ) then end 4
if( basic_callback(value) <> 4331 ) then end 5

dim as zstring * 128 buffer
format_values( @buffer, "%i %s %i", clng(123), @"hello", clng(456) )
if( buffer <> "123 hello 456" ) then end 6

'' The nested value begins at an odd byte address, even if the outer
'' allocation is more aligned than FIELD = 1 requires.
dim as packed_holder holder
holder.value.tag = 1
holder.value.a = 2
holder.value.b = 3
holder.value.c = 4
if( c_packed_aggregate(holder.value) <> 4321 ) then end 7
if( holder.value.tag <> 1 ) then end 8

#endif

print "C aggregate ABI passed"

'' end of backend-c-abi.bas
