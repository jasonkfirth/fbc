'' Project: FreeBASIC compiler - semantic macro provenance
'' File: tooling/semantic-macros.bas
'' Purpose: Preserve the compiler's selected definitions and actual expansions.
'' Responsibilities: Serialize identities, arguments, pieces, results, and origins.
'' This file intentionally does NOT contain: name lookup or callback execution.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-preprocessor.bi"
#include once "tooling/semantic-macros.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "preprocessor/pp.bi"

'' -------------------------------------------------------------------------
'' Module-local definition snapshots and active loaders
'' -------------------------------------------------------------------------

type SEMANTIC_MACRO_SLOT
	allocation as ulongint
	definition as longint
end type

const SEMANTIC_MACRO_MAX_DEFINITIONS = 1048576
dim shared as SEMANTIC_MACRO_SLOT ptr semantic_macro_slots
dim shared as integer semantic_macro_capacity, semantic_macro_count
dim shared as longint semantic_macro_loader
dim shared as string semantic_macro_phase

type SEMANTIC_MACRO_TOKEN
	previous as integer
	counter as longint
	expansion as longint
end type
dim shared as SEMANTIC_MACRO_TOKEN ptr semantic_macro_tokens
dim shared as integer semantic_macro_token_count, semantic_macro_token_capacity

'' Replacement tokens do not form a physical source range. Retain the actual
'' invocation names so typed observations can use a valid logical anchor.
'' IDs are appended in increasing order; lookup does not scan every expansion.
type SEMANTIC_MACRO_INVOCATION
	identity as longint
	parent as longint
	source as LEX_LOCATION
end type
dim shared as SEMANTIC_MACRO_INVOCATION ptr semantic_macro_invocations
dim shared as integer semantic_macro_invocation_count, semantic_macro_invocation_capacity
'' A hostile expansion must not multiply nested lookup work without a bound.
const SEMANTIC_MACRO_LOCATION_WORK_LIMIT = 8388608
dim shared as longint semantic_macro_location_work

'' A canonical symbol can be revisited by several declaration parser routes.
'' The same expansion-to-symbol observation is one relationship, not one
'' relationship per visit. Keep a bounded set without changing symbol lookup.
type SEMANTIC_MACRO_SYMBOL_ORIGIN
	subject as longint
	expansion as longint
	next_origin as integer
end type
const SEMANTIC_MACRO_ORIGIN_BUCKETS = 4096
dim shared as integer macro_origin_buckets(0 to SEMANTIC_MACRO_ORIGIN_BUCKETS - 1)
dim shared as SEMANTIC_MACRO_SYMBOL_ORIGIN ptr macro_symbol_origins
dim shared as integer macro_symbol_origin_count, macro_symbol_origin_capacity

sub fbSemanticModelResetMacros( )
	deallocate(macro_symbol_origins)
	macro_symbol_origins = NULL
	macro_symbol_origin_count = 0
	macro_symbol_origin_capacity = 0
	for bucket as integer = 0 to SEMANTIC_MACRO_ORIGIN_BUCKETS - 1
		macro_origin_buckets(bucket) = 0
	next
	deallocate(semantic_macro_slots)
	semantic_macro_slots = NULL
	semantic_macro_capacity = 0
	semantic_macro_count = 0
	semantic_macro_loader = 0
	semantic_macro_phase = ""
	deallocate(semantic_macro_tokens)
	semantic_macro_tokens = NULL
	semantic_macro_token_count = 0
	semantic_macro_token_capacity = 0
	deallocate(semantic_macro_invocations)
	semantic_macro_invocations = NULL
	semantic_macro_invocation_count = 0
	semantic_macro_invocation_capacity = 0
	semantic_macro_location_work = SEMANTIC_MACRO_LOCATION_WORK_LIMIT
	for depth as integer = 0 to FB_MAXINCRECLEVEL
		lex.ctxTB(depth).semantic_last_macro_token = -1
	next
end sub

private function hHash( byval allocation as ulongint ) as uinteger
	allocation xor= allocation shr 32
	allocation xor= allocation shr 16
	return cuint(allocation)
end function

