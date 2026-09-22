'' examples/manual/error/local.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'LOCAL'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgLocal
'' --------

'' compile with -lang fblite or qb

#lang "fblite"
#cmdline "-e"

Declare Sub foo

foo
Print "ok"
Sleep

Sub foo
  Dim errno As Integer
  Dim file_number As Integer
  file_number = FreeFile
  '' The procedure-local handler spans this compact compatibility demonstration
  '' and the source's -e directive supplies its required legacy error mode.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-004 FBL-ERR-005
  On Local Error Goto fail
  Open "xzxwz.zwz" For Input As #file_number
  Close #file_number
  On Local Error Goto 0
  Exit Sub
fail:                  ' here starts the error handler
  errno = Err
  Print "Error "; errno      ' just print the error number
  Sleep
End Sub
