/'
    Project: FreeBASIC C backend regression tests
    File: c-procedure-deep-reachability.bas
    Purpose: Retain private-call analysis across a deep constructor initializer.
    Responsibilities: Check global initialization and discard an uncalled private chain.
    This file intentionally does NOT use assembly or suppress C diagnostics.
'/
#lang "fb"

'' Each dynamic string needs initialization; 512 entries exceed the former
'' assembly scan's 256-level recursion limit without a large source asset.
type Entry
    text as string
    declare constructor(byval value as integer)
end type

constructor Entry(byval value as integer)
    text = string(value, "v")
end constructor

const ENTRY_COUNT as integer = 512
dim shared words(0 to ENTRY_COUNT - 1) as Entry = { _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), _
    Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1), Entry(1) _
}

'' This array is read only by a discarded procedure. Debug builds retain
'' its storage for inspection; release builds have no observable use for it.
dim shared private_deep_data(0 to 2) as integer = {11, 22, 33}

private function unused_deep_leaf() as integer
    return private_deep_data(1)
end function

private function unused_deep_caller() as integer
    return unused_deep_leaf()
end function

dim as integer total
for i as integer = lbound(words) to ubound(words)
    total += len(words(i).text)
next
if( total <> ENTRY_COUNT ) then end __LINE__
print "DEEP_REACHABILITY=PASS"

/' end of c-procedure-deep-reachability.bas '/
