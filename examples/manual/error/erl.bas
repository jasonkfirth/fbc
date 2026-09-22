'' examples/manual/error/erl.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'ERL'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgErl
'' --------

' compile with -lang fblite or qb

#lang "fblite"

' note: compilation with '-ex' option is required

'' This legacy handler is the subject of the ERL example.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-004 FBL-ERR-005
On Error Goto ErrorHandler

' Generate an explicit error
Error 100

End

'' END above prevents normal execution from entering this teaching handler.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-006 FBL-CF-004
ErrorHandler:
  Dim num As Long = Err
  Print "Error "; num; " on line "; Erl
  '' RESUME NEXT deliberately completes the one-error sample.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-007
  Resume Next

' Expected output is
' Error  100 on line  6
