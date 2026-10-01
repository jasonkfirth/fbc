'' Project: FreeBASIC compiler - compiler semantic observations
'' -----------------------------------------
''
'' File: tooling/semantic-model.bas
''
'' Purpose:
''
''     Export semantic facts selected by the compiler with stable source ranges.
''
'' Responsibilities:
''
''     - export compiler-owned symbol, expression, and binding facts
''     - preserve source locations and validate export completion
''
'' This file intentionally does NOT contain:
''
''     - a second parser or semantic decisions independent of the compiler
''

'' Project: FreeBASIC Compiler
'' -------------------------
''
'' File: semantic-model.bas
''
'' Purpose:
''
''     Export versioned semantic facts from the compiler's resolved symbols
''     and typed abstract syntax trees for static-analysis consumers.
''
'' Responsibilities:
''
''     - write a transactional, tab-separated semantic sidecar
''     - retain bounded expression facts under an explicit recovery footer
''     - preserve symbol identity, type, scope, and parent relationships
''     - serialize procedure AST nodes with source locations
''     - export implicit construction/destruction selections as anchored relationships
''     - reject incomplete output unless expression-only recovery is explicit
''     - export the bounded set of source files read by each compiler request
''     - release the output file and dynamic buffers at compiler shutdown
''
'' This file intentionally does NOT contain:
''
''     - source parsing or name resolution
''     - linter rules or diagnostics
''     - compiler command-line parsing
''
'' Resource ownership:
''
''     The sidecar file remains open from fbSemanticModelBegin() through
''     fbSemanticModelEnd(). Module buffers and symbol tables are released at the
''     end of the compiler invocation and reused only between modules. A failed
''     parse can commit only expression-only records through an explicit R tag.

#include once "core/fbint.bi"
#include once "ast/ast.bi"
#include once "lexer/lex.bi"
#include once "symbols/symb.bi"
#include once "crt/mem.bi"
#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-output.bi"

'' -------------------------------------------------------------------------
'' Export limits and process-local module state
'' -------------------------------------------------------------------------

private const SEMANTIC_MODEL_SCHEMA = "20"
private const SEMANTIC_MODEL_MAX_SYMBOLS = 1000000
private const SEMANTIC_MODEL_MAX_DEPENDENCIES = 5000
private const SEMANTIC_MODEL_INITIAL_SYMBOL_INDEX_CAPACITY = 256
private const SEMANTIC_MODEL_MAX_SYMBOL_INDEX_CAPACITY = 2097152
private const SEMANTIC_MODEL_INITIAL_MODULE_BUFFER_CAPACITY = 8192
private const SEMANTIC_MODEL_MAX_MODULE_BUFFER_BYTES = 268435456
private const SEMANTIC_MODEL_MAX_NODES_PER_MODEL = 1000000
private const SEMANTIC_MODEL_MAX_EXPRESSIONS_PER_MODEL = 1000000
private const SEMANTIC_MODEL_MAX_BINDINGS_PER_MODEL = 1000000
private const SEMANTIC_MODEL_MAX_IMPLICIT_CALLS_PER_MODEL = 1000000
'' Details share the bounded transaction buffer and invalidate the model on
'' overflow. Several details per symbol or node are normal.
private const SEMANTIC_MODEL_MAX_DETAILS_PER_MODEL = 16000000
'' Keep one malformed or pathological procedure from expanding a sidecar without bound.
private const SEMANTIC_MODEL_MAX_CALL_SIGNATURE_BYTES = 1048576
private const SEMANTIC_MODEL_MAX_SOURCE_CONTEXTS = FB_MAXINCRECLEVEL

private type SEMANTIC_MODEL_SYMBOL
	sym         as FBSYMBOL ptr
	identity    as ulongint
	state       as integer
	visit       as ulongint
	declared    as integer
	paramvarid  as longint
	initializerid as longint
end type

private type SEMANTIC_MODEL_SYMBOL_SLOT
	identity    as ulongint
	id          as integer
end type

private type SEMANTIC_MODEL_NODEFRAME
	node        as ASTNODE ptr
	parentid    as longint
	nodeid      as longint
	edge        as integer
end type

dim shared as any ptr semantic_model_output
dim shared as integer semantic_model_file_open
dim shared as integer semantic_model_expressions_only
dim shared as integer semantic_model_module_open
dim shared as integer semantic_model_module_failed
dim shared as integer semantic_model_any_failed
dim shared as integer semantic_model_recovery_module_count
dim shared as integer semantic_model_dependency_count
dim shared as integer semantic_model_dependencies_complete
dim shared as string semantic_model_dependencies(0 to SEMANTIC_MODEL_MAX_DEPENDENCIES - 1)
dim shared as string semantic_model_physical_sources(0 to SEMANTIC_MODEL_MAX_SOURCE_CONTEXTS - 1)
dim shared as integer semantic_model_source_remapped(0 to SEMANTIC_MODEL_MAX_SOURCE_CONTEXTS - 1)
dim shared as integer semantic_model_source_bound_valid
dim shared as integer semantic_model_source_bound_line
dim shared as integer semantic_model_source_bound_physical_line
dim shared as integer semantic_model_source_bound_columns
dim shared as string semantic_model_source_bound_file
dim shared as integer semantic_model_source_scan_valid
dim shared as integer semantic_model_source_scan_line
dim shared as integer semantic_model_source_scan_format
dim shared as longint semantic_model_source_scan_filepos
dim shared as string semantic_model_source_scan_file
dim shared as LEX_LOCATION semantic_model_active_expression_start
dim shared as LEX_LOCATION semantic_model_pending_scope_exit_source
dim shared as integer semantic_model_pending_scope_exit_valid
dim shared as longint semantic_model_pending_scope_exit_nonphysical
dim shared as longint semantic_model_active_expression_nonphysical
dim shared as string semantic_model_last_expression_fact
dim shared as string semantic_model_pending_expression_value
dim shared as integer semantic_model_pending_operator_override
dim shared as LEX_LOCATION semantic_model_pending_operator_start
dim shared as LEX_LOCATION semantic_model_pending_operator_end
dim shared as integer semantic_model_module_count
dim shared as string semantic_model_last_expression_shape
dim shared as integer semantic_model_last_expression_had_operator_override
dim shared as uinteger semantic_model_last_expression_buffer_start
dim shared as integer semantic_model_module_proc_count
dim shared as integer semantic_model_module_type_fact_count
dim shared as longint semantic_model_module_expression_count
dim shared as longint semantic_model_module_binding_count
dim shared as longint semantic_model_module_implicit_call_count
dim shared as longint semantic_model_module_node_count
dim shared as integer semantic_model_proc_count
dim shared as integer semantic_model_total_symbol_count
dim shared as integer semantic_model_total_type_fact_count
dim shared as longint semantic_model_expression_count
dim shared as longint semantic_model_binding_count
dim shared as longint semantic_model_implicit_call_count
dim shared as longint semantic_model_node_count
dim shared as string semantic_model_filename
dim shared as string semantic_model_module_source
dim shared as ubyte ptr semantic_model_module_buffer
dim shared as uinteger semantic_model_module_buffer_len
dim shared as uinteger semantic_model_module_buffer_capacity
dim shared as integer semantic_model_symbol_count
dim shared as SEMANTIC_MODEL_SYMBOL ptr semantic_model_symbols
dim shared as integer semantic_model_symbol_capacity
dim shared as SEMANTIC_MODEL_SYMBOL_SLOT ptr semantic_model_symbol_index
dim shared as integer semantic_model_symbol_index_capacity
dim shared as ulongint semantic_model_next_symbol_identity
dim shared as ulongint semantic_model_visit
dim shared as longint semantic_model_detail_count
dim shared as longint semantic_model_module_detail_count

declare function hSemanticModelRangeFitsCurrentSourceLine _
	( _
		byref source_end as LEX_LOCATION _
	) as integer

'' -------------------------------------------------------------------------
'' Sidecar lifecycle state
'' -------------------------------------------------------------------------
'' Module state for the one active sidecar. The compiler serializes source
'' compilation, so this state is not shared between concurrent compiler jobs.

'' -------------------------------------------------------------------------
'' Record formatting and bounded module staging
'' -------------------------------------------------------------------------

private function hSemanticModelNumber(byval value as longint) as string
	return ltrim(str(value))
end function

private function hSemanticModelHasSymbol(byval sym as FBSYMBOL ptr) as integer
	return (sym <> NULL) and (sym <> cast(FBSYMBOL ptr, INVALID))
end function

private function hSemanticModelEscape(byref value as string) as string
	const HEXDIGITS = "0123456789ABCDEF"
	dim as integer i, bytevalue, output_position, output_length
	dim as string result
	dim as ubyte ptr result_bytes

	output_length = len(value)
	if( output_length > SEMANTIC_MODEL_MAX_MODULE_BUFFER_BYTES ) then
		semantic_model_module_failed = TRUE
		semantic_model_any_failed = TRUE
		return ""
	end if

	for i = 1 to len(value)
		bytevalue = asc(mid(value, i, 1))
		if( (bytevalue < 32) or (bytevalue >= 127) or (bytevalue = 37) ) then
			if( output_length > SEMANTIC_MODEL_MAX_MODULE_BUFFER_BYTES - 2 ) then
				semantic_model_module_failed = TRUE
				semantic_model_any_failed = TRUE
				return ""
			end if
			output_length += 2
		end if
	next

	result = space(output_length)
	if( len(result) <> output_length ) then
		fbSemanticModelFail( )
		return ""
	end if
	result_bytes = strptr(result)
	output_position = 1
	for i = 1 to len(value)
		bytevalue = asc(mid(value, i, 1))
		select case bytevalue
		case 0 to 31, 37, 127 to 255
			result_bytes[output_position - 1] = asc("%")
			result_bytes[output_position] = asc(mid(HEXDIGITS, _
				(bytevalue \ 16) + 1, 1))
			result_bytes[output_position + 1] = asc(mid(HEXDIGITS, _
				(bytevalue mod 16) + 1, 1))
			output_position += 3
		case else
			result_bytes[output_position - 1] = bytevalue
			output_position += 1
		end select
	next

	return left(result, output_position - 1)
end function


private sub hSemanticModelAppendBytes(byval source as const any ptr, _
	byval byte_count as uinteger)
	dim as uinteger required, required_storage
	dim as uinteger new_capacity
	dim as ubyte ptr resized_buffer

	if( (semantic_model_module_failed) or (byte_count = 0) ) then exit sub

	required = semantic_model_module_buffer_len + byte_count
	if( (required < semantic_model_module_buffer_len) or _
		(required > SEMANTIC_MODEL_MAX_MODULE_BUFFER_BYTES) ) then
		semantic_model_module_failed = TRUE
		semantic_model_any_failed = TRUE
		exit sub
	end if
	required_storage = required + 1

	if( required_storage > semantic_model_module_buffer_capacity ) then
		new_capacity = semantic_model_module_buffer_capacity
		if( new_capacity = 0 ) then
			new_capacity = SEMANTIC_MODEL_INITIAL_MODULE_BUFFER_CAPACITY
		end if

		do while( new_capacity < required_storage )
			if( new_capacity > SEMANTIC_MODEL_MAX_MODULE_BUFFER_BYTES \ 2 ) then
				new_capacity = SEMANTIC_MODEL_MAX_MODULE_BUFFER_BYTES + 1
			else
				new_capacity *= 2
			end if
		loop

		resized_buffer = reallocate(semantic_model_module_buffer, new_capacity)
		if( resized_buffer = NULL ) then
			semantic_model_module_failed = TRUE
			semantic_model_any_failed = TRUE
			exit sub
		end if

		semantic_model_module_buffer = resized_buffer
		semantic_model_module_buffer_capacity = new_capacity
	end if

	memcpy(semantic_model_module_buffer + semantic_model_module_buffer_len, source, _
		byte_count)
	semantic_model_module_buffer_len = required
	semantic_model_module_buffer[semantic_model_module_buffer_len] = 0
end sub

private sub hSemanticModelAppendLine(byref value as string)
	'' A non-expression record separates parser results and breaks adjacency.
	if( (len(value) < 2) or (left(value, 2) <> "E" + TABCHAR) ) then
		if( len(semantic_model_pending_expression_value) > 0 ) then
			'' A value belongs to the final adjacent E fact. Delay its C record
			'' until the parser's deduplication/replacement of that E is finished.
			dim as string pending_value = semantic_model_pending_expression_value
			semantic_model_pending_expression_value = ""
			fbSemanticModelAppendDetail(pending_value)
		end if
		semantic_model_last_expression_fact = ""
		semantic_model_last_expression_shape = ""
		semantic_model_last_expression_had_operator_override = FALSE
	end if
	dim as string line_text = value + NEWLINE
	if( (len(value) >= 2) and (left(value, 2) = "E" + TABCHAR) ) then
		semantic_model_last_expression_buffer_start = semantic_model_module_buffer_len
	end if

	hSemanticModelAppendBytes(strptr(line_text), len(line_text))
end sub

'' Metadata modules use the same transaction and limits as the core records.
'' No exporter may bypass staging or write directly to the sidecar.
function fbSemanticModelFullEnabled( ) as integer
	return abs(semantic_model_file_open and semantic_model_module_open and _
		(not semantic_model_expressions_only) and (not semantic_model_module_failed))
end function

sub fbSemanticModelFail( )
	semantic_model_module_failed = TRUE
	semantic_model_any_failed = TRUE
