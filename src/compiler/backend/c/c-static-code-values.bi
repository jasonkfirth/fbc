/'
	Project: FreeBASIC C backend
	File: backend/c/c-static-code-values.bi
	Purpose: Fold private static procedure addresses whose storage is unobserved.
	Responsibilities: Prove constant initializers and value-only uses before AST
	loading, retain typed procedure addresses, and own per-procedure identities.
	This file intentionally does NOT contain: Source type changes, public storage
	rewrites, runtime initialization or native diagnostic suppression.
'/

type CSTATICCODEVALUE
	sym as FBSYMBOL ptr
	target as FBSYMBOL ptr
	identifier as zstring ptr
end type

type CSTATICCODEWORK
	node as ASTNODE ptr
end type

dim shared as TLIST c_static_code_values

dim shared as integer c_static_code_ready

private sub hCStaticCodeClear()
	if( c_static_code_ready = FALSE ) then exit sub
	do
		var entry = cptr(CSTATICCODEVALUE ptr, listGetHead(@c_static_code_values))
		if( entry = NULL ) then exit do
		ZstrFree(entry->identifier)
		listDelNode(@c_static_code_values, entry)
	loop
end sub

private sub hCStaticCodeFinish()
	if( c_static_code_ready = FALSE ) then exit sub
	hCStaticCodeClear()
	listEnd(@c_static_code_values)
	c_static_code_ready = FALSE
end sub

private sub hCStaticCodeInit()
	hCStaticCodeFinish()
	listInit(@c_static_code_values, 16, sizeof(CSTATICCODEVALUE), LIST_FLAGS_CLEAR)
	c_static_code_ready = TRUE
end sub

private function hCStaticCodeFind(byval sym as FBSYMBOL ptr) as CSTATICCODEVALUE ptr
	if( sym = NULL or c_static_code_ready = FALSE ) then return NULL
	var entry = cptr(CSTATICCODEVALUE ptr, listGetHead(@c_static_code_values))
	while( entry <> NULL )
		if( entry->sym = sym andalso *entry->identifier = hGetCId(sym) ) then return entry
		entry = listGetNext(entry)
	wend
	return NULL
end function

private function hCStaticCodeTarget(byval sym as FBSYMBOL ptr) as FBSYMBOL ptr
	if( sym = NULL orelse symbIsVar(sym) = FALSE ) then return NULL
	if( symbIsLocal(sym) = FALSE or symbIsStatic(sym) = FALSE or symbIsRef(sym) or symbIsParamVar(sym) ) then return NULL
	if( (sym->attrib and (FB_SYMBATTRIB_SHARED or FB_SYMBATTRIB_PUBLIC or _
	    FB_SYMBATTRIB_COMMON or FB_SYMBATTRIB_EXTERN or FB_SYMBATTRIB_IMPORT or _
	    FB_SYMBATTRIB_FUNCRESULT or FB_SYMBATTRIB_DESCRIPTOR)) <> 0 ) then return NULL
	if( symbIsExport(sym) or symbIsThreadLocal(sym) or symbIsThreadLocalC(sym) ) then return NULL
	if( symbIsDynamic(sym) or symbIsDataDesc(sym) or symbGetIsTemporary(sym) or symbGetIsLiteral(sym) ) then return NULL
	if( symbGetArrayDimensions(sym) <> 0 ) then return NULL
	var dtype = symbGetFullType(sym)
	if( typeIsPtr(dtype) = FALSE ) then return NULL
	if( typeGetDtAndPtrOnly(dtype) = typeAddrOf(FB_DATATYPE_FUNCTION) ) then return NULL
	var tree = symbGetTypeIniTree(sym)
	if( tree = NULL orelse tree->class <> AST_NODECLASS_TYPEINI ) then return NULL
	var assignment = tree->l
	if( assignment = NULL orelse assignment->class <> AST_NODECLASS_TYPEINI_ASSIGN ) then return NULL
	if( assignment->r <> NULL ) then return NULL
	var value = assignment->l
	while( value <> NULL andalso value->class = AST_NODECLASS_CONV )
		if( typeIsPtr(value->dtype) = FALSE ) then return NULL
		value = value->l
	wend
	if( value = NULL orelse value->class <> AST_NODECLASS_OFFSET ) then return NULL
	if( value->sym = NULL orelse symbIsProc(value->sym) = FALSE or value->ofs.ofs <> 0 ) then return NULL
	return value->sym
