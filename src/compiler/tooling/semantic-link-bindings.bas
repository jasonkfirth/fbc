'' Project: FreeBASIC compiler - native link bindings
'' File: tooling/semantic-link-bindings.bas
'' Purpose: Retain source revisions and accepted procedure identities for linking.
'' Responsibilities: Native stream checks, scalar snapshots and pointer lifetimes.
'' This file intentionally does NOT contain: symbol resolution or declaration parsing.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-link.bi"
#include once "tooling/semantic-source-file.bi"
#include once "support/containers/hash.bi"
#include once "file.bi"

'' -------------------------------------------------------------------------
'' Invocation-owned source revisions
'' -------------------------------------------------------------------------

private const LINK_BINDING_MAX_SOURCES = 5000
private const LINK_BINDING_MAX_PROCEDURES = 100000
private const LINK_BINDING_MAX_BYTES = 67108864
private const LINK_BINDING_MAX_SYMBOL_BYTES = 4096
private const LINK_BINDING_WORK_LIMIT = 20000000
private const LINK_BINDING_BUCKETS = 4096
private dim shared as integer binding_module, binding_source_count, binding_procedure_count
private dim shared as integer binding_bytes, binding_live_count, binding_work
private dim shared as any ptr binding_revisions(0 to FB_MAXINCRECLEVEL)
private dim shared as integer binding_sources(0 to FB_MAXINCRECLEVEL)
private dim shared as string binding_filenames(0 to FB_MAXINCRECLEVEL)
private dim shared as integer binding_remapped(0 to FB_MAXINCRECLEVEL)

'' Native procedure addresses are keys only while the symbol is alive. hashAdd
'' borrows the key, so the payload owns its immutable key until hashDel removes
'' it. Release copies a cached backend name before symb frees that allocation.
private type LINK_BINDING_PROCEDURE
	key_text as string
	spelling as string
	filename as string
	module as integer
	identity as integer
	backend as integer
	prototype as integer
	kind_text as string
	physical as integer
	line_number as integer
	column_number as integer
	item as HASHITEM ptr
	index as uinteger
	procedure as FBSYMBOL ptr
	object_observed as integer
	object_symbol as string
end type
private dim shared as THASH binding_procedures

private sub hBindingsClear( )
	if( binding_procedures.list <> NULL ) then
		for index as integer = 0 to binding_procedures.nodes - 1
			dim as HASHITEM ptr item = binding_procedures.list[index].head
			do while( item <> NULL )
				delete cptr(LINK_BINDING_PROCEDURE ptr, item->data)
				item = item->next
			loop
		next
		hashEnd(@binding_procedures)
		binding_procedures.list = NULL
	end if
	binding_live_count = 0
	binding_bytes = 0
end sub

sub fbSemanticLinkBindingsReset( )
	hBindingsClear( )
	binding_module = 0
	binding_source_count = 0
	binding_procedure_count = 0
	binding_work = LINK_BINDING_WORK_LIMIT
	for depth as integer = 0 to FB_MAXINCRECLEVEL
		'' Open revisions must be closed by the source owner while its CRT
		'' stream is live. A restart must never dereference a former stream.
		if( binding_revisions(depth) <> NULL ) then fbSemanticLinkFail( )
		binding_sources(depth) = 0
		binding_filenames(depth) = ""
		binding_remapped(depth) = FALSE
	next
end sub

sub fbSemanticLinkModule( byref filename as const string )
	if( fbSemanticLinkEnabled( ) = FALSE ) then exit sub
	if( (binding_module >= LINK_BINDING_MAX_PROCEDURES) or (binding_live_count <> 0) ) then
		fbSemanticLinkFail( )
		exit sub
	end if
	binding_module += 1
	fbSemanticLinkWrite("M" + TABCHAR + fbSemanticModelNumber(binding_module) + TABCHAR + _
		fbSemanticModelEscape(filename) + TABCHAR + fbSemanticModelEscape(fbGetTargetId()) + _
		TABCHAR + fbSemanticModelNumber(env.clopt.backend))
