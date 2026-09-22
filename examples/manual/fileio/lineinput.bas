'' examples/manual/fileio/lineinput.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'LINE INPUT #'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgLineinputPp
'' --------
''
'' File and memory ownership:
''
'' This manual example creates myfile.txt as the input fixture it reads later.
'' Each successful Open uses a FreeFile unit and is closed before the next
'' operation.  pz owns its maxlength-byte ZSTRING buffer while the bounded
'' input example is active.

Dim As Integer filenumber
Dim As Integer fileerror
Dim As Integer fileisready
Dim As String s
Dim As ZString Ptr pz

filenumber = FreeFile
' The named fixture is the subject of this manual example; each I/O result is checked below.
Open "myfile.txt" For Output As #filenumber  ' FB-LINTER: DISABLE-LINE FBL103 FBL-IO-005 -- documented fixture
fileerror = Err
If fileerror = 0 Then
	Print #filenumber, "Hello, World"
	fileerror = Err
	Close #filenumber
	If fileerror = 0 And Err = 0 Then  ' FB-LINTER: DISABLE-LINE FBL613 -- Err reports the preceding Close result
		fileisready = -1
	Else
		Print "Unable to write myfile.txt."
	End If
Else
	Print "Unable to create myfile.txt."
End If

If fileisready Then
	filenumber = FreeFile
	Open "myfile.txt" For Input As #filenumber
	fileerror = Err
	If fileerror = 0 Then
		Line Input #filenumber, s
		fileerror = Err
		Close #filenumber
		If fileerror = 0 And Err = 0 Then  ' FB-LINTER: DISABLE-LINE FBL613 -- Err reports the preceding Close result
			Print "'" & s & "'"
		Else
			Print "Unable to read myfile.txt."
		End If
	Else
		Print "Unable to open myfile.txt for input."
	End If
End If

Const maxlength = 6  '' max 5 characters plus 1 null terminal character
If fileisready Then
	pz = CAllocate(maxlength, SizeOf(ZString))
	If pz <> 0 Then
		filenumber = FreeFile
		Open "myfile.txt" For Input As #filenumber
		fileerror = Err
		If fileerror = 0 Then
			Line Input #filenumber, *pz, maxlength
			fileerror = Err
			Close #filenumber
			If fileerror = 0 And Err = 0 Then  ' FB-LINTER: DISABLE-LINE FBL613 -- Err reports the preceding Close result
				Print "'" & *pz & "'"
			Else
				Print "Unable to read myfile.txt into the ZSTRING buffer."
			End If
		Else
			Print "Unable to open myfile.txt for bounded input."
		End If
		Deallocate(pz)
		pz = 0
	Else
		Print "Unable to allocate the bounded input buffer."
	End If
End If

'' end of examples/manual/fileio/lineinput.bas
