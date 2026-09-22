'' examples/manual/console/width.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'WIDTH'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgWidth
'' --------

''Set up a graphics screen
Const W = 320, H = 200
If ScreenRes( W, H ) <> 0 Then
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

Dim As Long twid
Dim As UInteger tw, th

'' Fetch and print current text width/height:
twid = Width()
tw = LoWord(twid): th = HiWord(twid)
Print "Default for current screen (8*8)"
Print "Width:  " & tw
Print "Height: " & th
Sleep

' end of width.bas

Width W\8, H\16 '' Use 8*16 font

twid = Width()
tw = LoWord(twid): th = HiWord(twid)
Print "Set to 8*16 font"
Print "Width:  " & tw
Print "Height: " & th
Sleep

Width W\8, H\14 '' Use 8*14 font

twid = Width()
tw = LoWord(twid): th = HiWord(twid)
Print "Set to 8*14 font"
Print "Width:  " & tw
Print "Height: " & th
Sleep

Width W\8, H\8 '' Use 8*8 font

twid = Width()
tw = LoWord(twid): th = HiWord(twid)
Print "Set to 8*8 font"
Print "Width:  " & tw
Print "Height: " & th
Sleep
