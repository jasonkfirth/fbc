/'
	Project: FreeBASIC C backend
	File: backend/c/c-static-array-storage.bi
	Purpose: Omit unobserved private static storage and fixed-array metadata.
	Responsibilities: Retain initializer dependencies before AST loading consumes
	them, close live constant bindings, and preserve element lifetime and addresses.
	This file intentionally does NOT contain: Runtime initializer removal, source
	type changes, global storage liveness or native diagnostic suppression.
'/

type CSTATICINITIALIZERWORK
	node as ASTNODE ptr
end type

type CSTATICCONSTANT
	sym as FBSYMBOL ptr
	queued as integer
	next_pending as CSTATICCONSTANT ptr
end type

private function hCStaticArrayPrivate(byval sym as FBSYMBOL ptr, byval allow_reference as integer = FALSE) as integer
	if( sym = NULL orelse symbIsVar(sym) = FALSE ) then return FALSE
	if( symbIsLocal(sym) = FALSE orelse symbIsStatic(sym) = FALSE ) then return FALSE
	if( symbGetIsLiteral(sym) or symbIsDynamic(sym) or symbIsParamVar(sym) ) then return FALSE
	if( symbIsRef(sym) and (allow_reference = FALSE) ) then return FALSE
	if( (sym->attrib and (FB_SYMBATTRIB_SHARED or FB_SYMBATTRIB_PUBLIC or _
	    FB_SYMBATTRIB_COMMON or FB_SYMBATTRIB_EXTERN or FB_SYMBATTRIB_IMPORT)) <> 0 ) then return FALSE
	return (symbIsExport(sym) = FALSE) and (symbIsThreadLocal(sym) = FALSE) and (symbIsThreadLocalC(sym) = FALSE)
end function

private function hCStaticConstantCandidate(byval sym as FBSYMBOL ptr) as integer
	if( hCStaticArrayPrivate(sym, TRUE) = FALSE ) then return FALSE
	if( symbGetArrayDimensions(sym) <> 0 or symbIsDescriptor(sym) or symbIsDataDesc(sym) ) then return FALSE
	if( (sym->attrib and FB_SYMBATTRIB_FUNCRESULT) <> 0 ) then return FALSE
	if( symbIsRef(sym) = FALSE ) then
		if( symbGetIsTemporary(sym) or symbHasCtor(sym) or symbHasDtor(sym) ) then return FALSE
		select case as const typeGetClass(symbGetFullType(sym))
		case FB_DATACLASS_INTEGER, FB_DATACLASS_FPOINT
		case else
			return FALSE
		end select
	end if
	var initializer = symbGetTypeIniTree(sym)
	if( initializer = NULL ) then return FALSE
	'' A reference binding owns a pointer, not its pointee's lifetime. Only
	'' constant data can disappear here; runtime initialization stays in the AST.
	return astTypeIniIsConst(initializer)
end function

private function hCStaticConstantGet(byref index as THASH, byval sym as FBSYMBOL ptr) as CSTATICCONSTANT ptr
	if( sym = NULL ) then return NULL
	var key = str(culngint(sym))
	return hashLookup(@index, strptr(key))
end function

private sub hCStaticConstantCollect(byval sym as FBSYMBOL ptr, byref entries as TLIST, byref index as THASH)
	while( sym <> NULL )
		select case as const sym->class
		case FB_SYMBCLASS_SCOPE
			hCStaticConstantCollect(symbGetScopeSymbTbHead(sym), entries, index)
		case FB_SYMBCLASS_STRUCT, FB_SYMBCLASS_NAMESPACE
			hCStaticConstantCollect(symbGetNamespaceTbHead(sym), entries, index)
		case FB_SYMBCLASS_VAR
			if( hCStaticConstantCandidate(sym) ) then
				var entry = cptr(CSTATICCONSTANT ptr, listNewNode(@entries))
				entry->sym = sym
				var key = str(culngint(sym))
				hashAdd(@index, ZstrDup(strptr(key)), entry, hashHash(strptr(key)))
			end if
		end select
		sym = symbGetNext(sym)
	wend
end sub

private sub hCStaticConstantReference(byval sym as FBSYMBOL ptr, byref index as THASH, byref pending as CSTATICCONSTANT ptr)
	var entry = hCStaticConstantGet(index, sym)
	if( entry = NULL ) then exit sub
	hRememberCLocalReference(sym)
	if( entry->queued ) then exit sub
	entry->queued = TRUE
	entry->next_pending = pending
	pending = entry
end sub

