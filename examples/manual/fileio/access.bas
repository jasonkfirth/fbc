'' examples/manual/fileio/access.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'ACCESS'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgAccess
'' --------

'' Resource policy:
'' The source and destination are opened one at a time. The byte array owns a
'' same-target binary copy only while the source record is being transferred.
Dim As Integer o
Dim As LongInt file_length
Dim As Integer read_succeeded

  '' get an open file number.
  o = FreeFile

  '' open file for read-only access.
  If Open("data.raw" For Binary Access Read As #o) <> 0 Then
    Print "Could not open data.raw"
  Else
    file_length = LOF(o)

    If file_length <= 0 Then
      Print "data.raw does not contain a copyable byte range"
    Else
      '' make a buffer in memory thats the entire size of the file
      Dim As UByte file_char(0 To file_length - 1)

      '' data.raw and data.out are copied byte-for-byte by this manual example.
      '' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
      If Get(#o, , file_char()) <> 0 Then
        Print "Could not read data.raw"
      Else
        read_succeeded = -1

        Close #o

        '' get another open file number.
        o = FreeFile

        '' open file for write-only access.
        If Open("data.out" For Binary Access Write As #o) <> 0 Then
          Print "Could not open data.out"
        Else
          '' data.raw and data.out are copied byte-for-byte by this manual example.
          '' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
          If Put(#o, , file_char()) <> 0 Then Print "Could not write data.out"
          Close #o
        End If
      End If
    End If

    If read_succeeded = 0 Then Close #o
  End If

  If read_succeeded <> 0 Then Print "Copied file ""data.raw"" to file ""data.out"""

  Sleep
