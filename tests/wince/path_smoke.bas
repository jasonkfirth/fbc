''
'' FreeBASIC Windows CE path-resolution smoke test
'' ------------------------------------------------
''
'' File: tests/wince/path_smoke.bas
''
'' Purpose:
''
''     Verify absolute and relative file resolution below CERF's shared
''     Windows CE storage directory.
''
'' Responsibilities:
''
''     - record the runtime executable name and logical current directory
''     - probe one tracked fbctests resource by absolute and relative paths
''     - verify that CHDIR changes relative path resolution in-process
''     - publish numeric API results without writing to the guest display
''
'' This file intentionally does NOT contain:
''
''     - file-content assertions
''     - directory mutation
''     - emulator automation
''     - production diagnostic output
''

Const SharedRoot = "\Storage Card"
Const ReportPath = SharedRoot & "\wince-path-smoke.txt"
Const AbsoluteInput = SharedRoot & "\boolean\data-ascii.txt"
Const RelativeInput = "boolean\data-ascii.txt"

Dim As Integer absolute_file = FreeFile()
Dim As Integer absolute_status
Dim As Integer change_status
Dim As Integer relative_file
Dim As Integer relative_status
Dim As Integer report_file
Dim As String directory_before = CurDir()
Dim As String directory_after
Dim As String executable_name = Command( 0 )

absolute_status = Open( AbsoluteInput For Input As #absolute_file )
If absolute_status = 0 Then Close #absolute_file

change_status = ChDir( SharedRoot )
directory_after = CurDir()
relative_file = FreeFile()
relative_status = Open( RelativeInput For Input As #relative_file )
If relative_status = 0 Then Close #relative_file

report_file = FreeFile()
If Open( ReportPath For Output As #report_file ) <> 0 Then End 1
Print #report_file, "exename=" & executable_name
Print #report_file, "curdir-before=" & directory_before
Print #report_file, "absolute-open=" & Str( absolute_status )
Print #report_file, "chdir=" & Str( change_status )
Print #report_file, "curdir-after=" & directory_after
Print #report_file, "relative-open=" & Str( relative_status )
Close #report_file

End 0

'' end of tests/wince/path_smoke.bas
