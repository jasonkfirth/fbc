'' Project: FreeBASIC compiler regression tests
'' File: llvm-packed-layout.bas
'' Purpose: Verify that LLVM storage agrees with FreeBASIC's aggregate layout.
'' Responsibilities: Check nested packing, tail padding, union aliases and relocations.
'' This file intentionally does NOT contain: Cross-language ABI declarations.

private function increment(byval value as long) as long
	return value + 1
end function

type PackedPair field = 2
	tag as byte
	value as long
end type

type AlignedHolder
	tag as byte
	pair as PackedPair
	callback as function(byval as long) as long
end type

type PackedWide field = 8
	tag as byte
	value as uinteger<128>
	tail as byte
end type

union Overlay
	first as ubyte
	storage(0 to 1) as long
end union

type UnionHolder
	tag as byte
	u as Overlay
	tail as long
end type

dim shared holders(0 to 1) as AlignedHolder = { _
    type<AlignedHolder>(11, type<PackedPair>(12, 13), @increment), _
    type<AlignedHolder>(21, type<PackedPair>(22, 23), @increment) }
dim shared wide_values(0 to 1) as PackedWide = { _
    type<PackedWide>(31, &hFEDCBA9876543210ull, 33), _
    type<PackedWide>(41, &hABCDEF0987654321ull, 43) }
dim shared union_value as UnionHolder = (51, type<Overlay>(52), 53)

if( holders(0).tag <> 11 orelse holders(0).pair.tag <> 12 orelse _
    holders(0).pair.value <> 13 ) then end 1
if( holders(1).tag <> 21 orelse holders(1).pair.tag <> 22 orelse _
    holders(1).pair.value <> 23 orelse holders(1).callback(9) <> 10 ) then end 2
if( wide_values(0).tag <> 31 orelse wide_values(0).tail <> 33 orelse _
    wide_values(0).value <> &hFEDCBA9876543210ull ) then end 3
if( wide_values(1).tag <> 41 orelse wide_values(1).tail <> 43 orelse _
    wide_values(1).value <> &hABCDEF0987654321ull ) then end 4

if( union_value.u.first <> 52 orelse union_value.u.storage(1) <> 0 orelse _
    union_value.tail <> 53 ) then end 5
union_value.u.storage(0) = 54
if( union_value.u.first <> 54 ) then end 6

'' end of llvm-packed-layout.bas
