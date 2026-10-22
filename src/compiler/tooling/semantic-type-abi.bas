'' Project: FreeBASIC compiler semantic datatype ABI
'' File: tooling/semantic-type-abi.bas
'' Purpose: Encode native or stable raw datatype identities at the export boundary.
'' Responsibilities: Validate ABI requests and preserve independent type bit fields.
'' This file intentionally does NOT contain: compiler type changes or constant folding.

#include once "tooling/semantic-private.bi"

'' The dedicated semantic compiler can retain the client ABI by default while
'' ordinary compiler builds keep native enums. Both accept an explicit override.
#ifdef FBC_SEMANTIC_STABLE_TYPE_ABI
private dim shared as integer stable_type_abi = TRUE
#else
private dim shared as integer stable_type_abi
#endif

function fbSemanticTypeAbiConfigure(byref request as const string) as integer
	select case request
	case "native": stable_type_abi = FALSE
	case "stable-v1": stable_type_abi = TRUE
	case else: return FALSE
	end select
	return TRUE
end function

function fbSemanticTypeAbiStable( ) as integer
	return stable_type_abi
end function

private function hStableBaseType(byval base_type as integer) as integer
	'' Stable-v1 retains the original schema-27 slots 0..26. New integer
	'' widths use slots 27/28 even when their native compiler enum is inserted
	'' earlier. Keep this explicit so a future enum edit cannot shift clients.
	select case base_type
	case FB_DATATYPE_INTEGER128: return 27
	case FB_DATATYPE_UINTEGER128: return 28
	case FB_DATATYPE_SINGLE: return 15
	case FB_DATATYPE_DOUBLE: return 16
	case FB_DATATYPE_STRING: return 17
	case FB_DATATYPE_FIXSTR: return 18
	case FB_DATATYPE_VA_LIST: return 19
	case FB_DATATYPE_STRUCT: return 20
	case FB_DATATYPE_NAMESPC: return 21
	case FB_DATATYPE_FUNCTION: return 22
	case FB_DATATYPE_FWDREF: return 23
	case FB_DATATYPE_POINTER: return 24
	case FB_DATATYPE_XMMWORD: return 25
	case FB_DATATYPE_USTRING: return 26
	end select
	return base_type
end function

function fbSemanticModelTypeCode(byval dtype as longint) as longint
	if( stable_type_abi = FALSE ) then return dtype
	'' Raw type fields are unsigned 32-bit values. The base and builtin
	'' mangling subtype are independent five-bit fields; pointer depth,
	'' constness, ByRef flags and invalid-type sentinels must survive exactly.
	dim as ulong bits = culng(dtype)
	dim as integer base_type = bits and FB_DT_TYPEMASK
	dim as integer mangled_type = (bits and FB_DT_MANGLEMASK) shr FB_DT_MANGLEPOS
	bits = (bits and not culng(FB_DT_TYPEMASK or FB_DT_MANGLEMASK)) or _
		culng(hStableBaseType(base_type)) or _
		(culng(hStableBaseType(mangled_type)) shl FB_DT_MANGLEPOS)
	return clngint(bits)
end function

function fbSemanticModelTypeNumber(byval dtype as longint) as string
	return fbSemanticModelNumber(fbSemanticModelTypeCode(dtype))
end function

'' end of tooling/semantic-type-abi.bas
