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
#include once "tooling/semantic-macros.bi"
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
private const DIAGNOSTIC_MAX_RECORD_BYTES = 1048576
private dim shared as any ptr diagnostic_output
private dim shared as integer diagnostic_failed, diagnostic_modules
private dim shared as integer diagnostic_module_open
private dim shared as longint diagnostic_model_count, diagnostic_model_errors
private dim shared as longint diagnostic_model_warnings, diagnostic_model_promoted
private dim shared as integer diagnostic_count, diagnostic_errors, diagnostic_bytes
private dim shared as integer diagnostic_source_count
private dim shared as string diagnostic_sources(0 to DIAGNOSTIC_MAX_SOURCES - 1)
private dim shared as string diagnostic_context
private dim shared as LEX_LOCATION diagnostic_point
private dim shared as integer diagnostic_has_point

'' Context is copied only after the native reporter accepts a diagnostic.
'' No native symbol or parameter-name pointer survives the reporter callback.
private const DIAGNOSTIC_PARAMETER_LIMIT = 4096
private const DIAGNOSTIC_PARAMETER_BYTE_LIMIT = 1048576
private type DIAGNOSTIC_PARAMETER
	token as integer
	parameter as integer
	spelling as string
	procedure_name as string
	owner_name as string
	kind as string
end type
private dim shared as DIAGNOSTIC_PARAMETER ptr diagnostic_parameters(0 to DIAGNOSTIC_PARAMETER_LIMIT - 1)
private dim shared as integer diagnostic_parameter_count, diagnostic_parameter_bytes
private dim shared as string diagnostic_policy
private dim shared as integer diagnostic_initialized
private dim shared as integer diagnostic_warning_level = -1

private sub hDiagnosticContextClear( )
	for index as integer = 0 to diagnostic_parameter_count - 1
		delete diagnostic_parameters(index)
		diagnostic_parameters(index) = NULL
	next
	diagnostic_parameter_count = 0
	diagnostic_parameter_bytes = 0
	diagnostic_policy = ""
	diagnostic_initialized = FALSE
	diagnostic_warning_level = -1
end sub

sub fbSemanticDiagnosticsPolicy( byval initialized as integer, byval options as integer, _
	byval show_source as integer, byval error_count as integer, byval promoted as integer, byval warning_level as integer )
	hDiagnosticContextClear( )
	if( (diagnostic_output = NULL) and (fbSemanticModelEnabled( ) = FALSE) ) then exit sub
	if( (warning_level < -1) or (warning_level = 0) or (warning_level > FB_WARNINGMSGS_HIGHEST_LEVEL) ) then
		diagnostic_failed = TRUE
		fbSemanticModelFailAt("invalid native warning catalog level")
		exit sub
	end if
	if( promoted and fbSemanticModelEnabled( ) ) then diagnostic_model_promoted += 1
	diagnostic_initialized = initialized <> FALSE
	diagnostic_warning_level = warning_level
	diagnostic_policy = fbSemanticModelNumber(abs(diagnostic_initialized)) + TABCHAR + fbSemanticModelNumber(options) + TABCHAR + _
		fbSemanticModelNumber(abs(show_source <> FALSE)) + TABCHAR + fbSemanticModelNumber(error_count) + TABCHAR + _
		fbSemanticModelNumber(env.clopt.warninglevel) + TABCHAR + fbSemanticModelNumber(env.clopt.maxerrors) + TABCHAR + _
		fbSemanticModelNumber(abs(promoted <> FALSE))
end sub

