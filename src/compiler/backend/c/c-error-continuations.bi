'' Project: FreeBASIC C backend continuation lowering
'' File: backend/c/c-error-continuations.bi
'' Purpose: Return ERROR continuations to their live C activation.
'' Responsibilities: Own local token names, failure checkpoints and native handoff.
'' This file intentionally does NOT infer cleanup for opaque or foreign frames.

declare function irhlcErrorNeedsCleanupBoundary(byval body as ASTNODE ptr) as integer

type CRESUMETARGET
	identifier as zstring ptr
end type

enum CRESUMEAPI
	CRESUME_ENTER
	CRESUME_TARGET
	CRESUME_POP
	CRESUME_CHECKPOINT
	CRESUME_NATIVEDISPATCH
	CRESUME_APIS
end enum

'' Emission is sequential. The target pool belongs to one ordinary procedure
'' or outlined callback; the referenced compiler symbols remain borrowed.
dim shared as TLIST c_resume_targets
dim shared as THASH c_resume_names
dim shared as integer c_resume_ready, c_resume_enabled, c_resume_has_throw
dim shared as uinteger c_resume_declaration_offset
dim shared as integer c_resume_pending, c_resume_local_checkpoint, c_resume_native_pending
dim shared as string c_resume_retry_label, c_resume_next_label
dim shared as FBSYMBOL ptr c_resume_apis(0 to CRESUME_APIS - 1)

private sub hCResumeFinish()
	if( c_resume_ready = FALSE ) then exit sub
	hashEnd(@c_resume_names)
	var entry = cptr(CRESUMETARGET ptr, listGetHead(@c_resume_targets))
	while( entry <> NULL )
		ZstrFree(entry->identifier)
		entry = listGetNext(entry)
	wend
	listEnd(@c_resume_targets)
	c_resume_ready = FALSE
	c_resume_enabled = FALSE
	c_resume_has_throw = FALSE
end sub

private function hCResumeIsThrow(byval proc as FBSYMBOL ptr) as integer
	if( proc = NULL orelse symbGetIsRTL(proc) = FALSE ) then return FALSE
	var identifier = *symbGetMangledName(proc)
	return identifier = FB_RTL_ERRORTHROW or identifier = FB_RTL_ERRORTHROWEX
end function

'' These helpers return NULL on success and the selected handler on failure.
'' Their runtime ERROR call supplies no retry/next labels, so they need the
'' native transfer path without creating a resumable statement checkpoint.
private function hCResumeIsCheck(byval proc as FBSYMBOL ptr) as integer
	if( proc = NULL orelse symbGetIsRTL(proc) = FALSE ) then return FALSE
	select case *symbGetMangledName(proc)
	case FB_RTL_ARRAYSNGBOUNDCHK, FB_RTL_ARRAYBOUNDCHK, FB_RTL_ARRAYDIMENSIONCHK, FB_RTL_NULLPTRCHK
		return TRUE
	end select
	return FALSE
end function

private function hCResumeIsResume(byval proc as FBSYMBOL ptr) as integer
	if( proc = NULL orelse symbGetIsRTL(proc) = FALSE ) then return FALSE
	var identifier = *symbGetMangledName(proc)
	return identifier = "fb_ErrorResume" or identifier = "fb_ErrorResumeNext"
end function

private function hCResumeTreeThrows(byval body as ASTNODE ptr) as integer
	type PENDINGNODE
		value as ASTNODE ptr
	end type
	dim pending as TLIST
	listInit(@pending, 32, sizeof(PENDINGNODE), LIST_FLAGS_CLEAR)
	dim found as integer = FALSE
	while( body <> NULL andalso found = FALSE )
		var first = cptr(PENDINGNODE ptr, listNewNode(@pending))
		first->value = body
		while( listGetTail(@pending) <> NULL )
			var slot = cptr(PENDINGNODE ptr, listGetTail(@pending))
			var n = slot->value
			listDelNode(@pending, slot)
			if( n->class = AST_NODECLASS_CALL andalso hCResumeIsThrow(n->sym) ) then
				found = TRUE
				exit while
			end if
			if( n->l <> NULL ) then
				slot = listNewNode(@pending)
				slot->value = n->l
			end if
			if( n->r <> NULL ) then
				slot = listNewNode(@pending)
				slot->value = n->r
			end if
		wend
		body = body->next
	wend
	listEnd(@pending)
	return found
end function

