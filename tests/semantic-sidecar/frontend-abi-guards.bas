'' Project: FreeBASIC semantic sidecar tests
'' File: frontend-abi-guards.bas
'' Purpose: Prove incomplete types and variadic markers bypass unsafe ABI queries.
'' Responsibilities: Link the production observer to checked test boundaries.
'' This file intentionally does NOT implement compiler ABI classification.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-abi.bi"

dim shared as SYMBCTX symb
dim shared as FBSYMBOL procedure_symbol, parameter_symbol, unfinished_type, forward_type
dim shared as integer observing = TRUE, lowering_calls
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
	end 10
end sub

function fbSemanticModelModuleIdentity( ) as longint
	return 1
end function

function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	if sym = NULL then return 0
	if sym = @symb.globnspc then return 1
	if sym = @procedure_symbol then return 2
	if sym = @parameter_symbol then return 3
	if sym = @unfinished_type then return 4
	if sym = @forward_type then return 5
	end 11
end function

function symbProcReturnsOnStack( byval proc as FBSYMBOL ptr ) as integer
	'' Every result in this guard fixture is unresolved or explicitly BYREF.
	'' Entering the production classifier would violate its preconditions.
	end 12
end function

sub symbGetRealParamDtype overload( byval param as FBSYMBOL ptr, byref dtype as integer, byref subtype as FBSYMBOL ptr )
	if param->param.mode <> FB_PARAMMODE_BYREF then end 13
	lowering_calls += 1
	dtype = typeAddrOf(param->typ)
	subtype = param->subtype
end sub

unfinished_type.class = FB_SYMBCLASS_STRUCT
unfinished_type.udt.retdtype = FB_DATATYPE_INVALID
'' Poison the inactive classifier. The observer must inspect only retdtype.
unfinished_type.udt.retin2regs = 1234567
forward_type.class = FB_SYMBCLASS_FWDREF
procedure_symbol.class = FB_SYMBCLASS_PROC
procedure_symbol.typ = FB_DATATYPE_STRUCT
procedure_symbol.subtype = @unfinished_type
procedure_symbol.proc.realdtype = FB_DATATYPE_STRUCT
procedure_symbol.proc.realsubtype = @unfinished_type
parameter_symbol.class = FB_SYMBCLASS_PARAM
parameter_symbol.typ = FB_DATATYPE_STRUCT
parameter_symbol.subtype = @unfinished_type
parameter_symbol.param.mode = FB_PARAMMODE_BYVAL

fbSemanticModelProcedureAbi(@procedure_symbol, 2)
fbSemanticModelParameterAbi(@procedure_symbol, @parameter_symbol, 3, 0, FALSE)
if instr(captured, "pending-layout") = 0 or lowering_calls then end 1

captured = ""
parameter_symbol.typ = FB_DATATYPE_FWDREF
parameter_symbol.subtype = @forward_type
fbSemanticModelParameterAbi(@procedure_symbol, @parameter_symbol, 3, 0, FALSE)
if instr(captured, "pending-type") = 0 or lowering_calls then end 2

captured = ""
parameter_symbol.param.mode = FB_PARAMMODE_BYDESC
parameter_symbol.param.bydescrealsubtype = NULL
fbSemanticModelParameterAbi(@procedure_symbol, @parameter_symbol, 3, 0, FALSE)
if instr(captured, "pending-descriptor") = 0 or lowering_calls then end 3

captured = ""
parameter_symbol.param.mode = FB_PARAMMODE_VARARG
parameter_symbol.typ = FB_DATATYPE_INVALID
parameter_symbol.subtype = NULL
fbSemanticModelParameterAbi(@procedure_symbol, @parameter_symbol, 3, 0, FALSE)
if instr(captured, "variadic") = 0 or lowering_calls then end 4

captured = ""
parameter_symbol.param.mode = FB_PARAMMODE_BYVAL
fbSemanticModelParameterAbi(@procedure_symbol, @parameter_symbol, 3, 0, FALSE)
if instr(captured, "unavailable") = 0 or lowering_calls then end 8

captured = ""
procedure_symbol.pattrib = FB_PROCATTRIB_RETURNBYREF
procedure_symbol.proc.realdtype = typeAddrOf(FB_DATATYPE_STRUCT)
fbSemanticModelProcedureAbi(@procedure_symbol, 2)
if instr(captured, chr(9) + "byref") = 0 then end 5
parameter_symbol.param.mode = FB_PARAMMODE_BYREF
parameter_symbol.typ = FB_DATATYPE_STRUCT
parameter_symbol.subtype = @unfinished_type
fbSemanticModelParameterAbi(@procedure_symbol, @parameter_symbol, 3, 0, FALSE)
if lowering_calls <> 1 then end 6

captured = ""
observing = FALSE
fbSemanticModelProcedureAbi(NULL, 0)
fbSemanticModelParameterAbi(NULL, NULL, 0, 0, FALSE)
if len(captured) or lowering_calls <> 1 then end 7
fbSemanticModelFinishAbi(FALSE)
print "FRONTEND_ABI_GUARDS_PASS"

'' end of frontend-abi-guards.bas