end sub

sub fbSemanticLinkSourceOpen( byref filename as const string, byval depth as integer )
	if( fbSemanticLinkEnabled( ) = FALSE ) then exit sub
	if( (depth < 0) or (depth > FB_MAXINCRECLEVEL) or (binding_source_count >= LINK_BINDING_MAX_SOURCES) ) then
		fbSemanticLinkFail( )
		exit sub
	end if
	if( binding_revisions(depth) <> NULL ) then
		fbSemanticLinkFail( )
		exit sub
	end if
	fbSemanticLinkProtect(filename)
	dim as zstring * 65 digest
	dim as ulongint bytes = 0
	dim as long status = 0
	dim as any ptr stream = cptr(any ptr, fileattr(env.inf.num, 2))
	binding_revisions(depth) = fbSemanticSourceOpen(stream, @digest, @bytes, @status)
	if( (binding_revisions(depth) = NULL) or (status <> 1) ) then fbSemanticLinkFail( )
	binding_source_count += 1
	binding_sources(depth) = binding_source_count
	binding_filenames(depth) = filename
	binding_remapped(depth) = FALSE
	fbSemanticLinkWrite("FILE" + TABCHAR + fbSemanticModelNumber(binding_source_count) + TABCHAR + _
		fbSemanticModelNumber(binding_module) + TABCHAR + fbSemanticModelEscape(filename) + _
		TABCHAR + digest + TABCHAR + ltrim(str(bytes)))
end sub

sub fbSemanticLinkSourceClose( byval depth as integer )
	if( (depth < 0) or (depth > FB_MAXINCRECLEVEL) ) then exit sub
	if( binding_revisions(depth) = NULL ) then exit sub
	dim as long status = fbSemanticSourceClose(binding_revisions(depth))
	binding_revisions(depth) = NULL
	if( status <> 1 ) then fbSemanticLinkFail( )
	fbSemanticLinkWrite("SRE" + TABCHAR + fbSemanticModelNumber(binding_sources(depth)) + TABCHAR + "verified")
	binding_sources(depth) = 0
	binding_filenames(depth) = ""
	binding_remapped(depth) = FALSE
end sub

sub fbSemanticLinkSourceRemapped( byval depth as integer )
	if( fbSemanticLinkEnabled( ) = FALSE ) then exit sub
	if( (depth < 0) or (depth > FB_MAXINCRECLEVEL) ) then exit sub
	binding_remapped(depth) = TRUE
end sub

'' -------------------------------------------------------------------------
'' Accepted native procedure bindings
'' -------------------------------------------------------------------------

private function hProcedure( byref key_text as const string ) as LINK_BINDING_PROCEDURE ptr
	if( binding_procedures.list = NULL ) then return NULL
	dim as uinteger index = hashHash(strptr(key_text)) mod binding_procedures.nodes
	dim as HASHITEM ptr item = binding_procedures.list[index].head
	do while( item <> NULL )
		if( binding_work <= 0 ) then
			fbSemanticLinkFail( )
			return NULL
		end if
		binding_work -= 1
		if( *item->name = key_text ) then return item->data
		item = item->next
	loop
	return NULL
end function

