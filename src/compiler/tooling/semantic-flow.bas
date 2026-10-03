'' Project: FreeBASIC compiler - semantic control flow
'' File: tooling/semantic-flow.bas
'' Purpose: Preserve ordered AST actions and conservative procedure transfers.
'' Responsibilities: Own phase node observations, evaluation order, and CFG edges.
'' This file intentionally does NOT contain: optimization or invented indirect targets.

#include once "tooling/semantic-private.bi"
#include once "runtime/rtl.bi"
#include once "crt/mem.bi"

'' -------------------------------------------------------------------------
'' Checked phase observations
'' -------------------------------------------------------------------------

type SEMANTIC_FLOW_NODE
	astnode as ASTNODE ptr
	identity as longint
	parent as longint
	edge as integer
	left_child as longint
	right_child as longint
	first_auxiliary as longint
	next_auxiliary as longint
end type

type SEMANTIC_FLOW_ACTION
	node as longint
	target as longint
	kind as integer
end type

const FLOW_MAX_NODES = 1000000
const FLOW_MAX_DEPTH = 65536
dim shared as SEMANTIC_FLOW_NODE ptr flow_nodes
dim shared as integer flow_count, flow_capacity
dim shared as longint flow_phase, flow_proc, flow_base

private function hNode(byval identity as longint) as SEMANTIC_FLOW_NODE ptr
	if( (identity <= flow_base) or (identity > flow_base + flow_count) ) then return NULL
	if( flow_nodes[identity - flow_base - 1].identity <> identity ) then return NULL
	return @flow_nodes[identity - flow_base - 1]
end function

function fbSemanticModelPhaseBegin(byval proc as FBSYMBOL ptr, byref phase as const string, byval emitted as integer) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	deallocate(flow_nodes)
	flow_nodes = NULL
	flow_count = 0
	flow_capacity = 0
	flow_base = 0
	flow_proc = fbSemanticModelSymbolId(proc)
	flow_phase = fbSemanticModelNextDetailIdentity( )
	fbSemanticModelAppendDetail("PH" + TABCHAR + fbSemanticModelNumber(flow_phase) + TABCHAR + _
		fbSemanticModelNumber(flow_proc) + TABCHAR + phase + TABCHAR + fbSemanticModelNumber(abs(emitted <> FALSE)))
	return flow_phase
end function

sub fbSemanticModelPhaseEnd( )
	deallocate(flow_nodes)
	flow_nodes = NULL
	flow_count = 0
	flow_capacity = 0
	flow_phase = 0
end sub

sub fbSemanticModelFlowNode(byval node as ASTNODE ptr, byval identity as longint, byval parent as longint, byval edge as integer)
	if( flow_phase = 0 ) then exit sub
	if( flow_count = 0 ) then flow_base = identity - 1
	dim as longint index = identity - flow_base - 1
	if( (index < 0) or (index >= FLOW_MAX_NODES) ) then
		fbSemanticModelFail( )
		exit sub
	end if
	if( index >= flow_capacity ) then
		dim as integer capacity = iif(flow_capacity = 0, 128, flow_capacity)
		do while( index >= capacity )
			capacity *= 2
			if( capacity > FLOW_MAX_NODES ) then capacity = FLOW_MAX_NODES
		loop
		dim as SEMANTIC_FLOW_NODE ptr storage = reallocate(flow_nodes, capacity * sizeof(SEMANTIC_FLOW_NODE))
		if( storage = NULL ) then
			fbSemanticModelFail( )
			exit sub
		end if
		memset(storage + flow_capacity, 0, (capacity - flow_capacity) * sizeof(SEMANTIC_FLOW_NODE))
		flow_nodes = storage
		flow_capacity = capacity
	end if
	'' The serializer allocates child IDs while pushing stack frames, then
	'' visits those frames in LIFO order. Arrival order is therefore different
	'' from identity order. Index by the allocated ID and verify each slot.
	if( flow_nodes[index].identity <> 0 ) then
		fbSemanticModelFail( )
		exit sub
	end if
	with flow_nodes[index]
		.astnode = node
		.identity = identity
		.parent = parent
		.edge = edge
		.left_child = 0
		.right_child = 0
		.first_auxiliary = 0
		.next_auxiliary = 0
	end with
	if( index >= flow_count ) then flow_count = index + 1
	if( edge > 0 ) then
		dim as SEMANTIC_FLOW_NODE ptr owner = hNode(parent)
		if( owner = NULL ) then
			fbSemanticModelFail( )
			exit sub
		end if
		if( edge = 1 ) then owner->left_child = identity
		if( edge = 2 ) then owner->right_child = identity
		if( edge > 2 ) then
			flow_nodes[index].next_auxiliary = owner->first_auxiliary
			owner->first_auxiliary = identity
		end if
	end if
	fbSemanticModelAppendDetail("NP" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(flow_phase))
