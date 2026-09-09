'' FreeBASIC graphics tests: paint-alloc.bas
'' Verify allocation failures leave pixels unchanged and free discovered spans.
'' Instrument PAINT allocations through paint-alloc.c; no device is used.
' TEST_MODE : MULTI_MODULE_TEST

#include once "fbgfx.bi"

declare sub fail_at cdecl alias "fb_test_paint_fail_at" ( byval allocation as long )
declare function outstanding cdecl alias "fb_test_paint_outstanding" () as long

if screenres( 12, 10, 8, 1, fb.GFX_NULL ) <> 0 then end 1
dim pattern as string = chr( 255, 0, 128 )
'' One row table plus one span per row: fail every possible allocation.
for allocation as long = 1 to 11
	line ( 0, 0 )-( 11, 9 ), 2, bf
	fail_at allocation
	if fb.PaintPattern( 0, 5, 5, pattern, 1, 0, 3 ) <> 4 then end 2
	if outstanding() <> 0 then end 3
	for y as integer = 0 to 9
		for x as integer = 0 to 11
			if point( x, y ) <> 2 then end 4
		next
	next
next
pattern = chr( 255 )
fail_at 0
if fb.PaintPattern( 0, 5, 5, pattern, 1, 0, 3 ) <> 0 then end 5
if outstanding() <> 0 then end 6
if point( 0, 0 ) <> 1 or point( 11, 9 ) <> 1 then end 7
screen 0
print "paint-alloc: passed"

'' end of paint-alloc.bas
