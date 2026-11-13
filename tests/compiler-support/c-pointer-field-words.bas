/'
	Project: FreeBASIC C backend regression tests
	File: c-pointer-field-words.bas
	Purpose: Preserve pointer-field storage when IR carries native integer words.
	Responsibilities: Check word reads/copies, packed fields, short object views,
	const views, callable slots and single evaluation of storage addresses.
	This file intentionally does NOT dereference fabricated address values.
'/
#lang "fb"

type WordCallback as function cdecl(byval value as long) as long

type FieldWords
	first_value as any ptr
	second_value as any ptr
	handler as WordCallback
end type

type PackedWords field = 1
	before_value as ubyte
	value as any ptr
	after_value as ubyte
end type

dim shared as integer checks, address_calls

#macro VERIFY(condition)
	checks += 1
	if (condition) = 0 then
		print "POINTER_FIELD_WORDS_FAILED "; __LINE__
		end 1
	end if
#endmacro

function ReadFirst(byval record as const FieldWords ptr) as uinteger
	return cuint(record->first_value)
end function

function ReadSecond(byval record as const FieldWords ptr) as uinteger
	return cuint(record->second_value)
end function

function StorageAddress(byval record as FieldWords ptr) as FieldWords ptr
	address_calls += 1
	return record
end function

sub WriteSecond(byval record as FieldWords ptr, byval bits as uinteger)
	*cptr(uinteger ptr, cptr(ubyte ptr, StorageAddress(record)) + _
		offsetof(FieldWords, second_value)) = bits
end sub

function AddOne cdecl(byval value as long) as long
	return value + 1
end function

dim as integer first_target = 17, second_target = 29
dim as FieldWords record
record.first_value = @first_target
record.second_value = @second_target
record.handler = @AddOne
VERIFY(ReadFirst(@record) = cuint(@first_target))
VERIFY(ReadSecond(@record) = cuint(@second_target))
WriteSecond(@record, cuint(@first_target))
VERIFY(address_calls = 1)
VERIFY(record.second_value = @first_target)
VERIFY(record.first_value = @first_target)
VERIFY(cuint(record.handler) = cuint(@AddOne))
VERIFY(record.handler(41) = 42)

'' This view owns just the first slot. A byte copy must not acquire a
'' containing-record read or copy merely from the pointer's declared type.
dim as any ptr prefix_slot = @second_target
VERIFY(ReadFirst(cptr(const FieldWords ptr, @prefix_slot)) = cuint(@second_target))

'' The pointer is deliberately unaligned; the two sentinels must survive.
dim as PackedWords packed
packed.before_value = &h5a
packed.after_value = &ha5
packed.value = @first_target
VERIFY(cuint(packed.value) = cuint(@first_target))
*cptr(uinteger ptr, @packed.value) = cuint(@second_target)
VERIFY(packed.value = @second_target)
VERIFY(packed.before_value = &h5a andalso packed.after_value = &ha5)

'' Sixteen consecutive byte origins cover every pointer alignment used by
'' the supported 32/64-bit targets, independently of the local's placement.
'' Use an ordinary record here; its view must retain byte alignment too.
dim storage(0 to sizeof(FieldWords) + 16) as ubyte
for byte_offset as integer = 0 to 15
	var record_address = cptr(FieldWords ptr, @storage(byte_offset + 1))
	storage(byte_offset) = &h5a
	storage(byte_offset + 1 + sizeof(FieldWords)) = &ha5
	record_address->first_value = @first_target
	record_address->handler = @AddOne
	WriteSecond(record_address, cuint(@first_target))
	VERIFY(ReadSecond(record_address) = cuint(@first_target))
	WriteSecond(record_address, cuint(@second_target))
	VERIFY(ReadSecond(record_address) = cuint(@second_target))
	VERIFY(ReadFirst(record_address) = cuint(@first_target) andalso _
		record_address->handler(41) = 42 andalso _
		storage(byte_offset) = &h5a andalso _
		storage(byte_offset + 1 + sizeof(FieldWords)) = &ha5 andalso _
		address_calls = 1 + (byte_offset + 1) * 2)
next

print "POINTER_FIELD_WORDS_PASS "; checks

/' end of c-pointer-field-words.bas '/
