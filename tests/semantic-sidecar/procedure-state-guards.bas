'' Project: FreeBASIC procedure runtime-state tests
'' File: procedure-state-guards.bas
'' Purpose: Test the production collector with controlled native extensions.
'' Responsibilities: Inactive gates, valid classes, copied IDs and bounded reset.
'' This file intentionally does NOT generate error handlers or GOSUB lowering.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-procedure-state.bi"
#include once "tooling/semantic-constructs.bi"
#include once "parser/parser.bi"

dim shared as LEX_CTX lex
dim shared as PARSERCTX parser
dim shared as integer observing = TRUE, failures, writes, points, fail_closed
dim shared as integer full_mode, symbol_requests
dim shared as string captured

function fbSemanticModelTypedExpressionsEnabled( ) as integer
	return observing
end function
function fbSemanticModelFullEnabled( ) as integer
	return full_mode
end function
function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	if( sym = NULL ) then return 0
	symbol_requests += 1
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
sub fbSemanticModelAppendDetail( byref value as string )
	writes += 1
	if( writes <= 32 ) then captured += value + chr(10)
end sub
sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
	if( fail_closed ) then observing = FALSE
end sub

lex.ctx = @lex.ctxTB(0)
dim as FBSYMBOL proc, variable, label_
dim as FB_PROCEXT extension_
proc.class = FB_SYMBCLASS_PROC
proc.proc.ext = @extension_
variable.class = FB_SYMBCLASS_VAR
label_.class = FB_SYMBCLASS_LABEL
full_mode = TRUE
fbSemanticModelProcedureState(@proc, "begin")
extension_.err.lasthnd = @variable
extension_.err.hndsaved = @variable
fbSemanticModelProcedureState(@proc, "handler-allocated")
fbSemanticModelProcedureState(@proc, "handler-save", "none", @variable, @label_)
fbSemanticModelProcedureState(@proc, "before-error-exit")
extension_.err.lasthnd = NULL
extension_.err.hndsaved = NULL
fbSemanticModelProcedureState(@proc, "after-error-exit")
fbSemanticModelFinishProcedureState(TRUE)
if( failures or writes <> 6 or instr(captured, "native-procedure-state:1") = 0 ) then end 1

writes = 0
full_mode = FALSE
fbSemanticModelProcedureState(NULL, "bad", "bad")
fbSemanticModelFinishProcedureState(TRUE)
if( writes or failures ) then end 2
full_mode = TRUE
fbSemanticModelProcedureState(NULL, "begin")
fbSemanticModelProcedureState(@variable, "begin")
proc.proc.ext = NULL
fbSemanticModelProcedureState(@proc, "begin")
proc.proc.ext = @extension_
fbSemanticModelProcedureState(@proc, "bad")
fbSemanticModelProcedureState(@proc, "begin", "bad")
extension_.err.lastmod = @label_
fbSemanticModelProcedureState(@proc, "module-name-save")
if( failures <> 6 ) then end 3
extension_.err.lastmod = NULL
fbSemanticModelFinishProcedureState(FALSE)

writes = 0
captured = ""
for ordinal as integer = 1 to 1000000
	fbSemanticModelProcedureState(@proc, "begin")
next
if( failures <> 6 or writes <> 1000000 ) then end 4
fbSemanticModelProcedureState(@proc, "begin")
if( failures <> 7 or writes <> 1000000 ) then end 5
fbSemanticModelFinishProcedureState(FALSE)
writes = 0
captured = ""
fbSemanticModelProcedureState(@proc, "begin")
fbSemanticModelFinishProcedureState(TRUE)
if( writes <> 2 or instr(captured, "native-procedure-state:1") = 0 ) then end 6
print "PROCEDURE_STATE_GUARDS_PASS"

'' end of procedure-state-guards.bas
