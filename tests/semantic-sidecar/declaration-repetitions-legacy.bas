'' Project: FreeBASIC semantic declaration tests
'' File: declaration-repetitions-legacy.bas
'' Purpose: Retain accepted repeated legacy prototypes and their original formals.
'' Responsibilities: Exact default contracts, canonical identity and real execution.
'' This file intentionally does NOT exercise rejected signature mismatches.
#lang "fblite"
declare function EchoValue(byval InputValue as long = 1) as long
declare function EchoValue(byval OtherValue as long = 1) as long
function EchoValue(byval ActualValue as long) as long
    return ActualValue
end function
print EchoValue(5)
'' end of declaration-repetitions-legacy.bas