end sub

function fbSemanticModelNumber(byval value as longint) as string
	return hSemanticModelNumber(value)
end function

function fbSemanticModelEscape(byref value as string) as string
	return hSemanticModelEscape(value)
end function

sub fbSemanticModelAppendDetail(byref value as string)
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( semantic_model_detail_count + semantic_model_module_detail_count >= _
		SEMANTIC_MODEL_MAX_DETAILS_PER_MODEL ) then
		fbSemanticModelFail( )
		exit sub
	end if
	hSemanticModelAppendLine(value)
	if( semantic_model_module_failed = FALSE ) then semantic_model_module_detail_count += 1
end sub

private function hSemanticModelReplaceLastExpressionFact(byref expression_fact as string) as integer
	if( (semantic_model_module_buffer = NULL) or _
		(semantic_model_last_expression_buffer_start >= semantic_model_module_buffer_len) or _
		(semantic_model_expression_count + semantic_model_module_expression_count <= 0) ) then
		return FALSE
	end if

	'' Keep the existing expression identity and replace only the staged E record.
	semantic_model_module_buffer_len = semantic_model_last_expression_buffer_start
	semantic_model_module_buffer[semantic_model_module_buffer_len] = 0
	hSemanticModelAppendLine("E" + TABCHAR + _
		hSemanticModelNumber(semantic_model_expression_count + _
			semantic_model_module_expression_count) + TABCHAR + expression_fact)
	if( semantic_model_module_failed ) then return FALSE
	semantic_model_last_expression_fact = expression_fact
	return TRUE
end function

private sub hSemanticModelWriteDependencies( )
	for index as integer = 0 to semantic_model_dependency_count - 1
		dim as string line_text = "D" + TABCHAR + _
			hSemanticModelEscape(semantic_model_dependencies(index)) + NEWLINE
		if( fbSemanticOutputWrite(semantic_model_output, strptr(line_text), len(line_text)) = 0 ) then
			fbSemanticModelFail( )
			exit sub
		end if
	next
end sub

sub fbSemanticModelProtectFile(byref filename as const string)
	if( (semantic_model_file_open = FALSE) or (len(filename) = 0) ) then exit sub
	if( fbSemanticOutputProtect(semantic_model_output, strptr(filename)) = 0 ) then fbSemanticModelFail( )
end sub

sub fbSemanticModelAddDependency(byref filename as string)
	if( (semantic_model_file_open = FALSE) or (len(filename) = 0) ) then exit sub
	'' File protection remains active even after the public dependency limit.
	fbSemanticModelProtectFile(filename)
	for index as integer = 0 to semantic_model_dependency_count - 1
		if( semantic_model_dependencies(index) = filename ) then exit sub
	next
	if( semantic_model_dependency_count >= SEMANTIC_MODEL_MAX_DEPENDENCIES ) then
		semantic_model_dependencies_complete = FALSE
		exit sub
	end if
	semantic_model_dependencies(semantic_model_dependency_count) = filename
	semantic_model_dependency_count += 1
end sub

sub fbSemanticModelBeginSource(byref filename as string, byval depth as integer)
	if( semantic_model_file_open = FALSE ) then exit sub
	if( (depth < 0) or (depth >= SEMANTIC_MODEL_MAX_SOURCE_CONTEXTS) ) then exit sub
	semantic_model_physical_sources(depth) = filename
	semantic_model_source_remapped(depth) = FALSE
end sub

sub fbSemanticModelEndSource(byval depth as integer)
	if( semantic_model_file_open = FALSE ) then exit sub
	if( (depth < 0) or (depth >= SEMANTIC_MODEL_MAX_SOURCE_CONTEXTS) ) then exit sub
	semantic_model_physical_sources(depth) = ""
	semantic_model_source_remapped(depth) = FALSE
end sub

sub fbSemanticModelMarkSourceRemapped(byval depth as integer)
	if( semantic_model_file_open = FALSE ) then exit sub
	if( (depth < 0) or (depth >= SEMANTIC_MODEL_MAX_SOURCE_CONTEXTS) ) then exit sub
	semantic_model_source_remapped(depth) = TRUE
end sub

private function hSemanticModelLocationIsPhysical(byref source as LEX_LOCATION) as integer
	if( (source.is_physical = FALSE) or _
		(env.includerec < 0) or _
		(env.includerec >= SEMANTIC_MODEL_MAX_SOURCE_CONTEXTS) or _
		(semantic_model_source_remapped(env.includerec)) ) then
		return FALSE
	end if
	return abs(source.source_file = semantic_model_physical_sources(env.includerec))
end function

'' -------------------------------------------------------------------------
'' Compiler symbol and typed AST serialization
'' -------------------------------------------------------------------------

sub fbSemanticModelInitializeSymbol(byval sym as FBSYMBOL ptr)
	if( sym = NULL ) then exit sub
	if( sym->semantic_model_identity <> 0 ) then exit sub
	if( semantic_model_file_open = FALSE ) then exit sub
	if( semantic_model_next_symbol_identity = &hFFFFFFFFFFFFFFFFull ) then
		semantic_model_module_failed = TRUE
		semantic_model_any_failed = TRUE
		exit sub
	end if
	semantic_model_next_symbol_identity += 1
	sym->semantic_model_identity = semantic_model_next_symbol_identity
end sub

sub fbSemanticModelAllocateSymbol(byval sym as FBSYMBOL ptr)
	if( sym = NULL ) then exit sub
	'' Both ordinary symbols and preliminary procedure headers use the same
	'' pool. Reset allocation identity and cleanup provenance on both paths.
	'' Completing a preliminary header must retain this fresh identity.
	sym->semantic_model_identity = 0
	sym->semantic_dtor_dependency = -1
	sym->semantic_dtor_start_line = 0
	sym->semantic_dtor_start_column = 0
	sym->semantic_dtor_end_line = 0
	sym->semantic_dtor_end_column = 0
	fbSemanticModelInitializeSymbol(sym)
end sub

private function hSemanticModelSymbolHash(byval identity as ulongint) as uinteger
	identity xor= identity shr 4
	identity xor= identity shr 13
	identity xor= identity shr 23
	identity xor= identity shr 37
	return cuint(identity)
end function

private function hSemanticModelGrowSymbolIndex(byval new_capacity as integer) as integer
	dim as SEMANTIC_MODEL_SYMBOL_SLOT ptr new_index
	dim as uinteger slot_index

	new_index = callocate(new_capacity, sizeof(SEMANTIC_MODEL_SYMBOL_SLOT))
	if( new_index = NULL ) then return FALSE

	for i as integer = 0 to semantic_model_symbol_count - 1
		slot_index = hSemanticModelSymbolHash(semantic_model_symbols[i].identity) and _
			(new_capacity - 1)
		while( new_index[slot_index].identity <> 0 )
			slot_index = (slot_index + 1) and (new_capacity - 1)
		wend
		new_index[slot_index].identity = semantic_model_symbols[i].identity
		new_index[slot_index].id = i + 1
	next

	if( semantic_model_symbol_index <> NULL ) then deallocate(semantic_model_symbol_index)
	semantic_model_symbol_index = new_index
	semantic_model_symbol_index_capacity = new_capacity
	return TRUE
end function

private function hSemanticModelEnsureSymbolIndex() as integer
	if( semantic_model_symbol_index_capacity = 0 ) then
		return hSemanticModelGrowSymbolIndex(SEMANTIC_MODEL_INITIAL_SYMBOL_INDEX_CAPACITY)
	end if

	if( (semantic_model_symbol_count + 1) * 2 >= _
		semantic_model_symbol_index_capacity ) then
		if( semantic_model_symbol_index_capacity >= _
			SEMANTIC_MODEL_MAX_SYMBOL_INDEX_CAPACITY ) then
			return FALSE
		end if
		return hSemanticModelGrowSymbolIndex(semantic_model_symbol_index_capacity * 2)
	end if

	return TRUE
end function

private function hSemanticModelFindSymbol(byval sym as FBSYMBOL ptr) as integer
	dim as uinteger slot_index
	dim as ulongint identity = sym->semantic_model_identity

	if( (identity = 0) or (semantic_model_symbol_index_capacity = 0) ) then return -1
	slot_index = hSemanticModelSymbolHash(identity) and _
		(semantic_model_symbol_index_capacity - 1)
	do while( semantic_model_symbol_index[slot_index].identity <> 0 )
		if( semantic_model_symbol_index[slot_index].identity = identity ) then
			return semantic_model_symbol_index[slot_index].id - 1
		end if
		slot_index = (slot_index + 1) and _
			(semantic_model_symbol_index_capacity - 1)
	loop

	return -1
end function

private function hSemanticModelSymbolId(byval sym as FBSYMBOL ptr) as longint
	dim as integer index
	dim as longint symbolid
	dim as longint subtypeid, parentid
	dim as uinteger slot_index
	dim as SEMANTIC_MODEL_SYMBOL ptr resized_symbols
	dim as string symbolname

	if( hSemanticModelHasSymbol(sym) = FALSE ) then
		return 0
	end if
	if( sym->semantic_model_identity = 0 ) then
		fbSemanticModelInitializeSymbol(sym)
		if( sym->semantic_model_identity = 0 ) then return 0
	end if

	index = hSemanticModelFindSymbol(sym)
	if( index >= 0 ) then
		if( semantic_model_symbols[index].state <> 0 ) then
			return semantic_model_total_symbol_count + index + 1
		end if
	else
		if( semantic_model_symbol_count >= SEMANTIC_MODEL_MAX_SYMBOLS ) then
			semantic_model_module_failed = TRUE
			semantic_model_any_failed = TRUE
			return 0
		end if
		if( hSemanticModelEnsureSymbolIndex() = FALSE ) then
			semantic_model_module_failed = TRUE
			semantic_model_any_failed = TRUE
			return 0
		end if

		if( semantic_model_symbol_count >= semantic_model_symbol_capacity ) then
			if( semantic_model_symbol_capacity = 0 ) then
				resized_symbols = callocate(128, _
					sizeof(SEMANTIC_MODEL_SYMBOL))
				if( resized_symbols = NULL ) then
					semantic_model_module_failed = TRUE
					semantic_model_any_failed = TRUE
					return 0
				end if
			else
				resized_symbols = reallocate(semantic_model_symbols, _
					(semantic_model_symbol_capacity * 2 + 16) * sizeof(SEMANTIC_MODEL_SYMBOL))
				if( resized_symbols = NULL ) then
					semantic_model_module_failed = TRUE
					semantic_model_any_failed = TRUE
					return 0
				end if
			end if

			semantic_model_symbols = resized_symbols
			if( semantic_model_symbol_capacity = 0 ) then
				semantic_model_symbol_capacity = 128
			else
				semantic_model_symbol_capacity = (semantic_model_symbol_capacity * 2) + 16
			end if
		end if

		index = semantic_model_symbol_count
		semantic_model_symbols[index].sym = sym
		semantic_model_symbols[index].identity = sym->semantic_model_identity
		semantic_model_symbols[index].state = 0
		semantic_model_symbols[index].visit = 0
		semantic_model_symbols[index].declared = FALSE
		semantic_model_symbols[index].paramvarid = 0
		semantic_model_symbols[index].initializerid = 0
		semantic_model_symbol_count += 1

		slot_index = hSemanticModelSymbolHash(sym->semantic_model_identity) and _
			(semantic_model_symbol_index_capacity - 1)
		while( semantic_model_symbol_index[slot_index].identity <> 0 )
			slot_index = (slot_index + 1) and _
				(semantic_model_symbol_index_capacity - 1)
		wend
		semantic_model_symbol_index[slot_index].identity = sym->semantic_model_identity
		semantic_model_symbol_index[slot_index].id = index + 1
	end if

	symbolid = semantic_model_total_symbol_count + index + 1

	'' Mark before resolving the parent/type links because UDTs and nested
	'' namespaces can refer back to a symbol already being serialized.
	semantic_model_symbols[index].state = 1
	subtypeid = hSemanticModelSymbolId(sym->subtype)
	'' FBSYMBOL.parent describes anonymous UDT layout. Lexical ownership is
	'' the owner of the symbol table, including procedure and scope tables.
	parentid = 0
	if( sym->symtb <> NULL ) then
		if( sym->symtb->owner <> sym ) then parentid = hSemanticModelSymbolId(sym->symtb->owner)
	end if

	if( sym->id.name <> NULL ) then
		symbolname = *sym->id.name
	end if

	hSemanticModelAppendLine("S" + TABCHAR + hSemanticModelNumber(symbolid) + TABCHAR + _
		hSemanticModelEscape(symbolname) + TABCHAR + hSemanticModelNumber(sym->class) + TABCHAR + _
		hSemanticModelNumber(sym->typ) + TABCHAR + hSemanticModelNumber(subtypeid) + TABCHAR + _
		hSemanticModelNumber(sym->scope) + TABCHAR + hSemanticModelNumber(sym->attrib) + TABCHAR + _
		hSemanticModelNumber(sym->pattrib) + TABCHAR + hSemanticModelNumber(sym->lgt) + TABCHAR + _
		hSemanticModelNumber(sym->ofs) + TABCHAR + hSemanticModelNumber(parentid))
	semantic_model_symbols[index].state = 2

	return symbolid
end function

function fbSemanticModelSymbolId(byval sym as FBSYMBOL ptr) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	return hSemanticModelSymbolId(sym)
end function

