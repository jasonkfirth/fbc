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
''     - classify small x86-64 Linux aggregate results for the native ABI
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

const LLVM_SYSV_EIGHTBYTE_BYTES = 8
const LLVM_SYSV_RESULT_PARTS = 2
const LLVM_SYSV_SMALL_RESULT_BYTES = LLVM_SYSV_EIGHTBYTE_BYTES * LLVM_SYSV_RESULT_PARTS
const LLVM_BITS_PER_BYTE = 8

enum LLVMSYSVRESULTCLASS
	LLVM_SYSV_NO_CLASS
	LLVM_SYSV_SSE
	LLVM_SYSV_INTEGER
end enum

type LLVMSYSVRESULTPART
	kind as LLVMSYSVRESULTCLASS
	bytes as integer
end type

type LLVMSYSVRESULT
	subtype as FBSYMBOL ptr
	in_memory as integer
	part(0 to LLVM_SYSV_RESULT_PARTS - 1) as LLVMSYSVRESULTPART
end type

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
	'' The source alias resolves to its byte-backed UDT during ABI lowering.
	var subtype_name = ucase( *symbGetName( subtype ) )
	return (subtype_name = "CLONGDOUBLE") orElse _
	       (subtype_name = "__CLONGDOUBLE_STORAGE")
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

'' -------------------------------------------------------------------------
'' Small System V aggregate results
'' -------------------------------------------------------------------------

private sub hMergeSystemVResultBytes _
	( byref result as LLVMSYSVRESULT, byval first_byte as integer, _
	  byval end_byte as integer, byval kind as LLVMSYSVRESULTCLASS )

	assert( first_byte >= 0 andalso first_byte < end_byte )
	assert( end_byte <= LLVM_SYSV_SMALL_RESULT_BYTES )
	for part as integer = first_byte \ LLVM_SYSV_EIGHTBYTE_BYTES to _
	                       (end_byte - 1) \ LLVM_SYSV_EIGHTBYTE_BYTES
		var used = end_byte - part * LLVM_SYSV_EIGHTBYTE_BYTES
		if( used > LLVM_SYSV_EIGHTBYTE_BYTES ) then used = LLVM_SYSV_EIGHTBYTE_BYTES
		if( used > result.part(part).bytes ) then result.part(part).bytes = used
		'' INTEGER wins when fields share bytes, including union members.
		if( kind > result.part(part).kind ) then result.part(part).kind = kind
	next
end sub

