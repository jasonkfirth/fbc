'' Project: FreeBASIC semantic sidecar tests
'' File: temporary-lifetime-guards.bas
'' Purpose: Exercise production temporary lifetime events without a parser.
'' Responsibilities: Reference/recycle/flush transitions, bounds and inactive hooks.
'' This file intentionally does NOT build or execute destructor calls.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-temporaries.bi"

dim shared as SYMBCTX symb
dim shared as ASTCTX ast
dim shared as FBSYMBOL variable_symbol, proc_symbol
dim shared as ASTNODE proc_node, cleanup_node
dim shared as AST_DTORLIST_ITEM item
dim shared as integer observing = TRUE, failures, identity_queries
dim shared as integer metadata_queries
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
	if sym = @variable_symbol then return 3
	end 10
end function

sub fbSemanticModelExportSymbolDetails( byval sym as FBSYMBOL ptr, byval variables_live as integer )
	if sym <> @variable_symbol or variables_live = FALSE then end 10
	metadata_queries += 1
end sub

private sub fresh( )
	fbSemanticModelResetTemporaries( )
	failures = 0
	identity_queries = 0
	metadata_queries = 0
	captured = ""
end sub

variable_symbol.class = FB_SYMBCLASS_VAR
proc_symbol.class = FB_SYMBCLASS_PROC
proc_node.class = AST_NODECLASS_PROC
proc_node.sym = @proc_symbol
ast.proc.curr = @proc_node
item.sym = @variable_symbol
item.semantic_generation = 1
item.cookie = 1
item.refcount = 0
fbSemanticModelResetTemporaries( )
fbSemanticModelTemporaryScopeBegin(1, TRUE, 1)
fbSemanticModelTemporaryAdd(@item)
item.refcount = 1
fbSemanticModelTemporaryReference(@item, TRUE)
item.refcount = 0
fbSemanticModelTemporaryReference(@item, FALSE)
fbSemanticModelTemporaryRemove(@item, "reference-release")
fbSemanticModelTemporaryScopeEnd(1, 0)
dim as longint first_flush = fbSemanticModelTemporaryFlushBegin(0)
fbSemanticModelTemporaryFlushEnd(first_flush, 0, 0, 0)
fbSemanticModelTemporaryScopeBegin(1, TRUE, 1)
item.semantic_generation = 2
fbSemanticModelTemporaryAdd(@item)
fbSemanticModelTemporaryScopeEnd(1, 0)
dim as longint second_flush = fbSemanticModelTemporaryFlushBegin(1)
fbSemanticModelTemporaryFlushAction(second_flush, @item, @cleanup_node)
fbSemanticModelTemporaryNode(@cleanup_node, 42)
fbSemanticModelTemporaryRemove(@item, "flush")
fbSemanticModelTemporaryFlushEnd(second_flush, 1, 1, 1)
fbSemanticModelTemporaryConditional(@variable_symbol, second_flush, 0)
fbSemanticModelFinishTemporaries(TRUE)
if failures or first_flush <> 7 or second_flush <> 12 or metadata_queries <> 2 then end 1
if instr(captured, "native-temp-counts:1" + chr(9) + "16" + chr(9) + "2" + chr(9) + "2" + chr(9) + "1" + chr(9) + "0" + chr(9) + "0") = 0 then end 1
if instr(captured, "scope-begin" + chr(9) + "2" + chr(9) + "1") = 0 then end 1
if cleanup_node.semantic_temporary_cleanup <> 1 then end 1
dim as ASTNODE cloned_cleanup
astCopy(@cloned_cleanup, @cleanup_node)
if cloned_cleanup.semantic_temporary_cleanup <> 1 then end 1
astInitNode(@cloned_cleanup, AST_NODECLASS_NOP, FB_DATATYPE_INVALID, NULL)
if cloned_cleanup.semantic_temporary_cleanup then end 1

fresh( )
fbSemanticModelTemporaryAdd(NULL)
item.cookie = -2
fbSemanticModelTemporaryAdd(@item)
item.cookie = 1
item.semantic_generation = 0
fbSemanticModelTemporaryAdd(@item)
item.semantic_generation = 2
fbSemanticModelTemporaryScopeBegin(0, TRUE, 1)
fbSemanticModelTemporaryScopeEnd(1, 0)
if fbSemanticModelTemporaryFlushBegin(-1) then end 2
fbSemanticModelTemporaryFlushAction(0, @item, NULL)
fbSemanticModelTemporaryFlushEnd(1, 0, 2, 1)
fbSemanticModelTemporaryRemove(@item, "guessed")
fbSemanticModelTemporaryRemove(@item, "explicit-delete")
fbSemanticModelTemporaryConditional(NULL, 0, 0)
if failures <> 11 or len(captured) or identity_queries then end 2

fresh( )
dim as longint empty_flush = fbSemanticModelTemporaryFlushBegin(0)
dim as string previous_capture = captured
fbSemanticModelFinishTemporaries(FALSE)
if failures or previous_capture <> captured then end 3

fresh( )
observing = FALSE
fbSemanticModelTemporaryAdd(NULL)
fbSemanticModelTemporaryReference(NULL, TRUE)
fbSemanticModelTemporaryRescope(NULL, -2)
fbSemanticModelTemporaryRemove(NULL, "guessed")
fbSemanticModelTemporaryScopeBegin(-1, FALSE, -1)
fbSemanticModelTemporaryScopeEnd(-1, -1)
if fbSemanticModelTemporaryFlushBegin(-1) then end 4
fbSemanticModelTemporaryFlushAction(-1, NULL, NULL)
fbSemanticModelTemporaryNode(NULL, -1)
fbSemanticModelTemporaryFlushEnd(-1, -1, -1, -1)
fbSemanticModelTemporaryConditional(NULL, -1, -1)
fbSemanticModelFinishTemporaries(TRUE)
if failures or len(captured) or identity_queries or metadata_queries then end 4
print "TEMPORARY_GUARDS_PASS"

'' end of temporary-lifetime-guards.bas
