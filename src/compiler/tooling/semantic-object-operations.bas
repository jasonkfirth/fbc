'' Project: FreeBASIC compiler - native object operations
'' File: tooling/semantic-object-operations.bas
'' Purpose: Preserve actual object builder choices before native trees are consumed.
'' Responsibilities: Copied identities, construction nesting and retained root closure.
'' This file intentionally does NOT contain: another resolver or an execution-order model.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-object-operations.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"

private const OBJECT_OPERATION_LIMIT = 1000000
private const OBJECT_OPERATION_DEPTH = 65536
'' Choice strings share the sidecar's 64 KiB bounded text convention.
private const OBJECT_OPERATION_TEXT_BYTES = 65536

private type SEMANTIC_OBJECT_OPERATION
	nodes as longint
	choices as longint
	closed as integer
	selected as integer
end type

'' Compiler modules run serially. Only copied IDs survive a callback. The
'' bounded stack records compiler work, including nested generated procedures;
'' it does not assert that the corresponding runtime actions all execute.
private type SEMANTIC_OBJECT_OPERATION_LINK
	identity as longint
	previous as longint
end type

'' Immutable attachment links can be shared by cloned nodes. One construction
'' can expose a selected CALL and a later enclosing TYPEINI without rewriting
'' the history already attached to either native tree.
private dim shared as SEMANTIC_OBJECT_OPERATION_LINK ptr links
private dim shared as integer link_count, link_capacity

private dim shared as SEMANTIC_OBJECT_OPERATION ptr operations
private dim shared as integer operation_count, operation_capacity, operation_depth
private dim shared as longint operation_stack(0 to OBJECT_OPERATION_DEPTH - 1)
private dim shared as longint end_count, choice_count, selection_count, node_count

sub fbSemanticModelResetObjectOperations( )
	deallocate(operations)
	deallocate(links)
	links = NULL
	link_count = 0
	link_capacity = 0
	operations = NULL
	operation_count = 0
	operation_capacity = 0
	operation_depth = 0
	end_count = 0
	choice_count = 0
	selection_count = 0
	node_count = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hSymbol( byval sym as FBSYMBOL ptr ) as longint
	if( sym = NULL ) then return 0
	fbSemanticModelExportSymbolDetails(sym, FALSE)
	'' A canceled materialization can disappear before the normal symbol-table
	'' walk. Complete selected formals now, without reading stale body variables.
	if( sym->class = FB_SYMBCLASS_PROC ) then
		dim as FBSYMBOL ptr param = symbGetProcHeadParam(sym)
		dim as integer count = 0
		while( param <> NULL )
			if( (count >= OBJECT_OPERATION_LIMIT) orElse (param->class <> FB_SYMBCLASS_PARAM) ) then
				fbSemanticModelFailAt("object operation has another formal parameter chain")
				return 0
			end if
			fbSemanticModelExportSymbolDetails(param, FALSE)
			count += 1
			param = symbGetParamNext(param)
		wend
	end if
	return fbSemanticModelSymbolId(sym)
end function

private function hActive( byval identity as longint ) as integer
	if( (identity < 1) or (identity > operation_count) or (operation_depth < 1) ) then
		fbSemanticModelFailAt("object operation lacks its active construction frame")
		return FALSE
	end if
	if( operation_stack(operation_depth - 1) <> identity ) then
		fbSemanticModelFailAt("object operations are not nested in native construction order")
		return FALSE
	end if
	return TRUE
end function

