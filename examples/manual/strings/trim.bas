'' examples/manual/strings/trim.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'TRIM'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgTrim
'' --------

Dim s1 As String = " ... Stuck in the middle ... "
Print "'" + Trim(s1) + "'"
'' This intentionally removes the selected characters from both string ends.
'' FB-LINTER: DISABLE-NEXT-LINE FBL516
Print "'" + Trim(s1, Any " .") + "'"

Dim s2 As String = "BaaBaaaaB With You aaBBaaBaa"
'' This intentionally removes the selected substring from both string ends.
'' FB-LINTER: DISABLE-NEXT-LINE FBL516
Print "'" + Trim(s2, "Baa") + "'"
'' This intentionally removes any selected character from both string ends.
'' FB-LINTER: DISABLE-NEXT-LINE FBL516
Print "'" + Trim(s2, Any "Ba") + "'"
