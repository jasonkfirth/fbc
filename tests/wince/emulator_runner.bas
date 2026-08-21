''
'' FreeBASIC Windows CE emulator smoke runner
'' -------------------------------------------
''
'' File: tests/wince/emulator_runner.bas
''
'' Purpose:
''
''     Run the Windows CE target smoke executables from CERF's shared storage
''     and publish machine-readable results back to the host.
''
'' Responsibilities:
''
''     - launch each runtime, thread, network, graphics, and sound smoke test
''     - wait for every child process and record its exit status
''     - verify the file and console tests wrote their expected artifacts
''     - create a completion marker only after the report has been closed
''
'' This file intentionally does NOT contain:
''
''     - host emulator startup or user-interface automation
''     - fbctests selection policy
''     - interactive graphics or audio assertions
''

Const SharedRoot = "\Storage Card"
Const ResultsName = "wince-emulator-results.txt"
Const CompleteName = "wince-emulator-complete.txt"
Const BasicOutputName = "fb-wince-smoke.txt"
Const ConsoleOutputName = "fb-wince-console-smoke.txt"

Dim test_names( 0 To 5 ) As String = { _
	"basic-file.exe", _
	"console-smoke.exe", _
	"tcp-link.exe", _
	"threadcall-link.exe", _
	"gfx-link.exe", _
	"sfx-link.exe" _
}

Dim As Integer failures
Dim As Integer status
Dim As Integer report_file = FreeFile()
Dim As String results_path = SharedRoot & "\" & ResultsName
Dim As String complete_path = SharedRoot & "\" & CompleteName
Dim As String basic_output_path = SharedRoot & "\" & BasicOutputName
Dim As String console_output_path = SharedRoot & "\" & ConsoleOutputName

If Dir( results_path ) <> "" Then Kill results_path
If Dir( complete_path ) <> "" Then Kill complete_path
If Dir( basic_output_path ) <> "" Then Kill basic_output_path
If Dir( console_output_path ) <> "" Then
	Kill console_output_path
End If

If Open( results_path For Output As #report_file ) <> 0 Then End 1

For index As Integer = LBound( test_names ) To UBound( test_names )
	status = Shell( Chr( 34 ) & SharedRoot & "\" & test_names( index ) & Chr( 34 ) )
	Print #report_file, test_names( index );"=";status

	If status <> 0 Then
		failures += 1
	End If
Next

If Dir( basic_output_path ) = "" Then
	Print #report_file, "basic-file-output=missing"
	failures += 1
Else
	Print #report_file, "basic-file-output=present"
End If

If Dir( console_output_path ) = "" Then
	Print #report_file, "console-output=missing"
	failures += 1
Else
	Print #report_file, "console-output=present"
End If

Print #report_file, "failures=";failures
Close #report_file

report_file = FreeFile()
If Open( complete_path For Output As #report_file ) <> 0 Then End 1
Print #report_file, failures
Close #report_file

End failures

'' end of tests/wince/emulator_runner.bas
