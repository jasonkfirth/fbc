/'
	Project: FreeBASIC C backend
	File: backend/c/c-readonly-locals.bi
	Purpose: Retain pointed-to const on private pointer bindings used as values.
	Responsibilities: Inspect the complete optimized body before loading deletes
	AST nodes, and keep escaped bindings and pointee accesses unchanged.
	This file intentionally does NOT contain: BASIC type changes, ABI changes,
	pointee immutability assumptions or native diagnostic suppression.
'/

type CREADONLYLOCAL
	sym as FBSYMBOL ptr
	identifier as zstring ptr
	readonly_value as integer
end type

'' Entries retain symbol addresses and own their copied C identities. Loading
'' can recycle temporary symbol slots, so both must match during emission.
'' Qualification permits other aliases to write; it does not imply restrict.
dim shared as TLIST c_readonly_locals
dim shared as integer c_readonly_locals_ready

private sub hCReadOnlyLocalsClear()
	if( c_readonly_locals_ready = FALSE ) then exit sub
	do
		var entry = cptr(CREADONLYLOCAL ptr, listGetHead(@c_readonly_locals))
		if( entry = NULL ) then exit do
		ZstrFree(entry->identifier)
		listDelNode(@c_readonly_locals, entry)
	loop
end sub

private sub hCReadOnlyLocalsFinish()
	if( c_readonly_locals_ready = FALSE ) then exit sub
	hCReadOnlyLocalsClear()
	listEnd(@c_readonly_locals)
	c_readonly_locals_ready = FALSE
end sub

private sub hCReadOnlyLocalsInit()
	hCReadOnlyLocalsFinish()
	listInit(@c_readonly_locals, 16, sizeof(CREADONLYLOCAL), LIST_FLAGS_CLEAR)
	c_readonly_locals_ready = TRUE
end sub

private function hCReadOnlyLocalFind(byval sym as FBSYMBOL ptr) as CREADONLYLOCAL ptr
	if( sym = NULL or c_readonly_locals_ready = FALSE ) then return NULL
	var entry = cptr(CREADONLYLOCAL ptr, listGetHead(@c_readonly_locals))
	while( entry <> NULL )
		if( entry->sym = sym andalso *entry->identifier = hGetCId(sym) ) then return entry
		entry = listGetNext(entry)
	wend
	return NULL
end function

private function hCReadOnlyLocalCandidate(byval sym as FBSYMBOL ptr) as integer
	if( sym = NULL orelse symbIsVar(sym) = FALSE ) then return FALSE
	if( symbIsLocal(sym) = FALSE or symbIsParamVar(sym) or symbIsRef(sym) ) then return FALSE
	if( (sym->attrib and (FB_SYMBATTRIB_STATIC or FB_SYMBATTRIB_SHARED or _
	    FB_SYMBATTRIB_PUBLIC or FB_SYMBATTRIB_COMMON or FB_SYMBATTRIB_EXTERN or _
	    FB_SYMBATTRIB_IMPORT or FB_SYMBATTRIB_FUNCRESULT or FB_SYMBATTRIB_DESCRIPTOR)) <> 0 ) then return FALSE
	if( symbIsExport(sym) or symbIsThreadLocal(sym) or symbIsThreadLocalC(sym) ) then return FALSE
	if( symbGetIsTemporary(sym) or symbGetArrayDimensions(sym) <> 0 ) then return FALSE
	if( symbGetTypeIniTree(sym) <> NULL ) then return FALSE
	var dtype = symbGetFullType(sym)
	if( typeGetPtrCnt(dtype) <> 1 or typeIsConstAt(dtype, 1) ) then return FALSE
	select case as const typeGetDtOnly(dtype)
	case FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR, FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE
		return TRUE
	end select
	return FALSE
end function

private sub hCReadOnlyLocalCollect(byval n as ASTNODE ptr)
	if( n = NULL ) then exit sub
	if( hCReadOnlyLocalCandidate(n->sym) andalso hCReadOnlyLocalFind(n->sym) = NULL ) then
		var entry = cptr(CREADONLYLOCAL ptr, listNewNode(@c_readonly_locals))
		entry->sym = n->sym
		var identifier = hGetCId(n->sym)
		entry->identifier = ZstrDup(strptr(identifier))
	end if
	hCReadOnlyLocalCollect(n->l)
	hCReadOnlyLocalCollect(n->r)
end sub

private function hCReadOnlyLocalContains(byval n as ASTNODE ptr, byval sym as FBSYMBOL ptr) as integer
	if( n = NULL ) then return FALSE
	if( n->sym = sym ) then return TRUE
	if( hCReadOnlyLocalContains(n->l, sym) or hCReadOnlyLocalContains(n->r, sym) ) then return TRUE
	if( n->class = AST_NODECLASS_CALL ) then
		if( n->call.tmpres = sym ) then return TRUE
		var item = n->call.strtail
		while( item <> NULL )
			if( item->sym = sym or hCReadOnlyLocalContains(item->srctree, sym) ) then return TRUE
			item = item->prev
		wend
	end if
	return FALSE
