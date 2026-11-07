'' Project: FreeBASIC C backend regression tests
'' File: pointer-sized-types.bas
'' Purpose: Check native C types used by pointer-sized declarations.
'' Responsibilities: Expose size, difference and integer types to C checks.
'' This fixture does not require target runtime libraries.

#include once "crt/stddef.bi"
#include once "crt/stdint.bi"
extern "C"
declare function test_size alias "fbc_test_size"( byval as size_t ) as size_t
declare function test_ssize alias "fbc_test_ssize"( byval as ssize_t ) as ssize_t
declare function test_difference alias "fbc_test_difference"( byval as ptrdiff_t ) as ptrdiff_t
declare function test_intptr alias "fbc_test_intptr"( byval as intptr_t ) as intptr_t
declare function test_uintptr alias "fbc_test_uintptr"( byval as uintptr_t ) as uintptr_t
end extern

sub checkPointerSizedTypes cdecl alias "fbc_check_pointer_types"()
	test_size(not cuint(0))
	test_ssize(-2)
	test_difference(-3)
	test_intptr(-4)
	test_uintptr(not cuint(0))
end sub
'' end of pointer-sized-types.bas
