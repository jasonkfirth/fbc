'' Project: FreeBASIC semantic diagnostic tests
'' File: diagnostic-context-guards.bas
'' Purpose: Exercise production diagnostic copying, publication and bounds.
'' Responsibilities: Byte strings, active symbol classes, reset and failed writes.
'' This file intentionally does NOT replace native diagnostic serialization.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-diagnostics.bi"
#include once "tooling/semantic-macros.bi"
#include once "parser/parser.bi"

dim shared as FBENV env
dim shared as PARSERCTX parser
dim shared as LEX_CTX lex
dim shared as integer observing, failures, lexer_queries
dim shared as longint detail_identity
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
	'' This independent encoder implements the callback's documented byte ABI.
	'' The production collector must call it once per inner string and again
	'' for the outer K field; the test later compares that K value with DP.
	dim as string result
	for index as integer = 0 to len(value) - 1
		dim as ubyte ch = value[index]
		if( (ch < 32) or (ch > 126) or (ch = asc("%")) ) then
			result += "%" + hex(ch, 2)
		else
			result += chr(ch)
		end if
	next
	return result
end function
sub fbSemanticModelAppendProvenance( byref value as const string )
	captured += value + chr(10)
end sub
sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
end sub
function fbSemanticModelCurrentSource( ) as longint
	return 0
end function
function fbSemanticModelCurrentStatement( ) as longint
	return 0
end function
function fbSemanticModelNextDetailIdentity( ) as longint
	detail_identity += 1
	return detail_identity
end function
function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	if( sym <> NULL ) then end 30
	return 0
end function
function fbGetTargetId( ) as string
	return "guard-target"
end function
function lexGetLastLocation( ) as LEX_LOCATION
	if( env.inited = FALSE ) then end 31
	lexer_queries += 1
	dim as LEX_LOCATION observed_point
	return observed_point
end function
function fbSemanticModelMacroExpressionLocation( byref first as LEX_LOCATION, byref last as LEX_LOCATION, _
	byval first_counter as longint, byval last_counter as longint, byref invocation as LEX_LOCATION ) as integer
	return FALSE
end function

env.clopt.maxerrors = 20
env.inf.name = "retired-source.bas"
if( fbSemanticDiagnosticsBegin("invocation-guard.fbcdia") = FALSE ) then end 1
fbSemanticDiagnosticsPolicy(FALSE, 0, FALSE, 0, FALSE)
fbSemanticModelDiagnostic("warning", 13, -1, @"outside parser", NULL, NULL)
if( fbSemanticDiagnosticsEnd(TRUE) = FALSE or lexer_queries ) then end 2

open "guard-source.bas" for output as #1
print #1, "print 1"
close #1
env.inf.name = "guard-source.bas"
env.inited = TRUE
observing = TRUE
if( fbSemanticDiagnosticsBegin("context-guard.fbcdia") = FALSE ) then end 3
fbSemanticDiagnosticsModule(env.inf.name)

dim as FBSYMBOL selected, owner
dim as FBSYMBOLTB symbol_table
owner.id.name = @"OWNER"
symbol_table.owner = @owner
selected.symtb = @symbol_table
selected.id.name = @"SELECTED"
for kind as integer = 0 to 6
	fbSemanticDiagnosticsPolicy(TRUE, 0, FALSE, 0, FALSE)
	selected.class = FB_SYMBCLASS_PROC
	selected.pattrib = 0
	selected.stats = 0
	dim as FBSYMBOL ptr current_proc = @selected
	select case kind
	case 0: current_proc = NULL
	case 1: selected.class = FB_SYMBCLASS_VAR
	case 2: selected.pattrib = FB_PROCATTRIB_CONSTRUCTOR
	case 3: selected.pattrib = FB_PROCATTRIB_DESTRUCTOR1
	case 4: selected.pattrib = FB_PROCATTRIB_OPERATOR
	case 5: selected.stats = FB_SYMBSTATS_FUNCPTR
	end select
	dim as string spelling = "raw%" + chr(9, 10, 13, 1, 255)
	fbSemanticDiagnosticsParameter(current_proc, -1, 1, strptr(spelling))
	spelling = "overwritten"
	fbSemanticModelDiagnostic("warning", 13, 1, @"copied context", NULL, NULL)
next
fbSemanticDiagnosticsFinishModule("accepted", 0)
if( fbSemanticDiagnosticsEnd(TRUE) = FALSE or failures ) then end 4
if( instr(captured, "raw%2525%2509%250A%250D%2501%25FF") = 0 ) then end 5
if( instr(captured, "overwritten") ) then end 6
open "captured-context.tsv" for output as #1
print #1, captured;
close #1
observing = FALSE

'' Each failed attempt uses the same destination. The previous valid artifact
'' must survive both the unescaped input bounds and the escaped record bound.
for failure_case as integer = 0 to 5
	if( fbSemanticDiagnosticsBegin("context-guard.fbcdia") = FALSE ) then end 7
	fbSemanticDiagnosticsModule(env.inf.name)
	fbSemanticDiagnosticsPolicy(TRUE, 0, FALSE, 0, FALSE)
	select case failure_case
	case 0
		'' One MiB is the collector's total unescaped parameter-text budget.
		dim as string oversized = string(1048576 + 1, "x")
		fbSemanticDiagnosticsParameter(NULL, 0, 1, strptr(oversized))
	case 1
		'' This input fits that budget but expands beyond the reader's row bound.
		dim as string escaped_oversized = string(400000, chr(1))
		fbSemanticDiagnosticsParameter(NULL, 0, 1, strptr(escaped_oversized))
	case 3
		fbSemanticDiagnosticsPolicy(TRUE, 0, FALSE, 0, FALSE, 0)
	case 4
		fbSemanticDiagnosticsPolicy(TRUE, 0, FALSE, 0, FALSE, -2)
	case 5
		fbSemanticDiagnosticsPolicy(TRUE, 0, FALSE, 0, FALSE, 4)
	case 2
		'' The final callback exceeds the 4096-frame native observation bound.
		for ordinal as integer = 1 to 4097
			fbSemanticDiagnosticsParameter(NULL, 0, ordinal, NULL)
		next
	end select
	fbSemanticModelDiagnostic("warning", 13, 1, @"bounded context", NULL, NULL)
	if( fbSemanticDiagnosticsEnd(FALSE) ) then end 8
next
if( failures <> 5 ) then end 9
print "DIAGNOSTIC_CONTEXT_GUARDS_PASS"

'' end of diagnostic-context-guards.bas
