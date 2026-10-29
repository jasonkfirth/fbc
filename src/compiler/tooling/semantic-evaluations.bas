'' Project: FreeBASIC compiler - preprocessor evaluation contexts
'' File: tooling/semantic-evaluations.bas
'' Purpose: Preserve actual temporary lexer inputs, results and replay lifetimes.
'' Responsibilities: Bounded nesting, copied code units and module count closure.
'' This file intentionally does NOT contain: evaluation, lookup or parser recovery.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-evaluations.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-macros.bi"
#include once "parser/parser.bi"
#include once "crt/string.bi"

'' The compiler runs these callbacks serially. Frames retain only scalar IDs
'' and the native error count. All lexer pointers and callback result buffers
'' are copied before their owner can reset or release them.
private const EVALUATION_EVENT_LIMIT = 1000000
private const EVALUATION_DEPTH_LIMIT = FB_MAXINCRECLEVEL + 1
'' Temporary inputs can be longer than one literal after macro substitution.
'' Bound copied units independently of the parser's single-literal limit.
private const EVALUATION_UNIT_LIMIT = 65536
private type SEMANTIC_EVALUATION_FRAME
	identity as longint
	errors_before as integer
	buffered as integer
end type
private dim shared as SEMANTIC_EVALUATION_FRAME evaluation_frames(0 to EVALUATION_DEPTH_LIMIT - 1)
private dim shared as integer evaluation_depth
private dim shared as longint evaluation_events, evaluation_begins, evaluation_ends

private function hEnabled( ) as integer
	if( fbSemanticModelTypedExpressionsEnabled( ) = FALSE or lex.ctx = NULL ) then return FALSE
	return lex.ctx->semantic_probe = FALSE
end function

sub fbSemanticModelResetEvaluations( )
	evaluation_depth = 0
	evaluation_events = 0
	evaluation_begins = 0
	evaluation_ends = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendProvenance("K" + TABCHAR + "module" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

private sub hEvent( byref kind as const string, byval identity as longint, byref value as const string )
	if( evaluation_events >= EVALUATION_EVENT_LIMIT ) then
		fbSemanticModelFailAt("evaluation event limit")
		exit sub
	end if
	evaluation_events += 1
	hProperty("native-evaluation:" + fbSemanticModelNumber(evaluation_events), _
		kind + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + value)
end sub

private function hPayload _
	( byval value as const any ptr, byval units as integer, byval wide as integer ) as string

	if( units < 0 or units > EVALUATION_UNIT_LIMIT or (units > 0 and value = NULL) ) then
		fbSemanticModelFailAt("evaluation payload bound or unavailable buffer")
		return ""
	end if
	dim as string text
	dim as integer unit_bytes = 1
	if( wide ) then
		'' This is the host callback buffer's code-unit width. A cross target's
		'' WCHAR size must not reinterpret the host's WSTRING representation.
		unit_bytes = sizeof(wstring * 2) - sizeof(wstring * 1)
		for i as integer = 0 to units - 1
			if( i > 0 ) then text += ","
			text += fbSemanticModelNumber(culngint(asc(*cptr(const wstring ptr, value), i + 1)))
		next
	else
		text = space(units)
		'' A failed evaluator can return a string containing a zero byte.
		'' Copy its supplied length instead of treating it as a C string.
		if( units > 0 ) then memcpy(strptr(text), value, units)
	end if
	return iif(wide, "wide", "bytes") + TABCHAR + fbSemanticModelNumber(units) + TABCHAR + _
		fbSemanticModelNumber(unit_bytes) + TABCHAR + fbSemanticModelEscape(text)
end function

function fbSemanticModelEvaluationBegin _
	( byval parent_ctx as LEX_TKCTX ptr, byref purpose as const string, _
	  byval value as const any ptr, byval units as integer, byval wide as integer, _
	  byval replay_of as longint ) as longint

	if( hEnabled( ) = FALSE ) then return 0
	if( parent_ctx = NULL or evaluation_depth >= EVALUATION_DEPTH_LIMIT or _
		evaluation_begins >= EVALUATION_EVENT_LIMIT or _
		(purpose <> "constant-expression" and purpose <> "symbol-query") or _
		replay_of < 0 or replay_of > evaluation_begins ) then
		fbSemanticModelFailAt("evaluation context bound or invalid entry")
		return 0
	end if
	if( parent_ctx->head = NULL ) then
		fbSemanticModelFailAt("evaluation parent has no initialized token head")
		return 0
	end if
	dim as string payload = hPayload(value, units, wide)
	if( len(payload) = 0 ) then return 0
	dim as longint parent, loader
	if( evaluation_depth > 0 ) then parent = evaluation_frames(evaluation_depth - 1).identity
	if( fbSemanticModelMacroProvenanceEnabled( ) ) then loader = fbSemanticModelMacroLoader( )
	evaluation_begins += 1
	dim as longint identity = evaluation_begins
	evaluation_frames(evaluation_depth).identity = identity
	evaluation_frames(evaluation_depth).errors_before = errGetCount( )
	evaluation_frames(evaluation_depth).buffered = FALSE
	evaluation_depth += 1
	dim as LEX_LOCATION source = parent_ctx->head->source
	hEvent("begin", identity, fbSemanticModelNumber(parent) + TABCHAR + fbSemanticModelNumber(replay_of) + TABCHAR + _
		purpose + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + _
		fbSemanticModelNumber(parent_ctx->kind) + TABCHAR + payload + TABCHAR + fbSemanticModelEscape(source.source_file) + TABCHAR + _
		fbSemanticModelNumber(source.start_line) + TABCHAR + fbSemanticModelNumber(source.start_column) + TABCHAR + _
		fbSemanticModelNumber(source.end_line) + TABCHAR + fbSemanticModelNumber(source.end_column) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelLocationIsPhysical(source)) + TABCHAR + fbSemanticModelNumber(loader))
	return identity