private function hGrowDefinitions( ) as integer
	if( (semantic_macro_capacity <> 0) and (semantic_macro_count < semantic_macro_capacity \ 2) ) then return TRUE
	if( semantic_macro_count >= SEMANTIC_MACRO_MAX_DEFINITIONS ) then
		fbSemanticModelFailAt("semantic-macros.bas:101")
		return FALSE
	end if
	dim as integer capacity = iif(semantic_macro_capacity = 0, 128, semantic_macro_capacity * 2)
	dim as SEMANTIC_MACRO_SLOT ptr storage = callocate(capacity, sizeof(SEMANTIC_MACRO_SLOT))
	if( storage = NULL ) then
		fbSemanticModelFailAt("semantic-macros.bas:107")
		return FALSE
	end if
	for index as integer = 0 to semantic_macro_capacity - 1
		if( semantic_macro_slots[index].allocation = 0 ) then continue for
		dim as uinteger slot = hHash(semantic_macro_slots[index].allocation) and (capacity - 1)
		while( storage[slot].allocation <> 0 )
			slot = (slot + 1) and (capacity - 1)
		wend
		storage[slot] = semantic_macro_slots[index]
	next
	deallocate(semantic_macro_slots)
	semantic_macro_slots = storage
	semantic_macro_capacity = capacity
	return TRUE
end function

private function hWideText( byval value as const wstring ptr, byval units as integer = -1 ) as string
	if( value = NULL ) then return ""
	if( units < 0 ) then units = len(*value)
	dim as string result
	for index as integer = 0 to units - 1
		'' WSTRING indexing is a one-character string expression in BASIC;
		'' ASC reads its code unit instead of converting digit text to a number.
		result += hex(culng(asc(*value, index + 1)), 8)
	next
	return result
end function

private function hLocation( byref source as LEX_LOCATION ) as string
	return fbSemanticModelNumber(fbSemanticModelLocationIsPhysical(source)) + TABCHAR + _
		fbSemanticModelEscape(source.source_file) + TABCHAR + fbSemanticModelNumber(source.start_line) + TABCHAR + _
		fbSemanticModelNumber(source.start_column) + TABCHAR + fbSemanticModelNumber(source.end_line) + TABCHAR + _
		fbSemanticModelNumber(source.end_column)
end function

