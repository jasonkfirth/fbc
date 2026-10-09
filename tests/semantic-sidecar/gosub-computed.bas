'' Project: FreeBASIC compiler regression tests
'' File: gosub-computed.bas
'' Purpose: Verify computed and nested GOSUB stack ownership on native backends.
'' Responsibilities: Selected targets, out-of-range continuation and RETURN label.
'' This file intentionally does NOT depend on source-text lint heuristics.
#lang "fblite"

option nogosub
function ordinaryReturn() as integer
	return 23
end function
option gosub

sub computedTarget( byval selector as integer, byref total as integer )
	on selector gosub firstTarget, secondTarget
	goto finished
firstTarget:
	total += 1
	return
secondTarget:
	total += 2
	return
finished:
end sub

sub nestedTarget( byref total as integer )
	gosub outerTarget
	goto finished
outerTarget:
	total += 3
	gosub innerTarget
afterInner:
	total += 5
	return
innerTarget:
	total += 7
	return afterInner
finished:
end sub

dim as integer total = 0
for passValue as integer = 1 to 100
	for selector as integer = -1 to 3
		computedTarget( selector, total )
	next
	nestedTarget( total )
next
'' Each pass adds three from the selected table entries and fifteen from
'' the nested direct calls. Invalid selectors must not leave a return frame.
if total <> 100 * (3 + 15) then end 1
if ordinaryReturn() <> 23 then end 2
print "computed-gosub-ok"
'' end of gosub-computed.bas
