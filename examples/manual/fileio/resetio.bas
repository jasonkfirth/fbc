'' examples/manual/fileio/resetio.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'RESET'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgReset
'' --------

'' Resource ownership:
'' Each console-input pass owns its own checked handle and closes it before
'' RESET changes the runtime's standard-input routing.
Dim x As String
Dim file_number As Integer

'' Read from STDIN from piped input
file_number = FreeFile
Open Cons For Input As #file_number
If Err <> 0 Then
  Print "Could not open piped console input"
Else
  While EOF(file_number) = 0
    '' This manual page intentionally demonstrates INPUT # record parsing.
    '' FB-LINTER: DISABLE-NEXT-LINE FBL517
    Input #file_number, x
    Print """"; x; """"
  Wend
  Close #file_number
End If

'' Reset to read from the keyboard
Reset(0)

Print "Enter some text:"
Input x

'' Read from STDIN (now from keyboard)
file_number = FreeFile
Open Cons For Input As #file_number
If Err <> 0 Then
  Print "Could not open keyboard console input"
Else
  While EOF(file_number) = 0
    '' This manual page intentionally demonstrates INPUT # record parsing.
    '' FB-LINTER: DISABLE-NEXT-LINE FBL517
    Input #file_number, x
    Print """"; x; """"
  Wend
  Close #file_number
End If
