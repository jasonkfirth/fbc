'' Project: FreeBASIC compiler - LLVM ABI policy
'' -----------------------------------------
''
'' File: backend/llvm/ir-llvm-abi.bi
''
'' Purpose:
''
''     Describe LLVM procedure lowering that differs from BASIC storage types.
''
'' Responsibilities:
''
''     - lower i386 128-bit scalars to the aligned two-word C ABI
''     - identify large System V x86-64 aggregates passed in caller-owned memory
''     - keep declarations, calls, returns, and parameter storage on one ABI
''
'' This file intentionally does NOT contain:
''
''     - source type checking or ABI rules for other processor families
''

#ifndef __FB_IR_LLVM_ABI_BI__
#define __FB_IR_LLVM_ABI_BI__

'' Match the C backend's aligned i386 FB_INT128 pair at procedure boundaries.
'' The low 64-bit word is passed before the high word; results use the caller's slot.
const LLVM_INT128_STORAGE_ALIGNMENT = 16
const LLVM_I386_INT128_WORD_BITS = 64
const LLVM_I386_INT128_ABI_ALIGNMENT = LLVM_INT128_STORAGE_ALIGNMENT

private function hUsesI386Integer128ABI( ) as integer
	function = (fbGetCpuFamily( ) = FB_CPUFAMILY_X86) and (fbGetBits( ) = 32)
end function

private function hIsI386Integer128Param _
	( _
		byval dtype as integer, _
		byval mode as integer _
	) as integer

	function = hUsesI386Integer128ABI( ) and _
	           (mode = FB_PARAMMODE_BYVAL) and _
	           typeIsInteger128Scalar( dtype )
end function

private function hIsI386Integer128Result( byval proc as FBSYMBOL ptr ) as integer
	function = hUsesI386Integer128ABI( ) andalso _
	           symbProcReturnsOnStack( proc ) andalso _
	           typeIsInteger128Scalar( symbGetFullType( proc ) )
end function

private sub hSplitI386Integer128 _
	( _
		byval value as IRVREG ptr, _
		byref low_name as string, _
		byref high_name as string _
	)

	hLoadVreg( value )

	dim as IRVREG ptr low_word = irhlAllocVreg( FB_DATATYPE_ULONGINT, NULL )
	hWriteLine( hVregToStr( low_word ) + " = trunc i128 " + _
	            hVregToStr( value ) + " to i64" )
	low_name = hVregToStr( low_word )

	dim as IRVREG ptr shifted = irhlAllocVreg( FB_DATATYPE_UINTEGER128, NULL )
	hWriteLine( hVregToStr( shifted ) + " = lshr i128 " + _
	            hVregToStr( value ) + ", " + str( LLVM_I386_INT128_WORD_BITS ) )
	dim as IRVREG ptr high_word = irhlAllocVreg( FB_DATATYPE_ULONGINT, NULL )
	hWriteLine( hVregToStr( high_word ) + " = trunc i128 " + _
	            hVregToStr( shifted ) + " to i64" )
	high_name = hVregToStr( high_word )
end sub

private function hJoinI386Integer128Words _
	( _
		byref low_name as string, _
		byref high_name as string _
	) as string

	dim as IRVREG ptr low_word = irhlAllocVreg( FB_DATATYPE_UINTEGER128, NULL )
	hWriteLine( hVregToStr( low_word ) + " = zext i64 " + low_name + " to i128" )

	dim as IRVREG ptr high_word = irhlAllocVreg( FB_DATATYPE_UINTEGER128, NULL )
	hWriteLine( hVregToStr( high_word ) + " = zext i64 " + high_name + " to i128" )
	dim as IRVREG ptr shifted = irhlAllocVreg( FB_DATATYPE_UINTEGER128, NULL )
	hWriteLine( hVregToStr( shifted ) + " = shl i128 " + hVregToStr( high_word ) + _
	            ", " + str( LLVM_I386_INT128_WORD_BITS ) )
	dim as IRVREG ptr result = irhlAllocVreg( FB_DATATYPE_UINTEGER128, NULL )
	hWriteLine( hVregToStr( result ) + " = or i128 " + _
	            hVregToStr( shifted ) + ", " + hVregToStr( low_word ) )
	function = hVregToStr( result )
end function

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

private function hIsCLongDoubleType _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr ) as integer

	if( typeGetDtAndPtrOnly( dtype ) <> FB_DATATYPE_STRUCT ) then return FALSE
	if( subtype = NULL ) then return FALSE
	return (*symbGetName( subtype ) = "CLONGDOUBLE")
