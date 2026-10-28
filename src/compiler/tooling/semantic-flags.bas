'' Project: FreeBASIC compiler - normalized semantic flags
'' File: tooling/semantic-flags.bas
'' Purpose: Serialize class-aware flags and actual primitive remap/rank policy.
'' Responsibilities: Self-describing definitions, live snapshots and counted closure.
'' This file intentionally does NOT contain: flag mutation or inferred ABI locations.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-flags.bi"
#include once "tooling/semantic-flag-names.bi"

'' These counters belong to the serial module transaction. Match the bounded
'' detail domain used by the independent reader; staging imposes its own limit.
private const SEMANTIC_FLAGS_LIMIT = 1000000
private dim shared as longint symbol_flag_count, aggregate_flag_count, procedure_flag_count

private sub hProperty( byval identity as longint, byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(identity) + _
		TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hNext( byref count as longint ) as longint
	if( count >= SEMANTIC_FLAGS_LIMIT ) then
		fbSemanticModelFailAt("named flag snapshot limit exceeded")
		return 0
	end if
	count += 1
	return count
end function

sub fbSemanticModelBeginFlags( )
	symbol_flag_count = 0
	aggregate_flag_count = 0
	procedure_flag_count = 0
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint owner = fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))
	for ordinal as integer = 0 to fbSemanticFlagDefinitionCount( ) - 1
		dim as string group_name, flag_name, classes
		dim as ulong bits
		if( fbSemanticFlagDefinition(ordinal, group_name, flag_name, bits, classes) = FALSE ) then
			fbSemanticModelFailAt("named flag definition is unavailable")
			exit sub
		end if
		hProperty(owner, "flag-definition:" + fbSemanticModelNumber(ordinal), group_name + TABCHAR + flag_name + _
			TABCHAR + fbSemanticModelNumber(bits) + TABCHAR + classes)
	next
	hProperty(owner, "flag-definition-count", fbSemanticModelNumber(fbSemanticFlagDefinitionCount( )))
	hProperty(owner, "named-flag-contract", "1")
	'' The mangler reuses datatype slots as private classifiers. In particular,
	'' INTEGER here means C long, regardless of the source INTEGER spelling.
	hProperty(owner, "mangling-modifier-definition:0", fbSemanticModelTypeNumber(FB_DATATYPE_INTEGER) + TABCHAR + "c-long")
	hProperty(owner, "mangling-modifier-definition:1", fbSemanticModelTypeNumber(FB_DATATYPE_UINT) + TABCHAR + "c-unsigned-long")
	hProperty(owner, "mangling-modifier-definition:2", fbSemanticModelTypeNumber(FB_DATATYPE_CHAR) + TABCHAR + "c-char")
	hProperty(owner, "mangling-modifier-definition:3", fbSemanticModelTypeNumber(FB_DATATYPE_BOOLEAN) + TABCHAR + "c-bool")
	hProperty(owner, "mangling-modifier-definition:4", fbSemanticModelTypeNumber(FB_DATATYPE_VA_LIST) + TABCHAR + "c-va-list")
	hProperty(owner, "return-method-definition:0", fbSemanticModelNumber(FB_RETURN_FPU) + TABCHAR + "fpu")
	hProperty(owner, "return-method-definition:1", fbSemanticModelNumber(FB_RETURN_SSE) + TABCHAR + "sse")
	hProperty(owner, "return-method-definition:2", fbSemanticModelNumber(FB_RETURN_DEFAULT) + TABCHAR + "default")
	hProperty(owner, "aggregate-return-definition:0", fbSemanticModelNumber(FB_STRUCT_NONE) + TABCHAR + "none")
	hProperty(owner, "aggregate-return-definition:1", fbSemanticModelNumber(FB_STRUCT_R) + TABCHAR + "integer")
	hProperty(owner, "aggregate-return-definition:2", fbSemanticModelNumber(FB_STRUCT_RR) + TABCHAR + "integer-integer")
	hProperty(owner, "aggregate-return-definition:3", fbSemanticModelNumber(FB_STRUCT_RX) + TABCHAR + "integer-float")
	hProperty(owner, "aggregate-return-definition:4", fbSemanticModelNumber(FB_STRUCT_X) + TABCHAR + "float")
	hProperty(owner, "aggregate-return-definition:5", fbSemanticModelNumber(FB_STRUCT_XR) + TABCHAR + "float-integer")
	hProperty(owner, "aggregate-return-definition:6", fbSemanticModelNumber(FB_STRUCT_XX) + TABCHAR + "float-float")
	hProperty(owner, "integer-promotion-policy", iif(env.clopt.lang = FB_LANG_QB, "disabled-qb", "rank-based"))
end sub

private function hFlags( byval group_id as integer, byval sym as FBSYMBOL ptr, byval bits as ulong ) as string
	dim as ulong unmapped
	dim as string names = fbSemanticNamedFlags(group_id, sym->class, bits, unmapped)
	return names + TABCHAR + fbSemanticModelNumber(unmapped)
end function

