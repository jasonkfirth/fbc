/'
    Project: FreeBASIC compiler regression tests
    File: tls-keyword-consumer.bas
    Purpose: Check a C23 keyword thread-local external name across modules.
    Responsibilities: Read and update the value on two runtime threads.
    This fixture does not define the external storage.
'/

extern "C"
    thread local extern external_value alias "bool" as long
    declare function tls_keyword_read cdecl () as long
    declare function tls_keyword_c_read cdecl () as long
end extern

dim shared as long worker_result

private sub worker( byval ignored as any ptr )
    if external_value <> 17 orelse tls_keyword_read() <> 17 orelse tls_keyword_c_read() <> 17 then worker_result = 1 : exit sub
    external_value = 33
    if external_value <> 33 orelse tls_keyword_read() <> 33 orelse tls_keyword_c_read() <> 33 then worker_result = 2
end sub

if external_value <> 17 orelse tls_keyword_read() <> 17 orelse tls_keyword_c_read() <> 17 then end 1
dim as any ptr thread_ = threadcreate(@worker, 0)
if thread_ = 0 then end 2
threadwait(thread_)
if worker_result <> 0 orelse external_value <> 17 orelse tls_keyword_read() <> 17 orelse tls_keyword_c_read() <> 17 then end 3
print "KEYWORD_TLS_TEST_PASS"

/' end of tls-keyword-consumer.bas '/
