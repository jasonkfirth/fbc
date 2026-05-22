'
' File: android-perception.bas
'
' Purpose:
'
'     Android Human Perception Test
'
' This program intentionally runs interactively. It is meant for
' human observation across graphics modes, resolutions, touch input,
' and app lifecycle transitions.
'
' How to use:
'
'     fbc-android --runOnPhone --package org.freebasic.perception.perf tests/interactive/android-perception.bas
'
Type PerceptionResult
	section As String
	status As String
	note_text As String
End Type

#Include Once "fbgfx.bi"

Sub set_ui_text_scale()
	Dim As Integer sw
	Dim As Integer sh
	Dim As Integer sd
	Dim As Integer sb
	Dim As Integer sp
	Dim As Integer text_cols
	Dim As Integer text_rows

	screeninfo sw, sh, sd, sb, sp
	text_cols = sw \ 8
	text_rows = sh \ 16
	If text_cols < 20 Then text_cols = 20
	If text_rows < 12 Then text_rows = 12
	Width text_cols, text_rows
End Sub

Sub set_readable_gameframe()
	Dim As Integer screen_w
	Dim As Integer screen_h
	Dim As Integer mode_w
	Dim As Integer mode_h

	ScreenControl fb.GET_SCREEN_SIZE, screen_w, screen_h
	If screen_w < 320 Then
		screen_w = 960
	End If
	If screen_h < 240 Then
		screen_h = 540
	End If

	mode_w = screen_w \ 3
	mode_h = screen_h \ 3
	mode_w = (screen_w * 2) \ 3
	mode_h = (screen_h * 2) \ 3
	If mode_w < 320 Then
		mode_w = 320
	End If
	If mode_h < 240 Then
		mode_h = 240
	End If

	If screenres(mode_w, mode_h, 32) <> 0 Then
		If screenres(640, 360, 32) <> 0 Then
			screenres(320, 240, 16)
		End If
	End If
	set_ui_text_scale()
End Sub

Function normalize_choice(ByVal value As String) As String
	Dim As String c

	c = Trim$(UCase$(value))
	If Len(c) = 0 Then
		Function = ""
	Else
		Function = Left$(c, 1)
	End If
End Function

Sub show_header(ByVal section_name As String, ByVal section_goal As String)
	set_ui_text_scale()
	Cls
	Color Rgb(255, 255, 255), Rgb(0, 0, 0)
	Print "FreeBASIC Android Human Perception Test"
	Print "--------------------------------------"
	Print section_name
	Print section_goal
	Print
End Sub

Sub collect_verdict(ByRef result As PerceptionResult)
	Dim As String response
	Dim As String note_text

	Do
		Input "Result: [O]K  [F]ail  [N]ote  -> ", response
		response = normalize_choice(response)
		Select Case response
			Case "O"
				result.status = "OK"
				Exit Do
			Case "F"
				result.status = "FAIL"
				Exit Do
			Case "N"
				Line Input "Note: ", note_text
				result.status = "NOTE"
				result.note_text = note_text
				Exit Do
			Case Else
				Print "Please enter O, F, or N."
				Print
		End Select
	Loop
End Sub

Sub run_device_introspection(ByRef result As PerceptionResult)
	Dim As Integer logical_w, logical_h
	Dim As Integer depth, bpp, pitch
	Dim As Integer screen_w, screen_h

	result.section = "Device reporting"

	set_readable_gameframe()

	screeninfo logical_w, logical_h, depth, bpp, pitch
	ScreenControl fb.GET_SCREEN_SIZE, screen_w, screen_h

	show_header("Step 1: device and screen reporting", "Baseline")
	Print "Press any key after reading this section."
	Print
	Print "FB screen mode reported by screeninfo:"
	Print "    " & logical_w & " x " & logical_h & " @" & depth & " bit"
	Print
	Print "Screen pitch: " & pitch
	Print "Framebuffer hint: " & screen_w & " x " & screen_h
	Print
	Print "Expected: values should look reasonable on the attached phone."
	Sleep 3, 1
	collect_verdict(result)
End Sub

Sub render_pattern(ByVal label As String, ByVal req_width As Integer, ByVal req_height As Integer, ByVal expected_depth As Integer)
	Dim As Integer cx, cy
	Dim As Integer band_a

	cx = req_width \ 2
	cy = req_height \ 2
	band_a = req_height \ 20
	If band_a < 12 Then band_a = 12

	Line (0, 0)-(req_width - 1, req_height - 1), Rgb(16, 16, 20), BF
	Line (0, cy - band_a)-(req_width - 1, cy - band_a + 3), Rgb(192, 0, 0), BF
	Line (0, cy)-(req_width - 1, cy + 3), Rgb(0, 160, 0), BF
	Line (0, cy + band_a)-(req_width - 1, cy + band_a + 3), Rgb(0, 0, 192), BF
	Print "Pattern label: " & label
	Print "Expected depth: " & expected_depth
	Print "Current size:  " & req_width & " x " & req_height
	Print "Tap around and visually check the three bands stay crisp."
