'' examples/manual/proguide/errors/on-local.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Error Handling'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgErrorHandling
'' --------

'' Compile with -e
'' The -e command line option is needed to enable error handling.

Declare Sub foo
  foo
Sleep

Sub foo

	Dim filename As String
	Dim errmsg As String
	Dim file_number As Integer
	filename = ""
	file_number = FreeFile
	' This procedure-local handler covers the one demonstration open only. The
	' surrounding file documents the required -e compiler option.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-004 FBL-ERR-005
	On Local Error Goto fail
  Open filename For Input Access Read As #file_number
	Close #file_number
	Print "No error"
	On Local Error Goto 0
	Exit Sub

  fail:
  ' The handler label is reachable only through On Local Error, so these error
  ' intrinsics describe the failed Open rather than an ordinary program state.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL613
  errmsg = "Error " & Err
  ' ERFN is the legacy error-handler intrinsic for the failing function name.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL310
  errmsg &= " in function " & *Erfn
  errmsg &= " on line " & Erl
  Print errmsg

End Sub

'' end of on-local.bas
