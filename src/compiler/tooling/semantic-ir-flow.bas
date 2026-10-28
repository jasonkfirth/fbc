'' Project: FreeBASIC compiler - native IR control flow
'' File: tooling/semantic-ir-flow.bas
'' Purpose: Preserve outer IR dispatch as a conservative procedure control-flow graph.
'' Responsibilities: Actual transfers, action membership, labels and bounded closure.
'' This file intentionally does NOT contain: code generation or invented indirect targets.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-ir-flow.bi"
#include once "tooling/semantic-loads.bi"
#include once "tooling/semantic-effects.bi"
#include once "runtime/rtl.bi"
#include once "crt/mem.bi"

private const IR_FLOW_LIMIT = 1000000
private const IR_LABEL_SLOT_LIMIT = 2097152

private enum SEMANTIC_IR_EDGE_KIND
	IR_EDGE_FALLTHROUGH = 1
	IR_EDGE_BRANCH
	IR_EDGE_JUMP
	IR_EDGE_CASE
	IR_EDGE_DEFAULT
	IR_EDGE_GOSUB
	IR_EDGE_GOSUB_CONTINUATION
	IR_EDGE_CALL_CONTINUATION
	IR_EDGE_ASM_CONTINUATION
	IR_EDGE_UNKNOWN_INDIRECT
	IR_EDGE_SUBROUTINE_RETURN
	IR_EDGE_UNKNOWN_ASSEMBLY
	IR_EDGE_UNKNOWN_CALL_EXIT
	IR_EDGE_NO_RETURN
	IR_EDGE_PROCEDURE_EXIT
	IR_EDGE_RETURN_VALUE
end enum

private type SEMANTIC_IR_BLOCK
	first_action as longint
	last_action as longint
	first_edge as longint
	reachable as integer
end type

private type SEMANTIC_IR_EDGE
	source as longint
	target as longint
	label as longint
	next_edge as longint
	kind as integer
	guard as integer
end type

private type SEMANTIC_IR_LABEL
	identity as longint
	block as longint
end type

'' Modules and IR emission are serial. Arrays retain copied IDs only. Labels
'' use a checked hash table because globally allocated symbol IDs can be sparse
'' inside one procedure. Backend implementation callbacks remain atomic here.
private dim shared as SEMANTIC_IR_BLOCK ptr blocks
private dim shared as SEMANTIC_IR_EDGE ptr edges
private dim shared as SEMANTIC_IR_LABEL ptr labels
private dim shared as integer block_count, block_capacity, edge_count, edge_capacity, label_count, label_capacity
private dim shared as longint graph_count, total_actions, total_blocks, total_edges, total_cases, total_addresses
private dim shared as longint current_graph, current_proc, action_count, case_count, address_count
private dim shared as longint current_block, previous_block
private dim shared as integer pending_continuation, unknown_transfers

private sub hReleaseProcedure( )
	deallocate(blocks)
	deallocate(edges)
	deallocate(labels)
	blocks = NULL
	edges = NULL
	labels = NULL
	block_count = 0
	block_capacity = 0
	edge_count = 0
	edge_capacity = 0
	label_count = 0
	label_capacity = 0
	current_graph = 0
	current_proc = 0
	action_count = 0
	case_count = 0
	address_count = 0
	current_block = 0
	previous_block = 0
	pending_continuation = 0
	unknown_transfers = FALSE
end sub

sub fbSemanticModelResetIRFlow( )
	hReleaseProcedure( )
	graph_count = 0
	total_actions = 0
	total_blocks = 0
	total_edges = 0
	total_cases = 0
	total_addresses = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hSymbol( byval sym as FBSYMBOL ptr ) as longint
	if( sym = NULL ) then return 0
	fbSemanticModelExportSymbolDetails(sym, FALSE)
	return fbSemanticModelSymbolId(sym)
end function

private function hBackend( ) as string
	select case env.clopt.backend
	case FB_BACKEND_GCC: return "gcc"
	case FB_BACKEND_CLANG: return "clang"
	case FB_BACKEND_LLVM: return "llvm"
	case FB_BACKEND_GAS64: return "gas64"
	case else: return "gas"
	end select
