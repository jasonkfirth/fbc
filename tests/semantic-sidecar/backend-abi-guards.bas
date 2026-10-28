'' Project: FreeBASIC semantic sidecar tests
'' File: backend-abi-guards.bas
'' Purpose: Exercise bounded backend observations through their production entry points.
'' Responsibilities: Rejection, nested signature identity, reset and inactive hooks.
'' This file intentionally does NOT implement ABI lowering or a replacement hash table.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-backend-abi.bi"

dim shared as SYMBCTX symb
dim shared as FBENV env
dim shared as FBSYMBOL procedure_symbol, parameter_symbol
dim shared as integer observing = TRUE, failures
dim shared as string captured

function fbSemanticModelFullEnabled( ) as integer
	return observing
end function

function fbSemanticModelNumber( byval value as longint ) as string
	return ltrim(str(value))
end function

function fbSemanticModelTypeNumber( byval value as longint ) as string
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
	if sym = NULL then return 0
	if sym = @symb.globnspc then return 1
	if sym = @procedure_symbol then return 2
	if sym = @parameter_symbol then return 3
	end 10
end function

private sub fresh( )
	fbSemanticModelResetBackendAbi( )
	captured = ""
	failures = 0
end sub

procedure_symbol.class = FB_SYMBCLASS_PROC
parameter_symbol.class = FB_SYMBCLASS_PARAM
env.clopt.backend = FB_BACKEND_LLVM

'' Each rejected begin must leave the next signature identity untouched.
dim as string excessive = string(65537, "x")
if fbSemanticModelBackendAbiBegin(NULL, "llvm", "definition", "void", "") then end 1
if fbSemanticModelBackendAbiBegin(@parameter_symbol, "llvm", "definition", "void", "") then end 1
if fbSemanticModelBackendAbiBegin(@procedure_symbol, "pretend", "definition", "void", "") then end 1
if fbSemanticModelBackendAbiBegin(@procedure_symbol, "llvm", "pretend", "void", "") then end 1
if fbSemanticModelBackendAbiBegin(@procedure_symbol, "llvm", "definition", "", "") then end 1
if fbSemanticModelBackendAbiBegin(@procedure_symbol, "llvm", "definition", excessive, "") then end 1
if fbSemanticModelBackendAbiBegin(@procedure_symbol, "llvm", "definition", "void", excessive) then end 1
if failures <> 7 or len(captured) then end 1
dim as longint outer_id = fbSemanticModelBackendAbiBegin(@procedure_symbol, "llvm", "definition", "void", "")
if outer_id <> 1 then end 2

'' Parameter errors must not consume a component position or create a fact.
dim as string previous_capture = captured
failures = 0
fbSemanticModelBackendAbiParameter(999, @parameter_symbol, 0, 1, "value", "i32", FB_DATATYPE_LONG)
fbSemanticModelBackendAbiParameter(outer_id, @procedure_symbol, 0, 1, "value", "i32", FB_DATATYPE_LONG)
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "pretend", "i32", FB_DATATYPE_LONG)
fbSemanticModelBackendAbiParameter(outer_id, NULL, 0, 1, "value", "i32", FB_DATATYPE_LONG)
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 1, 1, "value", "i32", FB_DATATYPE_LONG)
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 2, "value", "i32", FB_DATATYPE_LONG)
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "value", "i32")
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "value", "i32", FB_DATATYPE_LONG, NULL, " invented")
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "value", excessive, FB_DATATYPE_LONG)
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 0, "variadic", "...", FB_DATATYPE_LONG)
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "hidden-result", "i32*", typeAddrOf(FB_DATATYPE_LONG))
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 2, "i386-int128-high", "i64", FB_DATATYPE_ULONGINT)
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "libc-adjusted", "i32", FB_DATATYPE_LONG)
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "argv", "char**")
if failures <> 14 or captured <> previous_capture then end 3

'' Nested pointer-type formatting can complete while its enclosing frame is open.
failures = 0
dim as longint nested_id = fbSemanticModelBackendAbiBegin(@procedure_symbol, "llvm", "pointer-type", "void", "")
fbSemanticModelBackendAbiParameter(nested_id, @parameter_symbol, 0, 0, "variadic", "...")
fbSemanticModelBackendAbiEnd(nested_id, "void (...) ")
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "value", "i32", FB_DATATYPE_LONG)
fbSemanticModelBackendAbiEnd(outer_id, "void @example(i32)")
if failures or nested_id <> 2 then end 4
fbSemanticModelFinishBackendAbi(TRUE)
if failures or instr(captured, "emitted-abi-counts:1" + chr(9) + "2" + chr(9) + "2") = 0 then end 4

fresh( )
outer_id = fbSemanticModelBackendAbiBegin(@procedure_symbol, "c", "declaration", "void", "")
previous_capture = captured
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "memory-byval", "UDT*", typeAddrOf(FB_DATATYPE_STRUCT))
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "va-list-decay", "UDT*", typeAddrOf(FB_DATATYPE_STRUCT))
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "argv", "char*")
fbSemanticModelBackendAbiEnd(outer_id, "")
fbSemanticModelBackendAbiEnd(outer_id, excessive)
if failures <> 5 or captured <> previous_capture then end 5
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "argv", "char**")
fbSemanticModelBackendAbiEnd(outer_id, "void example(char**)")
previous_capture = captured
failures = 0
fbSemanticModelBackendAbiEnd(outer_id, "void example(char**)")
fbSemanticModelBackendAbiParameter(outer_id, @parameter_symbol, 0, 1, "argv", "char**")
if failures <> 2 or captured <> previous_capture then end 6

'' An open signature poisons commit, while rollback releases it silently.
fresh( )
outer_id = fbSemanticModelBackendAbiBegin(@procedure_symbol, "llvm", "definition", "void", "")
fbSemanticModelFinishBackendAbi(TRUE)
if failures <> 1 then end 7
fresh( )
outer_id = fbSemanticModelBackendAbiBegin(@procedure_symbol, "llvm", "definition", "void", "")
previous_capture = captured
fbSemanticModelFinishBackendAbi(FALSE)
if failures or captured <> previous_capture then end 7
outer_id = fbSemanticModelBackendAbiBegin(@procedure_symbol, "llvm", "definition", "void", "")
if outer_id <> 1 then end 7

fresh( )
observing = FALSE
if fbSemanticModelBackendAbiBegin(NULL, "pretend", "pretend", "", "") then end 8
fbSemanticModelBackendAbiParameter(999, NULL, -1, -1, "pretend", "")
fbSemanticModelBackendAbiEnd(999, "")
fbSemanticModelFinishBackendAbi(TRUE)
if failures or len(captured) then end 8
print "BACKEND_ABI_GUARDS_PASS"

'' end of backend-abi-guards.bas
