'' Project: FreeBASIC semantic sidecar tests
'' File: object-operation-guards.bas
'' Purpose: Exercise production object-operation capture without constructing objects.
'' Responsibilities: Nesting, shared roots, cloning, resets and invalid/inactive inputs.
'' This file intentionally does NOT select a constructor or execute an object action.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-object-operations.bi"

dim shared as SYMBCTX symb
dim shared as ASTCTX ast
dim shared as LEX_CTX lex
dim shared as LEX_TKCTX probe_context
dim shared as FBSYMBOL owner_symbol, selected_symbol, field_symbol
dim shared as ASTNODE root, copied
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
function fbSemanticModelCurrentStatement( ) as longint
	return 8
end function
function fbSemanticModelCurrentSource( ) as longint
	return 9
end function
function fbSemanticModelCurrentContext( ) as longint
	return 10
end function
sub fbSemanticModelExportSymbolDetails( byval sym as FBSYMBOL ptr, byval variables_live as integer )
	if variables_live then end 10
end sub
function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	identity_queries += 1
	if sym = @symb.globnspc then return 1
	if sym = @owner_symbol then return 2
	if sym = @selected_symbol then return 3
	if sym = @field_symbol then return 4
	end 11
end function

owner_symbol.class = FB_SYMBCLASS_STRUCT
selected_symbol.class = FB_SYMBCLASS_PROC
field_symbol.class = FB_SYMBCLASS_FIELD
astInitNode(@root, AST_NODECLASS_CALL, FB_DATATYPE_VOID, NULL)
root.sym = @selected_symbol

dim as longint parent = fbSemanticModelObjectOperationBegin("constructor-chain", @owner_symbol)
dim as longint child = fbSemanticModelObjectOperationBegin("default-constructor-call", @owner_symbol, NULL, @selected_symbol)
fbSemanticModelObjectOperationEnd(child, @root, "built")
fbSemanticModelObjectOperationSelection(parent, @root)
fbSemanticModelObjectOperationEnd(parent, NULL, "complete")
fbSemanticModelObjectOperationNode(@root, 20)
astCopy(@copied, @root)
fbSemanticModelObjectOperationNode(@copied, 21)
fbSemanticModelFinishObjectOperations(TRUE)
if failures or instr(captured, "native-object-counts:1" + chr(9) + "2" + chr(9) + "2" + chr(9) + "0" + chr(9) + "1" + chr(9) + "4") = 0 then end 1
if instr(captured, "native-object-nodes:1" + chr(9) + "2") = 0 then end 2
if instr(captured, "native-object-nodes:2" + chr(9) + "2") = 0 then end 3
if root.sym <> @selected_symbol then end 4
astInitNode(@copied, AST_NODECLASS_NOP, FB_DATATYPE_VOID, NULL)
if copied.semantic_object_operation then end 5
captured = ""
fbSemanticModelFinishObjectOperations(TRUE)
if instr(captured, "native-object-counts:1" + chr(9) + "0" + chr(9) + "0" + chr(9) + "0" + chr(9) + "0" + chr(9) + "0") = 0 then end 6

captured = ""
identity_queries = 0
if fbSemanticModelObjectOperationBegin("constructor-chain", NULL) then end 7
if fbSemanticModelObjectOperationBegin("other", @owner_symbol) then end 7
if fbSemanticModelObjectOperationBegin("constructor-chain", @field_symbol) then end 7
if fbSemanticModelObjectOperationBegin("constructor-chain", @owner_symbol, NULL, @field_symbol) then end 7
fbSemanticModelObjectOperationEnd(1, NULL, "complete")
fbSemanticModelObjectOperationNode(NULL, 20)
if failures <> 6 or identity_queries or len(captured) then end 7

observing = FALSE
if fbSemanticModelObjectOperationBegin("other", NULL) then end 8
fbSemanticModelObjectOperationChoice("other", "other")
fbSemanticModelObjectOperationNumberChoice("other", 0)
fbSemanticModelObjectOperationSymbolChoice("other", NULL)
fbSemanticModelObjectOperationEnd(0, NULL, "other")
fbSemanticModelObjectOperationSelection(0, NULL)
fbSemanticModelObjectOperationNode(NULL, 0)
fbSemanticModelFinishObjectOperations(TRUE)
if failures <> 6 or identity_queries or len(captured) then end 8

observing = TRUE
probe_context.semantic_probe = TRUE
lex.ctx = @probe_context
if fbSemanticModelObjectOperationBegin("constructor-chain", @owner_symbol) then end 9
if failures <> 6 or identity_queries or len(captured) then end 9
lex.ctx = NULL
print "OBJECT_OPERATION_GUARDS_PASS"

'' end of object-operation-guards.bas