end function

private function hGrowLabels( ) as integer
	dim as integer capacity = iif(label_capacity = 0, 128, label_capacity * 2)
	if( capacity > IR_LABEL_SLOT_LIMIT ) then
		fbSemanticModelFailAt("IR label table exceeds its bound")
		return FALSE
	end if
	dim as SEMANTIC_IR_LABEL ptr storage = callocate(capacity, sizeof(SEMANTIC_IR_LABEL))
	if( storage = NULL ) then
		fbSemanticModelFailAt("IR label table allocation failed")
		return FALSE
	end if
	for index as integer = 0 to label_capacity - 1
		if( labels[index].identity = 0 ) then continue for
		dim as integer slot = (labels[index].identity xor (labels[index].identity shr 13)) and (capacity - 1)
		while( storage[slot].identity <> 0 )
			slot = (slot + 1) and (capacity - 1)
		wend
		storage[slot] = labels[index]
	next
	deallocate(labels)
	labels = storage
	label_capacity = capacity
	return TRUE
end function

private function hLabel( byval identity as longint, byval create as integer ) as SEMANTIC_IR_LABEL ptr
	if( identity <= 0 ) then return NULL
	if( create and ((label_capacity = 0) or ((label_count + 1) * 2 >= label_capacity)) ) then
		if( hGrowLabels( ) = FALSE ) then return NULL
	end if
	if( label_capacity = 0 ) then return NULL
	dim as integer slot = (identity xor (identity shr 13)) and (label_capacity - 1)
	for count as integer = 0 to label_capacity - 1
		if( labels[slot].identity = identity ) then return @labels[slot]
		if( labels[slot].identity = 0 ) then
			if( create = FALSE ) then return NULL
			labels[slot].identity = identity
			labels[slot].block = 0
			label_count += 1
			return @labels[slot]
		end if
		slot = (slot + 1) and (label_capacity - 1)
	next
	fbSemanticModelFailAt("IR label lookup exhausted its table")
	return NULL
end function

private function hNewBlock( ) as longint
	if( (block_count >= IR_FLOW_LIMIT) or (total_blocks >= IR_FLOW_LIMIT) ) then
		fbSemanticModelFailAt("IR block count exceeds its bound")
		return 0
	end if
	if( block_count = block_capacity ) then
		dim as integer capacity = iif(block_capacity = 0, 128, block_capacity * 2)
		if( capacity > IR_FLOW_LIMIT ) then capacity = IR_FLOW_LIMIT
		dim as SEMANTIC_IR_BLOCK ptr storage = reallocate(blocks, capacity * sizeof(SEMANTIC_IR_BLOCK))
		if( storage = NULL ) then
			fbSemanticModelFailAt("IR block allocation failed")
			return 0
		end if
		blocks = storage
		block_capacity = capacity
	end if
	with blocks[block_count]
		.first_action = 0
		.last_action = 0
		.first_edge = 0
		.reachable = FALSE
	end with
	block_count += 1
	total_blocks += 1
	return block_count
end function

private sub hEdge( byval source as longint, byval target as longint, byval label as longint, _
	byval kind as SEMANTIC_IR_EDGE_KIND, byval guard as integer = 0 )
	if( (source < 1) or (source > block_count) or (edge_count >= IR_FLOW_LIMIT) or (total_edges >= IR_FLOW_LIMIT) ) then
		fbSemanticModelFailAt("IR edge owner or count exceeds its bound")
		exit sub
	end if
	if( edge_count = edge_capacity ) then
		dim as integer capacity = iif(edge_capacity = 0, 128, edge_capacity * 2)
		if( capacity > IR_FLOW_LIMIT ) then capacity = IR_FLOW_LIMIT
		dim as SEMANTIC_IR_EDGE ptr storage = reallocate(edges, capacity * sizeof(SEMANTIC_IR_EDGE))
		if( storage = NULL ) then
			fbSemanticModelFailAt("IR edge allocation failed")
			exit sub
		end if
		edges = storage
		edge_capacity = capacity
	end if
	with edges[edge_count]
		.source = source
		.target = target
		.label = label
		.kind = kind
		.guard = guard
		.next_edge = blocks[source - 1].first_edge
	end with
	edge_count += 1
	total_edges += 1
	blocks[source - 1].first_edge = edge_count
