'' examples/manual/fileio/openpipe.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPEN PIPE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpenPipe
'' --------

'' This example uses Open Pipe to run a shell command and retrieve its output.
'' Resource ownership:
'' The pipe handle is acquired only after OPEN succeeds and is closed after
'' the command output has been consumed.
#ifdef __FB_UNIX__
Const TEST_COMMAND = "ls *"
#else
Const TEST_COMMAND = "dir *.*"
#endif

Dim file_number As Integer
Dim ln As String

file_number = FreeFile
Open Pipe TEST_COMMAND For Input As #file_number
If Err <> 0 Then
  Print "Could not open command pipe"
Else
  Do Until EOF(file_number)
	Line Input #file_number, ln
	Print ln
  Loop

  Close #file_number
End If
