'' Project: FreeBASIC compiler - semantic AST loading observations
'' File: tooling/semantic-loads.bas
'' Purpose: Preserve actual load order, inherited origins and late semantic actions.
'' Responsibilities: Bounded serial load intervals and copied call/check choices.
'' This file intentionally does NOT contain: AST loading or IR emission decisions.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-loads.bi"

private const LOAD_LIMIT = 1000000
private const LOAD_DEPTH_LIMIT = 65536

type SEMANTIC_LOAD_FRAME
	identity as longint
	origin as longint
	procedure_id as longint
end type

'' The native loader is recursive and modules are compiled serially. Frames
'' retain copied IDs only, including after the loaded node has been recycled.
private dim shared as SEMANTIC_LOAD_FRAME frames(0 to LOAD_DEPTH_LIMIT - 1)
private dim shared as integer depth
private dim shared as longint load_count, end_count, call_count, check_count
private dim shared as longint shape_count, payload_count, argument_count, jump_count, assembly_count
private dim shared as longint assembly_output_count, assembly_output_tokens

function fbSemanticModelCurrentLoad( ) as longint
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (depth = 0) ) then return 0
	return frames(depth - 1).identity
end function

sub fbSemanticModelResetLoads( )
	depth = 0
	load_count = 0
	end_count = 0
	call_count = 0
	check_count = 0
	shape_count = 0
	payload_count = 0
	argument_count = 0
	jump_count = 0
	assembly_count = 0
	assembly_output_count = 0
	assembly_output_tokens = 0
end sub

private function hNext( byref count as longint ) as longint
	if( count >= LOAD_LIMIT ) then
		fbSemanticModelFailAt("semantic load observation exceeds its bound")
		return 0
	end if
	count += 1
	return count
end function

private sub hProperty( byref key as const string, byref fields as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(fields))
end sub

private function hSymbol( byval sym as FBSYMBOL ptr ) as longint
	if( sym = NULL ) then return 0
	'' Load-time temporaries and intrinsic declarations can be created after
	'' ordinary symbol-table export. Complete metadata before they are freed.
	fbSemanticModelExportSymbolDetails(sym, FALSE)
	return fbSemanticModelSymbolId(sym)
end function

'' -------------------------------------------------------------------------
'' Active payloads before the loader consumes child nodes
'' -------------------------------------------------------------------------

