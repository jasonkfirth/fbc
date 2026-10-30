'' Project: FreeBASIC compiler - selected call ABI observations
'' File: tooling/semantic-call-abi.bas
'' Purpose: Preserve actual emitted call components and their carried source context.
'' Responsibilities: Copied identities/types, component order, limits and closure.
'' This file intentionally does NOT contain: ABI selection or retained native pointers.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-call-abi.bi"

'' -------------------------------------------------------------------------
'' Module-owned copied state and formatting helpers
'' -------------------------------------------------------------------------

private const CALL_ABI_LIMIT = 1000000
'' Match the selected signature spelling bound. Facts fail publication rather
'' than silently truncating a complete argument or emitted call expression.
private const CALL_ABI_BYTES = 65536

'' The compiler lowers one module on one thread. AST argument/target lowering
'' finishes before the C/LLVM call formatter runs. Thus one formatter can be
'' active at a time; formatting nested procedure-pointer types does not enter
'' another call formatter. Source context is copied only around the immediate
'' native emitCall/emitCallPtr dispatch, and cleared before profiling/copyback.
private dim shared as longint context_caller, context_expression, context_statement, context_source, context_target
private dim shared as integer context_hidden
private dim shared as longint call_count, argument_count, active_call, active_components
private dim shared as longint builtin_count
private dim shared as string active_stage
private dim shared as integer active_hidden

private function hEnabled( ) as integer
	return fbSemanticModelFullEnabled( ) and ((env.clopt.backend = FB_BACKEND_GCC) or _
		(env.clopt.backend = FB_BACKEND_CLANG) or (env.clopt.backend = FB_BACKEND_LLVM))
end function

private sub hClearContext( )
	context_caller = 0
	context_expression = 0
	context_statement = 0
	context_source = 0
	context_target = 0
	context_hidden = FALSE
end sub

sub fbSemanticModelResetCallAbi( )
	hClearContext( )
	call_count = 0
	argument_count = 0
	builtin_count = 0
	active_call = 0
	active_components = 0
	active_stage = ""
	active_hidden = FALSE
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hType( byval dtype as integer ) as string
	if( dtype = FB_DATATYPE_INVALID ) then return ""
	return fbSemanticModelTypeNumber(culng(dtype))
end function

private function hSubtype( byval dtype as integer, byval subtype as FBSYMBOL ptr ) as string
	if( dtype = FB_DATATYPE_INVALID ) then return ""
	return fbSemanticModelNumber(fbSemanticModelSymbolId(subtype))
end function

'' -------------------------------------------------------------------------
'' Immediate AST-to-backend context and selected call entry
'' -------------------------------------------------------------------------

sub fbSemanticModelCallAbiContext( byval node as ASTNODE ptr, byval caller as FBSYMBOL ptr )
	if( hEnabled( ) = FALSE ) then exit sub
	if( active_call <> 0 ) then
		fbSemanticModelFailAt("selected call ABI context changes during formatting")
		exit sub
	end if
	hClearContext( )
	if( node = NULL ) then exit sub
	if( node->class <> AST_NODECLASS_CALL ) then
		fbSemanticModelFailAt("selected call ABI context has another node class")
		exit sub
	end if
	if( (node->semantic_expression < 0) or (node->semantic_statement < 0) or (node->semantic_source < 0) ) then
		fbSemanticModelFailAt("selected call ABI context has a negative identity")
		exit sub
	end if
	if( caller <> NULL ) then
		if( caller->class <> FB_SYMBCLASS_PROC ) then
			fbSemanticModelFailAt("selected call ABI context has another caller class")
			exit sub
		end if
	end if
	context_caller = fbSemanticModelSymbolId(caller)
	context_expression = node->semantic_expression
	context_statement = node->semantic_statement
	context_source = node->semantic_source
	context_target = fbSemanticModelSymbolId(node->call.semantic_target)
	context_hidden = (node->call.tmpres <> NULL)
end sub

