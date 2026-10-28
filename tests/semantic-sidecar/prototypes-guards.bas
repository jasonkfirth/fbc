'' Project: FreeBASIC native prototype tests
'' File: prototypes-guards.bas
'' Purpose: Exercise the actual native prototype matcher with controlled symbols.
'' Responsibilities: Arity/property/instance rules, reversed comparisons and bounded receipts.
'' This file intentionally does NOT contain: replacement matching or overload scoring.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-prototypes.bi"
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

lex.ctx = @lex.ctxTB(0)
dim as FBSYMBOL head, alternative, query, first_param, second_param, query_param, first_type, second_type
head.class = FB_SYMBCLASS_PROC
head.id.name = @"FIRST"
head.proc.params = 1
head.proc.paramtb.tail = @first_param
head.proc.ovl.next = @alternative
alternative.class = FB_SYMBCLASS_PROC
alternative.id.name = @"SECOND"
alternative.proc.params = 1
alternative.proc.paramtb.tail = @second_param
query.class = FB_SYMBCLASS_PROC
query.id.name = @"QUERY"
query.proc.params = 1
query.proc.paramtb.tail = @query_param
first_param.class = FB_SYMBCLASS_PARAM
first_param.id.name = @"FIRST_PARAM"
first_param.param.mode = FB_PARAMMODE_BYVAL
first_param.typ = FB_DATATYPE_LONG
second_param.class = FB_SYMBCLASS_PARAM
second_param.id.name = @"SECOND_PARAM"
second_param.param.mode = FB_PARAMMODE_BYVAL
second_param.typ = FB_DATATYPE_DOUBLE
query_param.class = FB_SYMBCLASS_PARAM
query_param.id.name = @"QUERY_PARAM"
query_param.param.mode = FB_PARAMMODE_BYVAL
query_param.typ = FB_DATATYPE_DOUBLE

if( symbFindOverloadProc(@head, @query, FB_SYMBFINDOPT_NONE) <> @alternative ) then end 1
fbSemanticModelFinishPrototypes(TRUE)
if( failures ) then end 2
head.proc.params = 2
if( symbFindOverloadProc(@head, @query, FB_SYMBFINDOPT_NONE) <> @alternative ) then end 3
head.proc.params = 1
first_param.typ = FB_DATATYPE_DOUBLE
first_param.param.mode = FB_PARAMMODE_BYDESC
first_param.param.bydescdimensions = 2
second_param.param.mode = FB_PARAMMODE_BYDESC
second_param.param.bydescdimensions = 1
query_param.param.mode = FB_PARAMMODE_BYDESC
query_param.param.bydescdimensions = 1
if( symbFindOverloadProc(@head, @query, FB_SYMBFINDOPT_NONE) <> @alternative ) then end 4
first_param.param.bydescdimensions = -1
if( symbFindOverloadProc(@head, @query, FB_SYMBFINDOPT_NONE) <> @head ) then end 5
first_param.param.mode = FB_PARAMMODE_BYVAL
query_param.param.mode = FB_PARAMMODE_BYREF
if( symbFindOverloadProc(@head, @query, FB_SYMBFINDOPT_NONE) <> @head ) then end 6
fbSemanticModelFinishPrototypes(TRUE)
if( failures ) then end 7

first_type.class = FB_SYMBCLASS_STRUCT
first_type.typ = FB_DATATYPE_STRUCT
first_type.id.name = @"PAYLOAD"
second_type.class = FB_SYMBCLASS_STRUCT
second_type.typ = FB_DATATYPE_STRUCT
second_type.id.name = @"PAYLOAD"
first_param.param.mode = FB_PARAMMODE_BYVAL
second_param.param.mode = FB_PARAMMODE_BYVAL
query_param.param.mode = FB_PARAMMODE_BYVAL
first_param.typ = FB_DATATYPE_STRUCT
second_param.typ = FB_DATATYPE_STRUCT
query_param.typ = FB_DATATYPE_STRUCT
first_param.subtype = @first_type
second_param.subtype = @second_type
query_param.subtype = @second_type
if( symbFindOverloadProc(@head, @query, FB_SYMBFINDOPT_NONE) <> @alternative ) then end 8
fbSemanticModelFinishPrototypes(TRUE)
if( failures ) then end 9

