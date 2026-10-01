'' Project: FreeBASIC compiler - compiler storage containers
'' -----------------------------------------
''
'' File: support/containers/stack.bas
''
'' Purpose:
''
''     Provide block-allocated LIFO payloads for nested compiler operations.
''
'' Responsibilities:
''
''     - manage container-owned storage and its reuse policy
''     - expose stable traversal and allocation contracts to callers
''
'' This file intentionally does NOT contain:
''
''     - ownership of pointers stored inside caller payloads
''

'' generic stack
''
'' chng: jan/2005 written [v1ctor]
''

#include once "support/containers/stack.bi"

declare function hAllocTB _
	( _
		byval stk as TSTACK ptr, _
		byval nodes as integer _
	) as integer

'':::::
function stackNew _
	( _
		byval stk as TSTACK ptr, _
		byval nodes as integer, _
		byval nodelen as integer, _
		byval doclear as integer _
	) as integer

	'' fill ctrl struct
	stk->tbhead     = NULL
	stk->tbtail     = NULL
	stk->nodes      = 0
	'' Consecutive payloads must preserve pointer alignment on hosts that
	'' reject misaligned loads. Use the same stride policy as pooled lists.
	stk->nodelen    = xcheckedAdd(xcheckedAlign(nodelen, len(any ptr)), len(TSTACKNODE))
	stk->tos        = NULL
	stk->clear      = doclear

	'' allocate the initial pool
	function = hAllocTB( stk, nodes )

end function

'':::::
function stackFree _
	( _
		byval stk as TSTACK ptr _
	) as integer

	dim as TSTACKTB ptr tb, nxt

	'' for each pool, free the mem block and the pool ctrl struct
	tb = stk->tbhead
	do while( tb <> NULL )
		nxt = tb->next
		deallocate( tb->nodetb )
		deallocate( tb )
		tb = nxt
	loop

	stk->tbhead     = NULL
	stk->tbtail     = NULL
	stk->nodes      = 0
	stk->tos        = NULL

	function = TRUE

end function

'':::::
private function hAllocTB _
	( _
		byval stk as TSTACK ptr, _
		byval nodes as integer _
	) as integer static

	dim as TSTACKNODE ptr nodetb, node, prev
	dim as TSTACKTB ptr tb
	dim as integer i

	function = FALSE

	if( nodes <= 0 ) then
		exit function
	end if

	'' allocate the pool
	if( stk->clear ) then
		nodetb = xcallocate( xcheckedMultiply(nodes, stk->nodelen) )
	else
		nodetb = xallocate( xcheckedMultiply(nodes, stk->nodelen) )
	end if

	'' and the pool ctrl struct
	tb = xallocate( len( TSTACKTB ) )

	'' add the ctrl struct to pool list
	if( stk->tbhead = NULL ) then
		stk->tbhead = tb
	end if
	if( stk->tbtail <> NULL ) then
		stk->tbtail->next = tb
	end if
	stk->tbtail = tb

	tb->next = NULL
	tb->nodetb = nodetb
	tb->nodes = nodes

	'' relink
	stk->nodes = xcheckedAdd(stk->nodes, nodes)

	prev = stk->tos
	node = nodetb
	if( prev <> NULL ) then
		prev->next = node
	end if

	for i = 1 to nodes-1
		node->prev = prev
		node->next = cast( TSTACKNODE ptr, cast( byte ptr, node ) + stk->nodelen)
		prev = node
		node = node->next
	next

	node->prev = prev
	node->next = NULL

	''
	function = TRUE

end function

'':::::
function stackPush _
	( _
		byval stk as TSTACK ptr _
	) as any ptr static

	'' move up
	if( stk->tos = NULL ) then
		'' An empty initial pool is valid. Allocate before dereferencing it.
		if( stk->tbhead = NULL ) then
			hAllocTB( stk, 1 )
		end if
		stk->tos = stk->tbhead->nodetb
	else
		'' alloc new node if there are no free nodes
		if( stk->tos->next = NULL ) Then
			hAllocTB( stk, xgrowthCount(stk->nodes, 4) )
		end if

		stk->tos = stk->tos->next
	end if

	function = cast( byte ptr, stk->tos ) + len( TSTACKNODE )

end function

'':::::
sub stackPop _
	( _
		byval stk as TSTACK ptr _
	) static

	assert( stk->tos <> NULL )
	if( stk->tos = NULL ) then
		exit sub
	end if

	'' node can contain strings descriptors, so, erase it..
	if( stk->clear ) then
		clear( byval cast(TSTACKNODE ptr, stk->tos) + 1, 0, stk->nodelen - len( TSTACKNODE ) )
	end if

	'' move down
	stk->tos = stk->tos->prev

end sub

'':::::
function stackGetTOS _
	( _
		byval stk as TSTACK ptr _
	) as any ptr

	if( stk->tos = NULL ) then
		return NULL
	else
		return cast( byte ptr, stk->tos ) + len( TSTACKNODE )
	end if

end function

'' end of support/containers/stack.bas
