'' Project: FreeBASIC compiler - semantic block observations
'' File: tooling/semantic-blocks.bas
'' Purpose: Retain block boundaries and exits before native break lists are consumed.
'' Responsibilities: Copy selected owners, targets, statement intervals and cleanup order.
'' This file intentionally does NOT contain: branch validation or lifetime inference.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-blocks.bi"
#include once "tooling/semantic-constructs.bi"

'' Modules are compiled serially. These bounded counters belong to the current
'' module; the observer never retains native block, symbol or list addresses.
private const BLOCK_LIMIT = 1000000
private dim shared as longint procedure_count, scope_count, exit_count, cleanup_count
private dim shared as longint origin_count, cleanup_node_count

sub fbSemanticModelResetBlocks( )
	procedure_count = 0
	scope_count = 0
	exit_count = 0
	cleanup_count = 0
	origin_count = 0
	cleanup_node_count = 0
end sub

private function hNext( byref count as longint ) as longint
	if( count >= BLOCK_LIMIT ) then
		fbSemanticModelFailAt("native block observation exceeds its bound")
		return 0
	end if
	count += 1
	return count
end function

private function hSymbol( byval sym as FBSYMBOL ptr, byval kind as FB_SYMBCLASS ) as longint
	if( (sym = NULL) orElse (sym->class <> kind) ) then
		fbSemanticModelFailAt("native block observation has another symbol class")
		return 0
	end if
	return fbSemanticModelSymbolId(sym)
end function

private function hOwner( byval sym as FBSYMBOL ptr ) as longint
	if( sym = NULL ) then
		fbSemanticModelFailAt("native block owner is unavailable")
		return 0
	end if
	if( (sym->class <> FB_SYMBCLASS_PROC) and (sym->class <> FB_SYMBCLASS_SCOPE) ) then
		fbSemanticModelFailAt("native block owner is not a procedure or scope")
		return 0
	end if
	return fbSemanticModelSymbolId(sym)
end function

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hInterval( byval first as integer, byval last as integer ) as integer
	if( (first < 0) or (last < first) ) then
		fbSemanticModelFailAt("native block statement interval is reversed")
		return FALSE
	end if
	return TRUE
end function

'' -------------------------------------------------------------------------
'' Complete native block snapshots
'' -------------------------------------------------------------------------

private sub hOrigin( byval node as ASTNODE ptr, byval symbol_id as longint, byref kind as const string )
	if( (node->semantic_statement < 0) or (node->semantic_source < 0) or (node->semantic_context < 0) ) then
		fbSemanticModelFailAt("native block origin has a negative identity")
		exit sub
	end if
	dim as longint identity = hNext(origin_count)
	if( identity = 0 ) then exit sub
	'' These fields were copied by astNewNode at construction, before scope
	'' lowering or a later include/option change. Zero preserves an absent
	'' source/statement for generated wrappers instead of inventing a site.
	hProperty("native-block-origin:" + fbSemanticModelNumber(identity), kind + TABCHAR + _
		fbSemanticModelNumber(symbol_id) + TABCHAR + fbSemanticModelNumber(node->semantic_statement) + _
		TABCHAR + fbSemanticModelNumber(node->semantic_source) + TABCHAR + fbSemanticModelNumber(node->semantic_context))
end sub

