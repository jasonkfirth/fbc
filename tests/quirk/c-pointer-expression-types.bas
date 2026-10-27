'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-pointer-expression-types.bas
'' Purpose: Check C pointer-result types, comparisons and signed differences.
'' Responsibilities: Cover qualified/commuted offsets, record subtypes, one
''     evaluation per operand and unchanged numeric promotion behavior.
'' This file intentionally does NOT form addresses beyond the one-past limit.
#lang "fb"

type expression_record
	value as integer
	weight as double
end type
dim shared rows(0 to 7) as expression_record
dim shared as integer operand_calls
private function record_base() as const expression_record ptr
	operand_calls += 1
	return @rows(3)
end function
private function record_step() as integer
	operand_calls += 1
	return -2
end function

for index as integer = 0 to 7
	rows(index).value = index * 11
	rows(index).weight = index + 0.25
next
dim as const expression_record ptr cursor = @rows(1)
cursor = 2 + cursor
if( cursor <> @rows(3) or cursor->value <> 33 ) then end 1
if( cursor->weight <> 3.25 ) then end 2
if( @rows(0) - cursor <> -3 ) then end 3
if( cursor - @rows(0) <> 3 ) then end 4
cursor = record_base() + record_step()
if( cursor <> @rows(1) or cursor->value <> 11 ) then end 5
if( operand_calls <> 2 ) then end 6
dim as integer displacement = -1
cursor = cursor - displacement
if( cursor <> @rows(2) or cursor->value <> 22 ) then end 7
cursor = @rows(7) + 1
if( cursor - @rows(0) <> 8 ) then end 8

'' ANY PTR offsets count bytes; the C comparison must use matching types.
dim as any ptr raw_address = @rows(0)
raw_address += sizeof(expression_record)
if( raw_address <> @rows(1) ) then end 9
raw_address += -1
if( cuint(raw_address) + 1 <> cuint(@rows(1)) ) then end 10

dim as short small_signed = -7
dim as ushort small_unsigned = 3
if( small_signed + small_unsigned <> -4 ) then end 11
dim as ulongint large_unsigned = (1ull shl 40)
if( large_unsigned + 9ull <> (1ull shl 40) + 9ull ) then end 12
print "POINTER_EXPRESSION_TYPES_PASS"
'' end of c-pointer-expression-types.bas
