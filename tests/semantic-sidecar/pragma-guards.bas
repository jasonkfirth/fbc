'' Project: FreeBASIC pragma semantic tests
'' File: pragma-guards.bas
'' Purpose: Exercise production pragma stack helpers and the observer together.
'' Responsibilities: Saved values, overflow/underflow, copied lifetime and bounds.
'' This file intentionally does NOT contain: pragma parsing or option evaluation.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-pragmas.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "parser/parser.bi"
dim shared as LEX_CTX lex
dim shared as PARSERCTX parser
dim shared as integer observing = TRUE, full_mode = TRUE, failures, native_errors, writes, fail_closed
dim shared as string captured

function fbSemanticModelFullEnabled( ) as integer
	return full_mode
end function
function fbSemanticModelTypedExpressionsEnabled( ) as integer
	return observing
end function
function fbSemanticModelMacroProvenanceEnabled( ) as integer
	return TRUE
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
	if( full_mode = FALSE ) then return 0
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
function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	return 12
end function
sub fbSemanticModelAppendProvenance( byref value as const string )
	writes += 1
	if( writes <= 32 ) then captured += value + chr(10)
end sub
sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
	if( fail_closed ) then observing = FALSE: full_mode = FALSE
end sub
sub errReport( byval errnum as integer, byval isbefore as integer, byval customText as const zstring ptr )
	native_errors += 1
end sub

'' Native definitions are inserted here.

lex.ctx = @lex.ctxTB(0)
lex.ctx->head = @lex.ctx->tokenTB(0)
with lex.ctx->head->source
	.source_file = "copied-pragma.bas"
	.start_line = 1
	.end_line = 1
	.end_column = 7
	.is_physical = TRUE
end with
dim as longint pragma_id = fbSemanticModelPragmaBegin("push")
fbSemanticModelPragmaSelect(0, 1, FB_COMPOPT_MSBITFIELDS, 27, "push", "msbitfields")
for index as integer = 1 to FB_MAXPRAGMARECLEVEL
	pragmaPush(LEXPP_PRAGMAOPT_BITFIELD, index * 10)
next
pragmaPush(LEXPP_PRAGMAOPT_BITFIELD, 999)
if( native_errors <> 1 or pragmaStk(0).tos <> FB_MAXPRAGMARECLEVEL ) then end 1
fbSemanticModelPragmaEnd(pragma_id, "completed", 0, native_errors)
pragma_id = fbSemanticModelPragmaBegin("pop")
fbSemanticModelPragmaSelect(0, 1, FB_COMPOPT_MSBITFIELDS, 27, "pop", "msbitfields")
dim as longint value
for index as integer = FB_MAXPRAGMARECLEVEL to 1 step -1
	pragmaPop(LEXPP_PRAGMAOPT_BITFIELD, value)
	if( value <> index * 10 ) then end 2
next
pragmaPop(LEXPP_PRAGMAOPT_BITFIELD, value)
if( native_errors <> 2 or value <> 0 or pragmaStk(0).tos <> 0 ) then end 3
lex.ctx->head->source.source_file = "changed.bas"
if( instr(captured, "copied-pragma.bas") = 0 or instr(captured, "changed.bas") ) then end 4
fbSemanticModelPragmaEnd(pragma_id, "completed", 1, native_errors)
fbSemanticModelFinishPragmas(TRUE)
if( failures ) then end 5

writes = 0
observing = FALSE
if( fbSemanticModelPragmaBegin("ignored") ) then end 6
fbSemanticModelPragmaSelect(999, 999, 999, 999, "bad", "bad")
observing = TRUE
lex.ctx->semantic_probe = TRUE
if( fbSemanticModelPragmaBegin("ignored") ) then end 7
lex.ctx->semantic_probe = FALSE
if( writes or failures ) then end 8

fbSemanticModelPragmaSelect(0, 1, 35, 27, "set", "msbitfields")
fbSemanticModelPragmaEnd(999, "completed", 0, 0)
if( failures <> 2 ) then end 9
fbSemanticModelResetPragmas()
failures = 0
for depth as integer = 1 to 1024
	pragma_id = fbSemanticModelPragmaBegin("once")
next
if( fbSemanticModelPragmaBegin("once") ) then end 10
if( failures <> 1 ) then end 11
for depth as integer = 1024 to 1 step -1
	fbSemanticModelPragmaEnd(depth, "completed", 0, 0)
next
if( failures <> 1 ) then end 12
fbSemanticModelResetPragmas()

'' Four collector events per pragma_id reach the production million-event bound.
failures = 0
writes = 0
for ordinal as integer = 1 to 250000
	pragma_id = fbSemanticModelPragmaBegin("once")
	fbSemanticModelPragmaSelect(1, 0, 0, 4, "set", "once")
	fbSemanticModelPragmaCallback(1, "once")
	fbSemanticModelPragmaEnd(pragma_id, "completed", 0, 0)
next
if( failures or writes <> 1000000 ) then end 13
fail_closed = TRUE
pragma_id = fbSemanticModelPragmaBegin("once")
if( failures <> 1 or observing <> FALSE ) then end 14
fbSemanticModelFinishPragmas(TRUE)
observing = TRUE
full_mode = FALSE
fail_closed = FALSE
failures = 0
pragma_id = fbSemanticModelPragmaBegin("once")
fbSemanticModelPragmaSelect(1, 0, 0, 4, "set", "once")
fbSemanticModelPragmaCallback(1, "once")
fbSemanticModelPragmaEnd(pragma_id, "completed", 0, 0)
fbSemanticModelFinishPragmas(TRUE)
if( failures ) then end 15
print "PRAGMA_GUARDS_PASS"

'' end of pragma-guards.bas