function fbSemanticModelNextVisit( ) as ulongint
	if( semantic_model_visit = &hFFFFFFFFFFFFFFFFull ) then
		fbSemanticModelFail( )
		return 0
	end if
	semantic_model_visit += 1
	return semantic_model_visit
end function

function fbSemanticModelVisitSymbol(byval sym as FBSYMBOL ptr, byval visit as ulongint) as integer
	if( (visit = 0) or (fbSemanticModelSymbolId(sym) = 0) ) then return FALSE
	dim as integer index = hSemanticModelFindSymbol(sym)
	if( index < 0 ) then return FALSE
	if( semantic_model_symbols[index].visit = visit ) then return FALSE
	semantic_model_symbols[index].visit = visit
	return TRUE
end function

function fbSemanticModelSymbolOrigin(byval sym as FBSYMBOL ptr) as string
	if( symbGetIsRTL(sym) ) then return "runtime"
	dim as integer index = hSemanticModelFindSymbol(sym)
	if( index >= 0 ) then
		if( semantic_model_symbols[index].declared ) then return "source"
	end if
	return "compiler"
end function

sub fbSemanticModelMarkDeclared(byval sym as FBSYMBOL ptr)
	if( fbSemanticModelSymbolId(sym) = 0 ) then exit sub
	dim as integer index = hSemanticModelFindSymbol(sym)
	if( index >= 0 ) then semantic_model_symbols[index].declared = TRUE
end sub

sub fbSemanticModelExportInitializer(byval sym as FBSYMBOL ptr, _
	byval root as ASTNODE ptr, byref kind as const string)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (root = NULL) ) then exit sub
	dim as longint symbolid = hSemanticModelSymbolId(sym)
	if( symbolid = 0 ) then exit sub
	dim as integer index = hSemanticModelFindSymbol(sym)
	if( index < 0 ) then exit sub
	if( semantic_model_symbols[index].initializerid <> 0 ) then exit sub
	dim as longint next_node_id = semantic_model_node_count + semantic_model_module_node_count
	dim as longint rootid = fbSemanticModelExportTree(root, symbolid, next_node_id, 0, NULL)
	semantic_model_symbols[index].initializerid = rootid
	if( rootid = 0 ) then exit sub
	fbSemanticModelAppendDetail("H" + TABCHAR + "symbol" + TABCHAR + _
		hSemanticModelNumber(symbolid) + TABCHAR + "node" + TABCHAR + _
		hSemanticModelNumber(rootid) + TABCHAR + kind + TABCHAR + "0")
end sub

function fbSemanticModelParameterVariable(byval param as FBSYMBOL ptr, _
	byval variables_live as integer) as longint
	if( fbSemanticModelSymbolId(param) = 0 ) then return 0
	dim as integer index = hSemanticModelFindSymbol(param)
	if( index < 0 ) then return 0
	if( variables_live andalso hSemanticModelHasSymbol(param->param.var) ) then
		'' Flushing deletes locals while retaining the formal signature. Save
		'' the ID now; never dereference the stale param.var pointer later.
		dim as longint variableid = hSemanticModelSymbolId(param->param.var)
		semantic_model_symbols[index].paramvarid = variableid
		if( semantic_model_symbols[index].declared ) then
			dim as integer variable_index = hSemanticModelFindSymbol(param->param.var)
			if( variable_index >= 0 ) then semantic_model_symbols[variable_index].declared = TRUE
		end if
	end if
	return semantic_model_symbols[index].paramvarid
end function

'' Export one compiler-resolved identifier occurrence with its exact lexer span.
'' Physical-origin status lets editor consumers refuse macro-expanded ranges.
sub fbSemanticModelExportBinding _
	( _
		byval sym as FBSYMBOL ptr, _
		byref source as LEX_LOCATION, _
		byval is_declaration as integer _
	)

	if( (semantic_model_file_open = FALSE) or _
		(semantic_model_module_open = FALSE) or _
		(semantic_model_expressions_only) or _
		(semantic_model_module_failed) or _
		(hSemanticModelHasSymbol(sym) = FALSE) ) then
		exit sub
	end if

	if( (source.start_line < 1) or (source.start_column < 0) or _
		(source.end_line < source.start_line) or (source.end_column < 0) or _
		((source.end_line = source.start_line) and _
		 (source.end_column <= source.start_column)) or _
		(len(source.source_file) = 0) ) then
		exit sub
	end if

	if( semantic_model_binding_count + _
		semantic_model_module_binding_count >= _
		SEMANTIC_MODEL_MAX_BINDINGS_PER_MODEL ) then
		semantic_model_module_failed = TRUE
		semantic_model_any_failed = TRUE
		exit sub
	end if

	dim as integer physical_range = hSemanticModelLocationIsPhysical(source)
	dim as longint symbolid = hSemanticModelSymbolId(sym)
	if( (symbolid = 0) or (semantic_model_module_failed) ) then exit sub
	if( is_declaration ) then
		dim as integer index = hSemanticModelFindSymbol(sym)
		if( index >= 0 ) then semantic_model_symbols[index].declared = TRUE
	end if

	hSemanticModelAppendLine("B" + TABCHAR + hSemanticModelNumber(symbolid) + _
		TABCHAR + iif(is_declaration, "declaration", "reference") + _
		TABCHAR + hSemanticModelNumber(physical_range) + _
		TABCHAR + hSemanticModelEscape(source.source_file) + _
		TABCHAR + hSemanticModelNumber(source.start_line) + _
		TABCHAR + hSemanticModelNumber(source.start_column) + _
		TABCHAR + hSemanticModelNumber(source.end_line) + _
		TABCHAR + hSemanticModelNumber(source.end_column))
	if( semantic_model_module_failed = FALSE ) then
		semantic_model_module_binding_count += 1
	end if
end sub

'' Export a compiler-selected call that has no written callee token.
'' The owner range is a physical source anchor, not an editable call spelling.
private function hSemanticModelCallSignature _
	( _
		byval target as FBSYMBOL ptr, _
		byval owner_type as FBSYMBOL ptr _
	) as string

	dim as FBSYMBOL ptr param
	dim as FBSYMBOL ptr param_subtype
	dim as string signature = "sig2"
	dim as string mode_name, signature_part
	dim as integer param_optional, param_dimensions
	dim as longint param_subtype_id
	dim as integer skip_instance_param

	if( (hSemanticModelHasSymbol(target) = FALSE) or _
		(symbIsProc(target) = FALSE) or _
		(hSemanticModelHasSymbol(owner_type) = FALSE) ) then
		exit function
	end if

	param = symbGetProcHeadParam(target)
	skip_instance_param = symbIsMethod(target)
	while( hSemanticModelHasSymbol(param) )
		if( skip_instance_param ) then
			'' Methods store their compiler-generated instance parameter first.
			skip_instance_param = FALSE
		else
			select case symbGetParamMode(param)
			case FB_PARAMMODE_BYVAL
				mode_name = "byval"
			case FB_PARAMMODE_BYREF
				mode_name = "byref"
			case FB_PARAMMODE_BYDESC
				mode_name = "bydesc"
			case FB_PARAMMODE_VARARG
				mode_name = "vararg"
			case else
				exit function
			end select

			'' The formatted type spelling changes during a legal UDT rename. Keep
			'' the selected signature tied to the exact type symbol in this model;
			'' the preceding S record makes this identity available to consumers.
			param_subtype = symbGetSubtype(param)
			param_subtype_id = 0
			if( param_subtype <> NULL ) then
				if( hSemanticModelHasSymbol(param_subtype) = FALSE ) then exit function
				param_subtype_id = hSemanticModelSymbolId(param_subtype)
				if( param_subtype_id = 0 ) then exit function
			end if
			if( semantic_model_module_failed ) then exit function

			'' FreeBASIC TRUE is -1; the serialized signature uses stable 0/1 flags.
			param_optional = abs(symbParamIsOptional(param) <> FALSE)
			'' BYDESC dimensions -1 means the parameter accepts arrays of any rank.
			param_dimensions = param->param.bydescdimensions
			signature_part = "|" + mode_name + "|" + _
				hSemanticModelNumber(param_optional) + "|" + _
				hSemanticModelNumber(param_dimensions) + "|" + _
				hSemanticModelNumber(symbGetFullType(param)) + "|" + _
				hSemanticModelNumber(param_subtype_id)
			if( len(signature) > _
				SEMANTIC_MODEL_MAX_CALL_SIGNATURE_BYTES - len(signature_part) ) then
				fbSemanticModelFail( )
				exit function
			end if
			signature += signature_part
		end if
		param = symbGetParamNext(param)
	wend

	return signature
end function

private function hSemanticModelImplicitCallKindSupported(byref call_kind as const string) as integer
	select case call_kind
	case "default-constructor", "initializer-constructor", "new-constructor", _
		"destructor-call", "argument-constructor", "temporary-destructor", _
		"scope-exit-destructor", "delete-destructor", "return-constructor"
		return TRUE
	case else
		return FALSE
	end select
end function

private sub hSemanticModelEmitImplicitCall _
	( _
		byval owner as FBSYMBOL ptr, _
		byval target as FBSYMBOL ptr, _
		byref call_kind as const string, _
		byref source as LEX_LOCATION, _
		byval source_is_physical as integer _
	)

	if( hSemanticModelImplicitCallKindSupported(call_kind) = FALSE ) then exit sub
	if( (semantic_model_file_open = FALSE) or _
		(semantic_model_module_open = FALSE) or _
		(semantic_model_expressions_only) or _
		(semantic_model_module_failed) or _
		(hSemanticModelHasSymbol(owner) = FALSE) or _
		(hSemanticModelHasSymbol(target) = FALSE) ) then
		exit sub
	end if

	if( (source.start_line < 1) or (source.start_column < 0) or _
		(source.end_line < source.start_line) or (source.end_column < 0) or _
		((source.end_line = source.start_line) and _
		 (source.end_column <= source.start_column)) or _
		(len(source.source_file) = 0) ) then
		exit sub
	end if

	if( semantic_model_implicit_call_count + _
		semantic_model_module_implicit_call_count >= _
		SEMANTIC_MODEL_MAX_IMPLICIT_CALLS_PER_MODEL ) then
		semantic_model_module_failed = TRUE
		semantic_model_any_failed = TRUE
		exit sub
	end if

	dim as FBSYMBOL ptr owner_type = symbGetSubtype(owner)
	if( symbIsStruct(owner) ) then owner_type = owner
	if( hSemanticModelHasSymbol(owner_type) = FALSE ) then exit sub
	dim as string target_signature = hSemanticModelCallSignature(target, owner_type)
	if( len(target_signature) = 0 ) then exit sub
	dim as longint ownerid = hSemanticModelSymbolId(owner)
	dim as longint targetid = hSemanticModelSymbolId(target)
	dim as longint typeid = hSemanticModelSymbolId(owner_type)
	if( (ownerid = 0) or (targetid = 0) or (typeid = 0) or _
		(semantic_model_module_failed) ) then exit sub

	hSemanticModelAppendLine("I" + TABCHAR + hSemanticModelNumber(ownerid) + _
		TABCHAR + hSemanticModelNumber(targetid) + _
		TABCHAR + hSemanticModelNumber(typeid) + TABCHAR + call_kind + _
		TABCHAR + hSemanticModelNumber(abs(source_is_physical <> FALSE)) + _
		TABCHAR + hSemanticModelEscape(source.source_file) + _
		TABCHAR + hSemanticModelNumber(source.start_line) + _
		TABCHAR + hSemanticModelNumber(source.start_column) + _
		TABCHAR + hSemanticModelNumber(source.end_line) + _
		TABCHAR + hSemanticModelNumber(source.end_column) + _
		TABCHAR + hSemanticModelEscape(target_signature))
	if( semantic_model_module_failed = FALSE ) then
		semantic_model_module_implicit_call_count += 1
	end if
end sub

sub fbSemanticModelExportImplicitCall _
	( _
		byval owner as FBSYMBOL ptr, _
		byval target as FBSYMBOL ptr, _
		byval call_kind as string, _
		byref source as LEX_LOCATION _
	)
	if( hSemanticModelImplicitCallKindSupported(call_kind) = FALSE ) then exit sub
	hSemanticModelEmitImplicitCall(owner, target, call_kind, source, _
		hSemanticModelLocationIsPhysical(source))
end sub

sub fbSemanticModelSetPendingScopeExitSource _
	( _
		byref source as LEX_LOCATION, _
		byval nonphysical_tokens as longint _
	)
	semantic_model_pending_scope_exit_valid = FALSE
	if( (semantic_model_file_open = FALSE) or _
		(semantic_model_module_open = FALSE) or _
		(semantic_model_expressions_only) or _
		(semantic_model_module_failed) ) then
		exit sub
	end if
	semantic_model_pending_scope_exit_source = source
	semantic_model_pending_scope_exit_nonphysical = nonphysical_tokens
	semantic_model_pending_scope_exit_valid = TRUE
end sub

