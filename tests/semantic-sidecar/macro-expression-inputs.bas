'' Project: FreeBASIC semantic-sidecar fixtures
'' File: macro-expression-inputs.bas
'' Purpose: Preserve typed operators inside actual macro expansions.
'' Responsibilities: Nested substitutions and distinct operations at one origin.
'' This file intentionally does NOT execute numeric policy probes.
#lang "fb"

const oneValue as long = 1
#define AddOne(value) ((value) + oneValue)
#define NestedAdd(value) AddOne(value)
#define UnitRemainder(value) ((value) mod oneValue)
#define Negate(value) (-(value))

sub review(byval inputValue as long)
	dim as long resultValue
	resultValue = AddOne(inputValue)
	resultValue = NestedAdd(inputValue)
	resultValue = UnitRemainder(inputValue)
	resultValue = Negate(inputValue)
	print resultValue
end sub

'' end of macro-expression-inputs.bas
