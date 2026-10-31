'' Project: FreeBASIC semantic sidecar tests
'' File: call-abi-guards.bas
'' Purpose: Prove selected-call observation rejects invalid state and copies context.
'' Responsibilities: Production entry points, limits, closure, reset and inactive hooks.
'' This file intentionally does NOT implement ABI lowering or export publication.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-call-abi.bi"

dim shared as SYMBCTX symb
dim shared as FBENV env
dim shared as FBSYMBOL procedure_symbol, parameter_symbol, static_target
dim shared as ASTNODE call_node
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
	if sym = @static_target then return 4
	end 10
end function

sub symbGetRealParamDtype overload _
	(byval param as FBSYMBOL ptr, byref dtype as integer, byref subtype as FBSYMBOL ptr)
	'' This fixture supplies only by-value scalar formals. Copy their declared
	'' types; other parameter modes require the real compiler's ABI lowering.
	if param = NULL orelse param->class <> FB_SYMBCLASS_PARAM then end 11
	if param->param.mode <> FB_PARAMMODE_BYVAL then end 11
	dtype = symbGetFullType(param)
	subtype = symbGetSubtype(param)
end sub

private sub fresh( )
	fbSemanticModelResetCallAbi( )
	captured = ""
	failures = 0
end sub

procedure_symbol.class = FB_SYMBCLASS_PROC
static_target.class = FB_SYMBCLASS_PROC
parameter_symbol.class = FB_SYMBCLASS_PARAM
parameter_symbol.param.mode = FB_PARAMMODE_BYVAL
call_node.class = AST_NODECLASS_CALL
env.clopt.backend = FB_BACKEND_LLVM
dim as string excessive = string(65537, "x")

'' Context is copied, and caller AST storage can change immediately afterward.
call_node.semantic_expression = 11
call_node.semantic_statement = 12
call_node.semantic_source = 13
call_node.call.semantic_target = @static_target
fbSemanticModelCallAbiContext(@call_node, @procedure_symbol)
call_node.semantic_expression = 99
call_node.semantic_statement = 99
call_node.semantic_source = 99
call_node.call.semantic_target = NULL
dim as longint identity = fbSemanticModelCallAbiBegin(@procedure_symbol, "llvm", TRUE, TRUE, _
	FB_DATATYPE_LONG, NULL, "i32", "", 1, 0)
if identity <> 1 or failures then end 1
if instr(captured, "11" + chr(9) + "12" + chr(9) + "13" + chr(9) + "4") = 0 then end 1

'' Reject nested formatters without advancing the current identity or output.
dim as string previous_capture = captured
if fbSemanticModelCallAbiBegin(@procedure_symbol, "llvm", FALSE, FALSE, FB_DATATYPE_VOID, NULL, "void", "", 1, 0) then end 2
if failures <> 1 or captured <> previous_capture then end 2
failures = 0
fbSemanticModelCallAbiArgument(999, @parameter_symbol, 0, 1, "value", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, "i32", "", "1")
fbSemanticModelCallAbiArgument(identity, @procedure_symbol, 0, 1, "value", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, "i32", "", "1")
fbSemanticModelCallAbiArgument(identity, NULL, 0, 1, "value", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, "i32", "", "1")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 1, 1, "value", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, "i32", "", "1")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "pretend", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, "i32", "", "1")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "value", FB_DATATYPE_INVALID, NULL, FB_DATATYPE_LONG, NULL, "i32", "", "1")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "value", FB_DATATYPE_LONG, NULL, FB_DATATYPE_INVALID, NULL, "i32", "", "1")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "value", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, "i32", "", "")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "value", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, "i32", "", excessive)
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "value", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, excessive, "", "1")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "value", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, "i32", " invented", "1")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 2, "i386-int128-high", FB_DATATYPE_INTEGER128, NULL, FB_DATATYPE_ULONGINT, NULL, "i64", "", "%high")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "libc-adjusted", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, "i32", "", "1")
if failures <> 13 or captured <> previous_capture then end 3
failures = 0
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "value", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, "i32", "", "1")
fbSemanticModelCallAbiEnd(identity, "%r = call i32 %fn(i32 1)")
if failures then end 4
previous_capture = captured
fbSemanticModelCallAbiEnd(identity, "%r = call i32 %fn(i32 1)")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "value", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, "i32", "", "1")
if failures <> 2 or captured <> previous_capture then end 4

fresh( )
if fbSemanticModelCallAbiBegin(NULL, "llvm", FALSE, FALSE, FB_DATATYPE_VOID, NULL, "void", "", 0, 0) then end 5
if fbSemanticModelCallAbiBegin(@parameter_symbol, "llvm", FALSE, FALSE, FB_DATATYPE_VOID, NULL, "void", "", 0, 0) then end 5
if fbSemanticModelCallAbiBegin(@procedure_symbol, "pretend", FALSE, FALSE, FB_DATATYPE_VOID, NULL, "void", "", 0, 0) then end 5
if fbSemanticModelCallAbiBegin(@procedure_symbol, "llvm", FALSE, FALSE, FB_DATATYPE_VOID, NULL, "void", "", -1, 0) then end 5
if fbSemanticModelCallAbiBegin(@procedure_symbol, "llvm", FALSE, FALSE, FB_DATATYPE_VOID, NULL, "void", "", 0, -1) then end 5
if fbSemanticModelCallAbiBegin(@procedure_symbol, "llvm", FALSE, FALSE, FB_DATATYPE_VOID, NULL, excessive, "", 0, 0) then end 5
if fbSemanticModelCallAbiBegin(@procedure_symbol, "c", FALSE, FALSE, FB_DATATYPE_INVALID, NULL, "", "", 0, 0) then end 5
if fbSemanticModelCallAbiBegin(@procedure_symbol, "llvm", FALSE, FALSE, FB_DATATYPE_VOID, NULL, "", "", 0, 0) then end 5
if fbSemanticModelCallAbiBegin(@procedure_symbol, "llvm", FALSE, FALSE, FB_DATATYPE_INVALID, NULL, "void", "", 0, 0) then end 5
if fbSemanticModelCallAbiBegin(@procedure_symbol, "llvm", FALSE, FALSE, FB_DATATYPE_VOID, NULL, "void", "", 1000001, 0) then end 5
if failures <> 10 or len(captured) then end 5
identity = fbSemanticModelCallAbiBegin(@procedure_symbol, "llvm", FALSE, FALSE, FB_DATATYPE_VOID, NULL, "void", "", 0, 0)
if identity <> 1 then end 5
previous_capture = captured
failures = 0
fbSemanticModelCallAbiEnd(identity, "")
fbSemanticModelCallAbiEnd(identity, excessive)
if failures <> 2 or captured <> previous_capture then end 6
failures = 0
fbSemanticModelFinishCallAbi(TRUE)
if failures <> 1 then end 6