sub fbSemanticDiagnosticsParameter( byval proc as FBSYMBOL ptr, byval token as integer, _
	byval parameter as integer, byval spelling as const zstring ptr )
	if( len(diagnostic_policy) = 0 ) then exit sub
	if( diagnostic_parameter_count >= DIAGNOSTIC_PARAMETER_LIMIT ) then
		diagnostic_failed = TRUE
		fbSemanticModelFailAt("diagnostic parameter count exceeds its bound")
		exit sub
	end if
	dim as DIAGNOSTIC_PARAMETER ptr item = new DIAGNOSTIC_PARAMETER
	if( item = NULL ) then
		diagnostic_failed = TRUE
		fbSemanticModelFailAt("diagnostic parameter allocation failed")
		exit sub
	end if
	item->token = token
	item->parameter = parameter
	if( spelling <> NULL ) then item->spelling = *spelling
	item->kind = "none"
	if( proc <> NULL ) then
		if( proc->id.name <> NULL ) then item->procedure_name = *proc->id.name
		if( proc->symtb <> NULL ) then
			if( proc->symtb->owner <> NULL ) then
				if( proc->symtb->owner->id.name <> NULL ) then item->owner_name = *proc->symtb->owner->id.name
			end if
		end if
		item->kind = "unknown-symbol"
		if( proc->class = FB_SYMBCLASS_PROC ) then
			if( symbIsConstructor(proc) ) then
				item->kind = "constructor"
			elseif( symbIsDestructor0(proc) or symbIsDestructor1(proc) ) then
				item->kind = "destructor"
			elseif( symbIsOperator(proc) ) then
				item->kind = "operator"
			elseif( symbGetIsFuncPtr(proc) ) then
				item->kind = "procedure-pointer"
			else
				item->kind = "procedure"
			end if
		end if
	end if
	dim as integer bytes = len(item->spelling) + len(item->procedure_name) + len(item->owner_name) + len(item->kind)
	if( bytes > DIAGNOSTIC_PARAMETER_BYTE_LIMIT - diagnostic_parameter_bytes ) then
		delete item
		diagnostic_failed = TRUE
		fbSemanticModelFailAt("diagnostic parameter text exceeds its bound")
		exit sub
	end if
	diagnostic_parameters(diagnostic_parameter_count) = item
	diagnostic_parameter_count += 1
	diagnostic_parameter_bytes += bytes
end sub

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
	'' The independent reader has the same per-record bound. Escaping can
	'' triple a copied byte string, even when its unescaped input was bounded.
	if( (len(record_text) > DIAGNOSTIC_MAX_RECORD_BYTES) or _
		(len(record_text) >= DIAGNOSTIC_MAX_BYTES - diagnostic_bytes) ) then
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

sub fbSemanticDiagnosticsFinishModule( byref outcome as const string, byval error_count as integer )
	dim as integer valid = (((outcome = "accepted") or (outcome = "restarted")) and (error_count = 0)) or _
		((outcome = "rejected") and (error_count > 0)) or ((outcome = "aborted") and (error_count = -1))
	if( valid = FALSE ) then
		diagnostic_failed = TRUE
		fbSemanticModelFailAt("invalid diagnostic attempt outcome")
		exit sub
	end if
	if( fbSemanticModelEnabled( ) ) then
		fbSemanticModelAppendProvenance("K" + TABCHAR + "module" + TABCHAR + _
			fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + "reported-diagnostic-counts" + TABCHAR + _
			fbSemanticModelEscape(fbSemanticModelNumber(diagnostic_model_count) + TABCHAR + _
			fbSemanticModelNumber(diagnostic_model_errors) + TABCHAR + fbSemanticModelNumber(diagnostic_model_warnings) + TABCHAR + _
			fbSemanticModelNumber(diagnostic_model_promoted) + TABCHAR + fbSemanticModelNumber(error_count) + TABCHAR + outcome))
	end if
	if( (diagnostic_output = NULL) or (diagnostic_module_open = FALSE) ) then exit sub
	hDiagnosticWrite("ME" + TABCHAR + fbSemanticModelNumber(diagnostic_modules) + TABCHAR + _
		outcome + TABCHAR + fbSemanticModelNumber(error_count))
	diagnostic_module_open = FALSE
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
	diagnostic_model_count = 0
	diagnostic_model_errors = 0
	diagnostic_model_warnings = 0
	diagnostic_model_promoted = 0
	if( (diagnostic_output = NULL) or diagnostic_failed ) then exit sub
	'' The driver closes every normal attempt before releasing its symbols.
	'' An interrupted caller must not silently give that attempt accepted status.
	if( diagnostic_module_open ) then fbSemanticDiagnosticsFinishModule("aborted", -1)
	if( diagnostic_modules >= DIAGNOSTIC_MAX_RECORDS ) then
		diagnostic_failed = TRUE
		exit sub
	end if
	diagnostic_modules += 1
	diagnostic_module_open = TRUE
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
	hDiagnosticWrite("CAP" + TABCHAR + fbSemanticModelNumber(diagnostic_modules) + TABCHAR + _
		"unclosed-procedures" + TABCHAR + "available")
	hDiagnosticWrite("CAP" + TABCHAR + fbSemanticModelNumber(diagnostic_modules) + TABCHAR + _
		"error-resume-next-syntax" + TABCHAR + "available")
	hDiagnosticWrite("CAP" + TABCHAR + fbSemanticModelNumber(diagnostic_modules) + TABCHAR + _
		"declaration-suffix-as-conflicts" + TABCHAR + "available")
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