private function hDefinition( byval sym as FBSYMBOL ptr ) as longint
	if( sym = NULL ) then return 0
	if( symbIsDefine(sym) = FALSE ) then return 0
	fbSemanticModelInitializeSymbol(sym)
	if( hGrowDefinitions( ) = FALSE ) then return 0
	dim as ulongint allocation = sym->semantic_model_identity
	if( allocation = 0 ) then return 0
	dim as uinteger slot = hHash(allocation) and (semantic_macro_capacity - 1)
	while( semantic_macro_slots[slot].allocation <> 0 )
		if( semantic_macro_slots[slot].allocation = allocation ) then return semantic_macro_slots[slot].definition
		slot = (slot + 1) and (semantic_macro_capacity - 1)
	wend
	dim as longint symbolid = fbSemanticModelSymbolId(sym)
	dim as longint contextid = fbSemanticModelCurrentContext( )
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	dim as integer token_macro = (sym->typ and FB_DATATYPE_INVALID) <> 0
	dim as integer callback = sym->def.dprocz <> NULL
	if( token_macro ) then callback or= sym->def.mprocw <> NULL
	dim as string kind = iif(token_macro, "tokens", "text")
	if( callback ) then kind = iif(token_macro, "macro-callback", "define-callback")
	fbSemanticModelAppendProvenance("MD" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
		fbSemanticModelNumber(symbolid) + TABCHAR + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + _
		TABCHAR + fbSemanticModelNumber(contextid) + TABCHAR + fbSemanticModelEscape(*sym->id.name) + TABCHAR + kind + _
		TABCHAR + fbSemanticModelNumber(sym->def.params) + TABCHAR + fbSemanticModelNumber(sym->def.flags) + _
		TABCHAR + fbSemanticModelNumber(abs(sym->def.isargless <> FALSE)))
	semantic_macro_slots[slot].allocation = allocation
	semantic_macro_slots[slot].definition = identity
	semantic_macro_count += 1
	dim as string prefix = "MT" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR
	dim as FB_DEFPARAM ptr param = sym->def.paramhead
	while( param <> NULL )
		fbSemanticModelAppendProvenance(prefix + fbSemanticModelNumber(param->num) + TABCHAR + _
			"parameter" + TABCHAR + fbSemanticModelEscape(*param->name) + TABCHAR + "0")
		param = param->next
	wend
	if( callback ) then return identity
	if( token_macro = FALSE ) then
		dim as string value, text_kind = "text"
		if( sym->typ = FB_DATATYPE_WCHAR ) then
			value = hWideText(sym->def.textw)
			text_kind = "wide-text"
		elseif( sym->def.text <> NULL ) then
			value = *sym->def.text
		end if
		fbSemanticModelAppendProvenance(prefix + "0" + TABCHAR + text_kind + TABCHAR + fbSemanticModelEscape(value) + TABCHAR + "0")
		return identity
	end if
	dim as integer ordinal = 0
	dim as FB_DEFTOK ptr token = sym->def.tokhead
	while( token <> NULL )
		dim as string value, text_kind
		select case token->type
		case FB_DEFTOK_TYPE_PARAM
			text_kind = "parameter-reference"
			value = fbSemanticModelNumber(token->paramnum)
		case FB_DEFTOK_TYPE_PARAMSTR
			text_kind = "stringify-reference"
			value = fbSemanticModelNumber(token->paramnum)
		case FB_DEFTOK_TYPE_TEX
			text_kind = "text"
			if( token->text <> NULL ) then value = *token->text
		case FB_DEFTOK_TYPE_TEXW
			text_kind = "wide-text"
			value = hWideText(token->textw)
		end select
		fbSemanticModelAppendProvenance(prefix + fbSemanticModelNumber(ordinal) + TABCHAR + text_kind + TABCHAR + _
			fbSemanticModelEscape(value) + TABCHAR + fbSemanticModelNumber(abs(token->semantic_paste_before <> FALSE)))
		ordinal += 1
		token = token->next
	wend
	return identity
end function

'' -------------------------------------------------------------------------
'' Expansion and producer observations
'' -------------------------------------------------------------------------

private function hInvocationIndex( byval identity as longint ) as integer
	dim as integer first = 0
	dim as integer last = semantic_macro_invocation_count - 1
	while( first <= last )
		semantic_macro_location_work -= 1
		if( semantic_macro_location_work < 0 ) then
			fbSemanticModelFailAt("semantic-macros.bas:226")
			return -1
		end if
		dim as integer middle = first + (last - first) \ 2
		if( semantic_macro_invocations[middle].identity = identity ) then return middle
		if( semantic_macro_invocations[middle].identity < identity ) then
			first = middle + 1
		else
			last = middle - 1
		end if
	wend
	return -1
end function

function fbSemanticModelMacroExpressionLocation _
	( byref first as LEX_LOCATION, byref last as LEX_LOCATION, byval first_counter as longint, _
	  byval last_counter as longint, byref invocation as LEX_LOCATION ) as integer
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return FALSE
	dim as longint identity = first.macro_identity
	if( identity = 0 ) then identity = last.macro_identity
	if( identity = 0 ) then
		dim as integer token = lex.ctx->semantic_last_macro_token
		if( (token >= 0) and (token < semantic_macro_token_count) ) then
			if( (semantic_macro_tokens[token].counter > first_counter) and _
				(semantic_macro_tokens[token].counter <= last_counter) ) then identity = semantic_macro_tokens[token].expansion
		end if
	end if
	for depth as integer = 0 to 63
		dim as integer index = hInvocationIndex(identity)
		if( index < 0 ) then return FALSE
		if( fbSemanticModelLocationIsPhysical(semantic_macro_invocations[index].source) ) then
			invocation = semantic_macro_invocations[index].source
			'' This describes the expansion's origin, not the bytes of its result.
			invocation.is_physical = FALSE
			return TRUE
		end if
		identity = semantic_macro_invocations[index].parent
		'' #LINE remaps a written root invocation's logical location. Keep that
		'' observed anchor for typed facts even though it is not a physical edit
		'' range. Replacement-token coordinates still require the parent walk.
		if( identity = 0 ) then
			with semantic_macro_invocations[index].source
				if( (len(.source_file) > 0) and (.start_line > 0) and (.start_column >= 0) and _
					(.end_line >= .start_line) and (.end_column >= 0) and _
					((.end_line > .start_line) or (.end_column > .start_column)) ) then
					invocation = semantic_macro_invocations[index].source
					invocation.is_physical = FALSE
					return TRUE
				end if
			end with
		end if
		if( identity = 0 ) then return FALSE
	next
	return FALSE
