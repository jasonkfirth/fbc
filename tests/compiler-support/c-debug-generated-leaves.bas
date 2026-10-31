/'
	Project: FreeBASIC C backend regression controls
	File: c-debug-generated-leaves.bas
	Purpose: Keep live generated members and source debug bodies.
	Responsibilities: Default construction, nested copy and assignment, string
	ownership, callback initialization, and a compile-time private procedure address query.
	This file intentionally does NOT contain: Native warning suppressions,
	allocation failure injection or source-level ABI changes.
'/
#lang "fb"

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_DEBUG_GENERATED_LEAVES_FAILED", __LINE__
		end 1
	end if
#endmacro

type UncopiedValue
	text as string
	value as long
end type

type CopiedValue
	text as string
	value as long
end type

type NestedValue
	child as CopiedValue
end type

private function SourceBody cdecl alias "debug_source_body"() as long
	return 39
end function

private function LiveCallback cdecl alias "debug_live_callback"(byval value as long) as long
	return value + 3
end function

dim shared callback as function cdecl(byval as long) as long = @LiveCallback

#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif

'' A type query does not call the procedure. Debug builds still keep its
'' source-defined body available for inspection.
VERIFY(sizeof(procptr(SourceBody)) = sizeof(any ptr))
VERIFY(callback(7) = 10)

scope
	dim as UncopiedValue original
	original.text = "uncopied"
	original.value = 17
	VERIFY(original.text = "uncopied")
	VERIFY(original.value = 17)

	dim as NestedValue first_value
	first_value.child.text = "copied"
	first_value.child.value = 23
	dim as NestedValue second_value = first_value
	VERIFY(second_value.child.text = "copied")
	VERIFY(second_value.child.value = 23)
	first_value.child.text = "assigned"
	first_value.child.value = 29
	second_value = first_value
	first_value.child.text = "changed"
	VERIFY(second_value.child.text = "assigned")
	VERIFY(second_value.child.value = 29)
	VERIFY(first_value.child.text = "changed")
end scope
print "C_DEBUG_GENERATED_LEAVES_PASS"

/' end of c-debug-generated-leaves.bas '/
