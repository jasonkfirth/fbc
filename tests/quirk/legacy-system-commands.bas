'' Project: FreeBASIC test suite
'' File: legacy-system-commands.bas
'' Purpose: Verify legacy hardware and printer command syntax.
'' Responsibilities: Compile INP, OUT, WAIT, LPOS, BEEP and STOP forms.
'' This file intentionally does NOT contain: privileged port or printer I/O.
' TEST_MODE : COMPILE_ONLY_OK

Dim As Long portValue

portValue = Inp(&h3da)
Out &h3c8, 1
Wait &h3da, &h8
Beep
Stop

'' end of legacy-system-commands.bas