'' Property getter/setter discrimination happens before parameter traversal.
head.pattrib = FB_PROCATTRIB_METHOD or FB_PROCATTRIB_PROPERTY
alternative.pattrib = head.pattrib
query.pattrib = FB_PROCATTRIB_METHOD
head.typ = FB_DATATYPE_VOID
alternative.typ = FB_DATATYPE_LONG
if( symbFindOverloadProc(@head, @query, FB_SYMBFINDOPT_PROPGET) <> @alternative ) then end 10
if( symbFindOverloadProc(@head, @query, FB_SYMBFINDOPT_NONE) <> @head ) then end 11
head.pattrib = 0
alternative.pattrib = 0
query.pattrib = 0
head.proc.params = 0
alternative.proc.params = 0
query.proc.params = 0
if( symbFindOverloadProc(@head, @query, FB_SYMBFINDOPT_NONE) <> @head ) then end 12
fbSemanticModelFinishPrototypes(TRUE)
if( failures ) then end 13

if( symbFindOverloadProc(NULL, @query, FB_SYMBFINDOPT_NONE) <> NULL ) then end 14
if( symbFindOverloadProc(@head, NULL, FB_SYMBFINDOPT_NONE) <> NULL ) then end 15
if( symbFindOverloadProc(@first_param, @query, FB_SYMBFINDOPT_NONE) <> NULL ) then end 16
head.proc.params = 2
alternative.proc.params = 3
if( symbFindOverloadProc(@head, @query, FB_SYMBFINDOPT_NONE) <> NULL ) then end 17
fbSemanticModelFinishPrototypes(TRUE)
if( failures ) then end 18

observing = FALSE
writes = 0
head.proc.params = 0
if( symbFindOverloadProc(@head, @query, FB_SYMBFINDOPT_NONE) <> @head ) then end 19
if( fbSemanticModelPrototypeParameter(0, 0, 0, "bad", NULL, NULL, 7) <> 7 ) then end 20
observing = TRUE
lex.ctx->semantic_probe = TRUE
if( fbSemanticModelPrototypeBegin(NULL, NULL, 0) ) then end 21
lex.ctx->semantic_probe = FALSE
parser.options = FB_PARSEROPT_ISPP
if( fbSemanticModelPrototypeBegin(NULL, NULL, 0) ) then end 22
parser.options = 0
if( writes or failures ) then end 23

fbSemanticModelResetPrototypes()
for depth as integer = 1 to 1024
	if( fbSemanticModelPrototypeBegin(NULL, NULL, 0) <> depth ) then end 24
next
if( fbSemanticModelPrototypeBegin(NULL, NULL, 0) ) then end 25
if( failures <> 1 ) then end 26
fbSemanticModelFinishPrototypes(FALSE)

writes = 0
fail_closed = TRUE
dim as longint identity = fbSemanticModelPrototypeBegin(@head, @query, 0)
for ordinal as integer = 1 to 999999
	if( fbSemanticModelPrototypeCandidate(identity, @head, 1, 0, 0, FALSE) <> ordinal ) then end 27
next
fbSemanticModelPrototypeCandidate(identity, @head, 1, 0, 0, FALSE)
if( failures <> 2 or writes <> 1000000 ) then end 28
fbSemanticModelFinishPrototypes(FALSE)
observing = TRUE
fail_closed = FALSE
identity = fbSemanticModelPrototypeBegin(NULL, NULL, 0)
if( identity <> 1 ) then end 29
fbSemanticModelPrototypeEnd(identity, 0, NULL, "invalid-input")
fbSemanticModelFinishPrototypes(TRUE)
print "PROTOTYPE_GUARDS_PASS"

'' end of prototypes-guards.bas
