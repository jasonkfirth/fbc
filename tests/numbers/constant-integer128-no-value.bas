'' Project: FreeBASIC compiler tests
'' File: numbers/constant-integer128-no-value.bas
'' Purpose: Reject a conversion node that has no compile-time constant carrier.
'' Responsibilities: Diagnose wide CONST initialization before reading its node union.
'' This file intentionally does NOT reject ordinary wide integer variables.
' TEST_MODE : COMPILE_ONLY_FAIL

const wide_constant as integer<128> = 1

'' end of numbers/constant-integer128-no-value.bas