End Sub

Sub run_mode_case(ByVal request_w As Integer, ByVal request_h As Integer, ByVal request_bpp As Integer, ByVal label As String, ByRef result As PerceptionResult)
	Dim As Integer rc
	Dim As Integer mode_w, mode_h, mode_depth, mode_bpp, mode_pitch

	result.section = "Mode: " & label
	show_header("Step 2: screen mode transitions", label)
	rc = screenres(request_w, request_h, request_bpp)
	If rc <> 0 Then
		Print "Could not set request: " & request_w & "x" & request_h & "x" & request_bpp
		Print "This is an edge case worth capturing."
		collect_verdict(result)
		Exit Sub
	End If
	set_ui_text_scale()

	screeninfo mode_w, mode_h, mode_depth, mode_bpp, mode_pitch
	render_pattern(label, mode_w, mode_h, request_bpp)
	Print "Actual graphics mode: " & mode_w & " x " & mode_h & " x " & mode_depth
	Print "Requested: " & request_w & " x " & request_h & " x " & request_bpp
	Print
	Print "If this mode is fully visible and touchable, judge OK."
	collect_verdict(result)
End Sub

Sub run_mouse_probe(ByRef result As PerceptionResult)
	Dim As Integer x, y, z, buttons
	Dim As Integer screen_w, screen_h, screen_depth, screen_bpp, screen_pitch
	Dim As Double end_time
	Dim As Integer observed, pressed, last_down
	Dim As Integer last_x, last_y
	Dim As Integer last_buttons
	Dim As String state

	result.section = "Mouse and touch"
	set_readable_gameframe()
	screeninfo screen_w, screen_h, screen_depth, screen_bpp, screen_pitch
	last_x = -1
	last_y = -1
	last_buttons = 0

	show_header("Step 3: mouse/touch behavior", "Touch path + drag check")
	Print "For ~12 seconds: touch, drag, and press while watching the crosshair."
	observed = 0
	pressed = 0
	last_down = 0
	end_time = Timer + 12.0

	Do While Timer < end_time
		getmouse x, y, z, buttons
		Cls
		show_header("Step 3: mouse/touch behavior", "Touch path + drag check")
		Print "Touches detected: " & observed
		Print "Primary presses: " & pressed
		Print "Time remaining: " & CInt(end_time - Timer) & " seconds"
		Print

		If x >= 0 And y >= 0 And x < screen_w And y < screen_h Then
			If buttons <> last_buttons Or x <> last_x Or y <> last_y Then
				observed = observed + 1
			End If
			Line (x - 10, y)-(x + 10, y), Rgb(255, 255, 0)
			Line (x, y - 10)-(x, y + 10), Rgb(255, 255, 0)
			Circle (x, y), 14, Rgb(255, 128, 0)
		Else
			Print "Waiting for pointer or touch input..."
		End If

		If (buttons And 1) <> 0 Then
			state = "down"
			If last_down = 0 Then
				pressed = pressed + 1
			End If
		Else
			state = "up"
		End If

		last_down = buttons And 1
		last_x = x
		last_y = y
		last_buttons = buttons
		Print "Button state: " & state
		Print "If the crosshair follows your finger and click/press appears, it is good."
		Sleep 120, 1
	Loop

	If observed = 0 Then
		Print
		Print "No touch sample was observed."
	End If
	collect_verdict(result)
End Sub

Sub run_keyboard_probe(ByRef result As PerceptionResult)
	Dim As String typed
	Dim As String key
	Dim As Integer key_count
	Dim As Double end_time

	result.section = "Keyboard / inkey"
	set_readable_gameframe()

	show_header("Step 4: keyboard input", "IME + getkey path")
	Print "Spend about 8 seconds typing on the keyboard if available."
	Print

	key_count = 0
	end_time = Timer + 8.0
	Do While Timer < end_time
		key = Inkey()
		If Len(key) > 0 Then
			key_count = key_count + 1
		End If
		Sleep 50, 1
	Loop

	Print "Presses seen through inkey: " & key_count
	Print
	Input "Type a short phrase then Enter: ", typed
	If typed = "" Then
		Print "No typed phrase was entered."
	End If

	collect_verdict(result)
End Sub

Sub run_lifecycle_probe(ByRef result As PerceptionResult)
	Dim As Double end_time
	result.section = "Lifecycle leave/return"

	show_header("Step 5: lifecycle", "Press HOME and return to app")
	Print "Instructions:"
	Print "- Tap Home (or recent-app exit) now."
	Print "- Return to this app before timer ends."
	Print "- Confirm whether counter and visual continue correctly."
	Print
	Print "When you return, you should still be in this section."
	end_time = Timer + 25.0

	Do While Timer < end_time
		Cls
		show_header("Step 5: lifecycle", "Press HOME and return to app")
		Print "Return check countdown: " & CInt(end_time - Timer) & " seconds"
		Print "If the app is resumed correctly, this loop continues where you left it."
		Sleep 1000, 1
	Loop

	collect_verdict(result)
