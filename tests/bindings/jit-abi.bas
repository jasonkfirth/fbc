'' Project: FreeBASIC native library regression checks
'' File: jit-abi.bas
'' Purpose: Report binding layout for comparison with the real C headers.
'' Responsibilities: Scalar widths, constant layout and memory manager callbacks.
'' This file contains no assumptions about C long-double calling conventions.

#lang "fb"
#include once "jit.bi"

Type constant_alignment
	prefix As Byte
	value As jit_constant_t
End Type

Print "ABI "; SizeOf(jit_int); " "; SizeOf(jit_uint); " "; SizeOf(jit_nint); " "; _
	SizeOf(jit_nuint); " "; SizeOf(jit_size_t); " "; SizeOf(jit_label_t); " "; _
	SizeOf(jit_nfloat); " "; SizeOf(jit_constant_t); " "; _
	OffsetOf(jit_constant_t, un); " "; OffsetOf(constant_alignment, value); " "; _
	SizeOf(jit_memory_manager); " "; OffsetOf(jit_memory_manager, alloc_data)

'' end of jit-abi.bas
