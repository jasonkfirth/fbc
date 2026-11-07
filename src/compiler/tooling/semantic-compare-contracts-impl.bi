'' Project: FreeBASIC compiler - comparison contract observations
'' File: tooling/semantic-compare-contracts-impl.bi
'' Purpose: Observe actual equality selection for a selected Compare method.
'' Responsibilities: Original operands, silent overload scoring and complete counts.
'' This file intentionally does NOT construct replacement calls or infer contracts.

dim shared as longint semantic_compare_contract_count
const SEMANTIC_COMPARE_CONTRACT_LIMIT = 1000000

private sub hDeleteCompareSnapshot(byval node as ASTNODE ptr)
	if( node = NULL ) then exit sub
	hDeleteCompareSnapshot(node->l)
	hDeleteCompareSnapshot(node->r)
	deallocate(node)
end sub

private function hCopyCompareSnapshot(byval original as ASTNODE ptr, _
	byref work_left as integer, byref complete as integer, byval depth as integer = 0) as ASTNODE ptr
	if( original = NULL ) then return NULL
	work_left -= 1
	if( (work_left < 0) or (depth > 64) ) then complete = FALSE: return NULL
	dim as ASTNODE ptr snapshot = allocate(sizeof(ASTNODE))
	if( snapshot = NULL ) then complete = FALSE: return NULL
	*snapshot = *original
	snapshot->l = NULL: snapshot->r = NULL
	snapshot->prev = NULL: snapshot->next = NULL
	snapshot->l = hCopyCompareSnapshot(original->l, work_left, complete, depth + 1)
	if( complete ) then snapshot->r = hCopyCompareSnapshot(original->r, work_left, complete, depth + 1)
	if( complete = FALSE ) then hDeleteCompareSnapshot(snapshot): return NULL
	return snapshot
end function

constructor FB_SEMANTIC_COMPARE_PROBE( )
	receiver = NULL: callee = NULL
	receiver_id = 0: argument_id = 0
end constructor

destructor FB_SEMANTIC_COMPARE_PROBE( )
	'' Every parser recovery return releases the read-only snapshot. Auxiliary
	'' compiler data and symbols are borrowed; only these node copies are owned.
	if( receiver <> NULL ) then
		hDeleteCompareSnapshot(receiver)
	end if
end destructor

private sub hResetCompareContracts( )
	semantic_compare_contract_count = 0
end sub

sub fbSemanticModelExportCompareContractCount( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		"string-compare-contract-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + _
		fbSemanticModelNumber(semantic_compare_contract_count))
end sub

sub fbSemanticModelCompareBegin(byref observation as FB_SEMANTIC_COMPARE_PROBE, _
	byval callee as FBSYMBOL ptr, byval receiver as ASTNODE ptr, byval source_type as FBSYMBOL ptr)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or lex.ctx->semantic_probe ) then exit sub
	if( (callee = NULL) or (receiver = NULL) or (source_type = NULL) ) then exit sub
	if( symbIsMethod(callee) = FALSE ) then exit sub
	if( ucase(*symbGetName(callee)) <> "COMPARE" ) then exit sub
	if( symbGetProcParams(callee) <> 2 ) then exit sub
	'' Method-owner conversion is implicit. Stop at the type captured by the
	'' variable parser so an explicitly written cast retains its own meaning.
	dim as ASTNODE ptr original = receiver
	for depth as integer = 0 to 64
		if( astGetSubtype(original) = source_type ) then exit for
		if( (original->class <> AST_NODECLASS_CONV) or (original->l = NULL) ) then exit sub
		original = original->l
	next
	if( astGetSubtype(original) <> source_type ) then exit sub
	if( astGetDataType(original) <> FB_DATATYPE_STRUCT ) then exit sub
	if( original->semantic_expression <= 0 ) then fbSemanticModelExportCurrentExpressionPrefixNonPhysical(original)
	if( original->semantic_expression <= 0 ) then exit sub
	'' Overload scoring reads node shapes and types. A normal executable clone
	'' would allocate fresh IIf/LOOP labels and alter subsequent code generation.
	'' This bounded snapshot never enters an AST loader, constructor or deleter.
	'' It neither owns auxiliary compiler data nor registers temporary references.
	dim as integer work_left = 4096, complete = TRUE
	observation.receiver = hCopyCompareSnapshot(original, work_left, complete)
	if( complete = FALSE ) then
		fbSemanticModelFailAt("comparison contract receiver snapshot is incomplete")
		exit sub
	end if
	observation.receiver_id = original->semantic_expression
	observation.callee = callee
end sub

