'' Project: FreeBASIC compiler - C pointer arithmetic
'' File: backend/c/c-pointer-arithmetic.bi
'' Purpose: Keep pointer qualifiers and recover exact typed element offsets.
'' Responsibilities: Preserve byte-address constness and check constant scaling.
'' This file intentionally does NOT contain: pointer differences or array bounds.

private function hCBytePointerType _
	( byval dtype as integer, byval read_only as integer = FALSE ) as integer

	'' A byte view addresses the pointed-to object. Const on that object
	'' survives; const on the pointer value itself is irrelevant here.
	var result = FB_DATATYPE_UBYTE
	if( read_only orelse (typeIsPtr(dtype) andalso typeIsConstAt(dtype, 1)) ) then
		result = typeSetIsConst(result)
	end if
	return typeAddrOf(result)
end function

private function hCAccessPointerType _
	( byval dtype as integer, byval is_lvalue as integer ) as integer

	'' A C load needs only a read-only view, including when lowering has lost
	'' the source address's qualifier. The loaded value keeps its IR type;
	'' qualifying its binding does not change its own pointer target.
	if( is_lvalue = FALSE ) then
		dtype = typeSetIsConst(dtype)
	end if
	return typeAddrOf(dtype)
end function

private sub hCReadAddress( byval address as EXPRNODE ptr, byval remaining as integer = 64 )
	'' This tree is owned by a scalar read. Qualify its storage addresses,
	'' including intermediate casts, without changing the loaded value type.
	'' Stop at integer conversions and opaque text; neither proves a pointer
	'' path. The budget bounds traversal of source-generated cast chains.
	if( remaining <= 0 ) then exit sub
	if( typeIsPtr(address->dtype) = FALSE ) then exit sub
	if( typeGetPtrCnt(address->dtype) = 1 andalso _
	    typeGetDtOnly(address->dtype) = FB_DATATYPE_FUNCTION ) then exit sub
	address->dtype = typeAddrOf(typeSetIsConst(typeDeref(address->dtype)))
	select case address->class
	case EXPRCLASS_CAST
		if( typeIsPtr(address->l->dtype) ) then hCReadAddress(address->l, remaining - 1)
	case EXPRCLASS_BOP
		select case address->op
		case AST_OP_ADD, AST_OP_SUB
			if( typeIsPtr(address->l->dtype) ) then hCReadAddress(address->l, remaining - 1)
			if( typeIsPtr(address->r->dtype) ) then hCReadAddress(address->r, remaining - 1)
		end select
	end select
end sub

private function hCByteOffsetPointer( byval dtype as integer ) as integer

	dtype = typeUnsetIsConst(dtype)
	return (dtype = typeAddrOf(FB_DATATYPE_UBYTE)) orelse _
	       (dtype = typeAddrOf(typeSetIsConst(FB_DATATYPE_UBYTE)))
end function

private function hCByteOffsetInteger( byval dtype as integer ) as integer

	select case typeUnsetIsConst(dtype)
	case FB_DATATYPE_INTEGER, FB_DATATYPE_LONG, FB_DATATYPE_LONGINT
		return TRUE
	end select
	return FALSE
end function

private function hCByteOffsetNativeInteger _
	( byval value as EXPRNODE ptr, byref remaining as integer ) as integer

	'' Bound proof work per address. Larger or unknown expressions keep their
	'' existing path; casts establish a C result type without walking inside.
	if( remaining <= 0 ) then return FALSE
	remaining -= 1
	if( hCByteOffsetInteger(value->dtype) = FALSE ) then return FALSE
	if( typeGetSize(value->dtype) <> env.pointersize ) then return FALSE
	select case value->class
	case EXPRCLASS_CAST, EXPRCLASS_MEMBER
		'' MEMBER records the actual field type recovered from its declaration.
		return TRUE
	case EXPRCLASS_SYM
		dim as integer dtype = any
		dim as FBSYMBOL ptr subtype = any
		symbGetRealType( value->sym, dtype, subtype )
		return hCByteOffsetInteger(dtype) andalso _
		       (typeGetSize(dtype) = env.pointersize)
	case EXPRCLASS_IMM
		'' hEmitInt gives native signed literals the corresponding C type.
		if( env.pointersize = 4 ) then
			if( (value->val.i < -2147483648ll) or _
			    (value->val.i > 2147483647ll) ) then return FALSE
		end if
		return TRUE
	case EXPRCLASS_UOP
		select case value->op
		case AST_OP_NEG, AST_OP_NOT
			return hCByteOffsetNativeInteger(value->l, remaining)
		end select
	case EXPRCLASS_BOP
		'' These operators retain the proven native signed C operand type.
		'' Comparisons and arbitrary text cannot establish that result type.
		select case value->op
		case AST_OP_ADD, AST_OP_SUB, AST_OP_MUL, _
		     AST_OP_AND, AST_OP_OR, AST_OP_XOR, AST_OP_SHL, AST_OP_SHR
			return hCByteOffsetNativeInteger(value->l, remaining) andalso _
			       hCByteOffsetNativeInteger(value->r, remaining)
		end select
	end select
	return FALSE
