'' Project: FreeBASIC C backend regression tests
'' File: stdio-contract-mixed.bas
'' Purpose: Preserve distinct source stream views without inventing one FILE type.
'' Responsibilities: Keep both external procedure addresses and their own types.
'' This file intentionally does NOT call either procedure or define stream storage.

#include once "crt/stddef.bi"

type FirstStream
	first_value as long
end type

type SecondStream
	second_value as longint
end type

extern "C"
	declare function first_open alias "fopen" (byval as const zstring ptr, byval as const zstring ptr) as FirstStream ptr
	declare function second_read alias "fread" (byval as any ptr, byval as size_t, byval as size_t, byval as SecondStream ptr) as size_t
end extern

public dim shared mixed_open as typeof(procptr(first_open)) = procptr(first_open)
public dim shared mixed_read as typeof(procptr(second_read)) = procptr(second_read)

'' end of stdio-contract-mixed.bas
