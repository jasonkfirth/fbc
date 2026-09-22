'' examples/manual/system/fileattr.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'FILEATTR'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgFileattr
'' --------

#include "vbcompat.bi"
#include "crt.bi"

'' Resource ownership:
'' Each pass owns one checked handle. The input pass begins only after the
'' output fixture was written and closed.
Dim f As FILE Ptr, i As Integer, file_number As Integer

'' Open a file and write some text to it

file_number = FreeFile
'' test.txt is a named manual fixture retained for the following read pass.
'' FB-LINTER: DISABLE-NEXT-LINE FBL103 FBL-IO-005
If Open("test.txt" For Output As #file_number) <> 0 Then
  Print "Could not open test.txt for output"
Else
  f = Cast(FILE Ptr, FileAttr(file_number, fbFileAttrHandle))
  If f = 0 Then
    Print "Could not obtain the output file handle"
  Else
    For i = 1 To 10
      fprintf(f, !"Line %i\n", i)
    Next i
  End If
  Close #file_number

  '' re-open the file and read the text back

  file_number = FreeFile
  If Open("test.txt" For Input As #file_number) <> 0 Then
    Print "Could not open test.txt for input"
  Else
    f = Cast(FILE Ptr, FileAttr(file_number, fbFileAttrHandle))
    If f = 0 Then
      Print "Could not obtain the input file handle"
    Else
      While feof(f) = 0
        i = fgetc(f)
        Print Chr(i);
      Wend
    End If
    Close #file_number
  End If
End If
