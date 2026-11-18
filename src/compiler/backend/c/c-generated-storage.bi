/'
	Project: FreeBASIC C backend
	File: backend/c/c-generated-storage.bi
	Purpose: Omit unobserved private global storage from generated C.
	Responsibilities: Record C and ASM uses, close static initializer dependencies,
	and retain public, owned, thread-local and debugger-visible storage.
	This file intentionally does NOT contain: Procedure liveness, initialization
	evaluation, source access flag changes or public ABI changes.
'/

type CGLOBALSTORAGE
	'' The procedure pool owns this node and its C spelling until output closes.
	leaf as CPROCLEAF ptr
	needed as integer
	debug_retained as integer
end type

type CGLOBALINITIALIZERWORK
	node as ASTNODE ptr
end type

dim shared as TLIST c_global_storage_nodes, c_global_initializer_work
dim shared as THASH c_global_storage_names
dim shared as integer c_global_storage_ready, c_global_storage_prepared

private sub hCGlobalStorageFinish()
	if( c_global_storage_ready = FALSE ) then exit sub
	hashEnd(@c_global_storage_names)
	listEnd(@c_global_storage_nodes)
	listEnd(@c_global_initializer_work)
	c_global_storage_ready = FALSE
	c_global_storage_prepared = FALSE
end sub

private sub hCGlobalStorageInit()
	hCGlobalStorageFinish()
	listInit(@c_global_storage_nodes, 32, sizeof(CGLOBALSTORAGE), LIST_FLAGS_CLEAR)
	listInit(@c_global_initializer_work, 32, sizeof(CGLOBALINITIALIZERWORK), LIST_FLAGS_CLEAR)
	hashInit(@c_global_storage_names, 128, FALSE)
	c_global_storage_ready = TRUE
end sub

private function hCGlobalStorageCandidate(byval sym as FBSYMBOL ptr) as integer
	if( sym = NULL orelse symbIsVar(sym) = FALSE ) then return FALSE
	if( symbIsLocal(sym) or (symbIsStatic(sym) = FALSE) or symbGetIsLiteral(sym) or symbIsDynamic(sym) ) then return FALSE
	if( (sym->attrib and (FB_SYMBATTRIB_PUBLIC or FB_SYMBATTRIB_COMMON or _
	    FB_SYMBATTRIB_EXTERN or FB_SYMBATTRIB_IMPORT)) <> 0 ) then return FALSE
	if( symbIsExport(sym) or symbIsThreadLocal(sym) or symbIsThreadLocalC(sym) ) then return FALSE
	if( symbIsDataDesc(sym) ) then return FALSE
	select case as const symbGetType(sym)
	case FB_DATATYPE_STRING, FB_DATATYPE_USTRING
		'' Default string storage starts empty and owns no allocation. Actual
		'' assignment and cleanup references retain it through this index;
		'' an explicit initializer remains an ownership root.
		return (symbGetTypeIniTree(sym) = NULL)
	case typeAddrOf(FB_DATATYPE_WCHAR)
		if( symbGetIsTemporary(sym) ) then return FALSE
	end select
	if( symbHasCtor(sym) or symbHasDtor(sym) ) then return FALSE
	return TRUE
end function

private function hCGlobalStorageGet(byval sym as FBSYMBOL ptr) as CGLOBALSTORAGE ptr
	if( c_global_storage_ready = FALSE orelse hCGlobalStorageCandidate(sym) = FALSE ) then return NULL
	var identifier = hGetCId(sym)
	if( len(identifier) = 0 ) then return NULL
	var entry = cptr(CGLOBALSTORAGE ptr, hashLookup(@c_global_storage_names, strptr(identifier)))
	if( entry = NULL ) then
		entry = listNewNode(@c_global_storage_nodes)
		if( entry = NULL ) then return NULL
		entry->leaf = hCLeafGetName(identifier)
		if( entry->leaf = NULL ) then return NULL
		hashAdd(@c_global_storage_names, entry->leaf->name, entry, hashHash(entry->leaf->name))
	end if
	return entry
end function

private sub hCGlobalStorageReference(byval sym as FBSYMBOL ptr, byval owner as CPROCLEAF ptr = NULL, _
	byval asm_reference as integer = FALSE)
	if( sym = NULL ) then exit sub
	if( symbIsProc(sym) ) then
		var target = hCLeafGet(sym)
		if( target = NULL ) then target = hCLeafExternal(sym)
		hCLeafLink(owner, target)
		exit sub
	end if
	var entry = hCGlobalStorageGet(sym)
	if( entry = NULL ) then
		if( asm_reference ) then
			'' Owned globals and local statics bypass declaration pruning but
			'' still need a native reference when file-scope ASM names them.
			var identifier = hGetCId(sym)
			hCLeafLink(owner, hCLeafGetName(identifier), FALSE, TRUE)
		end if
		exit sub
	end if
	'' The dependency closure must finish before declarations are filtered.
	'' A missed dependency is an internal error, never an omitted definition.
	if( c_global_storage_prepared andalso entry->needed = FALSE ) then
		errReportEx(FB_ERRMSG_INTERNAL, __FUNCTION__)
	end if
	hCLeafLink(owner, entry->leaf, FALSE, asm_reference)