function fbSemanticModelObjectOperationBegin( byref kind as const string, _
	byval owner as FBSYMBOL ptr, byval target as FBSYMBOL ptr, _
	byval selected as FBSYMBOL ptr, byval options as longint, byval decision as integer ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	if( lex.ctx <> NULL ) then
		if( lex.ctx->semantic_probe ) then return 0
	end if
	if( (owner = NULL) orElse ((owner->class <> FB_SYMBCLASS_STRUCT) and (owner->class <> FB_SYMBCLASS_CLASS)) ) then
		fbSemanticModelFailAt("object operation lacks its native aggregate owner")
		return 0
	end if
	select case kind
	case "constructor-initialization", "base-initialization", "field-initialization", "union-initialization", _
		"vptr-initialization", "complete-destruction", "field-destruction", "base-destruction", _
		"deleting-destruction", "result-initialization", "default-constructor-call", "destructor-call", _
		"implicit-construction", "reused-construction", "constructor-chain", "base-initializer-source", _
		"generated-member-body", "generated-member-declaration", "generated-assignment-field", "generated-union-copy", "field-exclusion"
	case else
		fbSemanticModelFailAt("unknown native object operation")
		return 0
	end select
	if( (selected <> NULL) andAlso (selected->class <> FB_SYMBCLASS_PROC) ) then
		fbSemanticModelFailAt("object operation selected another symbol class")
		return 0
	end if
	if( (operation_count >= OBJECT_OPERATION_LIMIT) or (operation_depth >= OBJECT_OPERATION_DEPTH) ) then
		fbSemanticModelFailAt("object operation count or depth exceeds its bound")
		return 0
	end if
	if( operation_count = operation_capacity ) then
		dim as integer capacity = iif(operation_capacity = 0, 128, operation_capacity * 2)
		if( capacity > OBJECT_OPERATION_LIMIT ) then capacity = OBJECT_OPERATION_LIMIT
		dim as SEMANTIC_OBJECT_OPERATION ptr storage = reallocate(operations, capacity * sizeof(SEMANTIC_OBJECT_OPERATION))
		if( storage = NULL ) then
			fbSemanticModelFailAt("object operation allocation failed")
			return 0
		end if
		operations = storage
		operation_capacity = capacity
	end if
	dim as longint parent = 0, procedure = 0
	if( operation_depth > 0 ) then parent = operation_stack(operation_depth - 1)
	if( ast.proc.curr <> NULL ) then
		if( ast.proc.curr->sym <> NULL ) then procedure = hSymbol(ast.proc.curr->sym)
	end if
	dim as longint owner_id = hSymbol(owner), target_id = hSymbol(target), selected_id = hSymbol(selected)
	operation_count += 1
	dim as longint identity = operation_count
	with operations[identity - 1]
		.nodes = 0
		.choices = 0
		.closed = FALSE
		.selected = FALSE
	end with
	operation_stack(operation_depth) = identity
	operation_depth += 1
	hProperty("native-object-operation:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(parent) + TABCHAR + _
		kind + TABCHAR + fbSemanticModelNumber(procedure) + TABCHAR + fbSemanticModelNumber(owner_id) + TABCHAR + _
		fbSemanticModelNumber(target_id) + TABCHAR + fbSemanticModelNumber(selected_id) + TABCHAR + _
		fbSemanticModelNumber(options) + TABCHAR + fbSemanticModelNumber(decision) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )))
	return identity
end function

sub fbSemanticModelObjectOperationChoice( byref kind as const string, byref value as const string )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( operation_depth = 0 ) then exit sub
	if( lex.ctx <> NULL ) then
		if( lex.ctx->semantic_probe ) then exit sub
	end if
	if( (choice_count >= OBJECT_OPERATION_LIMIT) or (len(kind) = 0) or (len(value) > OBJECT_OPERATION_TEXT_BYTES) ) then
		fbSemanticModelFailAt("object operation choice exceeds its bound")
		exit sub
	end if
	dim as longint identity = operation_stack(operation_depth - 1)
	operations[identity - 1].choices += 1
	choice_count += 1
	hProperty("native-object-choice:" + fbSemanticModelNumber(identity) + ":" + _
		fbSemanticModelNumber(operations[identity - 1].choices), kind + TABCHAR + value)
end sub

sub fbSemanticModelObjectOperationNumberChoice( byref kind as const string, byval value as longint )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (operation_depth = 0) ) then exit sub
	fbSemanticModelObjectOperationChoice(kind, fbSemanticModelNumber(value))
end sub

sub fbSemanticModelObjectOperationSymbolChoice( byref kind as const string, byval sym as FBSYMBOL ptr )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (operation_depth = 0) ) then exit sub
	if( lex.ctx <> NULL ) then
		if( lex.ctx->semantic_probe ) then exit sub
	end if
	if( sym = NULL ) then
		fbSemanticModelFailAt("object operation choice lacks its selected symbol")
		exit sub
	end if
	fbSemanticModelObjectOperationChoice(kind, fbSemanticModelNumber(hSymbol(sym)))
end sub

