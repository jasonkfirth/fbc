'' examples/manual/faq/gfxlib2/check-for-close.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Frequently Asked FreeBASIC Graphics Library Questions'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=FaqPggfxlib2
'' --------

'' "X" close button example , Win32 and Linux only
Dim As String key
'' Mode 13 is the documented compatibility mode for this close-event lesson.
'' FB-LINTER: DISABLE-NEXT-LINE FBL734
Screen 13
Do
  Print "Click the 'x' to close this app."
	' This is the window-close wait, so it must block until the event arrives.
	' FB-LINTER: DISABLE-NEXT-LINE FBL602
  Sleep
  key = Inkey
Loop Until key = Chr(27) Or key = Chr(255, 107) 'escape or x-button