end sub

private sub hSplit( byval continuation as integer )
	previous_block = current_block
	pending_continuation = continuation
	current_block = 0
end sub

private function hActionBlock( byval label_action as integer ) as longint
	if( label_action and (current_block <> 0) ) then
		if( blocks[current_block - 1].first_action <> 0 ) then hSplit(IR_EDGE_FALLTHROUGH)
	end if
	if( current_block = 0 ) then
		current_block = hNewBlock( )
		if( current_block = 0 ) then return 0
		if( (previous_block <> 0) and (pending_continuation <> 0) ) then
			hEdge(previous_block, current_block, 0, pending_continuation)
		end if
		previous_block = 0
		pending_continuation = 0
	end if
	return current_block
end function

sub fbSemanticModelIRFlowBegin( byval proc as FBSYMBOL ptr, byval entry_label as FBSYMBOL ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (current_graph <> 0) or (graph_count >= IR_FLOW_LIMIT) or (proc = NULL) ) then
		fbSemanticModelFailAt("IR procedure begins inside another graph or exceeds its bound")
		exit sub
	end if
	if( proc->class <> FB_SYMBCLASS_PROC ) then
		fbSemanticModelFailAt("IR graph has another native procedure class")
		exit sub
	end if
	if( (entry_label = NULL) orElse (entry_label->class <> FB_SYMBCLASS_LABEL) ) then
		fbSemanticModelFailAt("IR graph lacks its actual entry label")
		exit sub
	end if
	graph_count += 1
	current_graph = graph_count
	current_proc = hSymbol(proc)
	current_block = hNewBlock( )
	hProperty("native-ir-procedure:" + fbSemanticModelNumber(current_graph), fbSemanticModelNumber(current_proc) + TABCHAR + _
		fbSemanticModelNumber(hSymbol(entry_label)) + TABCHAR + hBackend( ) + TABCHAR + "outer-ir-dispatch")
end sub

