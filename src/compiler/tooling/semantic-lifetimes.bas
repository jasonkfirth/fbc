'' Project: FreeBASIC compiler - semantic lifetime observations
'' File: tooling/semantic-lifetimes.bas
'' Purpose: Retain actual normal scope/procedure teardown and its inserted roots.
'' Responsibilities: Copy group owners, selected storage, order and bounded closure.
'' This file intentionally does NOT contain: destructor selection or AST lowering.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-lifetimes.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"

'' The compiler processes modules serially. These are observation counters;
'' group IDs travel in native stack locals, so nested generated procedures
'' cannot replace an outer traversal's owner. No native addresses are retained.
private const LIFETIME_LIMIT = 1000000
private dim shared as longint group_count, action_count, node_count

sub fbSemanticModelResetLifetimes( )
	group_count = 0
	action_count = 0
	node_count = 0
end sub

private function hNext( byref count as longint ) as longint
	if( count >= LIFETIME_LIMIT ) then
		fbSemanticModelFailAt("lexical cleanup observation exceeds its bound")
		return 0
	end if
	count += 1
	return count
end function

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hGroup( byval identity as longint ) as integer
	if( (identity < 1) or (identity > group_count) ) then
		fbSemanticModelFailAt("lexical cleanup has no observed native group")
		return FALSE
	end if
	return TRUE
end function

function fbSemanticModelLexicalCleanupBegin( byval owner as FBSYMBOL ptr ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	if( owner = NULL ) then
		fbSemanticModelFailAt("lexical cleanup owner is unavailable")
		return 0
	end if
	if( (owner->class <> FB_SYMBCLASS_PROC) and (owner->class <> FB_SYMBCLASS_SCOPE) ) then
		fbSemanticModelFailAt("lexical cleanup owner is not a procedure or scope")
		return 0
	end if
	if( ast.proc.curr = NULL ) then
		fbSemanticModelFailAt("lexical cleanup procedure is unavailable")
		return 0
	end if
	if( (ast.proc.curr->class <> AST_NODECLASS_PROC) orElse _
		(ast.proc.curr->sym = NULL) orElse (ast.proc.curr->sym->class <> FB_SYMBCLASS_PROC) ) then
		fbSemanticModelFailAt("lexical cleanup has another native procedure class")
		return 0
	end if
	dim as longint owner_id = fbSemanticModelSymbolId(owner), proc_id = fbSemanticModelSymbolId(ast.proc.curr->sym)
	if( (owner_id <= 0) or (proc_id <= 0) ) then exit function
	dim as longint identity = hNext(group_count)
	if( identity = 0 ) then exit function
	hProperty("native-lexical-cleanup-group:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(owner_id) + _
		TABCHAR + fbSemanticModelNumber(proc_id) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentContext( )))
	return identity
end function

sub fbSemanticModelLexicalCleanup( byval group_id as longint, byval ordinal as integer, byval variable as FBSYMBOL ptr, byval action as ASTNODE ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( hGroup(group_id) = FALSE ) then exit sub
	if( (ordinal < 1) or (ordinal > LIFETIME_LIMIT) or (variable = NULL) ) then
		fbSemanticModelFailAt("lexical cleanup has an unavailable ordinal or variable")
		exit sub
	end if
	if( variable->class <> FB_SYMBCLASS_VAR ) then
		fbSemanticModelFailAt("lexical cleanup storage has another native class")
		exit sub
	end if
	if( action <> NULL ) then
		if( (action->semantic_lexical_cleanup <> 0) or (action->semantic_exit_cleanup <> 0) ) then
			fbSemanticModelFailAt("lexical cleanup root already has a cleanup owner")
			exit sub
		end if
	end if
	dim as longint variable_id = fbSemanticModelSymbolId(variable)
	if( variable_id <= 0 ) then exit sub
	dim as longint identity = hNext(action_count)
	if( identity = 0 ) then exit sub
	if( action <> NULL ) then action->semantic_lexical_cleanup = identity
	hProperty("native-lexical-cleanup:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(group_id) + _
		TABCHAR + fbSemanticModelNumber(ordinal) + TABCHAR + fbSemanticModelNumber(variable_id) + _
		TABCHAR + fbSemanticModelNumber(abs(action <> NULL)))
end sub

sub fbSemanticModelLexicalCleanupEnd( byval group_id as longint, byval count as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( hGroup(group_id) = FALSE ) then exit sub
	if( (count < 0) or (count > LIFETIME_LIMIT) ) then
		fbSemanticModelFailAt("lexical cleanup count exceeds its bound")
		exit sub
	end if
	hProperty("native-lexical-cleanup-end:" + fbSemanticModelNumber(group_id), fbSemanticModelNumber(count))
end sub

sub fbSemanticModelLexicalCleanupNode( byval node as ASTNODE ptr, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (node = NULL) or (identity <= 0) ) then
		fbSemanticModelFailAt("lexical cleanup node has unavailable inputs")
		exit sub
	end if
	if( node->semantic_lexical_cleanup = 0 ) then exit sub
	if( (node->semantic_lexical_cleanup < 0) or (node->semantic_lexical_cleanup > action_count) ) then
		fbSemanticModelFailAt("lexical cleanup node has no selected action")
		exit sub
	end if
	if( hNext(node_count) = 0 ) then exit sub
	fbSemanticModelAppendDetail("K" + TABCHAR + "node" + TABCHAR + fbSemanticModelNumber(identity) + _
		TABCHAR + "native-lexical-cleanup" + TABCHAR + fbSemanticModelNumber(node->semantic_lexical_cleanup))
end sub

sub fbSemanticModelFinishLifetimes( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		hProperty("native-lexical-cleanup-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(group_count) + TABCHAR + fbSemanticModelNumber(action_count) + TABCHAR + fbSemanticModelNumber(node_count))
	end if
	fbSemanticModelResetLifetimes( )
end sub

'' end of tooling/semantic-lifetimes.bas
