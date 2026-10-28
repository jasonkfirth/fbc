'' Project: FreeBASIC EXTERN semantic tests
'' File: extern-mangling-guards.bas
'' Purpose: Exercise the production EXTERN observer independently of parsing.
'' Responsibilities: Copied lifetime, exclusions, bounded nesting and event closure.
'' This file intentionally does NOT implement the compiler's EXTERN grammar.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-extern.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "parser/parser.bi"

dim shared as LEX_CTX lex
dim shared as PARSERCTX parser
dim shared as integer full_mode = TRUE, observing = TRUE, failures, writes, fail_closed
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
function fbSemanticModelLocationIsPhysical( byref source as LEX_LOCATION ) as integer
	return abs(source.is_physical <> FALSE)
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
	return 100 + parser.mangling
end function
function fbSemanticModelCurrentStatement( ) as longint
	return 8
end function
function fbSemanticModelCurrentSource( ) as longint
	return 9
end function
sub fbSemanticModelAppendProvenance( byref value as const string )
	writes += 1
	if( writes <= 16 ) then captured += value + chr(10)
end sub
sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
	if( fail_closed ) then
		full_mode = FALSE
		observing = FALSE
	end if
end sub

lex.ctx = @lex.ctxTB(0)
dim as LEX_LOCATION source
source.source_file = "copied-source.bas"
source.start_line = 1
source.start_column = 1
source.end_line = 1
source.end_column = 7
source.is_physical = TRUE
dim as string spelling = "C++"
parser.mangling = FB_MANGLING_CPP
fbSemanticModelExternTransition("begin", 1, FB_MANGLING_BASIC, 100, source, spelling, TRUE)
spelling = "changed"
source.source_file = "changed.bas"
if( instr(captured, "C++") = 0 or instr(captured, "copied-source.bas") = 0 or instr(captured, "changed") ) then end 1
parser.mangling = FB_MANGLING_BASIC
fbSemanticModelExternTransition("end", 1, FB_MANGLING_CPP, 104, source)
fbSemanticModelFinishExtern(TRUE)
if( failures or instr(captured, "extern-mangling-counts" + chr(9) + "2" + chr(9) + "1" + chr(9) + "1" + chr(9) + "0") = 0 ) then end 2

'' Disabled, reduced, probe and preprocessor observations allocate no scope.
writes = 0
observing = FALSE
full_mode = FALSE
fbSemanticModelExternTransition("begin", 1, 0, 100, source)
observing = TRUE
full_mode = TRUE
lex.ctx->semantic_probe = TRUE
fbSemanticModelExternTransition("begin", 1, 0, 100, source)
lex.ctx->semantic_probe = FALSE
parser.options = FB_PARSEROPT_ISPP
fbSemanticModelExternTransition("begin", 1, 0, 100, source)
parser.options = 0
lex.ctx = NULL
fbSemanticModelExternTransition("begin", 1, 0, 100, source)
lex.ctx = @lex.ctxTB(0)
if( writes or failures ) then end 3

'' Recovery expression output has native modes and scope IDs, but no full
'' configuration graph. A zero context must remain explicitly unavailable.
full_mode = FALSE
parser.mangling = FB_MANGLING_CDECL
fbSemanticModelExternTransition("begin", 1, 0, 0, source, "", FALSE, TRUE)
parser.mangling = FB_MANGLING_BASIC
fbSemanticModelExternTransition("end", 1, 1, 0, source)
fbSemanticModelFinishExtern(TRUE)
if( failures ) then end 12
full_mode = TRUE

'' Each rejected frame leaves the previous valid frame available to close.
parser.mangling = FB_MANGLING_CDECL
fbSemanticModelExternTransition("begin", 2, 0, 100, source, "c", TRUE)
fbSemanticModelExternTransition("end", 3, 1, 101, source)
fbSemanticModelExternTransition("end", 2, 2, 101, source)
if( failures <> 2 ) then end 4
parser.mangling = FB_MANGLING_BASIC
fbSemanticModelExternTransition("end", 2, 1, 101, source)
fbSemanticModelExternTransition("end", 2, 1, 101, source)
fbSemanticModelExternTransition("unknown", 2, 1, 101, source)
fbSemanticModelExternTransition("begin", 0, 0, 100, source)
fbSemanticModelExternTransition("begin", 2, 0, 0, source)
fbSemanticModelExternTransition("begin", 2, 7, 100, source)
parser.mangling = 7
fbSemanticModelExternTransition("begin", 2, 0, 100, source)
parser.mangling = FB_MANGLING_CDECL
fbSemanticModelExternTransition("begin", 2, 0, 100, source, space(FB_MAXLITLEN + 1), TRUE)
fbSemanticModelExternTransition("begin", 2, 0, 100, source, "invented", FALSE)
if( failures <> 10 ) then end 5

fbSemanticModelResetExtern()
failures = 0
for depth as integer = 1 to 1024
	fbSemanticModelExternTransition("begin", depth, 1, 101, source, "c", TRUE)
next
fbSemanticModelExternTransition("begin", 1025, 1, 101, source, "c", TRUE)
if( failures <> 1 ) then end 6
for depth as integer = 1024 to 1 step -1
	fbSemanticModelExternTransition("end", depth, 1, 101, source)
next
if( failures <> 1 ) then end 7

fbSemanticModelResetExtern()
failures = 0
writes = 0
'' One million is the production event bound, including both directions.
for pair as integer = 1 to 500000
	fbSemanticModelExternTransition("begin", pair, 1, 101, source, "c", TRUE)
	fbSemanticModelExternTransition("end", pair, 1, 101, source)
next
if( failures or writes <> 1000000 ) then end 8
fail_closed = TRUE
fbSemanticModelExternTransition("begin", 1, 1, 101, source, "c", TRUE)
if( failures <> 1 or full_mode <> FALSE ) then end 9
fbSemanticModelFinishExtern(TRUE)
if( writes <> 1000000 ) then end 10
full_mode = TRUE
observing = TRUE
fail_closed = FALSE
failures = 0
writes = 0
captured = ""
fbSemanticModelFinishExtern(TRUE)
if( failures or writes <> 1 or instr(captured, "extern-mangling-counts" + chr(9) + "0" + chr(9) + "0" + chr(9) + "0" + chr(9) + "0") = 0 ) then end 11
print "EXTERN_MANGLING_GUARDS_PASS"

'' end of extern-mangling-guards.bas
