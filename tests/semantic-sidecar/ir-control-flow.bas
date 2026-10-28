'' Project: FreeBASIC semantic sidecar tests
'' File: ir-control-flow.bas
'' Purpose: Exercise actual IR branches, nested conditions, checks and returns.
'' Responsibilities: Loop exits, guarded calls, switches and unreachable source.
'' This file intentionally does NOT infer runtime paths from compiler load order.
#lang "fb"

function IRFlowRead(byval selector as long, byval address as integer ptr) as integer
    dim values(0 to 3) as integer
    values(selector) = *address
    dim total as integer = 0
    for index as long = 0 to selector
        if index = 2 then continue for
        total += index
        if total > 10 then exit for
    next
    while total < 3
        total += 1
    wend
    do
        total -= 1
    loop until total = 1
    select case as const selector
    case 0, 2
        total += 4
    case 3
        total += 6
    case else
        total += 8
    end select
    if selector > 0 andalso *address > 0 then
        total += iif(selector = 1, values(selector), 0)
    end if
    return total
    total = -999
end function

private sub IRFlowUnused()
    print "unemitted"
end sub

dim value as integer = 7
print IRFlowRead(1, @value)

'' end of ir-control-flow.bas
