'' examples/manual/error/erfn.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'ERFN'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgErfn
'' --------

'' test.bas
'' compile with fbc -exx -lang fblite test.bas

#lang "fblite"

Sub Generate_Error
  '' This compact FBlite example intentionally demonstrates one local handler,
  '' including its Resume Next continuation after the synthetic Error statement.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-004 FBL-ERR-005 FBL-ERR-007
  On Error Goto Handler
  Error 1000
  Exit Sub
Handler:
  Print "Error Function: "; *Erfn()
  Print "Error Module  : "; *Ermn()
  '' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-007
  Resume Next
End Sub

Generate_Error

'' end of erfn.bas
