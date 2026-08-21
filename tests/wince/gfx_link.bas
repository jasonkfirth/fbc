''
'' FreeBASIC Windows CE gfxlib2 link smoke test
'' ---------------------------------------------
''
'' File: tests/wince/gfx_link.bas
''
'' Purpose:
''
''     Force the Windows CE GDI presentation and input backend into one target
''     executable suitable for emulator validation.
''
'' Responsibilities:
''
''     - open a 32-bit gfxlib2 screen
''     - draw representative software primitives
''     - exercise the native mouse query entry point
''
'' This file intentionally does NOT contain:
''
''     - performance measurement
''     - interactive assertions
''     - emulator startup policy
''

Dim mouse_x As Integer
Dim mouse_y As Integer
Dim mouse_wheel As Integer
Dim mouse_buttons As Integer

If ScreenRes( 320, 240, 32 ) <> 0 Then
	End 1
End If

Line ( 0, 0 )-( 319, 239 ), RGB( 255, 0, 0 )
Circle ( 160, 120 ), 48, RGB( 0, 255, 0 )
Draw String ( 8, 8 ), "FreeBASIC Windows CE", RGB( 255, 255, 255 )
GetMouse mouse_x, mouse_y, mouse_wheel, mouse_buttons
ScreenSync
Screen 0

End 0

'' end of tests/wince/gfx_link.bas
