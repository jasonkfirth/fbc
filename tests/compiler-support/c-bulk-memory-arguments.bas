/'
	Project: FreeBASIC C backend regression
	File: c-bulk-memory-arguments.bas
	Purpose: Retain record origins at untyped byte-argument boundaries.
	Responsibilities: Exercise tail fills, copies and overlapping moves, with
	single evaluation of the record expressions and native overrun controls.
	The paired C fixture owns independent byte and pointer comparisons.
'/
#lang "fb"
#include once "crt/string.bi"

type MemoryArgumentRecord
	prefix as ulong
	first as ushort
	following as ushort
	rest(0 to 7) as ubyte
end type

dim shared as long memoryArgumentEvaluations

declare function memoryArgumentCases cdecl alias "probe_memory_argument_cases"() as long
declare function memoryArgumentObserve cdecl alias "probe_memory_argument_observe"(byval value as const any ptr) as long

private function memoryArgumentTarget(byval value as any ptr) as MemoryArgumentRecord ptr
	memoryArgumentEvaluations += 1
	return value
end function

private function memoryArgumentSource(byval value as const any ptr) as const MemoryArgumentRecord ptr
	memoryArgumentEvaluations += 1
	return value
end function

function memoryArgumentCount cdecl alias "probe_memory_argument_count"() as long
	var result = memoryArgumentEvaluations
	memoryArgumentEvaluations = 0
	return result
end function

function memoryArgumentFill cdecl alias "probe_memory_argument_fill"(byval destination as any ptr, byval count as size_t, byval value as long) as long
	var result = memset(@(memoryArgumentTarget(destination)->first), value, count)
	return iif(result = @cptr(MemoryArgumentRecord ptr, destination)->first, -1, 0)
end function

function memoryArgumentFixedFill cdecl alias "probe_memory_argument_fixed_fill"(byval destination as any ptr, byval value as long) as long
	var result = memset(@(memoryArgumentTarget(destination)->first), value, sizeof(MemoryArgumentRecord) - sizeof(ulong))
	return iif(result = @cptr(MemoryArgumentRecord ptr, destination)->first, -1, 0)
end function

function memoryArgumentExtendedFill cdecl alias "probe_memory_argument_extended_fill"(byval destination as any ptr, byval value as long) as long
	'' The caller supplies a larger allocation containing this record prefix.
	var result = memset(@(memoryArgumentTarget(destination)->first), value, 24)
	return iif(result = @cptr(MemoryArgumentRecord ptr, destination)->first, -1, 0)
end function

function memoryArgumentCopy cdecl alias "probe_memory_argument_copy"(byval destination as any ptr, byval source as const any ptr, byval count as size_t) as long
	var result = memcpy(@(memoryArgumentTarget(destination)->first), @(memoryArgumentSource(source)->first), count)
	return iif(result = @cptr(MemoryArgumentRecord ptr, destination)->first, -1, 0)
end function

function memoryArgumentMove cdecl alias "probe_memory_argument_move"(byval destination as any ptr, byval count as size_t) as long
	var result = memmove(@(memoryArgumentTarget(destination)->following), @(memoryArgumentSource(destination)->first), count)
	return iif(result = @cptr(MemoryArgumentRecord ptr, destination)->following, -1, 0)
end function

#ifdef MEMORY_ARGUMENT_BAD_EXTENT
private sub memoryArgumentOverrun()
	dim as MemoryArgumentRecord value
	'' Thirteen bytes starting at offset four exceed the sixteen-byte record.
	memset(@value.first, 0, 13)
	memoryArgumentObserve(@value)
end sub
memoryArgumentOverrun()
#endif

#ifndef MEMORY_ARGUMENT_EXPORTS
assert(memoryArgumentCases() = -1)
print "BULK_MEMORY_ARGUMENTS_PASS"
#endif

/' end of c-bulk-memory-arguments.bas '/
