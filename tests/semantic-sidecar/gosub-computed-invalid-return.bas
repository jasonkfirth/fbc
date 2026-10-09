'' Project: FreeBASIC compiler regression tests
'' File: gosub-computed-invalid-return.bas
'' Purpose: Prove that a skipped computed GOSUB does not create a return frame.
'' Responsibilities: Normal continuation once and a subsequent unmatched RETURN.
'' This file intentionally does NOT catch the expected runtime error.
#lang "fblite"
option gosub

dim as integer selector = -1
on selector gosub selectedTarget
print "continued-once"
return
selectedTarget:
print "unexpected-target"
return
'' end of gosub-computed-invalid-return.bas
