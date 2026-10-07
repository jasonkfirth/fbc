'' Project: FreeBASIC compiler - LLVM IR backend
'' -----------------------------------------
''
'' File: backend/llvm/ir-llvm-builtins.bi
''
'' Purpose:
''
''     Lower builtin.bi declarations to LLVM intrinsics and libc operations.
''
'' Responsibilities:
''
''     - serialize typed IR, data layouts, and control flow as LLVM IR
''     - maintain SSA identities, declarations, and intrinsic lowering
''
'' This file intentionally does NOT contain:
''
''     - BASIC grammar or invocation of llc and the linker
''

'' FreeBASIC Compiler
'' File: ir-llvm-builtins.bi
'' Purpose: Lower the compiler-only declarations exposed by builtin.bi.
'' Responsibilities: Emit LLVM intrinsics, adapt their results to BASIC types,
'' and validate operands that LLVM requires to be constant.
'' This file does not parse declarations or implement libc functions.

#ifndef __FB_IR_LLVM_BUILTINS_BI__
#define __FB_IR_LLVM_BUILTINS_BI__

private sub hDeclareIntrinsic( byref pname as string, byref signature as string )
	if( hashLookup( @ctx.declaredprocs.hash, strptr( pname ) ) <> NULL ) then exit sub
	strsetAdd( @ctx.declaredprocs, pname, 0 )
	var oldsection = ctx.section
	ctx.section = SECTION_HEAD
	hWriteLine( "declare " + signature )
	ctx.section = oldsection
end sub

private function hBuiltinConstant _
	( byval arg as IRVREG ptr, byval minimum as longint, byval maximum as longint ) as integer
	if( irIsIMM( arg ) = FALSE ) then
		errReport( FB_ERRMSG_EXPECTEDCONST, , ": builtin argument" )
		return FALSE
	end if
	if( typeIsPtr( arg->dtype ) or (typeGetClass( arg->dtype ) <> FB_DATACLASS_INTEGER) or _
	    (arg->value.i < minimum) or (arg->value.i > maximum) ) then
		errReport( FB_ERRMSG_INVALIDDATATYPES, , ": builtin argument outside permitted range" )
		return FALSE
	end if
	return TRUE
end function

private sub hBuiltinResult( byval vr as IRVREG ptr, byval value as IRVREG ptr )
	if( vr = NULL ) then exit sub
	_setVregDataType( value, vr->dtype, vr->subtype )
	if( irIsREG( vr ) ) then
		*vr = *value
	else
		hEmitStore( vr, value )
	end if
end sub

private function hBuiltinObjectBytes( byval arg as IRVREG ptr ) as longint
	'' A pointer variable's storage size says nothing about its pointee.
	'' Only a direct address of a fixed object supplies a known extent here.
	if( (arg->sym = NULL) or (arg->vidx <> NULL) ) then return -1
	if( symbIsVar( arg->sym ) = FALSE ) then return -1
	dim dtype as integer, subtype as FBSYMBOL ptr
	symbGetRealType( arg->sym, dtype, subtype )
	'' An address REG can name the alloca of a pointer temporary. Call
	'' preparation loads that pointer value; its storage is not the pointee.
	if( typeIsPtr( dtype ) ) then return -1
	if( arg->dtype <> typeAddrOf( dtype ) ) then return -1
	if( symbGetIsDynamic( arg->sym ) ) then return -1
	var bytes = symbGetRealSize( arg->sym )
	if( (arg->ofs < 0) or (arg->ofs > bytes) ) then return -1
	'' A UDT offset can refer to a field rather than to the complete object.
	if( (typeGetDtOnly( dtype ) = FB_DATATYPE_STRUCT) and (arg->ofs <> 0) ) then return -1
	return bytes - arg->ofs
end function

