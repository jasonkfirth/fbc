'' examples/manual/fileio/opencom.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPEN'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpen
'' --------

'OPEN A COM PORT
Dim file_number As Integer
file_number = FreeFile

Open Com "COM1:9600,N,8,1" As #file_number
If Err > 0 Then
  Print "The port could not be opened."
Else
  Close #file_number
End If

'COM1, 9600 BAUD, NO PARITY BIT, EIGHT DATA BITS, ONE STOP BIT
