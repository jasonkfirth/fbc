/'
    FreeBASIC DOS provider tests: freebasic-threads.bas
    Exercise compiler-selected threading with dynamic strings, per-thread
    error contexts, joins, and automatic sound shutdown at program exit.
    The companion C probe tests preemption without voluntary scheduling.
'/

#lang "fb"
#include once "string.bi"

private sub phase( byref message as const string )
	dim handle as integer = freefile
	if open( "PHASE.TXT" for append as #handle ) <> 0 then end 1
	print #handle, message
	close #handle
end sub

private sub worker( byval parameter as any ptr )
	dim failures as long ptr = parameter
	for i as integer = 1 to 2000
		dim text as string = Replace( StrReverse( "CbA" ), "abc", "done", 1, -1, fbTextCompare )
		if text <> "done" then *failures += 1
		text = Replace( "bad", "a", "b", 0 )
		if Err <> 1 then *failures += 1
		text = StrReverse( "ok" )
		if Err <> 0 or text <> "ko" then *failures += 1
	next
end sub

dim handles(0 to 3) as any ptr
dim failures(0 to 3) as long
phase( "before create" )
for i as integer = 0 to 3
	handles(i) = ThreadCreate( @worker, @failures(i) )
	if handles(i) = 0 then
		print "FAIL ThreadCreate"
		end 1
	end if
next
phase( "created workers" )
for i as integer = 0 to 3
	ThreadWait( handles(i) )
	phase( "joined " & str(i) )
	if failures(i) <> 0 then
		print "FAIL dynamic strings or error isolation"
		end 1
	end if
next
print "PASS FreeBASIC dynamic strings and thread joins"

phase( "before sound" )
sound 0, 440, 2.0, 0.5
phase( "sound started" )
dim started as double = timer
while timer - started < 0.25
wend
print "PASS FreeBASIC sound exit reached"
phase( "before END" )
'' Leave the sound worker active. The harness requires return to DOS too.
end 0

' end of freebasic-threads.bas
