'' Project: FreeBASIC semantic intrinsic dispatch tests
'' File: intrinsic-dispatch-guards.bas
'' Purpose: Check the production observer independently of native grammar fixtures.
'' Responsibilities: Copied values, inactive gates, invalid payloads and finite bounds.
'' This file intentionally does NOT resolve an intrinsic or retain an AST pointer.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-intrinsics.bi"
#include once "tooling/semantic-constructs.bi"
#include once "parser/parser.bi"
#include once "diagnostics/error.bi"

dim shared as LEX_CTX lex
dim shared as PARSERCTX parser
dim shared as integer observing = TRUE, errors, failures, writes, points
dim shared as string captured

function fbSemanticModelEnabled( ) as integer
	return observing
end function
function fbSemanticModelModuleIdentity( ) as longint
	return 1
end function
function fbSemanticModelNumber( byval value as longint ) as string
	return ltrim(str(value))
end function
function fbSemanticModelEscape( byref value as const string ) as string
	return value
end function
function fbSemanticModelTypeNumber( byval dtype as longint ) as string
	return fbSemanticModelNumber(dtype)
end function
sub fbSemanticModelAppendProvenance( byref value as const string )
	writes += 1
	if( writes <= 8 ) then captured += value + chr(10)
end sub
sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
end sub
function fbSemanticModelCurrentStatement( ) as longint
	return 8
end function
function fbSemanticModelCurrentContext( ) as longint
	return 9
end function
function errGetCount( ) as integer
	return errors
end function
function lexGetLastLocation( ) as LEX_LOCATION
	points += 1
	return lex.ctx->head->source
end function

dim as FBTOKEN token
lex.ctx = @lex.ctxTB(0)
lex.ctx->head = @token
token.source.source_file = "copied-entry.bas"
token.source.source_context = 1
token.source.start_line = 3
token.source.start_column = 4
token.source.raw_start_line = 3
token.source.raw_start_column = 4
token.source.is_physical = TRUE
token.source.raw_valid = TRUE
dim as SEMANTIC_INTRINSIC_SITE site
fbSemanticModelIntrinsicBegin(site)
token.source.source_file = "changed-after-entry.bas"
dim as ASTNODE original
original.class = AST_NODECLASS_CONST
original.dtype = FB_DATATYPE_DOUBLE
original.val.f = 1.5
fbSemanticModelIntrinsicEnd(site, "function", "cMathFunct", FB_TK_SIN, TRUE, @original)
if( original.class <> AST_NODECLASS_CONST or original.val.f <> 1.5 ) then end 1
if( instr(captured, "copied-entry.bas") = 0 or instr(captured, "changed-after-entry.bas") = 0 ) then end 2
fbSemanticModelIntrinsicEnd(site, "statement", "cPrintStmt", FB_TK_PRINT, FALSE)
errors = 1
fbSemanticModelIntrinsicEnd(site, "function", "cMathFunct", FB_TK_SIN, FALSE)
fbSemanticModelFinishIntrinsics(TRUE)
if( failures or writes <> 4 or points <> 3 ) then end 3
if( instr(captured, "declined") = 0 or instr(captured, "recovered") = 0 ) then end 4

writes = 0
points = 0
captured = ""
observing = FALSE
fbSemanticModelIntrinsicBegin(site)
fbSemanticModelIntrinsicEnd(site, "invalid", "bad", -1, TRUE, @original)
fbSemanticModelFinishIntrinsics(TRUE)
observing = TRUE
lex.ctx->semantic_probe = TRUE
fbSemanticModelIntrinsicBegin(site)
fbSemanticModelIntrinsicEnd(site, "invalid", "bad", -1, TRUE, @original)
lex.ctx->semantic_probe = FALSE
parser.options = FB_PARSEROPT_ISPP
fbSemanticModelIntrinsicBegin(site)
fbSemanticModelIntrinsicEnd(site, "invalid", "bad", -1, TRUE, @original)
parser.options = 0
if( writes or failures or points ) then end 5

lex.ctx->head = NULL
fbSemanticModelIntrinsicBegin(site)
lex.ctx->head = @token
fbSemanticModelIntrinsicBegin(site)
fbSemanticModelIntrinsicEnd(site, "function", "cMathFunct", -1, TRUE, @original)
fbSemanticModelIntrinsicEnd(site, "invalid", "cMathFunct", FB_TK_SIN, TRUE, @original)
fbSemanticModelIntrinsicEnd(site, "statement", "cPrintStmt", FB_TK_PRINT, TRUE, @original)
fbSemanticModelIntrinsicEnd(site, "function", "cMathFunct", FB_TK_SIN, FALSE, @original)
original.class = AST_CLASSES
fbSemanticModelIntrinsicEnd(site, "function", "cMathFunct", FB_TK_SIN, TRUE, @original)
original.class = AST_NODECLASS_CONST
errors = 0
fbSemanticModelIntrinsicEnd(site, "function", "cMathFunct", FB_TK_SIN, TRUE, @original)
errors = 1
if( failures <> 7 or writes or points ) then end 6

fbSemanticModelResetIntrinsics( )
for ordinal as integer = 1 to 100000
	fbSemanticModelIntrinsicEnd(site, "function", "cMathFunct", FB_TK_SIN, TRUE, @original)
next
fbSemanticModelIntrinsicEnd(site, "function", "cMathFunct", FB_TK_SIN, TRUE, @original)
if( failures <> 8 or writes <> 100000 or points <> 100000 ) then end 7
fbSemanticModelResetIntrinsics( )
writes = 0
captured = ""
fbSemanticModelIntrinsicEnd(site, "statement", "cPrintStmt", FB_TK_PRINT, TRUE)
fbSemanticModelFinishIntrinsics(FALSE)
if( writes <> 1 ) then end 8
fbSemanticModelFinishIntrinsics(TRUE)
if( writes <> 2 or instr(captured, "native-intrinsic-dispatch:1") = 0 ) then end 9
print "INTRINSIC_DISPATCH_GUARDS_PASS"

'' end of intrinsic-dispatch-guards.bas
