'' examples/manual/fileio/lock.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'LOCK'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgLock
'' --------

'' e.g. locking a file, reading 100 bytes, and unlocking it.
'' To run, make sure there exists a file called 'file.ext'
'' in the current directory that is at least 100 bytes.

Dim array(1 To 100) As Integer
Dim f As Integer, i As Integer
f = FreeFile
If Open("file.ext" For Binary As #f) <> 0 Then
  Print "Could not open file.ext"
Else
  Lock #f, 1 To 100
  For i = 1 To 100
    '' file.ext is a same-target Integer sequence for this locking example.
    '' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
	If Get(#f, i, array(i)) <> 0 Then
      Print "Could not read file.ext"
      Exit For
    End If
  Next
  Unlock #f, 1 To 100
  Close #f
End If
