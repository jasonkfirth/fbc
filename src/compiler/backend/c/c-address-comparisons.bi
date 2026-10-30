/'
	Project: FreeBASIC C backend
	File: backend/c/c-address-comparisons.bi
	Purpose: Prove equality with null for directly addressed owned objects.
	Responsibilities: Recognize pure object addresses and preserve unknown pointer
	values, reference parameters, imports and evaluated address computations.
	This file intentionally does NOT contain: Tree rewriting, pointer arithmetic,
	storage pruning or assumptions about the values stored in an object.
'/

private function hCAddressOwnedObject(byval sym as FBSYMBOL ptr) as integer
	if( sym = NULL orelse symbIsVar(sym) = FALSE ) then return FALSE
	if( symbIsRef(sym) or symbIsParamVar(sym) or symbIsDynamic(sym) ) then return FALSE
	if( (sym->attrib and (FB_SYMBATTRIB_EXTERN or FB_SYMBATTRIB_IMPORT or _
	    FB_SYMBATTRIB_COMMON or FB_SYMBATTRIB_FUNCRESULT)) <> 0 ) then return FALSE
	if( symbIsThreadLocal(sym) or symbIsThreadLocalC(sym) ) then return FALSE
	return symbGetRealSize(sym) > 0
end function

private function hCAddressNonNull(byval n as ASTNODE ptr) as integer
	if( n = NULL orelse n->vector <> 0 orelse typeIsPtr(n->dtype) = FALSE ) then return FALSE
	if( n->class = AST_NODECLASS_CONV ) then
		if( n->r <> NULL orelse n->l = NULL ) then return FALSE
		if( typeIsPtr(n->l->dtype) = FALSE ) then return FALSE
		return hCAddressNonNull(n->l)
	end if
	select case as const n->class
	case AST_NODECLASS_OFFSET
		if( n->ofs.ofs <> 0 ) then return FALSE
	case AST_NODECLASS_ADDROF
	case else
		return FALSE
	end select
	if( n->r <> NULL orelse n->l = NULL ) then return FALSE
	var operand = n->l
	'' OFFSET retains its original operand for later address optimizations.
	'' Require the simple VAR form too: an index or dereference may still
	'' evaluate a call, load an offset, or name storage through a null base.
	if( operand->class <> AST_NODECLASS_VAR or operand->vector <> 0 ) then return FALSE
	if( operand->l <> NULL or operand->r <> NULL or operand->var_.ofs <> 0 ) then return FALSE
	if( n->class = AST_NODECLASS_OFFSET andalso n->sym <> operand->sym ) then return FALSE
	return hCAddressOwnedObject(operand->sym)
end function

private function hCAddressNull(byval n as ASTNODE ptr) as integer
	if( n = NULL orelse n->vector <> 0 ) then return FALSE
	if( n->class = AST_NODECLASS_CONV ) then
		if( typeIsPtr(n->dtype) = FALSE orelse n->r <> NULL ) then return FALSE
		return hCAddressNull(n->l)
	end if
	if( n->class <> AST_NODECLASS_CONST ) then return FALSE
	if( typeGetClass(n->dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
	return astConstGetInt(n) = 0
end function

private function hCAddressTruth(byval n as ASTNODE ptr, byref truth as integer) as integer
	if( n->op.op <> AST_OP_EQ and n->op.op <> AST_OP_NE ) then return FALSE
	if( hCAddressNonNull(n->l) andalso hCAddressNull(n->r) ) then
	elseif( hCAddressNull(n->l) andalso hCAddressNonNull(n->r) ) then
	else
		return FALSE
	end if
	truth = (n->op.op = AST_OP_NE)
	if( n->op.options and AST_OPOPT_DOINVERSE ) then truth = not truth
	return TRUE
end function

/' end of backend/c/c-address-comparisons.bi '/
