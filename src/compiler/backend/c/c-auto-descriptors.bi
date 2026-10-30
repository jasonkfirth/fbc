/'
	Project: FreeBASIC C backend
	File: backend/c/c-auto-descriptors.bi
	Purpose: Omit unused private automatic fixed-array metadata and storage.
	Responsibilities: Prove descriptor initialization is pure and keep every
	descriptor read, escaped address, lifetime wrapper and uncertain access.
	This file intentionally does NOT contain: effectful store removal, lifetime changes or ABI changes.
'/

type CAUTODESC
	sym as FBSYMBOL ptr
	referenced as integer
	backing_used as integer
end type

'' Symbols are borrowed only while the current procedure is being emitted.
'' The pool owns no symbol and is emptied before the next procedure begins.
dim shared as TLIST c_auto_descriptors

private sub hCAutoDescClear()
	do
		var entry = listGetHead(@c_auto_descriptors)
		if( entry = NULL ) then exit do
		listDelNode(@c_auto_descriptors, entry)
	loop
end sub

private function hCAutoDescGet(byval sym as FBSYMBOL ptr) as CAUTODESC ptr
	var entry = cptr(CAUTODESC ptr, listGetHead(@c_auto_descriptors))
	while( entry <> NULL )
		if( entry->sym = sym ) then return entry
		entry = listGetNext(entry)
	wend
	return NULL
end function

private function hCAutoDescCandidate(byval sym as FBSYMBOL ptr) as integer
	if( sym = NULL orelse symbIsVar(sym) = FALSE ) then return FALSE
	if( symbIsLocal(sym) = FALSE orelse symbIsDescriptor(sym) = FALSE ) then return FALSE
	if( (sym->attrib and (FB_SYMBATTRIB_STATIC or FB_SYMBATTRIB_SHARED or _
	    FB_SYMBATTRIB_COMMON or FB_SYMBATTRIB_PUBLIC or FB_SYMBATTRIB_EXTERN)) <> 0 ) then return FALSE
	if( symbIsParamVar(sym) ) then return FALSE
	if( (sym->stats and FB_SYMBSTATS_IMPLICIT) = 0 ) then return FALSE
	var backing = sym->var_.desc.array
	if( backing = NULL orelse symbIsVar(backing) = FALSE ) then return FALSE
	if( symbIsLocal(backing) = FALSE orelse symbIsDynamic(backing) or symbIsStatic(backing) ) then return FALSE
	if( (backing->attrib and (FB_SYMBATTRIB_SHARED or FB_SYMBATTRIB_COMMON or _
	    FB_SYMBATTRIB_PUBLIC or FB_SYMBATTRIB_EXTERN)) <> 0 ) then return FALSE
	if( symbIsParamVar(backing) ) then return FALSE
	if( symbGetArrayDimensions(backing) <= 0 ) then return FALSE
	return (symbHasCtor(backing) = FALSE) and (symbHasDtor(backing) = FALSE)
end function

private sub hCAutoDescCollect(byval n as ASTNODE ptr)
	if( n = NULL ) then exit sub
	if( n->class = AST_NODECLASS_DECL andalso hCAutoDescCandidate(n->sym) ) then
		if( hCAutoDescGet(n->sym) = NULL ) then
			var entry = cptr(CAUTODESC ptr, listNewNode(@c_auto_descriptors))
			entry->sym = n->sym
			'' A default constructor or clearing operation has a separate owner.
			entry->referenced = (n->l <> NULL)
		end if
	end if
	hCAutoDescCollect(n->l)
	hCAutoDescCollect(n->r)
end sub

