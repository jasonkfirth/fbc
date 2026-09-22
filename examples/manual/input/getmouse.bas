'' examples/manual/input/getmouse.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'GETMOUSE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgGetmouse
'' --------


Dim As Long x, y, buttons, res
' Set video mode and enter loop
If ScreenRes(640, 480, 8) <> 0 Then
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

Do
	' Get mouse x, y and buttons. Discard wheel position.
	res = GetMouse (x, y, , buttons)
	Locate 1, 1
	If res <> 0 Then '' Failure

#ifdef __FB_DOS__
		Print "Mouse or mouse driver not available"
#else
		Print "Mouse not available or not on window"
#endif

	Else
		Print Using "Mouse position: ###:###  Buttons: "; x; y;
		If buttons And 1 Then Print "L";
		If buttons And 2 Then Print "R";
		If buttons And 4 Then Print "M";
		Print "   "
	End If

	Sleep 15, 1
Loop While Inkey = ""
End
