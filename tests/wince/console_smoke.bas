''
'' FreeBASIC Windows CE console smoke test
'' ---------------------------------------
''
'' File: tests/wince/console_smoke.bas
''
'' Purpose:
''
''     Force the complete logical text-console surface into one target image.
''
'' Responsibilities:
''
''     - exercise text-grid sizing, colors, cursor movement, and scrolling
''     - exercise text-page selection, SCREEN queries, and PCOPY
''     - link keyboard and pointer polling without waiting for user input
''
'' This file intentionally does NOT test:
''
''     - graphics-mode rendering
''     - interactive key timing
''     - sound or network services
''

Const OutputName = "\Storage Card\fb-wince-console-smoke.txt"

Dim As Integer columns
Dim As Integer rows
Dim As Integer console_size
Dim As Integer mouse_x
Dim As Integer mouse_y
Dim As Integer mouse_z
Dim As Integer mouse_buttons
Dim As Integer mouse_clip
Dim As Integer observed_character
Dim As Integer observed_color
Dim As Integer output_file = FreeFile()

Width 40, 20
console_size = Width()
columns = console_size And &hFFFF
rows = (console_size Shr 16) And &hFFFF

Cls
Color 14, 1
Locate 2, 3, 1
Print "CE"

observed_character = Screen( 2, 3, 0 )
observed_color = Screen( 2, 3, 1 )

View Print 2 To 10
Print String( 12, "x" )
Cls 2
View Print

Screen , 1, 1
Print "page one"
PCopy 1, 0
Screen , 0, 0

If MultiKey( &h01 ) Then
	Print "escape pressed"
End If

If GetMouse( mouse_x, mouse_y, mouse_z, mouse_buttons, mouse_clip ) = 0 Then
	SetMouse mouse_x, mouse_y, 1, 0
End If

If Open( OutputName For Output As #output_file ) <> 0 Then End 1
Print #output_file, columns;","; rows;","; observed_character;","; observed_color
Close #output_file

End 0

'' end of console_smoke.bas