private function hCAutoDescPureValue(byval n as ASTNODE ptr, byval backing as FBSYMBOL ptr) as integer
	if( n = NULL ) then return FALSE
	if( typeGetClass(n->dtype) <> FB_DATACLASS_INTEGER ) then return FALSE
	select case as const n->class
	case AST_NODECLASS_CONST
		return TRUE
	case AST_NODECLASS_OFFSET, AST_NODECLASS_ADDROF
		'' Automatic data uses ADDROF(VAR); global addresses may use OFFSET.
		'' Both must name this fixed array directly, without a dereference.
		if( n->class = AST_NODECLASS_OFFSET andalso n->sym <> backing ) then return FALSE
		return (n->l <> NULL) andalso _
		       (n->l->class = AST_NODECLASS_VAR) andalso (n->l->sym = backing)
	case AST_NODECLASS_CONV
		return hCAutoDescPureValue(n->l, backing)
	case AST_NODECLASS_BOP
		if( n->op.op <> AST_OP_ADD and n->op.op <> AST_OP_SUB ) then return FALSE
		return hCAutoDescPureValue(n->l, backing) andalso hCAutoDescPureValue(n->r, backing)
	end select
	return FALSE
end function

private function hCAutoDescInitializer(byval n as ASTNODE ptr) as CAUTODESC ptr
	if( n->class <> AST_NODECLASS_ASSIGN ) then return NULL
	if( (n->op.options and AST_OPOPT_ISINI) = 0 ) then return NULL
	var target = n->l
	if( target = NULL ) then return NULL
	if( target->class = AST_NODECLASS_FIELD ) then target = target->l
	if( target = NULL orelse target->class <> AST_NODECLASS_VAR ) then return NULL
	var entry = hCAutoDescGet(target->sym)
	if( entry = NULL ) then return NULL
	var bytes = typeGetSize(target->dtype)
	if( target->var_.ofs < 0 orelse bytes <= 0 ) then return NULL
	if( target->var_.ofs > symbGetSizeOf(entry->sym) ) then return NULL
	if( bytes > symbGetSizeOf(entry->sym) - target->var_.ofs ) then return NULL
	if( hCAutoDescPureValue(n->r, entry->sym->var_.desc.array) = FALSE ) then return NULL
	return entry
end function

private function hCAutoDescDataInitializer(byval n as ASTNODE ptr) as CAUTODESC ptr
	var target = n->l
	if( target = NULL ) then return NULL
	select case as const n->class
	case AST_NODECLASS_ASSIGN
		if( (n->op.options and AST_OPOPT_ISINI) = 0 ) then return NULL
		if( target->class = AST_NODECLASS_FIELD ) then target = target->l
	case AST_NODECLASS_MEM
		if( n->mem.op <> AST_OP_MEMFILL ) then return NULL
		if( n->r = NULL orelse n->r->class <> AST_NODECLASS_CONST ) then return NULL
		if( target->class = AST_NODECLASS_ADDROF ) then target = target->l
	case else
		return NULL
	end select
	if( target = NULL orelse target->class <> AST_NODECLASS_VAR ) then return NULL
	var entry = cptr(CAUTODESC ptr, listGetHead(@c_auto_descriptors))
	while( entry <> NULL )
		if( target->sym = entry->sym->var_.desc.array ) then exit while
		entry = listGetNext(entry)
	wend
	if( entry = NULL ) then return NULL
	var bytes = clngint(typeGetSize(target->dtype))
	if( n->class = AST_NODECLASS_MEM ) then bytes = astConstGetInt(n->r)
	var size = symbGetRealSize(target->sym)
	if( target->var_.ofs < 0 orelse bytes < 0 orelse target->var_.ofs > size ) then return NULL
	if( bytes > size - target->var_.ofs ) then return NULL
	if( n->class = AST_NODECLASS_ASSIGN ) then
		if( bytes = 0 orelse hCAutoDescPureValue(n->r, target->sym) = FALSE ) then return NULL
	end if
	return entry
end function

private sub hCAutoDescRemember(byval sym as FBSYMBOL ptr)
	var entry = cptr(CAUTODESC ptr, listGetHead(@c_auto_descriptors))
	while( entry <> NULL )
		if( sym = entry->sym ) then entry->referenced = TRUE
		if( sym = entry->sym->var_.desc.array ) then entry->backing_used = TRUE
		entry = listGetNext(entry)
	wend
