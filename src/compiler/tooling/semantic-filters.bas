'' Project: FreeBASIC compiler - native symbol filters
'' File: tooling/semantic-filters.bas
'' Purpose: Copy actual class, suffix and default-type filtering decisions.
'' Responsibilities: Candidate order, native access results and selected symbols.
'' This file intentionally does NOT contain: name lookup or replacement selection policy.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-filters.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "parser/parser.bi"

'' The native filter consumes chain-pool and same-name hash links serially.
'' Frames keep scalar receipts only; no symbol or chain pointer survives a call.
private const FILTER_DEPTH_LIMIT = 1024
private const FILTER_EVENT_LIMIT = 1000000
private const LOOKUP_TEXT_LIMIT = 65536
private type SEMANTIC_FILTER_FRAME
	identity as longint
	ordinal as longint
	matched as integer
	variable as integer
	access_result as integer
end type
private dim shared as SEMANTIC_FILTER_FRAME filter_stack(0 to FILTER_DEPTH_LIMIT - 1)
private dim shared as integer filter_depth
private dim shared as longint filter_events, filter_begins, filter_ends, filter_candidates, filter_accesses

private function hEnabled( ) as integer
	if( fbSemanticModelTypedExpressionsEnabled( ) = FALSE ) then return FALSE
	if( lex.ctx = NULL ) then return FALSE
	return (lex.ctx->semantic_probe = FALSE) and (fbGetIsPP( ) = FALSE)
end function

