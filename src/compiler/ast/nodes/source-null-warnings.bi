/'
	Project: FreeBASIC source diagnostics
	File: ast/nodes/source-null-warnings.bi
	Purpose: Warn when an evaluated memory access has a provably null address.
	Responsibilities: Inspect immediate straight-line pointer assignments, stop at
	unknown effects or control flow, and distinguish memory access from address use.
	This file intentionally does NOT execute expressions or perform alias analysis.
'/

private function hSourceNullConstant(byval n as ASTNODE ptr) as integer
	while( n <> NULL )
		select case as const n->class
		case AST_NODECLASS_CONV, AST_NODECLASS_PTRCHK
			n = n->l
		case AST_NODECLASS_CONST
			return (typeGetClass(astGetDataType(n)) = FB_DATACLASS_INTEGER) andalso (n->val.i = 0)
		case else
			return FALSE
		end select
	wend
	return FALSE
end function

'' Reverse evaluation of a preceding statement: 1 proves zero, -1 is a
'' barrier, 0 permits looking earlier. The bound in the caller keeps this a
'' small local check, independent of procedure size and optimization level.
private function hSourceNullStore(byval n as ASTNODE ptr, byval sym as FBSYMBOL ptr) as integer
	if( n = NULL ) then return 0
	select case as const n->class
	case AST_NODECLASS_LINK
		var result = hSourceNullStore(n->r, sym)
		if( result <> 0 ) then return result
		return hSourceNullStore(n->l, sym)
	case AST_NODECLASS_NOP, AST_NODECLASS_DBG
		return 0
	case AST_NODECLASS_DECL
		if( n->sym <> sym ) then return 0
		if( n->l <> NULL andalso symbGetDontInit(sym) = FALSE ) then return 1
		return -1
	case AST_NODECLASS_ASSIGN
		if( n->l = NULL orelse n->l->class <> AST_NODECLASS_VAR ) then return -1
		if( n->l->var_.ofs <> 0 ) then return -1
		if( n->l->sym = sym ) then return iif(hSourceNullConstant(n->r), 1, -1)
		'' Even a side-effect-free RHS could publish the pointer's address to
		'' an existing alias. Only unrelated constant assignments are transparent.
		if( n->r <> NULL andalso n->r->class = AST_NODECLASS_CONST ) then return 0
	end select
	return -1
end function

private function hSourceKnownNull(byval n as ASTNODE ptr) as integer
	if( hSourceNullConstant(n) ) then return TRUE
	while( n <> NULL )
		select case as const n->class
		case AST_NODECLASS_CONV, AST_NODECLASS_PTRCHK
			n = n->l
		case else
			exit while
		end select
	wend
	if( n = NULL orelse n->class <> AST_NODECLASS_VAR ) then return FALSE
	if( n->var_.ofs <> 0 ) then return FALSE
	var sym = n->sym
	if( sym = NULL orelse symbIsVar(sym) = FALSE ) then return FALSE
	if( symbIsLocal(sym) = FALSE or symbIsStatic(sym) or symbIsParamVar(sym) or symbIsRef(sym) ) then return FALSE
	if( symbIsShared(sym) or symbIsTemp(sym) ) then return FALSE
	var previous = ast.proc.curr->r
	for count as integer = 1 to 64
		if( previous = NULL ) then exit for
		var result = hSourceNullStore(previous, sym)
		if( result <> 0 ) then return (result = 1)
		previous = previous->prev
	next
	return FALSE
end function

private function hSourceNullUnknownEffects(byval n as ASTNODE ptr) as integer
	if( n = NULL ) then return FALSE
	select case as const n->class
	case AST_NODECLASS_CALL, AST_NODECLASS_ASM, _
	     AST_NODECLASS_IIF, AST_NODECLASS_BRANCH, AST_NODECLASS_JMPTB
		return TRUE
	case AST_NODECLASS_LINK
		'' An initializer commonly links an empty declaration to its store.
		'' It has no earlier evaluation which could change the pointer fact.
		'' Other linked statements require their own flow analysis.
		if( n->l <> NULL andalso n->l->class = AST_NODECLASS_DECL andalso n->l->l = NULL ) then
			return hSourceNullUnknownEffects(n->r)
		end if
		return TRUE
	case AST_NODECLASS_PTRCHK
		'' Its runtime check does not change the pointer being inspected.
		return hSourceNullUnknownEffects(n->l)
	end select
	return hSourceNullUnknownEffects(n->l) orelse hSourceNullUnknownEffects(n->r)
end function

private sub hWarnSourceNullAccess(byval n as ASTNODE ptr, byval address_only as integer = FALSE)
	if( n = NULL ) then exit sub
	select case as const n->class
	case AST_NODECLASS_ADDROF
		hWarnSourceNullAccess(n->l, TRUE)
		return
	case AST_NODECLASS_DEREF
		if( address_only = FALSE and n->ptr.ofs = 0 ) then
			if( n->l = NULL orelse hSourceKnownNull(n->l) ) then errReportWarn(FB_WARNINGMSG_NULLACCESS)
		end if
		'' Computing the outer address can still load an inner pointer.
		hWarnSourceNullAccess(n->l)
		return
	case AST_NODECLASS_FIELD
		hWarnSourceNullAccess(n->l, address_only)
		return
	case AST_NODECLASS_DECL, AST_NODECLASS_ASM
		return
	end select
	hWarnSourceNullAccess(n->l)
	hWarnSourceNullAccess(n->r)
end sub

'' end of ast/nodes/source-null-warnings.bi
