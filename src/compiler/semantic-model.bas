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

#include once "fbint.bi"
#include once "ast.bi"
#include once "lex.bi"
#include once "symb.bi"
#include once "crt/mem.bi"

'' -------------------------------------------------------------------------
'' Export limits and process-local module state
'' -------------------------------------------------------------------------

private const SEMANTIC_MODEL_SCHEMA = "8"
private const SEMANTIC_MODEL_MAX_SYMBOLS = 1000000
private const SEMANTIC_MODEL_MAX_DEPENDENCIES = 5000
private const SEMANTIC_MODEL_INITIAL_SYMBOL_INDEX_CAPACITY = 256
private const SEMANTIC_MODEL_MAX_SYMBOL_INDEX_CAPACITY = 2097152
private const SEMANTIC_MODEL_INITIAL_MODULE_BUFFER_CAPACITY = 8192
private const SEMANTIC_MODEL_MAX_MODULE_BUFFER_BYTES = 268435456
private const SEMANTIC_MODEL_MAX_NODES_PER_MODEL = 1000000
private const SEMANTIC_MODEL_MAX_EXPRESSIONS_PER_MODEL = 1000000
private const SEMANTIC_MODEL_MAX_BINDINGS_PER_MODEL = 1000000
private const SEMANTIC_MODEL_MAX_SOURCE_CONTEXTS = FB_MAXINCRECLEVEL
private const SEMANTIC_MODEL_MAX_SOURCE_LINE_BYTES = LEX_MAXBUFFCHARS

private type SEMANTIC_MODEL_SYMBOL
	sym         as FBSYMBOL ptr
	identity    as ulongint
	state       as integer
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

dim shared as integer semantic_model_file_num
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
dim shared as integer semantic_model_source_bound_columns
dim shared as integer semantic_model_source_bound_filepos
dim shared as string semantic_model_source_bound_file
dim shared as LEX_LOCATION semantic_model_active_expression_start
dim shared as longint semantic_model_active_expression_nonphysical
dim shared as string semantic_model_last_expression_fact
dim shared as integer semantic_model_module_count
dim shared as integer semantic_model_module_proc_count
dim shared as integer semantic_model_module_type_fact_count
dim shared as longint semantic_model_module_expression_count
dim shared as longint semantic_model_module_binding_count
dim shared as longint semantic_model_module_node_count
dim shared as integer semantic_model_proc_count
dim shared as integer semantic_model_total_symbol_count
dim shared as integer semantic_model_total_type_fact_count
dim shared as longint semantic_model_expression_count
dim shared as longint semantic_model_binding_count
dim shared as longint semantic_model_node_count
dim shared as string semantic_model_filename
dim shared as ubyte ptr semantic_model_module_buffer
dim shared as uinteger semantic_model_module_buffer_len
dim shared as uinteger semantic_model_module_buffer_capacity
dim shared as integer semantic_model_symbol_count
dim shared as SEMANTIC_MODEL_SYMBOL ptr semantic_model_symbols
dim shared as integer semantic_model_symbol_capacity
dim shared as SEMANTIC_MODEL_SYMBOL_SLOT ptr semantic_model_symbol_index
dim shared as integer semantic_model_symbol_index_capacity
dim shared as ulongint semantic_model_next_symbol_identity

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
		if( (bytevalue = 9) or (bytevalue = 10) or _
			(bytevalue = 13) or (bytevalue = 37) ) then
			if( output_length > SEMANTIC_MODEL_MAX_MODULE_BUFFER_BYTES - 2 ) then
				semantic_model_module_failed = TRUE
				semantic_model_any_failed = TRUE
				return ""
			end if
			output_length += 2
		end if
	next

	result = space(output_length)
	result_bytes = strptr(result)
	output_position = 1
	for i = 1 to len(value)
		bytevalue = asc(mid(value, i, 1))
		select case bytevalue
		case 9, 10, 13, 37
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
		semantic_model_last_expression_fact = ""
	end if
	dim as string line_text = value + NEWLINE

	hSemanticModelAppendBytes(strptr(line_text), len(line_text))
