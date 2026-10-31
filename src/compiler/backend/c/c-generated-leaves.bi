/'
	Project: FreeBASIC C backend
	File: backend/c/c-generated-leaves.bi
	Purpose: Omit unreachable private procedure bodies and their C declarations.
	Responsibilities: Own per-output C-name records, observe actual references,
	trace private and adapter dependencies, retain roots, and compact checked spans.
	This file intentionally does NOT contain: BASIC access flag, ABI or source diagnostic changes.
'/

type CPROCLEAFEDGE as CPROCLEAFEDGE_
type CPROCLEAFSPAN as CPROCLEAFSPAN_

type CPROCLEAF
	name as zstring ptr
	referenced as integer
	must_keep as integer
	body_span as CPROCLEAFSPAN ptr
	dependencies as CPROCLEAFEDGE ptr
	work_next as CPROCLEAF ptr
end type

type CPROCLEAFEDGE_
	target as CPROCLEAF ptr
	next_edge as CPROCLEAFEDGE ptr
end type

type CPROCLEAFSPAN_
	owner as CPROCLEAF ptr
	first as uinteger
	last as uinteger
	next_span as CPROCLEAFSPAN ptr
end type

dim shared as TLIST c_leaf_nodes, c_leaf_edges, c_leaf_spans
dim shared as THASH c_leaf_names
dim shared as integer c_leaf_ready, c_leaf_opaque
dim shared as CPROCLEAF ptr c_leaf_current
dim shared as CPROCLEAFSPAN ptr c_leaf_bodies, c_leaf_body_tail
dim shared as CPROCLEAFSPAN ptr c_leaf_protos, c_leaf_proto_tail

#include once "backend/c/c-generated-storage.bi"

private sub hCLeafFinish( )
	hCGlobalStorageFinish()
	if( c_leaf_ready = FALSE ) then exit sub
	listEnd(@c_leaf_spans)
	listEnd(@c_leaf_edges)
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
	listInit(@c_leaf_edges, 32, sizeof(CPROCLEAFEDGE), LIST_FLAGS_CLEAR)
	listInit(@c_leaf_spans, 32, sizeof(CPROCLEAFSPAN), LIST_FLAGS_CLEAR)
	hashInit(@c_leaf_names, 128, FALSE) '' Chained buckets do not limit procedure count.
	c_leaf_ready = TRUE
	c_leaf_opaque = FALSE
	hCGlobalStorageInit()
end sub

private function hCLeafGetName( byref identifier as string ) as CPROCLEAF ptr
	if( c_leaf_ready = FALSE ) then return NULL
	if( len(identifier) = 0 ) then return NULL
	var entry = cptr(CPROCLEAF ptr, hashLookup(@c_leaf_names, strptr(identifier)))
	if( entry = NULL ) then
		entry = listNewNode(@c_leaf_nodes)
		entry->name = ZstrDup(strptr(identifier))
		hashAdd(@c_leaf_names, entry->name, entry, hashHash(entry->name))
	end if
	return entry
end function

private function hCLeafGet( byval proc as FBSYMBOL ptr ) as CPROCLEAF ptr
	if( symbIsPrivate(proc) = FALSE ) then return NULL
	var identifier = hGetCId(proc)
	var entry = hCLeafGetName(identifier)
	if( entry = NULL ) then return NULL
	'' Private visibility supplies no external C entry point. Startup,
	'' exported and virtual entries still have callers outside this graph.
	entry->must_keep or= symbGetIsGlobalCtor(proc) or symbGetIsGlobalDtor(proc) or _
		symbGetIsModLevelProc(proc) or symbIsExport(proc) or symbIsVirtual(proc)
	return entry
end function

private sub hCLeafLink( byval owner as CPROCLEAF ptr, byval target as CPROCLEAF ptr )
	if( target = NULL ) then exit sub
	if( owner = NULL ) then
		'' Public bodies and global initializers are roots outside private spans.
		target->referenced = TRUE
		exit sub
	end if
	if( target = owner ) then exit sub
	var edge = owner->dependencies
	while( edge <> NULL )
		if( edge->target = target ) then exit sub
		edge = edge->next_edge
	wend
	edge = listNewNode(@c_leaf_edges)
	edge->target = target
	edge->next_edge = owner->dependencies
	owner->dependencies = edge