private sub hCResumeBegin(byval body as ASTNODE ptr)
	hCResumeFinish()
	listInit(@c_resume_targets, 16, sizeof(CRESUMETARGET), LIST_FLAGS_CLEAR)
	hashInit(@c_resume_names, 64, FALSE)
	c_resume_ready = TRUE
	c_resume_pending = FALSE
	c_resume_native_pending = FALSE
	c_resume_declaration_offset = 0
	c_resume_enabled = (env.clopt.target = FB_COMPTARGET_WIN32) andalso _
		(fbGetCpuFamily() = FB_CPUFAMILY_X86_64) andalso (ctx.computedgotoopaque = FALSE)
	if( c_resume_enabled ) then c_resume_has_throw = hCResumeTreeThrows(body)
end sub

private function hCResumeAPI(byval which as CRESUMEAPI) as FBSYMBOL ptr
	dim api as FBSYMBOL ptr
	select case which
	case CRESUME_ENTER: api = rtlProcLookup(@FB_RTL_ERRORCRESUMEENTER, FB_RTL_IDX_ERRORCRESUMEENTER)
	case CRESUME_TARGET: api = rtlProcLookup(@FB_RTL_ERRORCRESUMETARGET, FB_RTL_IDX_ERRORCRESUMETARGET)
	case CRESUME_POP: api = rtlProcLookup(@FB_RTL_ERRORCRESUMEPOP, FB_RTL_IDX_ERRORCRESUMEPOP)
	case CRESUME_CHECKPOINT: api = rtlProcLookup(@FB_RTL_ERRORCRESUMECHECKPOINT, FB_RTL_IDX_ERRORCRESUMECHECKPOINT)
	case CRESUME_NATIVEDISPATCH: api = rtlProcLookup(@FB_RTL_ERRORNATIVEDISPATCH, FB_RTL_IDX_ERRORNATIVEDISPATCH)
	end select
	if( hCErrKeepAPI(api) = FALSE ) then return NULL
	c_resume_apis(which) = api
	return api
end function

private function hCResumeName(byval which as CRESUMEAPI) as string
	var api = hCResumeAPI(which)
	if( api = NULL ) then return ""
	return hGetCId(api)
end function

private function hCResumeNeedsCaptures() as integer
	return c_resume_enabled andalso c_resume_has_throw andalso env.clopt.resumeerr andalso _
		(astGetProc()->block.proc.ismain = FALSE)
end function

private function hCResumeAddress(byval target as FBSYMBOL ptr) as string
	var identifier = hGetCId(target)
	if( c_resume_ready = FALSE or sectionInsideProc() = FALSE ) then return "&&" + identifier
	var entry = cptr(CRESUMETARGET ptr, hashLookup(@c_resume_names, strptr(identifier)))
	if( entry = NULL ) then
		entry = listNewNode(@c_resume_targets)
		entry->identifier = ZstrDup(strptr(identifier))
		hashAdd(@c_resume_names, entry->identifier, entry, hashHash(entry->identifier))
	end if
	return "fb_c_label_address(" + identifier + ")"
end function

'' Project: FreeBASIC C continuation lowering
'' File: local-dispatch.bi.inc
'' Purpose: Keep known ERROR transfers inside the source statement's live C scope.
'' Responsibilities: Own one failure block and return through its actual retry/next labels.
'' This file intentionally does NOT route known continuations through a function header.

private function hCResumeTargetLabel(byref value as const string) as string
	const PREFIX = "fb_c_label_address("
	var first = instr(value, PREFIX)
	if( first = 0 ) then return ""
	first += len(PREFIX)
	var last = instr(first, value, ")")
	if( last = 0 ) then return ""
	return mid(value, first, last - first)
end function

private sub hCResumeLocalChoices(byval release_buffer as integer)
	for choice as integer = 0 to 1
		var label = iif(choice = 0, c_resume_retry_label, c_resume_next_label)
		if( len(label) = 0 ) then continue for
		hWriteLine("if (fb_c_error_target == (const void*)fb_c_label_address(" + label + ")) {")
		if( release_buffer ) then
			hWriteLine("if (" + hCResumeName(CRESUME_POP) + "(fb_c_resume_buffer) != 0) fb_End(" + str(FB_RTERROR_ILLEGALRESUME) + ");")
		end if
		hWriteLine("goto " + label + "; }")
	next
end sub

