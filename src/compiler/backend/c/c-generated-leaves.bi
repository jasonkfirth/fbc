/'
	Project: FreeBASIC C backend
	File: c-generated-leaves.bi
	Purpose: Omit unreferenced compiler-generated leaf procedures from C.
	Responsibilities: Own per-output C-name records, observe actual references,
	retain conservative roots, and compact checked prototype/body spans.
	This file does not change BASIC access flags, ABI, or source diagnostics.
'/

type CPROCLEAF
	name as zstring ptr
	referenced as integer
	must_keep as integer
	body_first as uinteger
	body_last as uinteger
	proto_first as uinteger
	proto_last as uinteger
	body_next as CPROCLEAF ptr
	proto_next as CPROCLEAF ptr
end type

dim shared as TLIST c_leaf_nodes
dim shared as THASH c_leaf_names
dim shared as integer c_leaf_ready, c_leaf_opaque
dim shared as CPROCLEAF ptr c_leaf_current, c_leaf_bodies, c_leaf_body_tail
dim shared as CPROCLEAF ptr c_leaf_protos, c_leaf_proto_tail

private sub hCLeafFinish( )
	if( c_leaf_ready = FALSE ) then exit sub
	hashEnd(@c_leaf_names)
	var entry = cptr(CPROCLEAF ptr, listGetHead(@c_leaf_nodes))
	while( entry <> NULL )
		ZstrFree(entry->name)
		entry = listGetNext(entry)
	wend
	listEnd(@c_leaf_nodes)
	c_leaf_current = NULL
	c_leaf_bodies = NULL
	c_leaf_body_tail = NULL
	c_leaf_protos = NULL
	c_leaf_proto_tail = NULL
	c_leaf_ready = FALSE
end sub

private sub hCLeafInit( )
	hCLeafFinish( )
	listInit(@c_leaf_nodes, 32, sizeof(CPROCLEAF), LIST_FLAGS_CLEAR) '' Match expression-pool block sizing.
	hashInit(@c_leaf_names, 128, FALSE) '' Chained buckets do not limit procedure count.
	c_leaf_ready = TRUE
	c_leaf_opaque = FALSE
end sub

private function hCLeafGet( byval proc as FBSYMBOL ptr ) as CPROCLEAF ptr
	if( symbIsPrivate(proc) = FALSE ) then return NULL
	var identifier = hGetCId(proc)
	if( len(identifier) = 0 ) then return NULL
	var entry = cptr(CPROCLEAF ptr, hashLookup(@c_leaf_names, strptr(identifier)))
	if( entry = NULL ) then
		entry = listNewNode(@c_leaf_nodes)
		entry->name = ZstrDup(strptr(identifier))
		hashAdd(@c_leaf_names, entry->name, entry, hashHash(entry->name))
		'' Explicit procedures retain their established emission behavior.
		'' Only compiler-generated internal bodies can be omitted here.
		entry->must_keep = ((proc->attrib and FB_SYMBATTRIB_INTERNAL) = 0) or _
			symbGetIsGlobalCtor(proc) or symbGetIsGlobalDtor(proc) or _
			symbGetIsModLevelProc(proc) or symbIsExport(proc) or symbIsVirtual(proc)
	end if
	return entry
end function

private sub hCLeafReference( byval sym as FBSYMBOL ptr )
	if( c_leaf_ready = FALSE ) then exit sub
	if( symbIsProc(sym) ) then
		if( symbIsPrivate(sym) ) then
			var entry = hCLeafGet(sym)
			if( entry <> NULL ) then entry->referenced = TRUE
			'' Removing a caller must not strand another private procedure.
			'' Keep callers with private outgoing calls or addresses, even
			'' when that caller itself has no surviving C reference.
			if( c_leaf_current <> NULL ) then c_leaf_current->must_keep = TRUE
		end if
	elseif( symbIsVar(sym) andalso (symbGetIsLiteral(sym) = FALSE) ) then
		if( (symbIsLocal(sym) = FALSE) or symbIsStatic(sym) ) then
			'' Global and hoisted static declarations remain in the output.
			'' Retain their users so omission cannot create unused storage.
			if( c_leaf_current <> NULL ) then c_leaf_current->must_keep = TRUE
		end if
	end if
