'' Project: FreeBASIC semantic sidecar tests
'' File: evaluation-observer-guards.bas
'' Purpose: Exercise the production evaluation observer at its input boundaries.
'' Responsibilities: Copied storage, code units, depth, omissions and fail-closed limits.
'' This file intentionally does NOT contain: a replacement observer or evaluator.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-evaluations.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-macros.bi"
#include once "parser/parser.bi"
dim shared as LEX_CTX lex
dim shared as FBENV env
dim shared as integer observing = TRUE, failures, writes, fail_closed
dim shared as string captured
dim shared as string ready_buffer

function fbSemanticModelTypedExpressionsEnabled( ) as integer
	return observing
end function
function fbSemanticModelMacroProvenanceEnabled( ) as integer
	return FALSE
end function
function fbSemanticModelMacroLoader( ) as longint
	return 0
end function
function fbSemanticModelNumber( byval value as longint ) as string
	return ltrim(str(value))
end function
function fbSemanticModelEscape( byref value as const string ) as string
	return value
end function
function fbSemanticModelModuleIdentity( ) as longint
	return 1
end function
function fbSemanticModelCurrentContext( ) as longint
	return 9
end function
function fbSemanticModelCurrentStatement( ) as longint
	return 0
end function
function fbSemanticModelCurrentSource( ) as longint
	return 1
end function
function fbSemanticModelLocationIsPhysical( byref source as LEX_LOCATION ) as integer
	return abs(source.is_physical <> FALSE)
end function
function errGetCount( ) as integer
	return 0
end function
sub fbSemanticModelAppendProvenance( byref value as const string )
	writes += 1
	if( writes <= 32 ) then captured += value + chr(10)
end sub
sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
	if( fail_closed ) then observing = FALSE
end sub

private sub hPrepareBuffer( byval identity as longint )
	lex.ctx->kind = LEX_TKCTX_CONTEXT_EVAL
	lex.ctx->deftext.data = strptr(ready_buffer)
	lex.ctx->deftext.len = len(ready_buffer)
	fbSemanticModelEvaluationBuffer(identity)
	lex.ctx->kind = LEX_TKCTX_CONTEXT_INIT
end sub

lex.ctx = @lex.ctxTB(0)
lex.ctx->head = @lex.ctx->tokenTB(0)
env.inf.format = FBFILE_FORMAT_ASCII
ready_buffer = chr(10)
with lex.ctx->head->source
	.source_file = "before-evaluation.bas"
	.start_line = 1
	.end_line = 1
	.end_column = 7
	.is_physical = TRUE
end with
dim as string input_text = "A" + chr(0) + "B"
dim as longint evaluation_id = fbSemanticModelEvaluationBegin(lex.ctx, "constant-expression", strptr(input_text), len(input_text), FALSE)
hPrepareBuffer(evaluation_id)
input_text = "changed"
lex.ctx->head->source.source_file = "changed-evaluation.bas"
if( instr(captured, "before-evaluation.bas") = 0 or instr(captured, "changed-evaluation.bas") or _
	instr(captured, "A" + chr(0) + "B") = 0 ) then end 1
dim as wstring * 4 result_text = wchr(&h03BB) + wchr(&h00E9)
fbSemanticModelEvaluationEnd(evaluation_id, @result_text, len(result_text), TRUE, 0)
if( instr(captured, "955,233") = 0 ) then end 2
fbSemanticModelFinishEvaluations(TRUE)
if( failures ) then end 3

'' Disabled output and isolated probes must not even inspect an invalid buffer.
writes = 0
observing = FALSE
if( fbSemanticModelEvaluationBegin(NULL, "bad", NULL, -1, FALSE) ) then end 4
observing = TRUE
lex.ctx->semantic_probe = TRUE
if( fbSemanticModelEvaluationBegin(NULL, "bad", NULL, -1, FALSE) ) then end 5
lex.ctx->semantic_probe = FALSE
if( writes or failures ) then end 6

if( fbSemanticModelEvaluationBegin(NULL, "constant-expression", NULL, 0, FALSE) ) then end 7
if( fbSemanticModelEvaluationBegin(lex.ctx, "constant-expression", NULL, -1, FALSE) ) then end 8
if( fbSemanticModelEvaluationBegin(lex.ctx, "constant-expression", NULL, 65537, FALSE) ) then end 9
if( fbSemanticModelEvaluationBegin(lex.ctx, "constant-expression", NULL, 1, FALSE) ) then end 10
if( failures <> 4 ) then end 11
fbSemanticModelEvaluationEnd(999, NULL, 0, FALSE, 0)
if( failures <> 5 ) then end 12
lex.ctx->head = NULL
if( fbSemanticModelEvaluationBegin(lex.ctx, "constant-expression", NULL, 0, FALSE) ) then end 21
lex.ctx->head = @lex.ctx->tokenTB(0)
if( failures <> 6 ) then end 22
fbSemanticModelResetEvaluations( )

failures = 0
for depth as integer = 1 to FB_MAXINCRECLEVEL + 1
	evaluation_id = fbSemanticModelEvaluationBegin(lex.ctx, "constant-expression", NULL, 0, FALSE)
	if( evaluation_id <> depth ) then end 13
	hPrepareBuffer(evaluation_id)
next
if( fbSemanticModelEvaluationBegin(lex.ctx, "constant-expression", NULL, 0, FALSE) ) then end 14
if( failures <> 1 ) then end 15
for depth as integer = FB_MAXINCRECLEVEL + 1 to 1 step -1
	fbSemanticModelEvaluationEnd(depth, NULL, 0, FALSE, 0)
next
if( failures <> 1 ) then end 16
fbSemanticModelResetEvaluations( )

'' A fully populated copied buffer is accepted at the declared unit bound.
failures = 0
input_text = string(65536, "x")
evaluation_id = fbSemanticModelEvaluationBegin(lex.ctx, "constant-expression", strptr(input_text), len(input_text), FALSE)
if( evaluation_id = 0 ) then end 17
hPrepareBuffer(evaluation_id)
fbSemanticModelEvaluationEnd(evaluation_id, NULL, 0, FALSE, 0)
if( failures ) then end 18
fbSemanticModelResetEvaluations( )

'' Three actual events per scope reach the production million-event bound.
writes = 0
for ordinal as integer = 1 to 333333
	evaluation_id = fbSemanticModelEvaluationBegin(lex.ctx, "constant-expression", NULL, 0, FALSE)
	hPrepareBuffer(evaluation_id)
	fbSemanticModelEvaluationEnd(evaluation_id, NULL, 0, FALSE, 0)
next
if( failures or writes <> 999999 ) then end 19
fail_closed = TRUE
evaluation_id = fbSemanticModelEvaluationBegin(lex.ctx, "constant-expression", NULL, 0, FALSE)
hPrepareBuffer(evaluation_id)
if( failures <> 1 or observing <> FALSE ) then end 20
fbSemanticModelFinishEvaluations(FALSE)
print "EVALUATION_OBSERVER_PASS"
end 0

'' end of evaluation-observer-guards.bas
