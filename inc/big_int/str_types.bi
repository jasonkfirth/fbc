'' FreeBASIC big_int binding
'' File: str_types.bi
'' Purpose: Declare the matching big_int C interface.
'' Responsibilities: Native string buffer lengths and ownership layout.
'' This file contains no arithmetic implementation or buffer allocation.
''
''
'' str_types -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __bigint_str_types_bi__
#define __bigint_str_types_bi__

#include once "crt/stddef.bi"

type big_int_str
	str as zstring ptr
	len as size_t
	len_allocated as size_t
end type

#endif

'' end of str_types.bi
