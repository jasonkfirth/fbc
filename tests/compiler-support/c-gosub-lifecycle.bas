/'
    Project: FreeBASIC local C GOSUB qualification
    File: c-gosub-lifecycle.bas
    Purpose: Exercise emitted allocation failure, owner exit and empty RETURN.
    Responsibilities: Keep allocation observations outside the production runtime.
    This file intentionally does NOT change cross-procedure ERROR transport.
'/
#lang "fblite"
option gosub

declare sub gosub_test_arm cdecl alias "gosub_test_arm" (byval fail_next as long)
declare sub gosub_test_balanced cdecl alias "gosub_test_balanced" ()

private function owner_exit() as integer
    dim text_value as string = "owned"
    gosub leave_owner
leave_owner:
    function = len(text_value)
    exit function
end function

private sub missing_return()
    dim observed_errors as integer
#ifdef CHECKED_RETURN
    on local error goto handler
#endif
    return
#ifdef CHECKED_RETURN
    if observed_errors <> 1 then end 81
#else
    if err <> 16 then end 81
#endif
    return unexpected
#ifdef CHECKED_RETURN
    if observed_errors <> 2 then end 82
#else
    if err <> 16 then end 82
#endif
    exit sub
unexpected:
    end 83
#ifdef CHECKED_RETURN
handler:
    if err <> 16 then end 84
    observed_errors += 1
    resume next
#endif
end sub

gosub_test_arm(0)
#ifdef ALLOCATION_FAILURE
gosub_test_arm(1)
gosub target
end 85
target:
return
#else
for iteration as integer = 1 to 1000
    if owner_exit() <> 5 then end 86
next
missing_return()
gosub_test_balanced()
print "GOSUB_LIFECYCLE_PASS"
end 0
#endif

' end of c-gosub-lifecycle.bas
