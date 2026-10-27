'' Project: FreeBASIC compiler regression tests
'' File: c-reserved-names-and-fields.bas
'' Purpose: Keep reserved C spellings separate from BASIC and object identities.
'' Responsibilities: Check field paths, aliases, overloads and lifetime hooks.
'' This file intentionally does NOT prove empty non-main module elimination.
'' TEST_MODE : COMPILE_AND_RUN_OK
#lang "fb"

dim shared as integer lifetime_order, main_completed

'' User hook priorities start above the compiler-owned range through 100.
private sub _startup_first( ) constructor 201
	lifetime_order = lifetime_order * 10 + 1
end sub

private sub _startup_second( ) constructor 202
	lifetime_order = lifetime_order * 10 + 2
end sub

'' Destructors run in reverse priority order, after the main body completes.
private sub _shutdown_second( ) destructor 202
	lifetime_order = lifetime_order * 10 + 5
end sub

private sub _shutdown_first( ) destructor 201
	if( main_completed <> 1 or lifetime_order <> 12345 ) then
		print "unexpected shutdown state: "; main_completed; lifetime_order
		end 16
	end if
	print "c-reserved-names-and-fields-dtor-ok"
end sub

type _Inner
	_VALUE as long
	_ZERO as long
	fb_c_field__5FVALUE as long
end type

type _Nested
	_CHILD as _Inner
	_TAIL as long
end type

type _Anonymous
	_PREFIX as long
	union
		type
			_LEFT as long
			_RIGHT as long
		end type
		_WORDS(0 to 1) as long
	end union
	_TAIL as long
end type

type _Packed field = 1
	_GUARD as ubyte
	_WORD as longint
	_TAIL as long
end type

'' Omitted fields exercise nested zero initializer designators as well.
dim shared as _Nested _stored = ((11))
dim shared as _Packed _packed_value = (93, 117, 0)

function _public_value( byval value as long ) as long
	return value + 1
end function

private function _private_value( byval value as long ) as long
	return value + 2
end function

declare function alias_reference cdecl alias "_FB_AUDIT_RESERVED_PROC"( byval value as long ) as long
function alias_storage cdecl alias "_FB_AUDIT_RESERVED_PROC"( byval value as long ) as long
	return value + 3
end function

namespace _Names
	function _value overload( byval value as long ) as long
		return value + 4
	end function

	function _value overload( byval value as longint ) as longint
		return value + 5
	end function

	type _Tracked
		_VALUE as long
		declare constructor( )
		declare destructor( )
		declare function _read( ) as long
	end type

	constructor _Tracked( )
		_VALUE = 37
		lifetime_order = lifetime_order * 10 + 3
	end constructor

	destructor _Tracked( )
		lifetime_order = lifetime_order * 10 + 4
	end destructor

	function _Tracked._read( ) as long
		return _VALUE
	end function
end namespace

if( lifetime_order <> 12 ) then end 1
if( _stored._CHILD._VALUE <> 11 or _stored._CHILD._ZERO <> 0 ) then end 2
if( _stored._CHILD.fb_c_field__5FVALUE <> 0 or _stored._TAIL <> 0 ) then end 3
_stored._CHILD.fb_c_field__5FVALUE = 17
if( _stored._CHILD._VALUE <> 11 or _stored._CHILD.fb_c_field__5FVALUE <> 17 ) then end 4

dim as _Anonymous overlay
overlay._PREFIX = 19
overlay._LEFT = 23
overlay._RIGHT = 29
overlay._TAIL = 31
if( overlay._WORDS(0) <> 23 or overlay._WORDS(1) <> 29 ) then end 5
if( overlay._PREFIX <> 19 or overlay._TAIL <> 31 ) then end 6

if( sizeof(_Packed) <> 13 or offsetof(_Packed, _WORD) <> 1 ) then end 7
_packed_value._WORD += 2
if( _packed_value._GUARD <> 93 or _packed_value._WORD <> 119 ) then end 8
if( _packed_value._TAIL <> 0 ) then end 9

if( _public_value(10) <> 11 or _private_value(10) <> 12 ) then end 10
if( alias_reference(10) <> 13 or alias_storage(10) <> 13 ) then end 11
dim callback as function cdecl( byval as long ) as long = @alias_reference
if( callback(20) <> 23 ) then end 12
if( _Names._value(clng(10)) <> 14 or _Names._value(clngint(10)) <> 15 ) then end 13

scope
	dim as _Names._Tracked tracked
	if( tracked._read( ) <> 37 or lifetime_order <> 123 ) then end 14
end scope
if( lifetime_order <> 1234 ) then end 15
main_completed = 1
print "c-reserved-names-and-fields-main-ok"
end 0

'' end of c-reserved-names-and-fields.bas
