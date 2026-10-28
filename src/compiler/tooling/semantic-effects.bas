'' Project: FreeBASIC compiler - semantic effects
'' File: tooling/semantic-effects.bas
'' Purpose: Preserve selected runtime contracts without changing native optimization.
'' Responsibilities: Named domains/masks, declaration provenance and unknown calls.
'' This file intentionally does NOT contain: name guesses for externals or alias sets.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-effects.bi"
#include once "runtime/rtl.bi"

private const EFFECT_LIMIT = 1000000
private dim shared as longint declaration_count, call_count, operation_count

sub fbSemanticModelResetEffects( )
	declaration_count = 0
	call_count = 0
	operation_count = 0
end sub

private function hDomain( byval domain as integer ) as string
	select case domain
	case FB_RTL_DOMAIN_UNKNOWN: return "unknown"
	case FB_RTL_DOMAIN_MEMORY: return "memory"
	case FB_RTL_DOMAIN_MATH: return "math"
	case FB_RTL_DOMAIN_STRING: return "string"
	case FB_RTL_DOMAIN_UNICODE: return "unicode"
	case FB_RTL_DOMAIN_ARRAY: return "array"
	case FB_RTL_DOMAIN_FILE: return "file"
	case FB_RTL_DOMAIN_PRINT: return "print"
	case FB_RTL_DOMAIN_CONSOLE: return "console"
	case FB_RTL_DOMAIN_GRAPHICS: return "graphics"
	case FB_RTL_DOMAIN_SOUND: return "sound"
	case FB_RTL_DOMAIN_ERROR: return "error"
	case FB_RTL_DOMAIN_PROFILE: return "profile"
	case FB_RTL_DOMAIN_SYSTEM: return "system"
	case FB_RTL_DOMAIN_GOSUB: return "gosub"
	case FB_RTL_DOMAIN_DATA: return "data"
	case FB_RTL_DOMAIN_OBJECT: return "object"
	case FB_RTL_DOMAIN_SERIALIZATION: return "serialization"
	end select
	fbSemanticModelFailAt("runtime effect domain exceeds its vocabulary")
	return "unknown"
end function

private function hNames( byval mask as ulongint ) as string
	dim as string result
	for bit_index as integer = 0 to 14
		if( (mask and (1ull shl bit_index)) = 0 ) then continue for
		if( len(result) > 0 ) then result += ","
		select case bit_index
		case 0: result += "unknown"
		case 1: result += "allocate"
		case 2: result += "release"
		case 3: result += "argument-read"
		case 4: result += "argument-write"
		case 5: result += "runtime-read"
		case 6: result += "runtime-write"
		case 7: result += "external-io"
		case 8: result += "synchronization"
		case 9: result += "error-handling"
		case 10: result += "profiling"
		case 11: result += "process-exit"
		case 12: result += "code-execution"
		case 13: result += "computation"
		case 14: result += "control-transfer"
		end select
	next
	return result
end function

private function hContract( byval sym as FBSYMBOL ptr, byval indirect as integer ) as string
	dim as integer domain = FB_RTL_DOMAIN_UNKNOWN
	dim as ulongint mask = FB_RTL_EFFECT_UNKNOWN
	dim as uinteger native_options = 0
	dim as string origin = "unknown-callee"
	if( indirect ) then
		origin = "indirect-target"
	elseif( sym <> NULL ) then
		if( symbGetIsRTL(sym) and (symbGetIsFuncPtr(sym) = FALSE) ) then
			if( sym->proc.rtl.semantic_domain <> FB_RTL_DOMAIN_UNKNOWN ) then
				domain = sym->proc.rtl.semantic_domain
				mask = sym->proc.rtl.semantic_effects
				native_options = sym->proc.rtl.semantic_options
				origin = "runtime-registration"
			end if
		end if
	end if
	if( (mask = 0) or (mask > &h7FFFull) ) then fbSemanticModelFailAt("runtime effect mask exceeds its vocabulary")
	dim as string certainty = "known-potential"
	if( mask and FB_RTL_EFFECT_UNKNOWN ) then certainty = "unknown-remainder"
	'' Masks describe potential compiler-language effects, not runtime events.
	'' No contract identifies a concrete pointee or a complete memory alias set.
	return origin + TABCHAR + hDomain(domain) + TABCHAR + fbSemanticModelNumber(mask) + TABCHAR + hNames(mask) + TABCHAR + _
		certainty + TABCHAR + fbSemanticModelNumber(native_options) + TABCHAR + "unknown-aliases"