end sub

private sub hCAutoDescReferences(byval n as ASTNODE ptr)
	if( n = NULL ) then exit sub
	if( n->class = AST_NODECLASS_ASSIGN ) then
		if( hCAutoDescInitializer(n) <> NULL ) then exit sub
	end if
	if( hCAutoDescDataInitializer(n) <> NULL ) then exit sub
	select case as const n->class
	case AST_NODECLASS_VAR, AST_NODECLASS_IDX, AST_NODECLASS_OFFSET, AST_NODECLASS_ADDROF
		hCAutoDescRemember(n->sym)
	case AST_NODECLASS_CALL
		'' These references are loaded outside the ordinary operand trees.
		hCAutoDescRemember(n->call.tmpres)
		var item = n->call.strtail
		while( item <> NULL )
			hCAutoDescRemember(item->sym)
			hCAutoDescReferences(item->srctree)
			item = item->prev
		wend
		hCAutoDescReferences(n->call.profbegin)
		hCAutoDescReferences(n->call.profend)
	end select
	hCAutoDescReferences(n->l)
	hCAutoDescReferences(n->r)
end sub

private function hCAutoDescPrune(byval n as ASTNODE ptr) as integer
	if( n = NULL ) then return FALSE
	dim as CAUTODESC ptr entry = NULL
	if( n->class = AST_NODECLASS_ASSIGN ) then entry = hCAutoDescInitializer(n)
	if( entry = NULL ) then entry = hCAutoDescDataInitializer(n)
	if( entry <> NULL andalso entry->referenced = FALSE ) then
		if( (n->class = AST_NODECLASS_ASSIGN andalso hCAutoDescInitializer(n) = entry) orelse _
		    entry->backing_used = FALSE ) then
			'' Only bounded, pure writes reach here. Keep the statement node's
			'' links and source position while retiring its owned expressions.
			astDelTree(n->l)
			astDelTree(n->r)
			n->l = NULL
			n->r = NULL
			n->sym = NULL
			n->class = AST_NODECLASS_NOP
			n->dtype = FB_DATATYPE_VOID
			n->subtype = NULL
			return TRUE
		end if
	end if
	var changed = hCAutoDescPrune(n->l)
	return hCAutoDescPrune(n->r) or changed
end function

private function hCAutoDescBegin(byval first as ASTNODE ptr) as integer
	hCAutoDescClear()
	if( c_local_refs_complete = FALSE ) then return FALSE
	var n = first
	while( n <> NULL )
		hCAutoDescCollect(n)
		n = n->next
	wend
	n = first
	while( n <> NULL )
		hCAutoDescReferences(n)
		n = n->next
	wend
	'' An unread descriptor may be the only apparent use of an unread data
	'' array. Classify both before deleting either one's initialization.
	dim as integer changed = FALSE
	n = first
	while( n <> NULL )
		changed or= hCAutoDescPrune(n)
		n = n->next
	wend
	return changed
end function

private function hCAutoDescUnused(byval sym as FBSYMBOL ptr) as integer
	if( c_local_refs_complete = FALSE ) then return FALSE
	var entry = hCAutoDescGet(sym)
	if( entry = NULL ) then return FALSE
	return (entry->referenced = FALSE)
end function

private function hCAutoDescUnusedData(byval sym as FBSYMBOL ptr) as integer
	if( c_local_refs_complete = FALSE ) then return FALSE
	var entry = cptr(CAUTODESC ptr, listGetHead(@c_auto_descriptors))
	while( entry <> NULL )
		if( sym = entry->sym->var_.desc.array ) then
			return (entry->referenced = FALSE) and (entry->backing_used = FALSE)
		end if
		entry = listGetNext(entry)
	wend
	return FALSE
end function

'' end of backend/c/c-auto-descriptors.bi
