'' Project: FreeBASIC compiler - collection emptiness observations
'' File: tooling/semantic-collection-empty-impl.bi
'' Purpose: Observe the real lookup and selection of candidate empty accessors.
'' Responsibilities: Original receiver, access/const checks and coverage counts.
'' This file intentionally does NOT synthesize replacement calls or API effects.

dim shared as longint collection_empty_count
dim shared as longint collection_empty_comparison_count
const SEMANTIC_COLLECTION_EMPTY_LIMIT = 1000000

private sub hResetCollectionEmpty( )
	collection_empty_count = 0
	collection_empty_comparison_count = 0
end sub

private function hCollectionEmptyName(byval ordinal as integer) as string
	select case ordinal
	case 0: return "EMPTY"
	case 1: return "ISEMPTY"
	case else: return "IS_EMPTY"
	end select
end function

sub fbSemanticModelExportCollectionEmptyCount( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		"collection-empty-query-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + _
		fbSemanticModelNumber(collection_empty_count))
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		"collection-empty-comparison-query-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + _
		fbSemanticModelNumber(collection_empty_comparison_count))
end sub

sub fbSemanticModelCollectionEmptyBegin(byref observation as FB_SEMANTIC_COLLECTION_EMPTY, _
	byval callee as FBSYMBOL ptr, byval receiver as ASTNODE ptr, byval source_type as FBSYMBOL ptr, byval comparison as integer)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or lex.ctx->semantic_probe ) then exit sub
	if( (callee = NULL) or (receiver = NULL) or (source_type = NULL) ) then exit sub
	if( comparison ) then
		if( (symbIsOperator(callee) = FALSE) or (symbGetProcParams(callee) <> 2) ) then exit sub
		if( (symbGetProcOpOvl(callee) <> AST_OP_EQ) and (symbGetProcOpOvl(callee) <> AST_OP_NE) ) then exit sub
	elseif( symbIsMethod(callee) ) then
		if( symbGetProcParams(callee) <> 1 ) then exit sub
		select case ucase(*symbGetName(callee))
		case "SIZE", "LENGTH", "LEN", "COUNT"
		case else: exit sub
		end select
	else
		if( symbGetProcParams(callee) <> 1 ) then exit sub
		if( symbIsOperator(callee) = FALSE ) then exit sub
		if( symbGetProcOpOvl(callee) <> AST_OP_LEN ) then exit sub
	end if
	'' Recover only the implicit owner conversion. An explicit source cast has
	'' already changed source_type and must retain that selected receiver type.
	dim as ASTNODE ptr original = receiver
	for depth as integer = 0 to 64
		if( astGetSubtype(original) = source_type ) then exit for
		if( (original->class <> AST_NODECLASS_CONV) or (original->l = NULL) ) then exit sub
		original = original->l
	next
	if( (astGetSubtype(original) <> source_type) or (astGetDataType(original) <> FB_DATATYPE_STRUCT) ) then exit sub
	if( original->semantic_expression <= 0 ) then fbSemanticModelExportCurrentExpressionPrefixNonPhysical(original)
	if( original->semantic_expression <= 0 ) then
		fbSemanticModelFailAt("collection receiver source expression is unavailable"): exit sub
	end if
	observation.receiver_id = original->semantic_expression
	for ordinal as integer = 0 to 2
		dim as string getter_name = hCollectionEmptyName(ordinal), status = "no-member"
		dim as FBSYMBOL ptr getter = NULL
		dim as FB_ERRMSG error_number = FB_ERRMSG_OK
		dim as integer compatible = FALSE, saved_probe = lex.ctx->semantic_probe
		lex.ctx->semantic_probe = TRUE
		'' This qualified lookup includes the compiler's actual inherited-name
		'' hiding rules. Query before THIS is lowered; no AST clone is needed.
		dim as FBSYMCHAIN ptr member_chain = symbLookupAt(source_type, getter_name, FALSE, TRUE)
		if( member_chain <> NULL ) then
			if( member_chain->next <> NULL ) then
				status = "lookup-ambiguous"
			elseif( member_chain->sym = NULL ) then
				status = "not-callable"
			elseif( member_chain->sym->class <> FB_SYMBCLASS_PROC ) then
				status = "not-callable"
			else
				dim as FB_SYMBFINDOPT find_options = FB_SYMBFINDOPT_NO_ERROR
				'' Property lookup must select GET rather than the setter path.
				if( symbIsProperty(member_chain->sym) ) then find_options or= FB_SYMBFINDOPT_PROPGET
				getter = symbFindClosestOvlProc(member_chain->sym, 0, NULL, @error_number, find_options)
				if( getter = NULL ) then
					status = iif(error_number = FB_ERRMSG_AMBIGUOUSCALLTOPROC, "ambiguous", "no-match")
				elseif( symbCheckAccess(getter) = FALSE ) then
					status = "inaccessible"
				else
					status = "selected"
				end if
			end if
		end if
		if( getter <> NULL ) then
			if( symbIsMethod(getter) ) then
				dim as FBSYMBOL ptr instance = symbGetProcHeadParam(getter)
				if( instance <> NULL ) then
					if( symbIsInstanceParam(instance) ) then
						compatible = symbCheckConstAssign(symbGetFullType(instance), astGetFullType(original), _
							symbGetSubtype(instance), source_type, FB_PARAMMODE_BYREF)
					end if
				end if
			else
				compatible = TRUE
			end if
		end if
		lex.ctx->semantic_probe = saved_probe
		'' v1: receiver E/subtype/dtype, actual size callee, candidate getter,
		'' lookup status/error, receiver compatibility and source result contract.
		dim as string payload = "1" + TABCHAR + fbSemanticModelNumber(observation.receiver_id) + TABCHAR + _
			fbSemanticModelNumber(fbSemanticModelSymbolId(source_type)) + TABCHAR + fbSemanticModelTypeNumber(astGetFullType(original)) + TABCHAR + _
			fbSemanticModelNumber(fbSemanticModelSymbolId(callee)) + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(getter)) + TABCHAR + _
			status + TABCHAR + fbSemanticModelNumber(error_number) + TABCHAR + fbSemanticModelNumber(abs(compatible))
		if( getter <> NULL ) then
			payload += TABCHAR + fbSemanticModelTypeNumber(symbGetFullType(getter)) + TABCHAR + _
				fbSemanticModelNumber(fbSemanticModelSymbolId(symbGetSubtype(getter))) + TABCHAR + fbSemanticModelNumber(abs(symbIsReturnByref(getter)))
			fbSemanticModelExportSymbolDetails(getter)
		else
			payload += TABCHAR + "0" + TABCHAR + "0" + TABCHAR + "0"
		end if
		observation.payload(ordinal) = payload
	next
	fbSemanticModelExportSymbolDetails(source_type)
	fbSemanticModelExportSymbolDetails(callee)
