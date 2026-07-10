''
'' FreeBASIC macOS CRT tests
'' ------------------------
''
'' File: sched-darwin-smoke.bas
''
'' Purpose:
''
''     Verify that the FreeBASIC scheduling declarations match the macOS SDK
''     ABI and link to the scheduling API exported by macOS libSystem.
''
'' Responsibilities:
''
''     - check the SDK-derived sched_param layout and policy constants
''     - query the supported priority bounds
''     - yield the current thread without changing scheduling policy
''
'' This file intentionally does NOT contain:
''
''     - Linux CPU affinity or glibc internal scheduling APIs
''     - changes to the process or thread scheduling policy
''

#include once "crt/sched.bi"

type SchedParamAlignmentProbe
	prefix as ubyte
	value as sched_param
end type

const SMOKE_OK = 0
const SMOKE_PRIORITY_QUERY_FAILED = 1
const SMOKE_LAYOUT_ACCESS_FAILED = 2
const SMOKE_YIELD_FAILED = 3

#assert sizeof(sched_param) = 8
#assert offsetof(sched_param, sched_priority) = 0
#assert offsetof(sched_param, __opaque(0)) = 4
#assert offsetof(SchedParamAlignmentProbe, value) = 4

#assert SCHED_OTHER = 1
#assert SCHED_FIFO = 4
#assert SCHED_RR = 2

dim priority_minimum as long = sched_get_priority_min(SCHED_OTHER)
dim priority_maximum as long = sched_get_priority_max(SCHED_OTHER)

if( priority_minimum < 0 or priority_maximum < priority_minimum ) then
	end SMOKE_PRIORITY_QUERY_FAILED
end if

dim scheduling_parameters as sched_param
scheduling_parameters.sched_priority = priority_minimum

if( scheduling_parameters.sched_priority <> priority_minimum ) then
	end SMOKE_LAYOUT_ACCESS_FAILED
end if

if( sched_yield() <> 0 ) then
	end SMOKE_YIELD_FAILED
end if

end SMOKE_OK

'' end of sched-darwin-smoke.bas