private sub hPayload( byval node as ASTNODE ptr, byval identity as longint )
	if( hNext(shape_count) = 0 ) then exit sub
	hProperty("native-load-shape:" + fbSemanticModelNumber(identity), fbSemanticModelTypeNumber(culng(node->dtype)) + _
		TABCHAR + fbSemanticModelNumber(hSymbol(node->subtype)) + TABCHAR + fbSemanticModelNumber(hSymbol(node->sym)) + _
		TABCHAR + fbSemanticModelNumber(node->vector) + TABCHAR + fbSemanticModelNumber(abs(node->l <> NULL)) + _
		TABCHAR + fbSemanticModelNumber(abs(node->r <> NULL)))
	dim as string kind = "none", fields
	select case node->class
	case AST_NODECLASS_CONST
		kind = "constant"
		dim as string value_kind, value_text
		if( fbSemanticModelFormatValue(node->dtype, @node->val.value, value_kind, value_text) = FALSE ) then
			value_kind = "unavailable"
			value_text = ""
		end if
		fields = value_kind + TABCHAR + value_text + TABCHAR + fbSemanticModelNumber(abs(node->val.hassuffix <> FALSE))
	case AST_NODECLASS_LOAD
		kind = "result-load"
		fields = fbSemanticModelNumber(abs(node->lod.isres <> FALSE))
	case AST_NODECLASS_ASSIGN
		kind = "assignment"
		fields = fbSemanticModelNumber(node->op.options)
	case AST_NODECLASS_BOP, AST_NODECLASS_UOP, AST_NODECLASS_BRANCH
		kind = "operator"
		fields = fbSemanticModelNumber(node->op.op) + TABCHAR + fbSemanticModelOperatorCode(node->op.op) + _
			TABCHAR + fbSemanticModelNumber(node->op.options) + TABCHAR + fbSemanticModelNumber(hSymbol(node->op.ex))
	case AST_NODECLASS_ADDROF
		'' Only the opcode is initialized for this class. Options/ex can be
		'' stale union storage and must not be inspected as operator payload.
		kind = "address-of"
		fields = fbSemanticModelNumber(node->op.op)
	case AST_NODECLASS_CALL
		kind = "call"
		if( (node->call.args < 0) or (node->call.args > LOAD_LIMIT) ) then
			fbSemanticModelFailAt("load-time call argument count exceeds its bound")
			exit sub
		end if
		fields = fbSemanticModelNumber(node->call.args) + TABCHAR + fbSemanticModelNumber(abs(node->call.isrtl <> FALSE)) + _
			TABCHAR + fbSemanticModelNumber(hSymbol(node->call.tmpres)) + TABCHAR + fbSemanticModelNumber(hSymbol(node->call.semantic_target)) + _
			TABCHAR + fbSemanticModelNumber(abs(node->l <> NULL))
		dim as ASTNODE ptr arg = node->r
		dim as integer ordinal = 0
		while( arg <> NULL )
			if( (ordinal >= node->call.args) orElse (arg->class <> AST_NODECLASS_ARG) ) then
				fbSemanticModelFailAt("load-time call has another argument chain")
				exit sub
			end if
			dim as string mode
			select case arg->arg.mode
			case FB_PARAMMODE_BYVAL: mode = "byval"
			case FB_PARAMMODE_BYREF: mode = "byref"
			case FB_PARAMMODE_BYDESC: mode = "bydesc"
			case else: mode = "default"
			end select
			if( hNext(argument_count) = 0 ) then exit sub
			dim as longint origin = 0
			if( arg->l <> NULL ) then origin = arg->l->semantic_load_origin
			hProperty("native-load-argument:" + fbSemanticModelNumber(identity) + ":" + fbSemanticModelNumber(ordinal), _
				fbSemanticModelNumber(hSymbol(arg->sym)) + TABCHAR + mode + TABCHAR + fbSemanticModelNumber(arg->arg.lgt) + _
				TABCHAR + fbSemanticModelTypeNumber(culng(arg->dtype)) + TABCHAR + fbSemanticModelNumber(hSymbol(arg->subtype)) + _
				TABCHAR + fbSemanticModelNumber(origin) + TABCHAR + fbSemanticModelNumber(abs(arg->arg.semantic_defaulted <> FALSE)))
			ordinal += 1
			arg = arg->r
		wend
		if( ordinal <> node->call.args ) then
			fbSemanticModelFailAt("load-time call argument chain is incomplete")
			exit sub
		end if
	case AST_NODECLASS_CONV
		kind = "conversion"
		fields = fbSemanticModelNumber(abs(node->cast.doconv <> FALSE)) + TABCHAR + fbSemanticModelNumber(abs(node->cast.do_convfd2fs <> FALSE)) + _
			TABCHAR + fbSemanticModelNumber(abs(node->cast.convconst <> FALSE))
	case AST_NODECLASS_VAR
		kind = "offset"
		fields = fbSemanticModelNumber(node->var_.ofs)
	case AST_NODECLASS_IDX
		kind = "index"
		fields = fbSemanticModelNumber(node->idx.ofs) + TABCHAR + fbSemanticModelNumber(node->idx.mult)
	case AST_NODECLASS_DEREF
		kind = "offset"
		fields = fbSemanticModelNumber(node->ptr.ofs)
	case AST_NODECLASS_OFFSET
		kind = "offset"
		fields = fbSemanticModelNumber(node->ofs.ofs)
	case AST_NODECLASS_LABEL
		kind = "label"
		fields = fbSemanticModelNumber(abs(node->lbl.flush <> FALSE))
	case AST_NODECLASS_IIF
		kind = "conditional"
		fields = fbSemanticModelNumber(hSymbol(node->iif.falselabel))
	case AST_NODECLASS_LINK
		kind = "sequence"
		select case node->link.ret
		case AST_LINK_RETURN_LEFT: fields = "left"
		case AST_LINK_RETURN_RIGHT: fields = "right"
		case else: fields = "none"
		end select
	case AST_NODECLASS_MEM
		kind = "memory"
		fields = fbSemanticModelNumber(node->mem.op) + TABCHAR + fbSemanticModelOperatorCode(node->mem.op) + _
			TABCHAR + fbSemanticModelNumber(node->mem.bytes) + TABCHAR + fbSemanticModelNumber(node->mem.fillchar)
	case AST_NODECLASS_STACK
		kind = "stack"
		fields = fbSemanticModelNumber(node->stack.op) + TABCHAR + fbSemanticModelOperatorCode(node->stack.op)
	case AST_NODECLASS_MACRO
		kind = "runtime-macro"
		fields = fbSemanticModelNumber(node->op.op) + TABCHAR + fbSemanticModelOperatorCode(node->op.op)
	case AST_NODECLASS_LOOP
		kind = "loop"
		fields = fbSemanticModelNumber(node->op.op) + TABCHAR + fbSemanticModelNumber(hSymbol(node->op.ex))
	case AST_NODECLASS_JMPTB
		kind = "jump-table"
		if( (node->jmptb.labelcount < 0) or (node->jmptb.labelcount > LOAD_LIMIT) ) then
			fbSemanticModelFailAt("load-time jump table exceeds its bound")
			exit sub
		end if
		if( (node->jmptb.labelcount > 0) and ((node->jmptb.values = NULL) or (node->jmptb.labels = NULL)) ) then
			fbSemanticModelFailAt("load-time jump table has no native storage")
			exit sub
		end if
		fields = fbSemanticModelNumber(node->jmptb.labelcount) + TABCHAR + ltrim(str(node->jmptb.bias)) + _
			TABCHAR + ltrim(str(node->jmptb.span)) + TABCHAR + fbSemanticModelNumber(hSymbol(node->jmptb.deflabel))
		for index as integer = 0 to node->jmptb.labelcount - 1
			if( hNext(jump_count) = 0 ) then exit sub
			hProperty("native-load-jump:" + fbSemanticModelNumber(identity) + ":" + fbSemanticModelNumber(index), _
				ltrim(str(node->jmptb.values[index])) + TABCHAR + fbSemanticModelNumber(hSymbol(node->jmptb.labels[index])))
		next
	case AST_NODECLASS_ASM
		kind = "assembly"
		dim as ASTASMTOK ptr token = node->asm.tokhead
		dim as integer ordinal = 0
		while( token <> NULL )
			if( hNext(assembly_count) = 0 ) then exit sub
			dim as string token_kind, text
			dim as longint token_symbol = 0
			select case token->type
			case AST_ASMTOK_TEXT
				token_kind = "text"
				if( token->text <> NULL ) then text = *token->text
			case AST_ASMTOK_SYMB
				token_kind = "symbol"
				token_symbol = hSymbol(token->sym)
			case else
				fbSemanticModelFailAt("load-time assembly token has another native kind")
				exit sub
			end select
			hProperty("native-load-assembly:" + fbSemanticModelNumber(identity) + ":" + fbSemanticModelNumber(ordinal), _
				token_kind + TABCHAR + fbSemanticModelNumber(token_symbol) + TABCHAR + text)
			ordinal += 1
			token = token->next
		wend
		fields = fbSemanticModelNumber(ordinal) + TABCHAR + "unknown-memory-registers-control"
	case AST_NODECLASS_LIT
		kind = "text"
		if( node->lit.text <> NULL ) then fields = *node->lit.text
	case AST_NODECLASS_DBG
		kind = "debug"
		fields = fbSemanticModelNumber(node->dbg.op) + TABCHAR + fbSemanticModelNumber(node->dbg.ex)
		if( node->dbg.filename <> NULL ) then
			fields += TABCHAR + *node->dbg.filename
		else
			fields += TABCHAR
		end if
	end select
	if( hNext(payload_count) = 0 ) then exit sub
	hProperty("native-load-payload:" + fbSemanticModelNumber(identity), kind + TABCHAR + fields)