sub fbSemanticDiagnosticsPoint( byref source as LEX_LOCATION )
	if( diagnostic_output = NULL ) then exit sub
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
	hDiagnosticContextClear( )
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
	diagnostic_module_open = FALSE
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
	hDiagnosticWrite("CAP" + TABCHAR + "0" + TABCHAR + "reported-diagnostic-contexts" + TABCHAR + "available")
	hDiagnosticWrite("CAP" + TABCHAR + "0" + TABCHAR + "diagnostic-attempt-lifetimes" + TABCHAR + "available")
	hDiagnosticWrite("CAP" + TABCHAR + "0" + TABCHAR + "reported-warning-levels" + TABCHAR + "available")
	return diagnostic_failed = FALSE
end function

function fbSemanticDiagnosticsEnd( byval succeeded as integer ) as integer
	hDiagnosticContextClear( )
	hDiagnosticVariablesClear( )
	if( diagnostic_output = NULL ) then return TRUE
	if( diagnostic_module_open ) then fbSemanticDiagnosticsFinishModule("aborted", -1)
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

private function hDiagnosticPolicyFields( ) as string
	if( len(diagnostic_policy) = 0 ) then
		'' A legacy direct observer call has no native reporter snapshot.
		'' Its absent source/parameter context must not become a guessed one.
		diagnostic_policy = "0" + TABCHAR + "0" + TABCHAR + "0" + TABCHAR + "0" + TABCHAR + "-1" + TABCHAR + "0" + TABCHAR + "0"
	end if
	return diagnostic_policy + TABCHAR + fbSemanticModelNumber(diagnostic_parameter_count) + TABCHAR + _
		fbSemanticModelNumber(env.clopt.lang) + TABCHAR + fbSemanticModelNumber(env.clopt.backend) + TABCHAR + _
		fbGetTargetId( ) + TABCHAR + fbSemanticModelNumber(abs(env.inited and (lex.ctx <> NULL))) + TABCHAR + _
		fbSemanticModelNumber(diagnostic_warning_level)
end function