end function

private function hUsesCLongDoubleCABI( byval proc as FBSYMBOL ptr ) as integer
	if( proc = NULL ) then return FALSE
	return (symbGetProcMode( proc ) = FB_FUNCMODE_CDECL)
end function

private function hIsSystemVVarargAggregate _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr, byval mode as integer ) as integer

	if( mode <> FB_PARAMMODE_VARARG ) then return FALSE
	if( typeGetDtAndPtrOnly( dtype ) <> FB_DATATYPE_STRUCT ) then return FALSE
	if( subtype = NULL ) then return FALSE
	if( symbIsBuiltinVaListType( dtype, subtype ) ) then return FALSE
	if( hIsCLongDoubleType( dtype, subtype ) ) then return FALSE
	return (fbGetCpuFamily() = FB_CPUFAMILY_X86_64) andalso _
	       ((env.target.options and FB_TARGETOPT_UNIX) <> 0)
end function

private function hIsMicrosoftVarargAggregate _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr, byval mode as integer ) as integer

	if( mode <> FB_PARAMMODE_VARARG ) then return FALSE
	if( typeGetDtAndPtrOnly( dtype ) <> FB_DATATYPE_STRUCT ) then return FALSE
	if( subtype = NULL ) then return FALSE
	if( symbIsBuiltinVaListType( dtype, subtype ) ) then return FALSE
	if( hIsCLongDoubleType( dtype, subtype ) ) then return FALSE
	return (fbGetCpuFamily() = FB_CPUFAMILY_X86_64) andalso _
	       (env.clopt.target = FB_COMPTARGET_WIN32)
end function

private function hGetCLongDoubleLLVMType( byval subtype as FBSYMBOL ptr ) as string
	if( subtype = NULL ) then return ""
	if( subtype->lgt = 8 ) then return "double"

	select case fbGetCpuFamily( )
	case FB_CPUFAMILY_X86, FB_CPUFAMILY_X86_64
		return "x86_fp80"
	case FB_CPUFAMILY_PPC64, FB_CPUFAMILY_PPC64LE
		return "ppc_fp128"
	case FB_CPUFAMILY_AARCH64, FB_CPUFAMILY_RISCV64, _
	     FB_CPUFAMILY_S390X, FB_CPUFAMILY_LOONGARCH64, _
	     FB_CPUFAMILY_MIPS64, FB_CPUFAMILY_MIPS64EL
		return "fp128"
	case else
		return ""
	end select
end function

private function hGetSystemVStructRegisterCounts _
	( byval subtype as FBSYMBOL ptr, byref integer_count as integer, _
	  byref floating_count as integer ) as integer

	integer_count = 0
	floating_count = 0
	if( subtype = NULL ) then return FALSE
	if( (subtype->lgt <= 0) or (subtype->lgt > 16) ) then return FALSE

	select case as const hGetMagicStructNumber( subtype )
	case KSTRUCT_R
		integer_count = 1
	case KSTRUCT_X
		floating_count = 1
	case KSTRUCT_RR
		integer_count = 2
	case KSTRUCT_RX
		integer_count = 1
		floating_count = 1
	case KSTRUCT_XR
		integer_count = 1
		floating_count = 1
	case KSTRUCT_XX
		floating_count = 2
	case else
		return FALSE
	end select

	return TRUE
end function

private sub hGetSystemVArgumentRegisterCounts _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr, _
	  byval mode as integer, byref integer_count as integer, _
	  byref floating_count as integer )

	integer_count = 0
	floating_count = 0
	if( typeGetDtAndPtrOnly( dtype ) = FB_DATATYPE_STRUCT ) then
		if( (mode = FB_PARAMMODE_BYVAL) or (mode = FB_PARAMMODE_VARARG) ) then
			if( symbIsBuiltinVaListType( dtype, subtype ) ) then
				integer_count = 1
			elseif( hGetSystemVStructRegisterCounts( _
			        subtype, integer_count, floating_count ) = FALSE ) then
				'' Memory-class aggregates do not consume argument registers.
			end if
		else
			'' BYREF and BYDESC parameters cross the call boundary as pointers.
			integer_count = 1
		end if
	elseif( typeGetClass( dtype ) = FB_DATACLASS_FPOINT ) then
		floating_count = 1
	else
		integer_count = 1
	end if
end sub

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
