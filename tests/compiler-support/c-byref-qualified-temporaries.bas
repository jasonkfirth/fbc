'' Project: FreeBASIC argument qualification regressions
'' File: c-byref-qualified-temporaries.bas
'' Purpose: Qualify fresh pointer slots without changing caller-owned aliases.
'' Responsibilities: Check calls, conditional values, nested pointers and effects.
'' This file intentionally does NOT write through pointers to read-only objects.
#lang "fb"

type QualifiedPayload
	value as long
end type

dim shared as QualifiedPayload first_value = (41), second_value = (67)
dim shared slot as QualifiedPayload ptr = @first_value
dim shared as long checks, value_calls, alias_calls, address_matches

#macro VERIFY(condition)
	checks += 1
	if (condition) = 0 then
		print "BYREF_QUALIFIED_TEMPORARIES_FAILED "; __LINE__
		end 1
	end if
#endmacro

function FetchValue() as QualifiedPayload ptr
	value_calls += 1
	return slot
end function

function FetchAlias() byref as QualifiedPayload ptr
	alias_calls += 1
	return slot
end function

function FetchNested() as QualifiedPayload ptr ptr
	value_calls += 1
	return @slot
end function

function ReadBorrow(byref value as const QualifiedPayload ptr) as long
	if value = 0 then return 0
	return value->value
end function

function ReplaceTemporary(byref value as const QualifiedPayload ptr, _
	byval replacement as const QualifiedPayload ptr) as long
	value = replacement
	return value->value
end function

function ReadConstSlot(byref value as const QualifiedPayload const ptr) as long
	if cuint(@value) = cuint(@slot) then address_matches += 1
	return value->value
end function

function ReadNested(byref value as const QualifiedPayload const ptr ptr) as long
	return (*value)->value
end function

function ReadDefault(byref value as const QualifiedPayload ptr = @first_value) as long
	return value->value
end function

#ifdef QUALIFIED_ALIAS_PROOF
function ReplaceAlias(byref value as const QualifiedPayload ptr, _
	byval replacement as const QualifiedPayload ptr) as long
	'' Both actual objects are mutable. Observe the caller's slot before this
	'' call returns, so replacing the real alias with a copy cannot pass.
	value = replacement
	return slot->value
end function
#endif

VERIFY(ReadBorrow(FetchValue()) = 41)
VERIFY(value_calls = 1)
VERIFY(ReplaceTemporary(FetchValue(), @second_value) = 67)
VERIFY(slot = @first_value)
VERIFY(value_calls = 2)
VERIFY(ReadBorrow(@second_value) = 67)
VERIFY(ReplaceTemporary(iif(true, FetchValue(), @second_value), @second_value) = 67)
VERIFY(value_calls = 3)
VERIFY(slot = @first_value)
VERIFY(ReadBorrow(iif(false, FetchValue(), @second_value)) = 67)
VERIFY(value_calls = 3)
VERIFY(ReadConstSlot(FetchAlias()) = 41)
VERIFY(alias_calls = 1)
VERIFY(address_matches = 1)
VERIFY(slot = @first_value)
VERIFY(ReadNested(FetchNested()) = 41)
VERIFY(value_calls = 4)
VERIFY(ReadBorrow(cptr(QualifiedPayload ptr, 0)) = 0)
VERIFY(ReadDefault() = 41)
VERIFY(slot = @first_value)
VERIFY(value_calls = 4)
VERIFY(ReadBorrow(FetchValue() + 0) = 41)
VERIFY(value_calls = 5)
#ifdef QUALIFIED_ALIAS_PROOF
VERIFY(ReplaceAlias(slot, @second_value) = 67)
VERIFY(slot = @second_value)
if checks <> 25 then end 2
#else
if checks <> 23 then end 2
#endif
print "BYREF_QUALIFIED_TEMPORARIES_PASS"

'' end of c-byref-qualified-temporaries.bas