end function

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

sub fbSemanticModelEffectDeclaration( byval sym as FBSYMBOL ptr, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (sym = NULL) orElse (sym->class <> FB_SYMBCLASS_PROC) or (identity <= 0) or (declaration_count >= EFFECT_LIMIT) ) then
		fbSemanticModelFailAt("effect declaration has invalid native inputs")
		exit sub
	end if
	declaration_count += 1
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
		"effect-declaration:" + fbSemanticModelNumber(declaration_count) + TABCHAR + _
		fbSemanticModelEscape(hContract(sym, FALSE) + TABCHAR + fbSemanticModelNumber(abs((sym->stats and FB_SYMBSTATS_CANBECLONED) <> FALSE)) + _
		TABCHAR + fbSemanticModelNumber(abs(symbGetIsThrowable(sym) <> FALSE))))
end sub

sub fbSemanticModelEffectCall( byval sym as FBSYMBOL ptr, byval indirect as integer, _
	byval graph as longint, byval action as longint, byval load_id as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (sym = NULL) orElse (sym->class <> FB_SYMBCLASS_PROC) or (graph <= 0) or (action <= 0) or (call_count >= EFFECT_LIMIT) ) then
		fbSemanticModelFailAt("effect call has invalid native inputs")
		exit sub
	end if
	call_count += 1
	hProperty("effect-call:" + fbSemanticModelNumber(call_count), fbSemanticModelNumber(graph) + TABCHAR + fbSemanticModelNumber(action) + _
		TABCHAR + fbSemanticModelNumber(load_id) + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(sym)) + TABCHAR + _
		fbSemanticModelNumber(abs(indirect <> FALSE)) + TABCHAR + hContract(sym, indirect))
end sub

sub fbSemanticModelEffectOperation( byref kind as const string, byval opcode as integer, _
	byref role as const string, byval graph as longint, byval action as longint, byval load_id as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (graph <= 0) or (action <= 0) or (operation_count >= EFFECT_LIMIT) ) then
		fbSemanticModelFailAt("effect operation has invalid native inputs")
		exit sub
	end if
	operation_count += 1
	'' Assembly remains opaque. Other IR primitives are separate from runtime
	'' registration contracts; absent operand/alias knowledge stays unknown.
	dim as string effects = "unknown"
	if( kind = "assembly" ) then effects = "unknown-memory-registers-control"
	hProperty("effect-operation:" + fbSemanticModelNumber(operation_count), fbSemanticModelNumber(graph) + TABCHAR + _
		fbSemanticModelNumber(action) + TABCHAR + fbSemanticModelNumber(load_id) + TABCHAR + kind + TABCHAR + _
		fbSemanticModelNumber(opcode) + TABCHAR + role + TABCHAR + effects + TABCHAR + "unknown-aliases")
end sub

sub fbSemanticModelFinishEffects( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		for domain as integer = FB_RTL_DOMAIN_UNKNOWN to FB_RTL_DOMAIN_SERIALIZATION
			hProperty("effect-domain:" + fbSemanticModelNumber(domain), hDomain(domain))
		next
		for bit_index as integer = 0 to 14
			hProperty("effect-definition:" + fbSemanticModelNumber(bit_index), fbSemanticModelNumber(1ull shl bit_index) + TABCHAR + hNames(1ull shl bit_index))
		next
		hProperty("effect-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), fbSemanticModelNumber(declaration_count) + _
			TABCHAR + fbSemanticModelNumber(call_count) + TABCHAR + fbSemanticModelNumber(operation_count))
	end if
	fbSemanticModelResetEffects( )
end sub

'' end of tooling/semantic-effects.bas
