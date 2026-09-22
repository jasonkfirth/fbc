'' examples/manual/gfx/windowtitle.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'WINDOWTITLE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgWindowtitle
'' --------

'Set screen mode
'' Mode 13 is the documented compatibility mode for this WINDOWTITLE lesson.
'' FB-LINTER: DISABLE-NEXT-LINE FBL734
Screen 13

'Set the window title
WindowTitle "FreeBASIC example program"

Sleep
