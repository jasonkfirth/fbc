'' Project: FreeBASIC qualified lookup/filter tests
'' File: qualified-filter-guards.bas
'' Purpose: Verify the native wrapper links exactly the attempts it executed.
'' Responsibilities: Real matcher bodies, omitted filters, unchanged results and gates.
'' This file intentionally does NOT modify returned chain nodes or infer joins from names.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-lookup.bi"
#include once "tooling/semantic-filters.bi"
#include once "tooling/semantic-constructs.bi"
#include once "parser/parser.bi"

dim shared as LEX_CTX lex
dim shared as PARSERCTX parser
dim shared as integer observing = TRUE, failures, writes, points, fail_closed
dim shared as integer full_mode, symbol_requests
dim shared as string captured

function fbSemanticModelTypedExpressionsEnabled( ) as integer
	return observing
end function
function fbSemanticModelFullEnabled( ) as integer
	return full_mode
end function
function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	if( sym = NULL ) then return 0
	symbol_requests += 1
	return sym->class
end function
function fbSemanticModelModuleIdentity( ) as longint
	return 1
end function
function fbSemanticModelNumber( byval value as longint ) as string
	return ltrim(str(value))
end function
function fbSemanticModelTypeNumber( byval value as longint ) as string
	return fbSemanticModelNumber(value)
end function
function fbSemanticModelEscape( byref value as const string ) as string
	return value
end function
function fbSemanticModelCurrentStatement( ) as longint
	return 8
end function
function fbSemanticModelCurrentContext( ) as longint
	return 9
end function
function fbSemanticModelCurrentSource( ) as longint
	return 1
end function
function lexGetLastLocation( ) as LEX_LOCATION
	points += 1
	return lex.ctx->last_source
end function
sub fbSemanticModelAppendProvenance( byref value as const string )
	writes += 1
	if( writes <= 32 ) then captured += value + chr(10)
end sub
sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
	if( fail_closed ) then observing = FALSE
end sub

dim shared as FBSYMCHAIN ptr returned_chain

function symbLookupAt( byval ns as FBSYMBOL ptr, byval id as const zstring ptr, _
	byval preserve_case as integer, byval search_imports as integer ) as FBSYMCHAIN ptr
	dim as longint identity = fbSemanticModelLookupBegin("qualified", id, ns, preserve_case, search_imports)
	dim as FBSYMBOL ptr found = NULL
	if( returned_chain <> NULL ) then found = returned_chain->sym
	fbSemanticModelLookupProbe("qualified-own", ns, found)
	dim as string route = "imports-disabled"
	if( found <> NULL ) then route = "own-symbol"
	return fbSemanticModelLookupEnd(identity, returned_chain, INVALID, INVALID, route)
end function

function symbVarCheckAccess( byval sym as FBSYMBOL ptr ) as integer
	return 7
end function

lex.ctx = @lex.ctxTB(0)
dim as FBSYMBOL namespc, other, value
namespc.class = FB_SYMBCLASS_NAMESPACE
namespc.id.name = @"AREA"
other.class = FB_SYMBCLASS_CONST
other.typ = FB_DATATYPE_LONG
other.id.name = @"SAME"
other.hash.next = @value
value.class = FB_SYMBCLASS_VAR
value.typ = FB_DATATYPE_LONG
value.id.name = @"SAME"
dim as FBSYMCHAIN chain_
chain_.sym = @other
returned_chain = @chain_
if( symbLookupByNameAndClass(@namespc, @"Same", FB_SYMBCLASS_VAR, FALSE, FALSE) <> @value ) then end 1
if( fbSemanticModelLastLookup() <> 1 or fbSemanticModelLastFilter() <> 1 ) then end 2
if( symbLookupByNameAndClass(@namespc, @"Same", FB_SYMBCLASS_PROC, FALSE, FALSE) <> NULL ) then end 3
returned_chain = NULL
if( symbLookupByNameAndClass(@namespc, @"Missing", FB_SYMBCLASS_PROC, FALSE, FALSE) <> NULL ) then end 4
if( fbSemanticModelLastLookup() <> 3 or fbSemanticModelLastFilter() <> 2 ) then end 5
if( instr(captured, "native-qualified-filter:3" + chr(9) + "3" + chr(9) + "0" + chr(9) + "3") = 0 ) then end 6
if( chain_.sym <> @other or chain_.next <> NULL or other.hash.next <> @value ) then end 7
fbSemanticModelFinishFilters(TRUE)
fbSemanticModelFinishLookup(TRUE)
if( failures ) then end 8
if( fbSemanticModelLastLookup() or fbSemanticModelLastFilter() ) then end 9
observing = FALSE
writes = 0
returned_chain = @chain_
if( symbLookupByNameAndClass(@namespc, @"Same", FB_SYMBCLASS_VAR, FALSE, FALSE) <> @value ) then end 10
if( writes or fbSemanticModelLastLookup() or fbSemanticModelLastFilter() ) then end 11
observing = TRUE
fbSemanticModelResetFilters()
fbSemanticModelResetLookup()
if( symbLookupByNameAndClass(@namespc, @"Same", FB_SYMBCLASS_VAR, FALSE, FALSE) <> @value ) then end 12
if( fbSemanticModelLastLookup() <> 1 or fbSemanticModelLastFilter() <> 1 ) then end 13
print "QUALIFIED_FILTER_GUARDS_PASS"

'' end of qualified-filter-guards.bas
