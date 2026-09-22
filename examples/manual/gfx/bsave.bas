'' examples/manual/gfx/bsave.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'BSAVE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgBsave
'' --------

' Set gfx mode
If ScreenRes(320, 200, 32) <> 0 Then
  Print "Could not set the graphics mode"
Else

  ' Clear with black on white
  Color RGB(0, 0, 0), RGB(255, 255, 255)
  Cls

  Locate 13, 15: Print "Hello world!"

  ' Save screen as BMP
  If BSave("hello.bmp", 0) <> 0 Then Print "Could not save hello.bmp"
End If
