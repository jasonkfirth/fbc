'' Project: FreeBASIC C backend regression tests
'' File: stdio-contract.bas
'' Purpose: Check stdio declarations against native C stream operations.
'' Responsibilities: Exercise complete/opaque streams, procedure addresses and
''     an external object or procedure whose linker name is FILE.
'' This file intentionally does NOT inspect the C library's private stream layout.

#include once "crt/stddef.bi"
#include once "crt/long.bi"

#ifdef STDIO_OPAQUE
	type TestStream as TestStream_
	extern "C"
		declare function fopen(byval as const zstring ptr, byval as const zstring ptr) as TestStream ptr
		declare function fclose(byval as TestStream ptr) as long
		declare function fflush(byval as TestStream ptr) as long
		declare function fseek(byval as TestStream ptr, byval as clong, byval as long) as long
		declare function fread(byval as any ptr, byval as size_t, byval as size_t, byval as TestStream ptr) as size_t
		declare function fwrite(byval as const any ptr, byval as size_t, byval as size_t, byval as TestStream ptr) as size_t
	end extern
	const SEEK_SET = 0
#else
	#include once "crt/stdio.bi"
	type TestStream as FILE
#endif

#ifdef STDIO_FILE_PROC
	public function exported_file cdecl alias "FILE" () as long
		return 37
	end function
#else
	public dim shared exported_file alias "FILE" as long = 37
#endif

declare function fixture_stdio_collision cdecl alias "fixture_stdio_collision" () as long
declare function fixture_stdio_suffix cdecl alias "fixture_stdio_suffix" (byval as TestStream ptr) as long

dim as string filename = command(1)
if len(filename) = 0 then end 1
if fixture_stdio_collision() <> 37 then end 2
dim as TestStream ptr stream = fopen(strptr(filename), "w+b")
if stream = 0 then end 3

dim writer as typeof(procptr(fwrite)) = procptr(fwrite)
dim reader as typeof(procptr(fread)) = procptr(fread)
const payload = "FreeBASIC stdio"
if writer(strptr(payload), 1, len(payload), stream) <> len(payload) then end 4
if fixture_stdio_suffix(stream) <> 0 then end 5
dim as zstring * 2 suffix = "?"
if fwrite(@suffix, 1, 1, stream) <> 1 then end 6
if fflush(stream) <> 0 then end 7
if fseek(stream, 0, SEEK_SET) <> 0 then end 8

dim as zstring * 32 received = ""
const expected = payload + "!?"
if len(expected) >= sizeof(received) then end 15
if reader(@received, 1, len(expected), stream) <> len(expected) then end 9
'' fread returns bytes without a string terminator. Keep one spare byte.
received[len(expected)] = 0
if received <> expected then end 10
if fseek(stream, 0, SEEK_SET) <> 0 then end 11
received = ""
if fread(@received, 1, len(expected), stream) <> len(expected) then end 12
received[len(expected)] = 0
if received <> expected then end 13
if fclose(stream) <> 0 then end 14

'' end of stdio-contract.bas
