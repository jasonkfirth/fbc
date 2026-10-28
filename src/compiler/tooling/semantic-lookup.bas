'' Project: FreeBASIC compiler - native lookup explanations
'' File: tooling/semantic-lookup.bas
'' Purpose: Retain native search order and precedence without running another lookup.
'' Responsibilities: Copied query/symbol data, returned chains and bounded module closure.
'' This file intentionally does NOT contain: lexer queries, chain-pool allocation or binding inference.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-lookup.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "parser/parser.bi"

'' Lookup and the chain pool are serial compiler state. Only scalar frame IDs
'' survive a callback. Even the returned chain is copied before its pool slots
'' can be reused; this observer never calls a chain-pool or lookup function.
private const LOOKUP_DEPTH_LIMIT = 1024
private const LOOKUP_EVENT_LIMIT = 1000000
private const LOOKUP_TEXT_LIMIT = 65536
private type SEMANTIC_LOOKUP_FRAME
	identity as longint
	ordinal as longint
end type
private dim shared as SEMANTIC_LOOKUP_FRAME lookup_stack(0 to LOOKUP_DEPTH_LIMIT - 1)
private dim shared as integer lookup_depth
private dim shared as longint lookup_events, lookup_begins, lookup_ends
private dim shared as longint lookup_probes, lookup_decisions, lookup_results

private function hEnabled( ) as integer
	if( fbSemanticModelTypedExpressionsEnabled( ) = FALSE ) then return FALSE
	if( lex.ctx = NULL ) then return FALSE
	return (lex.ctx->semantic_probe = FALSE) and (fbGetIsPP( ) = FALSE)
end function

sub fbSemanticModelResetLookup( )
	lookup_depth = 0
	lookup_events = 0
	lookup_begins = 0
	lookup_ends = 0
	lookup_probes = 0
	lookup_decisions = 0
	lookup_results = 0
