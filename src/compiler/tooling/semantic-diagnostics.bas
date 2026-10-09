'' Project: FreeBASIC compiler - structured semantic diagnostics
'' File: tooling/semantic-diagnostics.bas
'' Purpose: Retain diagnostics actually reported by the compiler.
'' Responsibilities: Preserve severity, code, text, source, and parser context.
'' This file intentionally does NOT contain: diagnostic policy or console parsing.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-diagnostics.bi"
#include once "tooling/semantic-output.bi"
#include once "parser/parser.bi"
#include once "diagnostics/error.bi"
#include once "support/containers/hash.bi"

'' -------------------------------------------------------------------------
'' Invocation-owned diagnostic output
'' -------------------------------------------------------------------------

'' A rejected module cannot publish authoritative AST facts. Diagnostics are
'' observations of the compiler's actual checks and can still be complete.
'' This independent writer is bounded and uses the semantic writer's checked
'' staging, source identity protection and final publication.
private const DIAGNOSTIC_MAX_SOURCES = 5000
private const DIAGNOSTIC_MAX_RECORDS = 100000
private const DIAGNOSTIC_MAX_BYTES = 67108864
private dim shared as any ptr diagnostic_output
private dim shared as integer diagnostic_failed, diagnostic_modules
private dim shared as integer diagnostic_count, diagnostic_errors, diagnostic_bytes
private dim shared as integer diagnostic_source_count
private dim shared as string diagnostic_sources(0 to DIAGNOSTIC_MAX_SOURCES - 1)
private dim shared as string diagnostic_context
private dim shared as LEX_LOCATION diagnostic_point
private dim shared as integer diagnostic_has_point
'' The serial module owns these native identities. Only invalid retained
'' headers occupy the array, so ordinary accepted declarations allocate nothing.
'' A new accepted symbol at a reused address removes the former invalid entry.
private dim shared as FBSYMBOL ptr diagnostic_invalid_procedures()
private dim shared as integer diagnostic_invalid_count, diagnostic_invalid_capacity
'' Bound invalid-header identity comparisons independently of output bytes.
private const DIAGNOSTIC_HEADER_WORK_LIMIT = 2000000
private const DIAGNOSTIC_HEADER_INITIAL_CAPACITY = 32
private dim shared as integer diagnostic_header_work

'' Variable spellings are indexed by live native identity, not source names.
'' hashAdd borrows its key, so each payload owns that immutable key until
'' hashDel removes the item. Module teardown frees payloads before hashEnd.
private type DIAGNOSTIC_VARIABLE

	key_text as string
	spelling as string
	native_name as string
	owner as FBSYMBOLTB ptr
	item as HASHITEM ptr
	index as uinteger
end type
private const DIAGNOSTIC_VARIABLE_BUCKETS = 4096
private const DIAGNOSTIC_VARIABLE_WORK_LIMIT = 20000000
private dim shared as THASH diagnostic_variables
private dim shared as integer diagnostic_variable_count, diagnostic_variable_bytes
private dim shared as integer diagnostic_variable_work, diagnostic_variable_collision

private sub hDiagnosticVariablesClear( )
	if( diagnostic_variables.list <> NULL ) then
		for index as integer = 0 to diagnostic_variables.nodes - 1
			dim as HASHITEM ptr item = diagnostic_variables.list[index].head
			do while( item <> NULL )
				delete cptr(DIAGNOSTIC_VARIABLE ptr, item->data)
				item = item->next
			loop
		next
		hashEnd(@diagnostic_variables)
		diagnostic_variables.list = NULL
	end if
	diagnostic_variable_count = 0
	diagnostic_variable_bytes = 0
	diagnostic_variable_work = DIAGNOSTIC_VARIABLE_WORK_LIMIT
	diagnostic_variable_collision = FALSE
end sub

