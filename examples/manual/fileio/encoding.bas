'' examples/manual/fileio/encoding.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'ENCODING'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgEncoding
'' --------

'' This example will:
'' 1) Write a string to a text file with utf-16 encoding
'' 2) Display the byte contents of the file
'' 3) Read the text back from the file
''
'' WSTRING's will work as well but STRING has been
'' used in this example since not all consoles support
'' printing WSTRING's.

'' Resource ownership:
'' Each scope owns a new file handle and closes it after the corresponding
'' encoding operation. The byte-inspection phase rejects unexpectedly large
'' sample files before allocating its display buffer.
'' Four mebibytes is enough for this short text sample without accepting an
'' unbounded allocation from a pre-existing file with the same name.
Const MAX_SAMPLE_FILE_BYTES As LongInt = 4194304LL

'' The name of the file to use in this example
Dim f As String
f = "sample.txt"

''
Scope
  Dim s As String, file_number As Integer
  s = "FreeBASIC"

  Print "Text to write to " + f + ":"
  Print s
  Print

  '' open a file for output using utf-16 encoding
  '' and print a short message
  file_number = FreeFile

  '' The manual deliberately leaves this named sample file for inspection.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL103 FBL-IO-005
  If Open(f For Output Encoding "utf-16" As #file_number) <> 0 Then
    Print "Could not open " + f + " for output"
  Else

    '' The ascii string is converted to utf-16
    Print #file_number, s
    Close #file_number
  End If
End Scope

''
Scope
  Dim s As String, n As LongInt, file_number As Integer

  '' open the same file for binary and read all the bytes
  file_number = FreeFile
  If Open(f For Binary As #file_number) <> 0 Then
    Print "Could not open " + f + " for byte inspection"
  Else
    n = LOF(file_number)

    If n < 0 OrElse n > MAX_SAMPLE_FILE_BYTES Then
      Print "Sample file is outside the supported size range"
    Else
      s = Space(n)

      '' This reads the exact encoded bytes written by the preceding scope.
      '' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
      If Get(#file_number, , s) <> 0 Then
        Print "Could not read " + f
      Else
        Print "Binary contents of " + f + ":"
        For i As Integer = 1 To n
          Print Hex(Asc(Mid(s, i, 1)), 2); " ";
        Next
        Print
        Print
      End If
    End If

    Close #file_number
  End If

End Scope

''
Scope
  Dim s As String, file_number As Integer

  '' open a file for input using utf-16 encoding
  '' and read back the message
  file_number = FreeFile
  If Open(f For Input Encoding "utf-16" As #file_number) <> 0 Then
    Print "Could not open " + f + " for input"
  Else

    '' The ascii string is converted from utf-16
    Line Input #file_number, s
    Close #file_number

    '' Display the text
    Print "Text read from " + f + ":"
    Print s
    Print
  End If
End Scope
