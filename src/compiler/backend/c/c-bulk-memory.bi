/'
	Project: FreeBASIC C backend
	File: backend/c/c-bulk-memory.bi
	Purpose: Keep bulk byte operations within their containing record object.
	Responsibilities: Validate constant spans and retain aggregate origins at
	native byte-memory calls with the matching libc declarations.
	This file intentionally does NOT contain: Size changes, unknown-range proofs,
	initialization evaluation or ownership changes.
'/

private function hCByteMemoryArgument(byval argument as FBSYMBOL ptr) as integer
	if( argument = NULL orelse argument->class <> FB_SYMBCLASS_PARAM ) then return FALSE
	var proc = symbGetParent(argument)
	if( symbGetProcMode(proc) <> FB_FUNCMODE_CDECL ) then return FALSE
	if( symbGetIsParsed(proc) or symbGetIsFuncPtr(proc) ) then return FALSE
	if( symbGetProcParams(proc) <> 3 ) then return FALSE
	var dtype = symbGetProcRealType(proc)
	if( typeGetDtAndPtrOnly(dtype) <> typeAddrOf(FB_DATATYPE_VOID) ) then return FALSE
	var identifier = symbGetMangledName(proc)
	if( identifier = NULL ) then return FALSE
	dim as integer fill
	select case *identifier
	case "memset"
		fill = TRUE
	case "memcpy", "memmove"
		fill = FALSE
	case else
		return FALSE
	end select
	var param = symbGetProcHeadParam(proc)
	dim as integer byte_argument
	for i as integer = 0 to 2
		if( param = NULL orelse symbGetParamMode(param) <> FB_PARAMMODE_BYVAL ) then return FALSE
		dtype = symbGetFullType(param)
		if( i = 2 ) then
			if( typeIsPtr(dtype) or typeGetClass(dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
			if( typeGetSize(dtype) <> env.pointersize or typeIsSigned(dtype) ) then return FALSE
		elseif( i = 1 and fill ) then
			if( typeIsPtr(dtype) or typeGetClass(dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
			if( typeGetSize(dtype) <> 4 or typeIsSigned(dtype) = FALSE ) then return FALSE
		else
			if( typeGetDtAndPtrOnly(dtype) <> typeAddrOf(FB_DATATYPE_VOID) ) then return FALSE
			if( param = argument ) then byte_argument = TRUE
		end if
		param = symbGetNext(param)
	next
	return byte_argument
end function

private function hCBulkMemoryAddress(byval value as EXPRNODE ptr, byval bytes as longint, _
	byval byte_argument as integer = FALSE, byval read_only as integer = FALSE) as EXPRNODE ptr
	if( bytes <= 0 and byte_argument = FALSE ) then return value
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
	if( byte_argument = FALSE andalso bytes <= symbCalcLen(member->dtype, member->subtype) ) then return value
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
	if( extent <= 0 orelse offset >= extent orelse bytes > extent - offset ) then return value
	'' A native byte-memory call requires an address within actual storage.
	'' Retain its enclosing C object when the call spans a field group.
	'' An unknown argument span gains no range proof here. Native diagnostics
	'' can still check the offset and count against the enclosing object's size.
	'' MEMBER owns its origin. Detach it before releasing the old field path;
	'' the replacement must evaluate that same origin exactly once.
	member->l = NULL
	exprFreeTree(value)
	if( typeIsPtr(origin->dtype) = FALSE ) then origin = exprNewCAddress(origin)
	var byte_pointer = hCBytePointerType(origin->dtype, read_only)
	if( offset = 0 ) then return exprNewCAST(byte_pointer, NULL, origin)
	'' A flexible byte tail retains the actual allocation's extent. The
	'' prefix supplies the recorded field displacement, including for larger
	'' allocations containing a record prefix. Native tools can still infer
	'' a fixed object's remaining size when the byte count becomes known.
	'' Qualifying the enclosing record carries const to every byte member.
	dim as string qualifier
	if( typeIsConstAt(byte_pointer, 1) ) then qualifier = "const "
	var byte_type = hEmitType(FB_DATATYPE_UBYTE, NULL, TRUE)
	var text = "((" + qualifier + "struct { " + byte_type + " fb_c_prefix[" + str(offset) + "]; " + _
		byte_type + " fb_c_bytes[]; }*)" + exprFlush(origin) + ")->fb_c_bytes"
	return exprNewTEXT(byte_pointer, NULL, text)
end function

/' end of backend/c/c-bulk-memory.bi '/
