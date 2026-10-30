/'
	Project: FreeBASIC C backend
	File: backend/c/c-readonly-parameters.bi
	Purpose: Give proven read-only pointer arguments an internal C contract.
	Responsibilities: Check procedure bodies, own variant names and signatures,
	and preserve original entry points through forwarding wrappers.
	This file intentionally does NOT contain: BASIC type changes, ABI changes,
	call-result changes, or assumptions about uninspected procedure bodies.
'/

type CREADONLYPARAM
	param as FBSYMBOL ptr
	next_param as CREADONLYPARAM ptr
end type

type CREADONLYPROC
	proc as FBSYMBOL ptr
	identifier as zstring ptr
	header as DZSTRING
	arguments as DZSTRING
	parameters as CREADONLYPARAM ptr
	private_proc as integer
	returns_void as integer
	external_call as integer
	deferred_wrapper as integer
end type

'' Procedure and PARAM symbols outlive IR flushing. Local parameter variables
'' do not, so the output owns every name needed by the later wrapper emission.
dim shared as TLIST c_readonly_procs, c_readonly_params
dim shared as integer c_readonly_ready

private sub hCReadOnlyFinish()
	if( c_readonly_ready = FALSE ) then exit sub
	var entry = cptr(CREADONLYPROC ptr, listGetHead(@c_readonly_procs))
	while( entry <> NULL )
		ZstrFree(entry->identifier)
		DZstrAllocate(entry->header, 0)
		DZstrAllocate(entry->arguments, 0)
		entry = listGetNext(entry)
	wend
	listEnd(@c_readonly_params)
	listEnd(@c_readonly_procs)
	c_readonly_ready = FALSE
end sub

private sub hCReadOnlyInit()
	hCReadOnlyFinish()
	listInit(@c_readonly_procs, 16, sizeof(CREADONLYPROC), LIST_FLAGS_CLEAR)
	listInit(@c_readonly_params, 16, sizeof(CREADONLYPARAM), LIST_FLAGS_CLEAR)
	c_readonly_ready = TRUE
end sub

private function hCReadOnlyFind(byval proc as FBSYMBOL ptr) as CREADONLYPROC ptr
	var entry = cptr(CREADONLYPROC ptr, listGetHead(@c_readonly_procs))
	while( entry <> NULL )
		if( entry->proc = proc ) then return entry
		entry = listGetNext(entry)
	wend
	return NULL
end function

private function hCReadOnlyParam(byval entry as CREADONLYPROC ptr, byval param as FBSYMBOL ptr) as integer
	if( entry = NULL ) then return FALSE
	var item = entry->parameters
	while( item <> NULL )
		if( item->param = param ) then return TRUE
		item = item->next_param
	wend
	return FALSE
end function

private function hCReadOnlyContains(byval n as ASTNODE ptr, byval storage as FBSYMBOL ptr) as integer
	if( n = NULL ) then return FALSE
	if( n->sym = storage ) then return TRUE
	if( hCReadOnlyContains(n->l, storage) ) then return TRUE
	if( hCReadOnlyContains(n->r, storage) ) then return TRUE
	if( n->class = AST_NODECLASS_CALL ) then
		if( n->call.tmpres = storage ) then return TRUE
		var item = n->call.strtail
		while( item <> NULL )
			if( item->sym = storage orelse hCReadOnlyContains(item->srctree, storage) ) then return TRUE
			item = item->prev
		wend
	end if
	return FALSE
end function

