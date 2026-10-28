'' Project: FreeBASIC semantic sidecar tests
'' File: accepted-declaration-guards.bas
'' Purpose: Exercise production declaration guards without a running parser.
'' Responsibilities: Invalid inputs, reset, compact macros and inactive hooks.
'' This file intentionally does NOT resolve names or create source declarations.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-declarations.bi"
#include once "parser/parser.bi"

dim shared as SYMBCTX symb
dim shared as PARSERCTX parser
dim shared as FBSYMBOL selected
dim shared as integer observing = TRUE, macros = TRUE, failures, identity_queries
dim shared as string captured

function fbSemanticModelFullEnabled( ) as integer
	return observing
end function

function fbSemanticModelMacroProvenanceEnabled( ) as integer
	return macros
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
	if sym = @selected then return 2
	end 10
end function

selected.class = FB_SYMBCLASS_VAR
parser.stmt.cnt = 7
fbSemanticModelAcceptedDeclaration(@selected, "binding", 11)
fbSemanticModelAcceptedDeclaration(@selected, "declaration", 12)
fbSemanticModelAcceptedDeclaration(@selected, "macro-point", 13)
fbSemanticModelFinishDeclarations(TRUE)
if failures or instr(captured, "accepted-source-declaration-count:1" + chr(9) + "3") = 0 then end 1
if instr(captured, "binding" + chr(9) + "11" + chr(9) + "2" + chr(9) + "1" + chr(9) + "7" + chr(9) + "8" + chr(9) + "9" + chr(9) + "10") = 0 then end 2
if selected.class <> FB_SYMBCLASS_VAR or parser.stmt.cnt <> 7 then end 3
captured = ""
fbSemanticModelFinishDeclarations(TRUE)
if instr(captured, "accepted-source-declaration-count:1" + chr(9) + "0") = 0 then end 4

captured = ""
identity_queries = 0
fbSemanticModelAcceptedDeclaration(NULL, "binding", 1)
fbSemanticModelAcceptedDeclaration(@selected, "binding", 0)
fbSemanticModelAcceptedDeclaration(@selected, "binding", -1)
fbSemanticModelAcceptedDeclaration(@selected, "other", 1)
if failures <> 4 or identity_queries or len(captured) then end 5

macros = FALSE
fbSemanticModelAcceptedDeclaration(@selected, "macro-point", 1)
if failures <> 4 or identity_queries or len(captured) then end 6
observing = FALSE
fbSemanticModelAcceptedDeclaration(NULL, "other", 0)
fbSemanticModelFinishDeclarations(TRUE)
if failures <> 4 or identity_queries or len(captured) then end 7
print "DECLARATION_GUARDS_PASS"

'' end of accepted-declaration-guards.bas
