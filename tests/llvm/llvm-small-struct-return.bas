'' Project: FreeBASIC compiler regression tests
'' -------------------------------------------
''
'' File: llvm-small-struct-return.bas
''
'' Purpose:
''
''     Verify the x86-64 Linux aggregate return ABI across LLVM and C.
''
'' Responsibilities:
''
''     - check integer, SSE and mixed register results
''     - verify packed results that require a hidden return pointer
''     - exercise direct, indirect, variadic and discarded calls
''
'' This file intentionally does NOT contain:
''
''     - aggregate argument ABI tests or target dispatch

type pair
	first as long
	second as long
end type

type floats
	items(0 to 2) as single
end type

type mixed
	number as double
	marker as long
end type

type packed_result field = 1
	tag as ubyte
	payload as long
end type

'' The nested type is packed, but the outer prefix puts its integer at
'' offset four. This result returns in a register.
type nested_result field = 1
	prefix(0 to 2) as ubyte
	inner as packed_result
end type

type combined
	number as single
	marker as long
end type

union overlay
	number as single
	bits as ulong
end union

#assert sizeof(pair) = 8
#assert sizeof(floats) = 12
#assert offsetof(mixed, marker) = 8
#assert sizeof(packed_result) = 5
#assert sizeof(nested_result) = 8

extern "c"
	declare function c_return_pair(byval first as long) as pair
	declare function c_return_floats() as floats
	declare function c_return_mixed() as mixed
	declare function c_return_packed(byval payload as long) as packed_result
	declare function c_return_variadic(byval base_value as long, ...) as packed_result
	declare function c_return_nested() as nested_result
	declare function c_return_combined() as combined
	declare function c_return_overlay() as overlay
	declare function c_check_llvm_results() as long

	function llvm_return_pair(byval first as long) as pair
		return type<pair>(first, 789)
	end function

	function llvm_return_floats() as floats
		return type<floats>({3.5, -4.75, 6.25})
	end function

	function llvm_return_mixed() as mixed
		return type<mixed>(29.125, -8128)
	end function

	function llvm_return_packed(byval payload as long) as packed_result
		return type<packed_result>(&h5A, payload)
	end function

	function llvm_return_nested() as nested_result
		return type<nested_result>({31, 32, 33}, type<packed_result>(34, -35))
	end function

	function llvm_return_combined() as combined
		return type<combined>(-9.25, 4567)
	end function

	function llvm_return_overlay() as overlay
		dim result as overlay
		result.bits = &h12345678u
		return result
	end function
end extern

type pair_proc as function cdecl(byval as long) as pair
type packed_proc as function cdecl(byval as long) as packed_result
dim pair_pointer as pair_proc = @c_return_pair
dim packed_pointer as packed_proc = @c_return_packed

dim integers as pair = c_return_pair(123)
if( integers.first <> 123 orelse integers.second <> -123456789 ) then end __LINE__
integers = pair_pointer(-345)
if( integers.first <> -345 orelse integers.second <> -123456789 ) then end __LINE__

dim singles as floats = c_return_floats()
if( singles.items(0) <> 1.25 orelse singles.items(1) <> -2.5 orelse _
    singles.items(2) <> 3.75 ) then end __LINE__

dim split as mixed = c_return_mixed()
if( split.number <> -14.75 orelse split.marker <> 12345 ) then end __LINE__

dim packed as packed_result = c_return_packed(314159)
if( packed.tag <> &hA5 orelse packed.payload <> 314159 ) then end __LINE__
packed = packed_pointer(-12321)
if( packed.tag <> &hA5 orelse packed.payload <> -12321 ) then end __LINE__
c_return_packed(0)

packed = c_return_variadic(100, 23L)
if( packed.tag <> &hBC orelse packed.payload <> 123 ) then end __LINE__

dim nested as nested_result = c_return_nested()
if( nested.prefix(0) <> 21 orelse nested.prefix(1) <> 22 orelse _
    nested.prefix(2) <> 23 orelse nested.inner.tag <> 24 orelse _
    nested.inner.payload <> -25 ) then end __LINE__

dim shared_slot as combined = c_return_combined()
if( shared_slot.number <> 11.5 orelse shared_slot.marker <> -6789 ) then end __LINE__

dim union_value as overlay = c_return_overlay()
if( union_value.bits <> &hDEADBEEFu ) then end __LINE__

dim c_status as long = c_check_llvm_results()
if( c_status <> 0 ) then
	print "C caller failed aggregate check"; c_status
	end 1
end if

'' end of llvm-small-struct-return.bas