private function hCReadOnlyCheck(byval n as ASTNODE ptr, byval storage as FBSYMBOL ptr, _
	byref seen as integer, byval pointer_read as integer = FALSE) as integer
	if( n = NULL ) then return TRUE
	select case as const n->class
	case AST_NODECLASS_ASM, AST_NODECLASS_MACRO
		'' Opaque text and intrinsic macros can observe frames or hidden storage.
		return FALSE
	case AST_NODECLASS_VAR
		if( n->sym = storage ) then
			seen = TRUE
			return pointer_read
		end if
	case AST_NODECLASS_OFFSET
		if( n->sym = storage ) then return FALSE
	case AST_NODECLASS_ASSIGN, AST_NODECLASS_MEM
		'' Neither the pointer binding nor any addressed storage may be written.
		if( hCReadOnlyContains(n->l, storage) ) then return FALSE
	case AST_NODECLASS_ARG
		dim as integer dtype = FB_DATATYPE_INVALID, read_argument = FALSE
		dim as FBSYMBOL ptr subtype = NULL
		if( n->sym <> NULL andalso symbGetParamMode(n->sym) <> FB_PARAMMODE_VARARG ) then
			symbGetRealParamDtype(n->sym, dtype, subtype)
			var mangled = *symbGetMangledName(symbGetParent(n->sym))
			dim as integer param_index = 0
			var param = symbGetProcLastParam(symbGetParent(n->sym))
			while( param <> NULL andalso param <> n->sym )
				param_index += 1
				param = symbGetProcPrevParam(symbGetParent(n->sym), param)
			wend
			hAdjustCParamType(mangled, param_index, dtype, subtype)
			read_argument = typeGetPtrCnt(dtype) = 1 andalso typeIsConstAt(dtype, 1)
			if( hCReadOnlyParam(hCReadOnlyFind(symbGetParent(n->sym)), n->sym) ) then read_argument = TRUE
		end if
		if( hCReadOnlyCheck(n->l, storage, seen, read_argument) = FALSE ) then return FALSE
		return hCReadOnlyCheck(n->r, storage, seen)
	case AST_NODECLASS_DEREF
		if( hCReadOnlyCheck(n->l, storage, seen, TRUE) = FALSE ) then return FALSE
		return hCReadOnlyCheck(n->r, storage, seen)
	case AST_NODECLASS_ADDROF
		if( hCReadOnlyContains(n, storage) ) then
			'' Taking the pointer variable's address exposes its binding. Only
			'' an address of its pointee may flow to an existing const contract.
			if( pointer_read = FALSE orelse n->l = NULL ) then return FALSE
			if( n->l->class <> AST_NODECLASS_DEREF ) then return FALSE
			return hCReadOnlyCheck(n->l->l, storage, seen, TRUE)
		end if
	case AST_NODECLASS_CONV
		if( pointer_read andalso typeIsPtr(n->dtype) andalso n->l <> NULL andalso typeIsPtr(n->l->dtype) ) then
			return hCReadOnlyCheck(n->l, storage, seen, TRUE)
		end if
	case AST_NODECLASS_CALL
		if( n->call.tmpres = storage ) then return FALSE
		var item = n->call.strtail
		while( item <> NULL )
			if( item->sym = storage orelse hCReadOnlyContains(item->srctree, storage) ) then return FALSE
			item = item->prev
		wend
		if( hCReadOnlyCheck(n->call.profbegin, storage, seen) = FALSE ) then return FALSE
		if( hCReadOnlyCheck(n->call.profend, storage, seen) = FALSE ) then return FALSE
	end select
	if( hCReadOnlyCheck(n->l, storage, seen) = FALSE ) then return FALSE
	return hCReadOnlyCheck(n->r, storage, seen)
end function

private function hCReadOnlyCandidate(byval param as FBSYMBOL ptr, byval body as ASTNODE ptr) as integer
	if( (symbGetParamMode(param) <> FB_PARAMMODE_BYVAL) and _
	    (symbGetParamMode(param) <> FB_PARAMMODE_BYREF) ) then return FALSE
	dim as integer dtype = any
	dim as FBSYMBOL ptr subtype = any
	symbGetRealParamDtype(param, dtype, subtype)
	if( typeGetPtrCnt(dtype) <> 1 orelse typeIsConstAt(dtype, 1) ) then return FALSE
	select case as const typeGetDtOnly(dtype)
	case FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR, FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE
	case else
		return FALSE
	end select
	dim as integer seen = FALSE
	var n = body
	while( n <> NULL )
		if( hCReadOnlyCheck(n, symbGetParamVar(param), seen) = FALSE ) then return FALSE
		n = n->next
	wend
	return seen
end function

private function hCReadOnlyUsed(byval body as ASTNODE ptr, byval storage as FBSYMBOL ptr) as integer
	var n = body
	while( n <> NULL )
		if( hCReadOnlyContains(n, storage) ) then return TRUE
		n = n->next
	wend
	return FALSE
end function

