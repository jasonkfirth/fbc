/'
    Project: FreeBASIC C backend regression
    File: c-procedure-reachability-asm.bas
    Purpose: Check transitive private procedure reachability and address escapes.
    Responsibilities: Check that opaque inline assembly preserves procedures.
    This file intentionally does NOT implement procedure reachability.
'/
private function dead_leaf() as long
    return 73
end function
private function dead_middle() as long
    return dead_leaf()
end function
private declare function dead_cycle_a() as long
private declare function dead_cycle_b() as long
private function dead_cycle_a() as long
    return dead_cycle_b()
end function
private function dead_cycle_b() as long
    return dead_cycle_a()
end function
private function live_leaf() as long
    return 19
end function
private function live_middle() as long
    return live_leaf()
end function
private function escaped_callback() as long
    return 23
end function
private function dead_calls_escaped() as long
    return escaped_callback()
end function
dim shared callback as function() as long = @escaped_callback
asm
    nop
end asm
if live_middle() <> 19 then end 1
if callback() <> 23 then end 2
end 0
' end of c-procedure-reachability-asm.bas