end sub

private sub hSemanticModelWriteDependencies( )
	for index as integer = 0 to semantic_model_dependency_count - 1
		print #semantic_model_file_num, "D" + TABCHAR + _
			hSemanticModelEscape(semantic_model_dependencies(index))
	next
end sub

sub fbSemanticModelAddDependency(byref filename as string)
	if( (semantic_model_file_open = FALSE) or (len(filename) = 0) ) then exit sub
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
	parentid = hSemanticModelSymbolId(sym->parent)

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

private function hSemanticModelTypeKind(byval sym as FBSYMBOL ptr) as string
	dim as integer dtype

	if( sym = NULL ) then return "other"

	dtype = sym->typ
	if( (typeGetPtrCnt(dtype) > 0) or _
		(typeGet(dtype) = FB_DATATYPE_POINTER) ) then
		return "pointer"
	end if

	select case typeGetClass(dtype)
	case FB_DATACLASS_INTEGER, FB_DATACLASS_FPOINT
		return "numeric"
	case FB_DATACLASS_STRING
		if( typeGet(dtype) = FB_DATATYPE_STRING ) then
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

private function hSemanticModelNodeOperator(byval node as ASTNODE ptr) as integer
	select case node->class
	case AST_NODECLASS_BOP, AST_NODECLASS_UOP, AST_NODECLASS_CONV, _
		AST_NODECLASS_ADDROF, AST_NODECLASS_BRANCH, AST_NODECLASS_MEM
		return node->op.op
	case AST_NODECLASS_DBG
		return node->dbg.op
	case else
		return 0
	end select
end function

private function hSemanticModelConceptOperator(byval node as ASTNODE ptr, _
	byref operator_kind as string) as string
	dim as integer op

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
		case AST_NODECLASS_BOP, AST_NODECLASS_UOP, AST_NODECLASS_CONV, _
			AST_NODECLASS_ADDROF, AST_NODECLASS_MEM
			op = hSemanticModelNodeOperator(node)
		case else
			return ""
		end select
		operator_kind = "builtin"
	end if

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
	case AST_OP_TOINT: return "convert-to-integer"
	case AST_OP_TOFLT: return "convert-to-float"
	case AST_OP_NEW: return "allocate"
	case AST_OP_NEW_VEC: return "allocate-array"
	case AST_OP_DEL: return "deallocate"
	case AST_OP_DEL_VEC: return "deallocate-array"
	case else
		operator_kind = "none"
		return ""
	end select
end function

