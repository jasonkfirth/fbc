/'
	Project: FreeBASIC C backend
	File: backend/c/c-auto-descriptors.bi
	Purpose: Omit unused metadata for private automatic fixed arrays.
	Responsibilities: Prove descriptor initialization is pure and keep every
	descriptor read, escaped address, lifetime wrapper and uncertain access.
	This file intentionally does NOT contain: array data removal, initialization changes or ABI changes.
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

private sub hCAutoDescReferences(byval n as ASTNODE ptr, byval store_target as integer = FALSE)
	if( n = NULL ) then exit sub
	if( n->class = AST_NODECLASS_ASSIGN ) then
		if( hCAutoDescInitializer(n) <> NULL ) then exit sub
		hCAutoDescReferences(n->l, TRUE)
		hCAutoDescReferences(n->r)
		exit sub
	end if
	if( n->class = AST_NODECLASS_MEM andalso n->mem.op = AST_OP_MEMFILL ) then
		hCAutoDescReferences(n->l, TRUE)
		hCAutoDescReferences(n->r)
		exit sub
	end if
	if( n->class = AST_NODECLASS_FIELD ) then
		hCAutoDescReferences(n->l, store_target)
		exit sub
	end if
	select case as const n->class
	case AST_NODECLASS_VAR, AST_NODECLASS_IDX, AST_NODECLASS_OFFSET, AST_NODECLASS_ADDROF
		var entry = cptr(CAUTODESC ptr, listGetHead(@c_auto_descriptors))
		while( entry <> NULL )
			if( n->sym = entry->sym ) then entry->referenced = TRUE
			if( (store_target = FALSE) and (n->sym = entry->sym->var_.desc.array) ) then entry->backing_used = TRUE
			entry = listGetNext(entry)
		wend
	end select
	'' Subscripts of a store may read data too. Conservatively require a
	'' distinct read/address use before omitting the descriptor.
	hCAutoDescReferences(n->l, store_target)
	hCAutoDescReferences(n->r, store_target)
end sub

private sub hCAutoDescBegin(byval first as ASTNODE ptr)
	hCAutoDescClear()
	if( c_local_refs_complete = FALSE ) then exit sub
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
end sub

private function hCAutoDescUnused(byval sym as FBSYMBOL ptr) as integer
	if( c_local_refs_complete = FALSE ) then return FALSE
	var entry = hCAutoDescGet(sym)
	if( entry = NULL ) then return FALSE
	return (entry->referenced = FALSE) and entry->backing_used
end function

'' end of backend/c/c-auto-descriptors.bi
