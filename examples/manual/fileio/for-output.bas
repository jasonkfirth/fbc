'' examples/manual/fileio/for-output.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OUTPUT'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOutput
'' --------

'' Resource policy:
'' Each successful Open owns ff until its matching Close. The input pass runs
'' only after the output pass successfully created the sample file.

Dim ff As Integer
Dim randomvar As Integer
Dim name_str As String
Dim age_ubyte As UByte

ff = FreeFile
Input "What is your name? ", name_str
Input "What is your age? ", age_ubyte
Randomize

If Open("testfile" For Output As #ff) <> 0 Then
	Print "Could not write testfile"
Else
	'' This example intentionally demonstrates BASIC's formatted record syntax.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL517
	Write #ff, Int(Rnd(0)*42), name_str, age_ubyte
	Close #ff
	randomvar=0
	name_str=""
	age_ubyte=0

	ff = FreeFile
	If Open("testfile" For Input As #ff) <> 0 Then
		Print "Could not read testfile"
	Else
		'' This example intentionally demonstrates BASIC's formatted record syntax.
		'' FB-LINTER: DISABLE-NEXT-LINE FBL517
		Input #ff, randomvar, name_str, age_ubyte
		Close #ff
	End If
End If

Print "Random Number was: ", randomvar
Print "Your name is: " + name_str
Print "Your age is: " + Str(age_ubyte)

'File outputted by this sample will look like this,
'minus the comment of course:
'23,"Your Name",19