function fbSemanticModelCallAbiBegin( byval proc as FBSYMBOL ptr, byref stage as const string, _
	byval indirect as integer, byval demanded as integer, byval dtype as integer, byval subtype as FBSYMBOL ptr, _
	byref result_type as const string, byref convention as const string, byval level as integer, _
	byval bytestopop as integer ) as longint
	if( hEnabled( ) = FALSE ) then return 0
	if( (proc = NULL) orElse (proc->class <> FB_SYMBCLASS_PROC) ) then
		fbSemanticModelFailAt("selected call ABI has another target class")
		return 0
	end if
	if( (active_call <> 0) or (call_count >= CALL_ABI_LIMIT) or (level < 0) or (level > CALL_ABI_LIMIT) or (bytestopop < 0) or _
		((stage <> "c") and (stage <> "llvm")) or (len(result_type) > CALL_ABI_BYTES) or _
		(len(convention) > CALL_ABI_BYTES) or ((stage = "llvm") <> (env.clopt.backend = FB_BACKEND_LLVM)) or _
		((stage = "c") and ((len(result_type) <> 0) or (len(convention) <> 0))) or _
		((stage = "llvm") and ((len(result_type) = 0) or (dtype = FB_DATATYPE_INVALID))) ) then
		fbSemanticModelFailAt("selected call ABI has invalid state or exceeds its bound")
		return 0
	end if
	call_count += 1
	active_call = call_count
	active_components = 0
	active_stage = stage
	active_hidden = context_hidden
	dim as string dispatch = "direct"
	if( indirect ) then dispatch = "indirect"
	hProperty("emitted-abi-call:" + fbSemanticModelNumber(active_call), _
		fbSemanticModelNumber(fbSemanticModelSymbolId(proc)) + TABCHAR + stage + TABCHAR + dispatch + _
		TABCHAR + fbSemanticModelNumber(context_caller) + TABCHAR + fbSemanticModelNumber(context_expression) + _
		TABCHAR + fbSemanticModelNumber(context_statement) + TABCHAR + fbSemanticModelNumber(context_source) + _
		TABCHAR + fbSemanticModelNumber(context_target) + TABCHAR + fbSemanticModelNumber(abs(demanded <> FALSE)) + _
		TABCHAR + hType(dtype) + TABCHAR + hSubtype(dtype, subtype) + TABCHAR + result_type + TABCHAR + convention + _
		TABCHAR + fbSemanticModelNumber(level) + TABCHAR + fbSemanticModelNumber(bytestopop))
	return active_call
end function

'' -------------------------------------------------------------------------
'' Selected components, alternate builtin route and module closure
'' -------------------------------------------------------------------------

sub fbSemanticModelCallAbiArgument( byval identity as longint, byval param as FBSYMBOL ptr, _
	byval part as integer, byval parts as integer, byref kind as const string, _
	byval input_dtype as integer, byval input_subtype as FBSYMBOL ptr, _
	byval selected_dtype as integer, byval selected_subtype as FBSYMBOL ptr, _
	byref spelling as const string, byref attributes as const string, byref value as const string )
	if( (identity = 0) or (hEnabled( ) = FALSE) ) then exit sub
	if( (identity <> active_call) or (argument_count >= CALL_ABI_LIMIT) or _
		(len(value) = 0) or (len(value) > CALL_ABI_BYTES) or (len(spelling) > CALL_ABI_BYTES) or _
		(len(attributes) > CALL_ABI_BYTES) or (input_dtype = FB_DATATYPE_INVALID) ) then
		fbSemanticModelFailAt("selected call ABI argument has invalid state or exceeds its bound")
		exit sub
	end if
	if( param <> NULL ) then
		if( param->class <> FB_SYMBCLASS_PARAM ) then
			fbSemanticModelFailAt("selected call ABI argument has another formal class")
			exit sub
		end if
	end if
	dim as string selected_kind = kind
	if( (param = NULL) and (selected_kind = "unmapped-value") and active_hidden ) then selected_kind = "hidden-result"
	dim as integer valid = TRUE
	select case selected_kind
	case "hidden-result"
		valid = (param = NULL) and (active_components = 0) and (part = 0) and (parts = 1) and typeIsPtr(selected_dtype)
	case "unmapped-value"
		valid = (param = NULL) and (part = 0) and (parts = 1)
	case "i386-int128-low", "i386-int128-high"
		valid = (active_stage = "llvm") and (param <> NULL) and (parts = 2) and _
			(part = abs(selected_kind = "i386-int128-high")) and (spelling = "i64") and _
			(selected_dtype = FB_DATATYPE_ULONGINT) and (selected_subtype = NULL) and (len(attributes) = 0)
	case "readonly-implementation"
		'' Only the pointee qualifier changes in this internal C call. The
		'' original public signature and procedure-pointer contract stay mutable.
		valid = (active_stage = "c") and (param <> NULL) and (part = 0) and (parts = 1)
		if( valid ) then
			dim as integer original_dtype = any
			dim as FBSYMBOL ptr original_subtype = any
			symbGetRealParamDtype(param, original_dtype, original_subtype)
			valid = (param->param.mode = FB_PARAMMODE_BYVAL) and _
				(typeGetPtrCnt(original_dtype) = 1) and (typeIsConstAt(original_dtype, 1) = FALSE) and _
				(selected_subtype = original_subtype) and _
				(selected_dtype = typeAddrOf(typeSetIsConst(typeDeref(original_dtype))))
			select case as const typeGetDtOnly(original_dtype)
			case FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR, FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE
			case else
				valid = FALSE
			end select
		end if
	case "value", "variadic-value", "memory-byval", "va-list-decay", "array-callback", "libc-adjusted"
		valid = (param <> NULL) and (part = 0) and (parts = 1)
		if( (selected_kind = "memory-byval") or (selected_kind = "va-list-decay") ) then valid and= (active_stage = "llvm")
		if( (selected_kind = "array-callback") or (selected_kind = "libc-adjusted") ) then valid and= (active_stage = "c")
	case else
		valid = FALSE
	end select
	if( selected_kind = "array-callback" ) then
		valid and= (selected_dtype = FB_DATATYPE_INVALID)
	else
		valid and= (selected_dtype <> FB_DATATYPE_INVALID)
	end if
	if( param <> NULL ) then valid and= ((param->param.mode = FB_PARAMMODE_VARARG) = (selected_kind = "variadic-value"))
	if( active_stage = "llvm" ) then
		valid and= (len(spelling) > 0)
	else
		valid and= (len(spelling) = 0)
	end if
	if( (selected_kind <> "hidden-result") and (selected_kind <> "memory-byval") ) then valid and= (len(attributes) = 0)
	if( valid = FALSE ) then
		fbSemanticModelFailAt("selected call ABI argument contradicts its kind")
		exit sub
	end if
	argument_count += 1
	hProperty("emitted-abi-argument:" + fbSemanticModelNumber(argument_count), _
		fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(param)) + _
		TABCHAR + fbSemanticModelNumber(active_components) + TABCHAR + fbSemanticModelNumber(part) + _
		TABCHAR + fbSemanticModelNumber(parts) + TABCHAR + selected_kind + TABCHAR + hType(input_dtype) + _
		TABCHAR + hSubtype(input_dtype, input_subtype) + TABCHAR + hType(selected_dtype) + _
		TABCHAR + hSubtype(selected_dtype, selected_subtype) + TABCHAR + spelling + TABCHAR + attributes + TABCHAR + value)
	active_components += 1
