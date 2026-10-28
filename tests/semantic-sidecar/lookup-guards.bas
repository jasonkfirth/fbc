'' Project: FreeBASIC native lookup tests
'' File: lookup-guards.bas
'' Purpose: Check the production observer without linking a lookup implementation.
'' Responsibilities: Unchanged return chains, copied data, inactive gates and limits.
'' This file intentionally does NOT allocate native chain-pool slots or query a hash table.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-lookup.bi"
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
lex.ctx->last_source.source_file = "lookup-source.bas"
lex.ctx->last_source.start_line = 3
lex.ctx->last_source.start_column = 4
dim as FBSYMBOL namespc, first, second
namespc.class = FB_SYMBCLASS_NAMESPACE
namespc.id.name = @"AREA"
first.class = FB_SYMBCLASS_VAR
first.typ = FB_DATATYPE_LONG
first.id.name = @"COPIED_NAME"
second.class = FB_SYMBCLASS_VAR
second.typ = FB_DATATYPE_DOUBLE
second.id.name = @"SECOND"
dim as FBSYMCHAIN head, tail
head.sym = @first
head.next = @tail
head.isimport = TRUE
tail.sym = @second
tail.isimport = TRUE

dim as longint identity = fbSemanticModelLookupBegin("unqualified", @"OriginalQuery", @namespc, FALSE, TRUE)
if( identity <> 1 ) then end 1
if( fbSemanticModelLookupProbe("lexical", @namespc, @first) <> @first ) then end 2
fbSemanticModelLookupDecision("import-accepted", @namespc, @first)
if( fbSemanticModelLookupEnd(identity, @head, FB_TK_ID, FB_TKCLASS_IDENTIFIER, "namespace") <> @head ) then end 3
fbSemanticModelFinishLookup(TRUE)
first.id.name = @"CHANGED_AFTER_COPY"
if( head.sym <> @first or head.next <> @tail or tail.sym <> @second or tail.next <> NULL ) then end 4
if( failures or writes <> 7 or points <> 1 ) then end 5
if( instr(captured, "COPIED_NAME") = 0 or instr(captured, "CHANGED_AFTER_COPY") ) then end 6
if( instr(captured, "OriginalQuery") = 0 or instr(captured, "native-lookup-counts") = 0 ) then end 7

writes = 0
points = 0
observing = FALSE
if( fbSemanticModelLookupBegin("bad", NULL, NULL, FALSE, FALSE) ) then end 8
if( fbSemanticModelLookupProbe("bad", NULL, @first) <> @first ) then end 9
if( fbSemanticModelLookupEnd(999, @head, -1, -1, "bad") <> @head ) then end 10
observing = TRUE
lex.ctx->semantic_probe = TRUE
if( fbSemanticModelLookupBegin("bad", NULL, NULL, FALSE, FALSE) ) then end 11
lex.ctx->semantic_probe = FALSE
parser.options = FB_PARSEROPT_ISPP
if( fbSemanticModelLookupBegin("bad", NULL, NULL, FALSE, FALSE) ) then end 12
parser.options = 0
lex.ctx = NULL
if( fbSemanticModelLookupBegin("bad", NULL, NULL, FALSE, FALSE) ) then end 13
lex.ctx = @lex.ctxTB(0)
if( failures or writes or points ) then end 14

if( fbSemanticModelLookupBegin("bad", @"Query", @namespc, FALSE, FALSE) ) then end 15
if( fbSemanticModelLookupBegin("qualified", NULL, @namespc, FALSE, FALSE) ) then end 16
identity = fbSemanticModelLookupBegin("qualified", @"Query", @namespc, FALSE, TRUE)
if( fbSemanticModelLookupEnd(999, @head, INVALID, INVALID, "namespace-imports") <> @head ) then end 17
if( failures <> 3 ) then end 18
fbSemanticModelFinishLookup(FALSE)

'' More nodes than the native pool, including a cycle, must not make the
'' observer loop forever or alter the caller's original return pointer.
head.next = @head
identity = fbSemanticModelLookupBegin("qualified", @"Query", @namespc, FALSE, TRUE)
if( fbSemanticModelLookupEnd(identity, @head, INVALID, INVALID, "namespace-imports") <> @head ) then end 19
if( failures <> 4 or head.next <> @head ) then end 20
head.next = @tail
fbSemanticModelFinishLookup(FALSE)

dim as FBSYMCHAIN complete_pool(0 to CHAINPOOL_SIZE - 1)
for index as integer = 0 to CHAINPOOL_SIZE - 1
	complete_pool(index).sym = @first
	if( index < CHAINPOOL_SIZE - 1 ) then complete_pool(index).next = @complete_pool(index + 1)
next
writes = 0
identity = fbSemanticModelLookupBegin("qualified", @"Query", @namespc, FALSE, TRUE)
if( fbSemanticModelLookupEnd(identity, @complete_pool(0), INVALID, INVALID, "namespace-imports") <> @complete_pool(0) ) then end 29
fbSemanticModelFinishLookup(TRUE)
if( failures <> 4 or writes <> CHAINPOOL_SIZE + 3 ) then end 30
if( complete_pool(0).next <> @complete_pool(1) or complete_pool(CHAINPOOL_SIZE - 1).next <> NULL ) then end 31

full_mode = TRUE
writes = 0
identity = fbSemanticModelLookupBegin("qualified", @"Query", @namespc, FALSE, TRUE)
fbSemanticModelLookupProbe("qualified-own", @namespc, @first)
fbSemanticModelLookupDecision("type-own-selected", @namespc, @first)
fbSemanticModelLookupEnd(identity, @head, INVALID, INVALID, "namespace-imports")
fbSemanticModelFinishLookup(TRUE)
if( failures <> 4 or symbol_requests <> 7 or writes <> 7 ) then end 32
full_mode = FALSE

writes = 0
captured = ""
for depth as integer = 1 to 1024
	if( fbSemanticModelLookupBegin("qualified", @"Query", @namespc, FALSE, FALSE) <> depth ) then end 21
next
if( fbSemanticModelLookupBegin("qualified", @"Query", @namespc, FALSE, FALSE) ) then end 22
if( failures <> 5 ) then end 23
fbSemanticModelFinishLookup(FALSE)

writes = 0
captured = ""
fail_closed = TRUE
identity = fbSemanticModelLookupBegin("qualified", @"Query", @namespc, FALSE, TRUE)
for ordinal as integer = 1 to 999999
	if( fbSemanticModelLookupProbe("qualified-own", @namespc, @first) <> @first ) then end 24
next
fbSemanticModelLookupProbe("qualified-own", @namespc, @first)
if( failures <> 6 or writes <> 1000000 ) then end 25
fbSemanticModelFinishLookup(FALSE)
observing = TRUE
fail_closed = FALSE
writes = 0
captured = ""
identity = fbSemanticModelLookupBegin("qualified", @"Query", @namespc, FALSE, FALSE)
if( identity <> 1 ) then end 26
if( fbSemanticModelLookupEnd(identity, NULL, INVALID, INVALID, "imports-disabled") <> NULL ) then end 27
fbSemanticModelFinishLookup(TRUE)
if( writes <> 3 or instr(captured, "native-lookup:1") = 0 ) then end 28
print "LOOKUP_GUARDS_PASS"

'' end of lookup-guards.bas