sub fbSemanticModelProcedureBlock( byval node as ASTNODE ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (node = NULL) orElse (node->class <> AST_NODECLASS_PROC) ) then
		fbSemanticModelFailAt("procedure block observation has another node class")
		exit sub
	end if
	if( hInterval(node->block.inistmt, node->block.endstmt) = FALSE ) then exit sub
	dim as longint proc_id = hSymbol(node->sym, FB_SYMBCLASS_PROC)
	dim as longint entry_id = hSymbol(node->block.initlabel, FB_SYMBCLASS_LABEL)
	dim as longint exit_id = hSymbol(node->block.exitlabel, FB_SYMBCLASS_LABEL)
	if( (proc_id = 0) or (entry_id = 0) or (exit_id = 0) ) then exit sub
	dim as longint identity = hNext(procedure_count)
	if( identity = 0 ) then exit sub
	hProperty("native-procedure-block:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(proc_id) + _
		TABCHAR + fbSemanticModelNumber(entry_id) + TABCHAR + fbSemanticModelNumber(exit_id) + _
		TABCHAR + fbSemanticModelNumber(node->block.inistmt) + TABCHAR + fbSemanticModelNumber(node->block.endstmt) + _
		TABCHAR + fbSemanticModelNumber(abs(node->block.proc.ismain <> FALSE)))
	hOrigin(node, proc_id, "procedure")
end sub

sub fbSemanticModelScopeBlock( byval node as ASTNODE ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (node = NULL) orElse (node->class <> AST_NODECLASS_SCOPEBEGIN) ) then
		fbSemanticModelFailAt("scope block observation has another node class")
		exit sub
	end if
	if( (node->block.parent = NULL) or (ast.proc.curr = NULL) ) then
		fbSemanticModelFailAt("scope block observation has no native parent")
		exit sub
	end if
	if( hInterval(node->block.inistmt, node->block.endstmt) = FALSE ) then exit sub
	dim as longint scope_id = hSymbol(node->sym, FB_SYMBCLASS_SCOPE)
	dim as longint parent_id = hOwner(node->block.parent->sym)
	dim as longint proc_id = hSymbol(ast.proc.curr->sym, FB_SYMBCLASS_PROC)
	if( (scope_id = 0) or (parent_id = 0) or (proc_id = 0) ) then exit sub
	dim as longint identity = hNext(scope_count)
	if( identity = 0 ) then exit sub
	hProperty("native-scope-block:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(scope_id) + _
		TABCHAR + fbSemanticModelNumber(parent_id) + TABCHAR + fbSemanticModelNumber(proc_id) + _
		TABCHAR + fbSemanticModelNumber(node->block.inistmt) + TABCHAR + fbSemanticModelNumber(node->block.endstmt))
	hOrigin(node, scope_id, "scope")
end sub

'' -------------------------------------------------------------------------
'' Native exits and the cleanup actually selected for each exit
'' -------------------------------------------------------------------------

function fbSemanticModelScopeExit( byval node as ASTNODE ptr ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	if( (node = NULL) orElse (node->class <> AST_NODECLASS_SCOPE_BREAK) ) then
		fbSemanticModelFailAt("scope exit observation has another node class")
		return 0
	end if
	if( (node->break.parent = NULL) or (ast.proc.curr = NULL) or _
		(node->break.scope < 0) or (node->break.stmtnum < 0) or (node->break.linenum < 0) ) then
		fbSemanticModelFailAt("scope exit observation has unavailable native inputs")
		return 0
	end if
	dim as longint proc_id = hSymbol(ast.proc.curr->sym, FB_SYMBCLASS_PROC)
	dim as longint owner_id = hOwner(node->break.parent->sym)
	dim as longint target_id = hSymbol(node->sym, FB_SYMBCLASS_LABEL)
	if( (proc_id = 0) or (owner_id = 0) or (target_id = 0) ) then return 0
	dim as longint identity = hNext(exit_count)
	if( identity = 0 ) then return 0
	hProperty("native-scope-exit:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(proc_id) + _
		TABCHAR + fbSemanticModelNumber(owner_id) + TABCHAR + fbSemanticModelNumber(target_id) + _
		TABCHAR + fbSemanticModelNumber(node->break.scope) + TABCHAR + fbSemanticModelNumber(node->break.stmtnum) + _
		TABCHAR + fbSemanticModelNumber(node->break.linenum) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentStatement( )))
	return identity
end function

