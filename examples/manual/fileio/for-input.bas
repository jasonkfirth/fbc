'' examples/manual/fileio/for-input.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'INPUT (FILE MODE)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgInputfilemode
'' --------

'' Resource policy:
'' Each successful Open owns ff until its matching Close. The input phase runs
'' only after the sample data was written successfully.

Dim ff As Integer
Dim randomvar As Integer
Dim name_str As String
Dim age As Integer

'' collect the test data and output to file with Write #
Input "What is your name? ", name_str
Input "What is your age? ", age
Randomize
Print

ff = FreeFile

If Open("testfile" For Output As #ff) <> 0 Then
	Print "Could not write testfile"
Else
	'' This example intentionally demonstrates BASIC's formatted record syntax.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL517
	Write #ff, Int(Rnd*42), name_str, age
	Close #ff

	'' clear variables
	randomvar = 0
	name_str = ""
	age = 0

	'' input the variables, using Input #
	ff = FreeFile
	If Open("testfile" For Input As #ff) <> 0 Then
		Print "Could not read testfile"
	Else
		'' This example intentionally demonstrates BASIC's formatted record syntax.
		'' FB-LINTER: DISABLE-NEXT-LINE FBL517
		Input #ff, randomvar, name_str, age
		Close #ff
	End If
End If

Print "Random Number was: " & randomvar
Print "Your name is: " & name_str
Print "Your age is: " & age

'File outputted by this sample will look something like this
'(not including the leading comment marker):
'23,"Your Name",19
