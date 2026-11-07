/'
    Project: FreeBASIC C backend regression tests
    File: tls-keyword-private.bas
    Purpose: Check a C23 keyword thread-local object in a single module.
    Responsibilities: Exercise the storage from two procedures and threads.
    This fixture does not test a separately compiled consumer.
'/

extern "C"
    thread local dim shared private_value alias "false" as long = 19
end extern

dim shared as long worker_result

private function read_value() as long
    return private_value
end function

private sub worker( byval ignored as any ptr )
    if read_value() <> 19 then worker_result = 1 : exit sub
    private_value = 29
    if read_value() <> 29 then worker_result = 2
end sub

if read_value() <> 19 then end 1
dim as any ptr thread_ = threadcreate(@worker, 0)
if thread_ = 0 then end 2
threadwait(thread_)
if worker_result <> 0 orelse read_value() <> 19 then end 3
print "KEYWORD_PRIVATE_TLS_PASS"

/' end of tls-keyword-private.bas '/