end function

private function hCReadOnlyLocalCheck(byval n as ASTNODE ptr, byval sym as FBSYMBOL ptr, _
	byval value_read as integer = FALSE) as integer
	if( n = NULL ) then return TRUE
	select case as const n->class
	case AST_NODECLASS_ASM, AST_NODECLASS_MACRO
		return FALSE
	case AST_NODECLASS_VAR
		if( n->sym = sym ) then return value_read
	case AST_NODECLASS_ARG
		'' BYREF may expose a pointer slot without an explicit ADDROF node.
		'' The loader can also materialize that slot for a converted actual.
		var byref_argument = n->arg.mode = FB_PARAMMODE_BYREF
		if( n->sym <> NULL andalso n->sym->class = FB_SYMBCLASS_PARAM ) then
			byref_argument or= symbGetParamMode(n->sym) = FB_PARAMMODE_BYREF
		end if
		if( byref_argument andalso hCReadOnlyLocalContains(n->l, sym) ) then return FALSE
	case AST_NODECLASS_ASSIGN
		if( n->l <> NULL andalso n->l->class = AST_NODECLASS_VAR andalso n->l->sym = sym ) then
			if( n->l->var_.ofs <> 0 ) then return FALSE
			if( typeGetDtAndPtrOnly(n->l->dtype) <> typeGetDtAndPtrOnly(symbGetFullType(sym)) ) then return FALSE
			return hCReadOnlyLocalCheck(n->r, sym, TRUE)
		end if
	case AST_NODECLASS_MEM
		'' A default clear initializes the pointer slot, not its pointee.
		if( n->mem.op = AST_OP_MEMFILL andalso n->l <> NULL andalso _
		    n->l->class = AST_NODECLASS_ADDROF andalso n->l->l <> NULL andalso _
		    n->l->l->class = AST_NODECLASS_VAR andalso n->l->l->sym = sym andalso _
		    n->l->l->var_.ofs = 0 andalso n->r <> NULL andalso _
		    n->r->class = AST_NODECLASS_CONST ) then
			return astConstGetInt(n->r) = env.pointersize
		end if
	case AST_NODECLASS_BOP
		if( astOpIsRelational(n->op.op) ) then
			return hCReadOnlyLocalCheck(n->l, sym, TRUE) andalso hCReadOnlyLocalCheck(n->r, sym, TRUE)
		end if
	case AST_NODECLASS_CONV
		if( typeGetClass(n->dtype) = FB_DATACLASS_INTEGER andalso typeIsPtr(n->dtype) = FALSE ) then
			return hCReadOnlyLocalCheck(n->l, sym, TRUE)
		end if
	case AST_NODECLASS_CALL
		if( n->call.tmpres = sym ) then return FALSE
		var item = n->call.strtail
		while( item <> NULL )
			if( item->sym = sym orelse hCReadOnlyLocalCheck(item->srctree, sym) = FALSE ) then return FALSE
			item = item->prev
		wend
		if( hCReadOnlyLocalCheck(n->call.profbegin, sym) = FALSE ) then return FALSE
		if( hCReadOnlyLocalCheck(n->call.profend, sym) = FALSE ) then return FALSE
	end select
	if( n->sym = sym andalso n->class <> AST_NODECLASS_DECL ) then return FALSE
	return hCReadOnlyLocalCheck(n->l, sym) andalso hCReadOnlyLocalCheck(n->r, sym)
end function

private sub hCReadOnlyLocalsBegin(byval first as ASTNODE ptr)
	hCReadOnlyLocalsClear()
	if( c_readonly_locals_ready = FALSE or c_local_refs_complete = FALSE ) then exit sub
	var n = first
	while( n <> NULL )
		hCReadOnlyLocalCollect(n)
		n = n->next
	wend
	var entry = cptr(CREADONLYLOCAL ptr, listGetHead(@c_readonly_locals))
	while( entry <> NULL )
		entry->readonly_value = TRUE
		n = first
		while( n <> NULL )
			if( hCReadOnlyLocalCheck(n, entry->sym) = FALSE ) then
				entry->readonly_value = FALSE
				exit while
			end if
			n = n->next
		wend
		entry = listGetNext(entry)
	wend
end sub

private function hCReadOnlyLocalType(byval sym as FBSYMBOL ptr, byval dtype as integer) as integer
	var entry = hCReadOnlyLocalFind(sym)
	if( entry <> NULL andalso entry->readonly_value andalso typeGetPtrCnt(dtype) = 1 ) then
		return typeAddrOf(typeSetIsConst(typeDeref(dtype)))
	end if
	return dtype
end function

/' end of backend/c/c-readonly-locals.bi '/
