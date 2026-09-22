'' examples/manual/control/goto.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'GOTO'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgGoto
'' --------

	'' This lesson demonstrates the initial forward GOTO.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL101
	Goto there

backagain:
	End

there:
	Print "Welcome!"
	'' This lesson demonstrates the matching jump back to the terminating label.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL101
	Goto backagain