private function hDiagnosticVariable( byref key_text as const string ) as DIAGNOSTIC_VARIABLE ptr
	if( diagnostic_variables.list = NULL ) then return NULL
	dim as uinteger index = hashHash(strptr(key_text)) mod diagnostic_variables.nodes
	dim as HASHITEM ptr item = diagnostic_variables.list[index].head
	do while( item <> NULL )
		if( diagnostic_variable_work <= 0 ) then
			diagnostic_failed = TRUE
			return NULL
		end if
		diagnostic_variable_work -= 1
		if( *item->name = key_text ) then return item->data
		item = item->next
	loop
	return NULL
end function

sub fbSemanticDiagnosticsVariableReleased( byval sym as FBSYMBOL ptr )
	if( (diagnostic_variables.list = NULL) or (sym = NULL) ) then exit sub
	dim as string key_text = hex(cuint(sym))
	dim as DIAGNOSTIC_VARIABLE ptr entry = hDiagnosticVariable(key_text)
	if( entry = NULL ) then exit sub
	diagnostic_variable_count -= 1
	diagnostic_variable_bytes -= len(entry->key_text) + len(entry->spelling) + len(entry->native_name)
	hashDel(@diagnostic_variables, entry->item, entry->index)
	delete entry
end sub

sub fbSemanticDiagnosticsVariableCreated( byval sym as FBSYMBOL ptr, byval spelling as const zstring ptr )
	if( (diagnostic_output = NULL) or diagnostic_failed or (sym = NULL) or (spelling = NULL) ) then exit sub
	if( (sym->class <> FB_SYMBCLASS_VAR) or ((sym->attrib and FB_SYMBATTRIB_TEMP) <> 0) or (sym->id.name = NULL) ) then exit sub
	fbSemanticDiagnosticsVariableReleased(sym)
	if( diagnostic_failed ) then exit sub
	dim as string key_text = hex(cuint(sym))
	dim as integer bytes = len(key_text) + len(*spelling) + len(*sym->id.name)
	if( (diagnostic_variable_count >= DIAGNOSTIC_MAX_RECORDS) or (bytes >= DIAGNOSTIC_MAX_BYTES - diagnostic_variable_bytes) ) then
		diagnostic_failed = TRUE
		exit sub
	end if
	if( diagnostic_variables.list = NULL ) then hashInit(@diagnostic_variables, DIAGNOSTIC_VARIABLE_BUCKETS)
	dim as DIAGNOSTIC_VARIABLE ptr entry = new DIAGNOSTIC_VARIABLE
	if( entry = NULL ) then
		diagnostic_failed = TRUE
		exit sub
	end if
	entry->key_text = key_text
	entry->spelling = *spelling
	entry->native_name = *sym->id.name
	entry->owner = sym->symtb
	entry->index = hashHash(strptr(entry->key_text))
	entry->item = hashAdd(@diagnostic_variables, strptr(entry->key_text), entry, entry->index)
	diagnostic_variable_count += 1
	diagnostic_variable_bytes += bytes
end sub

sub fbSemanticDiagnosticsVariableAttempt( )
	diagnostic_variable_collision = FALSE
end sub

sub fbSemanticDiagnosticsVariableRejected( byval previous as FBSYMBOL ptr, byval spelling as const zstring ptr )
	if( (diagnostic_output = NULL) or diagnostic_failed or (previous = NULL) or (spelling = NULL) ) then exit sub
	if( (previous->class <> FB_SYMBCLASS_VAR) or (previous->id.name = NULL) ) then exit sub
	dim as string key_text = hex(cuint(previous))
	dim as DIAGNOSTIC_VARIABLE ptr entry = hDiagnosticVariable(key_text)
	if( entry = NULL ) then exit sub
	'' Scope and suffix compatibility have already failed in symbCanDuplicate.
	'' Identity guards exclude stale observations if a native address is reused.
	if( (entry->owner <> previous->symtb) or (entry->native_name <> *previous->id.name) ) then exit sub
	diagnostic_variable_collision = entry->spelling <> *spelling
end sub

function fbSemanticDiagnosticsVariableCollision( ) as integer
	dim as integer collision = diagnostic_variable_collision
	diagnostic_variable_collision = FALSE
	return (diagnostic_output <> NULL) and (diagnostic_failed = FALSE) and collision
end function

