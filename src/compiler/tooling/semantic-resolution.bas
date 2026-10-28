'' Project: FreeBASIC compiler - native resolution explanations
'' File: tooling/semantic-resolution.bas
'' Purpose: Retain actual scoring attempts without repeating a language decision.
'' Responsibilities: Copied snapshots, nested resolver order and bounded module closure.
'' This file intentionally does NOT contain: lookup, candidate selection or executed calls.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-resolution.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "parser/parser.bi"
#include once "diagnostics/error.bi"

'' -------------------------------------------------------------------------
'' Module-owned observation state
'' -------------------------------------------------------------------------

'' The compiler resolves serially. Nested conversion searches use this scalar
'' stack; no symbol or AST pointer is kept after its immediate callback.
private const RESOLUTION_DEPTH_LIMIT = 1024
private const RESOLUTION_EVENT_LIMIT = 1000000
private const RESOLUTION_NAME_BYTES = 65536
private type SEMANTIC_RESOLUTION_FRAME
	identity as longint
	candidate as longint
	candidates as longint
	pending_rank as longint
	chosen as longint
	scored as longint
	confirmation as integer
end type
private dim shared as SEMANTIC_RESOLUTION_FRAME resolution_stack(0 to RESOLUTION_DEPTH_LIMIT - 1)
private dim shared as integer resolution_depth
private dim shared as longint event_count, scope_count, scope_end_count
private dim shared as longint candidate_count, score_count, argument_count, rank_count, access_count
private dim shared as longint context_count, use_count, bound_use_count, last_resolution

private function hEnabled( ) as integer
	if( fbSemanticModelTypedExpressionsEnabled( ) = FALSE ) then return FALSE
	if( lex.ctx = NULL ) then return FALSE
	return (lex.ctx->semantic_probe = FALSE) and (fbGetIsPP( ) = FALSE)
end function

