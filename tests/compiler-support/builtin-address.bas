'' Project: FreeBASIC C backend regression tests
'' File: builtin-address.bas
'' Purpose: Check addresses of C library builtins in generated C.
'' Responsibilities: Exercise global and local addresses and direct calls.
'' This fixture does not require compiler-specific runtime behavior.

#include once "builtin.bi"

dim shared copyBytes as function cdecl( byval as any ptr, byval as const any ptr, byval as __fb_builtin_size_t ) as any ptr = @__builtin_memcpy
dim shared compareBytes as function cdecl( byval as const any ptr, byval as const any ptr, byval as __fb_builtin_size_t ) as long = @__builtin_memcmp
dim shared textLength as function cdecl( byval as const zstring ptr ) as __fb_builtin_size_t = @__builtin_strlen

dim as function cdecl( byval as any ptr, byval as const any ptr, byval as __fb_builtin_size_t ) as any ptr moveBytes = @__builtin_memmove
dim as function cdecl( byval as any ptr, byval as long, byval as __fb_builtin_size_t ) as any ptr clearBytes = @__builtin_memset
dim as function cdecl( byval as any ptr, byval as const any ptr, byval as __fb_builtin_size_t ) as any ptr localCopy = @__builtin_memcpy

dim as zstring * 16 buffer
dim as const zstring ptr inputText = @"abcd"
if( copyBytes <> localCopy ) then end 1
if( copyBytes(@buffer, inputText, 5) <> @buffer ) then end 2
if( textLength(@buffer) <> 4 ) then end 3
if( compareBytes(@buffer, inputText, 5) <> 0 ) then end 4
if( moveBytes(@buffer[1], @buffer, 5) <> @buffer[1] ) then end 5
if( buffer <> "aabcd" ) then end 6
if( clearBytes(@buffer, 0, 16) <> @buffer ) then end 7
if( __builtin_memcpy(@buffer, inputText, 5) <> @buffer ) then end 8
if( __builtin_strlen(@buffer) <> 4 ) then end 9

print "BUILTIN_ADDRESS_PASS"

'' end of builtin-address.bas
