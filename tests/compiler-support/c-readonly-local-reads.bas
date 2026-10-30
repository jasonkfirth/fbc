/'
	Project: FreeBASIC C backend regression controls
	File: c-readonly-local-reads.bas
	Purpose: Keep local read views qualified while retaining mutable interfaces.
	Responsibilities: Narrow/wide reads, existing const arguments, pointer steps,
	single evaluation, pointee writes, mutable calls and pointer-slot escapes.
	This file intentionally does NOT contain: Platform APIs or threading.
'/
#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_READONLY_LOCAL_READS_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as long narrow_calls, wide_calls
dim shared as zstring * 32 mutable_buffer = "before"
dim shared as zstring * 32 replacement_buffer = "replacement"

function NarrowValue() as const zstring ptr
	narrow_calls += 1
	return strptr("read")
end function

function WideValue() as const wstring ptr
	wide_calls += 1
	return @wstr("wide")
end function

function MutableStorage() as const zstring ptr
	'' The const view names mutable storage. The guarded conversions below
	'' may therefore write after recovering the BASIC mutable pointer type.
	return @mutable_buffer
end function

sub MutateText(byval value as zstring ptr)
	*value = "mutated"
end sub

sub MutateCharacter(byref value as zstring)
	value[0] = asc("X")
end sub

sub Rebind(byref value as zstring ptr)
	value = @replacement_buffer
end sub

function SlotPresent(byref value as zstring ptr) as long
	return value <> 0
end function

type PointerSlot
	value as zstring ptr
end type

private sub ReadPointers()
	dim as zstring ptr narrow_view = cptr(zstring ptr, NarrowValue())
	dim as wstring ptr wide_view = cptr(wstring ptr, WideValue())
	VERIFY(narrow_view[0] = asc("r"))
	VERIFY(len(*narrow_view) = 4)
	VERIFY(wide_view[1] = asc("i"))
	VERIFY(len(*wide_view) = 4)
	narrow_view += 1
	wide_view += 2
	VERIFY(*narrow_view = "ead")
	VERIFY(*wide_view = "de")
	VERIFY(narrow_calls = 1)
	VERIFY(wide_calls = 1)
#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif
end sub

private sub PointeeWrite()
	mutable_buffer = "before"
	dim as zstring ptr value = cptr(zstring ptr, MutableStorage())
	*value = "written"
	VERIFY(mutable_buffer = "written")
end sub

private sub MutableArgument()
	mutable_buffer = "before"
	dim as zstring ptr value = cptr(zstring ptr, MutableStorage())
	MutateText(value)
	VERIFY(mutable_buffer = "mutated")
end sub

private sub MutableReferenceArgument()
	mutable_buffer = "before"
	dim as zstring ptr value = cptr(zstring ptr, MutableStorage())
	MutateCharacter(*value)
	VERIFY(mutable_buffer = "Xefore")
end sub

private sub EscapedBinding()
	dim as zstring ptr value = cptr(zstring ptr, MutableStorage())
	Rebind(value)
	VERIFY(value = @replacement_buffer)
end sub

private sub ImplicitSlot()
	dim as zstring ptr value = cptr(zstring ptr, MutableStorage())
	'' A BYREF pointer formal can require an implicit pointer carrier when
	'' the actual is string storage. Its C slot type must remain compatible.
	VERIFY(SlotPresent(*value))
end sub

private sub EscapedValue()
	dim as zstring ptr value = cptr(zstring ptr, MutableStorage())
	dim as PointerSlot holder
	holder.value = value
	*holder.value = "escaped"
	VERIFY(mutable_buffer = "escaped")
end sub

ReadPointers()
PointeeWrite()
MutableArgument()
MutableReferenceArgument()
EscapedBinding()
ImplicitSlot()
EscapedValue()
print "C_READONLY_LOCAL_READS_PASS"

/' end of c-readonly-local-reads.bas '/