end sub

private sub hCLeafBegin( byval proc as FBSYMBOL ptr )
	c_leaf_current = NULL
	if( symbIsNaked(proc) ) then
		c_leaf_opaque = TRUE
		exit sub
	end if
	var entry = hCLeafGet(proc)
	if( entry = NULL ) then exit sub
	assert(entry->body_last = 0)
	entry->body_first = ctx.sections(1).bufferlen
	if( c_leaf_body_tail <> NULL ) then
		c_leaf_body_tail->body_next = entry
	else
		c_leaf_bodies = entry
	end if
	c_leaf_body_tail = entry
	c_leaf_current = entry
end sub

private sub hCLeafEnd( )
	if( c_leaf_current <> NULL ) then c_leaf_current->body_last = ctx.sections(1).bufferlen
	c_leaf_current = NULL
end sub

private sub hCLeafPrototype( byval proc as FBSYMBOL ptr, byval first as uinteger )
	var entry = hCLeafGet(proc)
	if( entry = NULL orelse entry->body_last = 0 ) then exit sub
	assert(entry->proto_last = 0)
	entry->proto_first = first
	entry->proto_last = ctx.sections(0).bufferlen
	if( c_leaf_proto_tail <> NULL ) then
		c_leaf_proto_tail->proto_next = entry
	else
		c_leaf_protos = entry
	end if
	c_leaf_proto_tail = entry
end sub

private function hCLeafRangesValid( byval is_header as integer, byref section_entry as const SECTIONENTRY ) as integer
	var entry = iif(is_header, c_leaf_protos, c_leaf_bodies)
	dim as uinteger previous_last = 0
	while( entry <> NULL )
		var first = iif(is_header, entry->proto_first, entry->body_first)
		var last = iif(is_header, entry->proto_last, entry->body_last)
		if( (first < previous_last) or (last < first) or _
			(last > section_entry.bufferlen) ) then return FALSE
		previous_last = last
		entry = iif(is_header, entry->proto_next, entry->body_next)
	wend
	return TRUE
end function

private sub hCLeafCompact( byval is_header as integer, byref section_entry as SECTIONENTRY )
	var entry = iif(is_header, c_leaf_protos, c_leaf_bodies)
	dim as uinteger read_position = 0, write_position = 0
	while( entry <> NULL )
		if( (entry->referenced = FALSE) and (entry->must_keep = FALSE) ) then
			var first = iif(is_header, entry->proto_first, entry->body_first)
			var last = iif(is_header, entry->proto_last, entry->body_last)
			var bytes = first - read_position
			if( bytes > 0 ) then memmove(section_entry.buffer + write_position, section_entry.buffer + read_position, bytes)
			write_position += bytes
			read_position = last
		end if
		entry = iif(is_header, entry->proto_next, entry->body_next)
	wend
	var bytes = section_entry.bufferlen - read_position
	if( bytes > 0 ) then memmove(section_entry.buffer + write_position, section_entry.buffer + read_position, bytes)
	section_entry.bufferlen = write_position + bytes
end sub

private sub hCLeafPrune( )
	'' Opaque assembly can name procedures without typed IR references.
	'' Callback adapters and conversion builtins have separately owned bodies;
	'' keep these modules intact until their dependencies are modeled here.
	'' Debug line directives also depend on the original output ordering.
	if( c_leaf_opaque or env.clopt.debuginfo or ((ctx.usedbuiltins and (not cuint(BUILTIN_STATICASSERT))) <> 0) or _
		(listGetHead(@ctx.arraycallbacks) <> NULL) ) then exit sub
	if( (hCLeafRangesValid(TRUE, ctx.sections(0)) = FALSE) or (hCLeafRangesValid(FALSE, ctx.sections(1)) = FALSE) ) then
		errReportEx(FB_ERRMSG_INTERNAL, __FUNCTION__)
		exit sub
	end if
	'' Each byte is moved at most once. Buffers retain their allocated capacity.
	hCLeafCompact(TRUE, ctx.sections(0))
	hCLeafCompact(FALSE, ctx.sections(1))
end sub

'' end of backend/c/c-generated-leaves.bi
