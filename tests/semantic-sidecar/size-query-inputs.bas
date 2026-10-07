'' Project: FreeBASIC semantic-sidecar fixtures
'' File: size-query-inputs.bas
'' Purpose: Observe selected LEN/SIZEOF input types before folding.
'' Responsibilities: Scalars, pointers, arrays, members and unevaluated calls.
'' This file intentionally does NOT execute pointer dereferences or callbacks.
#lang "fb"
type SizeRecord
    value as longint
    address as long ptr
end type
declare function make_pointer( ) as long ptr
dim address as long ptr
dim record as SizeRecord
dim addresses(0 to 3) as long ptr
dim text as string = "abc"
print sizeof(address), sizeof(*address), sizeof(long ptr), sizeof(SizeRecord)
print sizeof(record.address), sizeof(addresses), sizeof(addresses(1))
print len(address), len(text), sizeof(make_pointer( ))
print 2 * sizeof(address)
'' end of size-query-inputs.bas
