'' Project: FreeBASIC runtime tests
'' File: unix-exec-runtime.bas
'' Purpose: Check Unix EXEC through the ordinary BASIC runtime entry point.
'' Responsibilities: Verify quoted argument values, status and failed launches.
'' Allocation and small-stack controls belong to unix-transient-buffers.c.
#lang "fb"

if command(1) = "--exec-child" then
	if command(2) <> "two three" then end 38
	end 37
end if

dim result as long
result = exec(command(0), "--exec-child ""two three""")
if result <> 37 then end 10
result = exec("./freebasic-exec-missing-fixture", "")
if result <> -1 then end 11
print "UNIX_EXEC_RUNTIME_PASS"

'' end of unix-exec-runtime.bas
