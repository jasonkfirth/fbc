'
'   FreeBASIC Wii backend smoke tests
'   ---------------------------------
'
'   File: gfx_sfx_smoke.bas
'
'   Purpose:
'
'       Exercise the Wii gfxlib and sfxlib backends in Dolphin or on
'       hardware with observable output artifacts.
'
'   Responsibilities:
'
'       - switch through SCREEN 0 to SCREEN 13
'       - draw red, green, and blue screen regions in every graphics mode
'       - switch from legacy SCREEN modes to paged 32-bit SCREENRES
'       - prove SCREENSET and SCREENCOPY by drawing through hidden pages
'       - play one foreground tone and one background tone through sfxlib
'       - write a simple pass/fail log for unattended emulator runs
'
'   This file intentionally does NOT contain:
'
'       - Dolphin launch logic
'       - host-side image or audio analysis
'       - controller input tests
'

#include once "fbgfx.bi"

Declare Function fb_sfxDeviceCurrent CDecl Alias "fb_sfxDeviceCurrent" ( ) As Integer
Declare Function fb_sfxDeviceName CDecl Alias "fb_sfxDeviceName" ( ByVal id As Integer ) As ZString Ptr

Const LogFile = "wii-gfx-sfx-smoke.log"
Const DriverDumpFile = "wii-sfx-driver.tmp"

Dim Shared As Integer logHandle
Dim Shared As Integer failures

Sub LogLine( ByVal text As String )
	Print #logHandle, text
End Sub

Sub Pass( ByVal text As String )
	LogLine "PASS " + text
End Sub

Sub Fail( ByVal text As String )
	failures += 1
	LogLine "FAIL " + text
End Sub

Function TwoDigits( ByVal value As Integer ) As String
	Dim As String text = LTrim(Str(value))

	If Len(text) < 2 Then
		text = "0" + text
	End If

	Return text
End Function

Sub SetXfbDump( ByVal prefix As String )
	SetEnviron "FBGFX_WII_XFB_DUMP_PREFIX=" + prefix
End Sub

Function ColorStrong( ByVal value As UInteger, ByVal colorChannel As Integer ) As Integer
	Dim As Integer r = (value Shr 16) And &hff
	Dim As Integer g = (value Shr 8) And &hff
	Dim As Integer b = value And &hff

	Select Case colorChannel
	Case 0
		Return (r > 120 And r > g + 40 And r > b + 40)
	Case 1
		Return (g > 100 And g > r + 30 And g > b + 30)
	Case Else
		Return (b > 100 And b > r + 30 And b > g + 30)
	End Select
End Function

Sub DrawRgbBars( ByVal mode As Integer, ByVal label As String, ByVal presentAfterDraw As Integer = -1 )
	Dim As Integer w, h, depth, bypp, pitch
	Dim As Integer x1, x2
	Dim As UInteger redColor, greenColor, blueColor
	Dim As UInteger redPoint, greenPoint, bluePoint

	ScreenInfo w, h, depth, bypp, pitch

	If w <= 0 Or h <= 0 Then
		Fail label + " reported invalid size"
		Exit Sub
	End If

	x1 = w \ 3
	x2 = (w * 2) \ 3

	If depth >= 24 Then
		redColor = Rgb(255, 0, 0)
		greenColor = Rgb(0, 255, 0)
		blueColor = Rgb(0, 0, 255)
	ElseIf depth > 1 Then
		Palette 1, 255, 0, 0
		Palette 2, 0, 255, 0
		Palette 3, 0, 0, 255
		redColor = 1
		greenColor = 2
		blueColor = 3
	Else
		Palette 0, 0, 0, 0
		Palette 1, 255, 255, 255
		redColor = 0
		greenColor = 1
		blueColor = 1
	End If

	Cls
	Line (0, 0)-(x1 - 1, h - 1), redColor, BF
	Line (x1, 0)-(x2 - 1, h - 1), greenColor, BF
	Line (x2, 0)-(w - 1, h - 1), blueColor, BF

	Color Rgb(255, 255, 255), Rgb(0, 0, 0)
	Locate 2, 2
	Print "WII GFX SMOKE"
	Locate 4, 2
	Print label
	Locate 6, 2
	Print "w="; w; " h="; h; " depth="; depth; " bypp="; bypp

	If presentAfterDraw <> 0 Then
		ScreenSync
		Sleep 80, 1
	End If

	redPoint = Point(w \ 6, h \ 2)
	greenPoint = Point(w \ 2, h \ 2)
	bluePoint = Point((w * 5) \ 6, h \ 2)

	LogLine label + " size=" + Str(w) + "x" + Str(h) + _
		" depth=" + Str(depth) + " bypp=" + Str(bypp) + _
		" points=" + Hex(redPoint, 8) + "," + _
		Hex(greenPoint, 8) + "," + Hex(bluePoint, 8)

	If depth >= 24 Then
		If ColorStrong(redPoint, 0) = 0 Then Fail label + " red region was not red"
		If ColorStrong(greenPoint, 1) = 0 Then Fail label + " green region was not green"
		If ColorStrong(bluePoint, 2) = 0 Then Fail label + " blue region was not blue"
	ElseIf depth > 1 Then
		If redPoint <> redColor Then Fail label + " red palette index did not survive"
		If greenPoint <> greenColor Then Fail label + " green palette index did not survive"
		If bluePoint <> blueColor Then Fail label + " blue palette index did not survive"
	Else
		If redPoint = greenPoint Then
			Fail label + " one-bit regions did not produce distinct values"
		End If
	End If