private function hClassifySystemVResultFields _
	( byval subtype as FBSYMBOL ptr, byval base_offset as integer, _
	  byref result as LLVMSYSVRESULT ) as integer

	if( subtype = NULL orelse hIsCLongDoubleType( FB_DATATYPE_STRUCT, subtype ) ) then _
		return FALSE
	if( subtype->lgt <= 0 orelse subtype->lgt > LLVM_SYSV_SMALL_RESULT_BYTES - base_offset ) then _
		return FALSE

	'' Visit every union member and every element of nested arrays. The
	'' register class is based on absolute byte offsets, not the LLVM fields
	'' chosen to describe the aggregate's storage.
	var fld = symbUdtGetFirstField( subtype )
	while( fld )
		var dtype = symbGetType( fld )
		var element_type = symbGetSubtype( fld )
		var field_size = symbGetSizeOf( fld )
		var available = subtype->lgt - fld->ofs
		if( fld->ofs < 0 orelse available <= 0 orelse symbIsDynamic( fld ) ) then return FALSE

		var field_offset = base_offset + cint( fld->ofs )
		if( symbIsBitfield( fld ) ) then
			'' Bitfields occupy only their declared bits. Their backing
			'' integer type need not fit in the remaining bytes of a packed UDT.
			var first_bit = fld->var_.bitpos
			var end_bit = first_bit + fld->var_.bits
			if( first_bit < 0 orelse end_bit > available * LLVM_BITS_PER_BYTE ) then return FALSE
			hMergeSystemVResultBytes( result, field_offset + first_bit \ LLVM_BITS_PER_BYTE, _
			    field_offset + (end_bit + LLVM_BITS_PER_BYTE - 1) \ LLVM_BITS_PER_BYTE, _
			    LLVM_SYSV_INTEGER )
		else
			var elements = symbGetArrayElements( fld )
			if( field_size <= 0 orelse elements <= 0 orelse elements > available \ field_size ) then _
				return FALSE
			var data_class = typeGetClass( dtype )
			if( data_class = FB_DATACLASS_UDT andalso element_type = NULL ) then return FALSE
			var alignment = typeCalcNaturalAlign( dtype, element_type )
			if( data_class = FB_DATACLASS_UDT ) then
				'' A packed nested type can begin at any byte allowed by
				'' its pack setting. Its members still have to be checked
				'' at their absolute offsets before choosing registers.
				var pack = element_type->udt.align
				if( pack > 0 andalso pack < alignment ) then alignment = pack
			end if
			if( alignment <= 0 ) then return FALSE
			if( field_offset mod alignment <> 0 ) then result.in_memory = TRUE

			select case data_class
			case FB_DATACLASS_UDT
				for element as integer = 0 to cint( elements ) - 1
					if( hClassifySystemVResultFields( element_type, _
					    field_offset + element * cint( field_size ), result ) = FALSE ) then return FALSE
				next
			case FB_DATACLASS_FPOINT
				hMergeSystemVResultBytes( result, field_offset, _
				    field_offset + cint( elements * field_size ), LLVM_SYSV_SSE )
			case FB_DATACLASS_INTEGER
				if( dtype = FB_DATATYPE_XMMWORD ) then return FALSE
				hMergeSystemVResultBytes( result, field_offset, _
				    field_offset + cint( elements * field_size ), LLVM_SYSV_INTEGER )
			case else
				if( dtype <> FB_DATATYPE_FIXSTR ) then return FALSE
				hMergeSystemVResultBytes( result, field_offset, _
				    field_offset + cint( elements * field_size ), LLVM_SYSV_INTEGER )
			end select
		end if
		fld = symbUdtGetNextField( fld )
	wend
	return TRUE
end function

private function hGetLinuxX64SmallResult( byval proc as FBSYMBOL ptr ) as LLVMSYSVRESULT
	dim as LLVMSYSVRESULT result
	if( proc = NULL orelse env.clopt.target <> FB_COMPTARGET_LINUX orelse _
	    fbGetCpuFamily( ) <> FB_CPUFAMILY_X86_64 ) then return result
	if( symbProcReturnsOnStack( proc ) orelse _
	    typeGetDtAndPtrOnly( symbGetProcRealType( proc ) ) <> FB_DATATYPE_STRUCT ) then return result

	var subtype = symbGetProcRealSubtype( proc )
	if( hClassifySystemVResultFields( subtype, 0, result ) ) then
		if( result.part(0).kind <> LLVM_SYSV_NO_CLASS orelse _
		    result.part(1).kind <> LLVM_SYSV_NO_CLASS ) then result.subtype = subtype
	end if
	return result
end function

private function hSystemVResultPartType( byref part as const LLVMSYSVRESULTPART ) as string
	assert( part.kind <> LLVM_SYSV_NO_CLASS )
	if( part.kind = LLVM_SYSV_SSE ) then
		if( part.bytes <= typeGetSize( FB_DATATYPE_SINGLE ) ) then return "float"
		'' Two single values can share one SSE slot. Treat the slot as
		'' bits, with no floating point conversion or arithmetic.
		return "double"
	end if
	return "i" & (part.bytes * LLVM_BITS_PER_BYTE)
end function

private function hSystemVResultType( byref result as const LLVMSYSVRESULT ) as string
	if( result.in_memory ) then return "void"
	if( result.part(0).kind = LLVM_SYSV_NO_CLASS ) then return hSystemVResultPartType( result.part(1) )
	if( result.part(1).kind = LLVM_SYSV_NO_CLASS ) then return hSystemVResultPartType( result.part(0) )
	return "{ " + hSystemVResultPartType( result.part(0) ) + ", " + _
		hSystemVResultPartType( result.part(1) ) + " }"
end function

private function hSystemVResultPointerAttribute( byval subtype as FBSYMBOL ptr ) as string
	return " sret(" + hEmitType( FB_DATATYPE_STRUCT, subtype ) + ")"
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
