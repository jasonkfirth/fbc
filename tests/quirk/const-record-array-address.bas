'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC C backend regression tests
'' File: const-record-array-address.bas
'' Purpose: Preserve const record views when taking array element addresses.
'' Responsibilities: Exercise generated assignment, pointer slots and one evaluation.
'' This file does not test writes rejected by the BASIC frontend.

type const_array_record
	name as string
	numbers(0 to 2) as integer
	pointers(0 to 2) as integer ptr
end type

dim shared as integer source_calls

function record_source( byref value as const const_array_record ) as const const_array_record ptr
	source_calls += 1
	return @value
end function

sub copy_record( byref source as const const_array_record, byref target as const_array_record )
	var number_address = @source.numbers(1)
	if *number_address <> 23 then end 1
	target = source
end sub

dim as integer first = 17, second = 29
dim as const_array_record source, target
source.name = "source"
source.numbers(0) = 11
source.numbers(1) = 23
source.numbers(2) = 37
source.pointers(0) = @first
source.pointers(1) = @second
source.pointers(2) = @first

copy_record(*record_source(source), target)
if source_calls <> 1 then end 2
if target.name <> "source" then end 3
if target.numbers(0) <> 11 or target.numbers(1) <> 23 or target.numbers(2) <> 37 then end 4
if target.pointers(0) <> @first or target.pointers(1) <> @second or target.pointers(2) <> @first then end 5
if *target.pointers(1) <> 29 then end 6
target.name = "target"
target.numbers(1) = 41
if source.name <> "source" or source.numbers(1) <> 23 then end 7

print "CONST_RECORD_ARRAY_ADDRESS_PASS"

'' end of const-record-array-address.bas