End Sub

Sub ExerciseScreenModes()
	Dim As Integer mode

	Screen 0
	Cls
	Print "WII SCREEN 0 smoke"
	LogLine "SCREEN 0 text mode reached"
	Sleep 200, 1
	Pass "SCREEN 0"

	For mode = 1 To 13
		SetXfbDump "wii-xfb-screen-" + TwoDigits(mode)
		Screen mode
		DrawRgbBars mode, "SCREEN " + Str(mode)
		Pass "SCREEN " + Str(mode)
	Next mode
End Sub

Sub ExerciseScreenSet()
	ScreenRes 320, 240, 32, 2

	SetXfbDump "wii-xfb-screenset-a"
	ScreenSet 1, 0
	DrawRgbBars -1, "SCREENRES 320x240x32 page 1 -> visible 0", 0
	ScreenCopy 1, 0
	Sleep 120, 1
	Pass "SCREENSET 1,0 and SCREENCOPY"

	SetXfbDump "wii-xfb-screenset-b"
	ScreenSet 0, 1
	DrawRgbBars -1, "SCREENRES 320x240x32 page 0 -> visible 1", 0
	ScreenCopy 0, 1
	Sleep 120, 1
	Pass "SCREENSET 0,1 and SCREENCOPY"
End Sub

Sub ExerciseSound()
	Dim As Double started
	Dim As Double elapsedBlocking
	Dim As Double elapsedBackground
	Dim As Integer driver
	Dim As Integer selectResult
	Dim As ZString Ptr driverName

	SetEnviron "SFXLIB_DRIVER=asnd"
	SetEnviron "SFXLIB_DRIVER_DUMP=" + DriverDumpFile
	SetEnviron "SFXLIB_DRIVER_DUMP_FRAMES=96000"

	selectResult = fb_sfxDeviceSelect(0)
	LogLine "asnd select result=" + Str(selectResult)
	If selectResult <> 0 Then
		Fail "ASND device select failed"
	End If

	started = Timer
	Sound 440, 5
	elapsedBlocking = Timer - started
	LogLine "blocking sound elapsed=" + Str(elapsedBlocking)

	driver = fb_sfxDeviceCurrent()
	driverName = fb_sfxDeviceName(driver)
	If driverName <> 0 Then
		LogLine "sfx driver=" + *driverName
		If LCase(*driverName) <> "asnd" Then
			Fail "sfx driver was not ASND"
		End If
	Else
		Fail "sfx driver name was unavailable"
	End If

	If elapsedBlocking < 0.05 Then
		Fail "foreground SOUND returned too quickly"
	Else
		Pass "foreground SOUND"
	End If

	started = Timer
	Sound 1, 660, 0.35, 0.60
	elapsedBackground = Timer - started
	LogLine "background sound elapsed=" + Str(elapsedBackground)

	If elapsedBackground > 0.20 Then
		Fail "background SOUND blocked too long"
	Else
		Pass "background SOUND"
	End If

	Sleep 700, 1
	Pass "sfx driver dump requested"
End Sub

SetEnviron "FBGFX_WII_XFB_DUMP_FRAMES=32"

logHandle = FreeFile()
Open LogFile For Output As #logHandle

LogLine "WII GFX/SFX SMOKE START"

ExerciseScreenModes()
ExerciseScreenSet()
ExerciseSound()

If failures = 0 Then
	LogLine "RESULT PASS"
Else
	LogLine "RESULT FAIL failures=" + Str(failures)
End If

Close #logHandle

End failures

' end of gfx_sfx_smoke.bas
