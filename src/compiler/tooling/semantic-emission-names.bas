'' Project: FreeBASIC compiler - semantic emission identities
'' File: tooling/semantic-emission-names.bas
'' Purpose: Retain selected backend tokens and compiler-known object symbols.
'' Responsibilities: Copy names while symbols live and close per-module observations.
'' This file intentionally does NOT contain: calls to the mangler or external tools.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-emission-names.bi"
#include once "support/containers/hash.bi"

'' The serial backend may request the same name many times. Keys use immutable
'' semantic identities rather than symbol addresses, which the pool can reuse.
'' The string set owns its keys until module finish or transaction reset.
private const EMISSION_NAME_LIMIT = 1000000
private const EMISSION_NAME_BYTES = 65536
private const EMISSION_NAME_BUCKETS = 4096
private dim shared as TSTRSET emission_names
private dim shared as integer emission_names_ready
private dim shared as longint emission_name_count

sub fbSemanticModelResetEmissionNames( )
	if( emission_names_ready ) then strsetEnd(@emission_names)
	emission_names_ready = FALSE
	emission_name_count = 0
end sub

private function hNameKey( byval sym as FBSYMBOL ptr, byref stage as const string ) as string
	return fbSemanticModelNumber(fbSemanticModelSymbolId(sym)) + ":" + stage
end function

function fbSemanticModelObjectNameNeeded( byval sym as FBSYMBOL ptr ) as integer
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (sym = NULL) ) then return FALSE
	if( (sym->class <> FB_SYMBCLASS_VAR) and (sym->class <> FB_SYMBCLASS_PROC) ) then return FALSE
	if( sym->class = FB_SYMBCLASS_PROC ) then
		if( symbGetIsFuncPtr(sym) ) then return FALSE
	end if
	if( emission_names_ready = FALSE ) then return TRUE
	dim as string key = hNameKey(sym, "object")
	return hashLookup(@emission_names.hash, strptr(key)) = NULL
end function

sub fbSemanticModelEmissionName _
	( byval sym as FBSYMBOL ptr, byref stage as const string, byref selected_name as const string, _
	  byref state as const string, byref reason as const string )
	if( (fbSemanticModelFullEnabled( ) = FALSE) or (sym = NULL) ) then exit sub
	if( (sym->class <> FB_SYMBCLASS_VAR) and (sym->class <> FB_SYMBCLASS_PROC) ) then exit sub
	if( sym->class = FB_SYMBCLASS_PROC ) then
		if( symbGetIsFuncPtr(sym) ) then exit sub
	end if
	if( (stage <> "object") and (stage <> "c") and (stage <> "llvm") and _
	    (stage <> "gas") and (stage <> "gas64") ) then
		fbSemanticModelFailAt("unknown emission name stage")
		exit sub
	end if
	if( ((state = "selected") and ((len(selected_name) = 0) or (len(reason) <> 0))) or _
	    (((state = "unavailable") or (state = "none")) and ((len(selected_name) <> 0) or (len(reason) = 0))) or _
	    ((state <> "selected") and (state <> "unavailable") and (state <> "none")) or _
	    (len(selected_name) > EMISSION_NAME_BYTES) or (instr(selected_name, chr(0)) > 0) ) then
		fbSemanticModelFailAt("invalid emission name observation")
		exit sub
	end if
	if( emission_names_ready = FALSE ) then
		strsetInit(@emission_names, EMISSION_NAME_BUCKETS)
		emission_names_ready = TRUE
	end if
	dim as string key = hNameKey(sym, stage)
	if( hashLookup(@emission_names.hash, strptr(key)) <> NULL ) then exit sub
	if( emission_name_count >= EMISSION_NAME_LIMIT ) then
		fbSemanticModelFailAt("emission name observation limit exceeded")
		exit sub
	end if
	strsetAdd(@emission_names, key, 0)
	if( hashLookup(@emission_names.hash, strptr(key)) = NULL ) then
		fbSemanticModelFailAt("emission name observation allocation failed")
		exit sub
	end if
	emission_name_count += 1
	dim as string value = stage + TABCHAR + state + TABCHAR + fbSemanticModelEscape(selected_name) + TABCHAR + reason
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(sym)) + TABCHAR + _
		"emission-name:" + fbSemanticModelNumber(emission_name_count) + TABCHAR + fbSemanticModelEscape(value))
end sub

sub fbSemanticModelFinishEmissionNames( )
	if( fbSemanticModelFullEnabled( ) ) then
		fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
			fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
			"emission-name-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + _
			TABCHAR + fbSemanticModelNumber(emission_name_count))
	end if
	fbSemanticModelResetEmissionNames( )
end sub

'' end of tooling/semantic-emission-names.bas
