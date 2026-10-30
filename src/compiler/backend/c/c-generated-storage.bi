/'
	Project: FreeBASIC C backend
	File: backend/c/c-generated-storage.bi
	Purpose: Omit unobserved private global storage from generated C.
	Responsibilities: Record actual C uses, close static initializer dependencies,
	and retain public, owned and thread-local storage.
	This file intentionally does NOT contain: Procedure liveness, initialization
	evaluation, source access flag changes or public ABI changes.
'/

type CGLOBALSTORAGE
	name as zstring ptr
	sym as FBSYMBOL ptr
	needed as integer
	next_pending as CGLOBALSTORAGE ptr
end type

type CGLOBALINITIALIZERWORK
	node as ASTNODE ptr
end type

dim shared as TLIST c_global_storage_nodes, c_global_initializer_work
dim shared as THASH c_global_storage_names
dim shared as CGLOBALSTORAGE ptr c_global_storage_pending
dim shared as integer c_global_storage_ready, c_global_storage_prepared

private sub hCGlobalStorageFinish()
	if( c_global_storage_ready = FALSE ) then exit sub
	hashEnd(@c_global_storage_names)
	var entry = cptr(CGLOBALSTORAGE ptr, listGetHead(@c_global_storage_nodes))
	while( entry <> NULL )
		ZstrFree(entry->name)
		entry = listGetNext(entry)
	wend
	listEnd(@c_global_storage_nodes)
	listEnd(@c_global_initializer_work)
	c_global_storage_pending = NULL
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
	if( symbIsDataDesc(sym) or symbHasCtor(sym) or symbHasDtor(sym) ) then return FALSE
	select case as const symbGetType(sym)
	case FB_DATATYPE_STRING, FB_DATATYPE_USTRING
		return FALSE
	case typeAddrOf(FB_DATATYPE_WCHAR)
		if( symbGetIsTemporary(sym) ) then return FALSE
	end select
	return TRUE
end function

private function hCGlobalStorageGet(byval sym as FBSYMBOL ptr) as CGLOBALSTORAGE ptr
	if( c_global_storage_ready = FALSE orelse hCGlobalStorageCandidate(sym) = FALSE ) then return NULL
	var identifier = hGetCId(sym)
	if( len(identifier) = 0 ) then return NULL
	var entry = cptr(CGLOBALSTORAGE ptr, hashLookup(@c_global_storage_names, strptr(identifier)))
	if( entry = NULL ) then
		entry = listNewNode(@c_global_storage_nodes)
		entry->name = ZstrDup(strptr(identifier))
		hashAdd(@c_global_storage_names, entry->name, entry, hashHash(entry->name))
	end if
	return entry
end function

private sub hCGlobalStorageReference(byval sym as FBSYMBOL ptr)
	var entry = hCGlobalStorageGet(sym)
	if( entry = NULL orelse entry->needed ) then exit sub
	'' The dependency closure must finish before declarations are filtered.
	'' A missed dependency is an internal error, never an omitted definition.
	if( c_global_storage_prepared ) then
		errReportEx(FB_ERRMSG_INTERNAL, __FUNCTION__)
	end if
	entry->needed = TRUE
	entry->next_pending = c_global_storage_pending
	c_global_storage_pending = entry
end sub

private sub hCGlobalStorageInitializerReferences(byval n as ASTNODE ptr)
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
		hCGlobalStorageReference(n->sym)
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
		'' Only the global-symbol traversal lends symbol pointers to this index.
		'' Those symbols survive through the final global declaration pass.
		entry->sym = sym
	else
		'' Public and owned definitions remain roots even without body reads.
		hCGlobalStorageInitializerReferences(symbGetTypeIniTree(sym))
	end if
end sub

private sub hCGlobalStoragePrepare()
	if( c_global_storage_ready = FALSE ) then exit sub
	'' Opaque assembly can name storage without a typed C reference. Debug
	'' output also preserves the original declaration ordering and contents.
	if( c_leaf_opaque or env.clopt.debuginfo ) then exit sub
	symbForEachGlobal(FB_SYMBCLASS_VAR, @hCGlobalStorageRegister)
	while( c_global_storage_pending <> NULL )
		var entry = c_global_storage_pending
		c_global_storage_pending = entry->next_pending
		if( entry->sym <> NULL ) then
			hCGlobalStorageInitializerReferences(symbGetTypeIniTree(entry->sym))
		end if
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

/' end of backend/c/c-generated-storage.bi '/