end sub

sub fbSemanticModelCallAbiEnd( byval identity as longint, byref text as const string )
	if( (identity = 0) or (hEnabled( ) = FALSE) ) then exit sub
	if( (active_call <> identity) or (len(text) = 0) or (len(text) > CALL_ABI_BYTES) ) then
		fbSemanticModelFailAt("selected call ABI closure is invalid or exceeds its bound")
		exit sub
	end if
	hProperty("emitted-abi-call-end:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(active_components) + TABCHAR + text)
	active_call = 0
	active_components = 0
	active_stage = ""
	active_hidden = FALSE
end sub

'' LLVM builtin lowering can consume the call queue without entering hDoCall.
'' Keep a separate receipt for that selected route. It does not invent ordinary
'' call arguments or claim the builtin became one machine call instruction.
sub fbSemanticModelCallAbiBuiltin( byval proc as FBSYMBOL ptr, byval level as integer )
	if( hEnabled( ) = FALSE ) then exit sub
	if( (proc = NULL) orElse (proc->class <> FB_SYMBCLASS_PROC) ) then
		fbSemanticModelFailAt("LLVM builtin call observation has another target class")
		exit sub
	end if
	if( (env.clopt.backend <> FB_BACKEND_LLVM) or (active_call <> 0) or _
		(builtin_count >= CALL_ABI_LIMIT) or (level < 0) or (level > CALL_ABI_LIMIT) ) then
		fbSemanticModelFailAt("LLVM builtin call observation is invalid or exceeds its bound")
		exit sub
	end if
	builtin_count += 1
	hProperty("emitted-abi-builtin:" + fbSemanticModelNumber(builtin_count), _
		fbSemanticModelNumber(fbSemanticModelSymbolId(proc)) + TABCHAR + "llvm-builtin-lowering" + _
		TABCHAR + fbSemanticModelNumber(context_caller) + TABCHAR + fbSemanticModelNumber(context_expression) + _
		TABCHAR + fbSemanticModelNumber(context_statement) + TABCHAR + fbSemanticModelNumber(context_source) + _
		TABCHAR + fbSemanticModelNumber(context_target) + TABCHAR + fbSemanticModelNumber(level))
end sub

sub fbSemanticModelFinishCallAbi( byval commit as integer )
	if( commit and hEnabled( ) ) then
		if( active_call <> 0 ) then fbSemanticModelFailAt("selected call ABI formatter remains open")
		hProperty("emitted-abi-call-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(call_count) + TABCHAR + fbSemanticModelNumber(argument_count))
		if( env.clopt.backend = FB_BACKEND_LLVM ) then
			hProperty("emitted-abi-builtin-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), fbSemanticModelNumber(builtin_count))
		end if
	end if
	fbSemanticModelResetCallAbi( )
end sub

'' end of tooling/semantic-call-abi.bas