private function hSemanticModelSourceColumnCount(byref source_line as string) as integer
	if( len(source_line) = 0 ) then return 0
	dim as ubyte ptr source_bytes = cast(ubyte ptr, strptr(source_line))
	dim as integer column = 0, utf8_continuations_left = 0

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
		byval current_filepos as longint, _
		byref source_line as string _
	) as integer

	const READ_CHUNK_BYTES = 1024
	dim as longint old_file_position = seek(env.inf.num)
	dim as longint file_length = lof(env.inf.num)
	dim as longint scan_position, line_start = 0
	dim as integer chunk_length, index, source_byte
	dim as integer valid = TRUE, found_line_start = FALSE, found_line_end = FALSE
	dim as string chunk
	dim as ubyte ptr chunk_bytes

	function = FALSE
	source_line = ""
	if( (current_filepos < 0) or (current_filepos > file_length) ) then
		seek #env.inf.num, old_file_position
		exit function
	end if

	'' Locate the physical line start without lexPeekCurrentLine()'s bounded
	'' diagnostic excerpt. Semantic editor columns need the whole line length.
	scan_position = current_filepos
	do while( (scan_position > 0) and (found_line_start = FALSE) )
		chunk_length = iif(scan_position > READ_CHUNK_BYTES, _
			READ_CHUNK_BYTES, scan_position)
		dim as longint chunk_start = scan_position - chunk_length
		chunk = space(chunk_length)
		if( get(#env.inf.num, chunk_start + 1, chunk) <> 0 ) then
			valid = FALSE
			exit do
		end if
		chunk_bytes = cast(ubyte ptr, strptr(chunk))
		for index = chunk_length - 1 to 0 step -1
			source_byte = chunk_bytes[index]
			if( (source_byte = 10) or (source_byte = 13) ) then
				line_start = chunk_start + index + 1
				found_line_start = TRUE
				exit for
			end if
		next
		if( found_line_start = FALSE ) then
			scan_position = chunk_start
			if( current_filepos - scan_position > SEMANTIC_MODEL_MAX_SOURCE_LINE_BYTES ) then
				valid = FALSE
				exit do
			end if
		elseif( current_filepos - line_start > SEMANTIC_MODEL_MAX_SOURCE_LINE_BYTES ) then
			valid = FALSE
			exit do
		end if
	loop

	'' Read forward from the physical start and stop at the first line ending.
	'' The compiler's lexer caps one source line to this same byte count.
	scan_position = line_start
	do while( (valid) and (found_line_end = FALSE) and (scan_position < file_length) )
		chunk_length = iif(file_length - scan_position > READ_CHUNK_BYTES, _
			READ_CHUNK_BYTES, file_length - scan_position)
		chunk = space(chunk_length)
		if( get(#env.inf.num, scan_position + 1, chunk) <> 0 ) then
			valid = FALSE
			exit do
		end if
		chunk_bytes = cast(ubyte ptr, strptr(chunk))
		for index = 0 to chunk_length - 1
			source_byte = chunk_bytes[index]
			if( (source_byte = 10) or (source_byte = 13) ) then
				if( scan_position - line_start + index > _
					SEMANTIC_MODEL_MAX_SOURCE_LINE_BYTES ) then
					valid = FALSE
				elseif( index > 0 ) then
					source_line += left(chunk, index)
				end if
				found_line_end = TRUE
				exit for
			end if
		next
		if( (valid) and (found_line_end = FALSE) ) then
			scan_position += chunk_length
			if( scan_position - line_start > SEMANTIC_MODEL_MAX_SOURCE_LINE_BYTES ) then
				valid = FALSE
			else
				source_line += chunk
			end if
		end if
	loop

	seek #env.inf.num, old_file_position
	if( valid ) then
		return TRUE
	end if
	source_line = ""
	return FALSE

end function

private function hSemanticModelRangeFitsCurrentSourceLine _
	( _
		byref source_end as LEX_LOCATION _
	) as integer

	if( env.inf.format <> FBFILE_FORMAT_ASCII ) then return TRUE
	if( source_end.source_file <> env.inf.name ) then return TRUE
	if( source_end.end_line <> lex.ctx->linenum ) then return TRUE
	if( lex.ctx->lastfilepos <= 0 ) then return TRUE
	'' #line can reuse a logical filename and line number for a different
	'' physical line, so include the lexer's file position in the cache key.
	if( semantic_model_source_bound_valid and _
		(semantic_model_source_bound_file = source_end.source_file) and _
		(semantic_model_source_bound_line = source_end.end_line) and _
		(semantic_model_source_bound_filepos = lex.ctx->lastfilepos) ) then
		return source_end.end_column <= semantic_model_source_bound_columns
	end if

	'' The lexer can already be past a written token when a parser boundary
	'' exports a folded/generated AST value. Check its claimed end against the
	'' complete physical line before calling it an editable range. The reader
	'' preserves the source file position and fails closed on oversized lines.
	dim as string source_line
	if( hSemanticModelReadCurrentSourceLine(lex.ctx->lastfilepos, source_line) = FALSE ) then
		return FALSE
	end if
	semantic_model_source_bound_file = source_end.source_file
	semantic_model_source_bound_line = source_end.end_line
	semantic_model_source_bound_columns = hSemanticModelSourceColumnCount( source_line )
	semantic_model_source_bound_filepos = lex.ctx->lastfilepos
	semantic_model_source_bound_valid = TRUE
	return source_end.end_column <= semantic_model_source_bound_columns

end function

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
		byval nonphysical_tokens_at_end as longint _
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
	dim as string operator_kind = "none"
	dim as string operator_code = hSemanticModelConceptOperator(expr, operator_kind)
	if( semantic_model_expressions_only = FALSE ) then
		symbolid = hSemanticModelSymbolId(expr->sym)
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

	'' Parser precedence layers often return the same completed result while
	'' unwinding. Keep one copy when the adjacent fact payload is identical;
	'' distinct ranges, types, identities, or intervening records remain intact.
	if( expression_fact = semantic_model_last_expression_fact ) then exit sub

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
	end if
end sub

private sub hSemanticModelExportTree _
	( _
		byval root as ASTNODE ptr, _
		byval parentid as longint, _
		byref next_node_id as longint, _
		byval source_line as integer, _
		byval filename as zstring ptr _
	)

	if( root = NULL ) then
		exit sub
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
		exit sub
	end if

	next_node_id += 1
	stack[0].node = root
	stack[0].parentid = parentid
	stack[0].nodeid = next_node_id
	stack[0].edge = 0
	stack_count = 1

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
end sub

'' -------------------------------------------------------------------------
'' Sidecar and module lifecycle
'' -------------------------------------------------------------------------

function fbSemanticModelBegin(byref filename as string, byval expressions_only as integer) as integer
	if( semantic_model_file_open ) then
		close #semantic_model_file_num
		semantic_model_file_open = FALSE
	end if

	semantic_model_filename = filename
	semantic_model_source_bound_valid = FALSE
	semantic_model_expressions_only = (expressions_only <> FALSE)
	semantic_model_file_num = freefile
	if( open(filename for output as #semantic_model_file_num) <> 0 ) then
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
	semantic_model_node_count = 0
	print #semantic_model_file_num, "FBCSEM" + TABCHAR + SEMANTIC_MODEL_SCHEMA + _
		TABCHAR + FB_VERSION
	return TRUE ' fbSemanticModelEnd owns this handle for successful and failed compiles.
end function

function fbSemanticModelEnabled() as integer
	return semantic_model_file_open
end function

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

	if( fbSemanticModelExpressionsOnlyEnabled( ) = FALSE ) then exit sub
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

	if( fbSemanticModelExpressionsOnlyEnabled( ) = FALSE ) then exit sub
	semantic_model_active_expression_start = previous_start
	semantic_model_active_expression_nonphysical = previous_nonphysical_tokens
end sub

sub fbSemanticModelExportCurrentExpressionPrefix(byval expr as ASTNODE ptr)
	if( fbSemanticModelExpressionsOnlyEnabled( ) = FALSE ) then exit sub
	if( semantic_model_active_expression_start.start_line < 1 ) then exit sub
	dim as LEX_LOCATION source_end = lexGetLastLocation( )
	fbSemanticModelExportExpression(expr, semantic_model_active_expression_start, _
		source_end, semantic_model_active_expression_nonphysical, _
		lexGetNonphysicalTokenCount( ))
end sub

sub fbSemanticModelBeginModule(byref filename as string)
	if( semantic_model_file_open = FALSE ) then exit sub

	semantic_model_source_bound_valid = FALSE
	semantic_model_active_expression_start.start_line = 0
	semantic_model_active_expression_nonphysical = 0
	semantic_model_last_expression_fact = ""
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
	semantic_model_module_node_count = 0
	semantic_model_module_open = TRUE
	fbSemanticModelBeginSource(filename, 0)
	hSemanticModelAppendLine("M" + TABCHAR + hSemanticModelEscape(filename))
	fbSemanticModelAddDependency(filename)
end sub

sub fbSemanticModelFinishModule(byval commit as integer)
	if( semantic_model_module_open = FALSE ) then exit sub

	if( (commit <> FALSE) and (semantic_model_module_failed = FALSE) ) then
		if( semantic_model_module_buffer_len > 0 ) then
			print #semantic_model_file_num, _
				*cast(zstring ptr, semantic_model_module_buffer);
		end if
		semantic_model_module_count += 1
		semantic_model_proc_count += semantic_model_module_proc_count
		semantic_model_total_symbol_count += semantic_model_symbol_count
		semantic_model_total_type_fact_count += semantic_model_module_type_fact_count
		semantic_model_expression_count += semantic_model_module_expression_count
		semantic_model_binding_count += semantic_model_module_binding_count
		semantic_model_node_count += semantic_model_module_node_count
	end if

	semantic_model_module_buffer_len = 0
	semantic_model_module_open = FALSE
	fbSemanticModelEndSource(0)
	semantic_model_symbol_count = 0
	semantic_model_module_expression_count = 0
	semantic_model_module_binding_count = 0
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
	next_node_id = semantic_model_node_count + semantic_model_module_node_count
	if( proc->id.name <> NULL ) then procname = *proc->id.name
	if( proc->proc.ext <> NULL ) then
		proc_start_line = proc->proc.ext->dbg.iniline
		proc_end_line = proc->proc.ext->dbg.endline
	end if
	param = symbGetProcHeadParam(proc)
	while( hSemanticModelHasSymbol(param) )
		hSemanticModelSymbolId(param)
		if( hSemanticModelHasSymbol(param->param.var) ) then
			hSemanticModelSymbolId(param->param.var)
			hSemanticModelExportVariableType(param->param.var, procname)
		end if
		param = param->next
	wend

	symbol = symbGetProcSymbTbHead(proc)
	while( hSemanticModelHasSymbol(symbol) )
		hSemanticModelSymbolId(symbol)
		hSemanticModelExportVariableType(symbol, procname)
		symbol = symbol->next
	wend

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
		hSemanticModelExportTree(node, procid, next_node_id, source_line, _
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
	while( hSemanticModelHasSymbol(symbol) and (semantic_model_module_failed = FALSE) )
		hSemanticModelSymbolId(symbol)
		hSemanticModelExportVariableType(symbol, global_procname)
		symbol = symbol->next
	wend
end sub

sub fbSemanticModelEnd(byval succeeded as integer)
	if( semantic_model_file_open = FALSE ) then exit sub

	if( semantic_model_module_open ) then
		fbSemanticModelFinishModule(FALSE)
	end if
	hSemanticModelWriteDependencies()

	if( (succeeded <> FALSE) and (semantic_model_any_failed = FALSE) ) then
		print #semantic_model_file_num, "END" + TABCHAR + SEMANTIC_MODEL_SCHEMA + _
			TABCHAR + hSemanticModelNumber(semantic_model_module_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_proc_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_total_symbol_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_total_type_fact_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_node_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_expression_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_binding_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_dependency_count) + TABCHAR + _
			hSemanticModelNumber(abs(semantic_model_dependencies_complete))
	elseif( semantic_model_expressions_only and _
		(semantic_model_recovery_module_count > 0) and _
		(semantic_model_any_failed = FALSE) ) then
		'' RECOVERY is a bounded expression snapshot, not a complete semantic model.
		print #semantic_model_file_num, "RECOVERY" + TABCHAR + _
			SEMANTIC_MODEL_SCHEMA + TABCHAR + _
			hSemanticModelNumber(semantic_model_module_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_expression_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_recovery_module_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_binding_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_dependency_count) + TABCHAR + _
			hSemanticModelNumber(abs(semantic_model_dependencies_complete))
	end if

	close #semantic_model_file_num
	semantic_model_file_open = FALSE
	semantic_model_module_open = FALSE
	semantic_model_module_buffer_len = 0
	semantic_model_filename = ""
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
end sub

'' end of semantic-model.bas
