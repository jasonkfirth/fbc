/'
Project: FreeBASIC C backend regression tests
File: c-qualified-float-literals.bas
Purpose: Preserve floating literal widths after read-only member arithmetic.
Responsibilities: Check Single and Double results and reject implicit C promotion.
This file intentionally does NOT depend on graphics or audio libraries.
'/
#lang "fb"
' Only C compilers accept this diagnostic; LLVM still runs the value checks.
#if (__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang")
	#cmdline "-Wc -Werror=double-promotion"
#endif

type SingleRectangle
	y as single
	height as single
end type

type DoubleRectangle
	y as double
	height as double
end type

function single_offset( byval rectangle as const SingleRectangle ptr ) as single
	return rectangle->y + ((rectangle->height - 8) / 2.0f)
end function

function double_offset( byval rectangle as const DoubleRectangle ptr ) as double
	return rectangle->y + ((rectangle->height - 8) / 2.0)
end function

dim as SingleRectangle narrow = (1.0f, 12.0f)
dim as DoubleRectangle wide = (1.0, 12.0)
assert( single_offset(@narrow) = 3.0f )
assert( double_offset(@wide) = 3.0 )
narrow.height = -8.0f
wide.height = -8.0
assert( single_offset(@narrow) = -7.0f )
assert( double_offset(@wide) = -7.0 )
narrow.height = 0.0f
wide.height = 0.0
assert( single_offset(@narrow) = -3.0f )
assert( double_offset(@wide) = -3.0 )
wide.y = 0.0
wide.height = 16777217.0
assert( double_offset(@wide) = 8388604.5 )
print "C_QUALIFIED_FLOAT_LITERALS_PASS"

' end of c-qualified-float-literals.bas
