'' Project: FreeBASIC native resolution tests
'' File: resolution-guards.bas
'' Purpose: Exercise the production collector without invoking a resolver.
'' Responsibilities: Unchanged returns, copied snapshots, gates and finite lifetimes.
'' This file intentionally does NOT decide whether a procedure is compatible.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-resolution.bi"
#include once "tooling/semantic-constructs.bi"
#include once "parser/parser.bi"

dim shared as LEX_CTX lex
dim shared as PARSERCTX parser
dim shared as integer observing = TRUE, failures, writes, points, full_mode = TRUE, symbol_requests, fail_closed
dim shared as string captured

function fbSemanticModelTypedExpressionsEnabled( ) as integer
	return observing
end function
function fbSemanticModelMacroProvenanceEnabled( ) as integer
	return TRUE
end function
function fbSemanticModelFullEnabled( ) as integer
	return full_mode
end function
function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	symbol_requests += 1
	if( sym = NULL ) then return 0
	return sym->class
end function
function fbSemanticModelModuleIdentity( ) as longint
	return 1
end function
function fbSemanticModelNumber( byval value as longint ) as string
	return ltrim(str(value))
end function
function fbSemanticModelTypeNumber( byval value as longint ) as string
	return fbSemanticModelNumber(value)
end function
function fbSemanticModelEscape( byref value as const string ) as string
	return value
end function
function fbSemanticModelCurrentStatement( ) as longint
	return 8
end function
function fbSemanticModelCurrentContext( ) as longint
	return 9
end function
function fbSemanticModelCurrentSource( ) as longint
	return 1
end function
function lexGetLastLocation( ) as LEX_LOCATION
	points += 1
	return lex.ctx->last_source
end function
sub fbSemanticModelAppendProvenance( byref value as const string )
	writes += 1
	if( writes <= 64 ) then captured += value + chr(10)
end sub
sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
	if( fail_closed ) then observing = FALSE
end sub

lex.ctx = @lex.ctxTB(0)
with lex.ctx->last_source
	.source_file = "source.bas"
	.source_context = 1
	.start_line = 4
	.start_column = 5
	.end_line = 4
	.end_column = 6
	.raw_start_line = 4
	.raw_start_column = 5
	.raw_end_line = 4
	.raw_end_column = 6
	.raw_valid = TRUE
	.is_physical = TRUE
end with
dim as FBSYMBOL proc, param, wrong
proc.class = FB_SYMBCLASS_PROC
proc.typ = FB_DATATYPE_LONG
proc.proc.params = 1
proc.id.name = @"COPIED_PROCEDURE"
param.class = FB_SYMBCLASS_PARAM
param.typ = FB_DATATYPE_LONG
param.param.mode = FB_PARAMMODE_BYVAL
wrong.class = FB_SYMBCLASS_VAR
dim as ASTNODE original
original.class = AST_NODECLASS_CONST
original.dtype = FB_DATATYPE_LONG
original.val.i = 7
original.semantic_expression = 77

dim as longint scope_id = fbSemanticModelResolutionBegin("closest", @proc, 1, 0)
if( scope_id <> 1 ) then end 1
dim as longint first = fbSemanticModelResolutionCandidate(scope_id, @proc, "screen")
if( first <> 1 ) then end 2
if( fbSemanticModelResolutionArgument(first, 1, @param, @original, INVALID, 42, "type-difference") <> 42 ) then end 3
fbSemanticModelResolutionScore(first, 42, TRUE, TRUE, 0, 1, "parameter-checks", 0)
fbSemanticModelResolutionRank(scope_id, first, "better", 42, 1, first)
dim as longint confirmation = fbSemanticModelResolutionCandidate(scope_id, @proc, "confirm")
if( confirmation <> 2 ) then end 4
if( fbSemanticModelResolutionArgument(confirmation, 1, @param, @original, INVALID, 42, "type-difference") <> 42 ) then end 5
fbSemanticModelResolutionScore(confirmation, 42, TRUE, TRUE, 0, 1, "parameter-checks", 0)
if( fbSemanticModelResolutionEnd(scope_id, first, first, 42, 1, 0, @proc) <> @proc ) then end 35
if( failures <> 1 ) then end 36
failures = 0
if( fbSemanticModelResolutionEnd(scope_id, first, confirmation, 42, 1, 0, @proc) <> @proc ) then end 6
if( fbSemanticModelLastResolution( ) <> scope_id ) then end 37
fbSemanticModelResolutionUse(scope_id, @lex.ctx->last_source)
fbSemanticModelResolutionBoundUse(scope_id, 123)
if( fbSemanticModelResolutionAccess(@proc, 7, "member") <> 7 ) then end 7
fbSemanticModelFinishResolution(TRUE)
proc.id.name = @"CHANGED_AFTER_COPY"
if( original.val.i <> 7 or failures or writes <> 22 or points <> 2 ) then end 8
if( instr(captured, "COPIED_PROCEDURE") = 0 or instr(captured, "CHANGED_AFTER_COPY") ) then end 9
if( symbol_requests <> 21 ) then end 38
if( instr(captured, "native-resolution-context:1" + chr(9) + "3" + chr(9)) = 0 ) then end 39
if( instr(captured, "native-resolution-policy") = 0 or instr(captured, "native-resolution-counts") = 0 ) then end 10

