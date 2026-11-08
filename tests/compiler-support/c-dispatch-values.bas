/'
    Project: FreeBASIC C dispatch qualification
    File: c-dispatch-values.bas
    Purpose: Exercise real procedure continuations around the dispatch block.
    Responsibilities: Check return values, fallthrough, recursion and local handlers.
    This file intentionally does NOT transfer into a different procedure activation.
'/
#lang "fblite"
option gosub
#include once "c-opaque-nop.bi"

private function resumed_value(byval seed as integer) as integer
    dim values(0 to 3) as integer
    dim text_value as string = "live"
    values(3) = seed + 10
    on local error goto handler
    error 24
    if text_value <> "live" then end 41
    function = values(3)
    exit function
handler:
    if err <> 24 then end 42
    resume next
end function

private function retry_value() as integer
    dim attempts as integer
    on local error goto handler
    error 24
    function = attempts
    exit function
handler:
    if err <> 24 then end 43
    attempts += 1
    if attempts = 1 then
        resume
    else
        resume next
    end if
end function

private sub fallthrough_value(byref result_value as integer)
    dim values(0 to 3) as integer
    values(3) = 19
    on local error goto handler
    compiler_test_opaque_nop()
    error 24
    result_value = values(3)
    goto done
handler:
    if err <> 24 then end 44
    resume next
done:
end sub

'' Single-threaded GOSUB trace, owned only by this fixture. The separate
'' automatic-local variant retains the accepted provider's lifetime failure.
dim shared as integer gosub_trace

private function gosub_value() as integer
#ifdef AUTOMATIC_GOSUB_PROOF
    dim result_value as integer = 3
    gosub add_value
    function = result_value
    exit function
add_value:
    result_value += 4
    return
#else
    gosub_trace = 3
    gosub add_value
    function = gosub_trace
    exit function
add_value:
    gosub_trace += 4
    return
#endif
end function

private function recursive_value(byval depth as integer) as integer
    dim values(0 to 3) as integer
    values(3) = depth + 1
    if depth > 0 then values(3) += recursive_value(depth - 1)
    on local error goto handler
    error 24
    function = values(3)
    exit function
handler:
    if err <> 24 then end 45
    resume next
end function

private sub terminate_value()
    on local error goto handler
    error 24
    print "DISPATCH_TERMINATE_PASS"
    end 0
handler:
    if err <> 24 then end 46
    resume next
end sub

#ifdef TERMINATE_PROOF
    terminate_value()
    end 47
#else
    for seed as integer = 1 to 4
        if resumed_value(seed) <> seed + 10 then end 48
    next
    if retry_value() <> 2 then end 49
    dim result_value as integer
    fallthrough_value(result_value)
    if result_value <> 19 then end 50
    if recursive_value(3) <> 10 then end 52
    if gosub_value() <> 7 then end 51
    print "DISPATCH_VALUES_PASS"
    end 0
#endif

' end of c-dispatch-values.bas