private function hEmitBuiltinCall _
	( byval proc as FBSYMBOL ptr, byval vr as IRVREG ptr, byval level as integer ) as integer

	var builtin = hGetBuiltin( proc )
	if( (builtin = LLVM_BUILTIN_NONE) or (builtin = LLVM_BUILTIN_LIBC) ) then return FALSE

	'' Consume only this call's arguments. Nested calls share the same list.
	'' Three operands cover every declaration, including optional prefetch hints.
	dim args(0 to 2) as IRVREG ptr
	dim as integer argc = 0
	dim as longint objectbytes = -1
	var arg = cptr( IRCALLARG ptr, listGetTail( @irhl.callargs ) )
	while( arg andalso (arg->level = level) )
		var prev = cptr( IRCALLARG ptr, listGetPrev( arg ) )
		if( argc <= ubound( args ) ) then
			if( argc = 0 ) then objectbytes = hBuiltinObjectBytes( arg->vr )
			args(argc) = hLoadCallArg( arg )
		end if
		argc += 1
		listDelNode( @irhl.callargs, arg )
		arg = prev
	wend

	dim as integer expected = 1
	select case( builtin )
	case LLVM_BUILTIN_OBJECT_SIZE, LLVM_BUILTIN_DYNAMIC_OBJECT_SIZE, LLVM_BUILTIN_EXPECT
		expected = 2
	case LLVM_BUILTIN_OVERFLOW, LLVM_BUILTIN_EXPECT_PROBABILITY
		expected = 3
	case LLVM_BUILTIN_PREFETCH
		if( (argc >= 1) and (argc <= 3) ) then expected = argc
	case LLVM_BUILTIN_TRAP, LLVM_BUILTIN_UNREACHABLE
		expected = 0
	end select
	if( argc <> expected ) then
		errReport( FB_ERRMSG_ARGCNTMISMATCH, , ": builtin call" )
		return TRUE
	end if

	'' ALIAS can name a builtin without including builtin.bi. Reject malformed
	'' declarations here instead of sending an invalid intrinsic type to llc.
	dim as integer validtypes = TRUE
	select case( builtin )
	case LLVM_BUILTIN_FFS, LLVM_BUILTIN_CLZ, LLVM_BUILTIN_CTZ, LLVM_BUILTIN_CLRSB, _
	     LLVM_BUILTIN_POPCOUNT, LLVM_BUILTIN_PARITY, LLVM_BUILTIN_BSWAP, _
	     LLVM_BUILTIN_OVERFLOW, LLVM_BUILTIN_EXPECT, LLVM_BUILTIN_EXPECT_PROBABILITY
		validtypes = (typeIsPtr( args(0)->dtype ) = FALSE) and _
		    (typeGetClass( args(0)->dtype ) = FB_DATACLASS_INTEGER)
		var bytes = typeGetSize( args(0)->dtype )
		validtypes and= (bytes = 4) or (bytes = 8) or _
		    ((builtin = LLVM_BUILTIN_BSWAP) and (bytes = 2))
		if( (builtin = LLVM_BUILTIN_OVERFLOW) or (builtin = LLVM_BUILTIN_EXPECT) or _
		    (builtin = LLVM_BUILTIN_EXPECT_PROBABILITY) ) then
			validtypes and= hEmitType( args(0)->dtype, args(0)->subtype ) = _
			    hEmitType( args(1)->dtype, args(1)->subtype )
		end if
		if( builtin = LLVM_BUILTIN_OVERFLOW ) then
			validtypes and= hEmitType( args(2)->dtype, args(2)->subtype ) = _
			    hEmitType( args(0)->dtype, args(0)->subtype ) + "*"
		elseif( builtin = LLVM_BUILTIN_EXPECT_PROBABILITY ) then
			validtypes and= typeGetDtAndPtrOnly( args(2)->dtype ) = FB_DATATYPE_DOUBLE
		end if
	case LLVM_BUILTIN_OBJECT_SIZE, LLVM_BUILTIN_DYNAMIC_OBJECT_SIZE, LLVM_BUILTIN_PREFETCH
		validtypes = hEmitType( args(0)->dtype, args(0)->subtype ) = "i8*"
	end select
	if( validtypes = FALSE ) then
		errReport( FB_ERRMSG_INVALIDDATATYPES, , ": invalid builtin declaration: " + *symbGetName( proc ) )
		return TRUE
	end if

	if( (builtin = LLVM_BUILTIN_TRAP) or (builtin = LLVM_BUILTIN_UNREACHABLE) ) then
		if( builtin = LLVM_BUILTIN_TRAP ) then
			hDeclareIntrinsic( "@llvm.trap", "void @llvm.trap()" )
			hWriteLine( "call void @llvm.trap()" )
		end if
		hWriteLine( "unreachable" )
		'' The AST can emit more instructions after a terminating builtin.
		'' Put those instructions in a fresh, unreachable basic block.
		hWriteLabel( symbUniqueLabel( ) )
		return TRUE
	end if

	dim as string dtype = hEmitType( args(0)->dtype, args(0)->subtype )
	dim as string operand = hVregToStr( args(0) )
	dim as string pname, signature
	dim as IRVREG ptr result = NULL

	select case( builtin )
	case LLVM_BUILTIN_FFS, LLVM_BUILTIN_CLZ, LLVM_BUILTIN_CTZ, _
	     LLVM_BUILTIN_CLRSB, LLVM_BUILTIN_POPCOUNT, LLVM_BUILTIN_PARITY, LLVM_BUILTIN_BSWAP
		dim as string opname, extra
		select case( builtin )
		case LLVM_BUILTIN_FFS, LLVM_BUILTIN_CTZ
			opname = "cttz"
		case LLVM_BUILTIN_CLZ, LLVM_BUILTIN_CLRSB
			opname = "ctlz"
		case LLVM_BUILTIN_POPCOUNT, LLVM_BUILTIN_PARITY
			opname = "ctpop"
		case LLVM_BUILTIN_BSWAP
			opname = "bswap"
		end select
		if( builtin = LLVM_BUILTIN_CLRSB ) then
			'' Complement negative values before counting redundant sign bits.
			var signbits = irhlAllocVreg( args(0)->dtype, NULL )
			var magnitude = irhlAllocVreg( args(0)->dtype, NULL )
			hWriteLine( hVregToStr( signbits ) + " = ashr " + dtype + " " + operand + _
			    ", " & (typeGetSize( args(0)->dtype ) * 8 - 1) )
			hWriteLine( hVregToStr( magnitude ) + " = xor " + dtype + " " + operand + _
			    ", " + hVregToStr( signbits ) )
			operand = hVregToStr( magnitude )
		end if
		pname = "@llvm." + opname + "." + dtype
		signature = dtype + " " + pname + "(" + dtype
		if( (opname = "ctlz") or (opname = "cttz") ) then
			signature += ", i1"
			'' GCC leaves clz/ctz(0) undefined. ffs and clrsb define it.
			if( (builtin = LLVM_BUILTIN_CLZ) or (builtin = LLVM_BUILTIN_CTZ) ) then
				extra = ", i1 true"
			else
				extra = ", i1 false"
			end if
		end if
		hDeclareIntrinsic( pname, signature + ")" )
		result = irhlAllocVreg( args(0)->dtype, NULL )
		hWriteLine( hVregToStr( result ) + " = call " + dtype + " " + pname + _
		    "(" + dtype + " " + operand + extra + ")" )
		select case( builtin )
		case LLVM_BUILTIN_FFS
			var count = irhlAllocVreg( result->dtype, NULL )
			var iszero = irhlAllocVreg( FB_DATATYPE_BOOLEAN, NULL )
			var selected = irhlAllocVreg( result->dtype, NULL )
			hWriteLine( hVregToStr( count ) + " = add " + dtype + " " + hVregToStr( result ) + ", 1" )
			hWriteLine( hVregToStr( iszero ) + " = icmp eq " + dtype + " " + operand + ", 0" )
			hWriteLine( hVregToStr( selected ) + " = select i1 " + hVregToStr( iszero ) + _
			    ", " + dtype + " 0, " + dtype + " " + hVregToStr( count ) )
			result = selected
		case LLVM_BUILTIN_CLRSB, LLVM_BUILTIN_PARITY
			var adjusted = irhlAllocVreg( result->dtype, NULL )
			var op = iif( builtin = LLVM_BUILTIN_CLRSB, "sub", "and" )
			hWriteLine( hVregToStr( adjusted ) + " = " + op + " " + dtype + " " + _
			    hVregToStr( result ) + ", 1" )
			result = adjusted
		end select

	case LLVM_BUILTIN_OVERFLOW
		var opname = mid( *proc->id.alias, 11, 4 )
		pname = "@llvm." + opname + ".with.overflow." + dtype
		var pairtype = "{ " + dtype + ", i1 }"
		hDeclareIntrinsic( pname, pairtype + " " + pname + "(" + dtype + ", " + dtype + ")" )
		'' The pair and flag registers use names only; their LLVM types are
		'' not BASIC types. Normalize the i1 flag to the C ABI's i8 boolean.
		var pair = irhlAllocVreg( FB_DATATYPE_STRUCT, NULL )
		var value = irhlAllocVreg( args(0)->dtype, NULL )
		var overflow = irhlAllocVreg( FB_DATATYPE_BOOLEAN, NULL )
		result = irhlAllocVreg( FB_DATATYPE_BOOLEAN, NULL )
		hWriteLine( hVregToStr( pair ) + " = call " + pairtype + " " + pname + _
		    "(" + dtype + " " + operand + ", " + dtype + " " + hVregToStr( args(1) ) + ")" )
		hWriteLine( hVregToStr( value ) + " = extractvalue " + pairtype + " " + hVregToStr( pair ) + ", 0" )
		hWriteLine( "store " + dtype + " " + hVregToStr( value ) + ", " + _
		    hEmitType( args(2)->dtype, args(2)->subtype ) + " " + hVregToStr( args(2) ) )
		hWriteLine( hVregToStr( overflow ) + " = extractvalue " + pairtype + " " + hVregToStr( pair ) + ", 1" )
		hWriteLine( hVregToStr( result ) + " = zext i1 " + hVregToStr( overflow ) + " to i8" )

	case LLVM_BUILTIN_OBJECT_SIZE, LLVM_BUILTIN_DYNAMIC_OBJECT_SIZE
		if( hBuiltinConstant( args(1), 0, 3 ) = FALSE ) then return TRUE
		var mode = args(1)->value.i
		var resultdtype = typeGetDtAndPtrOnly( symbGetProcRealType( proc ) )
		if( objectbytes >= 0 ) then
			result = irhlAllocVrImm( resultdtype, NULL, objectbytes )
		elseif( (mode and 1) <> 0 ) then
			'' LLVM tracks the enclosing allocation, not C subobject bounds.
			'' If the field extent was lost, return GCC's conservative unknown.
			result = irhlAllocVrImm( resultdtype, NULL, iif( (mode and 2) <> 0, 0, -1 ) )
		else
			var resulttype = hEmitType( resultdtype, NULL )
			pname = "@llvm.objectsize." + resulttype + ".p0"
			hDeclareIntrinsic( pname, resulttype + " " + pname + "(i8*, i1, i1, i1)" )
			result = irhlAllocVreg( resultdtype, NULL )
			var minimum = iif( (mode and 2) <> 0, "true", "false" )
			var isdynamic = iif( builtin = LLVM_BUILTIN_DYNAMIC_OBJECT_SIZE, "true", "false" )
			hWriteLine( hVregToStr( result ) + " = call " + resulttype + " " + pname + _
			    "(i8* " + operand + ", i1 " + minimum + ", i1 true, i1 " + isdynamic + ")" )
		end if

	case LLVM_BUILTIN_EXPECT, LLVM_BUILTIN_EXPECT_PROBABILITY
		pname = "@llvm.expect"
		dim as string extra
		dim as integer withprobability = FALSE
		if( builtin = LLVM_BUILTIN_EXPECT_PROBABILITY ) then
			if( irIsIMM( args(2) ) = FALSE ) then
				errReport( FB_ERRMSG_EXPECTEDCONST, , ": builtin probability" )
				return TRUE
			end if
			if( not ((args(2)->value.f >= 0.0) and (args(2)->value.f <= 1.0)) ) then
				errReport( FB_ERRMSG_INVALIDDATATYPES, , ": builtin probability outside [0, 1]" )
				return TRUE
			end if
			'' ARM LLVM can select llvm.expect but not the probability form. Drop
			'' only the optimization probability hint on ARM.
			if( fbGetCpuFamily( ) <> FB_CPUFAMILY_ARM ) then
				pname += ".with.probability"
				extra = ", double " + hVregToStr( args(2) )
				withprobability = TRUE
			end if
		end if
		pname += "." + dtype
		signature = dtype + " " + pname + "(" + dtype + ", " + dtype
		if( withprobability ) then signature += ", double"
		hDeclareIntrinsic( pname, signature + ")" )
		result = irhlAllocVreg( args(0)->dtype, NULL )
		hWriteLine( hVregToStr( result ) + " = call " + dtype + " " + pname + _
		    "(" + dtype + " " + operand + ", " + dtype + " " + hVregToStr( args(1) ) + extra + ")" )

	case LLVM_BUILTIN_PREFETCH
		dim as longint rw = 0
		dim as longint locality = 3
		if( argc >= 2 ) then
			if( hBuiltinConstant( args(1), 0, 1 ) = FALSE ) then return TRUE
			rw = args(1)->value.i
		end if
		if( argc >= 3 ) then
			if( hBuiltinConstant( args(2), 0, 3 ) = FALSE ) then return TRUE
			locality = args(2)->value.i
		end if
		'' The fourth operand selects the data cache, as GCC prefetch does.
		hDeclareIntrinsic( "@llvm.prefetch.p0", "void @llvm.prefetch.p0(i8*, i32, i32, i32)" )
		hWriteLine( "call void @llvm.prefetch.p0(i8* " + operand + _
		    ", i32 " & rw & ", i32 " & locality & ", i32 1)" )
	end select

	if( result <> NULL ) then hBuiltinResult( vr, result )
	return TRUE
end function

#endif

'' end of backend/llvm/ir-llvm-builtins.bi
