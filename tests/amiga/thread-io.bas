'' FreeBASIC classic AmigaOS thread qualification
'' ---------------------------------------------
'' File: thread-io.bas
'' Purpose: Check inherited native streams and redirected output from threads.
'' Responsibilities: Exercise nested creation, joined cleanup and parent output.
'' This file intentionally does NOT contain: native handle or thread implementations.

extern "C"
	declare function amiga_test_redirect_output() as long
	declare function amiga_test_inherited_streams() as long
	declare sub amiga_test_restore_output()
end extern

dim shared child_status as long, nested_status as long

private sub NestedThread(byval unused as any ptr)
	nested_status = amiga_test_inherited_streams()
	print "NATIVE_NESTED_OUTPUT"
end sub

private sub ChildThread(byval unused as any ptr)
	child_status = amiga_test_inherited_streams()
	print "NATIVE_CHILD_OUTPUT"
	dim worker as any ptr = threadcreate(@NestedThread)
	if worker = 0 then
		child_status = -2
	else
		threadwait(worker)
	end if
end sub

if amiga_test_redirect_output() <> 0 then end 1
dim worker as any ptr = threadcreate(@ChildThread)
if worker = 0 then
	amiga_test_restore_output()
	end 2
end if
threadwait(worker)
print "NATIVE_PARENT_OUTPUT"
amiga_test_restore_output()
if child_status <> 0 or nested_status <> 0 then end 3

dim contents as string
if open("thread-console.txt" for binary access read as #1) <> 0 then end 4
contents = string(lof(1), 0)
get #1, , contents
close #1
if instr(contents, "NATIVE_CHILD_OUTPUT") = 0 then end 5
if instr(contents, "NATIVE_NESTED_OUTPUT") = 0 then end 6
if instr(contents, "NATIVE_PARENT_OUTPUT") = 0 then end 7
print "NATIVE_THREAD_IO_OK"
end 0

'' end of thread-io.bas