end function

sub fbSemanticModelEvaluationBuffer( byval identity as longint )
	if( identity = 0 or hEnabled( ) = FALSE ) then exit sub
	if( evaluation_depth = 0 ) then fbSemanticModelFailAt("evaluation buffer has no context"): exit sub
	if( evaluation_frames(evaluation_depth - 1).identity <> identity or _
		evaluation_frames(evaluation_depth - 1).buffered or lex.ctx->kind <> LEX_TKCTX_CONTEXT_EVAL ) then
		fbSemanticModelFailAt("evaluation buffer has another context or was already copied")
		exit sub
	end if
	'' hArgInsertArgA/W can convert the callback argument to the active file's
	'' lexer representation. Copy the selected, initialized union member after
	'' the native end-of-expression LF has been appended, not an inferred copy
	'' of the argument. This also preserves Z callbacks used in wide files.
	dim as string payload
	if( env.inf.format = FBFILE_FORMAT_ASCII ) then
		payload = hPayload(lex.ctx->deftext.data, lex.ctx->deftext.len, FALSE)
	else
		payload = hPayload(lex.ctx->deftextw.data, lex.ctx->deftextw.len, TRUE)
	end if
	if( len(payload) = 0 ) then exit sub
	hEvent("buffer", identity, payload)
	evaluation_frames(evaluation_depth - 1).buffered = TRUE
end sub

sub fbSemanticModelEvaluationEnd _
	( byval identity as longint, byval value as const any ptr, byval units as integer, _
	  byval wide as integer, byval error_code as integer, byref reason as const string )

	if( identity = 0 or hEnabled( ) = FALSE ) then exit sub
	if( evaluation_depth = 0 ) then fbSemanticModelFailAt("evaluation close has no context"): exit sub
	if( evaluation_frames(evaluation_depth - 1).identity <> identity or error_code < 0 or _
		evaluation_frames(evaluation_depth - 1).buffered = FALSE or _
		(reason <> "completed" and reason <> "replay") ) then
		fbSemanticModelFailAt("evaluation close disagrees with its context")
		exit sub
	end if
	dim as string payload
	if( reason = "replay" ) then
		if( value <> NULL or units <> 0 or wide ) then fbSemanticModelFailAt("evaluation replay invents a result"): exit sub
		payload = "none" + TABCHAR + "0" + TABCHAR + "0" + TABCHAR
	else
		payload = hPayload(value, units, wide)
		if( len(payload) = 0 ) then exit sub
	end if
	hEvent("end", identity, reason + TABCHAR + fbSemanticModelNumber(error_code) + TABCHAR + _
		fbSemanticModelNumber(evaluation_frames(evaluation_depth - 1).errors_before) + TABCHAR + _
		fbSemanticModelNumber(errGetCount( )) + TABCHAR + payload)
	evaluation_depth -= 1
	evaluation_ends += 1
end sub

sub fbSemanticModelFinishEvaluations( byval commit as integer )
	if( commit and fbSemanticModelTypedExpressionsEnabled( ) ) then
		hProperty("native-evaluation-counts", fbSemanticModelNumber(evaluation_events) + TABCHAR + _
			fbSemanticModelNumber(evaluation_begins) + TABCHAR + fbSemanticModelNumber(evaluation_ends) + TABCHAR + _
			fbSemanticModelNumber(evaluation_depth) + TABCHAR + fbSemanticModelNumber(EVALUATION_DEPTH_LIMIT))
	end if
	fbSemanticModelResetEvaluations( )
end sub

'' end of tooling/semantic-evaluations.bas
