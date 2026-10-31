#lang "fb"
/'
	Project: FreeBASIC C backend regression tests
	File: c-va-pointer-cursors.bas
	Purpose: Check native cursor operations in BASIC pointer storage.
	Responsibilities: Verify independent copies, BYREF updates, packed sentinels,
	single address evaluation, and ordinary native cursor objects.
	This file intentionally does NOT contain: Diagnostic suppression or copies
	of foreign struct/array va_list representations.
'/

#macro VERIFY(condition)
	if (condition) = 0 then end 1
#endmacro

#macro OPAQUE_BOUNDARY()
	#ifdef CHECK_OPAQUE
		asm
			nop
		end asm
	#endif
#endmacro

type CursorPair
	before as ulong
	first as cva_list
	middle as ulong
	second as cva_list
	after as ulong
end type

extern NativePair alias "fb_test_va_slots" as CursorPair
dim shared NativePair as CursorPair
dim shared as long getter_calls

private function PairAddress() as CursorPair ptr
	getter_calls += 1
	return @NativePair
end function

private function ReadNext(byref cursor as cva_list) as longint
	OPAQUE_BOUNDARY()
	return cva_arg(cursor, longint)
end function

private function RunPair cdecl(byval count as long, ...) as longint
	OPAQUE_BOUNDARY()
	NativePair.before = &h12345678ul
	NativePair.middle = &h19283746ul
	NativePair.after = &h31415926ul
	getter_calls = 0
	cva_start(PairAddress()->first, count)
	cva_copy(PairAddress()->second, PairAddress()->first)
	dim as longint first_total = ReadNext(NativePair.first)
	dim as longint second_total = cva_arg(NativePair.second, longint)
	VERIFY(first_total = 10 and second_total = 10)
	first_total += cva_arg(PairAddress()->first, longint)
	second_total += cva_arg(NativePair.second, longint)
	for i as long = 3 to count
		first_total += ReadNext(NativePair.first)
		second_total += cva_arg(NativePair.second, longint)
	next
	cva_end(PairAddress()->first)
	cva_end(PairAddress()->second)
	VERIFY(getter_calls = 6)
	VERIFY(first_total = 100 and second_total = 100)
	VERIFY(NativePair.before = &h12345678ul)
	VERIFY(NativePair.middle = &h19283746ul)
	VERIFY(NativePair.after = &h31415926ul)
	return first_total + second_total
end function

type PackedCursor field = 1
	before as ubyte
	args as cva_list
	after as ubyte
end type

private function RunPacked cdecl(byval count as long, ...) as longint
	OPAQUE_BOUNDARY()
	dim slot as PackedCursor
	slot.before = &hA5
	slot.after = &h5A
	cva_start(slot.args, count)
	dim as longint total = 0
	for i as long = 1 to count
		total += cva_arg(slot.args, longint)
	next
	cva_end(slot.args)
	VERIFY(slot.before = &hA5 and slot.after = &h5A)
	return total
end function

private function RunNative cdecl(byval count as long, ...) as longint
	OPAQUE_BOUNDARY()
	dim cursor as cva_list = any
	cva_start(cursor, count)
	dim as longint total = 0
	for i as long = 1 to count
		total += cva_arg(cursor, longint)
	next
	cva_end(cursor)
	return total
end function

for repeat as long = 1 to 3
	VERIFY(RunPair(4, 10ll, 20ll, 30ll, 40ll) = 200)
	VERIFY(RunPacked(4, 10ll, 20ll, 30ll, 40ll) = 100)
	VERIFY(RunNative(4, 10ll, 20ll, 30ll, 40ll) = 100)
next

print "C_VA_POINTER_CURSORS_PASS"

/' end of c-va-pointer-cursors.bas '/