fresh( )
fbSemanticModelCallAbiBuiltin(NULL, 0)
fbSemanticModelCallAbiBuiltin(@parameter_symbol, 0)
fbSemanticModelCallAbiBuiltin(@procedure_symbol, -1)
fbSemanticModelCallAbiBuiltin(@procedure_symbol, 1000001)
if failures <> 4 or len(captured) then end 9
failures = 0
fbSemanticModelCallAbiBuiltin(@procedure_symbol, 1)
fbSemanticModelFinishCallAbi(TRUE)
if failures or instr(captured, "emitted-abi-builtin-count:1" + chr(9) + "1") = 0 then end 9

fresh( )
env.clopt.backend = FB_BACKEND_GCC
identity = fbSemanticModelCallAbiBegin(@procedure_symbol, "c", FALSE, FALSE, FB_DATATYPE_INVALID, NULL, "", "", 0, 0)
previous_capture = captured
fbSemanticModelFinishCallAbi(FALSE)
if failures or captured <> previous_capture then end 7
identity = fbSemanticModelCallAbiBegin(@procedure_symbol, "c", FALSE, FALSE, FB_DATATYPE_INVALID, NULL, "", "", 0, 0)
if identity <> 1 then end 7
fbSemanticModelCallAbiEnd(identity, "example()")
fbSemanticModelFinishCallAbi(TRUE)
if failures or instr(captured, "emitted-abi-call-counts:1" + chr(9) + "1" + chr(9) + "0") = 0 then end 7

fresh( )
identity = fbSemanticModelCallAbiBegin(@procedure_symbol, "c", FALSE, FALSE, FB_DATATYPE_INVALID, NULL, "", "", 0, 0)
previous_capture = captured
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "graphics-blender", FB_DATATYPE_LONG, NULL, FB_DATATYPE_LONG, NULL, "", "", "callback")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "graphics-blender", FB_DATATYPE_LONG, NULL, typeAddrOf(FB_DATATYPE_FUNCTION), NULL, "", "", "callback")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "graphics-blender", FB_DATATYPE_LONG, NULL, typeAddrOf(FB_DATATYPE_FUNCTION), @parameter_symbol, "", "", "callback")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "graphics-blender-context", FB_DATATYPE_LONG, NULL, typeAddrOf(FB_DATATYPE_VOID), NULL, "", "", "context")
parameter_symbol.param.mode = FB_PARAMMODE_BYREF
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "graphics-blender-context", FB_DATATYPE_LONG, NULL, FB_DATATYPE_INVALID, NULL, "", "", "context")
parameter_symbol.param.mode = FB_PARAMMODE_BYVAL
if failures <> 5 or captured <> previous_capture then end 12
failures = 0
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "graphics-blender", FB_DATATYPE_LONG, NULL, typeAddrOf(FB_DATATYPE_FUNCTION), @static_target, "", "", "callback")
fbSemanticModelCallAbiArgument(identity, @parameter_symbol, 0, 1, "graphics-blender-context", FB_DATATYPE_LONG, NULL, FB_DATATYPE_INVALID, NULL, "", "", "context")
fbSemanticModelCallAbiEnd(identity, "graphics(callback, context)")
fbSemanticModelFinishCallAbi(TRUE)
if failures then end 12

fresh( )
observing = FALSE
fbSemanticModelCallAbiContext(@call_node, @parameter_symbol)
if fbSemanticModelCallAbiBegin(NULL, "pretend", FALSE, FALSE, FB_DATATYPE_INVALID, NULL, "", "", -1, -1) then end 8
fbSemanticModelCallAbiArgument(999, NULL, -1, -1, "pretend", FB_DATATYPE_INVALID, NULL, FB_DATATYPE_INVALID, NULL, "", "", "")
fbSemanticModelCallAbiEnd(999, "")
fbSemanticModelCallAbiBuiltin(NULL, -1)
fbSemanticModelFinishCallAbi(TRUE)
if failures or len(captured) then end 8
observing = TRUE
env.clopt.backend = FB_BACKEND_GAS64
if fbSemanticModelCallAbiBegin(NULL, "pretend", FALSE, FALSE, FB_DATATYPE_INVALID, NULL, "", "", -1, -1) then end 8
fbSemanticModelFinishCallAbi(TRUE)
if failures or len(captured) then end 8
print "CALL_ABI_GUARDS_PASS"

'' end of call-abi-guards.bas
