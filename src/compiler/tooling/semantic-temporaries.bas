'' Project: FreeBASIC compiler - temporary lifetime observations
'' File: tooling/semantic-temporaries.bas
'' Purpose: Preserve actual destructor-list state changes and branch-local flushes.
'' Responsibilities: Copied identities, scope occurrences, actions and bounded closure.
'' This file intentionally does NOT contain: temporary or destructor selection.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-temporaries.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "crt/mem.bi"

'' Modules are serial. Events contain copied scalars only. Native cookie
'' numbers can be reused after flush(0), even though a scope occurrence may
'' still be open. The occurrence ID is the scope-begin event, not that cookie.
private const TEMPORARY_LIMIT = 1000000
private const TEMPORARY_SCOPE_LIMIT = 65536
private dim shared as longint event_count, registration_count, flush_count, conditional_count, live_count, reset_epoch
private dim shared as integer scope_depth
private dim shared as longint scope_events(0 to TEMPORARY_SCOPE_LIMIT - 1)

type SEMANTIC_TEMPORARY_PROJECTION
	event_id as longint
	nodes as longint
end type

'' Only copied action IDs and counts survive AST updating. The node's small
'' index is copied by astCloneTree and cleared by astInitNode on pool reuse.
private dim shared as SEMANTIC_TEMPORARY_PROJECTION ptr projections
private dim shared as integer projection_count, projection_capacity
private dim shared as longint projected_node_count

sub fbSemanticModelResetTemporaries( )
	event_count = 0
	registration_count = 0
	flush_count = 0
	conditional_count = 0
	live_count = 0
	reset_epoch = 1
	scope_depth = 0
	deallocate(projections)
	projections = NULL
	projection_count = 0
	projection_capacity = 0
	projected_node_count = 0
end sub

private sub hProperty( byref key as const string, byref fields as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + key + TABCHAR + fbSemanticModelEscape(fields))
end sub

private function hEvent( byref kind as const string, byref fields as const string ) as longint
	if( event_count >= TEMPORARY_LIMIT ) then
		fbSemanticModelFailAt("temporary lifetime event bound exceeded")
		return 0
	end if
	event_count += 1
	hProperty("native-temp-event:" + fbSemanticModelNumber(event_count), kind + TABCHAR + fields)
	return event_count
end function

private function hItem( byval item as AST_DTORLIST_ITEM ptr ) as longint
	if( item = NULL ) then
		fbSemanticModelFailAt("temporary lifetime item is unavailable")
		return 0
	end if
	if( (item->sym = NULL) orElse (item->sym->class <> FB_SYMBCLASS_VAR) ) then
		fbSemanticModelFailAt("temporary lifetime storage has another native class")
		return 0
	end if
	if( (item->semantic_generation = 0) or (item->semantic_generation > TEMPORARY_LIMIT) or _
		(item->cookie < -1) or (item->refcount < 0) or (item->refcount > TEMPORARY_LIMIT) ) then
		fbSemanticModelFailAt("temporary lifetime item has invalid scalar state")
		return 0
	end if
	return clngint(item->semantic_generation)
end function

'' -------------------------------------------------------------------------
'' Native registration and list transitions
'' -------------------------------------------------------------------------

sub fbSemanticModelTemporaryAdd( byval item as AST_DTORLIST_ITEM ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint generation = hItem(item)
	if( generation = 0 ) then exit sub
	'' A canceled TYPEINI temporary can be deleted before a procedure's
	'' normal symbol-table export. Complete its metadata while it is live.
	fbSemanticModelExportSymbolDetails(item->sym, TRUE)
	dim as longint scope_id = 0, proc_id = 0
	if( scope_depth > 0 ) then scope_id = scope_events(scope_depth - 1)
	if( ast.proc.curr <> NULL ) then proc_id = fbSemanticModelSymbolId(ast.proc.curr->sym)
	dim as longint identity = hEvent("add", fbSemanticModelNumber(generation) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(item->sym)) + TABCHAR + fbSemanticModelNumber(item->cookie) + _
		TABCHAR + fbSemanticModelNumber(item->refcount) + TABCHAR + fbSemanticModelNumber(scope_id) + _
		TABCHAR + fbSemanticModelNumber(reset_epoch) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )) + _
		TABCHAR + fbSemanticModelNumber(proc_id))
	if( identity = 0 ) then exit sub
	registration_count += 1
	live_count += 1
end sub

