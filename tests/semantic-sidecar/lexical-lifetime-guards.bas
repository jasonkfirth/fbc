'' Project: FreeBASIC semantic sidecar tests
'' File: lexical-lifetime-guards.bas
'' Purpose: Exercise the production normal-teardown observer without a parser.
'' Responsibilities: Native class/range guards, inserted/elided roots and reset.
'' This file intentionally does NOT construct destructors or perform teardown.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-lifetimes.bi"

dim shared as SYMBCTX symb
dim shared as ASTCTX ast
dim shared as FBSYMBOL proc_symbol, scope_symbol, variable_one, variable_two
dim shared as ASTNODE proc_node, action_node
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

function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	identity_queries += 1
	if sym = @symb.globnspc then return 1
	if sym = @proc_symbol then return 2
	if sym = @scope_symbol then return 3
	if sym = @variable_one then return 4
	if sym = @variable_two then return 5
	end 10
end function

private sub fresh( )
	fbSemanticModelResetLifetimes( )
	failures = 0
	identity_queries = 0
	captured = ""
	action_node.semantic_lexical_cleanup = 0
	action_node.semantic_exit_cleanup = 0
end sub

proc_symbol.class = FB_SYMBCLASS_PROC
scope_symbol.class = FB_SYMBCLASS_SCOPE
variable_one.class = FB_SYMBCLASS_VAR
variable_two.class = FB_SYMBCLASS_VAR
proc_node.class = AST_NODECLASS_PROC
proc_node.sym = @proc_symbol
ast.proc.curr = @proc_node

dim as longint group_id = fbSemanticModelLexicalCleanupBegin(@scope_symbol)
fbSemanticModelLexicalCleanup(group_id, 1, @variable_one, @action_node)
fbSemanticModelLexicalCleanup(group_id, 2, @variable_two, NULL)
fbSemanticModelLexicalCleanupNode(@action_node, 42)
fbSemanticModelLexicalCleanupEnd(group_id, 2)
fbSemanticModelFinishLifetimes(TRUE)
if failures or group_id <> 1 or action_node.semantic_lexical_cleanup <> 1 then end 1
if instr(captured, "native-lexical-cleanup-counts:1" + chr(9) + "1" + chr(9) + "2" + chr(9) + "1") = 0 then end 1
if instr(captured, "native-lexical-cleanup:2" + chr(9) + "1" + chr(9) + "2" + chr(9) + "5" + chr(9) + "0") = 0 then end 1
dim as ASTNODE copied_action
astCopy(@copied_action, @action_node)
if copied_action.semantic_lexical_cleanup <> 1 then end 1
astInitNode(@copied_action, AST_NODECLASS_NOP, FB_DATATYPE_INVALID, NULL)
if copied_action.semantic_lexical_cleanup then end 1

fresh( )
if fbSemanticModelLexicalCleanupBegin(NULL) then end 2
if fbSemanticModelLexicalCleanupBegin(@variable_one) then end 2
ast.proc.curr = NULL
if fbSemanticModelLexicalCleanupBegin(@scope_symbol) then end 2
ast.proc.curr = @proc_node
fbSemanticModelLexicalCleanup(1, 1, @variable_one, NULL)
fbSemanticModelLexicalCleanupEnd(1, 0)
fbSemanticModelLexicalCleanupNode(NULL, 42)
if failures <> 6 or len(captured) or identity_queries then end 2

fresh( )
group_id = fbSemanticModelLexicalCleanupBegin(@proc_symbol)
captured = ""
fbSemanticModelLexicalCleanup(group_id, 0, @variable_one, NULL)
fbSemanticModelLexicalCleanup(group_id, 1, NULL, NULL)
fbSemanticModelLexicalCleanup(group_id, 1, @scope_symbol, NULL)
action_node.semantic_exit_cleanup = 1
fbSemanticModelLexicalCleanup(group_id, 1, @variable_one, @action_node)
fbSemanticModelLexicalCleanupEnd(group_id, -1)
fbSemanticModelLexicalCleanupEnd(group_id, 1000001)
action_node.semantic_lexical_cleanup = 1
fbSemanticModelLexicalCleanupNode(@action_node, 42)
if failures <> 7 or len(captured) then end 3

fresh( )
group_id = fbSemanticModelLexicalCleanupBegin(@scope_symbol)
dim as string previous_capture = captured
fbSemanticModelFinishLifetimes(FALSE)
if failures or captured <> previous_capture then end 4

fresh( )
observing = FALSE
if fbSemanticModelLexicalCleanupBegin(NULL) then end 5
fbSemanticModelLexicalCleanup(-1, -1, NULL, NULL)
fbSemanticModelLexicalCleanupEnd(-1, -1)
fbSemanticModelLexicalCleanupNode(NULL, -1)
fbSemanticModelFinishLifetimes(TRUE)
if failures or len(captured) or identity_queries then end 5
print "LEXICAL_GUARDS_PASS"

'' end of lexical-lifetime-guards.bas