end sub

private function hCGlobalStorageAsmReferenced(byval sym as FBSYMBOL ptr) as integer
	if( sym = NULL orelse c_leaf_ready = FALSE ) then return FALSE
	var identifier = hGetCId(sym)
	var entry = cptr(CPROCLEAF ptr, hashLookup(@c_leaf_names, strptr(identifier)))
	return (entry <> NULL) andalso entry->native_asm_referenced
end function

private sub hCGlobalStorageInitializerReferences(byval n as ASTNODE ptr, byval owner as CPROCLEAF ptr)
	if( n = NULL ) then exit sub
	'' TYPEINI entries use R for their sibling chain. An explicit work pool
	'' also handles deeply nested expressions without growing the call stack.
	assert(listGetTail(@c_global_initializer_work) = NULL)
	var item = cptr(CGLOBALINITIALIZERWORK ptr, listNewNode(@c_global_initializer_work))
	item->node = n
	while( listGetTail(@c_global_initializer_work) <> NULL )
		item = listGetTail(@c_global_initializer_work)
		n = item->node
		listDelNode(@c_global_initializer_work, item)
		hCGlobalStorageReference(n->sym, owner)
		if( n->r <> NULL ) then
			item = listNewNode(@c_global_initializer_work)
			item->node = n->r
		end if
		if( n->l <> NULL ) then
			item = listNewNode(@c_global_initializer_work)
			item->node = n->l
		end if
	wend
end sub

private sub hCGlobalStorageRegister(byval sym as FBSYMBOL ptr)
	'' Literal text and initializer trees share the variable payload union.
	'' Literals are emitted inline; fake dynamic data uses its descriptor.
	if( symbGetIsLiteral(sym) or symbIsDynamic(sym) ) then exit sub
	'' Do not promote dependencies of definitions that the emitter itself
	'' skips. In particular, an unused array descriptor must not root data.
	if( symbIsExtern(sym) and (symbGetIsAccessed(sym) = FALSE) ) then exit sub
	if( (symbIsPublic(sym) = FALSE) andalso (symbIsCommon(sym) = FALSE) andalso _
	    (symbGetIsAccessed(sym) = FALSE) ) then exit sub
	var entry = hCGlobalStorageGet(sym)
	if( entry <> NULL ) then
		if( env.clopt.debuginfo ) then
			entry->leaf->must_keep = TRUE
		end if
		hCGlobalStorageInitializerReferences(symbGetTypeIniTree(sym), entry->leaf)
	else
		'' Public and owned definitions remain roots even without body reads.
		hCGlobalStorageInitializerReferences(symbGetTypeIniTree(sym), NULL)
	end if
end sub

private sub hCGlobalStoragePrepare()
	if( c_global_storage_ready = FALSE ) then exit sub
	'' Opaque assembly can name storage without a typed C reference. Debug
	'' declarations remain roots, including their initializer dependencies.
	if( c_leaf_opaque ) then exit sub
	symbForEachGlobal(FB_SYMBCLASS_VAR, @hCGlobalStorageRegister)
	'' Include initializer edges before deciding which declarations survive.
	'' An ASM-only use belongs to its procedure, including dead callers.
	hCLeafReachable(TRUE)
	hCLeafReachable()
	var entry = cptr(CGLOBALSTORAGE ptr, listGetHead(@c_global_storage_nodes))
	while( entry <> NULL )
		entry->needed = entry->leaf->referenced
		entry->debug_retained = env.clopt.debuginfo and (entry->leaf->native_referenced = FALSE)
		entry = listGetNext(entry)
	wend
	c_global_storage_prepared = TRUE
end sub

private function hCGlobalStorageNeeded(byval sym as FBSYMBOL ptr) as integer
	if( c_global_storage_prepared = FALSE ) then return TRUE
	if( hCGlobalStorageCandidate(sym) = FALSE ) then return TRUE
	var entry = hCGlobalStorageGet(sym)
	if( entry = NULL ) then return TRUE
	return entry->needed
end function

private function hCGlobalStorageDebugRetained(byval sym as FBSYMBOL ptr) as integer
	if( env.clopt.debuginfo = FALSE or c_global_storage_prepared = FALSE ) then return FALSE
	if( hCGlobalStorageCandidate(sym) = FALSE ) then return FALSE
	var entry = hCGlobalStorageGet(sym)
	return (entry <> NULL) andalso entry->debug_retained
end function

/' end of backend/c/c-generated-storage.bi '/
