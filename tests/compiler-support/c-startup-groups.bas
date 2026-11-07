'' Project: FreeBASIC C backend tests
'' File: c-startup-groups.bas
'' Purpose: Make callback order observable at default and explicit priorities.
'' Responsibilities: Constructors, destructors and independent linked units.
'' This file intentionally does NOT depend on managed runtime initialization.

#ifdef SECOND_STARTUP_UNIT
	const STARTUP_OFFSET = 100
#else
	const STARTUP_OFFSET = 0
#endif

extern "c"
	declare sub StartupMark alias "startup_mark"(byval value as long)
end extern

private sub EarlyFirst() constructor 101
	StartupMark(STARTUP_OFFSET + 11)
end sub

private sub EarlySecond() constructor 101
	StartupMark(STARTUP_OFFSET + 12)
end sub

private sub DefaultFirst() constructor
	StartupMark(STARTUP_OFFSET + 21)
end sub

private sub DefaultSecond() constructor
	StartupMark(STARTUP_OFFSET + 22)
end sub

private sub PriorityCleanupFirst() destructor 101
	StartupMark(STARTUP_OFFSET + 31)
end sub

private sub PriorityCleanupSecond() destructor 101
	StartupMark(STARTUP_OFFSET + 32)
end sub

private sub DefaultCleanupFirst() destructor
	StartupMark(STARTUP_OFFSET + 41)
end sub

private sub DefaultCleanupSecond() destructor
	StartupMark(STARTUP_OFFSET + 42)
end sub

'' end of c-startup-groups.bas
