/'
    Project: FreeBASIC C backend regression tests
    File: c23-private-parameters.bas
    Purpose: Check unused parameters in directly called private procedures.
    Responsibilities: Preserve argument evaluation and the callable signature.
    This fixture does not provide an external callback interface.
'/

dim shared as long side_effects

private function next_value() as long
    side_effects += 1
    return side_effects
end function

private function direct_value cdecl( byval value as long, byval ignored as long ) as long
    return value
end function

private function direct_vararg cdecl( byval value as long, byval ignored as long, ... ) as long
    return value
end function

private function address_value cdecl( byval value as long, byval ignored as long ) as long
    return value + 1
end function

private sub write_ref cdecl( byref value as long, byval ignored as long )
    value = 12
end sub

private function factorial cdecl( byval value as long, byval ignored as long ) as long
    if value <= 1 then return 1
    return value * factorial(value - 1, next_value())
end function

if direct_value(7, next_value()) <> 7 orelse side_effects <> 1 then end 1
if direct_vararg(9, next_value(), 21) <> 9 orelse side_effects <> 2 then end 2
dim as function cdecl(byval as long, byval as long) as long callback = @address_value
if callback(3, next_value()) <> 4 orelse side_effects <> 3 then end 3
dim as long referenced
write_ref(referenced, next_value())
if referenced <> 12 orelse side_effects <> 4 then end 4
if factorial(4, next_value()) <> 24 orelse side_effects <> 8 then end 5
print "C23_PRIVATE_PARAMETERS_PASS"

/' end of c23-private-parameters.bas '/
