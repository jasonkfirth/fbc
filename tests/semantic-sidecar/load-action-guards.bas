'' Project: FreeBASIC semantic sidecar tests
'' File: load-action-guards.bas
'' Purpose: Exercise production load observation boundaries without loading ASTs.
'' Responsibilities: Nested origins, returns, action classes, reset and inactive hooks.
'' This file intentionally does NOT emit IR or invoke runtime handlers.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-loads.bi"

dim shared as SYMBCTX symb
dim shared as ASTCTX ast
dim shared as FBSYMBOL proc_symbol, callee_symbol, label_symbol
dim shared as ASTNODE proc_node, root_node, call_node, check_node
dim shared as integer observing = TRUE, failures, identity_queries, metadata_queries
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

function fbSemanticModelTypeNumber( byval value as longint ) as string
	return ltrim(str(value))
end function

function fbSemanticModelOperatorCode( byval value as integer ) as string
	return ""
end function

function fbSemanticModelFormatValue( byval dtype as integer, byval value as FBVALUE ptr, byref kind as string, byref text as string ) as integer
	return FALSE
end function

function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	identity_queries += 1
	if sym = @symb.globnspc then return 1
	if sym = @proc_symbol then return 2
	if sym = @callee_symbol then return 3
	if sym = @label_symbol then return 4
	end 10
end function

sub fbSemanticModelExportSymbolDetails( byval sym as FBSYMBOL ptr, byval variables_live as integer )
	if variables_live then end 10
	if sym <> @callee_symbol and sym <> @label_symbol then end 10
	metadata_queries += 1
end sub

private sub fresh( )
	fbSemanticModelResetLoads( )
	failures = 0
	identity_queries = 0
	metadata_queries = 0
	captured = ""
end sub

proc_symbol.class = FB_SYMBCLASS_PROC
callee_symbol.class = FB_SYMBCLASS_PROC
label_symbol.class = FB_SYMBCLASS_LABEL
proc_node.class = AST_NODECLASS_PROC
proc_node.sym = @proc_symbol
ast.proc.curr = @proc_node
ast.doemit = TRUE
root_node.class = AST_NODECLASS_VAR
call_node.class = AST_NODECLASS_CALL
check_node.class = AST_NODECLASS_BOUNDCHK
fbSemanticModelResetLoads( )
fbSemanticModelLoadOrigin(@root_node, 42)
fbSemanticModelLoadOrigin(@check_node, 43)
dim as longint root_id = fbSemanticModelLoadBegin(@root_node)
dim as longint call_id = fbSemanticModelLoadBegin(@call_node)
fbSemanticModelLoadCall(@callee_symbol, FALSE, "call")
fbSemanticModelLoadEnd(call_id, TRUE)
fbSemanticModelLoadEnd(root_id, TRUE)
dim as longint check_id = fbSemanticModelLoadBegin(@check_node)
fbSemanticModelLoadCheck("bounds-or-dimensions", @label_symbol)
fbSemanticModelLoadEnd(check_id, TRUE)
fbSemanticModelFinishLoads(TRUE)
if failures or root_id <> 1 or call_id <> 2 or check_id <> 3 or metadata_queries <> 2 then end 1
if root_node.semantic_load_origin <> 42 or call_node.semantic_load_origin then end 1
if instr(captured, "native-load-counts:1" + chr(9) + "3" + chr(9) + "3" + chr(9) + "1" + chr(9) + "1") = 0 then end 1
if instr(captured, "native-load-enter:2" + chr(9) + "1" + chr(9) + "2" + chr(9) + "0" + chr(9) + "42") = 0 then end 1

fresh( )
fbSemanticModelLoadOrigin(NULL, 1)
if fbSemanticModelLoadBegin(NULL) then end 2
fbSemanticModelLoadEnd(1, FALSE)
fbSemanticModelLoadCall(@callee_symbol, FALSE, "call")
fbSemanticModelLoadCheck("bounds-or-dimensions", @label_symbol)
if failures <> 5 or len(captured) or identity_queries or metadata_queries then end 2

fresh( )
call_id = fbSemanticModelLoadBegin(@call_node)
captured = ""
fbSemanticModelLoadCall(NULL, FALSE, "call")
fbSemanticModelLoadCall(@label_symbol, FALSE, "call")
fbSemanticModelLoadCall(@callee_symbol, FALSE, "guessed")
fbSemanticModelLoadCheck("guessed", @label_symbol)
fbSemanticModelLoadCheck("null-pointer", @callee_symbol)
fbSemanticModelLoadEnd(call_id + 1, FALSE)
if failures <> 6 or len(captured) or metadata_queries then end 3
fbSemanticModelLoadEnd(call_id, FALSE)

fresh( )
root_id = fbSemanticModelLoadBegin(@root_node)
dim as string previous_capture = captured
fbSemanticModelFinishLoads(FALSE)
if failures or captured <> previous_capture then end 4

fresh( )
observing = FALSE
fbSemanticModelLoadOrigin(NULL, -1)
if fbSemanticModelLoadBegin(NULL) then end 5
fbSemanticModelLoadEnd(-1, FALSE)
fbSemanticModelLoadCall(NULL, FALSE, "guessed")
fbSemanticModelLoadCheck("guessed", NULL)
fbSemanticModelFinishLoads(TRUE)
if failures or len(captured) or identity_queries or metadata_queries then end 5
print "LOAD_GUARDS_PASS"

'' end of load-action-guards.bas