end function

private function hCStaticCodeCheck(byval n as ASTNODE ptr, byval sym as FBSYMBOL ptr, _
	byref seen as integer, byval value_read as integer = FALSE) as integer
	if( n = NULL ) then return TRUE
	select case as const n->class
	case AST_NODECLASS_ASM, AST_NODECLASS_MACRO
		return FALSE
	case AST_NODECLASS_VAR
		if( n->sym = sym ) then
			if( value_read = FALSE or n->var_.ofs <> 0 or n->l <> NULL or n->r <> NULL ) then return FALSE
			if( n->dtype <> symbGetFullType(sym) or n->subtype <> symbGetSubtype(sym) ) then return FALSE
			seen = TRUE
			return TRUE
		end if
	case AST_NODECLASS_DECL
		if( n->sym = sym ) then return TRUE
	case AST_NODECLASS_ASSIGN
		return hCStaticCodeCheck(n->l, sym, seen) andalso hCStaticCodeCheck(n->r, sym, seen, TRUE)
	case AST_NODECLASS_ARG
		var byref_argument = (n->arg.mode = FB_PARAMMODE_BYREF or n->arg.mode = FB_PARAMMODE_BYDESC)
		if( n->sym <> NULL andalso n->sym->class = FB_SYMBCLASS_PARAM ) then
			byref_argument or= (symbGetParamMode(n->sym) <> FB_PARAMMODE_BYVAL and symbGetParamMode(n->sym) <> FB_PARAMMODE_VARARG)
		end if
		return hCStaticCodeCheck(n->l, sym, seen, (byref_argument = FALSE)) andalso hCStaticCodeCheck(n->r, sym, seen)
	case AST_NODECLASS_BOP, AST_NODECLASS_UOP
		'' Assignment optimization can remove ASSIGN and update the operand
		'' in place. A comparison without a result register still only reads.
		if( astOpIsRelational(n->op.op) = FALSE andalso (n->op.options and AST_OPOPT_ALLOCRES) = 0 ) then
			return hCStaticCodeCheck(n->l, sym, seen) andalso hCStaticCodeCheck(n->r, sym, seen, TRUE)
		end if
		return hCStaticCodeCheck(n->l, sym, seen, TRUE) andalso hCStaticCodeCheck(n->r, sym, seen, TRUE)
	case AST_NODECLASS_DEREF
		return hCStaticCodeCheck(n->l, sym, seen, TRUE) andalso hCStaticCodeCheck(n->r, sym, seen, TRUE)
	case AST_NODECLASS_CONV
		return hCStaticCodeCheck(n->l, sym, seen, value_read) andalso hCStaticCodeCheck(n->r, sym, seen)
	case AST_NODECLASS_CALL
		if( n->call.tmpres = sym ) then return FALSE
		var item = n->call.strtail
		while( item <> NULL )
			if( item->sym = sym orelse hCStaticCodeCheck(item->srctree, sym, seen) = FALSE ) then return FALSE
			item = item->prev
		wend
		if( hCStaticCodeCheck(n->call.profbegin, sym, seen) = FALSE ) then return FALSE
		if( hCStaticCodeCheck(n->call.profend, sym, seen) = FALSE ) then return FALSE
	end select
	if( n->sym = sym ) then return FALSE
	return hCStaticCodeCheck(n->l, sym, seen) andalso hCStaticCodeCheck(n->r, sym, seen)
end function

private sub hCStaticCodePush(byref work as TLIST, byval n as ASTNODE ptr)
	if( n = NULL ) then exit sub
	var item = cptr(CSTATICCODEWORK ptr, listNewNode(@work))
	item->node = n
end sub

