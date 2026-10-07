'' Project: FreeBASIC semantic-sidecar fixtures
'' File: scalar-for-steps.bas
'' Purpose: Preserve original scalar STEP inputs before counter conversion.
'' Responsibilities: Reused counters, defaults, aliases, values and pointer exclusion.
'' This file intentionally does NOT execute loops or link unresolved procedures.
#lang "fb"

type CounterAlias as uinteger
const DescendingStep = -2
sub ScalarLoops(byval unknownStep as longint)
    dim counter as CounterAlias
    for counter = 4 to 0 step -1
    next
    for counter = 4 to 0 step DescendingStep
    next
    for counter = 0 to 4
    next
    for counter = 0 to 4 step 0
    next
    for counter = 0 to 4 step 1.5
    next
    for counter = 0 to 4 step unknownStep
    next
    for wideCounter as ulongint = 4 to 0 step -2147483649ll
    next
    dim address as integer ptr
    for address = 0 to 0 step 1
    next
end sub

enum CounterKind
    FirstCounter = 0
    LastCounter = 4
end enum
#define ExpandedStep step 1
sub EnumAndMacroLoops()
    for enumCounter as CounterKind = FirstCounter to LastCounter step 0
    next
    for enumCounter as CounterKind = FirstCounter to LastCounter
    next
    for macroCounter as integer = 0 to 4 ExpandedStep
    next
end sub

sub SelectedStepConversions()
    for roundedCounter as integer = 0 to 1 step 0.4
    next
    for narrowedCounter as byte = 0 to 1 step 128
    next
    for wrappedCounter as ubyte = 0 to 1 step 256u
    next
end sub

'' end of scalar-for-steps.bas