end sub

'' Global LEN overloads bypass the normal procedure-argument parser. Retain
'' their selected binding on the actual CALL, without building a second call
'' or changing argument construction, expression ownership or evaluation.
function fbSemanticModelCollectionEmptyUnaryCall(byval callee as FBSYMBOL ptr, byval receiver as ASTNODE ptr) as ASTNODE ptr
	if( (fbSemanticModelFullEnabled( ) = FALSE) or lex.ctx->semantic_probe ) then return astBuildCall(callee, receiver)
	dim as FB_SEMANTIC_COLLECTION_EMPTY observation
	dim as LEX_LOCATION source = lexGetLastLocation( )
	if( receiver <> NULL ) then fbSemanticModelCollectionEmptyBegin(observation, callee, receiver, astGetSubtype(receiver))
	dim as ASTNODE ptr result = astBuildCall(callee, receiver)
	if( (result = NULL) or (observation.receiver_id <= 0) ) then return result
	dim as ASTNODE ptr call_node = result
	for depth as integer = 0 to 64
		if( call_node->class = AST_NODECLASS_CALL ) then exit for
		if( (call_node->class <> AST_NODECLASS_CONV) and (call_node->class <> AST_NODECLASS_DEREF) ) then exit for
		if( call_node->l = NULL ) then exit for
		call_node = call_node->l
	next
	if( call_node->class <> AST_NODECLASS_CALL ) then
		fbSemanticModelFailAt("selected LEN call is unavailable"): return result
	end if
	if( call_node->semantic_binding <= 0 ) then
		call_node->semantic_binding = fbSemanticModelExportCallBinding(callee, NULL, source, FALSE)
	end if
	fbSemanticModelCollectionEmptyFinish(observation, call_node->semantic_binding)
	return result
