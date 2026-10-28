'' Project: FreeBASIC semantic sidecar tests
'' File: layout-requests.bas
'' Purpose: Exercise packing requests and each native aggregate storage decision.
'' Responsibilities: Source coercion, managed fields, inherited/anonymous layout and bitfields.
'' This file intentionally does NOT require executing foreign target code.

type NaturalLayout
	marker as ubyte
	value as double
end type

type PackedLayout field = 1
	marker as ubyte
	value as double
end type

type CoercedLayout field = 3
	marker as ubyte
	value as double
end type

type ZeroLayout field = 0
	value as double
end type

type NegativeLayout field = -1
	value as double
end type

type ExcessiveLayout field = 16
	value as double
end type

type OddLayout field = 5
	marker as ubyte
	value as double
end type

type ManagedLayout field = 1
	marker as ubyte
	text as string
	items(any) as long
	tail as ubyte
end type

type ArrayLayout field = 2
	marker as ubyte
	items(1 to 3) as long
	pointers(0 to 1) as long ptr
end type

type BitLayout
	marker as ubyte
	first : 3 as uinteger
	second : 5 as uinteger
	third : 1 as uinteger
end type

type ParentLayout extends Object
	value as long
end type

type ChildLayout extends ParentLayout
	tail as ubyte
end type

type OuterLayout field = 1
	marker as ubyte
	union
		word as long
		type
			inner_marker as ubyte
			inner_value as double
		end type
	end union
	tail as ubyte
end type

type FutureAlias as FutureLayout
type ForwardLayout
	value as FutureAlias ptr
end type
type FutureLayout
	value as long
end type

type SelfLayout
	value as long
	const ValueOffset = offsetof(SelfLayout, value)
	declare function CopyValue(byval other as SelfLayout) as SelfLayout
end type

#if 0
type NeverLayout field = 2
	value as double
end type
#endif

dim shared natural_value as NaturalLayout, packed_value as PackedLayout

'' end of layout-requests.bas