end function

private function hCByteOffsetChain _
	( byval address as EXPRNODE ptr, byval correction as EXPRNODE ptr ) as EXPRNODE ptr

	if( (env.pointersize <> 4) and (env.pointersize <> 8) ) then return NULL
	if( hCByteOffsetPointer(address->dtype) = FALSE ) then return NULL
	if( correction->class <> EXPRCLASS_IMM ) then return NULL
	if( hCByteOffsetInteger(correction->dtype) = FALSE ) then return NULL
	if( correction->val.i = 0 ) then return NULL
	if( env.pointersize = 4 ) then
		if( (correction->val.i < -2147483648ll) or _
		    (correction->val.i > 2147483647ll) ) then return NULL
	end if

	'' Keep byte-view casts around the resulting address. Moving or removing
	'' them would change qualifier conversions independently of the offset.
	while( address->class = EXPRCLASS_CAST )
		if( hCByteOffsetPointer(address->l->dtype) = FALSE ) then return NULL
		address = address->l
	wend
	if( address->class <> EXPRCLASS_BOP ) then return NULL
	if( address->op <> AST_OP_ADD ) then return NULL
	if( hCByteOffsetPointer(address->l->dtype) = FALSE ) then return NULL
	if( hCByteOffsetInteger(address->r->dtype) = FALSE ) then return NULL
	if( typeGetSize(address->r->dtype) <> env.pointersize ) then return NULL

	'' Check the actual C types, not just the IR result types. Existing casts
	'' establish byte units or a native signed offset; retain those casts so
	'' wider and unsigned inputs are converted before the correction is added.
	dim as integer dtype = any
	dim as FBSYMBOL ptr subtype = any
	select case address->l->class
	case EXPRCLASS_CAST
	case EXPRCLASS_SYM
		symbGetRealType( address->l->sym, dtype, subtype )
		if( hCByteOffsetPointer(dtype) = FALSE ) then return NULL
	case else
		return NULL
	end select
	'' Sixty-four nodes cover ordinary scaled indices without making proof
	'' cost or recursion depend on an arbitrarily large expression tree.
	var remaining = 64
	if( hCByteOffsetNativeInteger(address->r, remaining) = FALSE ) then return NULL
	return address
end function

private function hCScalePointerOffset _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr, _
	  byval displacement as EXPRNODE ptr ) as integer

	if( typeIsPtr(dtype) = FALSE ) then return FALSE
	if( displacement->class <> EXPRCLASS_IMM ) then return FALSE
	if( typeIsPtr(displacement->dtype) ) then return FALSE
	if( typeGetClass(displacement->dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
	'' An unsigned value above LONGINT's range must not become negative.
	if( (typeIsSigned(displacement->dtype) = FALSE) andalso _
	    (displacement->val.i < 0) ) then return FALSE
	var pointee = typeDeref(dtype)
	if( typeIsPtr(pointee) = FALSE ) then
		select case typeGetDtOnly(pointee)
		case FB_DATATYPE_VOID, FB_DATATYPE_FUNCTION, FB_DATATYPE_FWDREF
			return FALSE
		case FB_DATATYPE_STRUCT
			if( subtype = NULL ) then return FALSE
		end select
	end if
	var elementsize = symbCalcLen(pointee, subtype)
	if( elementsize <= 0 ) then return FALSE
	if( displacement->val.i mod elementsize <> 0 ) then return FALSE
	'' IR offsets count bytes. C offsets count elements of the actual type.
	'' Divide only when exact, without negation or multiplication overflow.
	displacement->val.i \= elementsize
	return TRUE
end function

'' end of backend/c/c-pointer-arithmetic.bi
