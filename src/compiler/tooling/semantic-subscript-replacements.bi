'' Project: FreeBASIC compiler - hypothetical subscript observations
'' File: tooling/semantic-subscript-replacements.bi
'' Purpose: Query direct indexing for an observed pointer-getter subscript.
'' Responsibilities: Retain source receiver types and use compiler overload
'' selection without building a call or changing the executable expression.
'' This file intentionally does NOT assert a container's API equivalence.

type SEMANTIC_SUBSCRIPT_RECEIVER
	identity as longint
	receiver as FBSYMBOL ptr
	dtype as integer
end type

'' Ordinary bindings and macro references have separate identity domains.
'' Keep each sorted for logarithmic lookup; module reset releases both arrays.
dim shared as SEMANTIC_SUBSCRIPT_RECEIVER subscript_receivers()
dim shared as SEMANTIC_SUBSCRIPT_RECEIVER subscript_macro_receivers()
dim shared as integer subscript_receiver_count, subscript_macro_receiver_count
dim shared as longint subscript_observation_count
const SEMANTIC_SUBSCRIPT_MAX_RECEIVERS = 1000000

private sub hResetSubscriptReceivers( )
	erase subscript_receivers
	erase subscript_macro_receivers
	subscript_receiver_count = 0
	subscript_macro_receiver_count = 0
	subscript_observation_count = 0
end sub

sub fbSemanticModelExportSubscriptReplacementCount( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		"subscript-replacement-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + _
		fbSemanticModelNumber(subscript_observation_count))
end sub

private function hSubscriptReceiverSlot _
	( records() as SEMANTIC_SUBSCRIPT_RECEIVER, byval count as integer, _
	  byval identity as longint ) as integer
	dim as integer first = 0, last = count
	while( first < last )
		dim as integer middle = first + (last - first) \ 2
		if( records(middle).identity < identity ) then
			first = middle + 1
		else
			last = middle
		end if
	wend
	return first
end function

private sub hRetainSubscriptReceiver _
	( records() as SEMANTIC_SUBSCRIPT_RECEIVER, byref count as integer, _
	  byval identity as longint, byval receiver as FBSYMBOL ptr )
	dim as integer slot = hSubscriptReceiverSlot(records(), count, identity)
	if( slot < count ) then
		if( records(slot).identity = identity ) then
			if( records(slot).receiver <> receiver ) then
				fbSemanticModelFailAt("conflicting source call receiver")
			end if
			exit sub
		end if
	end if
	if( subscript_receiver_count + subscript_macro_receiver_count >= SEMANTIC_SUBSCRIPT_MAX_RECEIVERS ) then
		fbSemanticModelFailAt("source call receiver budget exceeded")
		exit sub
	end if
	if( count > ubound(records) ) then
		dim as integer capacity = 64
		if( count > 0 ) then capacity = count * 2
		if( capacity > SEMANTIC_SUBSCRIPT_MAX_RECEIVERS ) then capacity = SEMANTIC_SUBSCRIPT_MAX_RECEIVERS
		redim preserve records(0 to capacity - 1)
	end if
	for index as integer = count to slot + 1 step -1
		records(index) = records(index - 1)
	next
	records(slot).identity = identity
	records(slot).receiver = receiver
	records(slot).dtype = -1
	count += 1
end sub

sub fbSemanticModelRetainCallReceiver _
	( byval reference as longint, byval receiver as FBSYMBOL ptr )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or lex.ctx->semantic_probe ) then exit sub
	if( (reference = 0) or (receiver = NULL) ) then exit sub
	if( reference > 0 ) then
		hRetainSubscriptReceiver(subscript_receivers(), subscript_receiver_count, reference, receiver)
	else
		'' Macro tags are negated positive ordinals, never the signed minimum.
		hRetainSubscriptReceiver(subscript_macro_receivers(), subscript_macro_receiver_count, -reference, receiver)
	end if
end sub