private function hCStaticArrayBacking(byval sym as FBSYMBOL ptr) as FBSYMBOL ptr
	if( hCStaticArrayPrivate(sym) = FALSE ) then return NULL
	var backing = sym
	if( symbIsDescriptor(sym) ) then backing = sym->var_.desc.array
	if( hCStaticArrayPrivate(backing) = FALSE ) then return NULL
	if( symbGetArrayDimensions(backing) <= 0 or symbGetIsTemporary(backing) ) then return NULL
	if( symbHasCtor(backing) or symbHasDtor(backing) or symbIsDataDesc(backing) ) then return NULL
	'' Only default zero storage is omitted. An initializer has its own meaning
	'' even when the procedure never directly reads the declared array.
	if( symbGetTypeIniTree(backing) <> NULL ) then return NULL
	var descriptor = symbGetArrayDescriptor(backing)
	if( hCStaticArrayPrivate(descriptor) = FALSE ) then return NULL
	if( symbIsDescriptor(descriptor) = FALSE ) then return NULL
	if( (descriptor->stats and FB_SYMBSTATS_IMPLICIT) = 0 ) then return NULL
	if( descriptor->var_.desc.array <> backing ) then return NULL
	if( sym <> backing and sym <> descriptor ) then return NULL
	return backing
end function

private function hCStaticScalarCandidate(byval sym as FBSYMBOL ptr) as integer
	if( hCStaticArrayPrivate(sym) = FALSE ) then return FALSE
	if( symbGetArrayDimensions(sym) <> 0 or symbIsDescriptor(sym) ) then return FALSE
	if( symbGetIsTemporary(sym) or symbIsDataDesc(sym) ) then return FALSE
	if( (sym->attrib and FB_SYMBATTRIB_FUNCRESULT) <> 0 ) then return FALSE
	if( symbHasCtor(sym) or symbHasDtor(sym) ) then return FALSE
	if( symbGetTypeIniTree(sym) <> NULL ) then return FALSE
	select case as const symbGetType(sym)
	case FB_DATATYPE_STRING, FB_DATATYPE_USTRING
		return FALSE
	end select
	return TRUE
end function

private sub hCStaticArrayReference(byval sym as FBSYMBOL ptr)
	if( hCStaticArrayBacking(sym) <> NULL orelse hCStaticScalarCandidate(sym) ) then
		hRememberCLocalReference(sym)
		hCScalarStoreRemember(sym)
	end if
end sub

private sub hCStaticInitializerPush(byref work as TLIST, byval n as ASTNODE ptr)
	if( n = NULL ) then exit sub
	var item = cptr(CSTATICINITIALIZERWORK ptr, listNewNode(@work))
	item->node = n
end sub

private sub hCStaticInitializerTree(byval n as ASTNODE ptr, byref work as TLIST, byref index as THASH, byref pending as CSTATICCONSTANT ptr)
	assert(listGetTail(@work) = NULL)
	hCStaticInitializerPush(work, n)
	'' TYPEINI uses R for its sibling chain. A reusable work pool avoids call
	'' stack growth for large constant arrays; NEXT belongs to statement lists.
	while( listGetTail(@work) <> NULL )
		var item = cptr(CSTATICINITIALIZERWORK ptr, listGetTail(@work))
		n = item->node
		listDelNode(@work, item)
		select case as const n->class
		case AST_NODECLASS_VAR, AST_NODECLASS_IDX, AST_NODECLASS_OFFSET, AST_NODECLASS_ADDROF
			hCStaticArrayReference(n->sym)
			hCStaticConstantReference(n->sym, index, pending)
		end select
		if( n->class = AST_NODECLASS_CALL ) then
			hCStaticArrayReference(n->call.tmpres)
			hCStaticConstantReference(n->call.tmpres, index, pending)
			var tail = n->call.strtail
			while( tail <> NULL )
				hCStaticArrayReference(tail->sym)
				hCStaticConstantReference(tail->sym, index, pending)
				hCStaticInitializerPush(work, tail->srctree)
				tail = tail->prev
			wend
			hCStaticInitializerPush(work, n->call.profbegin)
			hCStaticInitializerPush(work, n->call.profend)
		end if
		'' TYPEINI's R is a tail alias, not another child. Its item list starts
		'' at L and links siblings through R; visit each expression once.
		if( n->class <> AST_NODECLASS_TYPEINI ) then hCStaticInitializerPush(work, n->r)
		hCStaticInitializerPush(work, n->l)
	wend
end sub

