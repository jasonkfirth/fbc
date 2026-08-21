''
'' FreeBASIC Windows CE file-output smoke test
'' -------------------------------------------
''
'' File: tests/wince/basic_file.bas
''
'' Purpose:
''
''     Prove that a target executable can initialize the FreeBASIC runtime,
''     write a file, and exit without relying on a desktop console.
''
'' Responsibilities:
''
''     - create a file through the Windows CE runtime
''     - write and close a deterministic payload
''     - report file-open failure through the process exit status
''
'' This file intentionally does NOT contain:
''
''     - console input or output
''     - graphics or sound
''     - network access
''

Const OutputName = "\Storage Card\fb-wince-smoke.txt"

Dim As Integer output_file = FreeFile()

If Open( OutputName For Output As #output_file ) <> 0 Then End 1
Print #output_file, "FreeBASIC Windows CE runtime smoke passed"
Close #output_file

End 0

'' end of tests/wince/basic_file.bas