private function hDiagnosticParameterFields( byval index as integer ) as string
	dim as DIAGNOSTIC_PARAMETER ptr item = diagnostic_parameters(index)
	'' K has one outer field. Escape the three byte strings before joining
	'' its inner fields, so a parameter spelling cannot supply a separator.
	return fbSemanticModelNumber(index + 1) + TABCHAR + fbSemanticModelNumber(item->token) + TABCHAR + _
		fbSemanticModelNumber(item->parameter) + TABCHAR + fbSemanticModelEscape(item->spelling) + TABCHAR + _
		fbSemanticModelEscape(item->procedure_name) + TABCHAR + fbSemanticModelEscape(item->owner_name) + TABCHAR + item->kind
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
	dim as integer reported_module = iif(diagnostic_initialized, diagnostic_modules, 0)
	dim as string reported_file
	if( reported_module > 0 ) then
		reported_file = env.inf.name
		fbSemanticDiagnosticsSource(reported_file)
	else
		kind_text = ""
	end if
	dim as LEX_LOCATION token_point
	'' Invocation diagnostics can run after lexEnd. Their absence of a source
	'' context is already known, so do not inspect a retired lexer snapshot.
	if( reported_module > 0 ) then
		token_point = lexGetLastLocation( )
		if( diagnostic_has_point ) then token_point = diagnostic_point
	end if
	dim as integer macro_origin = FALSE
	if( (reported_module > 0) and (token_point.is_physical = FALSE) ) then
		dim as LEX_LOCATION invocation_point
		if( fbSemanticModelMacroExpressionLocation(token_point, token_point, 0, 0, invocation_point) ) then
			'' A warning may point at the observed invocation. This does not
			'' make the generated replacement an editable physical expression.
			token_point = invocation_point
			macro_origin = TRUE
		end if
	end if
	dim as integer physical = FALSE, raw_line = 0, raw_column = 0
	'' A scoped parser point names the declaration that failed, even when the
	'' header ends on another line. Ordinary diagnostics still require the last
	'' consumed token to agree with the displayed line. #line never supplies
	'' the physical coordinates. A generated token can instead name its observed
	'' invocation when the full model retained that original macro lineage.
	if( (reported_module > 0) and (token_point.is_physical or macro_origin) and token_point.raw_valid and (token_point.source_file = env.inf.name) and _
		((token_point.start_line = line_number) or diagnostic_has_point or macro_origin) and (token_point.raw_start_line > 0) and (token_point.raw_start_column >= 0) ) then
		physical = TRUE
		raw_line = token_point.raw_start_line
		raw_column = token_point.raw_start_column
	end if
	hDiagnosticWrite("DI" + TABCHAR + fbSemanticModelNumber(diagnostic_count) + TABCHAR + _
		fbSemanticModelNumber(reported_module) + TABCHAR + severity + TABCHAR + fbSemanticModelNumber(code) + _
		TABCHAR + fbSemanticModelEscape(kind_text) + TABCHAR + fbSemanticModelEscape(reported_file) + _
		TABCHAR + fbSemanticModelNumber(line_number) + TABCHAR + fbSemanticModelEscape(message_text) + _
		TABCHAR + fbSemanticModelEscape(detail_text) + TABCHAR + fbSemanticModelEscape(custom_text) + _
		TABCHAR + fbSemanticModelNumber(abs(physical)) + TABCHAR + fbSemanticModelNumber(raw_line) + _
		TABCHAR + fbSemanticModelNumber(raw_column))
	hDiagnosticWrite("DC" + TABCHAR + fbSemanticModelNumber(diagnostic_count) + TABCHAR + fbSemanticModelEscape(hDiagnosticPolicyFields( )))
	for index as integer = 0 to diagnostic_parameter_count - 1
		dim as DIAGNOSTIC_PARAMETER ptr item = diagnostic_parameters(index)
		hDiagnosticWrite("DP" + TABCHAR + fbSemanticModelNumber(diagnostic_count) + TABCHAR + _
			fbSemanticModelNumber(index + 1) + TABCHAR + fbSemanticModelNumber(item->token) + TABCHAR + _
			fbSemanticModelNumber(item->parameter) + TABCHAR + fbSemanticModelEscape(item->spelling) + TABCHAR + _
			fbSemanticModelEscape(item->procedure_name) + TABCHAR + fbSemanticModelEscape(item->owner_name) + TABCHAR + item->kind)
	next
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
	if( fbSemanticModelEnabled( ) = FALSE ) then
		hDiagnosticContextClear( )
		exit sub
	end if
	diagnostic_model_count += 1
	if( severity = "error" ) then
		diagnostic_model_errors += 1
	else
		diagnostic_model_warnings += 1
	end if
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
	fbSemanticModelAppendProvenance("K" + TABCHAR + "diagnostic" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
		"reported-context" + TABCHAR + fbSemanticModelEscape(hDiagnosticPolicyFields( )))
	for index as integer = 0 to diagnostic_parameter_count - 1
		fbSemanticModelAppendProvenance("K" + TABCHAR + "diagnostic" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
			"reported-parameter:" + fbSemanticModelNumber(index + 1) + TABCHAR + fbSemanticModelEscape(hDiagnosticParameterFields(index)))
	next
	hDiagnosticContextClear( )
end sub

'' end of tooling/semantic-diagnostics.bas