sub fbSemanticModelIRFlowAction( byref kind as const string, byval opcode as integer, _
	byval label as FBSYMBOL ptr, byval callee as FBSYMBOL ptr, byref role as const string, byval argument as longint )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (current_graph = 0) ) then exit sub
	if( (action_count >= IR_FLOW_LIMIT) or (total_actions >= IR_FLOW_LIMIT) ) then
		fbSemanticModelFailAt("IR action count exceeds its bound")
		exit sub
	end if
	select case kind
	case "operation", "label", "conditional", "jump", "gosub", "indirect", "switch", _
		"subroutine-return", "return-value", "assembly", "call"
	case else
		fbSemanticModelFailAt("unknown native IR action kind")
		exit sub
	end select
	if( label <> NULL ) then
		if( label->class <> FB_SYMBCLASS_LABEL ) then fbSemanticModelFailAt("IR transfer has another target class"): exit sub
	end if
	if( callee <> NULL ) then
		if( callee->class <> FB_SYMBCLASS_PROC ) then fbSemanticModelFailAt("IR call has another signature class"): exit sub
	end if
	if( (kind = "label") or (kind = "conditional") or (kind = "jump") or (kind = "gosub") or (kind = "switch") ) then
		if( label = NULL ) then fbSemanticModelFailAt("IR transfer lacks its actual label"): exit sub
	end if
	if( kind = "call" ) then
		if( (callee = NULL) or ((opcode <> 0) and (opcode <> 1)) or (argument < 0) ) then
			fbSemanticModelFailAt("IR call has invalid native target or argument data")
			exit sub
		end if
	end if
	dim as longint block = hActionBlock(kind = "label")
	if( block = 0 ) then exit sub
	dim as longint target = hSymbol(label), selected = hSymbol(callee)
	dim as string policy = role
	if( kind = "call" ) then
		policy = "may-return"
		if( callee <> NULL ) then
			if( symbGetProcIsNoReturn(callee) ) then
				policy = "known-no-return"
			elseif( (opcode = 0) and symbGetIsRTL(callee) and (callee->id.alias <> NULL) ) then
				'' RTL registration owns this alias before backend mangling. Use
				'' that selected declaration, without invoking the native mangler
				'' or classifying arbitrary user/external names as program exit.
				if( *callee->id.alias = FB_RTL_END ) then policy = "runtime-end"
			end if
		end if
	end if
	action_count += 1
	total_actions += 1
	if( blocks[block - 1].first_action = 0 ) then blocks[block - 1].first_action = action_count
	blocks[block - 1].last_action = action_count
	hProperty("native-ir-action:" + fbSemanticModelNumber(current_graph) + ":" + fbSemanticModelNumber(action_count), _
		kind + TABCHAR + fbSemanticModelNumber(block) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentLoad( )) + TABCHAR + _
		fbSemanticModelNumber(opcode) + TABCHAR + fbSemanticModelNumber(target) + TABCHAR + fbSemanticModelNumber(selected) + TABCHAR + _
		policy + TABCHAR + fbSemanticModelNumber(argument))
	if( kind = "call" ) then
		fbSemanticModelEffectCall(callee, opcode, current_graph, action_count, fbSemanticModelCurrentLoad( ))
	else
		fbSemanticModelEffectOperation(kind, opcode, role, current_graph, action_count, fbSemanticModelCurrentLoad( ))
	end if
	select case kind
	case "label"
		dim as SEMANTIC_IR_LABEL ptr slot = hLabel(target, TRUE)
		if( slot = NULL ) then exit sub
		if( slot->block <> 0 ) then fbSemanticModelFailAt("IR label is emitted twice in one procedure"): exit sub
		slot->block = block
	case "conditional"
		hEdge(block, 0, target, IR_EDGE_BRANCH)
		hSplit(IR_EDGE_FALLTHROUGH)
	case "jump"
		hEdge(block, 0, target, IR_EDGE_JUMP)
		hSplit(0)
	case "gosub"
		hEdge(block, 0, target, IR_EDGE_GOSUB)
		hSplit(IR_EDGE_GOSUB_CONTINUATION)
	case "indirect"
		hEdge(block, 0, 0, IR_EDGE_UNKNOWN_INDIRECT)
		unknown_transfers = TRUE
		hSplit(0)
	case "subroutine-return"
		hEdge(block, 0, 0, IR_EDGE_SUBROUTINE_RETURN)
		unknown_transfers = TRUE
		hSplit(0)
	case "return-value"
		hEdge(block, 0, 0, IR_EDGE_RETURN_VALUE)
		hSplit(0)
	case "switch"
		hEdge(block, 0, target, IR_EDGE_DEFAULT)
		hSplit(0)
	case "assembly"
		hEdge(block, 0, 0, IR_EDGE_UNKNOWN_ASSEMBLY)
		unknown_transfers = TRUE
		hSplit(IR_EDGE_ASM_CONTINUATION)
	case "call"
		if( policy = "may-return" ) then
			hEdge(block, 0, 0, IR_EDGE_UNKNOWN_CALL_EXIT)
			unknown_transfers = TRUE
			hSplit(IR_EDGE_CALL_CONTINUATION)
		else
			hEdge(block, 0, 0, IR_EDGE_NO_RETURN)
			if( policy <> "runtime-end" ) then unknown_transfers = TRUE
			hSplit(0)
		end if
	end select
end sub

