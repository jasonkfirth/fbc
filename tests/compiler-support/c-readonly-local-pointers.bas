/'
	Project: FreeBASIC C backend regression controls
	File: c-readonly-local-pointers.bas
	Purpose: Keep literal pointer values qualified without changing bindings.
	Responsibilities: Address comparisons, rebinding, call evaluation, mutable
	aliases, pointee writes and an escaped pointer slot.
	This file intentionally does NOT contain: Platform APIs or threading.
'/
#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_READONLY_LOCAL_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as long literal_calls
dim shared as wstring * 32 rebound_buffer = "rebound"

function LiteralValue() as const wstring ptr
	literal_calls += 1
	return @wstr("returned")
end function

sub Rebind(byref value as wstring ptr)
	value = @rebound_buffer
end sub

function SlotPresent(byref value as zstring ptr) as long
	return value <> 0
end function

private sub CompareAddresses()
	dim as wstring ptr first_value, second_value
	first_value = @wstr("first")
	second_value = @wstr("second")
	VERIFY(first_value <> second_value)
	first_value = @wstr("second")
	VERIFY(first_value = second_value)
	first_value = cptr(wstring ptr, LiteralValue())
	VERIFY(first_value <> second_value)
	VERIFY(literal_calls = 1)
	second_value = cptr(wstring ptr, LiteralValue())
	VERIFY(first_value = second_value)
	VERIFY(literal_calls = 2)
	first_value = 0
	VERIFY(first_value = 0)
#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif
end sub

private sub MutableAliases()
	dim as zstring * 32 buffer = "before"
	dim as zstring ptr original = @buffer
	dim as zstring ptr other = original
	*other = "after"
	VERIFY(buffer = "after")
	VERIFY(original = other)
end sub

private sub EscapedBinding()
	dim as wstring * 32 buffer = "before"
	dim as wstring ptr value = @buffer
	Rebind(value)
	VERIFY(value = @rebound_buffer)
end sub

CompareAddresses()
MutableAliases()
EscapedBinding()
dim as zstring * 32 slot_buffer = "slot"
VERIFY(SlotPresent(slot_buffer))
print "C_READONLY_LOCAL_POINTERS_PASS"

/' end of c-readonly-local-pointers.bas '/
