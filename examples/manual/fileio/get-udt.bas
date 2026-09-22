'' examples/manual/fileio/get-udt.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'GET (File I/O)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgGetfileio
'' --------

'' Resource policy:
'' Save and Load each own one FreeFile handle after a successful Open and close
'' it before returning. The caller owns the UDT value passed to each method.

' 'THIS' can be used as argument for writing/filling all non-static data of an UDT instance to/from a file

'' Layout: bytes 0-31 are the fixed string; bytes 32-39 are the Double value.
'' File format policy: this 40-byte tutorial record is read by the same FreeBASIC target that wrote it.
Type UDT
	' FB-LINTER: DISABLE-NEXT-LINE FBL-STR-009
	Dim As String * 32 s
	Dim As Double d
	Declare Sub Save(ByRef filename As String)
	Declare Sub Load(ByRef filename As String)
End Type

Sub UDT.Save(ByRef filename As String)
	Dim As Integer f
	f = FreeFile()

	If Open(filename For Binary As #f) <> 0 Then
		Print "Could not open "; filename
	Else
		' The UDT layout above is the binary record contract for this tutorial file.
		' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
		If Put(#f, , This) <> 0 Then Print "Could not write "; filename
		Close #f
	End If
End Sub

Sub UDT.Load(ByRef filename As String)
	Dim As Integer f
	f = FreeFile()

	If Open(filename For Binary As #f) <> 0 Then
		Print "Could not open "; filename
	Else
		' The UDT layout above is the binary record contract for this tutorial file.
		' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
		If Get(#f, , This) <> 0 Then Print "Could not read "; filename
		Close #f
	End If
End Sub

Dim As UDT u1
u1.s = "PI number"
u1.d = 3.14159
u1.Save("file.ext")

Dim As UDT u2
u2.Load("file.ext")
Print u2.s
Print u2.d
