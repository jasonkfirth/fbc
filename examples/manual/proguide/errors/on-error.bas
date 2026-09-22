'' examples/manual/proguide/errors/on-error.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Error Handling'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgErrorHandling
'' --------

'' Compile with QB (-lang qb) dialect

'$lang: "qb"

Dim file_number As Integer
'' This QB example intentionally keeps the handler active only for OPEN.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-004 FBL-ERR-005
On Error Goto FAILED
file_number = FreeFile
Open "xzxwz.zwz" For Input As #file_number
On Error Goto 0
Close #file_number
Sleep
End

FAILED:
Dim e As Integer
e = Err
Print e
Sleep
End