'' A const C contract permits reads without promising that aliased memory is
'' globally immutable. Keep the BASIC signature and the public C entry point;
'' the internal variant owns the body, including its single set of statics.
'' Compiler-generated private bodies retain their existing pruning owner.
private function hCReadOnlyPrepare(byval proc as FBSYMBOL ptr, byval body as ASTNODE ptr) as CREADONLYPROC ptr
	'' Calls in position-independent code may be interposed by another object.
	'' Keep their public symbol as the actual call target.
	if( env.clopt.pic ) then return NULL
	if( env.clopt.debuginfo or symbIsNaked(proc) or symbIsMethod(proc) ) then return NULL
	if( symbIsPrivate(proc) andalso (proc->attrib and FB_SYMBATTRIB_INTERNAL) ) then return NULL
	if( symbProcReturnsOnStack(proc) or hUsesCLongDoubleCABI(proc) ) then return NULL
	if( symbGetIsGlobalCtor(proc) or symbGetIsGlobalDtor(proc) or symbGetIsModLevelProc(proc) ) then return NULL
	var param = symbGetProcLastParam(proc)
	dim as integer candidates = 0
	while( param <> NULL )
		if( symbGetParamMode(param) = FB_PARAMMODE_VARARG ) then return NULL
		if( hCReadOnlyUsed(body, symbGetParamVar(param)) = FALSE ) then return NULL
		if( hCReadOnlyCandidate(param, body) ) then candidates += 1
		param = symbGetProcPrevParam(proc, param)
	wend
	if( candidates = 0 ) then return NULL
	var entry = cptr(CREADONLYPROC ptr, listNewNode(@c_readonly_procs))
	entry->proc = proc
	entry->private_proc = symbIsPrivate(proc)
	entry->returns_void = (symbGetProcRealType(proc) = FB_DATATYPE_VOID)
	var implementation = hCUnalignedTempName() + "_readonly"
	entry->identifier = ZstrDup(strptr(implementation))
	param = symbGetProcLastParam(proc)
	while( param <> NULL )
		if( hCReadOnlyCandidate(param, body) ) then
			var item = cptr(CREADONLYPARAM ptr, listNewNode(@c_readonly_params))
			item->param = param
			item->next_param = entry->parameters
			entry->parameters = item
		end if
		param = symbGetProcPrevParam(proc, param)
	wend
	return entry
end function

private function hCReadOnlyBegin(byval proc as FBSYMBOL ptr, byref header as string) as CREADONLYPROC ptr
	var entry = hCReadOnlyFind(proc)
	if( entry = NULL ) then entry = hCReadOnlyPrepare(proc, astGetProc()->l)
	if( entry = NULL ) then return NULL
	DZstrAssign(entry->header, strptr(header))
	dim as string arguments
	var param = symbGetProcLastParam(proc)
	while( param <> NULL )
		if( len(arguments) ) then arguments += ", "
		arguments += hGetCId(symbGetParamVar(param))
		param = symbGetProcPrevParam(proc, param)
	wend
	DZstrAssign(entry->arguments, strptr(arguments))
	return entry
end function

private sub hCReadOnlyWriteWrapper(byval entry as CREADONLYPROC ptr)
	var header = *entry->header.data
	if( entry->private_proc andalso entry->external_call ) then
		'' Direct calls already use the implementation. This generated ABI
		'' forwarder should inline when an address-taken call permits it.
		'' With no call from another body, keep an ordinary definition so a
		'' source procedure used only by itself retains its unused diagnostic.
		assert(left(header, 7) = "static ")
		if( c_leaf_opaque ) then
			'' An assembly operand may name this entry point without a C use.
			'' Keep its out-of-line ABI symbol even when all visible calls use
			'' the implementation. A real caller established source reachability.
			header = "static __attribute__((used)) " + mid(header, 8)
		else
			header = "static inline " + mid(header, 8)
		end if
	end if
	sectionBegin()
	hWriteLine(header)
	hWriteLine("{")
	sectionIndent()
	var statement = *entry->identifier + "( " + *entry->arguments.data + " );"
	if( entry->returns_void = FALSE ) then statement = "return " + statement
	hWriteLine(statement)
	sectionUnindent()
	hWriteLine("}")
	sectionEnd()
end sub

private sub hCReadOnlyEnd(byval proc as FBSYMBOL ptr)
	var entry = hCReadOnlyFind(proc)
	if( entry = NULL ) then exit sub
	if( entry->private_proc ) then
		'' Later bodies can still call this implementation or take the original
		'' procedure's address. The ordinary prototype keeps that ABI available.
		entry->deferred_wrapper = TRUE
	else
		hCReadOnlyWriteWrapper(entry)
	end if
end sub

private sub hCReadOnlyWritePrivateWrappers()
	var entry = cptr(CREADONLYPROC ptr, listGetHead(@c_readonly_procs))
	while( entry <> NULL )
		if( entry->deferred_wrapper ) then hCReadOnlyWriteWrapper(entry)
		entry = listGetNext(entry)
	wend
end sub

'' end of backend/c/c-readonly-parameters.bi
