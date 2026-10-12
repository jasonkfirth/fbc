'' Project: FreeBASIC compiler optimizer qualification
'' File: power-variable-access.bas
'' Purpose: Verify the POW-to-MUL rewrite preserves types and evaluation.
'' Responsibilities: Check variable, field, index and dereference accesses.
'' This file intentionally does NOT benchmark the host math library.
'' Compile with -eassert so the runtime checks and counted index call execute.
#lang "fb"

type SampleValue
	value as integer
end type

'' Module state: the single test invocation owns this evaluation counter.
dim shared as integer index_calls
function NextIndex( ) as integer
	index_calls += 1
	return 0
end function

dim as integer scalar = 3
dim as SampleValue record
record.value = 4
dim as integer values(0 to 1) = { 5, 6 }
dim as integer ptr address = @scalar
dim as single fractional = 2.5

assert( scalar ^ 2 = 9.0 )
assert( record.value ^ 2 = 16.0 )
assert( values(0) ^ 2 = 25.0 )
assert( (*address) ^ 2 = 9.0 )
assert( fractional ^ 2 = 6.25 )
#assert len( scalar ^ 2 ) = len( double )
#assert len( fractional ^ 2 ) = len( double )
dim as double indexed_square = values(NextIndex( )) ^ 2
assert( indexed_square = 25.0 )
assert( index_calls = 1 )

print "POWER_VARIABLE_ACCESS_PASS"

'' end of power-variable-access.bas
