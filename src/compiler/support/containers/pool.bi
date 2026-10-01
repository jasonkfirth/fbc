'' Project: FreeBASIC compiler - compiler storage containers
'' -----------------------------------------
''
'' File: support/containers/pool.bi
''
'' Purpose:
''
''     Define size-class pools and the metadata needed to free their payloads.
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

#ifndef __POOL_BI__
#define __POOL_BI__

#include once "support/containers/list.bi"

type TPOOLITEM
	idx         as integer
end type

type TPOOL
	chunks      as integer
	chunksize   as integer
	chunkTb     as TLIST ptr
end type

declare sub poolInit _
	( _
		byval pool as TPOOL ptr, _
		byval items as integer, _
		byval minlen as integer, _
		byval maxlen as integer _
	)


declare sub poolEnd(byval pool as TPOOL ptr)

declare function poolNewItem _
	( _
		byval pool as TPOOL ptr, _
		byval len_ as integer _
	) as any ptr

declare sub poolDelItem _
	( _
		byval pool as TPOOL ptr, _
		byval node as any ptr _
	)


#endif '' __POOL_BI__

'' end of support/containers/pool.bi
