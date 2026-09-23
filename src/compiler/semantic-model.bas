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
''     - preserve symbol identity, type, scope, and parent relationships
''     - serialize procedure AST nodes with source locations
''     - reject incomplete module or invocation output with a missing footer
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
''     end of the compiler invocation and reused only between modules.

#include once "fbint.bi"
#include once "ast.bi"
#include once "lex.bi"
#include once "symb.bi"
#include once "crt/mem.bi"

'' -------------------------------------------------------------------------
'' Export limits and process-local module state
'' -------------------------------------------------------------------------

private const SEMANTIC_MODEL_SCHEMA = "3"
private const SEMANTIC_MODEL_MAX_SYMBOLS = 1000000
private const SEMANTIC_MODEL_INITIAL_SYMBOL_INDEX_CAPACITY = 256
private const SEMANTIC_MODEL_MAX_SYMBOL_INDEX_CAPACITY = 2097152
private const SEMANTIC_MODEL_INITIAL_MODULE_BUFFER_CAPACITY = 8192
private const SEMANTIC_MODEL_MAX_MODULE_BUFFER_BYTES = 268435456
private const SEMANTIC_MODEL_MAX_NODES_PER_MODEL = 1000000
private const SEMANTIC_MODEL_MAX_EXPRESSIONS_PER_MODEL = 1000000

private type SEMANTIC_MODEL_SYMBOL
	sym         as FBSYMBOL ptr
	state       as integer
end type

private type SEMANTIC_MODEL_SYMBOL_SLOT
	sym         as FBSYMBOL ptr
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
dim shared as integer semantic_model_module_open
dim shared as integer semantic_model_module_failed
dim shared as integer semantic_model_any_failed
dim shared as integer semantic_model_module_count
dim shared as integer semantic_model_module_proc_count
dim shared as integer semantic_model_module_type_fact_count
dim shared as longint semantic_model_module_expression_count
dim shared as longint semantic_model_module_node_count
dim shared as integer semantic_model_proc_count
dim shared as integer semantic_model_total_symbol_count
dim shared as integer semantic_model_total_type_fact_count
dim shared as longint semantic_model_expression_count
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
	dim as string line_text = value + NEWLINE

	hSemanticModelAppendBytes(strptr(line_text), len(line_text))
end sub

'' -------------------------------------------------------------------------
'' Compiler symbol and typed AST serialization
'' -------------------------------------------------------------------------

private function hSemanticModelSymbolHash(byval sym as FBSYMBOL ptr) as uinteger
	dim as ulongint address_value = 0

	memcpy(@address_value, @sym, sizeof(sym))
	address_value xor= address_value shr 4
	address_value xor= address_value shr 13
	address_value xor= address_value shr 23
	address_value xor= address_value shr 37
	return cuint(address_value)
end function