end function

'' Equality overloads consume both operands in astBuildCall. Observe each
'' original UDT first, then attach the selected binding to that one real call.
function fbSemanticModelCollectionEmptyComparisonCall(byval callee as FBSYMBOL ptr, byval left_expr as ASTNODE ptr, byval right_expr as ASTNODE ptr) as ASTNODE ptr
	if( (fbSemanticModelFullEnabled( ) = FALSE) or lex.ctx->semantic_probe ) then return astBuildCall(callee, left_expr, right_expr)
	dim as FB_SEMANTIC_COLLECTION_EMPTY observations(0 to 1)
	dim as LEX_LOCATION source = lexGetLastLocation( )
	if( left_expr <> NULL ) then fbSemanticModelCollectionEmptyBegin(observations(0), callee, left_expr, astGetSubtype(left_expr), TRUE)
	if( right_expr <> NULL ) then fbSemanticModelCollectionEmptyBegin(observations(1), callee, right_expr, astGetSubtype(right_expr), TRUE)
	observations(0).comparison_side = 1: observations(1).comparison_side = 2
	dim as ASTNODE ptr result = astBuildCall(callee, left_expr, right_expr)
	if( result = NULL ) then return result
	if( (observations(0).receiver_id <= 0) and (observations(1).receiver_id <= 0) ) then return result
	dim as ASTNODE ptr call_node = result
	for depth as integer = 0 to 64
		if( call_node->class = AST_NODECLASS_CALL ) then exit for
		if( (call_node->class <> AST_NODECLASS_CONV) and (call_node->class <> AST_NODECLASS_DEREF) ) then exit for
		if( call_node->l = NULL ) then exit for
		call_node = call_node->l
	next
	if( call_node->class <> AST_NODECLASS_CALL ) then
		fbSemanticModelFailAt("selected collection comparison call is unavailable"): return result
	end if
	if( call_node->semantic_binding <= 0 ) then
		call_node->semantic_binding = fbSemanticModelExportCallBinding(callee, NULL, source, FALSE)
	end if
	for side as integer = 0 to 1
		fbSemanticModelCollectionEmptyFinish(observations(side), call_node->semantic_binding)
	next
	return result
end function

sub fbSemanticModelCollectionEmptyFinish(byref observation as FB_SEMANTIC_COLLECTION_EMPTY, byval binding as longint)
	if( (observation.receiver_id <= 0) or (binding <= 0) ) then exit sub
	if( (collection_empty_count > SEMANTIC_COLLECTION_EMPTY_LIMIT - 3) or _
	    (collection_empty_comparison_count > SEMANTIC_COLLECTION_EMPTY_LIMIT - 3) ) then
		fbSemanticModelFailAt("collection empty query budget exceeded"): exit sub
	end if
	dim as string property_prefix = "collection-empty-method:" + fbSemanticModelNumber(binding) + ":"
	dim as string role_text = "collection-size-receiver"
	if( observation.comparison_side > 0 ) then
		property_prefix = "collection-empty-comparison:" + fbSemanticModelNumber(binding) + ":" + _
			fbSemanticModelNumber(observation.comparison_side - 1) + ":"
		role_text = iif(observation.comparison_side = 1, "collection-comparison-left", "collection-comparison-right")
	end if
	for ordinal as integer = 0 to 2
		if( len(observation.payload(ordinal)) = 0 ) then
			fbSemanticModelFailAt("collection empty query set is incomplete"): exit sub
		end if
		fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(observation.receiver_id) + TABCHAR + _
			property_prefix + hCollectionEmptyName(ordinal) + TABCHAR + _
			fbSemanticModelEscape(observation.payload(ordinal)))
		if( observation.comparison_side > 0 ) then
			collection_empty_comparison_count += 1
		else
			collection_empty_count += 1
		end if
	next
	fbSemanticModelAppendDetail("H" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(observation.receiver_id) + TABCHAR + _
		"binding" + TABCHAR + fbSemanticModelNumber(binding) + TABCHAR + role_text + TABCHAR + "0")
end sub

'' end of tooling/semantic-collection-empty-impl.bi
