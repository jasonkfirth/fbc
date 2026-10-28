'' Project: FreeBASIC semantic sidecar tests
'' File: ir-flow-guards.bas
'' Purpose: Exercise production IR graph capture without emitting instructions.
'' Responsibilities: Edges, unreachable blocks, unsigned cases, reset and input guards.
'' This file intentionally does NOT execute a branch or resolve an indirect target.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-ir-flow.bi"

dim shared as SYMBCTX symb
dim shared as FBENV env
dim shared as FBSYMBOL procedure_symbol, entry_symbol, exit_symbol, target_symbol, invalid_symbol
dim shared as integer observing = TRUE, failures, identity_queries
dim shared as string captured

function fbSemanticModelFullEnabled( ) as integer
	return observing
end function
function fbSemanticModelNumber( byval value as longint ) as string
	return ltrim(str(value))
end function
function fbSemanticModelEscape( byref value as const string ) as string
	return value
end function
sub fbSemanticModelAppendDetail( byref value as string )
	captured += value + chr(10)
end sub
sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
end sub
function fbSemanticModelModuleIdentity( ) as longint
	return 1
end function
function fbSemanticModelCurrentLoad( ) as longint
	return 0
end function
sub fbSemanticModelExportSymbolDetails( byval sym as FBSYMBOL ptr, byval variables_live as integer )
	if variables_live then end 10
end sub
function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	identity_queries += 1
	if sym = @symb.globnspc then return 1
	if sym = @procedure_symbol then return 2
	if sym = @entry_symbol then return 3
	if sym = @exit_symbol then return 4
	if sym = @target_symbol then return 5
	if sym = @invalid_symbol then return 6
	end 11
end function

sub fbSemanticModelEffectCall( byval sym as FBSYMBOL ptr, byval indirect as integer, byval graph as longint, byval action as longint, byval load_id as longint )
end sub
sub fbSemanticModelEffectOperation( byref kind as const string, byval opcode as integer, byref role as const string, byval graph as longint, byval action as longint, byval load_id as longint )
end sub

procedure_symbol.class = FB_SYMBCLASS_PROC
entry_symbol.class = FB_SYMBCLASS_LABEL
exit_symbol.class = FB_SYMBCLASS_LABEL
target_symbol.class = FB_SYMBCLASS_LABEL
invalid_symbol.class = FB_SYMBCLASS_VAR
env.clopt.backend = FB_BACKEND_GCC

fbSemanticModelIRFlowBegin(@procedure_symbol, @entry_symbol)
fbSemanticModelIRFlowAction("label", 0, @entry_symbol, NULL, "normal")
fbSemanticModelIRFlowAction("conditional", AST_OP_JEQ, @target_symbol)
fbSemanticModelIRFlowAction("jump", AST_OP_JMP, @exit_symbol)
fbSemanticModelIRFlowAction("operation", 0, NULL, NULL, "store")
fbSemanticModelIRFlowAction("label", 0, @target_symbol, NULL, "normal")
fbSemanticModelIRFlowAction("return-value")
fbSemanticModelIRFlowAction("label", 0, @exit_symbol, NULL, "normal")
fbSemanticModelIRFlowEnd(@procedure_symbol, @exit_symbol)
fbSemanticModelFinishIRFlow(TRUE)
if failures then end 1
if instr(captured, "native-ir-counts:1" + chr(9) + "1" + chr(9) + "7" + chr(9) + "5" + chr(9) + "6" + chr(9) + "0" + chr(9) + "0") = 0 then end 2
if instr(captured, "native-ir-block:1:3" + chr(9) + "4" + chr(9) + "4" + chr(9) + "unreachable") = 0 then end 3
if procedure_symbol.class <> FB_SYMBCLASS_PROC or target_symbol.class <> FB_SYMBCLASS_LABEL then end 4

captured = ""
dim as ulongint values(0 to 0) = { &h8000000000000001ull }
dim as FBSYMBOL ptr targets(0 to 0) = { @target_symbol }
fbSemanticModelIRFlowBegin(@procedure_symbol, @entry_symbol)
fbSemanticModelIRFlowAction("label", 0, @entry_symbol, NULL, "normal")
fbSemanticModelIRFlowAddress(@target_symbol)
fbSemanticModelIRFlowTable(@values(0), @targets(0), 1, @exit_symbol, 0, &h8000000000000001ull)
fbSemanticModelIRFlowAction("label", 0, @target_symbol, NULL, "normal")
fbSemanticModelIRFlowAction("return-value")
fbSemanticModelIRFlowAction("label", 0, @exit_symbol, NULL, "normal")
fbSemanticModelIRFlowEnd(@procedure_symbol, @exit_symbol)
fbSemanticModelFinishIRFlow(TRUE)
if failures or instr(captured, "9223372036854775809") = 0 then end 5
if instr(captured, "native-ir-counts:1" + chr(9) + "1" + chr(9) + "5" + chr(9) + "3" + chr(9) + "4" + chr(9) + "1" + chr(9) + "1") = 0 then end 6

captured = ""
identity_queries = 0
fbSemanticModelIRFlowBegin(NULL, @entry_symbol)
fbSemanticModelIRFlowBegin(@invalid_symbol, @entry_symbol)
fbSemanticModelIRFlowBegin(@procedure_symbol, NULL)
fbSemanticModelIRFlowEnd(NULL, NULL)
if failures <> 4 or identity_queries or len(captured) then end 7
fbSemanticModelIRFlowBegin(@procedure_symbol, @entry_symbol)
fbSemanticModelIRFlowAction("other")
fbSemanticModelIRFlowAction("jump")
fbSemanticModelIRFlowAction("call")
fbSemanticModelIRFlowAction("label", 0, @invalid_symbol)
fbSemanticModelIRFlowEnd(@procedure_symbol, NULL)
if failures <> 9 then end 8
fbSemanticModelFinishIRFlow(FALSE)
captured = ""
identity_queries = 0
observing = FALSE
fbSemanticModelIRFlowBegin(NULL, NULL)
fbSemanticModelIRFlowAction("other")
fbSemanticModelIRFlowTable(NULL, NULL, -1, NULL, 0, 0)
fbSemanticModelIRFlowAddress(NULL)
fbSemanticModelIRFlowEnd(NULL, NULL)
fbSemanticModelFinishIRFlow(TRUE)
if failures <> 9 or identity_queries or len(captured) then end 9
print "IR_FLOW_GUARDS_PASS"

'' end of ir-flow-guards.bas