end function

function fbSemanticModelMacroBegin( byval sym as FBSYMBOL ptr, byref source as LEX_LOCATION ) as longint
	if( fbSemanticModelEnabled( ) = FALSE ) then return 0
	'' Compact models still need the invocation chain to give generated typed
	'' expressions a truthful source anchor. Keep that small internal map even
	'' when the serialized expansion graph is disabled.
	if( (fbSemanticModelMacroProvenanceEnabled( ) = FALSE) and _
		(fbSemanticModelFullEnabled( ) = FALSE) ) then return 0
	if( lex.ctx->semantic_probe ) then return 0
	dim as longint definition = 0
	if( fbSemanticModelMacroProvenanceEnabled( ) ) then definition = hDefinition(sym)
	dim as longint contextid = fbSemanticModelCurrentContext( )
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	dim as longint parent = source.macro_identity
	dim as string relation = iif(parent = 0, "root", "replacement")
	if( semantic_macro_loader <> 0 ) then
		parent = semantic_macro_loader
		relation = semantic_macro_phase
	end if
	if( fbSemanticModelFullEnabled( ) ) then
		if( semantic_macro_invocation_count >= SEMANTIC_MACRO_MAX_DEFINITIONS ) then
			fbSemanticModelFailAt("semantic-macros.bas:283")
			return 0
		end if
		if( semantic_macro_invocation_count = semantic_macro_invocation_capacity ) then
			dim as integer capacity = iif(semantic_macro_invocation_capacity = 0, 128, semantic_macro_invocation_capacity * 2)
			dim as SEMANTIC_MACRO_INVOCATION ptr storage = reallocate(semantic_macro_invocations, capacity * sizeof(SEMANTIC_MACRO_INVOCATION))
			if( storage = NULL ) then
				fbSemanticModelFailAt("semantic-macros.bas:290")
				return 0
			end if
			'' LEX_LOCATION contains fixed storage. Each used slot is assigned
			'' completely before the count exposes it to binary lookup.
			semantic_macro_invocations = storage
			semantic_macro_invocation_capacity = capacity
		end if
		with semantic_macro_invocations[semantic_macro_invocation_count]
			.identity = identity
			.parent = parent
			.source = source
		end with
		semantic_macro_invocation_count += 1
	end if
	if( fbSemanticModelMacroProvenanceEnabled( ) ) then
		dim as string phase = "normal"
		if( pp.skipping ) then phase = "inactive"
		if( lex.ctx->kind = LEX_TKCTX_CONTEXT_EVAL ) then phase = "evaluation"
		fbSemanticModelAppendProvenance("MI" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
			fbSemanticModelNumber(parent) + TABCHAR + fbSemanticModelNumber(definition) + TABCHAR + _
			fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(contextid) + _
			TABCHAR + fbSemanticModelNumber(fbSemanticModelPPCurrentBranch( )) + TABCHAR + phase + TABCHAR + relation + TABCHAR + hLocation(source))
		fbSemanticModelExportCoordinates("macro-attempt", identity, "name", source, source)
	end if
	return identity
end function

sub fbSemanticModelMacroEnter( byval identity as longint, byref previous as longint )
	previous = semantic_macro_loader
	if( identity = 0 ) then exit sub
	semantic_macro_loader = identity
	semantic_macro_phase = "argument"
end sub