sub fbSemanticModelIRFlowTable( byval values as ulongint ptr, byval targets as FBSYMBOL ptr ptr, _
	byval count as integer, byval fallback as FBSYMBOL ptr, byval bias as ulongint, byval span as ulongint )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (current_graph = 0) ) then exit sub
	if( (count < 0) or (count > IR_FLOW_LIMIT) or (count > 0 and ((values = NULL) or (targets = NULL))) ) then
		fbSemanticModelFailAt("IR jump table has invalid native bounds")
		exit sub
	end if
	fbSemanticModelIRFlowAction("switch", 0, fallback, NULL, "jump-table", count)
	dim as longint block = previous_block
	if( block = 0 ) then exit sub
	hProperty("native-ir-table:" + fbSemanticModelNumber(current_graph) + ":" + fbSemanticModelNumber(action_count), _
		ltrim(str(bias)) + TABCHAR + ltrim(str(span)))
	for index as integer = 0 to count - 1
		if( (case_count >= IR_FLOW_LIMIT) or (total_cases >= IR_FLOW_LIMIT) ) then
			fbSemanticModelFailAt("IR jump table cases exceed their bound")
			exit sub
		end if
		dim as longint target = hSymbol(targets[index])
		hProperty("native-ir-case:" + fbSemanticModelNumber(current_graph) + ":" + fbSemanticModelNumber(action_count) + ":" + fbSemanticModelNumber(index), _
			ltrim(str(values[index])) + TABCHAR + fbSemanticModelNumber(target))
		hEdge(block, 0, target, IR_EDGE_CASE)
		case_count += 1
		total_cases += 1
	next
end sub

sub fbSemanticModelIRFlowAddress( byval sym as FBSYMBOL ptr )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (current_graph = 0) or (sym = NULL) ) then exit sub
	if( sym->class <> FB_SYMBCLASS_LABEL ) then exit sub
	if( (address_count >= IR_FLOW_LIMIT) or (total_addresses >= IR_FLOW_LIMIT) ) then
		fbSemanticModelFailAt("IR label address count exceeds its bound")
		exit sub
	end if
	address_count += 1
	total_addresses += 1
	hProperty("native-ir-address:" + fbSemanticModelNumber(current_graph) + ":" + fbSemanticModelNumber(address_count), _
		fbSemanticModelNumber(hSymbol(sym)) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentLoad( )))
end sub

private function hEdgeName( byval kind as integer ) as string
	select case kind
	case IR_EDGE_FALLTHROUGH: return "fallthrough"
	case IR_EDGE_BRANCH: return "branch"
	case IR_EDGE_JUMP: return "jump"
	case IR_EDGE_CASE: return "case"
	case IR_EDGE_DEFAULT: return "default"
	case IR_EDGE_GOSUB: return "subroutine-call"
	case IR_EDGE_GOSUB_CONTINUATION: return "subroutine-continuation"
	case IR_EDGE_CALL_CONTINUATION: return "call-continuation"
	case IR_EDGE_ASM_CONTINUATION: return "assembly-continuation"
	case IR_EDGE_UNKNOWN_INDIRECT: return "unknown-indirect"
	case IR_EDGE_SUBROUTINE_RETURN: return "unknown-subroutine-return"
	case IR_EDGE_UNKNOWN_ASSEMBLY: return "unknown-assembly"
	case IR_EDGE_UNKNOWN_CALL_EXIT: return "unknown-call-exit"
	case IR_EDGE_NO_RETURN: return "no-return"
	case IR_EDGE_PROCEDURE_EXIT: return "procedure-exit"
	case IR_EDGE_RETURN_VALUE: return "return-value"
	end select
	fbSemanticModelFailAt("unknown native IR edge kind")
	return "unknown"
end function

