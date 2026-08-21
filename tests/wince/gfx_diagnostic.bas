''
'' FreeBASIC Windows CE gfxlib2 runtime diagnostic
'' ------------------------------------------------
''
'' File: tests/wince/gfx_diagnostic.bas
''
'' Purpose:
''
''     Identify the first gfxlib2 operation that fails in a Windows CE guest.
''
'' Responsibilities:
''
''     - record a durable checkpoint before and after each graphics operation
''     - exercise mode creation, drawing, mouse state, presentation, and exit
''     - leave the last completed checkpoint on CERF's shared storage
''
'' This file intentionally does NOT contain:
''
''     - host emulator automation
''     - framebuffer correctness assertions
''     - performance measurement
''

Const DiagnosticName = "\Storage Card\wince-gfx-diagnostic.txt"

Sub record_checkpoint( ByRef message As Const String )
	Dim As Integer report = FreeFile()

	Open DiagnosticName For Append As #report
	Print #report, message
	Close #report
End Sub

Dim As Integer mouse_x
Dim As Integer mouse_y
Dim As Integer mouse_wheel
Dim As Integer mouse_buttons
Dim As Integer result

If Dir( DiagnosticName ) <> "" Then Kill DiagnosticName
record_checkpoint( "start" )

result = ScreenRes( 320, 240, 32 )
record_checkpoint( "screenres=" & Str( result ) )
If result <> 0 Then End 1

Line ( 0, 0 )-( 319, 239 ), RGB( 255, 0, 0 )
record_checkpoint( "line" )

Circle ( 160, 120 ), 48, RGB( 0, 255, 0 )
record_checkpoint( "circle" )

Draw String ( 8, 8 ), "FreeBASIC Windows CE", RGB( 255, 255, 255 )
record_checkpoint( "draw-string" )

GetMouse mouse_x, mouse_y, mouse_wheel, mouse_buttons
record_checkpoint( "getmouse" )

ScreenSync
record_checkpoint( "screensync" )

Screen 0
record_checkpoint( "screen-exit" )

End 0

'' end of tests/wince/gfx_diagnostic.bas