sub fbSemanticModelAttachPendingScopeExitSource(byval branch as ASTNODE ptr)
	if( branch = NULL ) then
		semantic_model_pending_scope_exit_valid = FALSE
		exit sub
	end if

	branch->break.semantic_dependency = -1
	branch->break.semantic_start_line = 0
	branch->break.semantic_start_column = 0
	branch->break.semantic_end_line = 0
	branch->break.semantic_end_column = 0
	if( semantic_model_pending_scope_exit_valid = FALSE ) then exit sub

	dim as LEX_LOCATION source = semantic_model_pending_scope_exit_source
	semantic_model_pending_scope_exit_valid = FALSE
	if( (source.start_line < 1) or (source.start_column < 0) or _
		(source.end_line < source.start_line) or (source.end_column < 0) or _
		((source.end_line = source.start_line) and _
		 (source.end_column <= source.start_column)) or _
		(len(source.source_file) = 0) or _
		(hSemanticModelLocationIsPhysical(source) = FALSE) or _
		(lexGetNonphysicalTokenCount() <> semantic_model_pending_scope_exit_nonphysical) or _
		(hSemanticModelRangeFitsCurrentSourceLine(source) = FALSE) ) then
		exit sub
	end if

	fbSemanticModelAddDependency(source.source_file)
	dim as integer source_dependency = -1
	for index as integer = 0 to semantic_model_dependency_count - 1
		if( semantic_model_dependencies(index) = source.source_file ) then
			source_dependency = index
			exit for
		end if
	next
	if( source_dependency < 0 ) then exit sub

	branch->break.semantic_dependency = source_dependency
	branch->break.semantic_start_line = source.start_line
	branch->break.semantic_start_column = source.start_column
	branch->break.semantic_end_line = source.end_line
	branch->break.semantic_end_column = source.end_column
end sub

sub fbSemanticModelExportScopeExitDestructor _
	( _
		byval owner as FBSYMBOL ptr, _
		byval target as FBSYMBOL ptr, _
		byval source_dependency as integer, _
		byval start_line as integer, _
		byval start_column as integer, _
		byval end_line as integer, _
		byval end_column as integer _
	)

	if( (source_dependency < 0) or _
		(source_dependency >= semantic_model_dependency_count) ) then
		exit sub
	end if
	dim as LEX_LOCATION source
	source.source_file = semantic_model_dependencies(source_dependency)
	source.start_line = start_line
	source.start_column = start_column
	source.end_line = end_line
	source.end_column = end_column
	source.is_physical = TRUE
	hSemanticModelEmitImplicitCall(owner, target, "scope-exit-destructor", source, TRUE)
end sub

sub fbSemanticModelExportTemporaryDestructor _
	( _
		byval owner as FBSYMBOL ptr, _
		byval target as FBSYMBOL ptr, _
		byval source_dependency as integer, _
		byval start_line as integer, _
		byval start_column as integer, _
		byval end_line as integer, _
		byval end_column as integer _
	)
	dim as LEX_LOCATION source
	if( (source_dependency < 0) or _
		(source_dependency >= semantic_model_dependency_count) ) then
		exit sub
	end if
	source.source_file = semantic_model_dependencies(source_dependency)
	source.start_line = start_line
	source.start_column = start_column
	source.end_line = end_line
	source.end_column = end_column
	source.is_physical = TRUE
	fbSemanticModelExportImplicitCall(owner, target, "temporary-destructor", source)
end sub

private function hSemanticModelTypeKind(byval sym as FBSYMBOL ptr) as string
	dim as integer dtype

	if( sym = NULL ) then return "other"

	dtype = sym->typ
	if( (typeGetPtrCnt(dtype) > 0) or _
		(typeGet(dtype) = FB_DATATYPE_POINTER) ) then
		return "pointer"
	end if
	'' CHAR/WCHAR are the language's ZSTRING/WSTRING storage types even
	'' though their individual code units belong to the integer data class.
	if( (typeGet(dtype) = FB_DATATYPE_CHAR) or (typeGet(dtype) = FB_DATATYPE_WCHAR) ) then
		return "fixed-string"
	end if

	select case typeGetClass(dtype)
	case FB_DATACLASS_INTEGER, FB_DATACLASS_FPOINT
		return "numeric"
	case FB_DATACLASS_STRING
		if( typeIsDynamicString( typeGet(dtype) ) ) then
			return "dynamic-string"
		end if
		return "fixed-string"
	case FB_DATACLASS_UDT
		return "aggregate"
	case FB_DATACLASS_PROC
		return "procedure"
	case else
		return "other"
	end select
end function

function fbSemanticModelTypeKind(byval sym as FBSYMBOL ptr) as string
	return hSemanticModelTypeKind(sym)
end function

private sub hSemanticModelExportVariableType(byval sym as FBSYMBOL ptr, _
	byref procname as string)
	dim as longint symbolid
	dim as string variablename

	if( hSemanticModelHasSymbol(sym) = FALSE ) then exit sub
	if( symbIsVar(sym) = FALSE ) then exit sub
	if( semantic_model_module_failed ) then exit sub
	if( sym->id.name = NULL ) then exit sub

	symbolid = hSemanticModelSymbolId(sym)
	if( (symbolid = 0) or (semantic_model_module_failed) ) then exit sub

	variablename = *sym->id.name
	hSemanticModelAppendLine("V" + TABCHAR + hSemanticModelNumber(symbolid) + TABCHAR + _
		hSemanticModelEscape(procname) + TABCHAR + hSemanticModelEscape(variablename) + _
		TABCHAR + hSemanticModelTypeKind(sym))
	if( semantic_model_module_failed = FALSE ) then
		semantic_model_module_type_fact_count += 1
	end if
end sub

sub fbSemanticModelExportVariable(byval sym as FBSYMBOL ptr)
	dim as string procname
	dim as FBSYMBOL ptr owner = sym
	'' Nested scopes belong to their nearest procedure. Namespace globals
	'' keep an empty procedure name, as in earlier sidecar schemas.
	for depth as integer = 0 to FB_MAXSCOPEDEPTH
		if( (owner = NULL) or (owner->symtb = NULL) ) then exit for
		owner = owner->symtb->owner
		if( owner = NULL ) then exit for
		if( symbIsProc(owner) ) then
			if( owner->id.name <> NULL ) then procname = *owner->id.name
			exit for
		end if
	next
	hSemanticModelExportVariableType(sym, procname)
end sub

private function hSemanticModelNodeOperator(byval node as ASTNODE ptr) as integer
	select case node->class
	case AST_NODECLASS_ASSIGN, AST_NODECLASS_BOP, AST_NODECLASS_UOP, _
		AST_NODECLASS_ADDROF, AST_NODECLASS_BRANCH
		return node->op.op
	case AST_NODECLASS_MEM
		return node->mem.op
	case AST_NODECLASS_CONV
		'' CONV uses AST_NODE_CAST, not AST_NODE_OP. Its union member contains
		'' conversion flags, so there is no raw operator code to report.
		return -1
	case AST_NODECLASS_DBG
		return node->dbg.op
	case else
		'' Most node classes do not carry an AST operator payload.
		return -1
	end select
end function

private function hSemanticModelConceptOperatorCode(byval op as integer) as string
	'' These codes are the editor-facing vocabulary; AST operator IDs remain
	'' compiler diagnostics and are not a stable external interface.
	select case op
	case AST_OP_ASSIGN: return "assign"
	case AST_OP_ADD_SELF: return "add-assign"
	case AST_OP_SUB_SELF: return "subtract-assign"
	case AST_OP_MUL_SELF: return "multiply-assign"
	case AST_OP_DIV_SELF: return "divide-assign"
	case AST_OP_INTDIV_SELF: return "integer-divide-assign"
	case AST_OP_MOD_SELF: return "modulo-assign"
	case AST_OP_AND_SELF: return "and-assign"
	case AST_OP_OR_SELF: return "or-assign"
	case AST_OP_ANDALSO_SELF: return "logical-and-assign"
	case AST_OP_ORELSE_SELF: return "logical-or-assign"
	case AST_OP_XOR_SELF: return "xor-assign"
	case AST_OP_EQV_SELF: return "equivalence-assign"
	case AST_OP_IMP_SELF: return "implication-assign"
	case AST_OP_SHL_SELF: return "shift-left-assign"
	case AST_OP_SHR_SELF: return "shift-right-assign"
	case AST_OP_POW_SELF: return "power-assign"
	case AST_OP_CONCAT_SELF: return "concatenate-assign"
	case AST_OP_ADD: return "add"
	case AST_OP_SUB: return "subtract"
	case AST_OP_MUL: return "multiply"
	case AST_OP_DIV: return "divide"
	case AST_OP_INTDIV: return "integer-divide"
	case AST_OP_MOD: return "modulo"
	case AST_OP_AND: return "and"
	case AST_OP_OR: return "or"
	case AST_OP_ANDALSO: return "logical-and"
	case AST_OP_ORELSE: return "logical-or"
	case AST_OP_XOR: return "xor"
	case AST_OP_EQV: return "equivalence"
	case AST_OP_IMP: return "implication"
	case AST_OP_SHL: return "shift-left"
	case AST_OP_SHR: return "shift-right"
	case AST_OP_POW: return "power"
	case AST_OP_CONCAT: return "concatenate"
	case AST_OP_EQ: return "equal"
	case AST_OP_GT: return "greater-than"
	case AST_OP_LT: return "less-than"
	case AST_OP_NE: return "not-equal"
	case AST_OP_GE: return "greater-or-equal"
	case AST_OP_LE: return "less-or-equal"
	case AST_OP_IS: return "identity-test"
	case AST_OP_NOT: return "not"
	case AST_OP_BOOLNOT: return "logical-not"
	case AST_OP_PLUS: return "unary-plus"
	case AST_OP_NEG: return "negate"
	case AST_OP_ADDROF: return "address-of"
	case AST_OP_DEREF: return "dereference"
	case AST_OP_PTRINDEX: return "index"
	case AST_OP_CAST: return "cast"
	case AST_OP_FLDDEREF: return "member-access"
	case AST_OP_LEN: return "length"
	case AST_OP_FOR: return "iterator-initialize"
	case AST_OP_STEP: return "iterator-step"
	case AST_OP_NEXT: return "iterator-next"
	case AST_OP_TOINT: return "convert-to-integer"
	case AST_OP_TOFLT: return "convert-to-float"
	case AST_OP_TOBOOL: return "convert-to-boolean"
	case AST_OP_TOSIGNED: return "convert-to-signed"
	case AST_OP_TOUNSIGNED: return "convert-to-unsigned"
	case AST_OP_ABS: return "absolute-value"
	case AST_OP_SGN: return "sign"
	case AST_OP_SIN: return "sine"
	case AST_OP_ASIN: return "arcsine"
	case AST_OP_COS: return "cosine"
	case AST_OP_ACOS: return "arccosine"
	case AST_OP_TAN: return "tangent"
	case AST_OP_ATAN: return "arctangent"
	case AST_OP_ATAN2: return "arctangent2"
	case AST_OP_SQRT: return "square-root"
	case AST_OP_RSQRT: return "reciprocal-square-root"
	case AST_OP_RCP: return "reciprocal"
	case AST_OP_LOG: return "logarithm"
	case AST_OP_EXP: return "exponential"
	case AST_OP_FLOOR: return "floor"
	case AST_OP_FIX: return "truncate"
	case AST_OP_FRAC: return "fractional-part"
	case AST_OP_NEW: return "allocate"
	case AST_OP_NEW_VEC: return "allocate-array"
	case AST_OP_DEL: return "deallocate"
	case AST_OP_DEL_VEC: return "deallocate-array"
	case AST_OP_NEW_SELF: return "allocate"
	case AST_OP_NEW_VEC_SELF: return "allocate-array"
	case AST_OP_DEL_SELF: return "deallocate"
	case AST_OP_DEL_VEC_SELF: return "deallocate-array"
	case else
		return ""
	end select
end function

function fbSemanticModelOperatorCode(byval op as integer) as string
	return hSemanticModelConceptOperatorCode(op)
end function

sub fbSemanticModelExportOperation(byval expression as ASTNODE ptr, _
	byval op as integer, byref source as LEX_LOCATION)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (expression = NULL) ) then exit sub
	dim as string code = hSemanticModelConceptOperatorCode(op)
	if( len(code) = 0 ) then exit sub
	if( (source.start_line < 1) or (source.start_column < 0) or _
		(source.end_line < source.start_line) or (source.end_column < 0) or _
		((source.end_line = source.start_line) and (source.end_column <= source.start_column)) ) then exit sub
	dim as string kind = "builtin"
	dim as longint targetid = 0
	'' Value-producing wrappers may surround a selected overloaded operation.
	'' Follow only their result edge, so nested operand calls cannot become the
	'' callee of the outer source operator.
	dim as ASTNODE ptr node = expression
	for depth as integer = 0 to 64
		if( node = NULL ) then exit for
		select case node->class
		case AST_NODECLASS_CALL
			if( hSemanticModelHasSymbol(node->sym) andalso symbIsOperator(node->sym) ) then
				kind = "overloaded"
				targetid = hSemanticModelSymbolId(node->sym)
			end if
			exit for
		case AST_NODECLASS_CALLCTOR
			node = node->l
		case AST_NODECLASS_CONV
			node = node->l
		case AST_NODECLASS_LINK
			select case node->link.ret
			case AST_LINK_RETURN_LEFT: node = node->l
			case AST_LINK_RETURN_RIGHT: node = node->r
			case else: exit for
			end select
		case else
			exit for
		end select
	next
	dim as integer physical = abs(hSemanticModelLocationIsPhysical(source) and _
		hSemanticModelRangeFitsCurrentSourceLine(source))
	fbSemanticModelAppendDetail("O" + TABCHAR + code + TABCHAR + kind + TABCHAR + _
		hSemanticModelNumber(targetid) + TABCHAR + hSemanticModelNumber(physical) + TABCHAR + _
		hSemanticModelEscape(source.source_file) + TABCHAR + hSemanticModelNumber(source.start_line) + _
		TABCHAR + hSemanticModelNumber(source.start_column) + TABCHAR + _
		hSemanticModelNumber(source.end_line) + TABCHAR + hSemanticModelNumber(source.end_column))
