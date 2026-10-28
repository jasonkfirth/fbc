'' Project: FreeBASIC semantic recovery tests
'' File: recovery-step-guards.bas
'' Purpose: Exercise copied recovery events without running the source parser.
'' Responsibilities: Root identity, null roots, probe/off omissions, limits and reset.
'' This file intentionally does NOT construct a replacement or run an optimizer.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-recovery.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "diagnostics/error.bi"

dim shared as LEX_CTX lex
dim shared as integer observing = TRUE, failures, writes, points
dim shared as string captured

function fbSemanticModelTypedExpressionsEnabled( ) as integer
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
function fbSemanticModelCurrentSource( ) as longint
	return 9
end function
function fbSemanticModelCurrentContext( ) as longint
	return 10
end function
function errGetCount( ) as integer
	return 1
end function
function lexGetLastLocation( ) as LEX_LOCATION
	points += 1
	dim as LEX_LOCATION observed
	observed.source_file = "source.bas"
	observed.start_line = 3
	observed.start_column = 4
	observed.raw_start_line = 3
	observed.raw_start_column = 4
	observed.raw_valid = TRUE
	observed.is_physical = TRUE
	return observed
end function

lex.ctx = @lex.ctxTB(0)
dim as ASTNODE original
original.class = AST_NODECLASS_CONST
original.dtype = FB_DATATYPE_INTEGER
original.val.i = 7
if( fbSemanticModelRecoveryNode(@original, "retained-prefix", "guard", "parser/expressions/parser-expr-binary.bas", 1) <> @original ) then end 1
if( original.val.i <> 7 or original.class <> AST_NODECLASS_CONST ) then end 2
if( fbSemanticModelRecoveryNode(NULL, "replacement-zero", "guard", "parser/expressions/parser-expr-unary.bas", 2) <> NULL ) then end 3
fbSemanticModelFinishRecovery(TRUE)
if( writes <> 6 or failures or points <> 2 ) then end 4
if( instr(captured, "native-expression-recovery-count" + chr(9) + "2") = 0 ) then end 5

writes = 0
captured = ""
if( fbSemanticModelRecoveryType(typeAddrOf(FB_DATATYPE_INTEGER), "pointer", "guard", "file", 1) <> typeAddrOf(FB_DATATYPE_INTEGER) ) then end 15
fbSemanticModelRecoveryName("copied-recovery-name", "guard", "file", 1)
dim as FBSYMBOL recovered_parameter
recovered_parameter.class = FB_SYMBCLASS_PARAM
recovered_parameter.typ = FB_DATATYPE_INTEGER
recovered_parameter.id.name = @"copied-parameter"
recovered_parameter.lgt = 8
if( fbSemanticModelRecoverySymbol(@recovered_parameter, "guard", "file", 1) <> @recovered_parameter ) then end 16
recovered_parameter.id.name = @"changed-after-copy"
if( instr(captured, "copied-recovery-name") = 0 or instr(captured, "copied-parameter") = 0 ) then end 17
if( instr(captured, "changed-after-copy") ) then end 18
fbSemanticModelFinishRecovery(FALSE)

writes = 0
captured = ""
if( fbSemanticModelRecoveryType(FB_DATATYPE_INTEGER, "pointer", "guard", "file", 1) <> FB_DATATYPE_INTEGER ) then end 19
if( fbSemanticModelRecoveryType(FB_DATATYPE_INTEGER, "discarded", "guard", "file", 1) <> FB_DATATYPE_INTEGER ) then end 20
recovered_parameter.class = FB_SYMBCLASS_VAR
if( fbSemanticModelRecoverySymbol(@recovered_parameter, "guard", "file", 1) <> @recovered_parameter ) then end 21
fbSemanticModelRecoveryName("", "guard", "file", 1)
if( failures <> 4 or writes ) then end 22
failures = 0

writes = 0
points = 0
captured = ""
observing = FALSE
if( fbSemanticModelRecoveryNode(@original, "bad", "", "", -1) <> @original ) then end 6
observing = TRUE
lex.ctx->semantic_probe = TRUE
if( fbSemanticModelRecoveryNode(@original, "bad", "", "", -1) <> @original ) then end 7
lex.ctx->semantic_probe = FALSE
if( writes or failures or points ) then end 8
lex.ctx = NULL
fbSemanticModelRecoveryNode(@original, "retained-prefix", "guard", "file", 1)
lex.ctx = @lex.ctxTB(0)
fbSemanticModelRecoveryNode(@original, "bad", "guard", "file", 1)
fbSemanticModelRecoveryNode(@original, "retained-prefix", "guard", "file", 0)
if( failures <> 3 or writes or points ) then end 9
original.class = AST_NODECLASS_LINK
if( fbSemanticModelRecoveryNode(@original, "replacement-constant", "guard", "file", 1) <> @original ) then end 12
original.class = AST_CLASSES
if( fbSemanticModelRecoveryNode(@original, "retained-prefix", "guard", "file", 1) <> @original ) then end 13
original.class = AST_NODECLASS_CONST
if( failures <> 5 or writes or points ) then end 14

'' The callback sink counts rows instead of retaining the whole stress input.
for ordinal as integer = 1 to 100000
	fbSemanticModelRecoveryNode(@original, "retained-prefix", "guard", "file", 1)
next
fbSemanticModelRecoveryNode(@original, "retained-prefix", "guard", "file", 1)
if( failures <> 6 or writes <> 100000 or points <> 100000 ) then end 10
fbSemanticModelFinishRecovery(FALSE)
writes = 0
captured = ""
fbSemanticModelRecoveryNode(@original, "retained-prefix", "guard", "file", 1)
fbSemanticModelFinishRecovery(TRUE)
if( writes <> 5 or instr(captured, "native-expression-recovery:1") = 0 ) then end 11
print "RECOVERY_STEP_GUARDS_PASS"

'' end of recovery-step-guards.bas