sub fbSemanticLinkProcedure( byval proc as FBSYMBOL ptr, byref source_point as LEX_LOCATION, _
	byval header_valid as integer, byval token as integer, byval prototype as integer )
	if( (fbSemanticLinkEnabled( ) = FALSE) or (proc = NULL) or (header_valid = FALSE) ) then exit sub
	if( (proc->class <> FB_SYMBCLASS_PROC) or symbGetIsFuncPtr(proc) or (proc->id.name = NULL) ) then exit sub
	dim as string key_text = hex(cuint(proc))
	if( hProcedure(key_text) <> NULL ) then exit sub
	if( fbSemanticLinkEnabled( ) = FALSE ) then exit sub
	if( len(*proc->id.name) > LINK_BINDING_MAX_SYMBOL_BYTES ) then
		fbSemanticLinkFail( )
		exit sub
	end if
	'' #line can replace env.inf.name with a logical filename. Keep the
	'' actual opened-source owner separately; logical names are never FILE
	'' revisions. The source owner closes an include after its parser returns,
	'' so accepted headers still belong to the current opened-source depth.
	dim as string source_filename
	if( (env.includerec >= 0) and (env.includerec <= FB_MAXINCRECLEVEL) ) then
		source_filename = binding_filenames(env.includerec)
	end if
	if( len(source_filename) = 0 ) then
		fbSemanticLinkFail( )
		exit sub
	end if
	dim as integer bytes = len(key_text) + len(*proc->id.name) + len(source_filename)
	if( (binding_procedure_count >= LINK_BINDING_MAX_PROCEDURES) or (bytes >= LINK_BINDING_MAX_BYTES - binding_bytes) ) then
		fbSemanticLinkFail( )
		exit sub
	end if
	if( binding_procedures.list = NULL ) then hashInit(@binding_procedures, LINK_BINDING_BUCKETS)
	dim as LINK_BINDING_PROCEDURE ptr entry = new LINK_BINDING_PROCEDURE
	if( entry = NULL ) then
		fbSemanticLinkFail( )
		exit sub
	end if
	entry->key_text = key_text
	entry->procedure = proc
	entry->spelling = *proc->id.name
	entry->filename = source_filename
	entry->module = binding_module
	binding_procedure_count += 1
	entry->identity = binding_procedure_count
	entry->backend = env.clopt.backend
	entry->prototype = prototype <> FALSE
	select case token
	case FB_TK_SUB: entry->kind_text = "sub"
	case FB_TK_FUNCTION: entry->kind_text = "function"
	case else: entry->kind_text = "other"
	end select
	if( source_point.is_physical and source_point.raw_valid and (binding_remapped(env.includerec) = FALSE) and _
		(source_point.source_file = source_filename) and _
		(source_point.raw_start_line > 0) and (source_point.raw_start_column >= 0) ) then
		entry->physical = TRUE
		entry->line_number = source_point.raw_start_line
		entry->column_number = source_point.raw_start_column
	end if
	entry->index = hashHash(strptr(entry->key_text))
	entry->item = hashAdd(@binding_procedures, strptr(entry->key_text), entry, entry->index)
	binding_live_count += 1
	binding_bytes += bytes
end sub

function fbSemanticLinkProcedureObjectNeeded( byval proc as FBSYMBOL ptr ) as integer
	if( (fbSemanticLinkEnabled( ) = FALSE) or (proc = NULL) ) then return FALSE
	if( (proc->class <> FB_SYMBCLASS_PROC) or symbGetIsFuncPtr(proc) ) then return FALSE
	dim as string key_text = hex(cuint(proc))
	return hProcedure(key_text) <> NULL
end function

'' C/LLVM emitters own the relationship between their textual identifiers
'' and the target object ABI. They supply a selected object name while the
'' native symbol is live. Only the scalar name is retained until release.
sub fbSemanticLinkProcedureObject( byval proc as FBSYMBOL ptr, byref symbol_text as const string )
	if( fbSemanticLinkProcedureObjectNeeded(proc) = FALSE ) then exit sub
	dim as string key_text = hex(cuint(proc))
	dim as LINK_BINDING_PROCEDURE ptr entry = hProcedure(key_text)
	if( entry = NULL ) then exit sub
	if( (len(symbol_text) = 0) or (len(symbol_text) > LINK_BINDING_MAX_SYMBOL_BYTES) or _
		(instr(symbol_text, chr(0)) > 0) ) then
		fbSemanticLinkFail( )
		exit sub
	end if
	if( entry->object_observed ) then
		if( entry->object_symbol <> symbol_text ) then fbSemanticLinkFail( )
		exit sub
	end if
	if( len(symbol_text) >= LINK_BINDING_MAX_BYTES - binding_bytes ) then
		fbSemanticLinkFail( )
		exit sub
	end if
	entry->object_symbol = symbol_text
	entry->object_observed = TRUE
	binding_bytes += len(symbol_text)
