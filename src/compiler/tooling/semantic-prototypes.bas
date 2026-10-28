'' Project: FreeBASIC compiler - native prototype comparisons
'' File: tooling/semantic-prototypes.bas
'' Purpose: Retain actual declaration/prototype matching decisions.
'' Responsibilities: Candidate arity, reversed parameter comparisons and returned identities.
'' This file intentionally does NOT contain: overload scoring or prototype selection policy.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-prototypes.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "parser/parser.bi"

'' Prototype matching walks native overload and parameter lists serially.
'' Only scalar receipt state survives a callback; copied snapshots own names.
private const PROTOTYPE_DEPTH_LIMIT = 1024
private const PROTOTYPE_EVENT_LIMIT = 1000000
private const LOOKUP_TEXT_LIMIT = 65536
private type SEMANTIC_PROTOTYPE_FRAME
	identity as longint
	ordinal as longint
	remaining as integer
	stage as integer
end type
private dim shared as SEMANTIC_PROTOTYPE_FRAME prototype_stack(0 to PROTOTYPE_DEPTH_LIMIT - 1)
private dim shared as integer prototype_depth
private dim shared as longint prototype_events, prototype_begins, prototype_ends, prototype_candidates, prototype_parameters

private function hEnabled( ) as integer
	if( fbSemanticModelTypedExpressionsEnabled( ) = FALSE ) then return FALSE
	if( lex.ctx = NULL ) then return FALSE
	return (lex.ctx->semantic_probe = FALSE) and (fbGetIsPP( ) = FALSE)
end function

