'' examples/manual/system/fileseteof1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'FILESETEOF'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgFileseteof
'' --------

#include "file.bi"

'' Resource ownership:
'' Each resize pass obtains and closes its own checked binary handle. A later
'' pass runs only after the preceding resize completed successfully.
Dim file_number As Integer

'' create a zero length file
file_number = FreeFile
If Open("file.dat" For Binary As #file_number) <> 0 Then
  Print "Could not create file.dat"
Else
  FileSetEof file_number
  Close #file_number

  '' open same file and extend to 10000 bytes size
  file_number = FreeFile
  If Open("file.dat" For Binary As #file_number) <> 0 Then
    Print "Could not extend file.dat"
  Else
    Seek #file_number, (10000 + 1)
    FileSetEof file_number
    Close #file_number

    '' open same file and truncate to 5000 bytes size
    file_number = FreeFile
    If Open("file.dat" For Binary As #file_number) <> 0 Then
      Print "Could not truncate file.dat"
    Else
      Seek #file_number, (5000 + 1)
      FileSetEof file_number
      Close #file_number

      '' clean-up
      Kill "file.dat"
    End If
  End If
End If