end sub

sub fbSemanticLinkProcedureReleased( byval proc as FBSYMBOL ptr )
	if( (binding_procedures.list = NULL) or (proc = NULL) or (proc->class <> FB_SYMBCLASS_PROC) ) then exit sub
	dim as string key_text = hex(cuint(proc))
	dim as LINK_BINDING_PROCEDURE ptr entry = hProcedure(key_text)
	if( entry = NULL ) then exit sub
	dim as string state_text = "unsupported-backend", symbol_text
	select case entry->backend
	case FB_BACKEND_GAS, FB_BACKEND_GAS64
		state_text = "unobserved"
		if( proc->id.mangled <> NULL ) then
			if( len(*proc->id.mangled) > LINK_BINDING_MAX_SYMBOL_BYTES ) then
				fbSemanticLinkFail( )
			else
				symbol_text = *proc->id.mangled
				if( len(symbol_text) > 0 ) then state_text = "observed"
			end if
		end if
	case FB_BACKEND_GCC, FB_BACKEND_CLANG, FB_BACKEND_LLVM
		state_text = "unobserved"
		if( entry->object_observed ) then
			state_text = "observed"
			symbol_text = entry->object_symbol
		end if
	end select
	'' Native calls and procedure addresses set ACCESSED on the selected
	'' symbol. Merely emitting a prototype does not establish a use, so
	'' consumers must retain this distinction when aliases share a name.
	fbSemanticLinkWrite("P" + TABCHAR + fbSemanticModelNumber(entry->identity) + TABCHAR + _
		fbSemanticModelNumber(entry->module) + TABCHAR + entry->kind_text + TABCHAR + _
		iif(entry->prototype, "prototype", "definition") + TABCHAR + fbSemanticModelEscape(entry->spelling) + _
		TABCHAR + state_text + TABCHAR + fbSemanticModelEscape(symbol_text) + TABCHAR + _
		fbSemanticModelEscape(entry->filename) + TABCHAR + fbSemanticModelNumber(abs(entry->physical)) + _
		TABCHAR + fbSemanticModelNumber(entry->line_number) + TABCHAR + fbSemanticModelNumber(entry->column_number) + _
		TABCHAR + fbSemanticModelNumber(abs(symbGetIsAccessed(proc))))
	binding_bytes -= len(entry->key_text) + len(entry->spelling) + len(entry->filename) + len(entry->object_symbol)
	binding_live_count -= 1
	hashDel(@binding_procedures, entry->item, entry->index)
	delete entry
end sub

function fbSemanticLinkBindingsEnd( ) as integer
	'' A rejected compilation exits before fbEnd destroys its symbol table.
	'' These borrows remain live because every native symbol release removes
	'' its entry first. Snapshot them without resuming or tearing down a failed
	'' parser. The invocation outcome still prevents a rejected module from
	'' claiming that a native link was attempted.
	if( binding_procedures.list <> NULL ) then
		for index as integer = 0 to binding_procedures.nodes - 1
			do while( binding_procedures.list[index].head <> NULL )
				dim as LINK_BINDING_PROCEDURE ptr entry = binding_procedures.list[index].head->data
				fbSemanticLinkProcedureReleased(entry->procedure)
				'' Exhausted lookup work must not leave this drain looping.
				if( binding_procedures.list[index].head <> NULL ) then
					if( binding_procedures.list[index].head->data = entry ) then exit do
				end if
			loop
		next
	end if
	dim as integer ok = binding_live_count = 0
	for depth as integer = 0 to FB_MAXINCRECLEVEL
		if( binding_revisions(depth) <> NULL ) then ok = FALSE
	next
	fbSemanticLinkWrite("TOTAL" + TABCHAR + fbSemanticModelNumber(binding_module) + TABCHAR + _
		fbSemanticModelNumber(binding_source_count) + TABCHAR + fbSemanticModelNumber(binding_procedure_count))
	hBindingsClear( )
	return ok
end function

'' end of tooling/semantic-link-bindings.bas
