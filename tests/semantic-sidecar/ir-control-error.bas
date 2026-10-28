'' Project: FreeBASIC semantic sidecar tests
'' File: ir-control-error.bas
'' Purpose: Execute one caught error and its native RESUME NEXT path.
'' Responsibilities: Native handler registration, error dispatch and recovery labels.
'' This file intentionally does NOT leave an uncaught runtime error.
#lang "fblite"

on error goto Trap
error 7
print "recovered"
end
Trap:
resume next

'' end of ir-control-error.bas
