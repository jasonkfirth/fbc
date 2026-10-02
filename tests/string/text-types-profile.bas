'' Project: FreeBASIC profiler Unicode tests
'' File: text-types-profile.bas
'' Purpose: Verify profiler name and filename paths for all string types.
'' Responsibilities: Check conversions, bounded output, and report creation.
'' This file intentionally does NOT measure profiling performance.

' TEST_MODE : COMPILE_AND_RUN_OK
#include once "fbc-int/profile.bi"
#include once "file.bi"

dim as ustring filename = "text-profile-" + uchr(&hE9, &h1F600) + ".tmp"
dim as wstring * 128 wide = wstr(filename), wideResult
dim as string bytes = cast(string, filename)
dim as zstring * 1024 byteResult
dim as ustring resultText
FBC.ProfileInit()
assert(FBC.ProfileSetFileName(bytes) = 0)
assert(FBC.ProfileSetFileName(wide) = 0)
assert(FBC.ProfileSetFileName(filename) = 0)
assert(FBC.ProfileGetFileName(byteResult, 1024) = 0)
assert(byteResult = bytes)
assert(FBC.ProfileGetFileName(wideResult, 128) = 0)
assert(wideResult = wide)
assert(FBC.ProfileGetFileName(resultText) = 0)
assert(resultText = filename)
assert(FBC.ProfileSetFileName(uchr(&hE9, &h1F600) + ".tmp") = 0)
assert(FBC.ProfileGetFileName(resultText, 3) = 0)
assert(resultText = uchr(&hE9))
assert(FBC.ProfileGetFileName(resultText, 2) = 0)
assert(len(resultText) = 0)
assert(FBC.ProfileGetFileName(wideResult, 2) = 0)
assert(wideResult = wchr(&hE9))
assert(FBC.ProfileGetFileName(wideResult, 1) = 0)
assert(len(wideResult) = 0)
assert(FBC.ProfileSetFileName(filename) = 0)
FBC.ProfileIgnore(wide)
FBC.ProfileIgnore(filename)
dim as any ptr context = FBC.ProfileBeginProc(wide)
FBC.ProfileEndProc(context)
context = FBC.ProfileBeginCall(filename)
FBC.ProfileEndCall(context)
assert(FBC.ProfileEnd(0) = 0)
assert(FileExists(filename))
kill filename
print "text-types profiler: Unicode names and bounded copies passed"

'' end of text-types-profile.bas