sub fbSemanticModelMacroLeave( byval previous as longint, byref phase as const string )
	semantic_macro_loader = previous
	semantic_macro_phase = phase
end sub

function fbSemanticModelMacroCurrentPhase( ) as string
	return semantic_macro_phase
end function

function fbSemanticModelMacroLoader( ) as longint
	return semantic_macro_loader
end function

sub fbSemanticModelMacroPhase( byref phase as const string )
	semantic_macro_phase = phase
end sub

function fbSemanticModelMacroTokenOrigin( ) as longint
	if( lex.ctx->semantic_macro_depth > 0 ) then return lex.ctx->semantic_macro_ids(lex.ctx->semantic_macro_depth - 1)
	return lex.ctx->semantic_eval_origin
end function

sub fbSemanticModelMacroPushOrigin( byval identity as longint, byval resume_length as integer, byval units as integer )
	if( (identity = 0) or (units = 0) ) then exit sub
	if( lex.ctx->semantic_macro_depth >= LEX_MAXMACROSTACK ) then
		fbSemanticModelFailAt("semantic-macros.bas:348")
		exit sub
	end if
	lex.ctx->semantic_macro_ids(lex.ctx->semantic_macro_depth) = identity
	lex.ctx->semantic_macro_resume(lex.ctx->semantic_macro_depth) = resume_length
	lex.ctx->semantic_macro_depth += 1
end sub

sub fbSemanticModelMacroArgument _
	( byval identity as longint, byval ordinal as integer, byval argument as LEXPP_ARG ptr, byval wide as integer )
	if( (identity = 0) or (fbSemanticModelMacroProvenanceEnabled( ) = FALSE) ) then exit sub
	dim as string value, kind = "bytes"
	if( wide ) then
		kind = "wide-units"
		value = hWideText(argument->textw.data)
	elseif( argument->text.data <> NULL ) then
		value = *argument->text.data
	end if
	dim as LEX_LOCATION source = argument->semantic_first
	dim as integer has_source = abs(argument->semantic_has_source <> FALSE)
	dim as integer coordinates_valid = has_source
	if( has_source ) then
		source.end_line = argument->semantic_last.end_line
		source.end_column = argument->semantic_last.end_column
		if( (source.source_file <> argument->semantic_last.source_file) or _
			(source.start_line < 1) or (source.start_column < 0) or _
			(source.end_line < source.start_line) or (source.end_column < 0) or _
			((source.end_line = source.start_line) and _
			 (source.end_column <= source.start_column)) ) then
			coordinates_valid = FALSE
		end if
		if( coordinates_valid ) then
			source.is_physical and= argument->semantic_last.is_physical
		else
			'' Keep a known token start as a non-editable point; never publish
			'' a reversed or zero-width argument as a physical source extent.
			source.is_physical = FALSE
			if( (len(source.source_file) = 0) or (source.start_line < 1) or _
				(source.start_column < 0) ) then
				has_source = FALSE
			else
				source.end_line = source.start_line
				source.end_column = source.start_column
			end if
		end if
	end if
	if( has_source = FALSE ) then
		source.source_file = ""
		source.start_line = 0
		source.start_column = 0
		source.end_line = 0
		source.end_column = 0
		source.is_physical = FALSE
	end if
	fbSemanticModelAppendProvenance("MA" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
		fbSemanticModelNumber(ordinal) + TABCHAR + kind + TABCHAR + fbSemanticModelEscape(value) + TABCHAR + _
		fbSemanticModelNumber(has_source) + TABCHAR + hLocation(source))
	if( coordinates_valid ) then
		fbSemanticModelExportCoordinates("macro-argument", identity, "formal-" + fbSemanticModelNumber(ordinal), _
			argument->semantic_first, argument->semantic_last)
	end if
	if( has_source ) then
		fbSemanticModelMacroOrigin("macro-argument", identity, argument->semantic_first.macro_identity, "start-" + fbSemanticModelNumber(ordinal))
		fbSemanticModelMacroOrigin("macro-argument", identity, argument->semantic_last.macro_identity, "end-" + fbSemanticModelNumber(ordinal))
	end if
end sub

