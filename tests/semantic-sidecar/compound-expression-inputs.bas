'' Project: FreeBASIC semantic-sidecar fixtures
'' File: compound-expression-inputs.bas
'' Purpose: Retain original compound assignment operands before AST lowering.
'' Responsibilities: Selected arithmetic width, floating conversions and indices.
'' This file intentionally does NOT execute arithmetic or side-effect probes.
#lang "fb"

const unitValue as long = 1
function readIndex( ) as integer
	return 0
end function

sub review(byval inputValue as long, byval realValue as double)
	dim as long resultValue
	dim as long values(0 to 1)
	resultValue \= unitValue
	resultValue mod= unitValue
	resultValue += inputValue
	resultValue *= realValue
	values(readIndex( )) mod= unitValue
end sub

'' end of compound-expression-inputs.bas
