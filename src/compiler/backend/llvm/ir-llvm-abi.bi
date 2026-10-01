'' Project: FreeBASIC compiler - LLVM ABI policy
'' -----------------------------------------
''
'' File: backend/llvm/ir-llvm-abi.bi
''
'' Purpose:
''
''     Describe LLVM parameter lowering that differs from BASIC storage types.
''
'' Responsibilities:
''
''     - identify large System V x86-64 aggregates passed in caller-owned memory
''     - keep declarations, calls, and parameter allocation on the same ABI
''
'' This file intentionally does NOT contain:
''
''     - source type checking or aggregate classification for other target ABIs
''

#ifndef __FB_IR_LLVM_ABI_BI__
#define __FB_IR_LLVM_ABI_BI__

private function hIsMemoryByval _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr, byval mode as integer ) as integer

	if( mode <> FB_PARAMMODE_BYVAL ) then return FALSE
	if( typeGetDtAndPtrOnly( dtype ) <> FB_DATATYPE_STRUCT ) then return FALSE
	if( subtype = NULL ) then return FALSE
	'' C's va_list array typedef decays to a pointer at a call boundary.
	'' Its backing struct is not an ordinary aggregate passed by value.
	if( symbIsBuiltinVaListType( dtype, subtype ) ) then return FALSE
	if( fbGetCpuFamily() <> FB_CPUFAMILY_X86_64 ) then return FALSE
	if( (env.target.options and FB_TARGETOPT_UNIX) = 0 ) then return FALSE

	'' System V can pass at most two eightbyte aggregate classes in registers.
	'' Larger trivial structs use a byval pointer in LLVM IR, which lowers to
	'' a stack copy at the C boundary. A bare LLVM struct value has a different
	'' register allocation contract, even when BASIC-only calls appear to work.
	'' Non-trivial BASIC values already have pointer real types and are excluded.
	return (subtype->lgt > 16)
end function

private function hMemoryByvalAlignment( byval subtype as FBSYMBOL ptr ) as integer
	'' System V rounds stack aggregate alignment up to eight bytes.
	var align = subtype->udt.natalign
	if( align < 8 ) then align = 8
	return align
end function

private function hMemoryByvalAttribute( byval subtype as FBSYMBOL ptr ) as string
	var align = hMemoryByvalAlignment( subtype )
	return " byval(" + hEmitType( FB_DATATYPE_STRUCT, subtype ) + ") align " & align
end function

#endif

'' end of backend/llvm/ir-llvm-abi.bi
