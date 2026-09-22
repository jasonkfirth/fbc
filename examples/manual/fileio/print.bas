'' examples/manual/fileio/print.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic '(PRINT | ?) #'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgPrintPp
'' --------

'' Resource policy:
'' This manual writes a named sample file and closes its handle after the two
'' PRINT # forms have been demonstrated.
Dim file_number As Integer
file_number = FreeFile

'' The named output is an intentional manual fixture for inspection.
'' FB-LINTER: DISABLE-NEXT-LINE FBL103 FBL-IO-005
If Open("bleh.dat" For Output As #file_number) <> 0 Then
	Print "Could not open bleh.dat"
Else
	Print #file_number, "abc def"
	Print #file_number, 1234, 5678.901, "xyz zzz"

	Close #file_number
End If
