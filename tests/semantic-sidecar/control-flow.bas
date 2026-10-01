'' Project: FreeBASIC semantic sidecar tests
'' File: control-flow.bas
'' Purpose: Exercise typed control flow, conversions, and memory access.
'' Responsibilities: Produce branch, jump-table, call, and argument node payloads.
'' This file intentionally does NOT contain: a source-level flow reconstruction.

function Compute(byval selector as long, byref result as double) as long
	dim values(0 to 3) as long
	dim pointer_value as long ptr = @values(0)
	*pointer_value = selector
	result = cdbl(selector) / 2
	select case as const selector
	case 1, 3
		result += 1
	case 5
		result += 2
	case else
		result -= 1
	end select
	while selector > 0
		selector -= 1
	wend
	do
		selector += 1
	loop until selector = 2
	if result > 0 then
		return values(selector)
	end if
	return -1
end function

dim result as double
print Compute(3, result), result

'' end of control-flow.bas