end sub

sub fbSemanticModelLoadOrigin( byval node as ASTNODE ptr, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (node = NULL) or (identity < 1) ) then
		fbSemanticModelFailAt("semantic load origin has unavailable inputs")
		exit sub
	end if
	'' Clones carry this as provenance. It never claims that a newly created
	'' load-time node is the same occurrence as its snapshot ancestor.
	node->semantic_load_origin = identity
end sub

function fbSemanticModelLoadBegin( byval node as ASTNODE ptr ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	if( node = NULL ) then
		fbSemanticModelFailAt("semantic load entry has no native node")
		return 0
	end if
	if( (depth >= LOAD_DEPTH_LIMIT) or (node->class < 0) or (node->class >= AST_CLASSES) or _
		(node->semantic_load_origin < 0) ) then
		fbSemanticModelFailAt("semantic load entry has invalid depth or node state")
		return 0
	end if
	if( (ast.proc.curr = NULL) orElse (ast.proc.curr->sym = NULL) orElse _
		(ast.proc.curr->sym->class <> FB_SYMBCLASS_PROC) ) then
		fbSemanticModelFailAt("semantic load entry has no native procedure")
		return 0
	end if
	dim as longint identity = hNext(load_count)
	if( identity = 0 ) then return 0
	dim as longint parent_id = 0, inherited = 0
	if( depth > 0 ) then
		parent_id = frames(depth - 1).identity
		inherited = frames(depth - 1).origin
	end if
	dim as longint proc_id = fbSemanticModelSymbolId(ast.proc.curr->sym)
	frames(depth).identity = identity
	frames(depth).origin = iif(node->semantic_load_origin > 0, node->semantic_load_origin, inherited)
	frames(depth).procedure_id = proc_id
	depth += 1
	hProperty("native-load-enter:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(parent_id) + _
		TABCHAR + fbSemanticModelNumber(proc_id) + TABCHAR + fbSemanticModelNumber(node->semantic_load_origin) + _
		TABCHAR + fbSemanticModelNumber(inherited) + TABCHAR + fbSemanticModelNumber(node->class) + _
		TABCHAR + fbSemanticModelNumber(node->semantic_statement) + TABCHAR + fbSemanticModelNumber(node->semantic_source) + _
		TABCHAR + fbSemanticModelNumber(node->semantic_context) + TABCHAR + fbSemanticModelNumber(abs(ast.doemit <> FALSE)) + _
		TABCHAR + fbSemanticModelNumber(depth))
	hPayload(node, identity)
	return identity
end function

sub fbSemanticModelLoadEnd( byval identity as longint, byval has_result as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (depth < 1) orElse (frames(depth - 1).identity <> identity) ) then
		fbSemanticModelFailAt("semantic load return closes another native entry")
		exit sub
	end if
	if( hNext(end_count) = 0 ) then exit sub
	hProperty("native-load-end:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(abs(has_result <> FALSE)))
	depth -= 1
end sub

sub fbSemanticModelLoadCall( byval callee as FBSYMBOL ptr, byval indirect as integer, byref role as const string )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (depth < 1) or (callee = NULL) ) then
		fbSemanticModelFailAt("semantic emitted call has no load or callee")
		exit sub
	end if
	if( (callee->class <> FB_SYMBCLASS_PROC) or ((role <> "call") and (role <> "profile-end")) ) then
		fbSemanticModelFailAt("semantic emitted call has another class or role")
		exit sub
	end if
	'' A callee body may already have been flushed. FALSE avoids following
	'' stale parameter-body variable pointers when completing a late intrinsic.
	fbSemanticModelExportSymbolDetails(callee, FALSE)
	dim as longint identity = hNext(call_count)
	if( identity = 0 ) then exit sub
	hProperty("native-load-call:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(frames(depth - 1).identity) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(callee)) + TABCHAR + fbSemanticModelNumber(abs(indirect <> FALSE)) + _
		TABCHAR + role + TABCHAR + fbSemanticModelNumber(abs(symbGetIsRTL(callee) <> FALSE)))
end sub

sub fbSemanticModelLoadCheck( byref kind as const string, byval continuation as FBSYMBOL ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (depth < 1) or (continuation = NULL) ) then
		fbSemanticModelFailAt("semantic check transfer has no load or continuation")
		exit sub
	end if
	if( (continuation->class <> FB_SYMBCLASS_LABEL) or ((kind <> "bounds-or-dimensions") and (kind <> "null-pointer")) ) then
		fbSemanticModelFailAt("semantic check transfer has another class or kind")
		exit sub
	end if
	fbSemanticModelExportSymbolDetails(continuation, FALSE)
	dim as longint identity = hNext(check_count)
	if( identity = 0 ) then exit sub
	hProperty("native-load-check:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(frames(depth - 1).identity) + _
		TABCHAR + kind + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(continuation)) + TABCHAR + "nonzero-handler" + TABCHAR + "unknown-indirect")
