'' Project: FreeBASIC compiler regression tests
'' File: llvm-conversions.bas
'' Purpose: Verify LLVM casts preserve storage and convert rounded floats safely.
'' Responsibilities: Exercise indexed and packed lvalues and integer boundaries.
'' This file intentionally does NOT contain: LLVM IR or platform-specific code.

type PackedValue field = 1
	tag as byte
	value as long
end type

dim as long scalar, items(1 to 4)
dim as PackedValue packed
dim as long index = 2
dim as long ptr p = @scalar
dim as long ptr cast_address = cptr(long ptr, @cast(ulong, items(index)))

cast(ulong, scalar) = &hFFFFFFFFu
cast(ulong, items(index)) = &hFFFFFFFEu
cast(ulong, packed.value) = &hFFFFFFFDu
if( cast_address <> @items(index) orelse scalar <> -1 orelse _
    items(index) <> -2 orelse packed.value <> -3 ) then end 1

cast(ulong, *p) += 3
if( scalar <> 2 ) then end 2

private function from_bits(byval bits as ulongint) as double
	return *cptr(double ptr, @bits)
end function

private function convert_unsigned(byval value as double) as ulongint
	return culngint(value)
end function

private function convert_signed(byval value as double) as long
	return clng(value)
end function

dim as single small = -1.25
if( cubyte(small) <> 255 orelse cushort(small) <> 65535 orelse _
    culng(small) <> &hFFFFFFFFu ) then end 3
if( convert_unsigned(small) <> &hFFFFFFFFFFFFFFFFull ) then end 4

small = 255.5
if( cubyte(small) <> 0 orelse cshort(small) <> 256 ) then end 5
if( convert_signed(2.5) <> 2 orelse convert_signed(3.5) <> 4 ) then end 6

'' At 2^64 the preceding DOUBLE is 2048 smaller. Construct these values
'' from their bit patterns so the test does not depend on decimal parsing.
if( convert_unsigned(from_bits(&h43EFFFFFFFFFFFFFull)) <> &hFFFFFFFFFFFFF800ull ) then end 7
if( convert_unsigned(from_bits(&h43F0000000000000ull)) <> &h8000000000000000ull ) then end 8
if( convert_signed(2147483647.5) <> clng(&h80000000u) ) then end 9
if( convert_signed(from_bits(&h7FF8000000000000ull)) <> clng(&h80000000u) ) then end 10
if( convert_unsigned(from_bits(&h7FF0000000000000ull)) <> &h8000000000000000ull ) then end 11

dim as double wide = from_bits(&h43F0000000000000ull)
if( cast(uinteger<128>, wide) <> (cast(uinteger<128>, 1) shl 64) ) then end 12

'' end of llvm-conversions.bas
