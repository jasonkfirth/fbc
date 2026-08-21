''
'' FreeBASIC Windows CE fbctests guest runner
'' ------------------------------------------
''
'' File: tests/wince/fbctests_runner.bas
''
'' Purpose:
''
''     Run one cross-built fbcunit batch inside Windows CE and publish its
''     exit status and XML report through CERF's shared storage.
''
'' Responsibilities:
''
''     - read the host-selected batch identifier from a fixed control file
''     - validate the identifier before using it in shared-storage paths
''     - launch fbc-tests.exe with deterministic reporting arguments
''     - wait for completion and write the child exit status
''
'' This file intentionally does NOT contain:
''
''     - test-directory selection or compilation
''     - CERF startup or user-interface automation
''     - result-summary interpretation
''     - retry or timeout policy
''

Const SharedRoot = "\Storage Card"
Const ControlPath = SharedRoot & "\fbctests-current.txt"
Const TestProgram = SharedRoot & "\fbc-tests.exe"

Function is_safe_batch_id( ByRef batch_id As Const String ) As Boolean
	If Len( batch_id ) = 0 Then Return False

	For index As Integer = 0 To Len( batch_id ) - 1
		Dim As Integer character = batch_id[index]
		Dim As Boolean is_letter = _
			( character >= Asc( "A" ) And character <= Asc( "Z" ) ) Or _
			( character >= Asc( "a" ) And character <= Asc( "z" ) )
		Dim As Boolean is_digit = _
			( character >= Asc( "0" ) And character <= Asc( "9" ) )

		If Not is_letter And Not is_digit And character <> Asc( "-" ) _
			And character <> Asc( "_" ) And character <> Asc( "." ) Then
			Return False
		End If
	Next

	Return True
End Function

Dim As Integer control_file = FreeFile()
Dim As String batch_id

If Open( ControlPath For Input As #control_file ) <> 0 Then End 2
Line Input #control_file, batch_id
Close #control_file

If Not is_safe_batch_id( batch_id ) Then End 3

Dim As String xml_path = SharedRoot & "\fbctests-" & batch_id & ".xml"
Dim As String result_path = SharedRoot & "\fbctests-" & batch_id & ".result"
Dim As String command_line = _
	Chr( 34 ) & TestProgram & Chr( 34 ) & _
	" --xml " & Chr( 34 ) & xml_path & Chr( 34 ) & _
	" --brief-summary --hide-cases"

If Dir( xml_path ) <> "" Then Kill xml_path
If Dir( result_path ) <> "" Then Kill result_path

Dim As Integer status = Shell( command_line )
Dim As Integer result_file = FreeFile()

If Open( result_path For Output As #result_file ) <> 0 Then End 4
Print #result_file, status
Close #result_file

End status

'' end of tests/wince/fbctests_runner.bas