private sub hAttach( byval identity as longint, byval root as ASTNODE ptr )
	if( root = NULL ) then exit sub
	dim as longint previous = root->semantic_object_operation
	while( previous <> 0 )
		if( (previous < 1) or (previous > link_count) ) then
			fbSemanticModelFailAt("object operation root has another observation chain")
			exit sub
		end if
		if( links[previous - 1].identity = identity ) then exit sub
		previous = links[previous - 1].previous
	wend
	if( link_count >= OBJECT_OPERATION_LIMIT ) then
		fbSemanticModelFailAt("object operation root link count exceeds its bound")
		exit sub
	end if
	if( link_count = link_capacity ) then
		dim as integer capacity = iif(link_capacity = 0, 128, link_capacity * 2)
		if( capacity > OBJECT_OPERATION_LIMIT ) then capacity = OBJECT_OPERATION_LIMIT
		dim as SEMANTIC_OBJECT_OPERATION_LINK ptr storage = reallocate(links, capacity * sizeof(SEMANTIC_OBJECT_OPERATION_LINK))
		if( storage = NULL ) then
			fbSemanticModelFailAt("object operation root link allocation failed")
			exit sub
		end if
		links = storage
		link_capacity = capacity
	end if
	links[link_count].identity = identity
	links[link_count].previous = root->semantic_object_operation
	link_count += 1
	root->semantic_object_operation = link_count
end sub

sub fbSemanticModelObjectOperationSelection( byval identity as longint, byval root as ASTNODE ptr )
	if( identity = 0 ) then exit sub
	if( hActive(identity) = FALSE ) then exit sub
	if( operations[identity - 1].selected ) then
		fbSemanticModelFailAt("object operation repeats its selected call")
		exit sub
	end if
	dim as longint selected = 0, node_class = 0
	if( root <> NULL ) then
		node_class = root->class
		if( root->class = AST_NODECLASS_CALL ) then selected = hSymbol(root->sym)
	end if
	hAttach(identity, root)
	operations[identity - 1].selected = TRUE
	selection_count += 1
	hProperty("native-object-selection:" + fbSemanticModelNumber(identity), _
		fbSemanticModelNumber(selected) + TABCHAR + fbSemanticModelNumber(node_class))
end sub

sub fbSemanticModelObjectOperationEnd( byval identity as longint, byval root as ASTNODE ptr, byref outcome as const string )
	if( identity = 0 ) then exit sub
	if( hActive(identity) = FALSE ) then exit sub
	select case outcome
	case "built", "empty", "shallow", "error", "reused", "skipped", "complete"
	case else
		fbSemanticModelFailAt("unknown native object operation outcome")
		exit sub
	end select
	dim as longint node_class = 0
	if( root <> NULL ) then
		node_class = root->class
		hAttach(identity, root)
	end if
	operations[identity - 1].closed = TRUE
	operation_depth -= 1
	end_count += 1
	hProperty("native-object-end:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(abs(root <> NULL)) + TABCHAR + _
		fbSemanticModelNumber(node_class) + TABCHAR + outcome + TABCHAR + fbSemanticModelNumber(operations[identity - 1].choices))
end sub

sub fbSemanticModelObjectOperationNode( byval node as ASTNODE ptr, byval node_id as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (node = NULL) or (node_id <= 0) ) then
		fbSemanticModelFailAt("object operation projection lacks its retained node")
		exit sub
	end if
	dim as longint link_id = node->semantic_object_operation
	while( link_id <> 0 )
		if( (link_id < 1) or (link_id > link_count) or (node_count >= OBJECT_OPERATION_LIMIT) ) then
			fbSemanticModelFailAt("object operation projection exceeds its bound")
			exit sub
		end if
		dim as longint identity = links[link_id - 1].identity
		if( (identity < 1) or (identity > operation_count) ) then
			fbSemanticModelFailAt("object operation projection has another event")
			exit sub
		end if
		if( operations[identity - 1].closed = FALSE ) then
			fbSemanticModelFailAt("object operation projected an unfinished root")
			exit sub
		end if
		operations[identity - 1].nodes += 1
		node_count += 1
		fbSemanticModelAppendDetail("K" + TABCHAR + "node" + TABCHAR + fbSemanticModelNumber(node_id) + TABCHAR + _
			"native-object-operation:" + fbSemanticModelNumber(identity) + TABCHAR + "1")
		link_id = links[link_id - 1].previous
	wend
end sub

sub fbSemanticModelFinishObjectOperations( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		if( operation_depth <> 0 ) then fbSemanticModelFailAt("object construction frames remain open")
		for identity as integer = 1 to operation_count
			hProperty("native-object-nodes:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(operations[identity - 1].nodes))
		next
		hProperty("native-object-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(operation_count) + TABCHAR + fbSemanticModelNumber(end_count) + TABCHAR + _
			fbSemanticModelNumber(choice_count) + TABCHAR + fbSemanticModelNumber(selection_count) + TABCHAR + fbSemanticModelNumber(node_count))
	end if
	fbSemanticModelResetObjectOperations( )
end sub

'' end of tooling/semantic-object-operations.bas