end sub

private function hSemanticModelConceptOperator(byval node as ASTNODE ptr, _
	byref operator_kind as string) as string
	dim as integer op
	dim as string operator_code

	operator_kind = "none"
	if( node = NULL ) then return ""

	if( (node->class = AST_NODECLASS_CALL) or _
		(node->class = AST_NODECLASS_CALLCTOR) ) then
		if( hSemanticModelHasSymbol(node->sym) = FALSE ) then return ""
		if( (symbIsProc(node->sym) = FALSE) or _
			(symbIsOperator(node->sym) = FALSE) ) then return ""
		op = symbGetProcOpOvl(node->sym)
		operator_kind = "overloaded"
	else
		select case node->class
		case AST_NODECLASS_ASSIGN
			op = AST_OP_ASSIGN
		case AST_NODECLASS_IDX
			operator_kind = "builtin"
			return "index"
		case AST_NODECLASS_FIELD
			operator_kind = "builtin"
			return "member-access"
		case AST_NODECLASS_DEREF
			op = AST_OP_DEREF
		case AST_NODECLASS_BOP, AST_NODECLASS_UOP, _
			AST_NODECLASS_ADDROF, AST_NODECLASS_MEM
			op = hSemanticModelNodeOperator(node)
		case else
			return ""
		end select
		operator_kind = "builtin"
	end if

	operator_code = hSemanticModelConceptOperatorCode(op)
	if( len(operator_code) = 0 ) then operator_kind = "none"
	return operator_code
end function

private function hSemanticModelSourceCodeUnitWidth() as integer
	'' Source encodings use power-of-two code units: 1, 2 or 4 bytes.
	select case env.inf.format
	case FBFILE_FORMAT_UTF16LE, FBFILE_FORMAT_UTF16BE
		return 2
	case FBFILE_FORMAT_UTF32LE, FBFILE_FORMAT_UTF32BE
		return 4
	case else
		return 1
	end select
end function

private function hSemanticModelSourceBOMLength() as integer
	select case env.inf.format
	case FBFILE_FORMAT_UTF8
		return 3
	case FBFILE_FORMAT_UTF16LE, FBFILE_FORMAT_UTF16BE
		return 2
	case FBFILE_FORMAT_UTF32LE, FBFILE_FORMAT_UTF32BE
		return 4
	case else
		return 0
	end select
end function

private function hSemanticModelSourceLineLimitBytes() as longint
	dim as integer bytes_per_column = 1
	select case env.inf.format
	case FBFILE_FORMAT_UTF8, FBFILE_FORMAT_UTF32LE, FBFILE_FORMAT_UTF32BE
		bytes_per_column = 4
	case FBFILE_FORMAT_UTF16LE, FBFILE_FORMAT_UTF16BE
		bytes_per_column = 2
	end select
	return culngint(LEX_MAXBUFFCHARS) * bytes_per_column
end function

private function hSemanticModelReadSourceCodeUnit _
	( _
		byref source_text as string, _
		byval offset as integer _
	) as ulongint

	dim as ubyte ptr source_bytes = cast(ubyte ptr, strptr(source_text))
	select case env.inf.format
	case FBFILE_FORMAT_UTF16LE
		return culngint(source_bytes[offset]) or _
			(culngint(source_bytes[offset + 1]) shl 8)
	case FBFILE_FORMAT_UTF16BE
		return (culngint(source_bytes[offset]) shl 8) or _
			culngint(source_bytes[offset + 1])
	case FBFILE_FORMAT_UTF32LE
		return culngint(source_bytes[offset]) or _
			(culngint(source_bytes[offset + 1]) shl 8) or _
			(culngint(source_bytes[offset + 2]) shl 16) or _
			(culngint(source_bytes[offset + 3]) shl 24)
	case FBFILE_FORMAT_UTF32BE
		return (culngint(source_bytes[offset]) shl 24) or _
			(culngint(source_bytes[offset + 1]) shl 16) or _
			(culngint(source_bytes[offset + 2]) shl 8) or _
			culngint(source_bytes[offset + 3])
	case else
		return source_bytes[offset]
	end select
end function

private function hSemanticModelUTF8SourceColumnCount(byref source_line as string) as integer
	dim as ubyte ptr source_bytes = cast(ubyte ptr, strptr(source_line))
	dim as integer column = 0, index = 0

	do while( index < len(source_line) )
		dim as integer first_byte = source_bytes[index]
		if( first_byte <= &h7F ) then
			column += 1
			index += 1
		elseif( (first_byte >= &hC2) and (first_byte <= &hDF) ) then
			if( index + 1 >= len(source_line) ) then return -1
			if( (source_bytes[index + 1] < &h80) or _
				(source_bytes[index + 1] > &hBF) ) then return -1
			column += 1
			index += 2
		elseif( (first_byte >= &hE0) and (first_byte <= &hEF) ) then
			if( index + 2 >= len(source_line) ) then return -1
			dim as integer second_byte3 = source_bytes[index + 1]
			dim as integer third_byte3 = source_bytes[index + 2]
			if( (second_byte3 < &h80) or (second_byte3 > &hBF) or _
				(third_byte3 < &h80) or (third_byte3 > &hBF) ) then return -1
			if( ((first_byte = &hE0) and (second_byte3 < &hA0)) or _
				((first_byte = &hED) and (second_byte3 > &h9F)) ) then return -1
			column += 1
			index += 3
		elseif( (first_byte >= &hF0) and (first_byte <= &hF4) ) then
			if( index + 3 >= len(source_line) ) then return -1
			dim as integer second_byte4 = source_bytes[index + 1]
			dim as integer third_byte4 = source_bytes[index + 2]
			dim as integer fourth_byte4 = source_bytes[index + 3]
			if( (second_byte4 < &h80) or (second_byte4 > &hBF) or _
				(third_byte4 < &h80) or (third_byte4 > &hBF) or _
				(fourth_byte4 < &h80) or (fourth_byte4 > &hBF) ) then return -1
			if( ((first_byte = &hF0) and (second_byte4 < &h90)) or _
				((first_byte = &hF4) and (second_byte4 > &h8F)) ) then return -1
			column += 2
			index += 4
		else
			return -1
		end if
	loop

	return column
end function

private function hSemanticModelSourceColumnCount(byref source_line as string) as integer
	if( len(source_line) = 0 ) then return 0
	if( env.inf.format = FBFILE_FORMAT_UTF8 ) then
		return hSemanticModelUTF8SourceColumnCount(source_line)
	end if
	dim as integer unit_width = hSemanticModelSourceCodeUnitWidth()
	if( (len(source_line) And (unit_width - 1)) <> 0 ) then return -1

	dim as integer column = 0
	if( (env.inf.format = FBFILE_FORMAT_UTF16LE) or _
		(env.inf.format = FBFILE_FORMAT_UTF16BE) ) then
		dim as integer index = 0
		do while( index < len(source_line) )
			dim as ulongint code_unit = hSemanticModelReadSourceCodeUnit(source_line, index)
			if( (code_unit >= &hD800) and (code_unit <= &hDBFF) ) then
				if( index + 2 >= len(source_line) ) then return -1
				dim as ulongint low_surrogate = hSemanticModelReadSourceCodeUnit(source_line, index + 2)
				if( (low_surrogate < &hDC00) or (low_surrogate > &hDFFF) ) then return -1
				column += 2
				index += 4
			elseif( (code_unit >= &hDC00) and (code_unit <= &hDFFF) ) then
				return -1
			else
				column += 1
				index += 2
			end if
		loop
		return column
	end if

	if( (env.inf.format = FBFILE_FORMAT_UTF32LE) or _
		(env.inf.format = FBFILE_FORMAT_UTF32BE) ) then
		for index as integer = 0 to len(source_line) - 1 step 4
			dim as ulongint codepoint = hSemanticModelReadSourceCodeUnit(source_line, index)
			if( (codepoint > &h10FFFF) or _
				((codepoint >= &hD800) and (codepoint <= &hDFFF)) ) then
				return -1
			end if
			column += iif(codepoint > &hFFFF, 2, 1)
		next
		return column
	end if

	dim as ubyte ptr source_bytes = cast(ubyte ptr, strptr(source_line))
	dim as integer utf8_continuations_left = 0
	for index as integer = 0 to len(source_line) - 1
		dim as integer source_byte = source_bytes[index]
		if( utf8_continuations_left > 0 ) then
			if( (source_byte >= &h80) and (source_byte <= &hBF) ) then
				utf8_continuations_left -= 1
				continue for
			end if
			utf8_continuations_left = 0
		end if

		select case source_byte
		case &hC2 to &hDF
			column += 1
			utf8_continuations_left = 1
		case &hE0 to &hEF
			column += 1
			utf8_continuations_left = 2
		case &hF0 to &hF4
			column += 2
			utf8_continuations_left = 3
		case else
			column += 1
		end select
	next

	return column

end function