private function hCStaticCodeInitializerMentions(byval n as ASTNODE ptr, byval sym as FBSYMBOL ptr, byref work as TLIST) as integer
	assert(listGetTail(@work) = NULL)
	hCStaticCodePush(work, n)
	dim as integer found = FALSE
	while( listGetTail(@work) <> NULL )
		var item = cptr(CSTATICCODEWORK ptr, listGetTail(@work))
		n = item->node
		listDelNode(@work, item)
		if( found ) then continue while
		if( n->sym = sym ) then found = TRUE
		'' TYPEINI's R aliases its last item. Walk its L sibling chain once;
		'' an explicit pool avoids recursion for large constant arrays.
		if( n->class <> AST_NODECLASS_TYPEINI ) then hCStaticCodePush(work, n->r)
		hCStaticCodePush(work, n->l)
		if( n->class = AST_NODECLASS_CALL ) then
			if( n->call.tmpres = sym ) then found = TRUE
			var tail = n->call.strtail
			while( tail <> NULL )
				if( tail->sym = sym ) then found = TRUE
				hCStaticCodePush(work, tail->srctree)
				tail = tail->prev
			wend
			hCStaticCodePush(work, n->call.profbegin)
			hCStaticCodePush(work, n->call.profend)
		end if
	wend
	return found
end function

private function hCStaticCodeOtherInitializers(byval first as FBSYMBOL ptr, byval sym as FBSYMBOL ptr, byref work as TLIST) as integer
	var current = first
	while( current <> NULL )
		select case as const current->class
		case FB_SYMBCLASS_SCOPE
			if( hCStaticCodeOtherInitializers(symbGetScopeSymbTbHead(current), sym, work) ) then return TRUE
		case FB_SYMBCLASS_STRUCT, FB_SYMBCLASS_NAMESPACE
			if( hCStaticCodeOtherInitializers(symbGetNamespaceTbHead(current), sym, work) ) then return TRUE
		case FB_SYMBCLASS_VAR
			if( current <> sym andalso symbGetIsLiteral(current) = FALSE andalso symbIsDynamic(current) = FALSE ) then
				if( hCStaticCodeInitializerMentions(symbGetTypeIniTree(current), sym, work) ) then return TRUE
			end if
		end select
		current = symbGetNext(current)
	wend
	return FALSE
end function

private sub hCStaticCodeCollect(byval first as FBSYMBOL ptr, byval all_symbols as FBSYMBOL ptr, byval body as ASTNODE ptr, byref work as TLIST)
	var sym = first
	while( sym <> NULL )
		select case as const sym->class
		case FB_SYMBCLASS_SCOPE
			hCStaticCodeCollect(symbGetScopeSymbTbHead(sym), all_symbols, body, work)
		case FB_SYMBCLASS_STRUCT, FB_SYMBCLASS_NAMESPACE
			hCStaticCodeCollect(symbGetNamespaceTbHead(sym), all_symbols, body, work)
		case FB_SYMBCLASS_VAR
			var target = hCStaticCodeTarget(sym)
			if( target <> NULL ) then
				dim as integer seen = FALSE, valid = TRUE
				var n = body
				while( n <> NULL )
					if( hCStaticCodeCheck(n, sym, seen) = FALSE ) then
						valid = FALSE
						exit while
					end if
					n = n->next
				wend
				if( valid and seen andalso hCStaticCodeOtherInitializers(all_symbols, sym, work) = FALSE ) then
					var entry = cptr(CSTATICCODEVALUE ptr, listNewNode(@c_static_code_values))
					entry->sym = sym
					entry->target = target
					var identifier = hGetCId(sym)
					entry->identifier = ZstrDup(strptr(identifier))
				end if
			end if
		end select
		sym = symbGetNext(sym)
	wend
end sub

private sub hCStaticCodeBegin(byval proc as FBSYMBOL ptr, byval body as ASTNODE ptr)
	hCStaticCodeClear()
	if( c_static_code_ready = FALSE or c_local_refs_complete = FALSE ) then exit sub
	dim as TLIST work
	listInit(@work, 16, sizeof(CSTATICCODEWORK), LIST_FLAGS_CLEAR)
	var symbols = symbGetProcSymbTbHead(proc)
	hCStaticCodeCollect(symbols, symbols, body, work)
	listEnd(@work)
end sub

/' end of backend/c/c-static-code-values.bi '/