writes = 0
points = 0
observing = FALSE
if( fbSemanticModelResolutionBegin("bad", @wrong, -1, 0) ) then end 11
if( fbSemanticModelResolutionArgument(999, 0, NULL, NULL, 99, -2, "bad") <> -2 ) then end 12
if( fbSemanticModelResolutionAccess(NULL, 7, "bad") <> 7 ) then end 13
if( fbSemanticModelResolutionEnd(999, 0, 0, -1, -1, -1, @wrong) <> @wrong ) then end 14
observing = TRUE
lex.ctx->semantic_probe = TRUE
if( fbSemanticModelResolutionBegin("bad", @wrong, -1, 0) ) then end 15
lex.ctx->semantic_probe = FALSE
parser.options = FB_PARSEROPT_ISPP
if( fbSemanticModelResolutionBegin("bad", @wrong, -1, 0) ) then end 16
parser.options = 0
lex.ctx = NULL
if( fbSemanticModelResolutionBegin("bad", @wrong, -1, 0) ) then end 17
lex.ctx = @lex.ctxTB(0)
if( writes or failures or points ) then end 18

if( fbSemanticModelResolutionBegin("bad", @proc, 1, 0) ) then end 19
if( fbSemanticModelResolutionBegin("closest", @proc, -1, 0) ) then end 20
if( fbSemanticModelResolutionBegin("closest", @wrong, 1, 0) ) then end 21
scope_id = fbSemanticModelResolutionBegin("closest", @proc, 1, 0)
if( scope_id <> 1 ) then end 22
if( fbSemanticModelResolutionCandidate(999, @proc, "screen") ) then end 23
if( fbSemanticModelResolutionCandidate(scope_id, NULL, "screen") ) then end 24
if( fbSemanticModelResolutionCandidate(scope_id, @proc, "bad") ) then end 25
first = fbSemanticModelResolutionCandidate(scope_id, @proc, "screen")
if( fbSemanticModelResolutionCandidate(scope_id, @proc, "screen") ) then end 26
fbSemanticModelResolutionArgument(first, 1, @wrong, @original, INVALID, 1, "same-type")
fbSemanticModelResolutionArgument(first, 1, @param, @original, INVALID, 1, "bad")
fbSemanticModelResolutionArgument(first, 1, @param, @original, 99, 1, "same-type")
fbSemanticModelResolutionArgument(first, 1, @param, @original, INVALID, -1, "same-type")
original.class = AST_CLASSES
fbSemanticModelResolutionArgument(first, 1, @param, @original, INVALID, 1, "same-type")
original.class = AST_NODECLASS_CONST
fbSemanticModelResolutionScore(999, 1, TRUE, TRUE, 1, 1, "parameter-checks", 0)
fbSemanticModelResolutionEnd(scope_id, 0, 0, 0, 0, 0, NULL)
if( failures <> 14 ) then end 27
fbSemanticModelFinishResolution(FALSE)

'' The fake sink counts events rather than storing the stress transcript.
writes = 0
points = 0
captured = ""
full_mode = FALSE
fail_closed = TRUE
for ordinal as integer = 1 to 1000000
	fbSemanticModelResolutionAccess(@proc, TRUE, "member")
next
fbSemanticModelResolutionAccess(@proc, TRUE, "member")
if( failures <> 15 or writes <> 2000000 ) then end 28
fbSemanticModelFinishResolution(FALSE)
observing = TRUE
fail_closed = FALSE
writes = 0
captured = ""
for depth as integer = 1 to 1024
	if( fbSemanticModelResolutionBegin("closest", NULL, 0, 0) <> depth ) then end 29
next
if( fbSemanticModelResolutionBegin("closest", NULL, 0, 0) ) then end 30
if( failures <> 16 ) then end 31
fbSemanticModelFinishResolution(FALSE)
writes = 0
captured = ""
scope_id = fbSemanticModelResolutionBegin("closest", NULL, 0, 0)
if( scope_id <> 1 ) then end 32
if( fbSemanticModelResolutionEnd(scope_id, 0, 0, 0, 0, 0, NULL) <> NULL ) then end 33
fbSemanticModelFinishResolution(TRUE)
if( writes <> 7 or instr(captured, "native-resolution:1") = 0 ) then end 34
print "RESOLUTION_GUARDS_PASS"

'' end of resolution-guards.bas
