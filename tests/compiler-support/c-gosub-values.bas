/'
    Project: FreeBASIC local C GOSUB qualification
    File: c-gosub-values.bas
    Purpose: Exercise complete local GOSUB values and control transfers.
    Responsibilities: Check managed fields, arrays, floating values, nested calls,
        computed selection, RETURN label, recursion and early owner exit.
    This file intentionally does NOT install cross-procedure ERROR handlers.
'/
#lang "fblite"
option gosub

type ValueState
    text_value as string
    count as integer
    slots(0 to 3) as integer
end type

' Owned by the single-threaded selector test; each selection must run once.
dim shared selection_reads as integer

private function choose_once() as integer
    selection_reads += 1
    function = 2
end function

private function local_values(byref incoming as ValueState) as integer
    dim state as ValueState
    dim dynamic_values() as integer
    dim fraction as double = 1.25
    dim wide_value as ulongint = &h1234567812345678ull
    state.text_value = "start"
    state.count = incoming.count
    state.slots(3) = 9
    redim dynamic_values(-2 to 2)
    dynamic_values(-2) = 17
    gosub update_values
    if state.text_value <> "start-done" then end 51
    if state.count <> incoming.count + 4 or state.slots(3) <> 27 then end 52
    if fraction <> 4.75 or wide_value <> &h1234567812345689ull then end 53
    if lbound(dynamic_values) <> -2 or dynamic_values(-2) <> 29 then end 54
    function = state.count + state.slots(3) + dynamic_values(-2)
    exit function
update_values:
    state.text_value += "-done"
    state.count += 4
    state.slots(3) *= 3
    fraction += 3.5
    wide_value += 17
    redim preserve dynamic_values(-2 to 4)
    dynamic_values(-2) = 29
    redim preserve dynamic_values(-2 to 7)
    return
end function

private function nested_values(byval depth as integer) as integer
    dim level as integer
    dim total as integer
    gosub descend
    function = total
    exit function
descend:
    level += 1
    if level < depth then gosub descend
    total += level
    level -= 1
    return
end function

private function recursive_values(byval depth as integer) as integer
    dim text_value as string = "local"
    dim result_value as integer = depth
    gosub change_value
    if depth > 0 then result_value += recursive_values(depth - 1)
    if text_value <> "local-changed" then end 55
    function = result_value
    exit function
change_value:
    result_value += 1
    text_value += "-changed"
    return
end function

private function redirected_values() as integer
    dim trace as integer
    gosub outer_call
    function = trace
    exit function
outer_call:
    trace = 2
    gosub inner_call
    end 56
redirected:
    trace = trace * 10 + 4
    return
inner_call:
    trace = trace * 10 + 3
    return redirected
end function

private function computed_values(byval selector as integer) as integer
    dim trace as integer
    on selector gosub first_call, second_call, third_call
    function = trace
    exit function
first_call:
    trace = 1
    return
second_call:
    trace = 2
    return
third_call:
    trace = 3
    return
end function

private function side_effect_selection() as integer
    dim trace as integer
    on choose_once() gosub first_call, second_call
    function = trace
    exit function
first_call:
    trace = 1
    return
second_call:
    trace = 2
    return
end function

private function early_exit() as integer
    dim text_value as string = "owner"
    gosub leave_owner
leave_owner:
    function = len(text_value)
    exit function
end function

private function error_value() as integer
    dim value as integer = 7
    on local error goto handler
    gosub fail_and_resume
    function = value
    exit function
fail_and_resume:
    error 24
    value += 5
    return
handler:
    if err <> 24 then end 57
    resume next
end function

private function retained_error() as integer
    err = 37
    gosub no_change
    function = err
    exit function
no_change:
    return
end function

dim initial as ValueState
initial.count = 6
if local_values(initial) <> 66 then end 61
if initial.count <> 6 then end 62
if nested_values(129) <> 8385 then end 63
if recursive_values(4) <> 15 then end 64
if redirected_values() <> 234 then end 65
for selector as integer = -1 to 4
    if computed_values(selector) <> iif(selector >= 1 and selector <= 3, selector, 0) then end 66
next
if side_effect_selection() <> 2 or selection_reads <> 1 then end 67
if early_exit() <> 5 then end 68
if error_value() <> 12 then end 69
if retained_error() <> 37 then end 70
print "GOSUB_VALUES_PASS"
end 0

' end of c-gosub-values.bas