sub fbSemanticModelResetResolution( )
	resolution_depth = 0
	event_count = 0
	scope_count = 0
	scope_end_count = 0
	candidate_count = 0
	score_count = 0
	argument_count = 0
	rank_count = 0
	access_count = 0
	context_count = 0
	use_count = 0
	bound_use_count = 0
	last_resolution = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendProvenance("K" + TABCHAR + "module" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

private sub hEvent( byref value as const string )
	if( event_count >= RESOLUTION_EVENT_LIMIT ) then
		fbSemanticModelFailAt("native resolution observation limit")
		exit sub
	end if
	event_count += 1
	hProperty("native-resolution:" + fbSemanticModelNumber(event_count), value)
end sub

private function hName( byval sym as FBSYMBOL ptr ) as string
	if( sym = NULL ) then return ""
	if( sym->id.name = NULL ) then return ""
	dim as string value = *sym->id.name
	if( len(value) > RESOLUTION_NAME_BYTES ) then
		fbSemanticModelFailAt("native resolution name limit")
		return ""
	end if
	return fbSemanticModelEscape(value)
end function

private function hSnapshot( byval sym as FBSYMBOL ptr ) as string
	dim as integer symbol_class = 0, params = -1
	dim as longint dtype = FB_DATATYPE_INVALID, attrib, pattrib
	dim as FBSYMBOL ptr owner = NULL
	if( sym <> NULL ) then
		symbol_class = sym->class
		dtype = culng(sym->typ)
		attrib = sym->attrib
		pattrib = sym->pattrib
		if( sym->class = FB_SYMBCLASS_PROC ) then params = sym->proc.params
		if( sym->symtb <> NULL ) then owner = sym->symtb->owner
	end if
	'' Name/owner hints are copied, not synthetic symbol identities. In a
	'' rejected module the public S/T graph may never have been published.
	return fbSemanticModelNumber(abs(sym <> NULL)) + TABCHAR + fbSemanticModelNumber(symbol_class) + TABCHAR + _
		fbSemanticModelTypeNumber(dtype) + TABCHAR + fbSemanticModelNumber(attrib) + TABCHAR + _
		fbSemanticModelNumber(pattrib) + TABCHAR + fbSemanticModelNumber(params) + TABCHAR + hName(sym) + TABCHAR + hName(owner)
end function

private sub hContext( byval event_id as longint, byval subject as FBSYMBOL ptr, _
	byval left_type as FBSYMBOL ptr, byval right_type as FBSYMBOL ptr, byval arg as ASTNODE ptr = NULL )
	if( hEnabled( ) = FALSE ) then exit sub
	if( (subject = cast(FBSYMBOL ptr, INVALID)) or (left_type = cast(FBSYMBOL ptr, INVALID)) or _
		(right_type = cast(FBSYMBOL ptr, INVALID)) ) then
		fbSemanticModelFailAt("invalid native resolution type context")
		exit sub
	end if
	dim as longint subject_id, left_id, right_id, expression_id
	if( fbSemanticModelFullEnabled( ) ) then
		subject_id = fbSemanticModelSymbolId(subject)
		left_id = fbSemanticModelSymbolId(left_type)
		right_id = fbSemanticModelSymbolId(right_type)
	end if
	'' Use an existing immutable parser association. A synthesized or erased
	'' argument without an E identity remains explicit zero; never export a new
	'' expression merely to make this relation look complete.
	if( arg <> NULL ) then expression_id = arg->semantic_expression
	context_count += 1
	hProperty("native-resolution-context:" + fbSemanticModelNumber(event_id), _
		fbSemanticModelNumber(subject_id) + TABCHAR + fbSemanticModelNumber(left_id) + TABCHAR + _
		fbSemanticModelNumber(right_id) + TABCHAR + fbSemanticModelNumber(expression_id) + TABCHAR + _
		hSnapshot(left_type) + TABCHAR + hSnapshot(right_type))
end sub

private function hPoint( ) as string
	dim as LEX_LOCATION source = lexGetLastLocation( )
	dim as integer physical = source.is_physical and source.raw_valid and _
		(source.raw_start_line > 0) and (source.raw_start_column >= 0)
	return fbSemanticModelEscape(source.source_file) + TABCHAR + fbSemanticModelNumber(source.source_context) + TABCHAR + _
		fbSemanticModelNumber(source.start_line) + TABCHAR + fbSemanticModelNumber(source.start_column) + TABCHAR + _
		fbSemanticModelNumber(iif(physical, source.raw_start_line, 0)) + TABCHAR + _
		fbSemanticModelNumber(iif(physical, source.raw_start_column, 0)) + TABCHAR + fbSemanticModelNumber(abs(physical))
end function

private function hCurrent( byval scope_id as longint ) as integer
	if( resolution_depth > 0 ) then
		if( resolution_stack(resolution_depth - 1).identity = scope_id ) then return TRUE
	end if
	fbSemanticModelFailAt("native resolution scope mismatch")
	return FALSE
end function

'' -------------------------------------------------------------------------
'' Native resolver and candidate lifetimes
'' -------------------------------------------------------------------------

function fbSemanticModelResolutionBegin( byref kind as const string, _
	byval head as FBSYMBOL ptr, byval args as integer, byval options as integer, _
	byval dtype as integer, byval subtype as FBSYMBOL ptr ) as longint
	if( hEnabled( ) = FALSE ) then return 0
	if( (resolution_depth >= RESOLUTION_DEPTH_LIMIT) or (args < 0) or _
		((kind <> "closest") and (kind <> "cast") and (kind <> "cast-precision")) ) then
		fbSemanticModelFailAt("invalid native resolution entry")
		return 0
	end if
	if( head <> NULL andalso head->class <> FB_SYMBCLASS_PROC ) then
		fbSemanticModelFailAt("native resolution head is not a procedure")
		return 0
	end if
	dim as longint parent, parent_candidate
	if( resolution_depth > 0 ) then
		parent = resolution_stack(resolution_depth - 1).identity
		parent_candidate = resolution_stack(resolution_depth - 1).candidate
	end if
	scope_count += 1
	with resolution_stack(resolution_depth)
		.identity = scope_count
		.candidate = 0
		.candidates = 0
		.pending_rank = 0
		.chosen = 0
		.scored = 0
		.confirmation = FALSE
	end with
	resolution_depth += 1
	hEvent("begin" + TABCHAR + fbSemanticModelNumber(scope_count) + TABCHAR + fbSemanticModelNumber(parent) + TABCHAR + _
		fbSemanticModelNumber(parent_candidate) + TABCHAR + kind + TABCHAR + fbSemanticModelNumber(args) + TABCHAR + _
		fbSemanticModelNumber(options) + TABCHAR + fbSemanticModelTypeNumber(culng(dtype)) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + hPoint( ) + TABCHAR + hSnapshot(head))
	dim as FBSYMBOL ptr result_type = NULL
	if( head <> NULL ) then result_type = head->subtype
	hContext(event_count, head, result_type, subtype)
	return scope_count
end function

function fbSemanticModelResolutionCandidate( byval scope_id as longint, _
	byval proc as FBSYMBOL ptr, byref pass as const string ) as longint
	if( scope_id = 0 or hEnabled( ) = FALSE ) then return 0
	if( hCurrent(scope_id) = FALSE ) then return 0
	if( (proc = NULL) orElse (proc->class <> FB_SYMBCLASS_PROC) ) then
		fbSemanticModelFailAt("native resolution candidate is not a procedure")
		return 0
	end if
	if( (pass <> "screen") and (pass <> "confirm") and (pass <> "cast") and (pass <> "precision") ) then
		fbSemanticModelFailAt("unknown native resolution pass")
		return 0
	end if
	with resolution_stack(resolution_depth - 1)
		if( (.candidate <> 0) or (.pending_rank <> 0) ) then
			fbSemanticModelFailAt("overlapping native resolution candidates")
			return 0
		end if
		candidate_count += 1
		.candidate = candidate_count
		.confirmation = (pass = "confirm")
		.candidates += 1
		hEvent("candidate" + TABCHAR + fbSemanticModelNumber(scope_id) + TABCHAR + fbSemanticModelNumber(candidate_count) + TABCHAR + _
			fbSemanticModelNumber(.candidates) + TABCHAR + pass + TABCHAR + hSnapshot(proc))
	end with
	hContext(event_count, proc, proc->subtype, NULL)
	return candidate_count
end function

'' -------------------------------------------------------------------------
'' Observed parameter checks and their native scores
'' -------------------------------------------------------------------------

function fbSemanticModelResolutionArgument( byval candidate as longint, _
	byval ordinal as integer, byval param as FBSYMBOL ptr, byval arg as ASTNODE ptr, _
	byval mode as integer, byval score as integer, byref reason as const string ) as integer
	if( candidate = 0 or hEnabled( ) = FALSE ) then return score
	if( resolution_depth = 0 ) then fbSemanticModelFailAt("resolution argument has no scope"): return score
	with resolution_stack(resolution_depth - 1)
		if( (.candidate <> candidate) or (ordinal < 1) or (param = NULL) ) then
			fbSemanticModelFailAt("invalid native resolution argument")
			return score
		end if
		if( param->class <> FB_SYMBCLASS_PARAM ) then
			fbSemanticModelFailAt("native resolution parameter has another class")
			return score
		end if
		select case reason
		case "optional-default", "required-omission", "descriptor-mode", "descriptor-type", "descriptor-rank", _
			"descriptor-match", "byref-descriptor", "byref-any", "byref-byval-width", "invalid-mode", _
			"argument-descriptor", "same-type", "compatible-const", "incompatible-const", "ctor-conversion", _
			"cast-conversion", "udt-no-conversion", "enum-mismatch", "type-difference", "trailing-optional", "trailing-required"
		case else
			fbSemanticModelFailAt("unknown native resolution argument outcome")
			return score
		end select
		dim as integer trailing = (reason = "trailing-optional") or (reason = "trailing-required")
		if( (score < -1) or ((score = -1) <> (trailing <> FALSE)) or _
			((mode <> INVALID) and ((mode < FB_PARAMMODE_BYVAL) or (mode > FB_PARAMMODE_VARARG))) or _
			(param->param.mode < FB_PARAMMODE_BYVAL) or (param->param.mode > FB_PARAMMODE_VARARG) or _
			(trailing and ((arg <> NULL) or (mode <> INVALID))) ) then
			fbSemanticModelFailAt("invalid native resolution argument score or mode")
			return score
		end if
		dim as integer raw_class = 0, dimensions = 0
		dim as longint dtype = FB_DATATYPE_INVALID
		dim as FBSYMBOL ptr subtype = NULL
		if( arg <> NULL ) then
			raw_class = arg->class
			dtype = culng(arg->dtype)
			subtype = arg->subtype
			if( (raw_class < 0) or (raw_class >= AST_CLASSES) ) then
				fbSemanticModelFailAt("native resolution argument has another AST class")
				return score
			end if
		end if
		if( param->param.mode = FB_PARAMMODE_BYDESC ) then dimensions = param->param.bydescdimensions
		argument_count += 1
		hEvent("argument" + TABCHAR + fbSemanticModelNumber(.identity) + TABCHAR + fbSemanticModelNumber(candidate) + TABCHAR + _
			fbSemanticModelNumber(ordinal) + TABCHAR + fbSemanticModelNumber(score) + TABCHAR + reason + TABCHAR + _
			fbSemanticModelNumber(mode) + TABCHAR + fbSemanticModelNumber(param->param.mode) + TABCHAR + _
			fbSemanticModelNumber(abs(param->param.optexpr <> NULL)) + TABCHAR + fbSemanticModelNumber(dimensions) + TABCHAR + _
			fbSemanticModelTypeNumber(culng(param->typ)) + TABCHAR + hName(param->subtype) + TABCHAR + _
			fbSemanticModelNumber(abs(arg <> NULL)) + TABCHAR + fbSemanticModelNumber(raw_class) + TABCHAR + _
			fbSemanticModelTypeNumber(dtype) + TABCHAR + hName(subtype) + TABCHAR + hName(param))
		hContext(event_count, param, param->subtype, subtype, arg)
	end with
	return score
end function

sub fbSemanticModelResolutionScore( byval candidate as longint, byval score as integer, _
	byval matches as integer, byval exact_known as integer, byval exact as integer, _
	byval params as integer, byref reason as const string, byval error_code as integer )
	if( candidate = 0 or hEnabled( ) = FALSE ) then exit sub
	if( resolution_depth = 0 ) then fbSemanticModelFailAt("resolution score has no scope"): exit sub
	with resolution_stack(resolution_depth - 1)
		if( .candidate <> candidate ) then fbSemanticModelFailAt("native resolution score mismatch"): exit sub
		if( (score < -1) or (params < -1) or (error_code < 0) or _
			(exact_known and (exact < 0)) or ((exact_known = FALSE) and (exact <> -1)) ) then
			fbSemanticModelFailAt("invalid native resolution score fields")
			exit sub
		end if
		score_count += 1
		hEvent("score" + TABCHAR + fbSemanticModelNumber(.identity) + TABCHAR + fbSemanticModelNumber(candidate) + TABCHAR + _
			fbSemanticModelNumber(score) + TABCHAR + fbSemanticModelNumber(abs(matches <> FALSE)) + TABCHAR + _
			fbSemanticModelNumber(abs(exact_known <> FALSE)) + TABCHAR + fbSemanticModelNumber(iif(exact_known, exact, -1)) + TABCHAR + _
			fbSemanticModelNumber(params) + TABCHAR + reason + TABCHAR + fbSemanticModelNumber(error_code))
		.candidate = 0
		.scored = candidate
		.pending_rank = iif(.confirmation, 0, candidate)
	end with
end sub

function fbSemanticModelResolutionCastScore( byval candidate as longint, byval score as integer, _
	byval params as integer, byref reason as const string ) as integer
	fbSemanticModelResolutionScore(candidate, score, score > 0, FALSE, -1, params, reason, 0)
	return score
end function

sub fbSemanticModelResolutionRank( byval scope_id as longint, byval candidate as longint, _
	byref action as const string, byval score as integer, byval ties as integer, byval chosen as longint )
	if( scope_id = 0 or hEnabled( ) = FALSE ) then exit sub
	if( hCurrent(scope_id) = FALSE ) then exit sub
	with resolution_stack(resolution_depth - 1)
		if( (.pending_rank <> candidate) or (.candidate <> 0) or (score < 0) or (ties < 0) or _
			(chosen < 0) or (chosen > candidate_count) ) then
			fbSemanticModelFailAt("invalid native resolution ranking fields")
			exit sub
		end if
		select case action
		case "better", "precision-better"
			if( chosen <> candidate ) then fbSemanticModelFailAt("native resolution winner mismatch"): exit sub
		case "rejected", "ineligible", "tie", "inferior", "precision-skip"
			if( chosen <> .chosen ) then fbSemanticModelFailAt("native resolution changed an unselected winner"): exit sub
		case else
			fbSemanticModelFailAt("unknown native resolution ranking outcome")
			exit sub
		end select
		.chosen = chosen
		.pending_rank = 0
	end with
	rank_count += 1
	hEvent("rank" + TABCHAR + fbSemanticModelNumber(scope_id) + TABCHAR + fbSemanticModelNumber(candidate) + TABCHAR + action + TABCHAR + _
		fbSemanticModelNumber(score) + TABCHAR + fbSemanticModelNumber(ties) + TABCHAR + fbSemanticModelNumber(chosen))
end sub

function fbSemanticModelResolutionEnd( byval scope_id as longint, byval chosen as longint, _
	byval confirmation as longint, byval score as integer, byval ties as integer, _
	byval error_code as integer, byval result as FBSYMBOL ptr ) as FBSYMBOL ptr
	if( scope_id = 0 or hEnabled( ) = FALSE ) then return result
	if( hCurrent(scope_id) = FALSE ) then return result
	if( (resolution_stack(resolution_depth - 1).candidate <> 0) or _
		(resolution_stack(resolution_depth - 1).pending_rank <> 0) ) then
		fbSemanticModelFailAt("native resolution ended with an open candidate")
		return result
	end if
	with resolution_stack(resolution_depth - 1)
		if( (score < 0) or (ties < 0) or (error_code < 0) or (chosen < 0) or (confirmation < 0) or _
			(confirmation > candidate_count) or ((result <> NULL) and ((chosen = 0) or (chosen <> .chosen))) or _
			((result = NULL) and (chosen <> 0)) ) then
			fbSemanticModelFailAt("invalid native resolution return fields")
			return result
		end if
		if( (confirmation <> 0) and ((confirmation <> .scored) or (.confirmation = FALSE)) ) then
			fbSemanticModelFailAt("native resolution confirmation mismatch")
			return result
		end if
	end with
	scope_end_count += 1
	hEvent("end" + TABCHAR + fbSemanticModelNumber(scope_id) + TABCHAR + fbSemanticModelNumber(chosen) + TABCHAR + _
		fbSemanticModelNumber(confirmation) + TABCHAR + fbSemanticModelNumber(score) + TABCHAR + _
		fbSemanticModelNumber(ties) + TABCHAR + fbSemanticModelNumber(error_code) + TABCHAR + fbSemanticModelNumber(abs(result <> NULL)))
	dim as FBSYMBOL ptr result_type = NULL
	if( result <> NULL ) then result_type = result->subtype
	hContext(event_count, result, result_type, NULL)
	last_resolution = scope_id
	resolution_depth -= 1
	return result
end function

'' -------------------------------------------------------------------------
'' Native access checks and terminal coverage
'' -------------------------------------------------------------------------

function fbSemanticModelResolutionAccess( byval sym as FBSYMBOL ptr, _
	byval allowed as integer, byref route as const string ) as integer
	if( hEnabled( ) = FALSE ) then return allowed
	if( sym = NULL ) then fbSemanticModelFailAt("native access check has no symbol"): return allowed
	if( (route <> "member") and (route <> "type-chain") ) then
		fbSemanticModelFailAt("unknown native access check route")
		return allowed
	end if
	dim as longint scope_id, candidate
	if( resolution_depth > 0 ) then
		scope_id = resolution_stack(resolution_depth - 1).identity
		candidate = resolution_stack(resolution_depth - 1).candidate
	end if
	access_count += 1
	hEvent("access" + TABCHAR + fbSemanticModelNumber(scope_id) + TABCHAR + fbSemanticModelNumber(candidate) + TABCHAR + route + TABCHAR + _
		fbSemanticModelNumber(abs(allowed <> FALSE)) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + hPoint( ) + TABCHAR + hSnapshot(sym))
	hContext(event_count, sym, sym->subtype, NULL)
	return allowed
end function

function fbSemanticModelLastResolution( ) as longint
	if( hEnabled( ) = FALSE ) then return 0
	return last_resolution
end function

sub fbSemanticModelResolutionUse( byval scope_id as longint, byval source as LEX_LOCATION ptr )
	if( scope_id = 0 or hEnabled( ) = FALSE ) then exit sub
	if( (scope_id <> last_resolution) or (scope_id > scope_count) ) then
		fbSemanticModelFailAt("native overload source use has no completed search")
		exit sub
	end if
	dim as LEX_LOCATION site
	if( source <> NULL ) then site = *source
	dim as integer physical = site.is_physical and site.raw_valid and _
		(site.raw_start_line > 0) and (site.raw_start_column >= 0)
	use_count += 1
	hProperty("native-resolution-use:" + fbSemanticModelNumber(scope_id), _
		fbSemanticModelNumber(abs(source <> NULL)) + TABCHAR + fbSemanticModelEscape(site.source_file) + TABCHAR + _
		fbSemanticModelNumber(site.source_context) + TABCHAR + fbSemanticModelNumber(site.start_line) + TABCHAR + _
		fbSemanticModelNumber(site.start_column) + TABCHAR + fbSemanticModelNumber(site.end_line) + TABCHAR + _
		fbSemanticModelNumber(site.end_column) + TABCHAR + fbSemanticModelNumber(abs(physical)) + TABCHAR + _
		fbSemanticModelNumber(iif(physical, site.raw_start_line, 0)) + TABCHAR + _
		fbSemanticModelNumber(iif(physical, site.raw_start_column, 0)) + TABCHAR + _
		fbSemanticModelNumber(iif(physical, site.raw_end_line, 0)) + TABCHAR + _
		fbSemanticModelNumber(iif(physical, site.raw_end_column, 0)))
end sub

sub fbSemanticModelResolutionBoundUse( byval scope_id as longint, byval binding as longint )
	if( scope_id = 0 or binding = 0 or hEnabled( ) = FALSE ) then exit sub
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (scope_id < 1) or (scope_id > scope_count) ) then
		fbSemanticModelFailAt("invalid native overload callee binding")
		exit sub
	end if
	bound_use_count += 1
	if( binding < 0 ) then
		if( (binding = &h8000000000000000ll) or (fbSemanticModelMacroProvenanceEnabled( ) = FALSE) ) then
			fbSemanticModelFailAt("invalid native overload macro reference")
			exit sub
		end if
		hProperty("native-resolution-macro-use:" + fbSemanticModelNumber(scope_id), fbSemanticModelNumber(-binding))
	else
		hProperty("native-resolution-bound-use:" + fbSemanticModelNumber(scope_id), fbSemanticModelNumber(binding))
	end if
