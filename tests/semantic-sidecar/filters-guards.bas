'' Project: FreeBASIC native symbol-filter tests
'' File: filters-guards.bas
'' Purpose: Exercise production native filters and the collector with controlled symbols.
'' Responsibilities: Unchanged predicates, first-match access, type quirks and limits.
'' This file intentionally does NOT implement a replacement filter or hash lookup.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-filters.bi"
#include once "tooling/semantic-constructs.bi"
#include once "parser/parser.bi"

dim shared as LEX_CTX lex
dim shared as PARSERCTX parser
dim shared as integer observing = TRUE, failures, writes, points, fail_closed
dim shared as integer full_mode, symbol_requests
dim shared as FBSYMBOL ptr denied_symbol
dim shared as integer access_checks
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


function symbVarCheckAccess( byval sym as FBSYMBOL ptr ) as integer
	access_checks += 1
	if( sym = denied_symbol ) then return FALSE
	return 7
end function


lex.ctx = @lex.ctxTB(0)
dim as FBSYMBOL first, second, other
first.class = FB_SYMBCLASS_VAR
first.typ = FB_DATATYPE_LONG
first.id.name = @"FIRST"
second.class = FB_SYMBCLASS_VAR
second.typ = FB_DATATYPE_DOUBLE
second.id.name = @"SECOND"
other.class = FB_SYMBCLASS_CONST
other.typ = FB_DATATYPE_LONG
other.id.name = @"OTHER"
other.hash.next = @first
dim as FBSYMCHAIN head, tail
head.sym = @other
head.next = @tail
tail.sym = @second

if( symbFindByClass(@head, FB_SYMBCLASS_VAR) <> @first ) then end 1
if( access_checks <> 1 or head.sym <> @other or other.hash.next <> @first or head.next <> @tail ) then end 2
fbSemanticModelFinishFilters(TRUE)
if( failures ) then end 3

access_checks = 0
if( symbFindByClassAndType(@head, FB_SYMBCLASS_VAR, FB_DATATYPE_DOUBLE) <> @second ) then end 4
if( access_checks <> 1 ) then end 5
fbSemanticModelFinishFilters(TRUE)

'' A denied first match ends the search even when a later match is public.
head.sym = @first
denied_symbol = @first
second.typ = FB_DATATYPE_LONG
access_checks = 0
if( symbFindByClass(@head, FB_SYMBCLASS_VAR) <> NULL ) then end 6
if( symbFindByClassAndType(@head, FB_SYMBCLASS_VAR, FB_DATATYPE_LONG) <> NULL ) then end 7
if( symbFindVarBySuffix(@head, FB_DATATYPE_LONG) <> NULL ) then end 8
if( symbFindVarByDefType(@head, FB_DATATYPE_LONG) <> NULL ) then end 9
if( access_checks <> 4 ) then end 10
fbSemanticModelFinishFilters(TRUE)
if( failures ) then end 11
denied_symbol = NULL

'' Native unsuffixed declarations keep their own type under DEF rules.
first.typ = FB_DATATYPE_DOUBLE
first.attrib = 0
if( symbFindVarByDefType(@head, FB_DATATYPE_LONG) <> @first ) then end 12
first.attrib = FB_SYMBATTRIB_SUFFIXED
if( symbFindVarByDefType(@head, FB_DATATYPE_LONG) <> @second ) then end 13
if( symbFindVarBySuffix(@head, FB_DATATYPE_LONG) <> @second ) then end 14
fbSemanticModelFinishFilters(TRUE)

