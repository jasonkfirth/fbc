' TEST_MODE : COMPILE_AND_RUN_OK

' FreeBASIC QB tests: on-error-goto.bas
' Purpose: Check legacy error-handler control flow.
' Responsibilities: Exercise ON ERROR GOTO and inspect the error code.
' This file intentionally does not cover RESUME or other handler forms.

#lang "qb"

on error goto handler
error 24
end 1

handler:
if err <> 24 then end 2
end 0

' end of on-error-goto.bas
