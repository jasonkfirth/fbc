''
'' FreeBASIC Darwin CRT bindings
'' -----------------------------
''
'' File: crt/darwin/sched.bi
''
'' Purpose:
''
''     Expose the scheduling interface provided by the macOS SDK.
''
'' Responsibilities:
''
''     - mirror the Darwin sched_param ABI
''     - define the scheduling policies published by the Darwin SDK
''     - declare the three scheduling functions exported by macOS libSystem
''
'' This file intentionally does NOT contain:
''
''     - Linux CPU affinity sets or allocation helpers
''     - unsupported process scheduler and round-robin interval functions
''     - glibc internal scheduling entry points
''

#ifndef __crt_darwin_sched_bi__
#define __crt_darwin_sched_bi__

extern "C"

const _SCHED_H = 1

'' Darwin keeps four opaque bytes after the public priority field.  They are
'' part of the ABI even though applications must not interpret their contents.
type sched_param
	sched_priority as long
	__opaque(0 to 3) as byte
end type

const SCHED_OTHER = 1
const SCHED_FIFO = 4
const SCHED_RR = 2

declare function sched_yield() as long
declare function sched_get_priority_min(byval __algorithm as long) as long
declare function sched_get_priority_max(byval __algorithm as long) as long

end extern

#endif

'' end of crt/darwin/sched.bi