sub fbSemanticModelCompareArgument(byref observation as FB_SEMANTIC_COMPARE_PROBE, _
	byval parameter as FBSYMBOL ptr, byval argument as ASTNODE ptr, byval argument_mode as integer)
	if( observation.receiver = NULL ) then exit sub
	if( parameter <> symbGetProcHeadParam(observation.callee)->next ) then exit sub
	if( argument = NULL ) then
		'' The declaration owns the default expression. Observe it before the
		'' normal argument builder clones/converts it, without taking ownership.
		argument = parameter->param.optexpr
		if( argument = NULL ) then exit sub
	end if
	if( observation.argument_id <> 0 ) then
		fbSemanticModelFailAt("comparison contract argument is repeated"): exit sub
	end if
	if( argument->semantic_expression <= 0 ) then fbSemanticModelExportCurrentExpressionPrefixNonPhysical(argument)
	if( argument->semantic_expression <= 0 ) then exit sub
	observation.argument_id = argument->semantic_expression
	dim as FB_CALL_ARG left_argument = any, right_argument = any
	left_argument.expr = observation.receiver: left_argument.mode = INVALID: left_argument.next = @right_argument
	right_argument.expr = argument: right_argument.mode = INVALID: right_argument.next = NULL
	dim as FBSYMBOL ptr selected, head = symb.globOpOvlTb(AST_OP_EQ).head
	dim as FB_ERRMSG error_number = FB_ERRMSG_OK
	dim as string status = "no-overload", mode_text = "other"
	if( argument_mode = INVALID ) then
		mode_text = "default"
	elseif( argument_mode = FB_PARAMMODE_BYVAL ) then
		mode_text = "byval"
	elseif( argument_mode = FB_PARAMMODE_BYREF ) then
		mode_text = "byref"
	elseif( argument_mode = FB_PARAMMODE_BYDESC ) then
		mode_text = "bydesc"
	end if
	dim as integer left_compatible = FALSE, right_compatible = FALSE
	if( head <> NULL ) then
		dim as integer saved_probe = lex.ctx->semantic_probe
		lex.ctx->semantic_probe = TRUE
		'' This is the same ordered candidate list and BOP scoring used by
		'' symbFindBopOvlProc. Its reporting wrapper is deliberately not called.
		selected = symbFindClosestOvlProc(head, 2, @left_argument, @error_number, _
			FB_SYMBFINDOPT_BOP_OVL or FB_SYMBFINDOPT_NO_ERROR)
		if( selected = NULL ) then
			status = iif(error_number = FB_ERRMSG_AMBIGUOUSCALLTOPROC, "ambiguous", "no-match")
		else
			status = iif(symbCheckAccess(selected), "selected", "inaccessible")
			dim as FBSYMBOL ptr formal_left = symbGetProcHeadParam(selected)
			if( formal_left <> NULL ) then
				left_compatible = symbCheckConstAssign(symbGetFullType(formal_left), astGetFullType(observation.receiver), _
					symbGetSubtype(formal_left), astGetSubtype(observation.receiver), symbGetParamMode(formal_left))
				if( formal_left->next <> NULL ) then
					dim as FBSYMBOL ptr formal_right = formal_left->next
					right_compatible = symbCheckConstAssign(symbGetFullType(formal_right), astGetFullType(argument), _
						symbGetSubtype(formal_right), astGetSubtype(argument), symbGetParamMode(formal_right))
				end if
			end if
		end if
		lex.ctx->semantic_probe = saved_probe
	end if
	'' v1 excludes the binding, which is stored in the property name: receiver
	'' E, argument E, original receiver dtype/subtype, argument dtype/subtype,
	'' actual Compare, prospective equality, status/error, receiver/argument
	'' qualification checks and the explicitly written argument passing mode.
	observation.payload = "1" + TABCHAR + fbSemanticModelNumber(observation.receiver_id) + TABCHAR + _
		fbSemanticModelNumber(observation.argument_id) + TABCHAR + fbSemanticModelTypeNumber(astGetFullType(observation.receiver)) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(astGetSubtype(observation.receiver))) + TABCHAR + _
		fbSemanticModelTypeNumber(astGetFullType(argument)) + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(astGetSubtype(argument))) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(observation.callee)) + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(selected)) + TABCHAR + _
		status + TABCHAR + fbSemanticModelNumber(error_number) + TABCHAR + _
		fbSemanticModelNumber(abs(left_compatible)) + TABCHAR + fbSemanticModelNumber(abs(right_compatible)) + TABCHAR + mode_text
	fbSemanticModelExportSymbolDetails(observation.callee)
	if( selected <> NULL ) then fbSemanticModelExportSymbolDetails(selected)
end sub

sub fbSemanticModelCompareFinish(byref observation as FB_SEMANTIC_COMPARE_PROBE, byval binding as longint)
	if( (observation.receiver = NULL) or (binding <= 0) ) then exit sub
	if( len(observation.payload) = 0 ) then exit sub
	if( semantic_compare_contract_count >= SEMANTIC_COMPARE_CONTRACT_LIMIT ) then
		fbSemanticModelFailAt("comparison contract observation budget exceeded"): exit sub
	end if
	fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(observation.receiver_id) + TABCHAR + _
		"string-compare-contract:" + fbSemanticModelNumber(binding) + TABCHAR + fbSemanticModelEscape(observation.payload))
	'' Binding ordinals are targets in the shared identity reader. Keep that
	'' established edge direction rather than inventing a binding subject domain.
	fbSemanticModelAppendDetail("H" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(observation.receiver_id) + _
		TABCHAR + "binding" + TABCHAR + fbSemanticModelNumber(binding) + TABCHAR + "string-compare-receiver" + TABCHAR + "0")
	fbSemanticModelAppendDetail("H" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(observation.argument_id) + _
		TABCHAR + "binding" + TABCHAR + fbSemanticModelNumber(binding) + TABCHAR + "string-compare-argument" + TABCHAR + "0")
	semantic_compare_contract_count += 1
end sub

'' end of tooling/semantic-compare-contracts-impl.bi
