/'
	Project: FreeBASIC C backend
	File: backend/c/c-unobserved-arguments.bi
	Purpose: Omit literal pointer values that a direct callee never observes.
	Responsibilities: Inspect complete bodies, retain durable parameter identities,
	and recognize pure literal addresses before their argument casts are emitted.
	This file intentionally does NOT contain: ABI changes, expression evaluation
	removal, native warning annotations or assumptions about unknown bodies.
'/

type CUNOBSERVEDPARAM
	param as FBSYMBOL ptr
end type

'' PARAM symbols outlive body loading; their local variable symbols do not.
'' Keep only the durable identity after inspecting the complete optimized AST.
type CUNOBSERVEDPROC
	proc as FBSYMBOL ptr
end type

dim shared as TLIST c_unobserved_params, c_unobserved_procs
dim shared as integer c_unobserved_ready

private sub hCUnobservedFinish()
	if( c_unobserved_ready = FALSE ) then exit sub
	listEnd(@c_unobserved_params)
	listEnd(@c_unobserved_procs)
	c_unobserved_ready = FALSE
end sub

private sub hCUnobservedInit()
	hCUnobservedFinish()
	listInit(@c_unobserved_params, 16, sizeof(CUNOBSERVEDPARAM), LIST_FLAGS_CLEAR)
	listInit(@c_unobserved_procs, 16, sizeof(CUNOBSERVEDPROC), LIST_FLAGS_CLEAR)
	c_unobserved_ready = TRUE
end sub

private sub hCUnobservedBegin(byval proc as FBSYMBOL ptr, byval body as ASTNODE ptr)
	var known = cptr(CUNOBSERVEDPROC ptr, listGetHead(@c_unobserved_procs))
	while( known <> NULL )
		if( known->proc = proc ) then exit sub
		known = listGetNext(known)
	wend
	known = listNewNode(@c_unobserved_procs)
	known->proc = proc
	if( env.clopt.pic or env.clopt.debuginfo or symbIsNaked(proc) ) then exit sub
	'' Native aliases may carry C builtin contracts not expressed by this AST.
	'' BASIC definitions own the contract used by ordinary direct calls.
	if( symbGetMangling(proc) <> FB_MANGLING_BASIC ) then exit sub
	if( (proc->stats and FB_SYMBSTATS_HASALIAS) <> 0 ) then exit sub
	var param = symbGetProcLastParam(proc)
	while( param <> NULL )
		if( symbGetParamMode(param) = FB_PARAMMODE_VARARG ) then exit sub
		param = symbGetProcPrevParam(proc, param)
	wend
	param = symbGetProcLastParam(proc)
	while( param <> NULL )
		'' ABI lowering also introduces pointers for by-value records. Only a
		'' source pointer parameter supplies the value-only contract used here.
		dim as integer dtype = FB_DATATYPE_INVALID
		dim as FBSYMBOL ptr subtype = NULL
		symbGetRealParamDtype(param, dtype, subtype)
		if( symbGetParamMode(param) = FB_PARAMMODE_BYVAL andalso _
		    typeIsPtr(symbGetFullType(param)) andalso typeIsPtr(dtype) andalso _
		    typeGetDtOnly(dtype) <> FB_DATATYPE_FUNCTION ) then
			var storage = symbGetParamVar(param)
			if( storage <> NULL andalso hCReadOnlyUsed(body, storage) = FALSE ) then
				'' The shared inspector also rejects opaque macros, assembly and
				'' call-owned storage that can observe a parameter outside VAR nodes.
				dim as integer seen = FALSE, safe = TRUE
				var n = body
				while( n <> NULL )
					if( hCReadOnlyCheck(n, storage, seen) = FALSE ) then
						safe = FALSE
						exit while
					end if
					n = n->next
				wend
				if( safe andalso seen = FALSE ) then
					var entry = cptr(CUNOBSERVEDPARAM ptr, listNewNode(@c_unobserved_params))
					entry->param = param
				end if
			end if
		end if
		param = symbGetProcPrevParam(proc, param)
	wend
end sub

private function hCUnobservedLiteralAddress(byval actual as EXPRNODE ptr) as integer
	'' Only an existing literal address has no evaluation or lifetime work.
	'' Do not discard pointer arithmetic, getter calls, temporaries or variables.
	while( actual <> NULL )
		select case as const actual->class
		case EXPRCLASS_CAST
			if( typeIsPtr(actual->dtype) = FALSE ) then return FALSE
			actual = actual->l
		case EXPRCLASS_UOP
			if( actual->op <> AST_OP_ADDROF ) then return FALSE
			actual = actual->l
		case EXPRCLASS_SYM
			if( actual->sym = NULL orelse symbIsVar(actual->sym) = FALSE ) then return FALSE
			return symbGetIsLiteral(actual->sym)
		case else
			return FALSE
		end select
	wend
	return FALSE
end function

private function hCUnobservedLiteral(byval param as FBSYMBOL ptr, byval actual as EXPRNODE ptr) as integer
	if( param = NULL or c_unobserved_ready = FALSE or c_local_refs_complete = FALSE ) then return FALSE
	if( env.clopt.pic or env.clopt.debuginfo or c_leaf_opaque ) then return FALSE
	if( symbGetParamMode(param) <> FB_PARAMMODE_BYVAL ) then return FALSE
	if( typeIsPtr(symbGetFullType(param)) = FALSE ) then return FALSE
	if( hCUnobservedLiteralAddress(actual) = FALSE ) then return FALSE
	var entry = cptr(CUNOBSERVEDPARAM ptr, listGetHead(@c_unobserved_params))
	while( entry <> NULL )
		if( entry->param = param ) then exit while
		entry = listGetNext(entry)
	wend
	if( entry = NULL ) then
		'' A caller may precede a complete pending definition. The current
		'' body is being consumed, so its proof must come from procedure entry.
		var proc = symbGetParent(param)
		if( proc = ctx.currproc ) then return FALSE
		var body = ast.proc.head
		while( body <> NULL )
			if( body->sym = proc ) then exit while
			body = body->next
		wend
		if( body = NULL ) then return FALSE
		hCUnobservedBegin(proc, body->l)
		entry = listGetHead(@c_unobserved_params)
		while( entry <> NULL )
			if( entry->param = param ) then exit while
			entry = listGetNext(entry)
		wend
		if( entry = NULL ) then return FALSE
	end if
	return TRUE
end function

'' end of backend/c/c-unobserved-arguments.bi