sub fbSemanticModelExitCleanup( byval branch as ASTNODE ptr, byval variable as FBSYMBOL ptr, byval owner as FBSYMBOL ptr, byval action as ASTNODE ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (branch = NULL) orElse (branch->class <> AST_NODECLASS_SCOPE_BREAK) ) then
		fbSemanticModelFailAt("exit cleanup observation has another source node class")
		exit sub
	end if
	if( (branch->break.semantic_identity <= 0) or (branch->break.semantic_identity > exit_count) or _
		(branch->break.semantic_cleanup_count < 0) or (branch->break.semantic_cleanup_count >= BLOCK_LIMIT) ) then
		fbSemanticModelFailAt("exit cleanup has no observed scope exit")
		exit sub
	end if
	dim as longint variable_id = hSymbol(variable, FB_SYMBCLASS_VAR), owner_id = hOwner(owner)
	if( (variable_id = 0) or (owner_id = 0) ) then exit sub
	dim as longint identity = hNext(cleanup_count)
	if( identity = 0 ) then exit sub
	if( action <> NULL ) then
		if( action->semantic_exit_cleanup <> 0 ) then
			fbSemanticModelFailAt("native cleanup root already has an exit owner")
			exit sub
		end if
		action->semantic_exit_cleanup = identity
	end if
	branch->break.semantic_cleanup_count += 1
	hProperty("native-exit-cleanup:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(branch->break.semantic_identity) + _
		TABCHAR + fbSemanticModelNumber(branch->break.semantic_cleanup_count) + TABCHAR + fbSemanticModelNumber(variable_id) + _
		TABCHAR + fbSemanticModelNumber(owner_id))
	hProperty("native-cleanup-insertion:" + fbSemanticModelNumber(identity), fbSemanticModelNumber(abs(action <> NULL)))
end sub

sub fbSemanticModelCleanupNode( byval node as ASTNODE ptr, byval identity as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (node = NULL) or (identity <= 0) ) then
		fbSemanticModelFailAt("native cleanup node has unavailable inputs")
		exit sub
	end if
	if( node->semantic_exit_cleanup = 0 ) then exit sub
	if( (node->semantic_exit_cleanup < 0) or (node->semantic_exit_cleanup > cleanup_count) ) then
		fbSemanticModelFailAt("native cleanup node has no selected cleanup")
		exit sub
	end if
	if( hNext(cleanup_node_count) = 0 ) then exit sub
	fbSemanticModelAppendDetail("K" + TABCHAR + "node" + TABCHAR + fbSemanticModelNumber(identity) + _
		TABCHAR + "native-exit-cleanup" + TABCHAR + fbSemanticModelNumber(node->semantic_exit_cleanup))
end sub

sub fbSemanticModelScopeExitEnd( byval node as ASTNODE ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (node = NULL) orElse (node->class <> AST_NODECLASS_SCOPE_BREAK) ) then
		fbSemanticModelFailAt("scope exit closure has another native node class")
		exit sub
	end if
	if( (node->break.semantic_identity <= 0) or (node->break.semantic_identity > exit_count) or _
		(node->break.semantic_cleanup_count < 0) or (node->break.semantic_cleanup_count > BLOCK_LIMIT) ) then
		fbSemanticModelFailAt("scope exit closure has no observed exit")
		exit sub
	end if
	hProperty("native-scope-exit-end:" + fbSemanticModelNumber(node->break.semantic_identity), _
		fbSemanticModelNumber(node->break.semantic_cleanup_count))
end sub

sub fbSemanticModelFinishBlocks( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		hProperty("native-block-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(procedure_count) + TABCHAR + fbSemanticModelNumber(scope_count) + _
			TABCHAR + fbSemanticModelNumber(exit_count) + TABCHAR + fbSemanticModelNumber(cleanup_count))
		hProperty("native-block-provenance-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(origin_count) + TABCHAR + fbSemanticModelNumber(cleanup_node_count))
	end if
	fbSemanticModelResetBlocks( )
end sub

'' end of tooling/semantic-blocks.bas
