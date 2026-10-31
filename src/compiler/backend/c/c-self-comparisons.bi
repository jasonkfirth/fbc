/'
	Project: FreeBASIC C backend
	File: backend/c/c-self-comparisons.bi
	Purpose: Prove comparisons of repeatable scalar values and owned addresses.
	Responsibilities: Recognize identical operands without dropping calls,
	indirect reads, floating-point comparisons or opaque procedure observations.
	This file intentionally does NOT contain: Tree rewriting, storage pruning,
	value propagation between statements or native diagnostic suppression.
'/

declare function hCSelfRepeatable(byval n as ASTNODE ptr) as integer

private function hCSelfAddress(byval n as ASTNODE ptr) as integer
	if( n = NULL orelse n->vector <> 0 orelse n->r <> NULL ) then return FALSE
	select case as const n->class
	case AST_NODECLASS_VAR
		if( n->l <> NULL ) then return FALSE
		return hCAddressOwnedObject(n->sym)
	case AST_NODECLASS_FIELD
		return hCSelfAddress(n->l)
	case AST_NODECLASS_DEREF
		'' ADDROF cancels this dereference. Only its pointer computation is
		'' evaluated; the addressed memory is not read by the comparison.
		return hCSelfRepeatable(n->l)
	end select
	return FALSE
end function

private function hCSelfRepeatable(byval n as ASTNODE ptr) as integer
	if( n = NULL orelse n->vector <> 0 ) then return FALSE
	if( typeGetClass(n->dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
	select case as const n->class
	case AST_NODECLASS_CONST
		'' astIsTreeEqual compares the scalar payload. Keep wide constants
		'' outside this proof until equality checks both 128-bit halves.
		return typeGetSize(n->dtype) <= 8
	case AST_NODECLASS_VAR
		var sym = n->sym
		if( sym = NULL orelse symbIsVar(sym) = FALSE ) then return FALSE
		if( symbIsRef(sym) or symbIsParamVarBydescOrByref(sym) ) then return FALSE
		if( symbIsThreadLocal(sym) or symbIsThreadLocalC(sym) ) then return FALSE
		if( n->l <> NULL orelse n->r <> NULL orelse n->var_.ofs <> 0 ) then return FALSE
		if( symbGetArrayDimensions(sym) <> 0 ) then return FALSE
		return typeGetDtAndPtrOnly(n->dtype) = typeGetDtAndPtrOnly(symbGetFullType(sym))
	case AST_NODECLASS_CONV
		if( n->r <> NULL ) then return FALSE
		return hCSelfRepeatable(n->l)
	case AST_NODECLASS_ADDROF, AST_NODECLASS_OFFSET
		if( n->r <> NULL ) then return FALSE
		return hCSelfAddress(n->l)
	case AST_NODECLASS_UOP
		if( n->r <> NULL ) then return FALSE
		if( n->op.op <> AST_OP_NEG and n->op.op <> AST_OP_NOT ) then return FALSE
		return hCSelfRepeatable(n->l)
	case AST_NODECLASS_BOP
		'' Integer arithmetic and bit operations have no independent
		'' observations. Division and other potentially trapping operations
		'' retain their evaluation even when both operand trees match.
		select case as const n->op.op
		case AST_OP_ADD, AST_OP_SUB, AST_OP_MUL, AST_OP_AND, AST_OP_OR, _
		     AST_OP_XOR, AST_OP_SHL, AST_OP_SHR
			return hCSelfRepeatable(n->l) andalso hCSelfRepeatable(n->r)
		end select
	end select
	return FALSE
end function

private function hCSelfTruth(byval n as ASTNODE ptr, byref truth as integer) as integer
	var l = n->l
	var r = n->r
	if( l = NULL orelse r = NULL ) then return FALSE
	if( typeGetClass(l->dtype) <> FB_DATACLASS_INTEGER orelse _
	    typeGetClass(r->dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
	'' No-conversion casts can distinguish BASIC aliases with the same C
	'' representation. Both operands must still use the same value domain;
	'' signed/unsigned reinterpretations need their usual comparison rules.
	if( typeIsPtr(l->dtype) <> typeIsPtr(r->dtype) ) then return FALSE
	if( typeGetSize(l->dtype) <> typeGetSize(r->dtype) ) then return FALSE
	if( typeIsPtr(l->dtype) = FALSE ) then
		if( typeIsSigned(l->dtype) <> typeIsSigned(r->dtype) ) then return FALSE
	end if
	l = astSkipNoConvCAST(l)
	r = astSkipNoConvCAST(r)
	if( hCSelfRepeatable(l) = FALSE orelse hCSelfRepeatable(r) = FALSE ) then return FALSE
	if( astIsTreeEqual(l, r) = FALSE ) then return FALSE
	select case as const n->op.op
	case AST_OP_EQ, AST_OP_LE, AST_OP_GE
		truth = TRUE
	case AST_OP_NE, AST_OP_LT, AST_OP_GT
		truth = FALSE
	case else
		return FALSE
	end select
	if( n->op.options and AST_OPOPT_DOINVERSE ) then truth = not truth
	return TRUE
end function

/' end of backend/c/c-self-comparisons.bi '/
