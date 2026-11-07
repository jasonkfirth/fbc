/'
    Project: FreeBASIC C backend regression tests
    File: driver-warning-probe.bas
    Purpose: Exercise an observable C uninitialized-variable diagnostic.
    Responsibilities: Compile a conditional assignment through the C driver.
    This fixture is compiled only and is not executed.
'/

function conditional_value cdecl( byval condition as long ) as long
    dim as long value = any
    if condition <> 0 then value = 7
    return value
end function

print conditional_value(0)

/' end of driver-warning-probe.bas '/