end sub

'' -------------------------------------------------------------------------
'' Ordered evaluation observed from the AST load contracts
'' -------------------------------------------------------------------------

private sub hOrder(byval parent as longint, byval child as longint, byval ordinal as integer, byref condition as const string)
	if( child = 0 ) then exit sub
	fbSemanticModelAppendDetail("EV" + TABCHAR + fbSemanticModelNumber(parent) + TABCHAR + _
		fbSemanticModelNumber(child) + TABCHAR + fbSemanticModelNumber(ordinal) + TABCHAR + condition)
end sub

private sub hNodeOrder(byval item as SEMANTIC_FLOW_NODE ptr)
	dim as ASTNODE ptr node = item->astnode
	dim as longint identity = item->identity
	select case node->class
	case AST_NODECLASS_ASSIGN
		'' astLoadASSIGN evaluates its RHS before its destination address.
		hOrder(identity, item->right_child, 0, "always")
		hOrder(identity, item->left_child, 1, "always")
	case AST_NODECLASS_CALL
		dim as longint argument = item->right_child
		dim as integer ordinal = 0
		while( argument <> 0 )
			dim as SEMANTIC_FLOW_NODE ptr arg = hNode(argument)
			if( arg = NULL ) then
				fbSemanticModelFail( )
				exit sub
			end if
			hOrder(identity, argument, ordinal, "argument")
			argument = arg->right_child
			ordinal += 1
		wend
		'' Profiling begins after actual argument evaluation; a call through
		'' a pointer evaluates that pointer after arguments and profiling.
		dim as longint auxiliary = item->first_auxiliary
		while( auxiliary <> 0 )
			dim as SEMANTIC_FLOW_NODE ptr action = hNode(auxiliary)
			if( action->edge = 4 ) then hOrder(identity, auxiliary, ordinal, "profile-begin")
			auxiliary = action->next_auxiliary
		wend
		hOrder(identity, item->left_child, ordinal + 1, "call-target")
		auxiliary = item->first_auxiliary
		while( auxiliary <> 0 )
			dim as SEMANTIC_FLOW_NODE ptr action = hNode(auxiliary)
			if( action->edge = 5 ) then hOrder(identity, auxiliary, ordinal + 2, "profile-end")
			if( action->edge = 3 ) then hOrder(identity, auxiliary, ordinal + 3, "copyback")
			auxiliary = action->next_auxiliary
		wend
	case AST_NODECLASS_ARG
		hOrder(identity, item->left_child, 0, "always")
	case AST_NODECLASS_IIF
		dim as SEMANTIC_FLOW_NODE ptr choices = hNode(item->right_child)
		if( choices = NULL ) then exit sub
		dim as SEMANTIC_FLOW_NODE ptr arms = hNode(choices->right_child)
		if( arms = NULL ) then exit sub
		hOrder(identity, choices->left_child, 0, "condition")
		hOrder(identity, arms->left_child, 1, "true")
		hOrder(identity, arms->right_child, 1, "false")
		hOrder(identity, item->left_child, 2, "result")
	case else
		hOrder(identity, item->left_child, 0, "always")
		hOrder(identity, item->right_child, 1, "always")
	end select
