/'
    Project: FreeBASIC C backend regression
    File: c-procedure-reachability.bas
    Purpose: Check transitive private procedure reachability and address escapes.
    Responsibilities: Check live calls, dead call chains and saved callbacks.
    This file intentionally does NOT contain opaque inline assembly.
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
type ReachabilityWorker
    value as long
private:
    declare function DeadMethodLeaf() as long
    declare function DeadMethodWrapper() as long
    declare function LiveMethodLeaf() as long
public:
    declare function Run() as long
end type

private function ReachabilityWorker.DeadMethodLeaf() as long
    return 41
end function
private function ReachabilityWorker.DeadMethodWrapper() as long
    return DeadMethodLeaf()
end function
private function ReachabilityWorker.LiveMethodLeaf() as long
    return 29 + this.value
end function
private function ReachabilityWorker.Run() as long
    return LiveMethodLeaf()
end function

private function pick overload (byval value as long) as long
    return value + 1
end function
private function pick overload (byref value as const string) as long
    return len(value)
end function
private function dead_overload_caller() as long
    return pick("discarded")
end function

dim shared callback as function() as long = @escaped_callback
if live_middle() <> 19 then end 1
if callback() <> 23 then end 2
dim worker as ReachabilityWorker
if worker.Run() <> 29 then end 3
if pick(clng(7)) <> 8 then end 4
end 0
' end of c-procedure-reachability.bas
