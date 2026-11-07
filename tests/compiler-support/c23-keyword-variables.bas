/'
    Project: FreeBASIC C backend regression tests
    File: c23-keyword-variables.bas
    Purpose: Exercise each new C23 keyword as an external variable name.
    Responsibilities: Read separately defined C objects and check their values.
    This fixture does not define the external variables.
'/

extern "C"
    extern value_alignas alias "alignas" as long
    extern value_alignof alias "alignof" as long
    extern value_bool alias "bool" as long
    extern value_constexpr alias "constexpr" as long
    extern value_false alias "false" as long
    extern value_nullptr alias "nullptr" as long
    extern value_static_assert alias "static_assert" as long
    extern value_thread_local alias "thread_local" as long
    extern value_true alias "true" as long
    extern value_typeof_unqual alias "typeof_unqual" as long
end extern

const expected_sum as long = 55
dim as long result_ = value_alignas + value_alignof + value_bool + value_constexpr + _
    value_false + value_nullptr + value_static_assert + value_thread_local + value_true + value_typeof_unqual
if result_ <> expected_sum then end 1
print "C23_KEYWORD_VARIABLES_PASS"

/' end of c23-keyword-variables.bas '/