sub fbSemanticModelRetainCallReceiverType(byval reference as longint, byval dtype as integer)
	if( (fbSemanticModelFullEnabled( ) = FALSE) or lex.ctx->semantic_probe or (reference = 0) ) then exit sub
	dim as integer slot
	if( reference > 0 ) then
		slot = hSubscriptReceiverSlot(subscript_receivers(), subscript_receiver_count, reference)
		if( slot >= subscript_receiver_count ) then exit sub
		if( subscript_receivers(slot).identity = reference ) then subscript_receivers(slot).dtype = dtype
	else
		slot = hSubscriptReceiverSlot(subscript_macro_receivers(), subscript_macro_receiver_count, -reference)
		if( slot >= subscript_macro_receiver_count ) then exit sub
		if( subscript_macro_receivers(slot).identity = -reference ) then subscript_macro_receivers(slot).dtype = dtype
	end if
end sub

private function hGetSubscriptReceiver _
	( records() as SEMANTIC_SUBSCRIPT_RECEIVER, byval count as integer, _
	  byval identity as longint, byref dtype as integer ) as FBSYMBOL ptr
	dim as integer slot = hSubscriptReceiverSlot(records(), count, identity)
	if( slot >= count ) then return NULL
	if( records(slot).identity <> identity ) then return NULL
	dtype = records(slot).dtype
	return records(slot).receiver
end function