first.typ = FB_DATATYPE_STRING
if( symbFindVarBySuffix(@head, FB_DATATYPE_STRING) <> @first ) then end 15
if( symbFindVarByDefType(@head, FB_DATATYPE_STRING) <> @first ) then end 16
first.typ = FB_DATATYPE_USTRING
if( symbFindVarBySuffix(@head, FB_DATATYPE_STRING) <> @first ) then end 17
if( symbFindVarByDefType(@head, FB_DATATYPE_STRING) <> @first ) then end 18
first.typ = FB_DATATYPE_FIXSTR
if( symbFindVarBySuffix(@head, FB_DATATYPE_STRING) <> @first ) then end 19
if( symbFindVarByDefType(@head, FB_DATATYPE_STRING) <> @first ) then end 20
first.typ = FB_DATATYPE_CHAR
if( symbFindVarBySuffix(@head, FB_DATATYPE_STRING) <> @first ) then end 21
if( symbFindVarByDefType(@head, FB_DATATYPE_STRING) <> @first ) then end 22
fbSemanticModelFinishFilters(TRUE)

head.next = NULL
first.typ = FB_DATATYPE_CHAR or FB_DT_CONSTMASK
if( symbFindVarBySuffix(@head, FB_DATATYPE_STRING) <> @first ) then end 23
if( symbFindVarByDefType(@head, FB_DATATYPE_STRING) <> NULL ) then end 24
if( symbFindByClassAndType(@head, FB_SYMBCLASS_VAR, FB_DATATYPE_CHAR) <> NULL ) then end 25
if( symbFindByClass(@head, FB_SYMBCLASS_PROC) <> NULL ) then end 26
if( symbFindByClass(NULL, FB_SYMBCLASS_VAR) <> NULL ) then end 27
fbSemanticModelFinishFilters(TRUE)
if( failures ) then end 28

observing = FALSE
writes = 0
access_checks = 0
if( symbFindByClass(@head, FB_SYMBCLASS_VAR) <> @first ) then end 29
if( access_checks <> 1 or writes ) then end 30
if( fbSemanticModelFilterAccess(7) <> 7 ) then end 31
observing = TRUE
lex.ctx->semantic_probe = TRUE
if( fbSemanticModelFilterBegin("bad", 0, 0, NULL) ) then end 32
lex.ctx->semantic_probe = FALSE
parser.options = FB_PARSEROPT_ISPP
if( fbSemanticModelFilterBegin("bad", 0, 0, NULL) ) then end 33
parser.options = 0
if( writes or failures ) then end 34

'' Collector guards preserve native values, including noncanonical TRUE.
if( fbSemanticModelFilterBegin("bad", 0, 0, NULL) ) then end 35
fbSemanticModelFilterBegin("class", 1, FB_DATATYPE_INVALID, NULL)
if( fbSemanticModelFilterAccess(7) <> 7 ) then end 36
if( fbSemanticModelFilterEnd(999, @first, "selected") <> @first ) then end 37
if( failures <> 3 ) then end 38
fbSemanticModelFinishFilters(FALSE)
for depth as integer = 1 to 1024
	if( fbSemanticModelFilterBegin("class", FB_SYMBCLASS_VAR, FB_DATATYPE_INVALID, NULL) <> depth ) then end 39
next
if( fbSemanticModelFilterBegin("class", 1, FB_DATATYPE_INVALID, NULL) ) then end 40
if( failures <> 4 ) then end 41
fbSemanticModelFinishFilters(FALSE)

writes = 0
fail_closed = TRUE
dim as longint identity = fbSemanticModelFilterBegin("class", 1, FB_DATATYPE_INVALID, @head)
for ordinal as integer = 1 to 999999
	if( fbSemanticModelFilterCandidate(@first, FALSE, "class") <> FALSE ) then end 42
next
fbSemanticModelFilterCandidate(@first, FALSE, "class")
if( failures <> 5 or writes <> 1000000 ) then end 43
fbSemanticModelFinishFilters(FALSE)
observing = TRUE
fail_closed = FALSE
identity = fbSemanticModelFilterBegin("class", FB_SYMBCLASS_CONST, FB_DATATYPE_INVALID, NULL)
if( identity <> 1 ) then end 44
if( fbSemanticModelFilterEnd(identity, NULL, "not-found") <> NULL ) then end 45
fbSemanticModelFinishFilters(TRUE)
print "FILTER_GUARDS_PASS"

'' end of filters-guards.bas