sub fbSemanticModelMacroSegment _
	( byval identity as longint, byval ordinal as integer, byval token_ordinal as integer, _
	  byval parameter as integer, byref kind as const string, byval offset as integer, byval units as integer )
	if( (identity = 0) or (fbSemanticModelMacroProvenanceEnabled( ) = FALSE) ) then exit sub
	fbSemanticModelAppendProvenance("MS" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
		fbSemanticModelNumber(ordinal) + TABCHAR + fbSemanticModelNumber(token_ordinal) + TABCHAR + _
		fbSemanticModelNumber(parameter) + TABCHAR + kind + TABCHAR + fbSemanticModelNumber(offset) + TABCHAR + fbSemanticModelNumber(units))
end sub

sub fbSemanticModelMacroCallback( byval identity as longint, byref value as const string, byval error_code as integer )
	if( (identity = 0) or (fbSemanticModelMacroProvenanceEnabled( ) = FALSE) ) then exit sub
	fbSemanticModelAppendProvenance("MC" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + "bytes" + _
		TABCHAR + fbSemanticModelNumber(error_code) + TABCHAR + fbSemanticModelEscape(value))
end sub

sub fbSemanticModelMacroCallbackW( byval identity as longint, byval value as const wstring ptr, byval error_code as integer )
	if( (identity = 0) or (fbSemanticModelMacroProvenanceEnabled( ) = FALSE) ) then exit sub
	fbSemanticModelAppendProvenance("MC" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + "wide-units" + _
		TABCHAR + fbSemanticModelNumber(error_code) + TABCHAR + hWideText(value))
end sub

sub fbSemanticModelMacroResult _
	( byval identity as longint, byref outcome as const string, byval value as const any ptr, _
	  byval units as integer, byval wide as integer, byval arguments as integer, byref ending as LEX_LOCATION )
	if( (identity = 0) or (fbSemanticModelMacroProvenanceEnabled( ) = FALSE) ) then exit sub
	dim as string text, kind = "bytes"
	dim as LEX_LOCATION result_location = ending
	'' Empty expansions can end at a physical token boundary, but that point
	'' is not an editable source extent and cannot be advertised as one.
	if( (result_location.start_line = result_location.end_line) and _
		(result_location.start_column = result_location.end_column) ) then
		result_location.is_physical = FALSE
	end if
	if( wide ) then
		kind = "wide-units"
		text = hWideText(value, units)
	elseif( units > 0 ) then
		text = string(units, 0)
		for index as integer = 0 to units - 1
			text[index] = cptr(const ubyte ptr, value)[index]
		next
	end if
	fbSemanticModelAppendProvenance("ME" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + outcome + TABCHAR + _
		kind + TABCHAR + fbSemanticModelNumber(units) + TABCHAR + fbSemanticModelEscape(text) + TABCHAR + _
		fbSemanticModelNumber(arguments) + TABCHAR + hLocation(result_location))
end sub

sub fbSemanticModelMacroLifecycle _
	( byval sym as FBSYMBOL ptr, byref action as const string, byref spelling as const string, byref source as LEX_LOCATION )
	if( (fbSemanticModelEnabled( ) = FALSE) or _
		(fbSemanticModelMacroProvenanceEnabled( ) = FALSE) ) then exit sub
	dim as longint definition = hDefinition(sym)
	dim as longint contextid = fbSemanticModelCurrentContext( )
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	fbSemanticModelAppendProvenance("ML" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
		fbSemanticModelNumber(definition) + TABCHAR + action + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(contextid) + _
		TABCHAR + fbSemanticModelEscape(spelling) + TABCHAR + hLocation(source))
	fbSemanticModelExportCoordinates("macro-lifetime", identity, "range", source, source)
end sub