end sub

sub fbSemanticModelFinishResolution( byval commit as integer )
	if( commit and fbSemanticModelTypedExpressionsEnabled( ) ) then
		'' These are native ranking constants, independent of the raw type ABI.
		'' Reduced expression models do not publish the primitive Y table.
		hProperty("native-resolution-policy", fbSemanticModelNumber(FB_OVLPROC_MINORSCALE) + TABCHAR + _
			fbSemanticModelNumber(FB_OVLPROC_TYPEMATCH) + TABCHAR + fbSemanticModelNumber(FB_OVLPROC_FULLMATCH) + TABCHAR + _
			fbSemanticModelNumber(RESOLUTION_DEPTH_LIMIT) + TABCHAR + fbSemanticModelNumber(RESOLUTION_EVENT_LIMIT))
		hProperty("native-resolution-counts", fbSemanticModelNumber(event_count) + TABCHAR + fbSemanticModelNumber(scope_count) + TABCHAR + _
			fbSemanticModelNumber(scope_end_count) + TABCHAR + fbSemanticModelNumber(candidate_count) + TABCHAR + _
			fbSemanticModelNumber(score_count) + TABCHAR + fbSemanticModelNumber(argument_count) + TABCHAR + _
			fbSemanticModelNumber(rank_count) + TABCHAR + fbSemanticModelNumber(access_count) + TABCHAR + fbSemanticModelNumber(resolution_depth))
		hProperty("native-resolution-context-counts", fbSemanticModelNumber(context_count) + TABCHAR + _
			fbSemanticModelNumber(use_count) + TABCHAR + fbSemanticModelNumber(bound_use_count))
	end if
	fbSemanticModelResetResolution( )
end sub

'' end of tooling/semantic-resolution.bas