end sub

sub fbSemanticModelLoadAssembly( byval head as ASTASMTOK ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (depth < 1) or (head = NULL) ) then
		fbSemanticModelFailAt("emitted assembly has no native load or token stream")
		exit sub
	end if
	dim as longint identity = hNext(assembly_output_count)
	if( identity = 0 ) then exit sub
	dim as ASTASMTOK ptr token = head
	dim as integer ordinal = 0
	while( token <> NULL )
		if( hNext(assembly_output_tokens) = 0 ) then exit sub
		dim as string kind, text
		dim as longint symbol_id = 0
		select case token->type
		case AST_ASMTOK_TEXT
			kind = "text"
			if( token->text <> NULL ) then text = *token->text
		case AST_ASMTOK_SYMB
			kind = "symbol"
			symbol_id = hSymbol(token->sym)
		case else
			fbSemanticModelFailAt("emitted assembly token has another native kind")
			exit sub
		end select
		hProperty("native-load-assembly-output-token:" + fbSemanticModelNumber(identity) + ":" + fbSemanticModelNumber(ordinal), _
			kind + TABCHAR + fbSemanticModelNumber(symbol_id) + TABCHAR + text)
		ordinal += 1
		token = token->next
	wend
	hProperty("native-load-assembly-output:" + fbSemanticModelNumber(identity), _
		fbSemanticModelNumber(frames(depth - 1).identity) + TABCHAR + fbSemanticModelNumber(ordinal))
end sub

sub fbSemanticModelFinishLoads( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		if( depth <> 0 ) then
			fbSemanticModelFailAt("semantic load stack remains open at module completion")
		else
			hProperty("native-load-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
				fbSemanticModelNumber(load_count) + TABCHAR + fbSemanticModelNumber(end_count) + _
				TABCHAR + fbSemanticModelNumber(call_count) + TABCHAR + fbSemanticModelNumber(check_count))
			hProperty("native-load-payload-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
				fbSemanticModelNumber(shape_count) + TABCHAR + fbSemanticModelNumber(payload_count) + _
				TABCHAR + fbSemanticModelNumber(argument_count) + TABCHAR + fbSemanticModelNumber(jump_count) + TABCHAR + fbSemanticModelNumber(assembly_count))
			hProperty("native-load-assembly-output-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
				fbSemanticModelNumber(assembly_output_count) + TABCHAR + fbSemanticModelNumber(assembly_output_tokens))
		end if
	end if
	fbSemanticModelResetLoads( )
end sub

'' end of tooling/semantic-loads.bas