End Sub

Sub run_resolution_variants(ByRef result1 As PerceptionResult, ByRef result2 As PerceptionResult, ByRef result3 As PerceptionResult, ByRef result4 As PerceptionResult)
	Dim As Integer phone_w
	Dim As Integer phone_h
	Dim As Integer test_w1, test_h1
	Dim As Integer test_w2, test_h2
	Dim As Integer test_w3, test_h3

	phone_w = 1024
	phone_h = 1024
	ScreenControl fb.GET_SCREEN_SIZE, phone_w, phone_h
	If phone_w < 320 Or phone_h < 240 Then
		phone_w = 960
		phone_h = 540
	End If

	test_w1 = phone_w
	test_h1 = phone_h
	test_w2 = phone_w \ 2
	test_h2 = phone_h \ 2
	test_w3 = phone_w \ 3
	test_h3 = phone_h \ 3
	If test_w2 < 1 Then
		test_w2 = test_w1
		test_h2 = test_h1
	End If
	If test_h2 < 1 Then
		test_w2 = test_w1
		test_h2 = test_h1
	End If
	If test_w3 < 1 Then
		test_w3 = test_w1
		test_h3 = test_h1
	End If
	If test_h3 < 1 Then
		test_w3 = test_w1
		test_h3 = test_h1
	End If
	run_mode_case(test_w1, test_h1, 32, "native", result1)
	run_mode_case(test_w2, test_h2, 32, "half", result2)
	run_mode_case(test_w3, test_h3, 32, "third", result3)
	run_mode_case(360, 640, 32, "portrait-like", result4)
End Sub

Sub print_summary(ByRef r1 As PerceptionResult, _
		ByRef r2 As PerceptionResult, _
		ByRef r3 As PerceptionResult, _
		ByRef r4 As PerceptionResult, _
		ByRef r5 As PerceptionResult, _
		ByRef r6 As PerceptionResult, _
		ByRef r7 As PerceptionResult, _
		ByRef r8 As PerceptionResult)

	show_header("Test summary", "Final pass report")
	Print "1. " & r1.section
	Print "   Status: " & r1.status
	If Len(r1.note_text) > 0 Then
		Print "   Note:   " & r1.note_text
	End If
	Print
	Print "2. " & r2.section
	Print "   Status: " & r2.status
	If Len(r2.note_text) > 0 Then
		Print "   Note:   " & r2.note_text
	End If
	Print
	Print "3. " & r3.section
	Print "   Status: " & r3.status
	If Len(r3.note_text) > 0 Then
		Print "   Note:   " & r3.note_text
		Print
	End If
	Print
	Print "4. " & r4.section
	Print "   Status: " & r4.status
	If Len(r4.note_text) > 0 Then
		Print "   Note:   " & r4.note_text
	End If
	Print
	Print "5. " & r5.section
	Print "   Status: " & r5.status
	If Len(r5.note_text) > 0 Then
		Print "   Note:   " & r5.note_text
	End If
	Print
	Print "6. " & r6.section
	Print "   Status: " & r6.status
	If Len(r6.note_text) > 0 Then
		Print "   Note:   " & r6.note_text
	End If
	Print
	Print "7. " & r7.section
	Print "   Status: " & r7.status
	If Len(r7.note_text) > 0 Then
		Print "   Note:   " & r7.note_text
	End If
	Print
	Print "8. " & r8.section
	Print "   Status: " & r8.status
	If Len(r8.note_text) > 0 Then
		Print "   Note:   " & r8.note_text
	End If
	Print

	Print "Thank you. Keep this result list for your report."
	Sleep 1, 1
	Input "Type anything to exit this program: ", r1.note_text
End Sub

Function fb_android_program_main(ByVal argc As Integer, ByVal argv As ZString Ptr Ptr) As Integer
	Dim As PerceptionResult result1
	Dim As PerceptionResult result2
	Dim As PerceptionResult result3
	Dim As PerceptionResult result4
	Dim As PerceptionResult result5
	Dim As PerceptionResult result6
	Dim As PerceptionResult result7
	Dim As PerceptionResult result8

	run_device_introspection(result1)
	run_resolution_variants(result2, result3, result4, result5)
	run_mouse_probe(result6)
	run_keyboard_probe(result7)
	run_lifecycle_probe(result8)
	print_summary(result1, result2, result3, result4, result5, result6, result7, result8)
	fb_android_program_main = 0
End Function

' end of tests/interactive/android-perception.bas
