/'
    Project: FreeBASIC C backend regression tests
    File: c23-keyword-defined.bas
    Purpose: Export a variable whose linker name is a C23 keyword.
    Responsibilities: Check reads and writes through separately compiled C.
    This fixture does not define the C helper functions.
'/

const initial_value as long = 7
const changed_value as long = 9

extern "C"
    public dim shared exported_value alias "thread_local" as long = initial_value
    declare function c_keyword_read cdecl () as long
    declare sub c_keyword_write cdecl (byval value as long)
end extern

if c_keyword_read() <> initial_value then end 1
c_keyword_write(changed_value)
if exported_value <> changed_value then end 2
print "C23_KEYWORD_DEFINED_PASS"

/' end of c23-keyword-defined.bas '/