sub fbSemanticModelMacroOrigin _
	( byref domain as const string, byval subject as longint, byval expansion as longint, byref role as const string )
	if( (expansion = 0) or (fbSemanticModelMacroProvenanceEnabled( ) = FALSE) ) then exit sub
	if( (domain = "symbol") and (left(role, 12) = "declaration-") ) then
		dim as integer bucket = hHash(culngint(subject) xor (culngint(expansion) shl 1)) and (SEMANTIC_MACRO_ORIGIN_BUCKETS - 1)
		dim as integer origin = macro_origin_buckets(bucket)
		while( origin <> 0 )
			if( (macro_symbol_origins[origin - 1].subject = subject) and (macro_symbol_origins[origin - 1].expansion = expansion) ) then exit sub
			origin = macro_symbol_origins[origin - 1].next_origin
		wend
		if( macro_symbol_origin_count >= SEMANTIC_MACRO_MAX_DEFINITIONS ) then
			fbSemanticModelFailAt("semantic-macros.bas:487")
			exit sub
		end if
		if( macro_symbol_origin_count = macro_symbol_origin_capacity ) then
			dim as integer capacity = iif(macro_symbol_origin_capacity = 0, 128, macro_symbol_origin_capacity * 2)
			dim as SEMANTIC_MACRO_SYMBOL_ORIGIN ptr storage = reallocate(macro_symbol_origins, capacity * sizeof(SEMANTIC_MACRO_SYMBOL_ORIGIN))
			if( storage = NULL ) then
				fbSemanticModelFailAt("semantic-macros.bas:494")
				exit sub
			end if
			macro_symbol_origins = storage
			macro_symbol_origin_capacity = capacity
		end if
		with macro_symbol_origins[macro_symbol_origin_count]
			.subject = subject
			.expansion = expansion
			.next_origin = macro_origin_buckets(bucket)
		end with
		macro_symbol_origin_count += 1
		macro_origin_buckets(bucket) = macro_symbol_origin_count
	end if
	fbSemanticModelAppendProvenance("MR" + TABCHAR + domain + TABCHAR + fbSemanticModelNumber(subject) + _
		TABCHAR + fbSemanticModelNumber(expansion) + TABCHAR + role)
end sub

sub fbSemanticModelMacroConsumed( byval expansion as longint, byval counter as longint )
	if( (expansion = 0) or lex.ctx->semantic_probe ) then exit sub
	if( semantic_macro_token_count = semantic_macro_token_capacity ) then
		if( semantic_macro_token_count >= SEMANTIC_MACRO_MAX_DEFINITIONS ) then
			fbSemanticModelFailAt("semantic-macros.bas:516")
			exit sub
		end if
		dim as integer capacity = iif(semantic_macro_token_capacity = 0, 128, semantic_macro_token_capacity * 2)
		dim as SEMANTIC_MACRO_TOKEN ptr storage = reallocate(semantic_macro_tokens, capacity * sizeof(SEMANTIC_MACRO_TOKEN))
		if( storage = NULL ) then
			fbSemanticModelFailAt("semantic-macros.bas:522")
			exit sub
		end if
		semantic_macro_tokens = storage
		semantic_macro_token_capacity = capacity
	end if
	with semantic_macro_tokens[semantic_macro_token_count]
		.previous = lex.ctx->semantic_last_macro_token
		.counter = counter
		.expansion = expansion
	end with
	lex.ctx->semantic_last_macro_token = semantic_macro_token_count
	semantic_macro_token_count += 1
end sub

sub fbSemanticModelMacroExpressionOrigins( byval expression as longint, byval first_counter as longint, byval last_counter as longint )
	if( (fbSemanticModelEnabled( ) = FALSE) or _
		(fbSemanticModelMacroProvenanceEnabled( ) = FALSE) ) then exit sub
	'' These links come from actual token consumption, not overlapping source
	'' ranges. Each lexer context owns a chain, including temporary EVAL contexts.
	dim as integer index = lex.ctx->semantic_last_macro_token
	while( (index >= 0) and (index < semantic_macro_token_count) )
		if( semantic_macro_tokens[index].counter <= first_counter ) then exit while
		if( semantic_macro_tokens[index].counter <= last_counter ) then
			fbSemanticModelMacroOrigin("expression", expression, semantic_macro_tokens[index].expansion, _
				"token-" + fbSemanticModelNumber(semantic_macro_tokens[index].counter))
		end if
		index = semantic_macro_tokens[index].previous
	wend
end sub

'' end of tooling/semantic-macros.bas
