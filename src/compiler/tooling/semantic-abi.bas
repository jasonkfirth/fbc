'' Project: FreeBASIC compiler - frontend ABI snapshots
'' File: tooling/semantic-abi.bas
'' Purpose: Preserve source modes separately from native frontend ABI types.
'' Responsibilities: Initialized result policy, formal lowering and counted closure.
'' This file intentionally does NOT contain: backend decomposition or machine locations.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-abi.bi"

'' Counters belong to the compiler's serial module transaction. This matches
'' the reader bound; staging also limits total output. No native pointer is saved.
private const ABI_SNAPSHOT_LIMIT = 1000000
private dim shared as longint procedure_abi_count, parameter_abi_count

sub fbSemanticModelResetAbi( )
	procedure_abi_count = 0
	parameter_abi_count = 0
end sub

private function hNext( byref count as longint ) as longint
	if( count >= ABI_SNAPSHOT_LIMIT ) then
		fbSemanticModelFailAt("frontend ABI snapshot limit exceeded")
		return 0
	end if
	count += 1
	return count
end function

private sub hProperty( byval identity as longint, byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(identity) + _
		TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hPendingValueType( byval dtype as integer, byval subtype as FBSYMBOL ptr ) as string
	if( dtype = FB_DATATYPE_INVALID ) then return "unavailable"
	select case typeGetDtAndPtrOnly(dtype)
	case FB_DATATYPE_FWDREF
		return "pending-type"
	case FB_DATATYPE_STRUCT
		if( (subtype = NULL) orElse (subtype->class <> FB_SYMBCLASS_STRUCT) ) then
			return "pending-type"
		end if
		'' Recursive headers can precede default-member and return classification.
		'' Reading their ABI as complete here would use a provisional layout.
		if( subtype->udt.retdtype = FB_DATATYPE_INVALID ) then return "pending-layout"
	end select
	return "available"
end function

sub fbSemanticModelProcedureAbi( byval proc as FBSYMBOL ptr, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (proc = NULL) orElse (proc->class <> FB_SYMBCLASS_PROC) ) then
		fbSemanticModelFailAt("procedure ABI snapshot has another symbol class")
		exit sub
	end if
	dim as longint ordinal = hNext(procedure_abi_count)
	if( ordinal = 0 ) then exit sub
	dim as string state = "available", result_kind
	if( symbIsReturnByRef(proc) = FALSE ) then state = hPendingValueType(proc->typ, proc->subtype)
	if( proc->proc.realdtype = FB_DATATYPE_INVALID ) then state = "unavailable"
	if( state = "available" ) then
		if( symbIsReturnByRef(proc) ) then
			result_kind = "byref"
		elseif( symbProcReturnsOnStack(proc) ) then
			result_kind = "hidden"
		else
			result_kind = "value"
		end if
	end if
	hProperty(identity, "frontend-procedure-abi:" + fbSemanticModelNumber(ordinal), state + TABCHAR + _
		fbSemanticModelTypeNumber(culng(proc->typ)) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(proc->subtype)) + TABCHAR + _
		fbSemanticModelTypeNumber(culng(proc->proc.realdtype)) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(proc->proc.realsubtype)) + TABCHAR + result_kind)
end sub

sub fbSemanticModelParameterAbi( byval proc as FBSYMBOL ptr, byval param as FBSYMBOL ptr, _
	byval identity as longint, byval ordinal as integer, byval instance as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (proc = NULL) orElse (proc->class <> FB_SYMBCLASS_PROC) ) then
		fbSemanticModelFailAt("parameter ABI snapshot lacks its procedure")
		exit sub
	end if
	if( (param = NULL) orElse (param->class <> FB_SYMBCLASS_PARAM) ) then
		fbSemanticModelFailAt("parameter ABI snapshot has another symbol class")
		exit sub
	end if
	dim as longint observation = hNext(parameter_abi_count)
	if( observation = 0 ) then exit sub
	dim as string mode_name, state = "available"
	select case param->param.mode
	case FB_PARAMMODE_BYVAL
		mode_name = "byval"
		state = hPendingValueType(param->typ, param->subtype)
	case FB_PARAMMODE_BYREF: mode_name = "byref"
	case FB_PARAMMODE_BYDESC
		mode_name = "bydesc"
		if( param->param.bydescrealsubtype = NULL ) then state = "pending-descriptor"
	case FB_PARAMMODE_VARARG
		mode_name = "vararg"
		state = "variadic"
	case else
		fbSemanticModelFailAt("unknown formal ABI passing mode")
		exit sub
	end select
	dim as string abi_dtype, abi_subtype, classification
	if( state = "available" ) then
		'' This production helper only transforms the two local output values.
		'' It does not resolve a source overload or mutate a formal. Incomplete
		'' Byval layouts and VARARG markers deliberately never reach it.
		dim as integer dtype
		dim as FBSYMBOL ptr subtype
		symbGetRealParamDtype(param, dtype, subtype)
		abi_dtype = fbSemanticModelTypeNumber(culng(dtype))
		abi_subtype = fbSemanticModelNumber(fbSemanticModelSymbolId(subtype))
		select case param->param.mode
		case FB_PARAMMODE_BYVAL
			classification = iif(dtype = param->typ, "value", "implicit-reference")
		case FB_PARAMMODE_BYREF: classification = "reference"
		case FB_PARAMMODE_BYDESC: classification = "descriptor"
		end select
	elseif( state = "variadic" ) then
		classification = "variadic"
	end if
	hProperty(identity, "frontend-parameter-abi:" + fbSemanticModelNumber(observation), _
		fbSemanticModelNumber(fbSemanticModelSymbolId(proc)) + TABCHAR + fbSemanticModelNumber(ordinal) + _
		TABCHAR + state + TABCHAR + mode_name + TABCHAR + fbSemanticModelTypeNumber(culng(param->typ)) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(param->subtype)) + _
		TABCHAR + abi_dtype + TABCHAR + abi_subtype + TABCHAR + fbSemanticModelNumber(param->lgt) + _
		TABCHAR + fbSemanticModelNumber(param->param.regnum) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(param->param.bydescrealsubtype)) + _
		TABCHAR + fbSemanticModelNumber(abs(instance <> FALSE)) + TABCHAR + classification)
end sub

sub fbSemanticModelFinishAbi( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		hProperty(fbSemanticModelSymbolId(@symbGetGlobalNamespc( )), _
			"frontend-abi-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(procedure_abi_count) + TABCHAR + fbSemanticModelNumber(parameter_abi_count))
	end if
	fbSemanticModelResetAbi( )
end sub

'' end of tooling/semantic-abi.bas