sub fbSemanticModelSubscriptReplacement _
	( byval pointer_expr as ASTNODE ptr, byval index_expr as ASTNODE ptr, _
	  byval pointer_id as longint, byval index_id as longint )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or lex.ctx->semantic_probe ) then exit sub
	if( (pointer_expr = NULL) or (index_expr = NULL) or (pointer_id <= 0) or (index_id <= 0) ) then exit sub
	dim as ASTNODE ptr call_expr = astSkipNoConvCAST(pointer_expr)
	'' BYREF pointer getters have a language-value DEREF around the CALL.
	'' The parser may attach the source binding to that result wrapper.
	dim as longint source_binding = call_expr->semantic_binding
	if( call_expr->class = AST_NODECLASS_DEREF ) then
		if( call_expr->l = NULL ) then exit sub
		call_expr = astSkipNoConvCAST(call_expr->l)
	end if
	if( call_expr->class <> AST_NODECLASS_CALL ) then exit sub
	if( call_expr->semantic_binding <> 0 ) then source_binding = call_expr->semantic_binding
	dim as FBSYMBOL ptr getter = call_expr->sym
	if( call_expr->call.semantic_target <> NULL ) then getter = call_expr->call.semantic_target
	if( getter = NULL ) then exit sub
	if( (getter->class <> FB_SYMBCLASS_PROC) or (symbIsMethod(getter) = FALSE) ) then exit sub
	if( typeIsPtr(symbGetFullType(getter)) = FALSE ) then exit sub
	'' The original API has no explicit arguments. Optional arguments would
	'' disappear on replacement and must not be mistaken for this signature.
	dim as FBSYMBOL ptr getter_this = symbGetProcHeadParam(getter)
	if( getter_this = NULL ) then exit sub
	if( (symbIsInstanceParam(getter_this) = FALSE) or (symbGetParamNext(getter_this) <> NULL) ) then exit sub
	if( (call_expr->call.args <> 1) or (call_expr->r = NULL) ) then exit sub
	dim as FBSYMBOL ptr receiver
	dim as integer receiver_dtype = -1
	if( source_binding > 0 ) then
		receiver = hGetSubscriptReceiver(subscript_receivers(), subscript_receiver_count, source_binding, receiver_dtype)
	elseif( source_binding < 0 ) then
		receiver = hGetSubscriptReceiver(subscript_macro_receivers(), subscript_macro_receiver_count, -source_binding, receiver_dtype)
	end if
	if( receiver = NULL ) then exit sub
	if( receiver->class <> FB_SYMBCLASS_STRUCT ) then exit sub
	dim as FBSYMBOL ptr selected, parameter
	dim as FB_ERRMSG error_number = FB_ERRMSG_OK
	dim as string status = "no-overload", mode = "none"
	dim as integer receiver_compatible = FALSE
	if( receiver->udt.ext <> NULL ) then
		dim as FBSYMBOL ptr head = symbGetUDTOpOvlTb(receiver)(AST_OP_PTRINDEX - AST_OP_SELFBASE)
		if( head <> NULL ) then
			dim as FB_CALL_ARG argument
			argument.expr = index_expr
			argument.mode = astBydescArrayArg(index_expr)
			argument.next = NULL
			'' Use the same candidate scorer as the ordinary [] parser. Its
			'' NO_ERROR mode suppresses diagnostics for this hypothetical call.
			'' Probe mode prevents candidate checking from publishing new source
			'' observations. Neither AST construction nor argument lowering runs.
			dim as integer saved_probe = lex.ctx->semantic_probe
			lex.ctx->semantic_probe = TRUE
			selected = symbFindClosestOvlProc(head, 1, @argument, @error_number, FB_SYMBFINDOPT_NO_ERROR)
			if( selected <> NULL ) then
				if( symbCheckAccess(selected) ) then
					status = "selected"
				else
					status = "inaccessible"
				end if
			else
				status = iif(error_number = FB_ERRMSG_AMBIGUOUSCALLTOPROC, "ambiguous", "no-match")
			end if
			lex.ctx->semantic_probe = saved_probe
		end if
	end if
	dim as string payload = "1" + TABCHAR + fbSemanticModelNumber(pointer_id) + TABCHAR + _
		fbSemanticModelNumber(index_id) + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(receiver)) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(getter)) + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(selected)) + TABCHAR + _
		status + TABCHAR + fbSemanticModelNumber(error_number)
	if( selected <> NULL ) then
		dim as FBSYMBOL ptr instance = symbGetProcHeadParam(selected)
		if( instance <> NULL ) then
			if( symbIsInstanceParam(instance) ) then
				parameter = symbGetParamNext(instance)
				'' Argument lowering may add the getter's CONST qualification.
				'' The parser records the object type before that ABI conversion.
				if( receiver_dtype >= 0 ) then
					if( typeIsPtr(receiver_dtype) ) then receiver_dtype = typeDeref(receiver_dtype)
					if( typeGetDtAndPtrOnly(receiver_dtype) = FB_DATATYPE_STRUCT ) then
						receiver_compatible = symbCheckConstAssign(symbGetFullType(instance), _
							receiver_dtype, symbGetSubtype(instance), receiver, FB_PARAMMODE_BYREF)
					end if
				end if
			end if
		end if
	end if
	if( parameter <> NULL ) then
		select case symbGetParamMode(parameter)
		case FB_PARAMMODE_BYVAL: mode = "byval"
		case FB_PARAMMODE_BYREF: mode = "byref"
		case FB_PARAMMODE_BYDESC: mode = "bydesc"
		end select
		payload += TABCHAR + fbSemanticModelTypeNumber(symbGetFullType(parameter)) + TABCHAR + _
			fbSemanticModelNumber(fbSemanticModelSymbolId(symbGetSubtype(parameter))) + TABCHAR + mode
	else
		payload += TABCHAR + "0" + TABCHAR + "0" + TABCHAR + mode
	end if
	if( selected <> NULL ) then
		payload += TABCHAR + fbSemanticModelTypeNumber(symbGetFullType(selected)) + TABCHAR + _
			fbSemanticModelNumber(fbSemanticModelSymbolId(symbGetSubtype(selected))) + TABCHAR + _
			fbSemanticModelNumber(abs(symbIsReturnByref(selected))) + TABCHAR + fbSemanticModelNumber(abs(receiver_compatible))
		fbSemanticModelExportSymbolDetails(selected)
	else
		payload += TABCHAR + "0" + TABCHAR + "0" + TABCHAR + "0" + TABCHAR + "0"
	end if
	fbSemanticModelExportSymbolDetails(receiver)
	fbSemanticModelExportSymbolDetails(getter)
	fbSemanticModelAppendDetail("K" + TABCHAR + "expression" + TABCHAR + fbSemanticModelNumber(index_id) + _
		TABCHAR + "subscript-replacement-input" + TABCHAR + fbSemanticModelEscape(payload))
	subscript_observation_count += 1
end sub

'' end of tooling/semantic-subscript-replacements.bi