private sub hDiagnosticWrite( byref record_text as const string )
	if( (diagnostic_output = NULL) or diagnostic_failed ) then exit sub
	if( len(record_text) >= DIAGNOSTIC_MAX_BYTES - diagnostic_bytes ) then
		diagnostic_failed = TRUE
		exit sub
	end if
	dim as string line_text = record_text + NEWLINE
	if( fbSemanticOutputWrite(diagnostic_output, strptr(line_text), len(line_text)) = 0 ) then
		diagnostic_failed = TRUE
	else
		diagnostic_bytes += len(line_text)
	end if
end sub

sub fbSemanticDiagnosticsProtectFile( byref filename as const string )
	if( (diagnostic_output = NULL) or (len(filename) = 0) ) then exit sub
	if( fbSemanticOutputProtect(diagnostic_output, strptr(filename)) = 0 ) then diagnostic_failed = TRUE
end sub

sub fbSemanticDiagnosticsSource( byref filename as const string )
	if( (diagnostic_output = NULL) or diagnostic_failed or (len(filename) = 0) ) then exit sub
	fbSemanticDiagnosticsProtectFile(filename)
	for index as integer = 0 to diagnostic_source_count - 1
		if( diagnostic_sources(index) = filename ) then exit sub
	next
	if( diagnostic_source_count >= DIAGNOSTIC_MAX_SOURCES ) then
		diagnostic_failed = TRUE
		exit sub
	end if
	diagnostic_sources(diagnostic_source_count) = filename
	diagnostic_source_count += 1
	hDiagnosticWrite("SRC" + TABCHAR + fbSemanticModelNumber(diagnostic_source_count) + TABCHAR + fbSemanticModelEscape(filename))
end sub

sub fbSemanticDiagnosticsModule( byref filename as const string )
	if( (diagnostic_output = NULL) or diagnostic_failed ) then exit sub
	if( diagnostic_modules >= DIAGNOSTIC_MAX_RECORDS ) then
		diagnostic_failed = TRUE
		exit sub
	end if
	diagnostic_modules += 1
	diagnostic_context = ""
	diagnostic_has_point = FALSE
	erase diagnostic_invalid_procedures
	diagnostic_invalid_count = 0
	diagnostic_invalid_capacity = 0
	diagnostic_header_work = DIAGNOSTIC_HEADER_WORK_LIMIT
	hDiagnosticVariablesClear( )
	fbSemanticDiagnosticsSource(filename)
	hDiagnosticWrite("M" + TABCHAR + fbSemanticModelNumber(diagnostic_modules) + TABCHAR + _
		fbSemanticModelEscape(filename) + TABCHAR + fbSemanticModelEscape(fbGetTargetId()))
	hDiagnosticWrite("CAP" + TABCHAR + fbSemanticModelNumber(diagnostic_modules) + TABCHAR + _
		"procedure-signature-mismatches" + TABCHAR + "available")
	hDiagnosticWrite("CAP" + TABCHAR + fbSemanticModelNumber(diagnostic_modules) + TABCHAR + _
		"variable-case-collisions" + TABCHAR + "available")
end sub

sub fbSemanticDiagnosticsContext( byref context_text as const string )
	if( diagnostic_output <> NULL ) then
		diagnostic_context = context_text
		diagnostic_has_point = FALSE
	end if
end sub

sub fbSemanticDiagnosticsAt( byref context_text as const string, byref source as LEX_LOCATION )
	if( diagnostic_output = NULL ) then exit sub
	diagnostic_context = context_text
	diagnostic_point = source
	diagnostic_has_point = TRUE
end sub

