'' Project: FreeBASIC compiler - compiler storage containers
'' -----------------------------------------
''
'' File: support/containers/stack.bi
''
'' Purpose:
''
''     Define stack block storage, top-of-stack state, and LIFO operations.
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

#ifndef __STACK_BI__
#define __STACK_BI__

#include once "support/common.bi"

type TSTACKNODE
	prev    as TSTACKNODE ptr
	next    as TSTACKNODE ptr
end type

type TSTACKTB
	next    as TSTACKTB ptr
	nodetb  as TSTACKNODE ptr
	nodes   as integer
end type

type TSTACK
	tbhead  as TSTACKTB ptr
	tbtail  as TSTACKTB ptr
	nodes   as integer
	nodelen as integer
	tos     as TSTACKNODE ptr                   '' top-of-stack
	clear   as integer                          '' clear nodes?
end type

declare function stackNew _
	( _
		byval stk as TSTACK ptr, _
		byval nodes as integer, _
		byval nodelen as integer, _
		byval doclear as integer = TRUE _
	) as integer

declare function stackFree _
	( _
		byval stk as TSTACK ptr _
	) as integer

declare function stackPush _
	( _
		byval stk as TSTACK ptr _
	) as any ptr

declare sub stackPop _
	( _
		byval stk as TSTACK ptr _
	)

declare function stackGetTOS _
	( _
		byval stk as TSTACK ptr _
	) as any ptr

#endif '' _STACK_BI__

'' end of support/containers/stack.bi
