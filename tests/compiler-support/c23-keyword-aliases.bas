/'
    Project: FreeBASIC C backend regression tests
    File: c23-keyword-aliases.bas
    Purpose: Check C23 keyword spellings used as external object names.
    Responsibilities: Call external functions and read external variables.
    This fixture does not define the external C helper objects.
'/

extern "C"
    declare function f_alignas cdecl alias "alignas" (byval value as long) as long
    declare function f_alignof cdecl alias "alignof" (byval value as long) as long
    declare function f_bool cdecl alias "bool" (byval value as long) as long
    declare function f_static_assert cdecl alias "static_assert" (byval value as long) as long
    declare function f_thread_local cdecl alias "thread_local" (byval value as long) as long
    declare function f_true cdecl alias "true" (byval value as long) as long
    declare function f_typeof_unqual cdecl alias "typeof_unqual" (byval value as long) as long
    extern false_value alias "false" as long
    extern nullptr_value alias "nullptr" as long

    function f_constexpr cdecl alias "constexpr" (byval value as long) as long
        return value + 10
    end function
end extern

const expected_sum as long = 55
dim as long result_ = f_alignas(0) + f_alignof(0) + f_bool(0) + f_static_assert(0) + _
    f_thread_local(0) + f_true(0) + f_typeof_unqual(0) + false_value + nullptr_value + f_constexpr(0)
if result_ <> expected_sum then end 1
print "C23_KEYWORD_ALIASES_PASS"

/' end of c23-keyword-aliases.bas '/