sub fbSemanticModelTemporaryReference( byval item as AST_DTORLIST_ITEM ptr, byval added as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint generation = hItem(item)
	if( generation = 0 ) then exit sub
	dim as string kind = iif(added, "reference-add", "reference-remove")
	hEvent(kind, fbSemanticModelNumber(generation) + TABCHAR + fbSemanticModelNumber(item->cookie) + TABCHAR + fbSemanticModelNumber(item->refcount))
end sub

sub fbSemanticModelTemporaryRescope( byval item as AST_DTORLIST_ITEM ptr, byval newcookie as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint generation = hItem(item)
	if( generation = 0 ) then exit sub
	if( newcookie < -1 ) then
		fbSemanticModelFailAt("temporary rescope has another native cookie state")
		exit sub
	end if
	hEvent("rescope", fbSemanticModelNumber(generation) + TABCHAR + fbSemanticModelNumber(item->cookie) + TABCHAR + fbSemanticModelNumber(newcookie))
end sub

sub fbSemanticModelTemporaryRemove( byval item as AST_DTORLIST_ITEM ptr, byref reason as const string )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint generation = hItem(item)
	if( generation = 0 ) then exit sub
	if( (reason <> "flush") and (reason <> "discard") and (reason <> "reference-release") and (reason <> "explicit-delete") ) then
		fbSemanticModelFailAt("temporary removal has another native reason")
		exit sub
	end if
	if( live_count < 1 ) then
		fbSemanticModelFailAt("temporary removal underflows observed registrations")
		exit sub
	end if
	hEvent("remove", fbSemanticModelNumber(generation) + TABCHAR + reason)
	live_count -= 1
end sub

'' -------------------------------------------------------------------------
'' Scope occurrences and selected flush actions
'' -------------------------------------------------------------------------

sub fbSemanticModelTemporaryScopeBegin( byval cookie as integer, byval allocated as integer, byval depth as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (cookie < 1) or (scope_depth >= TEMPORARY_SCOPE_LIMIT) or (depth <> scope_depth + 1) ) then
		fbSemanticModelFailAt("temporary scope begin disagrees with native depth or cookie")
		exit sub
	end if
	dim as longint identity = hEvent("scope-begin", fbSemanticModelNumber(reset_epoch) + TABCHAR + fbSemanticModelNumber(cookie) + _
		TABCHAR + fbSemanticModelNumber(abs(allocated <> FALSE)) + TABCHAR + fbSemanticModelNumber(depth))
	if( identity = 0 ) then exit sub
	scope_events(scope_depth) = identity
	scope_depth += 1
end sub

sub fbSemanticModelTemporaryScopeEnd( byval cookie as integer, byval depth as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (cookie < 1) or (scope_depth < 1) or (depth <> scope_depth - 1) ) then
		fbSemanticModelFailAt("temporary scope end disagrees with native depth or cookie")
		exit sub
	end if
	hEvent("scope-end", fbSemanticModelNumber(scope_events(scope_depth - 1)) + TABCHAR + fbSemanticModelNumber(cookie) + TABCHAR + fbSemanticModelNumber(depth))
	scope_depth -= 1
end sub

function fbSemanticModelTemporaryFlushBegin( byval cookie as integer ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	if( cookie < 0 ) then
		fbSemanticModelFailAt("temporary flush has a negative native cookie")
		return 0
	end if
	dim as longint identity = hEvent("flush-begin", fbSemanticModelNumber(reset_epoch) + TABCHAR + fbSemanticModelNumber(cookie))
	if( identity <> 0 ) then flush_count += 1
	return identity
end function

sub fbSemanticModelTemporaryFlushAction( byval flush_id as longint, byval item as AST_DTORLIST_ITEM ptr, byval action as ASTNODE ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint generation = hItem(item)
	if( generation = 0 ) then exit sub
	if( (flush_id < 1) or (flush_id > event_count) ) then
		fbSemanticModelFailAt("temporary action has no observed flush")
		exit sub
	end if
	if( action <> NULL ) then
		if( action->semantic_temporary_cleanup <> 0 ) then
			fbSemanticModelFailAt("temporary cleanup root already has an action")
			exit sub
		end if
	end if
	if( projection_count >= TEMPORARY_LIMIT ) then
		fbSemanticModelFailAt("temporary cleanup projection bound exceeded")
		exit sub
	end if
	if( projection_count = projection_capacity ) then
		dim as integer capacity = iif(projection_capacity = 0, 128, projection_capacity * 2)
		if( capacity > TEMPORARY_LIMIT ) then capacity = TEMPORARY_LIMIT
		dim as SEMANTIC_TEMPORARY_PROJECTION ptr storage = reallocate(projections, capacity * sizeof(SEMANTIC_TEMPORARY_PROJECTION))
		if( storage = NULL ) then
			fbSemanticModelFailAt("temporary cleanup projection allocation failed")
			exit sub
		end if
		memset(storage + projection_capacity, 0, (capacity - projection_capacity) * sizeof(SEMANTIC_TEMPORARY_PROJECTION))
		projections = storage
		projection_capacity = capacity
	end if
	dim as longint event_id = hEvent("flush-action", fbSemanticModelNumber(flush_id) + TABCHAR + _
		fbSemanticModelNumber(generation) + TABCHAR + fbSemanticModelNumber(abs(action <> NULL)))
	if( event_id = 0 ) then exit sub
	projections[projection_count].event_id = event_id
	projections[projection_count].nodes = 0
	projection_count += 1
	if( action <> NULL ) then action->semantic_temporary_cleanup = projection_count
end sub

sub fbSemanticModelTemporaryFlushEnd( byval flush_id as longint, byval cookie as integer, byval actions as integer, byval removed as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (flush_id < 1) or (flush_id > event_count) or (cookie < 0) or (actions < 0) or (removed < actions) ) then
		fbSemanticModelFailAt("temporary flush closure has invalid native counters")
		exit sub
	end if
	hEvent("flush-end", fbSemanticModelNumber(flush_id) + TABCHAR + fbSemanticModelNumber(actions) + TABCHAR + fbSemanticModelNumber(removed))
	if( cookie = 0 ) then reset_epoch += 1
end sub

sub fbSemanticModelTemporaryConditional( byval result as FBSYMBOL ptr, byval true_flush as longint, byval false_flush as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (result = NULL) orElse (result->class <> FB_SYMBCLASS_VAR) ) then
		fbSemanticModelFailAt("temporary conditional result has another native class")
		exit sub
	end if
	if( (true_flush < 0) or (false_flush < 0) or (true_flush > event_count) or (false_flush > event_count) ) then
		fbSemanticModelFailAt("temporary conditional has an unavailable flush")
		exit sub
	end if
	if( hEvent("conditional", fbSemanticModelNumber(fbSemanticModelSymbolId(result)) + TABCHAR + _
		fbSemanticModelNumber(true_flush) + TABCHAR + fbSemanticModelNumber(false_flush)) <> 0 ) then conditional_count += 1
end sub

sub fbSemanticModelTemporaryNode( byval node as ASTNODE ptr, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (node = NULL) or (identity <= 0) ) then
		fbSemanticModelFailAt("temporary cleanup projection has unavailable inputs")
		exit sub
	end if
	if( node->semantic_temporary_cleanup = 0 ) then exit sub
	if( (node->semantic_temporary_cleanup < 0) or (node->semantic_temporary_cleanup > projection_count) or _
		(projected_node_count >= TEMPORARY_LIMIT) ) then
		fbSemanticModelFailAt("temporary cleanup projection has no bounded action")
		exit sub
	end if
	dim as integer index = cint(node->semantic_temporary_cleanup - 1)
	projections[index].nodes += 1
	projected_node_count += 1
	fbSemanticModelAppendDetail("K" + TABCHAR + "node" + TABCHAR + fbSemanticModelNumber(identity) + _
		TABCHAR + "native-temp-cleanup" + TABCHAR + fbSemanticModelNumber(projections[index].event_id))
end sub

sub fbSemanticModelFinishTemporaries( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		for index as integer = 0 to projection_count - 1
			hProperty("native-temp-action-nodes:" + fbSemanticModelNumber(projections[index].event_id), fbSemanticModelNumber(projections[index].nodes))
		next
		hProperty("native-temp-projection-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(projection_count) + TABCHAR + fbSemanticModelNumber(projected_node_count))
		hProperty("native-temp-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), fbSemanticModelNumber(event_count) + _
			TABCHAR + fbSemanticModelNumber(registration_count) + TABCHAR + fbSemanticModelNumber(flush_count) + _
			TABCHAR + fbSemanticModelNumber(conditional_count) + TABCHAR + fbSemanticModelNumber(live_count) + TABCHAR + fbSemanticModelNumber(scope_depth))
	end if
	fbSemanticModelResetTemporaries( )
end sub

'' end of tooling/semantic-temporaries.bas
