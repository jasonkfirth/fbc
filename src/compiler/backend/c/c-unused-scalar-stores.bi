/'
	Project: FreeBASIC C backend
	File: backend/c/c-unused-scalar-stores.bi
	Purpose: Discard unobserved scalar stores without discarding required calls.
	Responsibilities: Prove private storage is unread, retain address escapes and
	hidden loader references, and preserve evaluation of scalar call results.
	This file intentionally does NOT contain: interprocedural analysis, string or
	UDT lifetime changes, or native diagnostic suppression.
'/

type CSCALARDEPENDENCY
	sym as FBSYMBOL ptr
	next_dependency as CSCALARDEPENDENCY ptr
end type

type CSCALARSTORE
	sym as FBSYMBOL ptr
	referenced as integer
	dependencies as CSCALARDEPENDENCY ptr
	next_pending as CSCALARSTORE ptr
end type

'' Entries borrow symbols until the current procedure has finished emitting.
'' The pool owns its entries; the hash owns only its duplicated address keys.
dim shared as TLIST c_scalar_stores, c_scalar_dependencies
dim shared as CSCALARSTORE ptr c_scalar_pending
dim shared as THASH c_scalar_store_index

private sub hCScalarStoresClear()
	if( c_scalar_store_index.list <> NULL ) then hashEnd(@c_scalar_store_index)
	c_scalar_pending = NULL
	do
		var dependency = listGetHead(@c_scalar_dependencies)
		if( dependency = NULL ) then exit do
		listDelNode(@c_scalar_dependencies, dependency)
	loop
	do
		var entry = listGetHead(@c_scalar_stores)
		if( entry = NULL ) then exit do
		listDelNode(@c_scalar_stores, entry)
	loop
end sub

private function hCScalarStoreGet(byval sym as FBSYMBOL ptr) as CSCALARSTORE ptr
	if( sym = NULL orelse c_scalar_store_index.list = NULL ) then return NULL
	var key = str(culngint(sym))
	return hashLookup(@c_scalar_store_index, strptr(key))
end function

private function hCScalarStoreType(byval dtype as integer) as integer
	'' Character strings and vector storage share integer-class lowering but
	'' do not have the scalar evaluation/lifetime contract used by this pass.
	select case as const typeGetDtAndPtrOnly(dtype)
	case FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR, FB_DATATYPE_XMMWORD
		return FALSE
	end select
	select case as const typeGetClass(dtype)
	case FB_DATACLASS_INTEGER, FB_DATACLASS_FPOINT
		return TRUE
	end select
	return FALSE
end function

private function hCScalarStoreHasLifetime(byval sym as FBSYMBOL ptr) as integer
	if( symbHasCtor(sym) or symbHasDtor(sym) ) then return TRUE
	'' Match the expression-temporary ownership handled by astDtorListRemoveRef.
	'' A dynamic array's separate scope cleanup is not owned by an IDX read.
	select case as const symbGetType(sym)
	case FB_DATATYPE_STRING, FB_DATATYPE_USTRING
		return TRUE
	case typeAddrOf(FB_DATATYPE_WCHAR)
		return symbGetIsTemporary(sym)
	end select
	return FALSE
end function

private function hCScalarStoreCandidate(byval sym as FBSYMBOL ptr) as integer
	if( sym = NULL orelse symbIsVar(sym) = FALSE ) then return FALSE
	if( symbIsLocal(sym) = FALSE orelse symbIsParamVar(sym) or symbIsRef(sym) ) then return FALSE
	if( (sym->attrib and (FB_SYMBATTRIB_STATIC or FB_SYMBATTRIB_SHARED or _
	    FB_SYMBATTRIB_COMMON or FB_SYMBATTRIB_PUBLIC or FB_SYMBATTRIB_EXTERN or _
	    FB_SYMBATTRIB_IMPORT or FB_SYMBATTRIB_FUNCRESULT or FB_SYMBATTRIB_DESCRIPTOR)) <> 0 ) then return FALSE
	if( symbGetArrayDimensions(sym) <> 0 orelse symbGetTypeIniTree(sym) <> NULL ) then return FALSE
	if( hCScalarStoreHasLifetime(sym) ) then return FALSE
	return hCScalarStoreType(symbGetFullType(sym))
end function