sub fbSemanticModelResetFilters( )
	filter_depth = 0
	filter_events = 0
	filter_begins = 0
	filter_ends = 0
	filter_candidates = 0
	filter_accesses = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendProvenance("K" + TABCHAR + "module" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

private sub hEvent( byref value as const string )
	if( filter_events >= FILTER_EVENT_LIMIT ) then
		fbSemanticModelFailAt("native symbol filter event limit")
		exit sub
	end if
	filter_events += 1
	hProperty("native-symbol-filter:" + fbSemanticModelNumber(filter_events), value)
end sub

private function hName( byval sym as FBSYMBOL ptr ) as string
	if( sym = NULL ) then return ""
	if( sym->id.name = NULL ) then return ""
	dim as string value = *sym->id.name
	if( len(value) > LOOKUP_TEXT_LIMIT ) then fbSemanticModelFailAt("native lookup name limit"): return ""
	return fbSemanticModelEscape(value)
end function

private function hSymbol( byval sym as FBSYMBOL ptr ) as string
	dim as integer symbol_class = 0
	dim as longint dtype = FB_DATATYPE_INVALID, attrib, pattrib, identity
	dim as FBSYMBOL ptr owner = NULL
	if( sym <> NULL ) then
		symbol_class = sym->class
		dtype = culng(sym->typ)
		attrib = sym->attrib
		pattrib = sym->pattrib
		if( sym->symtb <> NULL ) then owner = sym->symtb->owner
		if( fbSemanticModelFullEnabled( ) ) then identity = fbSemanticModelSymbolId(sym)
	end if
	return fbSemanticModelNumber(abs(sym <> NULL)) + TABCHAR + fbSemanticModelNumber(symbol_class) + TABCHAR + _
		fbSemanticModelTypeNumber(dtype) + TABCHAR + fbSemanticModelNumber(attrib) + TABCHAR + _
		fbSemanticModelNumber(pattrib) + TABCHAR + hName(sym) + TABCHAR + hName(owner) + TABCHAR + fbSemanticModelNumber(identity)
end function

function fbSemanticModelFilterBegin( byref kind as const string, byval class_ as integer, _
	byval dtype as integer, byval chain_ as FBSYMCHAIN ptr ) as longint
	if( hEnabled( ) = FALSE ) then return 0
	if( filter_depth >= FILTER_DEPTH_LIMIT ) then fbSemanticModelFailAt("native symbol filter depth limit"): return 0
	select case kind
	case "class", "class-and-type", "suffix", "default-type"
	case else: fbSemanticModelFailAt("invalid native symbol filter kind"): return 0
	end select
	dim as longint parent
	if( filter_depth ) then parent = filter_stack(filter_depth - 1).identity
	filter_begins += 1
	with filter_stack(filter_depth)
		.identity = filter_begins
		.ordinal = 0
		.matched = FALSE
		.variable = FALSE
		.access_result = INVALID
	end with
	filter_depth += 1
	dim as FBSYMBOL ptr head = NULL
	if( chain_ <> NULL ) then head = chain_->sym
	hEvent("begin" + TABCHAR + fbSemanticModelNumber(filter_begins) + TABCHAR + fbSemanticModelNumber(parent) + TABCHAR + _
		kind + TABCHAR + fbSemanticModelNumber(class_) + TABCHAR + fbSemanticModelTypeNumber(culng(dtype)) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(abs(chain_ <> NULL)) + TABCHAR + hSymbol(head))
	return filter_begins
end function

function fbSemanticModelFilterCandidate( byval sym as FBSYMBOL ptr, byval matches as integer, _
	byref reason as const string ) as integer
	if( hEnabled( ) = FALSE or filter_depth = 0 ) then return matches
	if( sym = NULL ) then fbSemanticModelFailAt("native symbol filter has no candidate"): return matches
	filter_candidates += 1
	with filter_stack(filter_depth - 1)
		.ordinal += 1
		.matched = (matches <> FALSE)
		.variable = (sym->class = FB_SYMBCLASS_VAR)
		.access_result = INVALID
		hEvent("candidate" + TABCHAR + fbSemanticModelNumber(.identity) + TABCHAR + fbSemanticModelNumber(.ordinal) + TABCHAR + _
			fbSemanticModelNumber(abs(matches <> FALSE)) + TABCHAR + reason + TABCHAR + hSymbol(sym))
	end with
	return matches
end function

function fbSemanticModelFilterAccess( byval allowed as integer ) as integer
	if( hEnabled( ) = FALSE or filter_depth = 0 ) then return allowed
	with filter_stack(filter_depth - 1)
		if( .matched = FALSE or .variable = FALSE or .access_result <> INVALID ) then
			fbSemanticModelFailAt("native symbol filter access has no pending variable")
			return allowed
		end if
		.access_result = abs(allowed <> FALSE)
		filter_accesses += 1
		hEvent("access" + TABCHAR + fbSemanticModelNumber(.identity) + TABCHAR + fbSemanticModelNumber(.ordinal) + TABCHAR + _
			fbSemanticModelNumber(.access_result))
	end with
	return allowed
end function

function fbSemanticModelFilterEnd( byval identity as longint, byval result as FBSYMBOL ptr, _
	byref outcome as const string ) as FBSYMBOL ptr
	if( identity = 0 or hEnabled( ) = FALSE ) then return result
	if( filter_depth = 0 ) then fbSemanticModelFailAt("native symbol filter end has no entry"): return result
	dim as longint selected
	with filter_stack(filter_depth - 1)
		if( .identity <> identity ) then fbSemanticModelFailAt("native symbol filter end has another entry"): return result
		select case outcome
		case "selected"
			if( result = NULL or .matched = FALSE or (.variable and .access_result <> 1) ) then
				fbSemanticModelFailAt("native symbol filter selection has no eligible candidate")
				return result
			end if
			selected = .ordinal
		case "denied"
			if( result <> NULL or .matched = FALSE or .access_result <> 0 ) then
				fbSemanticModelFailAt("native symbol filter denial has no access result")
				return result
			end if
		case "not-found"
			if( result <> NULL or .matched ) then fbSemanticModelFailAt("native symbol filter absence contradicts a candidate"): return result
		case else
			fbSemanticModelFailAt("invalid native symbol filter outcome")
			return result
		end select
	end with
	filter_ends += 1
	hEvent("end" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(selected) + TABCHAR + outcome + TABCHAR + hSymbol(result))
	filter_depth -= 1
	return result
end function

sub fbSemanticModelFinishFilters( byval commit as integer )
	if( commit and fbSemanticModelTypedExpressionsEnabled( ) ) then
		'' These are native predicate inputs, not a second type policy in the
		'' consumer. Datatype values follow the selected sidecar type ABI.
		hProperty("native-symbol-filter-policy", fbSemanticModelNumber(FB_DT_TYPEMASK or FB_DT_PTRMASK) + TABCHAR + _
			fbSemanticModelNumber(FB_SYMBATTRIB_SUFFIXED) + TABCHAR + fbSemanticModelTypeNumber(FB_DATATYPE_STRING) + TABCHAR + _
			fbSemanticModelTypeNumber(FB_DATATYPE_USTRING) + TABCHAR + fbSemanticModelTypeNumber(FB_DATATYPE_FIXSTR) + TABCHAR + _
			fbSemanticModelTypeNumber(FB_DATATYPE_CHAR))
		hProperty("native-symbol-filter-counts", fbSemanticModelNumber(filter_events) + TABCHAR + fbSemanticModelNumber(filter_begins) + TABCHAR + _
			fbSemanticModelNumber(filter_ends) + TABCHAR + fbSemanticModelNumber(filter_candidates) + TABCHAR + _
			fbSemanticModelNumber(filter_accesses) + TABCHAR + fbSemanticModelNumber(filter_depth))
	end if
	fbSemanticModelResetFilters( )
end sub

'' end of tooling/semantic-filters.bas
