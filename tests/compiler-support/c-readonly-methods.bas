/'
	Project: FreeBASIC C backend regression tests
	File: c-readonly-methods.bas
	Purpose: Check const internal method inputs while preserving public C entries.
	Responsibilities: Exercise constructors, shared method statics, native callback
	calls, mutable inputs and pointer escapes.
	This file intentionally does NOT contain: Warning suppression or changed
	public parameter types.
'/
#lang "fb"

dim shared as long constructors
dim shared as zstring ptr saved_input

'' The native reader uses this one LONG at offset zero. There is no vptr.
type Reader
	value as long
	declare constructor cdecl alias "ro_method_ctor"(byval text as zstring ptr)
	declare function ReadText cdecl alias "ro_method_read"(byval text as zstring ptr) as long
	declare sub Mutate cdecl alias "ro_method_mutate"(byval text as zstring ptr)
	declare sub KeepInput cdecl(byval text as zstring ptr)
end type

constructor Reader cdecl(byval text as zstring ptr)
	constructors += 1
	value = len(*text)
end constructor

function Reader.ReadText cdecl(byval text as zstring ptr) as long
	static as long calls
	calls += 1
	value = len(*text)
	return calls
end function

sub Reader.Mutate cdecl(byval text as zstring ptr)
	text[0] = asc("Z")
	value += 1
end sub

sub Reader.KeepInput cdecl(byval text as zstring ptr)
	saved_input = text
	value += 1
end sub

declare function CheckNative cdecl alias "check_native_methods"(byval item as Reader ptr) as long

dim as Reader item = Reader("first")
if( constructors <> 1 or item.value <> 5 ) then end 1
if( item.ReadText("read") <> 1 or item.value <> 4 ) then end 2
if( CheckNative(@item) <> 0 ) then end 3
if( constructors <> 2 or item.value <> 7 ) then end 4
if( item.ReadText("again") <> 3 or item.value <> 5 ) then end 5
dim as zstring * 8 mutable_text = "alter"
item.KeepInput(@mutable_text)
item.Mutate(@mutable_text)
if( mutable_text <> "Zlter" or saved_input <> @mutable_text ) then end 6
print "C_READONLY_METHODS_PASS"

/' end of c-readonly-methods.bas '/
