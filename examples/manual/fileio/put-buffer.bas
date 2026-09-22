'' examples/manual/fileio/put-buffer.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'PUT (File I/O)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgPutfileio
'' --------

'' Resource policy:
'' lpBuffer owns its byte storage after Allocate succeeds. hFile is used only
'' after a successful Open and is closed before the buffer is released.

Dim As Byte Ptr lpBuffer
Dim As Integer hFile
Dim As Integer Counter
Dim As UInteger Size

Size = 256

lpBuffer = Allocate(Size)

If lpBuffer = 0 Then
  Print "Could not allocate the output buffer"
Else
  For Counter = 0 To Size-1
    lpBuffer[Counter] = (Counter And &HFF)
  Next

  ' Get free file file number
  hFile = FreeFile()

  ' Open the file "test.bin" in binary writing mode
  If Open("test.bin" For Binary Access Write As #hFile) <> 0 Then
    Print "Could not open test.bin"
  Else
    ' test.bin is exactly Size raw byte values from lpBuffer[0] through lpBuffer[Size - 1].
    ' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
    If Put(#hFile, , lpBuffer[0], Size) <> 0 Then Print "Could not write test.bin"

    ' Close the file
    Close #hFile
  End If

  ' Free the allocated memory
  Deallocate lpBuffer
  lpBuffer = 0
End If