private sub hCResumeBeforeThrow(byref retry as const string, byref following as const string)
	if( c_resume_enabled = FALSE ) then exit sub
	assert(c_resume_pending = FALSE)
	c_resume_retry_label = hCResumeTargetLabel(retry)
	c_resume_next_label = hCResumeTargetLabel(following)
	if( (len(c_resume_retry_label) = 0) <> (len(c_resume_next_label) = 0) ) then
		'' Keep the existing representation for a partial continuation contract.
		c_resume_enabled = FALSE
		exit sub
	end if
	c_resume_pending = TRUE
	c_resume_local_checkpoint = len(c_resume_retry_label) > 0 andalso _
		(astGetProc()->block.proc.ismain = FALSE or errorhelper)
	if( c_resume_local_checkpoint ) then
		if( errorplan = NULL orelse errorplan->heap = FALSE ) then
			errReportEx(FB_ERRMSG_INTERNAL, "C resumable checkpoint without stable captures")
			exit sub
		end if
	end if
	hCErrKeepAPI(rtlProcLookup(@FB_RTL_END, FB_RTL_IDX_END))
	hWriteLine("{")
	hWriteLine("void *fb_c_error_target;")
	if( c_resume_local_checkpoint ) then
		var owner = "((void*)0)"
		if( errorplan <> NULL ) then owner = "((void*)&fb_error_capture->frame)"
		hWriteLine("void *const fb_c_resume_buffer = " + hCResumeName(CRESUME_ENTER) + "(" + owner + ", " + retry + ", " + following + ");")
		hWriteLine("if (fb_c_resume_buffer == 0) fb_End(" + str(FB_RTERROR_OUTOFMEM) + ");")
		'' The failure block remains live while the native handler runs.
		'' Its constant buffer pointer is unchanged across the checkpoint.
		hWriteLine("if (" + hCResumeName(CRESUME_CHECKPOINT) + "(fb_c_resume_buffer, (void*)0) != 0) {")
		hWriteLine("fb_c_error_target = " + hCResumeName(CRESUME_TARGET) + "(fb_c_resume_buffer);")
		hCResumeLocalChoices(TRUE)
		hWriteLine("fb_End(" + str(FB_RTERROR_ILLEGALRESUME) + "); }")
	end if
end sub

private sub hCResumeLocalJump(byref target as const string)
	assert(c_resume_pending)
	hWriteLine("fb_c_error_target = " + target + ";")
	hCResumeLocalChoices(c_resume_local_checkpoint)
	hWriteLine(hCResumeName(CRESUME_NATIVEDISPATCH) + "(fb_c_error_target);")
	hWriteLine("__builtin_unreachable();")
	hWriteLine("}")
	c_resume_pending = FALSE
end sub

'' end of local-dispatch.bi.inc

private sub hCResumeDeclarationStart()
	assert(ctx.section = 2)
	assert(ctx.sections(2).old = FALSE)
	c_resume_declaration_offset = ctx.sections(2).bufferlen
end sub

private sub hCResumeCloseBody()
	var saved = sectionGosub(2)
	dim as string declarations
	if( listGetHead(@c_resume_targets) <> NULL ) then
		if( c_resume_enabled ) then
			hWriteLine("#define fb_c_label_address(label) (&fb_c_token_##label)", TRUE)
			var entry = cptr(CRESUMETARGET ptr, listGetHead(@c_resume_targets))
			while( entry <> NULL )
				declarations += chr(9) + "static unsigned char fb_c_token_" + *entry->identifier + ";" + NEWLINE
				entry = listGetNext(entry)
			wend
		else
			hWriteLine("#define fb_c_label_address(label) (&&label)", TRUE)
		end if
	end if
	'' ERROR transfers now leave their failure block through local choices
	'' or the typed runtime handoff. No path may revive the old header jump.
	assert(ctx.computedgotodispatch = FALSE)
	if( len(declarations) > 0 ) then
		'' Other header emitters can append initialization statements. Insert
		'' these late-discovered declarations at the header's beginning while
		'' retaining the dispatch block after all original prologue effects.
		var header = @ctx.sections(2)
		var previous_bytes = header->bufferlen
		if( c_resume_declaration_offset = 0 or c_resume_declaration_offset > previous_bytes ) then
			errReportEx(FB_ERRMSG_INTERNAL, "C resume declaration header boundary")
			exit sub
		end if
		var added_bytes = cuint(len(declarations))
		sectionAppend(header, strptr(declarations), added_bytes)
		if( header->bufferlen < previous_bytes orelse header->bufferlen - previous_bytes <> added_bytes ) then
			errReportEx(FB_ERRMSG_INTERNAL, "C resume declaration insertion")
			exit sub
		end if
		var tail_bytes = previous_bytes - c_resume_declaration_offset
		memmove(header->buffer + c_resume_declaration_offset + added_bytes, header->buffer + c_resume_declaration_offset, tail_bytes)
		memcpy(header->buffer + c_resume_declaration_offset, strptr(declarations), added_bytes)
	end if
	sectionReturn(saved)
end sub

private sub hCResumeEnd()
	if( listGetHead(@c_resume_targets) <> NULL ) then hWriteLine("#undef fb_c_label_address", TRUE)
	hCResumeFinish()
end sub

'' end of backend/c/c-error-continuations.bi
