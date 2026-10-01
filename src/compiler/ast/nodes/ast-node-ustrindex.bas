'' Project: FreeBASIC compiler - UTF-8 scalar access
'' File: ast/nodes/ast-node-ustrindex.bas
'' Purpose: Preserve USTRING indexed reads and writes until runtime lowering.
'' Responsibilities: Convert scalar access into checked descriptor operations.
'' This file intentionally does NOT contain: UTF-8 decoding or raw byte access.

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "ast/ast.bi"
#include once "backend/ir.bi"
#include once "runtime/rtl.bi"

'' An indexed scalar has no stable address. Changing its encoded width can
'' reallocate the descriptor's buffer, so it must never become a byte lvalue.
function astLoadUstrIndex( byval n as ASTNODE ptr ) as IRVREG ptr
	var proc = astNewCALL( PROCLOOKUP( USTRINDEXGET ) )
	if( astNewARG( proc, n->l ) = NULL ) then return NULL
	if( astNewARG( proc, n->r ) = NULL ) then return NULL
	n->l = NULL
	n->r = NULL
	function = astLoad( proc )
	astDelNode( proc )
end function

function astBuildUstrIndexAssign( byval n as ASTNODE ptr, byval value as ASTNODE ptr ) as ASTNODE ptr
	if( astCanTakeAddrOf( n->l ) = FALSE ) then return NULL
	if( astGetDataClass( value ) >= FB_DATACLASS_STRING ) then return NULL
	var proc = astNewCALL( PROCLOOKUP( USTRINDEXSET ) )
	if( astNewARG( proc, n->l ) = NULL ) then return NULL
	if( astNewARG( proc, n->r ) = NULL ) then return NULL
	if( astNewARG( proc, value ) = NULL ) then return NULL
	n->l = NULL
	n->r = NULL
	astDelNode( n )
	return proc
end function

'' end of ast/nodes/ast-node-ustrindex.bas
