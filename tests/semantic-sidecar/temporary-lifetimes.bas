'' Project: FreeBASIC semantic sidecar tests
'' File: temporary-lifetimes.bas
'' Purpose: Exercise selected and discarded conditional temporary lifetimes.
'' Responsibilities: UDT results, nested IIf, strings and short-circuit paths.
'' This file intentionally does NOT execute both conditional branches.

dim shared as integer true_temporary_calls, false_temporary_calls, destroyed_temporaries

type TemporaryValue
	value as integer
	declare constructor( )
	declare constructor( byref source as TemporaryValue )
	declare constructor( byval number as integer )
	declare destructor( )
end type

constructor TemporaryValue( )
	value = 0
end constructor

constructor TemporaryValue( byref source as TemporaryValue )
	value = source.value
end constructor

constructor TemporaryValue( byval number as integer )
	value = number
end constructor

destructor TemporaryValue( )
	destroyed_temporaries += 1
end destructor

function TrueTemporary( ) as TemporaryValue
	true_temporary_calls += 1
	return 10
end function

function FalseTemporary( ) as TemporaryValue
	false_temporary_calls += 1
	return 20
end function

function TemporaryText( ) as string
	return "temporary"
end function

sub ProbeTemporaryBranches( byval flag as integer )
	dim chosen_temporary as TemporaryValue = iif(flag, TrueTemporary(), FalseTemporary())
	print chosen_temporary.value
	dim folded_temporary as TemporaryValue = iif(true, TrueTemporary(), FalseTemporary())
	dim nested_temporary as string = iif(flag, iif(flag, TemporaryText(), "other"), "no")
	if flag andalso len(TemporaryText()) then nested_temporary += " selected"
	if flag orelse len(TemporaryText()) then nested_temporary += " visited"
end sub

ProbeTemporaryBranches(1)
ProbeTemporaryBranches(0)
print true_temporary_calls
print false_temporary_calls

'' end of temporary-lifetimes.bas
