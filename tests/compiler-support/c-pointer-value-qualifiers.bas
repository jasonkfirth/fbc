/'
	Project: FreeBASIC C backend regression controls
	File: c-pointer-value-qualifiers.bas
	Purpose: Preserve pointed-to const through intermediate pointer arithmetic.
	Responsibilities: Narrow and wide offsets, single call evaluation, mutable
	aliases, escaped bindings and nested pointer storage.
	This file intentionally does NOT contain: Platform APIs or threading.
'/
#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_POINTER_VALUE_QUALIFIERS_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as long narrow_calls, wide_calls

type NarrowBox extends zstring
	data as zstring * 16
	declare operator cast() byref as const zstring
end type

type WideBox extends wstring
	data as wstring * 16
	declare operator cast() byref as const wstring
end type

operator NarrowBox.cast() byref as const zstring
	narrow_calls += 1
	return data
end operator

operator WideBox.cast() byref as const wstring
	wide_calls += 1
	return data
end operator

sub RebindNarrow(byref value as zstring ptr, byval replacement as zstring ptr)
	value = replacement
end sub

private sub ComparePointers()
	dim as NarrowBox narrow_value
	dim as WideBox wide_value
	narrow_value.data = "abcd"
	wide_value.data = "abcd"

	dim as zstring ptr first_narrow = strptr(narrow_value) + 1
	dim as zstring ptr second_narrow = @(strptr(narrow_value)[2])
	VERIFY(cuint(first_narrow) = cuint(@narrow_value.data) + 1)
	VERIFY(cuint(second_narrow) - cuint(first_narrow) = 1)
	first_narrow = strptr(narrow_value) + 3
	VERIFY(cuint(first_narrow) = cuint(@narrow_value.data) + 3)
	VERIFY(narrow_calls = 3)
	second_narrow = 0
	VERIFY(second_narrow = 0)

	dim as wstring ptr first_wide = strptr(wide_value) + 1
	dim as wstring ptr second_wide = @(strptr(wide_value)[2])
	VERIFY(cuint(first_wide) = cuint(@wide_value.data) + sizeof(wstring))
	VERIFY(cuint(second_wide) - cuint(first_wide) = sizeof(wstring))
	first_wide = strptr(wide_value) + 3
	VERIFY(cuint(first_wide) = cuint(@wide_value.data) + 3 * sizeof(wstring))
	VERIFY(wide_calls = 3)
	second_wide = 0
	VERIFY(second_wide = 0)
#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif
end sub

private sub MutableAliases()
	dim as NarrowBox narrow_value
	dim as WideBox wide_value
	narrow_value.data = "abcd"
	wide_value.data = "abcd"
	'' The views name mutable BASIC storage. A real mutable conversion must
	'' remain when the pointer is used to write rather than compare addresses.
	dim as zstring ptr narrow_alias = cptr(zstring ptr, strptr(narrow_value) + 1)
	dim as wstring ptr wide_alias = cptr(wstring ptr, strptr(wide_value) + 1)
	*narrow_alias = "XY"
	*wide_alias = "XY"
	VERIFY(narrow_value.data = "aXY")
	VERIFY(wide_value.data = "aXY")
end sub

private sub EscapedBinding()
	dim as zstring * 16 first_value = "first", second_value = "second"
	dim as zstring ptr value = @first_value
	dim as zstring ptr ptr slot = @value
	RebindNarrow(value, @second_value)
	VERIFY(*slot = @second_value)
	**slot = "changed"
	VERIFY(second_value = "changed")
end sub

ComparePointers()
MutableAliases()
EscapedBinding()
print "C_POINTER_VALUE_QUALIFIERS_PASS"

/' end of c-pointer-value-qualifiers.bas '/