private sub hCStaticInitializerSymbols(byval sym as FBSYMBOL ptr, byref work as TLIST, byval close_descriptors as integer, byref index as THASH, byref pending as CSTATICCONSTANT ptr)
	while( sym <> NULL )
		select case as const sym->class
		case FB_SYMBCLASS_SCOPE
			hCStaticInitializerSymbols(symbGetScopeSymbTbHead(sym), work, close_descriptors, index, pending)
		case FB_SYMBCLASS_STRUCT, FB_SYMBCLASS_NAMESPACE
			hCStaticInitializerSymbols(symbGetNamespaceTbHead(sym), work, close_descriptors, index, pending)
		case FB_SYMBCLASS_VAR
			'' Literal text shares the initializer payload union. Fake dynamic
			'' data also has a different owner; neither payload is a TYPEINI.
			if( symbGetIsLiteral(sym) = FALSE and symbIsDynamic(sym) = FALSE ) then
				var backing = hCStaticArrayBacking(sym)
				if( close_descriptors ) then
					if( backing <> NULL andalso symbIsDescriptor(sym) andalso hHasCLocalReference(sym) ) then
						hCStaticArrayReference(backing)
					end if
				elseif( backing = NULL orelse symbIsDescriptor(sym) = FALSE ) then
					'' An implicit fixed descriptor only points to its own backing
					'' array. Root it when another initializer or the body uses it,
					'' rather than allowing dormant metadata to retain dormant data.
					var entry = hCStaticConstantGet(index, sym)
					if( entry = NULL ) then
						hCStaticInitializerTree(symbGetTypeIniTree(sym), work, index, pending)
					elseif( hHasCLocalReference(sym) ) then
						hCStaticConstantReference(sym, index, pending)
					end if
				end if
			end if
		end select
		sym = symbGetNext(sym)
	wend
end sub

private sub hCStaticArrayInitializers(byval proc as FBSYMBOL ptr)
	if( c_local_refs_complete = FALSE ) then exit sub
	dim as TLIST work, entries
	dim as THASH index
	dim as CSTATICCONSTANT ptr pending
	listInit(@work, 32, sizeof(CSTATICINITIALIZERWORK), LIST_FLAGS_CLEAR)
	listInit(@entries, 32, sizeof(CSTATICCONSTANT), LIST_FLAGS_CLEAR)
	hashInit(@index, 256, TRUE)
	'' Borrow symbols only until this procedure's declarations are emitted.
	'' Roots are actual body uses and initializers whose storage remains. A
	'' pending list closes reference chains without repeated whole-scope scans;
	'' unobserved self-references never become roots merely by naming themselves.
	hCStaticConstantCollect(symbGetProcSymbTbHead(proc), entries, index)
	hCStaticInitializerSymbols(symbGetProcSymbTbHead(proc), work, FALSE, index, pending)
	hCStaticInitializerSymbols(symbGetProcSymbTbHead(proc), work, TRUE, index, pending)
	while( pending <> NULL )
		var entry = pending
		pending = entry->next_pending
		hCStaticInitializerTree(symbGetTypeIniTree(entry->sym), work, index, pending)
	wend
	hashEnd(@index)
	listEnd(@entries)
	listEnd(@work)
end sub

private function hCStaticArrayStorageUnused(byval sym as FBSYMBOL ptr) as integer
	if( c_local_refs_complete = FALSE ) then return FALSE
	var backing = hCStaticArrayBacking(sym)
	if( backing = NULL ) then return FALSE
	if( hCScalarStoreUnusedValue(backing) ) then return TRUE
	if( hHasCLocalReference(sym) ) then return FALSE
	'' A referenced descriptor keeps the data address it contains. The other
	'' direction is unnecessary: direct array indexing needs only the data.
	if( sym = backing ) then return (hHasCLocalReference(symbGetArrayDescriptor(backing)) = FALSE)
	return TRUE
end function

private function hCStaticScalarStorageUnused(byval sym as FBSYMBOL ptr) as integer
	if( c_local_refs_complete = FALSE ) then return FALSE
	if( hCStaticConstantCandidate(sym) ) then return (hHasCLocalReference(sym) = FALSE)
	if( hCStaticScalarCandidate(sym) = FALSE ) then return FALSE
	'' A folded address comparison no longer needs the object's storage.
	'' Pending static initializers were scanned before declarations consume
	'' them, so a retained pointer initializer still keeps its target alive.
	return (hHasCLocalReference(sym) = FALSE) or hCScalarStoreUnusedValue(sym)
end function

/' end of backend/c/c-static-array-storage.bi '/
