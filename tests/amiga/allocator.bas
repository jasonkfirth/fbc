'' FreeBASIC classic AmigaOS allocator qualification
'' ------------------------------------------------
'' File: allocator.bas
'' Purpose: Check native heap contracts and REDIM PRESERVE capacity reuse.
'' Responsibilities: Run the C heap checks and verify BASIC array contents.
'' This file intentionally does NOT contain allocator implementations.

declare function amiga_test_allocator cdecl alias "amiga_test_allocator" () as long

dim result as long = amiga_test_allocator()
if result <> 0 then end result

dim values(any) as ulong
for i as long = 0 to 4095

	redim preserve values(0 to i)
	if values(i) <> 0 then end 80
	values(i) = culng(i) xor &h5a93c617u
next

for i as long = 0 to 4095
	if values(i) <> (culng(i) xor &h5a93c617u) then end 81
next

'' Reusing a heap block must not preserve discarded BASIC elements when
'' the logical array grows again. The array runtime clears the new range.
redim preserve values(0 to 1023)
redim preserve values(0 to 4095)
for i as long = 0 to 4095
	dim expected as ulong = 0
	if i <= 1023 then expected = culng(i) xor &h5a93c617u
	if values(i) <> expected then end 82
next
erase values

end 0

'' end of allocator.bas