sub fbSemanticModelResetPrototypes( )
	prototype_depth = 0
	prototype_events = 0
	prototype_begins = 0
	prototype_ends = 0
	prototype_candidates = 0
	prototype_parameters = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendProvenance("K" + TABCHAR + "module" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

private sub hEvent( byref value as const string )
	if( prototype_events >= PROTOTYPE_EVENT_LIMIT ) then
		fbSemanticModelFailAt("native prototype event limit")
		exit sub
	end if
	prototype_events += 1
	hProperty("native-prototype:" + fbSemanticModelNumber(prototype_events), value)
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

private function hDeclaredParams( byval sym as FBSYMBOL ptr ) as integer
	if( sym = NULL ) then return INVALID
	if( sym->class <> FB_SYMBCLASS_PROC ) then return INVALID
	return sym->proc.params
end function

function fbSemanticModelPrototypeBegin( byval head as FBSYMBOL ptr, byval query as FBSYMBOL ptr, _
	byval options as integer ) as longint
	if( hEnabled( ) = FALSE ) then return 0
	if( prototype_depth >= PROTOTYPE_DEPTH_LIMIT ) then fbSemanticModelFailAt("native prototype depth limit"): return 0
	dim as longint parent
	if( prototype_depth ) then parent = prototype_stack(prototype_depth - 1).identity
	prototype_begins += 1
	dim as longint identity = prototype_begins
	with prototype_stack(prototype_depth)
		.identity = identity
		.ordinal = 0
		.remaining = 0
		.stage = 0
	end with
	prototype_depth += 1
	hEvent("begin" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(parent) + TABCHAR + _
		fbSemanticModelNumber(options) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + _
		hSymbol(head) + TABCHAR + hSymbol(query) + TABCHAR + fbSemanticModelNumber(hDeclaredParams(head)) + TABCHAR + _
		fbSemanticModelNumber(hDeclaredParams(query)))
	return identity
end function

function fbSemanticModelPrototypeCandidate( byval identity as longint, byval proc as FBSYMBOL ptr, _
	byval expected as integer, byval declared as integer, byval effective as integer, byval is_property as integer ) as longint
	if( identity = 0 or hEnabled( ) = FALSE ) then return 0
	if( prototype_depth = 0 ) then fbSemanticModelFailAt("native prototype candidate has no entry"): return 0
	dim as longint ordinal
	with prototype_stack(prototype_depth - 1)
		if( .identity <> identity or (.stage <> 0 and .stage <> 4) ) then
			fbSemanticModelFailAt("native prototype candidate has an unfinished predecessor")
			return 0
		end if
		.ordinal += 1
		ordinal = .ordinal
		.remaining = effective
		.stage = 4
		if( expected = effective and effective >= 0 ) then
			if( effective = 0 ) then .stage = 5 else .stage = 1
		end if
	end with
	prototype_candidates += 1
	hEvent("candidate" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(ordinal) + TABCHAR + _
		fbSemanticModelNumber(expected) + TABCHAR + fbSemanticModelNumber(declared) + TABCHAR + fbSemanticModelNumber(effective) + TABCHAR + _
		fbSemanticModelNumber(abs(is_property <> FALSE)) + TABCHAR + hSymbol(proc))
	return ordinal
end function

function fbSemanticModelPrototypeParameter( byval identity as longint, byval ordinal as longint, byval remaining as integer, _
	byref stage as const string, byval left_ as FBSYMBOL ptr, byval right_ as FBSYMBOL ptr, byval distinct as integer ) as integer
	if( identity = 0 or hEnabled( ) = FALSE ) then return distinct
	if( prototype_depth = 0 or left_ = NULL or right_ = NULL ) then
		fbSemanticModelFailAt("native prototype parameter has no input")
		return distinct
	end if
	if( left_->class <> FB_SYMBCLASS_PARAM or right_->class <> FB_SYMBCLASS_PARAM ) then
		fbSemanticModelFailAt("native prototype parameter has another symbol class")
		return distinct
	end if
	dim as integer wanted
	select case stage
	case "bydesc": wanted = 1
	case "type": wanted = 2
	case "subtype": wanted = 3
	case else: fbSemanticModelFailAt("invalid native prototype comparison stage"): return distinct
	end select
	with prototype_stack(prototype_depth - 1)
		if( .identity <> identity or .ordinal <> ordinal or .remaining <> remaining or .stage <> wanted ) then
			fbSemanticModelFailAt("native prototype comparison is out of order")
			return distinct
		end if
		if( distinct ) then
			.stage = 4
		elseif( wanted < 3 ) then
			.stage += 1
		else
			.remaining -= 1
			if( .remaining = 0 ) then .stage = 5 else .stage = 1
		end if
	end with
	dim as integer left_dims, right_dims
	if( left_->param.mode = FB_PARAMMODE_BYDESC ) then left_dims = left_->param.bydescdimensions
	if( right_->param.mode = FB_PARAMMODE_BYDESC ) then right_dims = right_->param.bydescdimensions
	prototype_parameters += 1
	hEvent("parameter" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(ordinal) + TABCHAR + _
		fbSemanticModelNumber(remaining) + TABCHAR + stage + TABCHAR + fbSemanticModelNumber(abs(distinct <> FALSE)) + TABCHAR + _
		fbSemanticModelNumber(left_->param.mode) + TABCHAR + fbSemanticModelNumber(left_dims) + TABCHAR + _
		fbSemanticModelNumber(right_->param.mode) + TABCHAR + fbSemanticModelNumber(right_dims) + TABCHAR + _
		hSymbol(left_) + TABCHAR + hSymbol(right_) + TABCHAR + hSymbol(left_->subtype) + TABCHAR + hSymbol(right_->subtype))
	return distinct
end function

function fbSemanticModelPrototypeEnd( byval identity as longint, byval ordinal as longint, _
	byval result as FBSYMBOL ptr, byref outcome as const string ) as FBSYMBOL ptr
	if( identity = 0 or hEnabled( ) = FALSE ) then return result
	if( prototype_depth = 0 ) then fbSemanticModelFailAt("native prototype end has no entry"): return result
	with prototype_stack(prototype_depth - 1)
		if( .identity <> identity ) then fbSemanticModelFailAt("native prototype end has another entry"): return result
		select case outcome
		case "selected"
			if( .stage <> 5 or .ordinal <> ordinal or result = NULL ) then
				fbSemanticModelFailAt("native prototype return has no matched candidate")
				return result
			end if
		case "not-found"
			if( .stage <> 4 or result <> NULL or ordinal ) then
				fbSemanticModelFailAt("native prototype absence has an unfinished comparison")
				return result
			end if
		case "invalid-input"
			if( .ordinal or ordinal or result <> NULL ) then fbSemanticModelFailAt("native prototype invalid input has candidates"): return result
		case else
			fbSemanticModelFailAt("invalid native prototype outcome")
			return result
		end select
	end with
	prototype_ends += 1
	hEvent("end" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(ordinal) + TABCHAR + outcome + TABCHAR + hSymbol(result))
	prototype_depth -= 1
	return result
end function

sub fbSemanticModelFinishPrototypes( byval commit as integer )
	if( commit and fbSemanticModelTypedExpressionsEnabled( ) ) then
		hProperty("native-prototype-policy", fbSemanticModelNumber(FB_PROCATTRIB_METHOD) + TABCHAR + _
			fbSemanticModelNumber(FB_PROCATTRIB_PROPERTY) + TABCHAR + fbSemanticModelNumber(FB_SYMBFINDOPT_PROPGET))
		hProperty("native-prototype-counts", fbSemanticModelNumber(prototype_events) + TABCHAR + fbSemanticModelNumber(prototype_begins) + TABCHAR + _
			fbSemanticModelNumber(prototype_ends) + TABCHAR + fbSemanticModelNumber(prototype_candidates) + TABCHAR + _
			fbSemanticModelNumber(prototype_parameters) + TABCHAR + fbSemanticModelNumber(prototype_depth))
	end if
	fbSemanticModelResetPrototypes( )
end sub

'' end of tooling/semantic-prototypes.bas