sub fbSemanticDiagnosticsProcedure( byval proc as FBSYMBOL ptr, byval header_valid as integer )
	if( (diagnostic_output = NULL) or diagnostic_failed or (proc = NULL) ) then exit sub
	for index as integer = 0 to diagnostic_invalid_count - 1
		if( diagnostic_header_work <= 0 ) then
			diagnostic_failed = TRUE
			exit sub
		end if
		diagnostic_header_work -= 1
		if( diagnostic_invalid_procedures(index) <> proc ) then continue for
		if( header_valid ) then
			diagnostic_invalid_count -= 1
			diagnostic_invalid_procedures(index) = diagnostic_invalid_procedures(diagnostic_invalid_count)
		end if
		exit sub
	next
	if( header_valid ) then exit sub
	if( diagnostic_invalid_count >= DIAGNOSTIC_MAX_RECORDS ) then
		diagnostic_failed = TRUE
		exit sub
	end if
	if( diagnostic_invalid_count >= diagnostic_invalid_capacity ) then
		if( diagnostic_invalid_capacity = 0 ) then
			diagnostic_invalid_capacity = DIAGNOSTIC_HEADER_INITIAL_CAPACITY
		elseif( diagnostic_invalid_capacity > DIAGNOSTIC_MAX_RECORDS \ 2 ) then
			diagnostic_invalid_capacity = DIAGNOSTIC_MAX_RECORDS
		else
			diagnostic_invalid_capacity *= 2
		end if
		redim preserve diagnostic_invalid_procedures(0 to diagnostic_invalid_capacity - 1)
	end if
	diagnostic_invalid_procedures(diagnostic_invalid_count) = proc
	diagnostic_invalid_count += 1
end sub

function fbSemanticDiagnosticsProcedureValid( byval proc as FBSYMBOL ptr ) as integer
	if( (diagnostic_output = NULL) or diagnostic_failed or (proc = NULL) ) then return FALSE
	for index as integer = 0 to diagnostic_invalid_count - 1
		if( diagnostic_header_work <= 0 ) then
			diagnostic_failed = TRUE
			return FALSE
		end if
		diagnostic_header_work -= 1
		if( diagnostic_invalid_procedures(index) = proc ) then return FALSE
	next
	return TRUE
end function

function fbSemanticDiagnosticsBegin( byref filename as const string ) as integer
	hDiagnosticVariablesClear( )
	if( diagnostic_output <> NULL ) then
		if( fbSemanticOutputFinish(diagnostic_output, FALSE) = 0 ) then
			diagnostic_output = NULL
			return FALSE
		end if
	end if
	diagnostic_output = NULL
	for index as integer = 0 to diagnostic_source_count - 1
		diagnostic_sources(index) = ""
	next
	diagnostic_source_count = 0
	diagnostic_modules = 0
	diagnostic_count = 0
	diagnostic_errors = 0
	diagnostic_bytes = 0
	diagnostic_context = ""
	diagnostic_has_point = FALSE
	erase diagnostic_invalid_procedures
	diagnostic_invalid_count = 0
	diagnostic_invalid_capacity = 0
	diagnostic_header_work = DIAGNOSTIC_HEADER_WORK_LIMIT
	diagnostic_failed = FALSE
	diagnostic_output = fbSemanticOutputOpen(strptr(filename))
	if( diagnostic_output = NULL ) then return FALSE
	hDiagnosticWrite("FBCDIA" + TABCHAR + "1" + TABCHAR + FB_VERSION)
	return diagnostic_failed = FALSE
end function

function fbSemanticDiagnosticsEnd( byval succeeded as integer ) as integer
	hDiagnosticVariablesClear( )
	if( diagnostic_output = NULL ) then return TRUE
	hDiagnosticWrite("END" + TABCHAR + "1" + TABCHAR + fbSemanticModelNumber(diagnostic_modules) + _
		TABCHAR + fbSemanticModelNumber(diagnostic_source_count) + TABCHAR + fbSemanticModelNumber(diagnostic_count) + _
		TABCHAR + fbSemanticModelNumber(diagnostic_errors) + TABCHAR + fbSemanticModelNumber(abs(succeeded <> FALSE)))
	dim as integer published = fbSemanticOutputFinish(diagnostic_output, diagnostic_failed = FALSE)
	diagnostic_output = NULL
	for index as integer = 0 to diagnostic_source_count - 1
		diagnostic_sources(index) = ""
	next
	diagnostic_source_count = 0
	diagnostic_context = ""
	diagnostic_has_point = FALSE
	erase diagnostic_invalid_procedures
	diagnostic_invalid_count = 0
	diagnostic_invalid_capacity = 0
	diagnostic_header_work = DIAGNOSTIC_HEADER_WORK_LIMIT
	return (published <> 0) and (diagnostic_failed = FALSE)
