/'
	Project: FreeBASIC C backend
	File: backend/c/c-literal-offsets.bi
	Purpose: Retain bounded literal suffix choices in emitted C expressions.
	Responsibilities: Prove a zero-or-one byte offset and preserve its evaluation.
	This file intentionally does NOT contain general pointer arithmetic or mutable-storage lowering.
'/

private function hCLiteralOffsetRange _
	( byval value as EXPRNODE ptr, byval depth as integer = 0 ) as integer

	if( value = NULL or depth >= 64 ) then return 0
	if( typeIsPtr(value->dtype) or typeGetClass(value->dtype) <> FB_DATACLASS_INTEGER ) then return 0

	'' A positive result denotes {0, 1}; a negative result denotes {0, -1}.
	'' Every accepted cast preserves that set. In particular, an unsigned
	'' cast of -1 changes the range and cannot pass through this proof.
	select case as const value->class
	case EXPRCLASS_IMM
		if( value->val.i = 0 or value->val.i = 1 ) then return 1
	case EXPRCLASS_CAST
		var child_range = hCLiteralOffsetRange(value->l, depth + 1)
		if( child_range < 0 andalso typeIsSigned(value->dtype) = FALSE ) then return 0
		return child_range
	case EXPRCLASS_UOP
		if( value->op <> AST_OP_NEG or typeIsSigned(value->dtype) = FALSE ) then return 0
		if( value->l = NULL orelse typeIsSigned(value->l->dtype) = FALSE ) then return 0
		return -hCLiteralOffsetRange(value->l, depth + 1)
	case EXPRCLASS_BOP
		if( astOpIsRelational(value->op) = FALSE ) then return 0
		if( value->l = NULL or value->r = NULL ) then return 0
		if( typeGetClass(value->l->dtype) <> FB_DATACLASS_INTEGER or _
		    typeGetClass(value->r->dtype) <> FB_DATACLASS_INTEGER ) then return 0
		'' C comparisons produce zero or one. BASIC's negative true value
		'' is represented by the enclosing negation in this expression tree.
		return 1
	end select
	return 0
end function

private function hCLiteralOffsetOrigin(byval value as EXPRNODE ptr) as FBSYMBOL ptr
	if( value = NULL orelse value->class <> EXPRCLASS_CAST ) then return NULL
	if( typeIsPtr(value->dtype) or typeGetClass(value->dtype) <> FB_DATACLASS_INTEGER ) then return NULL
	if( typeGetSize(value->dtype) <> env.pointersize or typeIsSigned(value->dtype) ) then return NULL
	var origin = value->l
	if( origin = NULL ) then return NULL
	for depth as integer = 0 to 63
		if( origin->class <> EXPRCLASS_CAST ) then return hCLiteralAddressSymbol(origin)
		if( typeIsPtr(origin->dtype) = FALSE or typeIsConstAt(origin->dtype, 1) = FALSE ) then return NULL
		origin = origin->l
		if( origin = NULL ) then return NULL
	next
	return NULL
end function

private function hCLiteralOffsetChoice _
	( byval value as EXPRNODE ptr, byref offset_value as EXPRNODE ptr ) as FBSYMBOL ptr

	if( typeGetPtrCnt(value->dtype) <> 1 or typeIsConstAt(value->dtype, 1) = FALSE ) then return NULL
	select case as const typeGetDtOnly(value->dtype)
	case FB_DATATYPE_CHAR, FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE
	case else
		return NULL
	end select
	var address = value->l
	if( address = NULL orelse address->class <> EXPRCLASS_BOP ) then return NULL
	if( address->op <> AST_OP_ADD or typeIsPtr(address->dtype) ) then return NULL
	if( typeGetClass(address->dtype) <> FB_DATACLASS_INTEGER ) then return NULL
	if( typeGetSize(address->dtype) <> env.pointersize or typeIsSigned(address->dtype) ) then return NULL

	var literal = hCLiteralOffsetOrigin(address->l)
	var candidate_offset = address->r
	if( literal = NULL ) then
		literal = hCLiteralOffsetOrigin(address->r)
		candidate_offset = address->l
	end if
	if( literal = NULL ) then return NULL
	'' The suffix at byte one must still point into the same character array,
	'' including its terminator. Empty literals keep their original lowering.
	if( symbGetStrLength(literal) < 1 ) then return NULL
	if( hCLiteralOffsetRange(candidate_offset) <> 1 ) then return NULL
	offset_value = candidate_offset
	return literal
end function

/' end of backend/c/c-literal-offsets.bi '/
