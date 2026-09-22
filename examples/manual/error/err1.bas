'' examples/manual/error/err1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'ERR'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgErr
'' --------

'' Compile with -lang fblite or qb

#lang "fblite"

'' This legacy handler is the subject of the ERR example.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-004 FBL-ERR-005
On Error Goto Error_Handler
Error 150
End

'' END above prevents normal execution from entering this teaching handler.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-006 FBL-CF-004
Error_Handler:
	'' ERR is read only on the handler path established above.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL613
  n = Err()
  Print "Error #"; n
	'' RESUME NEXT deliberately completes the one-error sample.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-007
  Resume Next
