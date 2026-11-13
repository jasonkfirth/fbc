/'
	Project: FreeBASIC C backend
	File: backend/c/c-debug-locals.bi
	Purpose: Describe named debug storage without evaluating an unread value.
	Responsibilities: Index optimized-AST reads and retain local declarations,
	stores and lifetime code required by source debugging.
	This file intentionally does NOT remove storage, change source diagnostics,
	inspect C text or acknowledge compiler-generated temporaries.
'/

'' Symbols belong to the current procedure. The hash owns its address keys
'' and is cleared with the other per-procedure storage indexes.
dim shared as THASH c_debug_local_reads
dim shared as uinteger c_debug_local_id

private sub hCDebugLocalsClear()
	if( c_debug_local_reads.list <> NULL ) then hashEnd(@c_debug_local_reads)
	c_debug_local_id = 0
end sub

private sub hCDebugLocalRead(byval sym as FBSYMBOL ptr)
	if( sym = NULL orelse symbIsVar(sym) = FALSE ) then exit sub
	if( symbIsLocal(sym) = FALSE ) then exit sub
	if( c_debug_local_reads.list = NULL ) then hashInit(@c_debug_local_reads, 256, TRUE)
	var key = str(culngint(cuint(sym)))
	if( hashLookup(@c_debug_local_reads, strptr(key)) = NULL ) then
		hashAdd(@c_debug_local_reads, ZstrDup(strptr(key)), sym, hashHash(strptr(key)))
	end if
end sub

private sub hCDebugLocalReads(byval n as ASTNODE ptr)
	if( env.clopt.debuginfo = FALSE or n = NULL ) then exit sub
	select case as const n->class
	case AST_NODECLASS_ASSIGN
		if( n->l <> NULL andalso n->l->class = AST_NODECLASS_VAR ) then
			'' A direct store preserves the debugger's value, but does not read
			'' the destination. Read its entire RHS, including effectful calls.
			hCDebugLocalReads(n->r)
			exit sub
		end if
	case AST_NODECLASS_VAR, AST_NODECLASS_OFFSET
		hCDebugLocalRead(n->sym)
	case AST_NODECLASS_CALL
		'' These loader-owned addresses are not all present in the L/R tree.
		'' A real result slot, copyback or address escape observes storage.
		hCDebugLocalRead(n->call.tmpres)
		var item = n->call.strtail
		while( item <> NULL )
			hCDebugLocalRead(item->sym)
			hCDebugLocalReads(item->srctree)
			item = item->prev
		wend
		hCDebugLocalReads(n->call.profbegin)
		hCDebugLocalReads(n->call.profend)
	end select
	'' An address, indirect store or constructor remains an observation.
	'' Declaration symbols alone are not reads; their initializer children are.
	hCDebugLocalReads(n->l)
	hCDebugLocalReads(n->r)
end sub

private function hCDebugLocalAcknowledgement _
	( byval sym as FBSYMBOL ptr, byref identifier as const string ) as string

	if( env.clopt.debuginfo = FALSE or ctx.computedgotoopaque ) then return ""
	if( symbIsLocal(sym) = FALSE or symbIsParamVar(sym) or symbIsStatic(sym) ) then return ""
	if( symbIsTemp(sym) or symbGetIsImplicit(sym) or symbIsDescriptor(sym) ) then return ""
	if( symbIsRef(sym) or symbGetArrayDimensions(sym) <> 0 ) then return ""
	if( (sym->attrib and (FB_SYMBATTRIB_SHARED or FB_SYMBATTRIB_COMMON or _
	    FB_SYMBATTRIB_PUBLIC or FB_SYMBATTRIB_EXTERN or FB_SYMBATTRIB_IMPORT or _
	    FB_SYMBATTRIB_FUNCRESULT or FB_SYMBATTRIB_THREADLOCAL or FB_SYMBATTRIB_THREADLOCALC)) <> 0 ) then return ""
	if( c_debug_local_reads.list <> NULL ) then
		var key = str(culngint(cuint(sym)))
		if( hashLookup(@c_debug_local_reads, strptr(key)) <> NULL ) then return ""
	end if
	'' Keep the declaration and every assignment for source debugging. sizeof
	'' a storage address is constant and never evaluates an uninitialized value.
	'' An enum remains a declaration in GNU11 as well as C23, and adds no line
	'' to the existing source mapping. No native diagnostic option is changed.
	c_debug_local_id += 1
	return " enum { fb_c_debug_local_" + str(c_debug_local_id) + " = sizeof(&" + identifier + ") };"
end function

'' end of backend/c/c-debug-locals.bi
