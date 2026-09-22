'' examples/manual/proguide/lineseparator.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Line Separator'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgLineSeparator
'' --------

'' These 5 statements are stacked on the same line, using the ':' character as a separator
'' The initialized declaration is intentional here because it is one of those five statements.
'' FB-LINTER: DISABLE-NEXT-LINE FBL104
Dim As String text = "Hello!" : Color 14, 3 : Print text : Color 7, 0 : Sleep