private sub hCScalarStoreCollect(byval n as ASTNODE ptr)
	if( n = NULL ) then exit sub
	if( hCScalarStoreCandidate(n->sym) andalso hCScalarStoreGet(n->sym) = NULL ) then
		if( c_scalar_store_index.list = NULL ) then hashInit(@c_scalar_store_index, 256, TRUE)
		var entry = cptr(CSCALARSTORE ptr, listNewNode(@c_scalar_stores))
		entry->sym = n->sym
		var key = str(culngint(n->sym))
		hashAdd(@c_scalar_store_index, ZstrDup(strptr(key)), entry, hashHash(strptr(key)))
	end if
	hCScalarStoreCollect(n->l)
	hCScalarStoreCollect(n->r)
end sub

private function hCScalarStoreDiscardable(byval n as ASTNODE ptr) as integer
	if( n = NULL orelse hCScalarStoreType(n->dtype) = FALSE ) then return FALSE
	if( n->vector <> 0 ) then return FALSE
	select case as const n->class
	case AST_NODECLASS_CONST
		return TRUE
	case AST_NODECLASS_VAR
		'' Ordinary integer loads have no volatile qualifier in this AST.
		'' Do not alter string/UDT temporary-reference lifetime accounting.
		if( typeGetClass(n->dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
		if( n->sym = NULL orelse symbIsVar(n->sym) = FALSE ) then return FALSE
		return (hCScalarStoreHasLifetime(n->sym) = FALSE)
	case AST_NODECLASS_CALL
		'' Keep the original call and status/error policy. The result alone
		'' can disappear; hidden aggregate result storage has a separate owner.
		return (n->call.tmpres = NULL)
	case AST_NODECLASS_CONV
		'' Floating conversions can affect the floating-point environment.
		if( n->r <> NULL orelse n->l = NULL ) then return FALSE
		if( typeGetClass(n->dtype) <> FB_DATACLASS_INTEGER orelse _
		    typeGetClass(n->l->dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
		return hCScalarStoreDiscardable(n->l)
	case AST_NODECLASS_DEREF, AST_NODECLASS_FIELD
		if( typeGetClass(n->dtype) <> FB_DATACLASS_INTEGER orelse n->r <> NULL ) then return FALSE
		return hCScalarStoreDiscardable(n->l)
	case AST_NODECLASS_IDX
		if( typeGetClass(n->dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
		if( n->r = NULL orelse n->r->class <> AST_NODECLASS_VAR ) then return FALSE
		if( n->r->sym = NULL ) then return FALSE
		if( hCScalarStoreHasLifetime(n->r->sym) ) then return FALSE
		return (n->l = NULL) orelse hCScalarStoreDiscardable(n->l)
	case AST_NODECLASS_UOP
		if( typeGetClass(n->dtype) <> FB_DATACLASS_INTEGER orelse n->r <> NULL ) then return FALSE
		if( n->op.ex <> NULL orelse (n->op.options and AST_OPOPT_ALLOCRES) = 0 ) then return FALSE
		if( n->l = NULL orelse typeGetClass(n->l->dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
		if( n->op.op <> AST_OP_NEG and n->op.op <> AST_OP_NOT ) then return FALSE
		return hCScalarStoreDiscardable(n->l)
	case AST_NODECLASS_BOP
		if( typeGetClass(n->dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
		if( n->l = NULL orelse n->r = NULL ) then return FALSE
		if( typeGetClass(n->l->dtype) <> FB_DATACLASS_INTEGER orelse _
		    typeGetClass(n->r->dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
		if( n->op.ex <> NULL orelse (n->op.options and AST_OPOPT_ALLOCRES) = 0 ) then return FALSE
		'' Do not discard division/remainder traps, floating operations, or
		'' branches. Wrapped integer arithmetic and comparisons add no effects.
		select case as const n->op.op
		case AST_OP_ADD, AST_OP_SUB, AST_OP_MUL, AST_OP_AND, AST_OP_OR, AST_OP_XOR, _
		     AST_OP_SHL, AST_OP_SHR, AST_OP_EQ, AST_OP_NE, AST_OP_GT, AST_OP_LT, AST_OP_GE, AST_OP_LE
			return hCScalarStoreDiscardable(n->l) andalso hCScalarStoreDiscardable(n->r)
		end select
	end select
	return FALSE
end function

private function hCScalarStoreTarget(byval n as ASTNODE ptr) as CSCALARSTORE ptr
	var target = n->l
	if( target = NULL ) then return NULL
	select case as const n->class
	case AST_NODECLASS_ASSIGN
		if( hCScalarStoreDiscardable(n->r) = FALSE ) then return NULL
	case AST_NODECLASS_MEM
		if( n->mem.op <> AST_OP_MEMFILL ) then return NULL
		if( n->r = NULL orelse n->r->class <> AST_NODECLASS_CONST ) then return NULL
		if( typeGetClass(n->r->dtype) <> FB_DATACLASS_INTEGER ) then return NULL
		if( target->class = AST_NODECLASS_ADDROF ) then target = target->l
	case else
		return NULL
	end select
	if( target = NULL orelse target->class <> AST_NODECLASS_VAR ) then return NULL
	var entry = hCScalarStoreGet(target->sym)
	if( entry = NULL orelse target->var_.ofs <> 0 ) then return NULL
	var bytes = clngint(typeGetSize(target->dtype))
	if( n->class = AST_NODECLASS_MEM ) then bytes = astConstGetInt(n->r)
	if( bytes <= 0 orelse bytes <> symbGetSizeOf(entry->sym) ) then return NULL
	return entry
end function

private sub hCScalarStoreRemember(byval sym as FBSYMBOL ptr, byval owner as CSCALARSTORE ptr = NULL)
	var entry = hCScalarStoreGet(sym)
	if( entry = NULL ) then exit sub
	if( owner <> NULL ) then
		if( owner = entry ) then exit sub
		var dependency = cptr(CSCALARDEPENDENCY ptr, listNewNode(@c_scalar_dependencies))
		dependency->sym = sym
		dependency->next_dependency = owner->dependencies
		owner->dependencies = dependency
	elseif( entry->referenced = FALSE ) then
		entry->referenced = TRUE
		entry->next_pending = c_scalar_pending
		c_scalar_pending = entry
	end if
end sub

private sub hCScalarStoreReferences(byval n as ASTNODE ptr, byval value_used as integer, byval effects_only as integer = FALSE, byval dependency_owner as CSCALARSTORE ptr = NULL)
	if( n = NULL ) then exit sub
	if( dependency_owner <> NULL ) then
		'' Calls and their arguments are already unconditional effect roots.
		'' The other reads matter only if this assignment's destination is live.
		if( n->class = AST_NODECLASS_CALL ) then exit sub
		hCScalarStoreRemember(n->sym, dependency_owner)
		hCScalarStoreReferences(n->l, TRUE, FALSE, dependency_owner)
		hCScalarStoreReferences(n->r, TRUE, FALSE, dependency_owner)
		exit sub
	end if
	if( effects_only andalso n->class <> AST_NODECLASS_CALL ) then
		'' IDX loads its index expression, but its R child is array metadata.
		'' VAR/CONST reads themselves add no effects to the discarded result.
		select case as const n->class
		case AST_NODECLASS_CONST, AST_NODECLASS_VAR
			exit sub
		case AST_NODECLASS_IDX
			hCScalarStoreReferences(n->l, TRUE, TRUE)
			exit sub
		end select
		hCScalarStoreReferences(n->l, TRUE, TRUE)
		hCScalarStoreReferences(n->r, TRUE, TRUE)
		exit sub
	end if
	if( n->class = AST_NODECLASS_LINK ) then
		hCScalarStoreReferences(n->l, value_used and (n->link.ret = AST_LINK_RETURN_LEFT))
		hCScalarStoreReferences(n->r, value_used and (n->link.ret = AST_LINK_RETURN_RIGHT))
		exit sub
	end if
	if( n->class = AST_NODECLASS_DECL ) then
		hCScalarStoreReferences(n->l, FALSE)
		hCScalarStoreReferences(n->r, FALSE)
		exit sub
	end if
	var entry = hCScalarStoreTarget(n)
	if( entry <> NULL ) then
		if( value_used ) then
			'' ADDROF(ASSIGN(temp, value)) supplies BYREF argument storage.
			'' Its result is observable even without a separate VAR read.
			hCScalarStoreRemember(entry->sym)
		else
			'' Preserve arguments and hidden loader data of retained calls.
			hCScalarStoreReferences(n->r, TRUE, TRUE)
			hCScalarStoreReferences(n->r, TRUE, FALSE, entry)
			exit sub
		end if
	end if
	hCScalarStoreRemember(n->sym)
	if( n->class = AST_NODECLASS_CALL ) then
		hCScalarStoreRemember(n->call.tmpres)
		var item = n->call.strtail
		while( item <> NULL )
			hCScalarStoreRemember(item->sym)
			hCScalarStoreReferences(item->srctree, TRUE)
			item = item->prev
		wend
		hCScalarStoreReferences(n->call.profbegin, TRUE)
		hCScalarStoreReferences(n->call.profend, TRUE)
	end if
	hCScalarStoreReferences(n->l, TRUE)
	hCScalarStoreReferences(n->r, TRUE)
end sub

private function hCScalarStoreDiscardValue(byval n as ASTNODE ptr) as ASTNODE ptr
	if( n = NULL ) then return NULL
	select case as const n->class
	case AST_NODECLASS_CALL
		'' astIgnoreCallResult() may introduce RTL error checking. A value that
		'' was assigned but never read must retain its original error policy.
		astSetType(n, FB_DATATYPE_VOID, NULL)
		return n
	case AST_NODECLASS_CONST, AST_NODECLASS_VAR
		astDelTree(n)
		return NULL
	case AST_NODECLASS_IDX
		'' As in astLoadIDX(), the R-side VAR is metadata, not an evaluation.
		astDelTree(n->r)
		n->r = NULL
	end select
	var left_expr = n->l
	var right_expr = n->r
	n->l = NULL
	n->r = NULL
	astDelNode(n)
	'' astLoadBOP evaluates L before R. Preserve that order for every call
	'' whose enclosing integer operation no longer has a consumer.
	var left_effects = hCScalarStoreDiscardValue(left_expr)
	var right_effects = hCScalarStoreDiscardValue(right_expr)
	return astNewLINK(left_effects, right_effects, AST_LINK_RETURN_NONE)
end function

private function hCScalarStorePrune(byval n as ASTNODE ptr, byval value_used as integer) as integer
	if( n = NULL ) then return FALSE
	var entry = hCScalarStoreTarget(n)
	if( value_used = FALSE andalso entry <> NULL andalso entry->referenced = FALSE ) then
		astDelTree(n->l)
		n->l = NULL
		dim as ASTNODE ptr retained_effects = NULL
		if( n->class = AST_NODECLASS_ASSIGN ) then
			retained_effects = hCScalarStoreDiscardValue(n->r)
		else
			astDelTree(n->r)
		end if
		n->r = NULL
		n->dtype = FB_DATATYPE_VOID
		n->subtype = NULL
		n->sym = NULL
		if( retained_effects <> NULL ) then
			'' Keep the statement node's list links and source position.
			n->class = AST_NODECLASS_LINK
			n->link.ret = AST_LINK_RETURN_NONE
			n->l = retained_effects
		else
			n->class = AST_NODECLASS_NOP
		end if
		return TRUE
	end if
	if( n->class = AST_NODECLASS_LINK ) then
		var changed = hCScalarStorePrune(n->l, value_used and (n->link.ret = AST_LINK_RETURN_LEFT))
		return hCScalarStorePrune(n->r, value_used and (n->link.ret = AST_LINK_RETURN_RIGHT)) or changed
	end if
	if( n->class = AST_NODECLASS_DECL ) then
		var changed = hCScalarStorePrune(n->l, FALSE)
		return hCScalarStorePrune(n->r, FALSE) or changed
	end if
	var changed = hCScalarStorePrune(n->l, TRUE)
	return hCScalarStorePrune(n->r, TRUE) or changed
end function

private function hCScalarStoresBegin(byval first as ASTNODE ptr) as integer
	hCScalarStoresClear()
	if( c_local_refs_complete = FALSE ) then return FALSE
	var n = first
	while( n <> NULL )
		hCScalarStoreCollect(n)
		n = n->next
	wend
	if( c_scalar_store_index.list = NULL ) then return FALSE
	n = first
	while( n <> NULL )
		hCScalarStoreReferences(n, FALSE)
		n = n->next
	wend
	'' Close liveness over assignment dependencies before deleting any store.
	'' Each entry is queued at most once, so long chains do not require
	'' repeatedly rescanning the whole procedure.
	while( c_scalar_pending <> NULL )
		var entry = c_scalar_pending
		c_scalar_pending = entry->next_pending
		var dependency = entry->dependencies
		while( dependency <> NULL )
			hCScalarStoreRemember(dependency->sym)
			dependency = dependency->next_dependency
		wend
	wend
	dim as integer changed = FALSE
	n = first
	while( n <> NULL )
		changed or= hCScalarStorePrune(n, FALSE)
		n = n->next
	wend
	return changed
end function

'' end of backend/c/c-unused-scalar-stores.bi
