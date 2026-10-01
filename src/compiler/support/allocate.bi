'' Project: FreeBASIC compiler support
'' -----------------------------------------
''
'' File: support/allocate.bi
''
'' Purpose:
''
''     Define checked storage sizes and allocation for compiler-owned memory.
''
'' Responsibilities:
''
''     - reject invalid sizes before arithmetic or allocation
''     - keep allocation failure behavior consistent across compiler subsystems
''
'' This file intentionally does NOT contain:
''
''     - container layouts or ownership of caller payloads
''

#ifndef __ALLOCATE_BI__
#define __ALLOCATE_BI__

'' Compiler counts and pointer offsets use signed, host-sized INTEGER values.
'' The allocator must accept that same width, including on 64-bit hosts.
const XALLOC_MAX_SIZE as integer = (not cuint(0)) shr 1

declare function xcheckedAdd(byval a as integer, byval b as integer) as integer
declare function xcheckedMultiply(byval count as integer, byval size as integer) as integer
declare function xcheckedAlign(byval size as integer, byval alignment as integer) as integer
declare function xgrowthCount(byval count as integer, byval divisor as integer) as integer

declare function xallocate(byval size as integer) as any ptr
declare function xcallocate(byval size as integer) as any ptr
declare function xreallocate(byval old as any ptr, byval size as integer) as any ptr

#endif

'' end of support/allocate.bi
