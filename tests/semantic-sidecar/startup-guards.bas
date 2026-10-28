'' Project: FreeBASIC semantic sidecar tests
'' File: startup-guards.bas
'' Purpose: Exercise production lifecycle capture boundaries without a parser.
'' Responsibilities: Class/range checks, complete counters, reset and inactive hooks.
'' This file intentionally does NOT register constructors or execute cleanup.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-startup.bi"

dim shared as SYMBCTX symb
dim shared as FBSYMBOL variable_symbol, parent_symbol, wrapper_symbol, guard_symbol, exit_symbol
dim shared as FB_PROCEXT parent_ext, wrapper_ext
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

function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	identity_queries += 1
	if sym = NULL then return 0
	if sym = @symb.globnspc then return 1
	if sym = @variable_symbol then return 2
	if sym = @parent_symbol then return 3
	if sym = @wrapper_symbol then return 4
	if sym = @guard_symbol then return 5
	if sym = @exit_symbol then return 6
	end 10
end function

private sub fresh( )
	fbSemanticModelResetStartup( )
	captured = ""
	failures = 0
	identity_queries = 0
end sub

variable_symbol.class = FB_SYMBCLASS_VAR
parent_symbol.class = FB_SYMBCLASS_PROC
parent_symbol.proc.ext = @parent_ext
parent_ext.priority = 101
wrapper_symbol.class = FB_SYMBCLASS_PROC
wrapper_symbol.proc.ext = @wrapper_ext
guard_symbol.class = FB_SYMBCLASS_VAR
exit_symbol.class = FB_SYMBCLASS_LABEL

fbSemanticModelStartupRegistration(@parent_symbol, "constructor")
fbSemanticModelStartupPriority(@parent_symbol, 101)
fbSemanticModelDeclarationOrder(2, 3, 1, 1, 1, 1)
dim as longint global_id = fbSemanticModelGlobalInstance(@variable_symbol, TRUE, TRUE)
dim as longint static_id = fbSemanticModelStaticInstance(@variable_symbol, @parent_symbol, @wrapper_symbol, TRUE, TRUE, TRUE, "initialization")
fbSemanticModelStaticGuard(static_id, @guard_symbol, @exit_symbol)
fbSemanticModelStartupWrapper(@parent_symbol, "constructor")
fbSemanticModelGlobalInstanceAction(global_id, @parent_symbol, "initialize", 1)
fbSemanticModelStaticCleanupBody(@variable_symbol, @parent_symbol, @wrapper_symbol)
fbSemanticModelStartupEmission(@parent_symbol, "constructor", "llvm", "array-entry", "101", "llvm.global_ctors", "i32 101, void ()* @example")
fbSemanticModelFinishStartup(TRUE)
if failures or global_id <> 1 or static_id <> 1 then end 1
if instr(captured, "startup-lifecycle-counts:1" + chr(9) + "1" + chr(9) + "1" + chr(9) + "1" + chr(9) + "1" + chr(9) + "1" + chr(9) + "1" + chr(9) + "1" + chr(9) + "1" + chr(9) + "1") = 0 then end 1
if parent_ext.priority <> 101 then end 1
if instr(captured, "accepted-declarator-count:1" + chr(9) + "1") = 0 then end 1

fresh( )
fbSemanticModelStartupRegistration(NULL, "constructor")
fbSemanticModelStartupRegistration(@variable_symbol, "constructor")
fbSemanticModelStartupRegistration(@parent_symbol, "pretend")
fbSemanticModelStartupPriority(@parent_symbol, -1)
fbSemanticModelStartupPriority(@parent_symbol, 65536)
if fbSemanticModelGlobalInstance(@parent_symbol, TRUE, TRUE) then end 2
if fbSemanticModelStaticInstance(@variable_symbol, @parent_symbol, NULL, TRUE, FALSE, FALSE, "pretend") then end 2
fbSemanticModelStaticGuard(1, @guard_symbol, @exit_symbol)
fbSemanticModelGlobalInstanceAction(1, @parent_symbol, "initialize", 1)
fbSemanticModelStaticCleanupBody(@parent_symbol, @parent_symbol, @wrapper_symbol)
if failures <> 10 or len(captured) then end 2

fresh( )
fbSemanticModelStartupEmission(@parent_symbol, "constructor", "pretend", "array-entry", "", "x", "x")
fbSemanticModelStartupEmission(@parent_symbol, "constructor", "llvm", "section-entry", "", "x", "x")
fbSemanticModelStartupEmission(@parent_symbol, "constructor", "llvm", "array-entry", "", "", "x")
fbSemanticModelStartupEmission(@parent_symbol, "constructor", "llvm", "array-entry", "", "x", "")
dim as string excessive = string(65537, "x")
fbSemanticModelStartupEmission(@parent_symbol, "constructor", "llvm", "array-entry", "", "x", excessive)
if failures <> 5 or len(captured) then end 3

fresh( )
global_id = fbSemanticModelGlobalInstance(@variable_symbol, TRUE, FALSE)
dim as string previous_capture = captured
fbSemanticModelFinishStartup(FALSE)
if failures or captured <> previous_capture then end 4
captured = ""
global_id = fbSemanticModelGlobalInstance(@variable_symbol, FALSE, TRUE)
if global_id <> 1 or instr(captured, "global-instance:1" + chr(9)) = 0 then end 4

fresh( )
observing = FALSE
fbSemanticModelStartupRegistration(NULL, "pretend")
fbSemanticModelStartupPriority(NULL, -1)
fbSemanticModelDeclarationOrder(-1, -1, -1, -1, -1, -1)
if fbSemanticModelGlobalInstance(NULL, TRUE, TRUE) then end 5
if fbSemanticModelStaticInstance(NULL, NULL, NULL, TRUE, TRUE, TRUE, "pretend") then end 5
fbSemanticModelStaticGuard(-1, NULL, NULL)
fbSemanticModelStartupWrapper(NULL, "pretend")
fbSemanticModelGlobalInstanceAction(-1, NULL, "pretend", -1)
fbSemanticModelStaticCleanupBody(NULL, NULL, NULL)
fbSemanticModelStartupEmission(NULL, "pretend", "pretend", "pretend", "", "", "")
fbSemanticModelFinishStartup(TRUE)
if failures or len(captured) or identity_queries then end 5
print "STARTUP_GUARDS_PASS"

'' end of startup-guards.bas
