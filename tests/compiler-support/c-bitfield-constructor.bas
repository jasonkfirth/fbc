'' Project: FreeBASIC compiler regression tests
'' File: c-bitfield-constructor.bas
'' Purpose: Verify initialization of default-constructed bitfield storage.
'' Responsibilities: Check default and explicit fields, neighboring bits,
''                  string lifetime, and repeated construction.
'' This file intentionally does NOT test every target ABI layout.
#lang "fb"

type DefaultBits
	values(0 to 2) as integer
	text as string
	before as ubyte = &ha5
	first : 1 as uinteger
	second : 4 as uinteger
	third : 3 as uinteger
	after as ubyte = &h5a
end type

type PackedBits field = 1
	before as ubyte = &h37
	first : 2 as uinteger
	second : 3 as uinteger
	after as ubyte = &h73
	text as string
end type

type SplitBits
	text as string
#ifdef __FB_64BIT__
	first : 40 as ulongint
	second : 40 as ulongint
#else
	first : 20 as ulong
	second : 20 as ulong
#endif
	after as ubyte = &h6d
end type

type ExplicitBits
	first : 3 as uinteger = 5
	second : 4 as uinteger
	third : 3 as uinteger = 6
	text as string
end type

for iteration as integer = 1 to 4
	scope
		dim item as DefaultBits
		if item.first <> 0 or item.second <> 0 or item.third <> 0 then end 1
		if item.before <> &ha5 or item.after <> &h5a then end 6
		item.first = 1 : item.second = 11 : item.third = 5
		item.text = "ready"
		if item.first <> 1 or item.second <> 11 or item.third <> 5 then end 2
		if item.text <> "ready" then end 3
	end scope
	scope
		dim item as PackedBits
		if item.first <> 0 or item.second <> 0 then end 7
		if item.before <> &h37 or item.after <> &h73 then end 8
		item.first = 3 : item.second = 6
		if item.first <> 3 or item.second <> 6 then end 9
		if item.before <> &h37 or item.after <> &h73 then end 10
	end scope
	scope
		dim item as SplitBits
		if item.first <> 0 or item.second <> 0 or item.after <> &h6d then end 11
		item.first = &h12345 : item.second = &h54321
		if item.first <> &h12345 or item.second <> &h54321 then end 12
		if item.after <> &h6d then end 13
	end scope
	scope
		dim item as ExplicitBits
		if item.first <> 5 or item.second <> 0 or item.third <> 6 then end 4
		item.second = 9
		if item.first <> 5 or item.second <> 9 or item.third <> 6 then end 5
	end scope
next
print "C_BITFIELD_CONSTRUCTOR_PASS"

'' end of c-bitfield-constructor.bas
