'' Project: FreeBASIC compiler - container data accessor observations
'' File: tooling/semantic-container-data-pointer.bi
'' Purpose: Resolve a hypothetical Data accessor at an actual UDT subscript.
'' Responsibilities: Compiler lookup, overload/access/receiver checks and counts.
'' This file intentionally does NOT assert library layout or construct calls.

dim shared as longint container_data_pointer_count
const SEMANTIC_CONTAINER_DATA_POINTER_LIMIT = 1000000

private sub hResetContainerDataPointer( )
	container_data_pointer_count = 0
end sub

sub fbSemanticModelExportContainerDataPointerCount( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		"container-data-pointer-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + _
		fbSemanticModelNumber(container_data_pointer_count))
end sub

function fbSemanticModelContainerDataPointerCandidate _
	( byval receiver_expr as ASTNODE ptr, byval indexer as FBSYMBOL ptr, _
	  byval receiver_id as longint, byval index_id as longint ) as string
	if( (fbSemanticModelFullEnabled( ) = FALSE) or lex.ctx->semantic_probe ) then return ""
	if( (receiver_expr = NULL) or (indexer = NULL) or (receiver_id <= 0) or (index_id <= 0) ) then return ""
	dim as FBSYMBOL ptr receiver = astGetSubtype(receiver_expr), getter
	if( receiver = NULL ) then return ""
	if( receiver->class <> FB_SYMBCLASS_STRUCT ) then return ""
	dim as string status = "no-data"
	dim as FB_ERRMSG error_number = FB_ERRMSG_OK
	dim as integer compatible = FALSE
	dim as integer saved_probe = lex.ctx->semantic_probe
	lex.ctx->semantic_probe = TRUE
	'' Qualified lookup observes inherited members and local name hiding. An
	'' overload member_chain is owned by the selected procedure head; separate lookup
	'' chains must not be silently merged into a guessed callable member.
	dim as FBSYMCHAIN ptr member_chain = symbLookupAt(receiver, "DATA", FALSE, TRUE)
	if( member_chain <> NULL ) then
		if( member_chain->next <> NULL ) then
			status = "lookup-ambiguous"
		elseif( member_chain->sym = NULL ) then
			status = "not-callable"
		elseif( member_chain->sym->class <> FB_SYMBCLASS_PROC ) then
			status = "not-callable"
		else
			getter = symbFindClosestOvlProc(member_chain->sym, 0, NULL, @error_number, FB_SYMBFINDOPT_NO_ERROR)
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
					compatible = symbCheckConstAssign(symbGetFullType(instance), astGetFullType(receiver_expr), _
						symbGetSubtype(instance), receiver, FB_PARAMMODE_BYREF)
				end if
			end if
		else
			'' Static member calls can discard a receiver; consumers still need
			'' to prove that its original evaluation has no observable effects.
			compatible = TRUE
		end if
	end if
	lex.ctx->semantic_probe = saved_probe
	'' v1: receiver E, original index E, receiver subtype/full dtype, actual
	'' indexer, hypothetical getter, status/error, receiver compatibility,
	'' declared getter result dtype/subtype and BYREF result flag.
	dim as string payload = "1" + TABCHAR + fbSemanticModelNumber(receiver_id) + TABCHAR + _
		fbSemanticModelNumber(index_id) + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(receiver)) + TABCHAR + _
		fbSemanticModelTypeNumber(astGetFullType(receiver_expr)) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(indexer)) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(getter)) + TABCHAR + status + TABCHAR + _
		fbSemanticModelNumber(error_number) + TABCHAR + fbSemanticModelNumber(abs(compatible))
	if( getter <> NULL ) then
		payload += TABCHAR + fbSemanticModelTypeNumber(symbGetFullType(getter)) + TABCHAR + _
			fbSemanticModelNumber(fbSemanticModelSymbolId(symbGetSubtype(getter))) + TABCHAR + _
			fbSemanticModelNumber(abs(symbIsReturnByref(getter)))
		fbSemanticModelExportSymbolDetails(getter)
	else
		payload += TABCHAR + "0" + TABCHAR + "0" + TABCHAR + "0"
	end if
	fbSemanticModelExportSymbolDetails(receiver)
	fbSemanticModelExportSymbolDetails(indexer)
	return payload
end function

sub fbSemanticModelContainerDataPointerResult(byval result as ASTNODE ptr, byref payload as string, _
	byval receiver_id as longint, byval index_id as longint)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or lex.ctx->semantic_probe ) then exit sub
	if( (result = NULL) or (len(payload) = 0) ) then exit sub
	if( result->semantic_expression <= 0 ) then exit sub
	if( container_data_pointer_count >= SEMANTIC_CONTAINER_DATA_POINTER_LIMIT ) then
		fbSemanticModelFailAt("container data pointer observation budget exceeded")
		exit sub
	end if
	fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + _
		fbSemanticModelNumber(result->semantic_expression) + TABCHAR + "container-data-pointer-input" + TABCHAR + _
		fbSemanticModelEscape(payload))
	'' Independent source edges let a consumer reject a payload whose valid
	'' expression identities were substituted from a different subscript.
	dim as string relation_prefix = "H" + TABCHAR + "expression" + TABCHAR + _
		fbSemanticModelNumber(result->semantic_expression) + TABCHAR + "expression" + TABCHAR
	fbSemanticModelAppendDetail(relation_prefix + fbSemanticModelNumber(receiver_id) + TABCHAR + _
		"container-subscript-receiver" + TABCHAR + "0")
	fbSemanticModelAppendDetail(relation_prefix + fbSemanticModelNumber(index_id) + TABCHAR + _
		"container-subscript-index" + TABCHAR + "0")
	container_data_pointer_count += 1
end sub

'' end of tooling/semantic-container-data-pointer.bi