end sub

private sub hCLeafDependency( byval target as CPROCLEAF ptr )
	hCLeafLink(c_leaf_current, target)
end sub

'' The output owns every node and edge until pruning finishes. Edges borrow
'' pooled nodes, never FBSYMBOL pointers that may expire during IR flushing.
private sub hCLeafReachable( )
	dim as CPROCLEAF ptr first = NULL, last = NULL
	var entry = cptr(CPROCLEAF ptr, listGetHead(@c_leaf_nodes))
	while( entry <> NULL )
		entry->work_next = NULL
		if( entry->referenced or entry->must_keep ) then
			entry->referenced = TRUE
			if( last <> NULL ) then
				last->work_next = entry
			else
				first = entry
			end if
			last = entry
		end if
		entry = listGetNext(entry)
	wend
	'' Each reachable node enters this queue once, including cycles. The pool
	'' supplies the links, so traversal needs neither recursion nor allocation.
	while( first <> NULL )
		entry = first
		first = first->work_next
		if( first = NULL ) then last = NULL
		var edge = entry->dependencies
		while( edge <> NULL )
			var target = edge->target
			if( target->referenced = FALSE ) then
				target->referenced = TRUE
				target->work_next = NULL
				if( last <> NULL ) then
					last->work_next = target
				else
					first = target
				end if
				last = target
			end if
			edge = edge->next_edge
		wend
	wend
end sub

private function hCLeafNeeded(byval proc as FBSYMBOL ptr) as integer
	if( c_leaf_ready = FALSE or c_leaf_opaque or env.clopt.debuginfo ) then return TRUE
	var entry = hCLeafGet(proc)
	if( entry = NULL ) then return TRUE
	return entry->referenced or entry->must_keep
end function

private sub hCLeafReference( byval sym as FBSYMBOL ptr )
	if( c_leaf_ready = FALSE ) then exit sub
	if( symbIsProc(sym) ) then
		if( symbIsPrivate(sym) ) then
			var entry = hCLeafGet(sym)
			if( entry <> NULL ) then hCLeafDependency(entry)
		end if
	elseif( symbIsVar(sym) andalso (symbGetIsLiteral(sym) = FALSE) ) then
		if( (symbIsLocal(sym) = FALSE) or symbIsStatic(sym) ) then
			hCGlobalStorageReference(sym)
			'' Global and hoisted static declarations remain in the output.
			'' Retain their users so omission cannot create unused storage.
			if( c_leaf_current <> NULL ) then c_leaf_current->must_keep = TRUE
		end if
	end if
end sub

private sub hCLeafNativeReference( byref identifier as string )
	hCLeafDependency(hCLeafGetName(identifier))
end sub

private sub hCLeafCallback( byval target as FBSYMBOL ptr, byref identifier as string )
	if( c_leaf_ready = FALSE ) then exit sub
	var adapter = hCLeafGetName(identifier)
	'' A typed adapter belongs to its callers, not to the output as a whole.
	'' Its original target is retained only when the adapter is reachable.
	hCLeafDependency(adapter)
	if( symbIsPrivate(target) ) then hCLeafLink(adapter, hCLeafGet(target))
end sub

private function hCLeafSpan(byval entry as CPROCLEAF ptr, byval is_header as integer, _
	byval first as uinteger, byval last as uinteger) as CPROCLEAFSPAN ptr
	var span = cptr(CPROCLEAFSPAN ptr, listNewNode(@c_leaf_spans))
	span->owner = entry
	span->first = first
	span->last = last
	if( is_header ) then
		if( c_leaf_proto_tail <> NULL ) then
			c_leaf_proto_tail->next_span = span
		else
			c_leaf_protos = span
		end if
		c_leaf_proto_tail = span
	else
		if( c_leaf_body_tail <> NULL ) then
			c_leaf_body_tail->next_span = span
		else
			c_leaf_bodies = span
		end if
		c_leaf_body_tail = span
	end if
	return span