sub fbSemanticModelIRFlowEnd( byval proc as FBSYMBOL ptr, byval exit_label as FBSYMBOL ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (current_graph = 0) or (proc = NULL) ) then fbSemanticModelFailAt("IR procedure ends without its graph"): exit sub
	if( fbSemanticModelSymbolId(proc) <> current_proc ) then fbSemanticModelFailAt("IR procedure closes another graph"): exit sub
	if( (exit_label = NULL) orElse (exit_label->class <> FB_SYMBCLASS_LABEL) ) then
		fbSemanticModelFailAt("IR graph lacks its actual exit label")
		exit sub
	end if
	if( current_block <> 0 ) then
		hEdge(current_block, 0, 0, IR_EDGE_PROCEDURE_EXIT)
	elseif( (previous_block <> 0) and (pending_continuation <> 0) ) then
		hEdge(previous_block, 0, 0, IR_EDGE_PROCEDURE_EXIT, pending_continuation)
	end if
	for index as integer = 0 to edge_count - 1
		if( edges[index].label <> 0 ) then
			dim as SEMANTIC_IR_LABEL ptr label = hLabel(edges[index].label, FALSE)
			if( (label = NULL) orElse (label->block = 0) ) then
				fbSemanticModelFailAt("IR transfer target lacks its actual procedure label")
				exit sub
			end if
			edges[index].target = label->block
		end if
	next
	'' Reachability is a conservative graph property, never an execution claim.
	'' Unknown transfers can re-enter through destinations not proved here, so
	'' a disconnected block remains unknown rather than being declared dead.
	dim as longint ptr queue = callocate(block_count, sizeof(longint))
	if( queue = NULL ) then fbSemanticModelFailAt("IR reachability allocation failed"): exit sub
	dim as integer read_index = 0, write_index = 1
	queue[0] = 1
	blocks[0].reachable = TRUE
	while( read_index < write_index )
		dim as longint edge = blocks[queue[read_index] - 1].first_edge
		while( edge <> 0 )
			dim as longint target = edges[edge - 1].target
			if( target <> 0 ) then
				if( blocks[target - 1].reachable = FALSE ) then
					blocks[target - 1].reachable = TRUE
					queue[write_index] = target
					write_index += 1
				end if
			end if
			edge = edges[edge - 1].next_edge
		wend
		read_index += 1
	wend
	deallocate(queue)
	for index as integer = 0 to block_count - 1
		dim as string reachability = "unreachable"
		if( blocks[index].reachable ) then
			reachability = "entry-reachable"
		elseif( unknown_transfers ) then
			reachability = "unknown-entry"
		end if
		hProperty("native-ir-block:" + fbSemanticModelNumber(current_graph) + ":" + fbSemanticModelNumber(index + 1), _
			fbSemanticModelNumber(blocks[index].first_action) + TABCHAR + fbSemanticModelNumber(blocks[index].last_action) + TABCHAR + reachability)
	next
	for index as integer = 0 to edge_count - 1
		hProperty("native-ir-edge:" + fbSemanticModelNumber(current_graph) + ":" + fbSemanticModelNumber(index + 1), _
			fbSemanticModelNumber(edges[index].source) + TABCHAR + fbSemanticModelNumber(edges[index].target) + TABCHAR + _
			hEdgeName(edges[index].kind) + TABCHAR + fbSemanticModelNumber(edges[index].label) + TABCHAR + fbSemanticModelNumber(edges[index].guard))
	next
	hProperty("native-ir-end:" + fbSemanticModelNumber(current_graph), fbSemanticModelNumber(action_count) + TABCHAR + _
		fbSemanticModelNumber(block_count) + TABCHAR + fbSemanticModelNumber(edge_count) + TABCHAR + fbSemanticModelNumber(case_count) + TABCHAR + _
		fbSemanticModelNumber(address_count) + TABCHAR + fbSemanticModelNumber(hSymbol(exit_label)) + TABCHAR + fbSemanticModelNumber(abs(unknown_transfers <> FALSE)))
	hReleaseProcedure( )
end sub

sub fbSemanticModelFinishIRFlow( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		if( current_graph <> 0 ) then fbSemanticModelFailAt("IR procedure graph remains open")
		hProperty("native-ir-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), fbSemanticModelNumber(graph_count) + TABCHAR + _
			fbSemanticModelNumber(total_actions) + TABCHAR + fbSemanticModelNumber(total_blocks) + TABCHAR + fbSemanticModelNumber(total_edges) + TABCHAR + _
			fbSemanticModelNumber(total_cases) + TABCHAR + fbSemanticModelNumber(total_addresses))
	end if
	fbSemanticModelResetIRFlow( )
end sub

'' end of tooling/semantic-ir-flow.bas