end sub

'' -------------------------------------------------------------------------
'' Procedure block membership and conservative transfers
'' -------------------------------------------------------------------------

private sub hEdge(byval source as longint, byval target as longint, byref kind as const string, byval label as FBSYMBOL ptr = NULL)
	fbSemanticModelAppendDetail("CE" + TABCHAR + fbSemanticModelNumber(flow_phase) + TABCHAR + _
		fbSemanticModelNumber(source) + TABCHAR + fbSemanticModelNumber(target) + TABCHAR + kind + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(label)))
end sub

sub fbSemanticModelExportFlow(byval phase as longint)
	if( (phase = 0) or (phase <> flow_phase) ) then exit sub
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	for index as integer = 0 to flow_count - 1
		if( (flow_nodes[index].identity = 0) or (flow_nodes[index].astnode = NULL) ) then
			fbSemanticModelFail( )
			exit sub
		end if
		hNodeOrder(@flow_nodes[index])
	next
	'' Root order is the AST procedure list, never serializer DFS order.
	'' Nested conditional trees retain separate EV path guards above.
	dim as longint previous = 0, ordinal = 0
	for index as integer = 0 to flow_count - 1
		dim as SEMANTIC_FLOW_NODE ptr item = @flow_nodes[index]
		if( (item->edge <> 0) or (item->parent <> flow_proc) ) then continue for
		dim as longint block = fbSemanticModelNextDetailIdentity( )
		fbSemanticModelAppendDetail("CB" + TABCHAR + fbSemanticModelNumber(block) + TABCHAR + fbSemanticModelNumber(phase) + _
			TABCHAR + fbSemanticModelNumber(ordinal))
		fbSemanticModelAppendDetail("CN" + TABCHAR + fbSemanticModelNumber(block) + TABCHAR + fbSemanticModelNumber(item->identity) + TABCHAR + "0")
		if( previous <> 0 ) then hEdge(previous, block, "fallthrough")
		dim as ASTNODE ptr node = item->astnode
		previous = block
		select case node->class
		case AST_NODECLASS_LABEL
			fbSemanticModelAppendDetail("CL" + TABCHAR + fbSemanticModelNumber(phase) + TABCHAR + _
				fbSemanticModelNumber(fbSemanticModelSymbolId(node->sym)) + TABCHAR + fbSemanticModelNumber(block))
		case AST_NODECLASS_BRANCH
			select case node->op.op
			case AST_OP_JMP
				hEdge(block, 0, "label", node->op.ex)
				previous = 0
			case AST_OP_JUMPPTR
				hEdge(block, 0, "unknown-indirect")
				previous = 0
			case AST_OP_RET
				hEdge(block, 0, "subroutine-return")
				previous = 0
			case AST_OP_CALL, AST_OP_CALLPTR
				hEdge(block, 0, "subroutine-call", node->op.ex)
			case else
				hEdge(block, 0, "conditional-label", node->op.ex)
			end select
		case AST_NODECLASS_BOP, AST_NODECLASS_UOP
			if( node->op.ex <> NULL ) then hEdge(block, 0, "conditional-label", node->op.ex)
		case AST_NODECLASS_JMPTB
			for target as integer = 0 to node->jmptb.labelcount - 1
				hEdge(block, 0, "case-label", node->jmptb.labels[target])
			next
			hEdge(block, 0, "default-label", node->jmptb.deflabel)
			previous = 0
		case AST_NODECLASS_ASM
			hEdge(block, 0, "unknown-assembly")
		end select
		ordinal += 1
	next
	if( previous <> 0 ) then hEdge(previous, 0, "procedure-exit")
end sub

'' end of tooling/semantic-flow.bas
