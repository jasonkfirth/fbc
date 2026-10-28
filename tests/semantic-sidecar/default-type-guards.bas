'' Project: FreeBASIC DEF policy tests
'' File: default-type-guards.bas
'' Purpose: Exercise production native table helpers and the observer together.
'' Responsibilities: Native normalization, fallback, copying, exclusions and limits.
'' This file intentionally does NOT implement alternate DEF parsing or table setters.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-default-types.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "parser/parser.bi"
dim shared as LEX_CTX lex
dim shared as PARSERCTX parser
dim shared as FBENV env
dim shared as integer deftypeTB(0 to 30)
dim shared as integer observing = TRUE, full_mode = TRUE, failures, writes, fail_closed
dim shared as longint context_id = 10
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
function fbSemanticModelTypeNumber( byval value as longint ) as string
	return fbSemanticModelNumber(value)
end function
function fbSemanticModelEscape( byref value as const string ) as string
	return value
end function
function fbSemanticModelModuleIdentity( ) as longint
	return 1
end function
function fbSemanticModelCurrentContext( ) as longint
	if( full_mode = FALSE ) then return 0
	return context_id
end function
sub fbSemanticModelDefaultTypesChanged( )
	if( full_mode ) then context_id += 1
end sub
function fbSemanticModelCurrentStatement( ) as longint
	return 8
end function
function fbSemanticModelCurrentSource( ) as longint
	return 1
end function
function lexGetLastLocation( ) as LEX_LOCATION
	return lex.ctx->last_source
end function
function fbSemanticModelLocationIsPhysical( byref source as LEX_LOCATION ) as integer
	return abs(source.is_physical <> FALSE)
end function
sub fbSemanticModelAppendProvenance( byref value as const string )
	writes += 1
	if( writes <= 12 ) then captured += value + chr(10)
end sub
sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
	if( fail_closed ) then observing = FALSE: full_mode = FALSE
end sub

'' Native bodies are inserted here.

lex.ctx = @lex.ctxTB(0)
lex.ctx->last_source.source_file = "copied.bas"
lex.ctx->last_source.start_line = 1
lex.ctx->last_source.end_line = 1
lex.ctx->last_source.end_column = 1
lex.ctx->last_source.is_physical = TRUE
env.clopt.lang = FB_LANG_FB_FBLITE
for index as integer = 0 to 30
	deftypeTB(index) = FB_DATATYPE_INTEGER
next
if( symbGetDefType(@"@outside") <> FB_DATATYPE_INTEGER ) then end 1
env.clopt.lang = FB_LANG_QB
if( symbGetDefType(@"@outside") <> FB_DATATYPE_SINGLE ) then end 2
env.clopt.lang = FB_LANG_FB_FBLITE
fbSemanticModelBeginDefaultTypes()
symbSetDefType(1000000, -1000000, FB_DATATYPE_STRING)
'' ASC constants are unsigned in the native helper. A negative direct API
'' argument therefore clamps to underscore, not to A.
if( deftypeTB(0) <> FB_DATATYPE_INTEGER or deftypeTB(30) <> FB_DATATYPE_STRING ) then end 3
symbSetDefType(0, 1000000, FB_DATATYPE_STRING)
for index as integer = 0 to 30
	if( deftypeTB(index) <> FB_DATATYPE_STRING ) then end 3
next
if( symbGetDefType(@"aValue") <> FB_DATATYPE_STRING or symbGetDefType(@"_Value") <> FB_DATATYPE_STRING ) then end 4
if( context_id <> 12 ) then end 5
symbSetDefType(asc("Z"), asc("A"), FB_DATATYPE_STRING)
if( context_id <> 12 ) then end 6
lex.ctx->last_source.source_file = "changed.bas"
if( instr(captured, "copied.bas") = 0 or instr(captured, "changed.bas") ) then end 7
fbSemanticModelFinishDefaultTypes(TRUE)
if( failures or writes <> 5 ) then end 8

writes = 0
observing = FALSE
full_mode = FALSE
fbSemanticModelBeginDefaultTypes()
symbSetDefType(65, 65, FB_DATATYPE_LONG)
if( writes or deftypeTB(0) <> FB_DATATYPE_LONG ) then end 9
observing = TRUE
full_mode = TRUE
fbSemanticModelBeginDefaultTypes()
lex.ctx->semantic_probe = TRUE
symbSetDefType(65, 65, FB_DATATYPE_STRING)
lex.ctx->semantic_probe = FALSE
parser.options = FB_PARSEROPT_ISPP
symbSetDefType(65, 65, FB_DATATYPE_LONG)
parser.options = 0
if( writes <> 1 or failures ) then end 10
fbSemanticModelFinishDefaultTypes(FALSE)

fbSemanticModelBeginDefaultTypes()
fbSemanticModelDefaultTypeRange(65, 65, 64, 65, FB_DATATYPE_LONG, FALSE, context_id)
fbSemanticModelDefaultTypeRange(65, 65, 65, 96, FB_DATATYPE_LONG, FALSE, context_id)
fbSemanticModelDefaultTypeRange(65, 65, 66, 65, FB_DATATYPE_LONG, FALSE, context_id)
fbSemanticModelDefaultTypeRange(65, 65, 65, 65, FB_DATATYPE_INVALID, FALSE, context_id)
fbSemanticModelDefaultTypeRange(65, 65, 65, 65, FB_DATATYPE_LONG, FALSE, 0)
if( failures <> 5 ) then end 11
fbSemanticModelFinishDefaultTypes(FALSE)

failures = 0
writes = 0
captured = ""
fbSemanticModelBeginDefaultTypes()
for ordinal as integer = 1 to 1000000
	symbSetDefType(65, 65, FB_DATATYPE_LONG)
next
if( failures or writes <> 1000001 ) then end 12
fail_closed = TRUE
symbSetDefType(65, 65, FB_DATATYPE_LONG)
if( failures <> 1 or observing <> FALSE ) then end 13
fbSemanticModelFinishDefaultTypes(TRUE)
observing = TRUE
full_mode = FALSE
fail_closed = FALSE
failures = 0
fbSemanticModelBeginDefaultTypes()
symbSetDefType(65, 65, FB_DATATYPE_STRING)
fbSemanticModelFinishDefaultTypes(TRUE)
if( failures ) then end 14
print "DEFAULT_TYPE_GUARDS_PASS"

'' end of default-type-guards.bas