sub fbSemanticModelSymbolFlags( byval sym as FBSYMBOL ptr, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint ordinal = hNext(symbol_flag_count)
	if( ordinal = 0 ) then exit sub
	'' Only the common, initialized symbol header is read here. Reused status
	'' bits are decoded by class before any active payload is considered.
	hProperty(identity, "symbol-flags:" + fbSemanticModelNumber(ordinal), _
		hFlags(0, sym, culng(sym->attrib)) + TABCHAR + hFlags(1, sym, culng(sym->pattrib)) + _
		TABCHAR + hFlags(2, sym, culng(sym->stats)))
end sub

sub fbSemanticModelAggregateFlags( byval sym as FBSYMBOL ptr, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (sym = NULL) orElse (sym->class <> FB_SYMBCLASS_STRUCT) ) then
		fbSemanticModelFailAt("aggregate flag snapshot has another symbol class")
		exit sub
	end if
	dim as longint ordinal = hNext(aggregate_flag_count)
	if( ordinal > 0 ) then hProperty(identity, "aggregate-flags:" + fbSemanticModelNumber(ordinal), hFlags(3, sym, culng(sym->udt.options)))
end sub

sub fbSemanticModelProcedureFlags( byval sym as FBSYMBOL ptr, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (sym = NULL) orElse (sym->class <> FB_SYMBCLASS_PROC) ) then
		fbSemanticModelFailAt("procedure status snapshot has another symbol class")
		exit sub
	end if
	if( sym->proc.ext = NULL ) then exit sub
	dim as longint ordinal = hNext(procedure_flag_count)
	if( ordinal > 0 ) then hProperty(identity, "procedure-flags:" + fbSemanticModelNumber(ordinal), hFlags(4, sym, culng(sym->proc.ext->stats)))
end sub

sub fbSemanticModelReturnMethod( byval sym as FBSYMBOL ptr, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as string method_name
	select case sym->proc.returnMethod
	case FB_RETURN_FPU: method_name = "fpu"
	case FB_RETURN_SSE: method_name = "sse"
	case FB_RETURN_DEFAULT: method_name = "default"
	case else
		fbSemanticModelFailAt("unknown procedure return method")
		exit sub
	end select
	hProperty(identity, "return-method-kind", method_name)
end sub

sub fbSemanticModelAggregateReturn( byval classification as integer, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as string method_name
	select case classification
	case FB_STRUCT_NONE: method_name = "none"
	case FB_STRUCT_R: method_name = "integer"
	case FB_STRUCT_RR: method_name = "integer-integer"
	case FB_STRUCT_RX: method_name = "integer-float"
	case FB_STRUCT_X: method_name = "float"
	case FB_STRUCT_XR: method_name = "float-integer"
	case FB_STRUCT_XX: method_name = "float-float"
	case else
		fbSemanticModelFailAt("unknown aggregate register return classification")
		exit sub
	end select
	hProperty(identity, "aggregate-register-return-kind", method_name)
end sub

private function hSizeCategory( byval category as integer ) as string
	select case category
	case -1: return "none"
	case FB_SIZETYPE_BOOLEAN: return "boolean"
	case FB_SIZETYPE_INT8: return "signed-8"
	case FB_SIZETYPE_UINT8: return "unsigned-8"
	case FB_SIZETYPE_INT16: return "signed-16"
	case FB_SIZETYPE_UINT16: return "unsigned-16"
	case FB_SIZETYPE_INT32: return "signed-32"
	case FB_SIZETYPE_UINT32: return "unsigned-32"
	case FB_SIZETYPE_INT64: return "signed-64"
	case FB_SIZETYPE_UINT64: return "unsigned-64"
	case FB_SIZETYPE_INT128: return "signed-128"
	case FB_SIZETYPE_UINT128: return "unsigned-128"
	case FB_SIZETYPE_FLOAT32: return "float-32"
	case FB_SIZETYPE_FLOAT64: return "float-64"
	end select
	fbSemanticModelFailAt("unknown primitive storage category")
	return ""
end function

sub fbSemanticModelPrimitivePolicy( byval dtype as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as integer remap = symbGetDatatypeInfo(dtype).remaptype
	dim as string remap_name
	if( (remap >= 0) and (remap < FB_DATATYPES) ) then
		remap_name = *symbGetDatatypeInfo(remap).name
		if( remap = FB_DATATYPE_FIXSTR ) then remap_name = "fixed-string"
	end if
	dim as string type_name = *symbGetDatatypeInfo(dtype).name
	if( dtype = FB_DATATYPE_FIXSTR ) then type_name = "fixed-string"
	hProperty(fbSemanticModelSymbolId(@symbGetGlobalNamespc( )), "primitive-type-policy:" + fbSemanticModelTypeNumber(culng(dtype)), _
		type_name + TABCHAR + fbSemanticModelNumber(symbGetDatatypeInfo(dtype).intrank) + TABCHAR + _
		remap_name + TABCHAR + hSizeCategory(symbGetDatatypeInfo(dtype).sizetype))
end sub

sub fbSemanticModelFinishFlags( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		hProperty(fbSemanticModelSymbolId(@symbGetGlobalNamespc( )), _
			"named-flag-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(symbol_flag_count) + TABCHAR + fbSemanticModelNumber(aggregate_flag_count) + _
			TABCHAR + fbSemanticModelNumber(procedure_flag_count))
	end if
	symbol_flag_count = 0
	aggregate_flag_count = 0
	procedure_flag_count = 0
end sub

'' end of tooling/semantic-flags.bas