private function hSemanticModelGrowSymbolIndex(byval new_capacity as integer) as integer
	dim as SEMANTIC_MODEL_SYMBOL_SLOT ptr new_index
	dim as uinteger slot_index

	new_index = callocate(new_capacity, sizeof(SEMANTIC_MODEL_SYMBOL_SLOT))
	if( new_index = NULL ) then return FALSE

	for i as integer = 0 to semantic_model_symbol_count - 1
		slot_index = hSemanticModelSymbolHash(semantic_model_symbols[i].sym) and _
			(new_capacity - 1)
		while( new_index[slot_index].sym <> NULL )
			slot_index = (slot_index + 1) and (new_capacity - 1)
		wend
		new_index[slot_index].sym = semantic_model_symbols[i].sym
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

	if( semantic_model_symbol_index_capacity = 0 ) then return -1
	slot_index = hSemanticModelSymbolHash(sym) and _
		(semantic_model_symbol_index_capacity - 1)
	do while( semantic_model_symbol_index[slot_index].sym <> NULL )
		if( semantic_model_symbol_index[slot_index].sym = sym ) then
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

	if( sym = NULL ) then
		return 0
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
		semantic_model_symbols[index].state = 0
		semantic_model_symbol_count += 1

		slot_index = hSemanticModelSymbolHash(sym) and _
			(semantic_model_symbol_index_capacity - 1)
		while( semantic_model_symbol_index[slot_index].sym <> NULL )
			slot_index = (slot_index + 1) and _
				(semantic_model_symbol_index_capacity - 1)
		wend
		semantic_model_symbol_index[slot_index].sym = sym
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

	if( (sym = NULL) or (symbIsVar(sym) = FALSE) or _
		(semantic_model_module_failed) ) then
		exit sub
	end if
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

	if( semantic_model_expression_count + _
		semantic_model_module_expression_count >= _
		SEMANTIC_MODEL_MAX_EXPRESSIONS_PER_MODEL ) then
		semantic_model_module_failed = TRUE
		semantic_model_any_failed = TRUE
		exit sub
	end if

	if( (source_start.start_line < 1) or (source_start.start_column < 0) or _
		(source_end.end_line < source_start.start_line) or _
		(source_end.end_column < 0) ) then
		exit sub
	end if

	dim as string sourcefile = source_start.source_file
	dim as integer physical_range = source_start.is_physical and _
		source_end.is_physical and _
		(nonphysical_tokens_at_start = nonphysical_tokens_at_end) and _
		(source_start.source_file = source_end.source_file)
	dim as longint symbolid = hSemanticModelSymbolId(expr->sym)
	dim as longint subtypeid = hSemanticModelSymbolId(expr->subtype)
	dim as longint expressionid = semantic_model_expression_count + _
		semantic_model_module_expression_count + 1

	hSemanticModelAppendLine("E" + TABCHAR + hSemanticModelNumber(expressionid) + _
		TABCHAR + hSemanticModelNumber(physical_range) + TABCHAR + _
		hSemanticModelEscape(sourcefile) + TABCHAR + _
		hSemanticModelNumber(source_start.start_line) + TABCHAR + _
		hSemanticModelNumber(source_start.start_column) + TABCHAR + _
		hSemanticModelNumber(source_end.end_line) + TABCHAR + _
		hSemanticModelNumber(source_end.end_column) + TABCHAR + _
		hSemanticModelNumber(expr->class) + TABCHAR + _
		hSemanticModelNumber(hSemanticModelNodeOperator(expr)) + TABCHAR + _
		hSemanticModelNumber(astGetFullType(expr)) + TABCHAR + _
		hSemanticModelNumber(symbolid) + TABCHAR + _
		hSemanticModelNumber(subtypeid))
	if( semantic_model_module_failed = FALSE ) then
		semantic_model_module_expression_count += 1
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
	dim as string sourcefile

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
		hSemanticModelAppendLine("N" + TABCHAR + hSemanticModelNumber(current.nodeid) + TABCHAR + _
			hSemanticModelNumber(current.parentid) + TABCHAR + hSemanticModelEdgeName(current.edge) + _
			TABCHAR + hSemanticModelNumber(current.node->class) + TABCHAR + _
			hSemanticModelNumber(hSemanticModelNodeOperator(current.node)) + TABCHAR + _
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

function fbSemanticModelBegin(byref filename as string) as integer
	if( semantic_model_file_open ) then
		close #semantic_model_file_num
		semantic_model_file_open = FALSE
	end if

	semantic_model_filename = filename
	semantic_model_file_num = freefile
	if( open(filename for output as #semantic_model_file_num) <> 0 ) then
		return FALSE
	end if

	semantic_model_file_open = TRUE
	semantic_model_module_open = FALSE
	semantic_model_any_failed = FALSE
	semantic_model_module_count = 0
	semantic_model_proc_count = 0
	semantic_model_total_symbol_count = 0
	semantic_model_total_type_fact_count = 0
	semantic_model_expression_count = 0
	semantic_model_node_count = 0
	print #semantic_model_file_num, "FBCSEM" + TABCHAR + SEMANTIC_MODEL_SCHEMA + _
		TABCHAR + FB_VERSION
	return TRUE ' fbSemanticModelEnd owns this handle for successful and failed compiles.
end function

function fbSemanticModelEnabled() as integer
	return semantic_model_file_open
end function

sub fbSemanticModelBeginModule(byref filename as string)
	if( semantic_model_file_open = FALSE ) then exit sub

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
	semantic_model_module_node_count = 0
	semantic_model_module_open = TRUE
	hSemanticModelAppendLine("M" + TABCHAR + hSemanticModelEscape(filename))
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
		semantic_model_node_count += semantic_model_module_node_count
	end if

	semantic_model_module_buffer_len = 0
	semantic_model_module_open = FALSE
	semantic_model_symbol_count = 0
	semantic_model_module_expression_count = 0
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
		(semantic_model_module_failed) ) then
		exit sub
	end if
	if( (proc = NULL) or (astproc = NULL) ) then exit sub

	procid = hSemanticModelSymbolId(proc)
	next_node_id = semantic_model_node_count + semantic_model_module_node_count
	if( proc->id.name <> NULL ) then procname = *proc->id.name
	if( proc->proc.ext <> NULL ) then
		proc_start_line = proc->proc.ext->dbg.iniline
		proc_end_line = proc->proc.ext->dbg.endline
	end if
	param = symbGetProcHeadParam(proc)
	while( param <> NULL )
		hSemanticModelSymbolId(param)
		if( param->param.var <> NULL ) then
			hSemanticModelSymbolId(param->param.var)
			hSemanticModelExportVariableType(param->param.var, procname)
		end if
		param = param->next
	wend

	symbol = symbGetProcSymbTbHead(proc)
	while( symbol <> NULL )
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

	if( (semantic_model_file_open = FALSE) or (semantic_model_module_open = FALSE) ) then
		exit sub
	end if
	while( (symbol <> NULL) and (semantic_model_module_failed = FALSE) )
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

	if( (succeeded <> FALSE) and (semantic_model_any_failed = FALSE) ) then
		print #semantic_model_file_num, "END" + TABCHAR + SEMANTIC_MODEL_SCHEMA + _
			TABCHAR + hSemanticModelNumber(semantic_model_module_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_proc_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_total_symbol_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_total_type_fact_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_node_count) + TABCHAR + _
			hSemanticModelNumber(semantic_model_expression_count)
	end if

	close #semantic_model_file_num
	semantic_model_file_open = FALSE
	semantic_model_module_open = FALSE
	semantic_model_module_buffer_len = 0
	semantic_model_filename = ""
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