end function

private sub hDiagnosticObserve _
	( byref severity as const string, byval code as integer, byval line_number as integer, _
	  byref message_text as const string, byref detail_text as const string, byref custom_text as const string )
	if( (diagnostic_output = NULL) or diagnostic_failed ) then exit sub
	if( diagnostic_count >= DIAGNOSTIC_MAX_RECORDS ) then
		diagnostic_failed = TRUE
		exit sub
	end if
	diagnostic_count += 1
	dim as string kind_text
	if( severity = "error" ) then
		diagnostic_errors += 1
		kind_text = diagnostic_context
		if( code = FB_ERRMSG_FORNEXTVARIABLEMISMATCH ) then kind_text = "for-next-variable-mismatch"
	end if
	fbSemanticDiagnosticsSource(env.inf.name)
	dim as LEX_LOCATION token_point = lexGetLastLocation( )
	if( diagnostic_has_point ) then token_point = diagnostic_point
	dim as integer physical = FALSE, raw_line = 0, raw_column = 0
	'' A scoped parser point names the declaration that failed, even when the
	'' header ends on another line. Ordinary diagnostics still require the last
	'' consumed token to agree with the displayed line. #line never supplies
	'' the physical coordinates, and generated tokens have no editable point.
	if( token_point.is_physical and token_point.raw_valid and (token_point.source_file = env.inf.name) and _
		((token_point.start_line = line_number) or diagnostic_has_point) and (token_point.raw_start_line > 0) and (token_point.raw_start_column >= 0) ) then
		physical = TRUE
		raw_line = token_point.raw_start_line
		raw_column = token_point.raw_start_column
	end if
	hDiagnosticWrite("DI" + TABCHAR + fbSemanticModelNumber(diagnostic_count) + TABCHAR + _
		fbSemanticModelNumber(diagnostic_modules) + TABCHAR + severity + TABCHAR + fbSemanticModelNumber(code) + _
		TABCHAR + fbSemanticModelEscape(kind_text) + TABCHAR + fbSemanticModelEscape(env.inf.name) + _
		TABCHAR + fbSemanticModelNumber(line_number) + TABCHAR + fbSemanticModelEscape(message_text) + _
		TABCHAR + fbSemanticModelEscape(detail_text) + TABCHAR + fbSemanticModelEscape(custom_text) + _
		TABCHAR + fbSemanticModelNumber(abs(physical)) + TABCHAR + fbSemanticModelNumber(raw_line) + _
		TABCHAR + fbSemanticModelNumber(raw_column))
end sub

'' -------------------------------------------------------------------------
'' Shared compiler diagnostic observation
'' -------------------------------------------------------------------------

sub fbSemanticModelDiagnostic _
	( byref severity as const string, byval code as integer, byval line_number as integer, _
	  byval message as const zstring ptr, byval detail as const zstring ptr, byval custom_text as const zstring ptr )
	if( (fbSemanticModelEnabled( ) = FALSE) and (diagnostic_output = NULL) ) then exit sub
	dim as string text, extra, custom
	if( message <> NULL ) then text = *message
	if( detail <> NULL ) then extra = *detail
	if( custom_text <> NULL ) then custom = *custom_text
	hDiagnosticObserve(severity, code, line_number, text, extra, custom)
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	'' Reporting must not ask the lexer to scan another token. A diagnostic
	'' point is informational, even when its line resembles a source range.
	fbSemanticModelAppendProvenance("DI" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + severity + _
		TABCHAR + fbSemanticModelNumber(code) + TABCHAR + fbSemanticModelEscape(text) + TABCHAR + _
		fbSemanticModelEscape(extra) + TABCHAR + fbSemanticModelEscape(custom) + TABCHAR + _
		fbSemanticModelEscape(env.inf.name) + TABCHAR + fbSemanticModelNumber(line_number) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(parser.currproc)))
end sub

'' end of tooling/semantic-diagnostics.bas
