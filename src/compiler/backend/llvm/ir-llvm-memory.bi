'' Project: FreeBASIC compiler - LLVM memory lowering
'' -----------------------------------------------
''
'' File: backend/llvm/ir-llvm-memory.bi
''
'' Purpose:
''
''     Lower memory operations and prepare aligned aggregate argument copies.
''
'' Responsibilities:
''
''     - emit memory fill and move intrinsics with explicit byte alignment
''     - place BYVAL aggregate copies in aligned stack storage for each procedure
''
'' This file intentionally does NOT contain:
''
''     - source type checking, target ABI classification, or runtime allocation
''

#ifndef __FB_IR_LLVM_MEMORY_BI__
#define __FB_IR_LLVM_MEMORY_BI__

private sub _emitMem _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval v2 as IRVREG ptr, _
		byval bytes as longint, _
		byval fillchar as integer _
	)

	dim as string ln

	ln = "call void "

	select case( op )
	case AST_OP_MEMFILL
		hAstCommand( "memfill " + vregPretty( v1 ) )
	case AST_OP_MEMMOVE
		hAstCommand( "memmove " + vregPretty( v1 ) + " <= " + vregPretty( v2 ) )
	end select

	hLoadVreg( v1 )
	hLoadVreg( v2 )

	select case( op )
	case AST_OP_MEMFILL
		var countbits = typeGetSize( v2->dtype ) * 8
		if( countbits = 64 ) then
			builtins(BUILTIN_MEMSET64).used = TRUE
		else
			builtins(BUILTIN_MEMSET).used = TRUE
		end if
		_setVregDataType( v1, typeAddrOf( FB_DATATYPE_BYTE ), NULL )

		ln += "@llvm.memset.p0i8.i" & countbits & "( "
		ln += "i8* " + hVregToStr( v1 ) + ", "
		ln += "i8 " + str(fillchar) + ", "
		ln += "i" & countbits & " " + hVregToStr( v2 ) + ", "

	case AST_OP_MEMMOVE
		var countbits = typeGetSize( FB_DATATYPE_UINT ) * 8
		if( countbits = 64 ) then
			builtins(BUILTIN_MEMMOVE64).used = TRUE
		else
			builtins(BUILTIN_MEMMOVE).used = TRUE
		end if
		_setVregDataType( v1, typeAddrOf( FB_DATATYPE_BYTE ), NULL )
		_setVregDataType( v2, typeAddrOf( FB_DATATYPE_BYTE ), NULL )

		ln += "@llvm.memmove.p0i8.p0i8.i" & countbits & "( "
		ln += "i8* " + hVregToStr( v1 ) + ", "
		ln += "i8* " + hVregToStr( v2 ) + ", "
		ln += "i" & countbits & " " + str( bytes ) + ", "

	end select

	ln += "i32 1, i1 false )"

	hWriteLine( ln )
end sub

private function hCopyMemoryByval _
	( byval value as IRVREG ptr, byval dtype as integer, byval subtype as FBSYMBOL ptr ) as IRVREG ptr

	'' LLVM's byval alignment also describes the source pointer. A packed
	'' field need not meet the stack-copy alignment, so materialize an aligned
	'' value before the call. Hoist storage to avoid per-iteration stack growth.
	var slot = irhlAllocVreg( typeAddrOf(dtype), subtype )
	var valuetype = hEmitType( dtype, subtype )
	var allocline = string( ctx.indent, TABCHAR ) + hVregToStr(slot) + _
	    " = alloca " + valuetype + ", align " & hMemoryByvalAlignment(subtype) & NEWLINE
	hAppendOutputBuffer( @ctx.proc_allocas, strptr(allocline), len(allocline) )

	if( irIsREG(value) andalso (typeIsPtr(value->dtype) = FALSE) ) then
		hWriteLine( "store " + valuetype + " " + hVregToStr(value) + ", " + _
		    valuetype + "* " + hVregToStr(slot) )
	else
		hPrepareAddress( value )
		_setVregDataType( value, typeAddrOf(dtype), subtype )
		'' The memory helper uses byte alignment. Preserve the typed slot
		'' register while it converts the copy's destination to a byte pointer.
		dim as IRVREG destination = *slot
		_emitMem( AST_OP_MEMMOVE, @destination, value, subtype->lgt, 0 )
	end if
	return slot
end function

#endif

'' end of backend/llvm/ir-llvm-memory.bi
