'' Project: FreeBASIC DOS qualification
'' File: emulator-x87.bas
'' Purpose: Check x87 comparisons before running the DOS language suites.
'' Responsibilities: Reject incorrect loop termination without unbounded output.
'' This file intentionally does NOT replace the language regression suite.

const EXPECTED_ITERATIONS = 7
dim as integer iterations

'' Some older DOSBox-X dynamic cores terminate this loop after one iteration.
'' Bound the body as well, so a comparison defect cannot fill the host disk.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-LOOP-010 REASON: This probe deliberately checks a bounded floating-point loop.
for value as single = 3 to 0 step -0.5
	iterations += 1
	if iterations > EXPECTED_ITERATIONS then end 2
next

if iterations <> EXPECTED_ITERATIONS then end 1
print "DOS_X87=PASS"
end 0

'' end of emulator-x87.bas
