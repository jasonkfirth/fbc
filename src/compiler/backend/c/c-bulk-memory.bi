/'
	Project: FreeBASIC C backend
	File: backend/c/c-bulk-memory.bi
	Purpose: Keep bulk byte operations within their containing record object.
	Responsibilities: Validate constant spans before replacing a recovered field
	address with a byte offset from its recorded aggregate origin.
	This file intentionally does NOT contain: Size changes, unknown-range proofs,
	initialization evaluation or ownership changes.
'/

private function hCBulkMemoryAddress(byval value as EXPRNODE ptr, byval bytes as longint) as EXPRNODE ptr
	if( bytes <= 0 ) then return value
	var address = value
	while( address->class = EXPRCLASS_CAST )
		if( typeIsPtr(address->dtype) = FALSE orelse typeIsPtr(address->l->dtype) = FALSE ) then return value
		address = address->l
	wend
	if( address->class <> EXPRCLASS_UOP orelse address->op <> AST_OP_ADDROF ) then return value
	var member = address->l
	if( member->class <> EXPRCLASS_MEMBER orelse member->r = NULL ) then return value
	if( member->r->class <> EXPRCLASS_IMM ) then return value
	var offset = member->r->val.i
	if( offset < 0 ) then return value
	'' A scalar member is already a suitable owner for a copy that fits it.
	'' Larger field groups need the enclosing object, not pointer arithmetic
	'' beyond that first scalar's C subobject bounds.
	if( bytes <= symbCalcLen(member->dtype, member->subtype) ) then return value
	var origin = member->l
	var dtype = origin->dtype
	if( typeIsPtr(dtype) ) then
		if( typeGetPtrCnt(dtype) <> 1 ) then return value
		dtype = typeDeref(dtype)
	end if
	select case typeGetDtOnly(dtype)
	case FB_DATATYPE_STRUCT
		if( origin->subtype = NULL ) then return value
	case FB_DATATYPE_STRING, FB_DATATYPE_USTRING
	case else
		return value
	end select
	var extent = symbCalcLen(dtype, origin->subtype)
	'' Subtraction avoids overflowing offset + bytes for malformed IR ranges.
	if( extent <= 0 orelse offset > extent orelse bytes > extent - offset ) then return value
	'' MEMBER owns its origin. Detach it before releasing the old field path;
	'' the replacement must evaluate that same origin exactly once.
	member->l = NULL
	exprFreeTree(value)
	if( typeIsPtr(origin->dtype) = FALSE ) then origin = exprNewCAddress(origin)
	origin = exprNewCAST(hCBytePointerType(origin->dtype), NULL, origin)
	if( offset > 0 ) then origin = exprNewBOP(AST_OP_ADD, origin, exprNewIMMi(offset))
	return origin
end function

/' end of backend/c/c-bulk-memory.bi '/