end function

private sub hCLeafNativeSpan(byref identifier as string, byval is_header as integer, _
	byval first as uinteger, byval last as uinteger)
	var entry = hCLeafGetName(identifier)
	if( entry = NULL ) then exit sub
	var span = hCLeafSpan(entry, is_header, first, last)
	if( is_header = FALSE ) then
		assert(entry->body_span = NULL)
		entry->body_span = span
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
	assert(entry->body_span = NULL)
	entry->body_span = hCLeafSpan(entry, FALSE, ctx.sections(1).bufferlen, 0)
	c_leaf_current = entry
end sub

private sub hCLeafEnd( )
	if( c_leaf_current <> NULL ) then c_leaf_current->body_span->last = ctx.sections(1).bufferlen
	c_leaf_current = NULL
end sub

private sub hCLeafAuxPrototype(byval proc as FBSYMBOL ptr, byval first as uinteger)
	var entry = hCLeafGet(proc)
	if( entry = NULL ) then exit sub
	'' A read-only implementation can be declared before its body. Every
	'' declaration owns a separate ordered span, including the later ABI one.
	hCLeafSpan(entry, TRUE, first, ctx.sections(0).bufferlen)
end sub

private sub hCLeafPrototype( byval proc as FBSYMBOL ptr, byval first as uinteger )
	var entry = hCLeafGet(proc)
	if( entry = NULL orelse entry->body_span = NULL ) then exit sub
	hCLeafAuxPrototype(proc, first)
end sub

private function hCLeafRangesValid( byval is_header as integer, byref section_entry as const SECTIONENTRY ) as integer
	var span = iif(is_header, c_leaf_protos, c_leaf_bodies)
	dim as uinteger previous_last = 0
	while( span <> NULL )
		var first = span->first
		var last = span->last
		if( (first < previous_last) or (last < first) or _
			(last > section_entry.bufferlen) ) then return FALSE
		previous_last = last
		span = span->next_span
	wend
	return TRUE
end function

private sub hCLeafCompact( byval is_header as integer, byref section_entry as SECTIONENTRY )
	var span = iif(is_header, c_leaf_protos, c_leaf_bodies)
	dim as uinteger read_position = 0, write_position = 0
	while( span <> NULL )
		var entry = span->owner
		if( (entry->referenced = FALSE) and (entry->must_keep = FALSE) ) then
			var first = span->first
			var last = span->last
			var bytes = first - read_position
			if( bytes > 0 ) then memmove(section_entry.buffer + write_position, section_entry.buffer + read_position, bytes)
			write_position += bytes
			read_position = last
		end if
		span = span->next_span
	wend
	var bytes = section_entry.bufferlen - read_position
	if( bytes > 0 ) then memmove(section_entry.buffer + write_position, section_entry.buffer + read_position, bytes)
	section_entry.bufferlen = write_position + bytes
end sub

private sub hCLeafPrune( )
	'' Opaque assembly can name procedures without typed IR references.
	'' Adapter declarations and bodies have the same checked spans as BASIC
	'' procedures. Reachability retains shared adapters and their typed targets.
	'' Numeric conversion helpers use reserved names and only system builtins;
	'' they cannot refer to a private BASIC procedure.
	'' Debug line directives still depend on the original output ordering.
	if( c_leaf_opaque or env.clopt.debuginfo ) then exit sub
	if( (hCLeafRangesValid(TRUE, ctx.sections(0)) = FALSE) or (hCLeafRangesValid(FALSE, ctx.sections(1)) = FALSE) ) then
		errReportEx(FB_ERRMSG_INTERNAL, __FUNCTION__)
		exit sub
	end if
	'' Reachability was closed after global initializers and before deferred
	'' private ABI wrappers. Unreachable wrappers were never emitted.
	'' Each byte is moved at most once. Buffers retain their allocated capacity.
	hCLeafCompact(TRUE, ctx.sections(0))
	hCLeafCompact(FALSE, ctx.sections(1))
end sub

'' end of backend/c/c-generated-leaves.bi
