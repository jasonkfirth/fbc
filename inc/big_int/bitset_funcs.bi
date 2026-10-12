'' FreeBASIC big_int binding
'' File: bitset_funcs.bi
'' Purpose: Declare the matching big_int C interface.
'' Responsibilities: C bit outputs, random callback and native bit positions.
'' This file contains no arithmetic implementation or buffer allocation.
''
''
'' bitset_funcs -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __bigint_bitset_funcs_bi__
#define __bigint_bitset_funcs_bi__

#include once "crt/stddef.bi"
#include once "big_int/big_int.bi"

type big_int_rnd_fp as function cdecl() as long

declare sub      big_int_bit_length       cdecl alias "big_int_bit_length"       (byval a as big_int ptr, byval len as ulong ptr)
declare sub      big_int_bit1_cnt         cdecl alias "big_int_bit1_cnt"         (byval a as big_int ptr, byval cnt as ulong ptr)

declare function big_int_subint           cdecl alias "big_int_subint"           (byval a as big_int ptr, byval start_bit as size_t, byval bit_len as size_t, byval is_invert as long, byval answer as big_int ptr) as long

declare function big_int_or               cdecl alias "big_int_or"               (byval a as big_int ptr, byval b as big_int ptr, byval start_bit as size_t, byval answer as big_int ptr) as long 'MJK - Parm#3 is new!
declare function big_int_and              cdecl alias "big_int_and"              (byval a as big_int ptr, byval b as big_int ptr, byval start_bit as size_t, byval answer as big_int ptr) as long 'MJK - Parm#3 is new!
declare function big_int_andnot           cdecl alias "big_int_andnot"           (byval a as big_int ptr, byval b as big_int ptr, byval start_bit as size_t, byval answer as big_int ptr) as long 'MJK - Parm#3 is new!
declare function big_int_xor              cdecl alias "big_int_xor"              (byval a as big_int ptr, byval b as big_int ptr, byval start_bit as size_t, byval answer as big_int ptr) as long 'MJK - Parm#3 is new!

declare function big_int_set_bit          cdecl alias "big_int_set_bit"          (byval a as big_int ptr, byval n_bit as size_t, byval answer as big_int ptr) as long
declare function big_int_clr_bit          cdecl alias "big_int_clr_bit"          (byval a as big_int ptr, byval n_bit as size_t, byval answer as big_int ptr) as long
declare function big_int_inv_bit          cdecl alias "big_int_inv_bit"          (byval a as big_int ptr, byval n_bit as size_t, byval answer as big_int ptr) as long
declare function big_int_test_bit         cdecl alias "big_int_test_bit"         (byval a as big_int ptr, byval n_bit as size_t, byval bit_value as long ptr) as long

declare function big_int_scan1_bit        cdecl alias "big_int_scan1_bit"        (byval a as big_int ptr, byval pos_start as size_t, byval pos_found as size_t ptr) as long
declare function big_int_scan0_bit        cdecl alias "big_int_scan0_bit"        (byval a as big_int ptr, byval pos_start as size_t, byval pos_found as size_t ptr) as long
declare function big_int_hamming_distance cdecl alias "big_int_hamming_distance" (byval a as big_int ptr, byval b as big_int ptr,    byval distance as ulong ptr) as long
declare function big_int_rshift           cdecl alias "big_int_rshift"           (byval a as big_int ptr, byval n_bits as long, byval answer as big_int ptr) as long
declare function big_int_lshift           cdecl alias "big_int_lshift"           (byval a as big_int ptr, byval n_bits as long, byval answer as big_int ptr) as long
declare function big_int_rand             cdecl alias "big_int_rand"             (byval rand_func as big_int_rnd_fp, byval n_bits as size_t, byval answer as big_int ptr) as long

#endif

'' end of bitset_funcs.bi
