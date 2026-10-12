'' FreeBASIC big_int binding
'' File: big_int.bi
'' Purpose: Declare the matching big_int C interface.
'' Responsibilities: 32-bit limbs, C enum widths and native allocation lengths.
'' This file contains no arithmetic implementation or buffer allocation.
''
''
'' big_int -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __bigint_big_int_bi__
#define __bigint_big_int_bi__

#inclib "big_int"

#include once "crt/stddef.bi"
#include once "big_int/memory_manager.bi"

#define BIG_INT_DIGIT_SIZE 32

type big_int_word as ulong
type big_int_dword as ulongint

'' C enums remain 32-bit on both Win64 and Unix64. Allocation lengths
'' follow size_t instead, so their width must track the native C ABI.
type sign_type as long
enum
	PLUS
	MINUS
end enum


type bin_op_type as long
enum
	OP_ADD
	OP_SUB
	OP_MUL
	OP_DIV
	OP_MOD
	OP_CMP
	OP_POW
	OP_GCD
	OP_OR
	OP_XOR
	OP_AND
	OP_ANDNOT
end enum


type big_int
	num as big_int_word ptr
	sign as sign_type
	len as size_t
	len_allocated as size_t
end type

#endif

'' end of big_int.bi