private function hSemanticModelReadCurrentSourceLine _
	( _
		byval current_physical_line as integer, _
		byref source_line as string _
	) as integer

	const READ_CHUNK_BYTES = 1024
	dim as longint old_file_position = seek(env.inf.num)
	dim as longint file_length = lof(env.inf.num)
	dim as longint scan_position, line_start
	dim as integer unit_width = hSemanticModelSourceCodeUnitWidth()
	dim as integer bom_length = hSemanticModelSourceBOMLength()
	dim as longint max_line_bytes = hSemanticModelSourceLineLimitBytes()
	dim as integer chunk_length, index, scan_line
	dim as integer valid = TRUE, found_line_end = FALSE
	dim as string chunk, line_feed_unit
	dim as ulongint source_char

	function = FALSE
	source_line = ""
	if( (current_physical_line < 1) or (file_length < bom_length) or _
		(((file_length - bom_length) And (unit_width - 1)) <> 0) ) then
		seek #env.inf.num, old_file_position
		exit function
	end if

	'' Keep a forward-only physical line cursor so checking many consecutive
	'' expressions does not rescan the entire encoded file for each line.
	if( semantic_model_source_scan_valid and _
		(semantic_model_source_scan_file = env.inf.name) and _
		(semantic_model_source_scan_format = env.inf.format) and _
		(current_physical_line >= semantic_model_source_scan_line) ) then
		scan_line = semantic_model_source_scan_line
		scan_position = semantic_model_source_scan_filepos
	else
		scan_line = 1
		scan_position = bom_length
	end if

	do while( (valid) and (scan_line <= current_physical_line) )
		line_start = scan_position
		found_line_end = FALSE
		source_line = ""
			do while( (valid) and (found_line_end = FALSE) and _
				(scan_position < file_length) )
			chunk_length = iif(file_length - scan_position > READ_CHUNK_BYTES, _
				READ_CHUNK_BYTES, file_length - scan_position)
			chunk_length -= (chunk_length And (unit_width - 1))
			if( chunk_length <= 0 ) then
				valid = FALSE
				exit do
			end if
			chunk = space(chunk_length)
			if( get(#env.inf.num, scan_position + 1, chunk) <> 0 ) then
				valid = FALSE
				exit do
			end if
			for index = 0 to chunk_length - unit_width step unit_width
				source_char = hSemanticModelReadSourceCodeUnit(chunk, index)
				if( (source_char = 10) or (source_char = 13) ) then
					if( scan_line = current_physical_line ) then
						if( scan_position - line_start + index > max_line_bytes ) then
							valid = FALSE
						elseif( index > 0 ) then
							source_line += left(chunk, index)
						end if
					end if
					scan_position += index + unit_width
					if( (source_char = 13) and _
						(scan_position + unit_width <= file_length) ) then
						line_feed_unit = space(unit_width)
						if( get(#env.inf.num, scan_position + 1, line_feed_unit) <> 0 ) then
							valid = FALSE
						elseif( hSemanticModelReadSourceCodeUnit(line_feed_unit, 0) = 10 ) then
							scan_position += unit_width
						end if
					end if
					found_line_end = TRUE
					exit for
				end if
			next
			if( (valid) and (found_line_end = FALSE) ) then
				scan_position += chunk_length
				if( scan_line = current_physical_line ) then
					if( scan_position - line_start > max_line_bytes ) then
						valid = FALSE
					else
						source_line += chunk
					end if
				end if
			end if
		loop

		if( (valid = FALSE) or (scan_line = current_physical_line) ) then
			exit do
		end if
		if( found_line_end = FALSE ) then
			valid = FALSE
			exit do
		end if
		scan_line += 1
	loop

	seek #env.inf.num, old_file_position
	if( valid and (scan_line = current_physical_line) ) then
		semantic_model_source_scan_valid = TRUE
		semantic_model_source_scan_file = env.inf.name
		semantic_model_source_scan_format = env.inf.format
		semantic_model_source_scan_line = current_physical_line + 1
		semantic_model_source_scan_filepos = scan_position
		return TRUE
	end if
	source_line = ""
	return FALSE

end function

private function hSemanticModelRangeFitsCurrentSourceLine _
	( _
		byref source_end as LEX_LOCATION _
	) as integer

	if( source_end.source_file <> env.inf.name ) then return TRUE
	if( source_end.end_line <> lex.ctx->linenum ) then return TRUE
	if( lex.ctx->physical_linenum < 1 ) then return FALSE
	'' #line can reuse a logical filename and line number for a different
	'' physical line, so include the physical line number in the cache key.
	if( semantic_model_source_bound_valid and _
		(semantic_model_source_bound_file = source_end.source_file) and _
		(semantic_model_source_bound_line = source_end.end_line) and _
		(semantic_model_source_bound_physical_line = lex.ctx->physical_linenum) ) then
		return source_end.end_column <= semantic_model_source_bound_columns
	end if

	'' The lexer can already be past a written token when a parser boundary
	'' exports a folded/generated AST value. Check its claimed end against the
	'' complete physical line before calling it an editable range. The reader
	'' follows physical lines across #line remapping and fails closed on unsafe input.
	dim as string source_line
	if( hSemanticModelReadCurrentSourceLine(lex.ctx->physical_linenum, source_line) = FALSE ) then
		return FALSE
	end if
	dim as integer source_columns = hSemanticModelSourceColumnCount(source_line)
	semantic_model_source_bound_file = source_end.source_file
	semantic_model_source_bound_line = source_end.end_line
	semantic_model_source_bound_physical_line = lex.ctx->physical_linenum
	semantic_model_source_bound_columns = source_columns
	semantic_model_source_bound_valid = TRUE
	return (source_columns >= 0) and _
		source_end.end_column <= semantic_model_source_bound_columns

end function

private sub hSemanticModelAssociateTypeIniSourceRange _
	( _
		byval node as ASTNODE ptr, _
		byval source_dependency as integer, _
		byref source_start as LEX_LOCATION, _
		byref source_end as LEX_LOCATION _
	)

	if( node = NULL ) then exit sub

	dim as SEMANTIC_MODEL_NODEFRAME ptr stack = NULL
	dim as SEMANTIC_MODEL_NODEFRAME ptr resized_stack
	dim as SEMANTIC_MODEL_NODEFRAME current
	dim as integer stack_count = 0
	dim as integer stack_capacity = 64
	dim as integer stack_failed = FALSE

	stack = callocate(stack_capacity, sizeof(SEMANTIC_MODEL_NODEFRAME))
	if( stack = NULL ) then
		semantic_model_module_failed = TRUE
		semantic_model_any_failed = TRUE
		exit sub
	end if
	stack[0].node = node
	stack_count = 1

	do while( stack_count > 0 )
		stack_count -= 1
		current = stack[stack_count]
		node = current.node
		if( node = NULL ) then continue do

		if( (node->class = AST_NODECLASS_TYPEINI) and _
			(node->typeini.semantic_dependency < 0) ) then
			node->typeini.semantic_dependency = source_dependency
			node->typeini.semantic_start_line = source_start.start_line
			node->typeini.semantic_start_column = source_start.start_column
			node->typeini.semantic_end_line = source_end.end_line
			node->typeini.semantic_end_column = source_end.end_column
		end if

		dim as integer child_count = iif(node->l <> NULL, 1, 0) + _
			iif(node->r <> NULL, 1, 0)
		if( child_count > 0 ) then
			while( stack_count + child_count > stack_capacity )
				if( stack_capacity >= SEMANTIC_MODEL_MAX_NODES_PER_MODEL ) then
					semantic_model_module_failed = TRUE
					semantic_model_any_failed = TRUE
					stack_failed = TRUE
					exit while
				end if
				resized_stack = reallocate(stack, _
					(stack_capacity * 2 + 16) * sizeof(SEMANTIC_MODEL_NODEFRAME))
				if( resized_stack = NULL ) then
					semantic_model_module_failed = TRUE
					semantic_model_any_failed = TRUE
					stack_failed = TRUE
					exit while
				end if
				stack = resized_stack
				stack_capacity = stack_capacity * 2 + 16
			wend
			if( stack_failed ) then exit do

			if( node->r <> NULL ) then
				stack[stack_count].node = node->r
				stack_count += 1
			end if
			if( node->l <> NULL ) then
				stack[stack_count].node = node->l
				stack_count += 1
			end if
		end if
	loop

	deallocate(stack)
end sub

'' Temporary destructors are emitted after expression parsing. Attach a
'' verified physical parser range to live dtor-list entries and TYPEINIs
'' before later AST lowering, without storing a path in every AST list item.
sub fbSemanticModelAssociateTemporaryDestructors _
	( _
		byval generation as ulongint, _
		byref source_start as LEX_LOCATION, _
		byref source_end as LEX_LOCATION, _
		byval nonphysical_tokens_at_start as longint, _
		byval nonphysical_tokens_at_end as longint, _
		byval expression as ASTNODE ptr _
	)

	if( (semantic_model_file_open = FALSE) or _
		(semantic_model_expressions_only) or _
		(semantic_model_module_open = FALSE) or _
		(semantic_model_module_failed) or _
		(source_start.start_line < 1) or (source_start.start_column < 0) or _
		(source_end.end_line < source_start.start_line) or _
		(source_end.end_column < 0) or _
		((source_end.end_line = source_start.start_line) and _
		 (source_end.end_column <= source_start.start_column)) or _
		(source_start.is_physical = FALSE) or (source_end.is_physical = FALSE) or _
		(nonphysical_tokens_at_start <> nonphysical_tokens_at_end) or _
		(source_start.source_file <> source_end.source_file) or _
		(hSemanticModelLocationIsPhysical(source_start) = FALSE) or _
		(hSemanticModelLocationIsPhysical(source_end) = FALSE) or _
		(hSemanticModelRangeFitsCurrentSourceLine(source_end) = FALSE) ) then
		exit sub
	end if

	fbSemanticModelAddDependency(source_start.source_file)
	dim as integer source_dependency = -1
	for index as integer = 0 to semantic_model_dependency_count - 1
		if( semantic_model_dependencies(index) = source_start.source_file ) then
			source_dependency = index
			exit for
		end if
	next
	if( source_dependency < 0 ) then exit sub

	'' Preserve the validated range on TYPEINIs that astUpdate() lowers later.
	if( (expression <> NULL) and (ast.typeinicount > 0) ) then
		hSemanticModelAssociateTypeIniSourceRange( _
			expression, source_dependency, source_start, source_end )
	end if

	dim as AST_DTORLIST_ITEM ptr item = listGetTail(@ast.dtorlist)
	while( item )
		'' Entries are appended in generation order, so older parser temporaries
		'' delimit the scan instead of making each expression walk the full list.
		if( item->semantic_generation <= generation ) then exit while
		if( item->semantic_dependency < 0 ) then
			dim as FBSYMBOL ptr subtype = symbGetSubtype(item->sym)
			if( subtype <> NULL ) then
				if( symbIsStruct(subtype) ) then
					item->semantic_dependency = source_dependency
					item->semantic_start_line = source_start.start_line
					item->semantic_start_column = source_start.start_column
					item->semantic_end_line = source_end.end_line
					item->semantic_end_column = source_end.end_column
					item->sym->semantic_dtor_dependency = source_dependency
					item->sym->semantic_dtor_start_line = source_start.start_line
					item->sym->semantic_dtor_start_column = source_start.start_column
					item->sym->semantic_dtor_end_line = source_end.end_line
					item->sym->semantic_dtor_end_column = source_end.end_column
				end if
			end if
		end if
		item = listGetPrev(item)
	wend
end sub

'' Convert the parser-owned range passed to astNewIIF() to the lexer's checked
'' location form, keeping AST construction independent from lexer structures.
sub fbSemanticModelAssociateTemporaryDestructorsForRange _
	( _
		byval generation as ulongint, _
		byval source_range as AST_SEMANTIC_SOURCE_RANGE ptr _
	)

	if( (source_range = NULL) or (len(source_range->source_file) = 0) ) then exit sub

	dim as LEX_LOCATION source_start = any, source_end = any
	source_start.source_file = source_range->source_file
	source_start.start_line = source_range->start_line
	source_start.start_column = source_range->start_column
	source_start.end_line = source_range->start_line
	source_start.end_column = source_range->start_column
	source_start.is_physical = source_range->start_is_physical
	source_end.source_file = source_range->source_file
	source_end.start_line = source_range->end_line
	source_end.start_column = source_range->end_column
	source_end.end_line = source_range->end_line
	source_end.end_column = source_range->end_column
	source_end.is_physical = source_range->end_is_physical

	fbSemanticModelAssociateTemporaryDestructors( _
		generation, source_start, source_end, _
		source_range->nonphysical_at_start, source_range->nonphysical_at_end, NULL )
end sub

'' WITH extends temporary lifetimes into a lexical scope and removes their
'' dtor-list entries. Export those compiler-selected destructors at the
'' original expression range before the parser promotes the temporary vars.
sub fbSemanticModelExportTemporaryDestructorForRange _
	( _
		byval owner as FBSYMBOL ptr, _
		byval target as FBSYMBOL ptr, _
		byval source_range as AST_SEMANTIC_SOURCE_RANGE ptr _
	)

	if( (semantic_model_file_open = FALSE) or _
		(semantic_model_expressions_only) or _
		(semantic_model_module_open = FALSE) or _
		(semantic_model_module_failed) or _
		(owner = NULL) or (target = NULL) or (source_range = NULL) or _
		(source_range->start_line < 1) or (source_range->start_column < 0) or _
		(source_range->end_line < source_range->start_line) or _
		(source_range->end_column < 0) or _
		((source_range->end_line = source_range->start_line) and _
		 (source_range->end_column <= source_range->start_column)) or _
		(len(source_range->source_file) = 0) or _
		(source_range->start_is_physical = FALSE) or _
		(source_range->end_is_physical = FALSE) or _
		(source_range->nonphysical_at_start <> source_range->nonphysical_at_end) ) then
		exit sub
	end if

	dim as LEX_LOCATION source
	source.source_file = source_range->source_file
	source.start_line = source_range->start_line
	source.start_column = source_range->start_column
	source.end_line = source_range->end_line
	source.end_column = source_range->end_column
	source.is_physical = TRUE
	if( (hSemanticModelLocationIsPhysical(source) = FALSE) or _
		(hSemanticModelRangeFitsCurrentSourceLine(source) = FALSE) ) then
		exit sub
	end if

	fbSemanticModelExportImplicitCall(owner, target, "temporary-destructor", source)
end sub

sub fbSemanticModelCaptureArgumentSource(byval arg as FB_CALL_ARG ptr, _
	byref source_range as AST_SEMANTIC_SOURCE_RANGE)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (arg = NULL) or _
		(source_range.start_line < 1) ) then exit sub
	fbSemanticModelReleaseArgumentSource(arg)
	dim as AST_SEMANTIC_SOURCE_RANGE ptr retained = allocate(sizeof(AST_SEMANTIC_SOURCE_RANGE))
	if( retained = NULL ) then
		fbSemanticModelFail( )
		exit sub
	end if
	*retained = source_range
	'' Verify the physical line while parsing is still at this argument.
	'' Overload resolution may run after subsequent arguments have been read.
	dim as LEX_LOCATION source_end
	source_end.source_file = source_range.source_file
	source_end.end_line = source_range.end_line
	source_end.end_column = source_range.end_column
	if( hSemanticModelRangeFitsCurrentSourceLine(source_end) = FALSE ) then
		retained->end_is_physical = FALSE
	end if
	arg->semantic_source = retained
end sub

sub fbSemanticModelReleaseArgumentSource(byval arg as FB_CALL_ARG ptr)
	if( arg = NULL ) then exit sub
	if( arg->semantic_source <> NULL ) then deallocate(arg->semantic_source)
	arg->semantic_source = NULL
end sub

sub fbSemanticModelExportArgumentConstructor(byval owner as FBSYMBOL ptr, _
	byval target as FBSYMBOL ptr, byval source_range as AST_SEMANTIC_SOURCE_RANGE ptr)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (source_range = NULL) ) then exit sub
	dim as LEX_LOCATION source
	source.source_file = source_range->source_file
	source.start_line = source_range->start_line
	source.start_column = source_range->start_column
	source.end_line = source_range->end_line
	source.end_column = source_range->end_column
	source.is_physical = source_range->start_is_physical and source_range->end_is_physical and _
		(source_range->nonphysical_at_start = source_range->nonphysical_at_end)
	hSemanticModelEmitImplicitCall(owner, target, "argument-constructor", source, _
		hSemanticModelLocationIsPhysical(source))
end sub

private function hSemanticModelEdgeName(byval edge as integer) as string
	select case edge
	case 1
		return "left"
	case 2
		return "right"
	case else
		return "root"
	end select
end function

sub fbSemanticModelExportExpression _
	( _
		byval expr as ASTNODE ptr, _
		byref source_start as LEX_LOCATION, _
		byref source_end as LEX_LOCATION, _
		byval nonphysical_tokens_at_start as longint, _
		byval nonphysical_tokens_at_end as longint, _
		byval semantic_operator_override as integer = -1 _
	)

	if( (semantic_model_file_open = FALSE) or _
		(semantic_model_module_open = FALSE) or _
		(semantic_model_module_failed) or (expr = NULL) ) then
		exit sub
	end if

	if( (source_start.start_line < 1) or (source_start.start_column < 0) or _
		(source_end.end_line < source_start.start_line) or _
		(source_end.end_column < 0) or _
		((source_end.end_line = source_start.start_line) and _
		 (source_end.end_column <= source_start.start_column))) then
		exit sub
	end if

	dim as string sourcefile = source_start.source_file
	dim as string type_name = symbTypeToStr(expr->dtype, expr->subtype)
	dim as integer physical_range = abs(hSemanticModelLocationIsPhysical(source_start) and _
		hSemanticModelLocationIsPhysical(source_end) and _
		source_start.is_physical and source_end.is_physical and _
		(nonphysical_tokens_at_start = nonphysical_tokens_at_end) and _
		(source_start.source_file = source_end.source_file) and _
		hSemanticModelRangeFitsCurrentSourceLine(source_end))
	dim as longint symbolid = 0, subtypeid = 0
	dim as integer effective_operator_override = semantic_operator_override
	if( (effective_operator_override < 0) and _
		(semantic_model_pending_operator_override >= 0) ) then
		if( (semantic_model_pending_operator_start.source_file = source_start.source_file) and _
			(semantic_model_pending_operator_start.start_line = source_start.start_line) and _
			(semantic_model_pending_operator_start.start_column = source_start.start_column) and _
			(semantic_model_pending_operator_end.end_line = source_end.end_line) and _
			(semantic_model_pending_operator_end.end_column = source_end.end_column) ) then
			effective_operator_override = semantic_model_pending_operator_override
		end if
		semantic_model_pending_operator_override = -1
	end if
	dim as string operator_kind = "none"
	dim as string operator_code = hSemanticModelConceptOperator(expr, operator_kind)
	if( effective_operator_override >= 0 ) then
		'' A parser operation can be lowered to a helper call before its
		'' enclosing expression is exported. Keep the compiler-known source
		'' operation concept without rewriting the raw AST fields.
		operator_code = hSemanticModelConceptOperatorCode(effective_operator_override)
		if( len(operator_code) = 0 ) then
			operator_kind = "none"
		elseif( operator_kind <> "overloaded" ) then
			operator_kind = "builtin"
		end if
	end if
	if( semantic_model_expressions_only = FALSE ) then
		symbolid = hSemanticModelSymbolId(expr->sym)
		if( (expr->class = AST_NODECLASS_CALL) andalso (expr->call.semantic_target <> NULL) ) then
			symbolid = hSemanticModelSymbolId(expr->call.semantic_target)
		end if
		subtypeid = hSemanticModelSymbolId(expr->subtype)
	end if
	dim as string expression_fact = _
		hSemanticModelNumber(physical_range) + TABCHAR + _
		hSemanticModelEscape(sourcefile) + TABCHAR + _
		hSemanticModelNumber(source_start.start_line) + TABCHAR + _
		hSemanticModelNumber(source_start.start_column) + TABCHAR + _
		hSemanticModelNumber(source_end.end_line) + TABCHAR + _
		hSemanticModelNumber(source_end.end_column) + TABCHAR + _
		hSemanticModelNumber(expr->class) + TABCHAR + _
		hSemanticModelNumber(hSemanticModelNodeOperator(expr)) + TABCHAR + _
		hSemanticModelEscape(operator_code) + _
		TABCHAR + hSemanticModelEscape(operator_kind) + TABCHAR + _
		hSemanticModelNumber(astGetFullType(expr)) + TABCHAR + _
		hSemanticModelNumber(symbolid) + TABCHAR + _
		hSemanticModelNumber(subtypeid) + TABCHAR + _
		hSemanticModelEscape(type_name)
	dim as string expression_fact_shape = _
		hSemanticModelNumber(physical_range) + TABCHAR + _
		hSemanticModelEscape(sourcefile) + TABCHAR + _
		hSemanticModelNumber(source_start.start_line) + TABCHAR + _
		hSemanticModelNumber(source_start.start_column) + TABCHAR + _
		hSemanticModelNumber(source_end.end_line) + TABCHAR + _
		hSemanticModelNumber(source_end.end_column) + TABCHAR + _
		hSemanticModelNumber(expr->class) + TABCHAR + _
		hSemanticModelNumber(hSemanticModelNodeOperator(expr)) + TABCHAR + _
		hSemanticModelNumber(astGetFullType(expr)) + TABCHAR + _
		hSemanticModelNumber(symbolid) + TABCHAR + _
		hSemanticModelNumber(subtypeid) + TABCHAR + _
		hSemanticModelEscape(type_name)
	dim as string value_kind, value_text
	if( semantic_model_expressions_only = FALSE ) then
		if( expr->class = AST_NODECLASS_CONST ) then
			fbSemanticModelFormatValue(expr->dtype, @expr->val.value, value_kind, value_text)
		elseif( (expr->class = AST_NODECLASS_VAR) andalso _
			((astGetDataType(expr) = FB_DATATYPE_CHAR) or (astGetDataType(expr) = FB_DATATYPE_WCHAR)) ) then
			dim as FBVALUE literal_value
			literal_value.s = expr->sym
			fbSemanticModelFormatValue(expr->dtype, @literal_value, value_kind, value_text)
		end if
	end if
	expression_fact_shape += TABCHAR + value_kind + TABCHAR + value_text
	dim as integer has_operator_override = (effective_operator_override >= 0)

	'' Parser precedence layers often return the same completed result while
	'' unwinding. Keep one copy when the adjacent fact payload is identical;
	'' distinct ranges, types, identities, or intervening records remain intact.
	if( (len(semantic_model_last_expression_shape) > 0) and _
		(expression_fact_shape = semantic_model_last_expression_shape) ) then
		if( has_operator_override ) then
			if( semantic_model_last_expression_had_operator_override = FALSE ) then
				'' Parser-lowered operations can first reach the exporter without
				'' their source operator. Keep the later parser-resolved concept.
				if( expression_fact <> semantic_model_last_expression_fact ) then
					if( hSemanticModelReplaceLastExpressionFact(expression_fact) = FALSE ) then exit sub
				end if
				semantic_model_last_expression_had_operator_override = TRUE
				exit sub
			elseif( expression_fact <> semantic_model_last_expression_fact ) then
				if( hSemanticModelReplaceLastExpressionFact(expression_fact) = FALSE ) then exit sub
				semantic_model_last_expression_had_operator_override = TRUE
				exit sub
			end if
		elseif( semantic_model_last_expression_had_operator_override ) then
			'' Do not let a later export of the same lowered AST erase source intent.
			exit sub
		end if
	end if
	if( (expression_fact = semantic_model_last_expression_fact) and _
		(expression_fact_shape = semantic_model_last_expression_shape) ) then exit sub
	if( len(semantic_model_pending_expression_value) > 0 ) then
		dim as string pending_value = semantic_model_pending_expression_value
		semantic_model_pending_expression_value = ""
		fbSemanticModelAppendDetail(pending_value)
	end if

	if( semantic_model_expression_count + _
		semantic_model_module_expression_count >= _
		SEMANTIC_MODEL_MAX_EXPRESSIONS_PER_MODEL ) then
		semantic_model_module_failed = TRUE
		semantic_model_any_failed = TRUE
		exit sub
	end if

	dim as longint expressionid = semantic_model_expression_count + _
		semantic_model_module_expression_count + 1

	hSemanticModelAppendLine("E" + TABCHAR + _
		hSemanticModelNumber(expressionid) + TABCHAR + expression_fact)
	if( semantic_model_module_failed = FALSE ) then
		semantic_model_module_expression_count += 1
		semantic_model_last_expression_fact = expression_fact
		semantic_model_last_expression_shape = expression_fact_shape
		semantic_model_last_expression_had_operator_override = has_operator_override
		if( len(value_kind) > 0 ) then
			semantic_model_pending_expression_value = "C" + TABCHAR + "expression" + _
				TABCHAR + hSemanticModelNumber(expressionid) + TABCHAR + value_kind + TABCHAR + value_text
		end if
	end if
end sub

function fbSemanticModelExportTree _
	( _
		byval root as ASTNODE ptr, _
		byval parentid as longint, _
		byref next_node_id as longint, _
		byval source_line as integer, _
		byval filename as zstring ptr _
	) as longint

	if( root = NULL ) then
		return 0
	end if

	dim as SEMANTIC_MODEL_NODEFRAME ptr stack = NULL
	dim as integer stack_count = 0, stack_capacity = 0
	dim as SEMANTIC_MODEL_NODEFRAME current
	dim as SEMANTIC_MODEL_NODEFRAME ptr resized_stack
	dim as longint symbolid, subtypeid
	dim as string sourcefile, operator_kind, operator_code

	stack_capacity = 64
	stack = callocate(stack_capacity, sizeof(SEMANTIC_MODEL_NODEFRAME))
	if( stack = NULL ) then
		semantic_model_module_failed = TRUE
		semantic_model_any_failed = TRUE
		return 0
	end if

	next_node_id += 1
	stack[0].node = root
	stack[0].parentid = parentid
	stack[0].nodeid = next_node_id
	stack[0].edge = 0
	stack_count = 1
	dim as longint rootid = next_node_id

	if( filename <> NULL ) then
		sourcefile = *filename
	end if

	do while( stack_count > 0 )
		stack_count -= 1
		current = stack[stack_count]
		if( current.node = NULL ) then
			continue do
		end if

		if( semantic_model_node_count + semantic_model_module_node_count >= _
			SEMANTIC_MODEL_MAX_NODES_PER_MODEL ) then
			semantic_model_module_failed = TRUE
			semantic_model_any_failed = TRUE
			exit do
		end if

		symbolid = hSemanticModelSymbolId(current.node->sym)
		subtypeid = hSemanticModelSymbolId(current.node->subtype)
		operator_kind = "none"
		operator_code = hSemanticModelConceptOperator(current.node, operator_kind)
		hSemanticModelAppendLine("N" + TABCHAR + hSemanticModelNumber(current.nodeid) + TABCHAR + _
			hSemanticModelNumber(current.parentid) + TABCHAR + hSemanticModelEdgeName(current.edge) + _
			TABCHAR + hSemanticModelNumber(current.node->class) + TABCHAR + _
			hSemanticModelNumber(hSemanticModelNodeOperator(current.node)) + TABCHAR + _
			hSemanticModelEscape(operator_code) + TABCHAR + _
			hSemanticModelEscape(operator_kind) + TABCHAR + _
			hSemanticModelNumber(current.node->dtype) + TABCHAR + hSemanticModelNumber(symbolid) + _
			TABCHAR + hSemanticModelNumber(subtypeid) + TABCHAR + _
			hSemanticModelNumber(source_line) + _
			TABCHAR + hSemanticModelEscape(sourcefile))
		semantic_model_module_node_count += 1
		fbSemanticModelExportNodeDetails(current.node, current.nodeid)

		if( (current.node->r <> NULL) or (current.node->l <> NULL) ) then
			while( stack_count + 2 > stack_capacity )
				if( stack_capacity >= SEMANTIC_MODEL_MAX_NODES_PER_MODEL ) then
					semantic_model_module_failed = TRUE
					semantic_model_any_failed = TRUE
					exit do
				end if
				resized_stack = reallocate(stack, _
					(stack_capacity * 2 + 16) * sizeof(SEMANTIC_MODEL_NODEFRAME))
				if( resized_stack = NULL ) then
					semantic_model_module_failed = TRUE
					semantic_model_any_failed = TRUE
					exit do
				end if
				stack = resized_stack
				stack_capacity = (stack_capacity * 2) + 16
			wend
			if( stack = NULL ) then exit do

			if( current.node->r <> NULL ) then
				next_node_id += 1
				stack[stack_count].node = current.node->r
				stack[stack_count].parentid = current.nodeid
				stack[stack_count].nodeid = next_node_id
				stack[stack_count].edge = 2
				stack_count += 1
			end if

			if( current.node->l <> NULL ) then
				next_node_id += 1
				stack[stack_count].node = current.node->l
				stack[stack_count].parentid = current.nodeid
				stack[stack_count].nodeid = next_node_id
				stack[stack_count].edge = 1
				stack_count += 1
			end if
		end if
	loop

	deallocate(stack)
	return rootid
end function

'' -------------------------------------------------------------------------
'' Sidecar and module lifecycle
'' -------------------------------------------------------------------------

function fbSemanticModelBegin(byref filename as string, byval expressions_only as integer) as integer
	semantic_model_pending_scope_exit_valid = FALSE
	if( semantic_model_file_open ) then
		if( fbSemanticOutputFinish(semantic_model_output, FALSE) = 0 ) then
			semantic_model_output = NULL
			semantic_model_file_open = FALSE
			return FALSE
		end if
		semantic_model_output = NULL
		semantic_model_file_open = FALSE
	end if

	semantic_model_filename = filename
	semantic_model_source_bound_valid = FALSE
	semantic_model_source_scan_valid = FALSE
	semantic_model_expressions_only = (expressions_only <> FALSE)
	semantic_model_output = fbSemanticOutputOpen(strptr(filename))
	if( semantic_model_output = NULL ) then
		return FALSE
	end if

	semantic_model_file_open = TRUE
	semantic_model_module_open = FALSE
	semantic_model_any_failed = FALSE
	semantic_model_module_count = 0
	semantic_model_recovery_module_count = 0
	semantic_model_dependency_count = 0
	semantic_model_dependencies_complete = TRUE
	for index as integer = 0 to SEMANTIC_MODEL_MAX_DEPENDENCIES - 1
		semantic_model_dependencies(index) = ""
	next
	semantic_model_proc_count = 0
	semantic_model_total_symbol_count = 0
	semantic_model_total_type_fact_count = 0
	semantic_model_expression_count = 0
	semantic_model_binding_count = 0
	semantic_model_implicit_call_count = 0
	semantic_model_node_count = 0
	semantic_model_detail_count = 0
	semantic_model_visit = 0
	dim as string header = "FBCSEM" + TABCHAR + SEMANTIC_MODEL_SCHEMA + _
		TABCHAR + FB_VERSION + NEWLINE
	if( fbSemanticOutputWrite(semantic_model_output, strptr(header), len(header)) = 0 ) then
		fbSemanticOutputFinish(semantic_model_output, FALSE)
		semantic_model_output = NULL
		semantic_model_file_open = FALSE
		return FALSE
	end if
	'' The semantic-model lifecycle owns this handle until fbSemanticModelEnd.
	'' fblint: disable-next-line FBL-IO-004
	return TRUE
end function

function fbSemanticModelEnabled() as integer
	return semantic_model_file_open
end function

sub fbSemanticModelSetExpressionOperatorOverride _
	( _
		byref source_start as LEX_LOCATION, _
		byref source_end as LEX_LOCATION, _
		byval operator_override as integer _
	)
	if( (semantic_model_file_open = FALSE) or _
		(semantic_model_expressions_only) or _
		(operator_override < 0) ) then
		exit sub
	end if
	semantic_model_pending_operator_start = source_start
	semantic_model_pending_operator_end = source_end
	semantic_model_pending_operator_override = operator_override
end sub

function fbSemanticModelExpressionsOnlyEnabled() as integer
	return abs(semantic_model_file_open and semantic_model_expressions_only)
end function

sub fbSemanticModelPushExpressionRange _
	( _
		byref source_start as LEX_LOCATION, _
		byval nonphysical_tokens_at_start as longint, _
		byref previous_start as LEX_LOCATION, _
		byref previous_nonphysical_tokens as longint _
	)

	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	previous_start = semantic_model_active_expression_start
	previous_nonphysical_tokens = semantic_model_active_expression_nonphysical
	semantic_model_active_expression_start = source_start
	semantic_model_active_expression_nonphysical = nonphysical_tokens_at_start
end sub

sub fbSemanticModelPopExpressionRange _
	( _
		byref previous_start as LEX_LOCATION, _
		byval previous_nonphysical_tokens as longint _
	)

	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	semantic_model_active_expression_start = previous_start
	semantic_model_active_expression_nonphysical = previous_nonphysical_tokens
end sub

sub fbSemanticModelExportCurrentExpressionPrefix(byval expr as ASTNODE ptr)
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	if( semantic_model_active_expression_start.start_line < 1 ) then exit sub
	dim as LEX_LOCATION source_end = lexGetLastLocation( )
	fbSemanticModelExportExpression(expr, semantic_model_active_expression_start, _
		source_end, semantic_model_active_expression_nonphysical, _
		lexGetNonphysicalTokenCount( ))
end sub

sub fbSemanticModelBeginModule(byref filename as string)
	if( semantic_model_file_open = FALSE ) then exit sub

	semantic_model_pending_scope_exit_valid = FALSE
	semantic_model_source_bound_valid = FALSE
	semantic_model_source_scan_valid = FALSE
	semantic_model_active_expression_start.start_line = 0
	semantic_model_active_expression_nonphysical = 0
	semantic_model_pending_operator_override = -1
	semantic_model_last_expression_fact = ""
	semantic_model_pending_expression_value = ""
	semantic_model_last_expression_shape = ""
	semantic_model_last_expression_had_operator_override = FALSE
	semantic_model_last_expression_buffer_start = 0
	semantic_model_module_buffer_len = 0
	semantic_model_symbol_count = 0
	if( semantic_model_symbol_index <> NULL ) then
		memset(semantic_model_symbol_index, 0, _
			semantic_model_symbol_index_capacity * sizeof(SEMANTIC_MODEL_SYMBOL_SLOT))
	end if
	semantic_model_module_failed = FALSE
	semantic_model_module_proc_count = 0
	semantic_model_module_type_fact_count = 0
	semantic_model_module_expression_count = 0
	semantic_model_module_binding_count = 0
	semantic_model_module_implicit_call_count = 0
	semantic_model_module_node_count = 0
	semantic_model_module_detail_count = 0
	semantic_model_module_open = TRUE
	semantic_model_module_source = filename
	fbSemanticModelBeginSource(filename, 0)
	hSemanticModelAppendLine("M" + TABCHAR + hSemanticModelEscape(filename))
end sub

sub fbSemanticModelFinishModule(byval commit as integer)
	if( semantic_model_module_open = FALSE ) then exit sub
	if( (commit <> FALSE) and (len(semantic_model_pending_expression_value) > 0) ) then
		dim as string pending_value = semantic_model_pending_expression_value
		semantic_model_pending_expression_value = ""
		fbSemanticModelAppendDetail(pending_value)
	end if
	semantic_model_pending_expression_value = ""

	if( (commit <> FALSE) and (semantic_model_module_failed = FALSE) ) then
		if( semantic_model_module_buffer_len > 0 ) then
			if( fbSemanticOutputWrite(semantic_model_output, semantic_model_module_buffer, _
				semantic_model_module_buffer_len) = 0 ) then fbSemanticModelFail( )
		end if
	end if
	if( (commit <> FALSE) and (semantic_model_module_failed = FALSE) ) then
		semantic_model_module_count += 1
		semantic_model_proc_count += semantic_model_module_proc_count
		semantic_model_total_symbol_count += semantic_model_symbol_count
		semantic_model_total_type_fact_count += semantic_model_module_type_fact_count
		semantic_model_expression_count += semantic_model_module_expression_count
		semantic_model_binding_count += semantic_model_module_binding_count
		semantic_model_implicit_call_count += semantic_model_module_implicit_call_count
		semantic_model_node_count += semantic_model_module_node_count
		semantic_model_detail_count += semantic_model_module_detail_count
	end if

	semantic_model_module_buffer_len = 0
	semantic_model_module_open = FALSE
	fbSemanticModelEndSource(0)
	semantic_model_symbol_count = 0
	semantic_model_module_expression_count = 0
	semantic_model_module_binding_count = 0
	semantic_model_module_implicit_call_count = 0
end sub

'' Commit expression-only facts from a module whose parser recovered with
'' errors. The R record distinguishes this incomplete module from a normal
'' module; consumers must also reject source ranges that overlap recovery.
sub fbSemanticModelFinishRecoveryModule( )
	if( semantic_model_module_open = FALSE ) then exit sub
	if( semantic_model_expressions_only = FALSE ) then
		fbSemanticModelFinishModule(FALSE)
		exit sub
	end if
	if( semantic_model_module_failed ) then
		fbSemanticModelFinishModule(FALSE)
		exit sub
	end if

	hSemanticModelAppendLine("R" + TABCHAR + SEMANTIC_MODEL_SCHEMA + _
		TABCHAR + hSemanticModelNumber(semantic_model_module_expression_count))
	if( semantic_model_module_failed ) then
		fbSemanticModelFinishModule(FALSE)
		exit sub
	end if

	fbSemanticModelFinishModule(TRUE)
	semantic_model_recovery_module_count += 1
end sub

sub fbSemanticModelExportProc(byval proc as FBSYMBOL ptr, byval astproc as ASTNODE ptr)
	dim as FBSYMBOL ptr symbol, param
	dim as ASTNODE ptr node
	dim as integer proc_start_line = 0, proc_end_line = 0
	dim as string filename, procname
	dim as zstring ptr sourcefileptr
	dim as integer source_line = 0
	dim as longint procid, subtypeid, next_node_id

	if( (semantic_model_file_open = FALSE) or (semantic_model_module_open = FALSE) or _
		(semantic_model_expressions_only) or _
		(semantic_model_module_failed) ) then
		exit sub
	end if
	if( (hSemanticModelHasSymbol(proc) = FALSE) or (astproc = NULL) ) then exit sub

	procid = hSemanticModelSymbolId(proc)
	if( proc->id.name <> NULL ) then procname = *proc->id.name
	if( proc->proc.ext <> NULL ) then
		proc_start_line = proc->proc.ext->dbg.iniline
		proc_end_line = proc->proc.ext->dbg.endline
	end if
	'' Capture local and nested tables before flushing recycles their symbols.
	fbSemanticModelExportSymbolDetails(proc, TRUE)
	fbSemanticModelExportSymbols(symbGetProcHeadParam(proc))
	fbSemanticModelExportSymbols(symbGetProcSymbTbHead(proc))
	next_node_id = semantic_model_node_count + semantic_model_module_node_count

	if( (proc->proc.ext <> NULL) and _
		(proc->proc.ext->dbg.incfile <> NULL) ) then
		filename = *proc->proc.ext->dbg.incfile
	end if

	hSemanticModelAppendLine("P" + TABCHAR + hSemanticModelNumber(procid) + TABCHAR + _
		hSemanticModelEscape(procname) + TABCHAR + hSemanticModelNumber(proc->class) + TABCHAR + _
		hSemanticModelNumber(proc->typ) + TABCHAR + _
		hSemanticModelNumber(hSemanticModelSymbolId(proc->subtype)) + _
		TABCHAR + hSemanticModelNumber(proc_start_line) + TABCHAR + _
		hSemanticModelNumber(proc_end_line) + TABCHAR + hSemanticModelEscape(filename))
	semantic_model_module_proc_count += 1

	node = astproc->l
	while( node <> NULL )
		if( node->class = AST_NODECLASS_DBG ) then
			if( node->dbg.op = AST_OP_DBG_LINEINI ) then
				source_line = node->dbg.ex
				filename = ""
				if( node->dbg.filename <> NULL ) then
					filename = *node->dbg.filename
				end if
			end if
		end if
		sourcefileptr = NULL
		if( len(filename) > 0 ) then sourcefileptr = strptr(filename)
		fbSemanticModelExportTree(node, procid, next_node_id, source_line, _
			sourcefileptr)
		node = node->next
	wend
end sub

sub fbSemanticModelExportGlobals(byval symbol as FBSYMBOL ptr)
	dim as string global_procname

	if( (semantic_model_file_open = FALSE) or (semantic_model_module_open = FALSE) or _
		(semantic_model_expressions_only) ) then
		exit sub
	end if
	fbSemanticModelExportContext(semantic_model_module_source)
	fbSemanticModelExportSymbols(symbol)
end sub

function fbSemanticModelEnd(byval succeeded as integer) as integer
	if( semantic_model_file_open = FALSE ) then return TRUE

	if( semantic_model_module_open ) then
		fbSemanticModelFinishModule(FALSE)
	end if
	hSemanticModelWriteDependencies()

	dim as string footer
	if( (succeeded <> FALSE) and (semantic_model_any_failed = FALSE) ) then
		footer = "END" + TABCHAR + SEMANTIC_MODEL_SCHEMA + _
			TABCHAR + hSemanticModelNumber(semantic_model_module_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_proc_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_total_symbol_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_total_type_fact_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_node_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_expression_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_binding_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_implicit_call_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_dependency_count) + TABCHAR + _
			hSemanticModelNumber(abs(semantic_model_dependencies_complete)) + TABCHAR + _
			hSemanticModelNumber(semantic_model_detail_count)
	elseif( semantic_model_expressions_only and _
		(semantic_model_recovery_module_count > 0) and _
		(semantic_model_any_failed = FALSE) ) then
		'' RECOVERY is a bounded expression snapshot, not a complete semantic model.
		footer = "RECOVERY" + TABCHAR + _
			SEMANTIC_MODEL_SCHEMA + TABCHAR + _
			hSemanticModelNumber(semantic_model_module_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_expression_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_recovery_module_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_binding_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_dependency_count) + TABCHAR + _
			hSemanticModelNumber(abs(semantic_model_dependencies_complete))
	end if

	if( len(footer) > 0 ) then
		footer += NEWLINE
		if( fbSemanticOutputWrite(semantic_model_output, strptr(footer), len(footer)) = 0 ) then fbSemanticModelFail( )
	end if
	'' Publication follows all writes and the checked close. Parser errors can
	'' retain the historical incomplete/recovery snapshot, but transport errors
	'' preserve the destination and make the compiler report failure.
	dim as integer published = fbSemanticOutputFinish(semantic_model_output, semantic_model_any_failed = FALSE)
	semantic_model_output = NULL
	function = abs((published <> 0) and (semantic_model_any_failed = FALSE))
	semantic_model_file_open = FALSE
	semantic_model_module_open = FALSE
	semantic_model_module_buffer_len = 0
	semantic_model_filename = ""
	semantic_model_module_source = ""
	for index as integer = 0 to semantic_model_dependency_count - 1
		semantic_model_dependencies(index) = ""
	next
	semantic_model_dependency_count = 0
	if( semantic_model_module_buffer <> NULL ) then
		deallocate(semantic_model_module_buffer)
		semantic_model_module_buffer = NULL
	end if
	semantic_model_module_buffer_capacity = 0
	if( semantic_model_symbols <> NULL ) then
		deallocate(semantic_model_symbols)
		semantic_model_symbols = NULL
	end if
	if( semantic_model_symbol_index <> NULL ) then
		deallocate(semantic_model_symbol_index)
		semantic_model_symbol_index = NULL
	end if
	semantic_model_symbol_capacity = 0
	semantic_model_symbol_count = 0
	semantic_model_symbol_index_capacity = 0
end function

'' end of tooling/semantic-model.bas
