'' Project: FreeBASIC semantic sidecar tests
'' File: native-block-guards.bas
'' Purpose: Exercise production block capture without parser or AST lowering.
'' Responsibilities: Native class/range guards, module reset and inactive hooks.
'' This file intentionally does NOT perform cleanup or resolve branch targets.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-blocks.bi"

dim shared as SYMBCTX symb
dim shared as ASTCTX ast
dim shared as FBSYMBOL proc_symbol, scope_symbol, variable_symbol, entry_symbol, exit_symbol
dim shared as ASTNODE proc_node, scope_node, break_node, action_node
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

function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	identity_queries += 1
	if sym = @symb.globnspc then return 1
	if sym = @proc_symbol then return 2
	if sym = @scope_symbol then return 3
	if sym = @variable_symbol then return 4
	if sym = @entry_symbol then return 5
	if sym = @exit_symbol then return 6
	end 10
end function

private sub fresh( )
	fbSemanticModelResetBlocks( )
	captured = ""
	failures = 0
	identity_queries = 0
	break_node.break.semantic_identity = 0
	break_node.break.semantic_cleanup_count = 0
	action_node.semantic_exit_cleanup = 0
end sub

proc_symbol.class = FB_SYMBCLASS_PROC
scope_symbol.class = FB_SYMBCLASS_SCOPE
variable_symbol.class = FB_SYMBCLASS_VAR
entry_symbol.class = FB_SYMBCLASS_LABEL
exit_symbol.class = FB_SYMBCLASS_LABEL
proc_node.class = AST_NODECLASS_PROC
proc_node.sym = @proc_symbol
proc_node.block.initlabel = @entry_symbol
proc_node.block.exitlabel = @exit_symbol
proc_node.block.inistmt = 0
proc_node.block.endstmt = 3
proc_node.block.proc.ismain = TRUE
scope_node.class = AST_NODECLASS_SCOPEBEGIN
scope_node.sym = @scope_symbol
scope_node.block.parent = @proc_node
scope_node.block.inistmt = 1
scope_node.block.endstmt = 2
break_node.class = AST_NODECLASS_SCOPE_BREAK
break_node.sym = @exit_symbol
break_node.break.parent = @scope_node
break_node.break.scope = 2
break_node.break.stmtnum = 1
break_node.break.linenum = 1
ast.proc.curr = @proc_node

fbSemanticModelProcedureBlock(@proc_node)
fbSemanticModelScopeBlock(@scope_node)
break_node.break.semantic_identity = fbSemanticModelScopeExit(@break_node)
fbSemanticModelExitCleanup(@break_node, @variable_symbol, @scope_symbol, @action_node)
fbSemanticModelCleanupNode(@action_node, 42)
fbSemanticModelScopeExitEnd(@break_node)
fbSemanticModelFinishBlocks(TRUE)
if failures or break_node.break.semantic_identity <> 1 or break_node.break.semantic_cleanup_count <> 1 then end 1
if instr(captured, "native-block-counts:1" + chr(9) + "1" + chr(9) + "1" + chr(9) + "1" + chr(9) + "1") = 0 then end 1
if instr(captured, "native-scope-exit-end:1" + chr(9) + "1") = 0 then end 1
if action_node.semantic_exit_cleanup <> 1 then end 1
if instr(captured, "native-block-provenance-counts:1" + chr(9) + "2" + chr(9) + "1") = 0 then end 1
if proc_node.block.endstmt <> 3 or break_node.sym <> @exit_symbol then end 1
dim as ASTNODE copied_action
astCopy(@copied_action, @action_node)
if copied_action.semantic_exit_cleanup <> 1 then end 1
astInitNode(@copied_action, AST_NODECLASS_NOP, FB_DATATYPE_INVALID, NULL)
if copied_action.semantic_exit_cleanup <> 0 then end 1

fresh( )
break_node.break.semantic_identity = fbSemanticModelScopeExit(@break_node)
fbSemanticModelExitCleanup(@break_node, @variable_symbol, @scope_symbol)
fbSemanticModelScopeExitEnd(@break_node)
fbSemanticModelFinishBlocks(TRUE)
if failures or instr(captured, "native-cleanup-insertion:1" + chr(9) + "0") = 0 then end 6
if instr(captured, "native-block-provenance-counts:1" + chr(9) + "0" + chr(9) + "0") = 0 then end 6

fresh( )
fbSemanticModelProcedureBlock(NULL)
fbSemanticModelProcedureBlock(@scope_node)
fbSemanticModelScopeBlock(@proc_node)
if fbSemanticModelScopeExit(NULL) then end 2
fbSemanticModelExitCleanup(NULL, @variable_symbol, @scope_symbol)
fbSemanticModelScopeExitEnd(NULL)
fbSemanticModelCleanupNode(NULL, 42)
if failures <> 7 or len(captured) or identity_queries then end 2

fresh( )
proc_node.block.endstmt = -1
fbSemanticModelProcedureBlock(@proc_node)
proc_node.block.endstmt = 3
scope_node.block.parent = NULL
fbSemanticModelScopeBlock(@scope_node)
scope_node.block.parent = @proc_node
break_node.break.semantic_identity = 0
fbSemanticModelExitCleanup(@break_node, @variable_symbol, @scope_symbol)
fbSemanticModelScopeExitEnd(@break_node)
if failures <> 4 or len(captured) or identity_queries then end 3

fresh( )
fbSemanticModelProcedureBlock(@proc_node)
dim as string previous_capture = captured
fbSemanticModelFinishBlocks(FALSE)
if captured <> previous_capture or failures then end 4

fresh( )
observing = FALSE
fbSemanticModelProcedureBlock(NULL)
fbSemanticModelScopeBlock(NULL)
if fbSemanticModelScopeExit(NULL) then end 5
fbSemanticModelExitCleanup(NULL, NULL, NULL)
fbSemanticModelScopeExitEnd(NULL)
fbSemanticModelCleanupNode(NULL, 42)
fbSemanticModelFinishBlocks(TRUE)
if failures or len(captured) or identity_queries then end 5
print "BLOCK_GUARDS_PASS"

'' end of native-block-guards.bas