end sub

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendProvenance("K" + TABCHAR + "module" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

private sub hEvent( byref value as const string )
	if( lookup_events >= LOOKUP_EVENT_LIMIT ) then
		fbSemanticModelFailAt("native lookup event limit")
		exit sub
	end if
	lookup_events += 1
	hProperty("native-lookup:" + fbSemanticModelNumber(lookup_events), value)
end sub

private function hName( byval sym as FBSYMBOL ptr ) as string
	if( sym = NULL ) then return ""
	if( sym->id.name = NULL ) then return ""
	dim as string value = *sym->id.name
	if( len(value) > LOOKUP_TEXT_LIMIT ) then fbSemanticModelFailAt("native lookup name limit"): return ""
	return fbSemanticModelEscape(value)
end function

private function hSymbol( byval sym as FBSYMBOL ptr ) as string
	dim as integer symbol_class = 0
	dim as longint dtype = FB_DATATYPE_INVALID, attrib, pattrib, identity
	dim as FBSYMBOL ptr owner = NULL
	if( sym <> NULL ) then
		symbol_class = sym->class
		dtype = culng(sym->typ)
		attrib = sym->attrib
		pattrib = sym->pattrib
		if( sym->symtb <> NULL ) then owner = sym->symtb->owner
		if( fbSemanticModelFullEnabled( ) ) then identity = fbSemanticModelSymbolId(sym)
	end if
	return fbSemanticModelNumber(abs(sym <> NULL)) + TABCHAR + fbSemanticModelNumber(symbol_class) + TABCHAR + _
		fbSemanticModelTypeNumber(dtype) + TABCHAR + fbSemanticModelNumber(attrib) + TABCHAR + _
		fbSemanticModelNumber(pattrib) + TABCHAR + hName(sym) + TABCHAR + hName(owner) + TABCHAR + fbSemanticModelNumber(identity)
end function

function fbSemanticModelLookupBegin( byref kind as const string, byval spelling as const zstring ptr, _
	byval namespc as FBSYMBOL ptr, byval preserve_case as integer, byval search_imports as integer ) as longint
	if( hEnabled( ) = FALSE ) then return 0
	if( (spelling = NULL) or (lookup_depth >= LOOKUP_DEPTH_LIMIT) or _
		((kind <> "unqualified") and (kind <> "qualified")) ) then
		fbSemanticModelFailAt("invalid native lookup entry")
		return 0
	end if
	dim as string query = *spelling
	if( len(query) > LOOKUP_TEXT_LIMIT ) then fbSemanticModelFailAt("native lookup query limit"): return 0
	dim as longint parent
	if( lookup_depth > 0 ) then parent = lookup_stack(lookup_depth - 1).identity
	lookup_begins += 1
	with lookup_stack(lookup_depth)
		.identity = lookup_begins
		.ordinal = 0
	end with
	lookup_depth += 1
	dim as LEX_LOCATION source = lexGetLastLocation( )
	'' This is the last consumed lexer point, not a fabricated location for
	'' a token that the lexer may still be filling or looking ahead to.
	hEvent("begin" + TABCHAR + fbSemanticModelNumber(lookup_begins) + TABCHAR + fbSemanticModelNumber(parent) + TABCHAR + _
		kind + TABCHAR + fbSemanticModelEscape(query) + TABCHAR + fbSemanticModelNumber(abs(preserve_case <> FALSE)) + TABCHAR + _
		fbSemanticModelNumber(abs(search_imports <> FALSE)) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + _
		fbSemanticModelEscape(source.source_file) + TABCHAR + fbSemanticModelNumber(source.start_line) + TABCHAR + _
		fbSemanticModelNumber(source.start_column) + TABCHAR + hSymbol(namespc))
	return lookup_begins
end function

function fbSemanticModelLookupProbe( byref phase as const string, byval namespc as FBSYMBOL ptr, _
	byval found as FBSYMBOL ptr ) as FBSYMBOL ptr
	if( hEnabled( ) = FALSE or lookup_depth = 0 ) then return found
	lookup_probes += 1
	with lookup_stack(lookup_depth - 1)
		.ordinal += 1
		hEvent("probe" + TABCHAR + fbSemanticModelNumber(.identity) + TABCHAR + fbSemanticModelNumber(.ordinal) + TABCHAR + _
			phase + TABCHAR + hSymbol(namespc) + TABCHAR + hSymbol(found))
	end with
	return found
end function

sub fbSemanticModelLookupDecision( byref reason as const string, byval namespc as FBSYMBOL ptr, _
	byval found as FBSYMBOL ptr )
	if( hEnabled( ) = FALSE or lookup_depth = 0 ) then exit sub
	lookup_decisions += 1
	with lookup_stack(lookup_depth - 1)
		.ordinal += 1
		hEvent("decision" + TABCHAR + fbSemanticModelNumber(.identity) + TABCHAR + fbSemanticModelNumber(.ordinal) + TABCHAR + _
			reason + TABCHAR + hSymbol(namespc) + TABCHAR + hSymbol(found))
	end with
end sub

function fbSemanticModelLookupEnd( byval identity as longint, byval result as FBSYMCHAIN ptr, _
	byval token as integer, byval token_class as integer, byref route as const string ) as FBSYMCHAIN ptr
	if( identity = 0 or hEnabled( ) = FALSE ) then return result
	if( lookup_depth = 0 ) then fbSemanticModelFailAt("native lookup end has no entry"): return result
	if( lookup_stack(lookup_depth - 1).identity <> identity ) then
		fbSemanticModelFailAt("native lookup end has another entry")
		return result
	end if
	dim as integer count = 0
	dim as FBSYMCHAIN ptr item = result
	do while( item <> NULL )
		'' A valid return cannot contain more live pool slots than the pool.
		if( count >= CHAINPOOL_SIZE or item->sym = NULL ) then
			fbSemanticModelFailAt("invalid native lookup result chain")
			return result
		end if
		count += 1
		lookup_results += 1
		hEvent("result" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(count) + TABCHAR + _
			fbSemanticModelNumber(abs(item->isimport <> FALSE)) + TABCHAR + hSymbol(item->sym))
		item = item->next
	loop
	lookup_ends += 1
	hEvent("end" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(token) + TABCHAR + _
		fbSemanticModelNumber(token_class) + TABCHAR + fbSemanticModelNumber(count) + TABCHAR + route)
	lookup_depth -= 1
	return result
end function

sub fbSemanticModelFinishLookup( byval commit as integer )
	if( commit and fbSemanticModelTypedExpressionsEnabled( ) ) then
		hProperty("native-lookup-counts", fbSemanticModelNumber(lookup_events) + TABCHAR + fbSemanticModelNumber(lookup_begins) + TABCHAR + _
			fbSemanticModelNumber(lookup_ends) + TABCHAR + fbSemanticModelNumber(lookup_probes) + TABCHAR + _
			fbSemanticModelNumber(lookup_decisions) + TABCHAR + fbSemanticModelNumber(lookup_results) + TABCHAR + fbSemanticModelNumber(lookup_depth))
	end if
	fbSemanticModelResetLookup( )
end sub

'' end of tooling/semantic-lookup.bas
