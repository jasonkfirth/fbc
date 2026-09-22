'' examples/manual/proguide/errors/on-error2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Error Handling'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgErrorHandling
'' --------

'' Resource ownership:
'' Each configuration closes file unit 1 only after its corresponding OPEN
'' succeeded. The intentionally failing paths never acquire an owned handle.

#define Config 1
'#DEFINE Config 2
'#DEFINE Config 3
'#DEFINE Config 4



#if Config = 1 '-----------------------------------------------------------

'' This configuration intentionally demonstrates an unchecked legacy OPEN.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-IO-001 FBL-IO-002
Open "does_not_exist" For Input As #1
Close #1

Print "main end"
Sleep
System

' - with compiler option 'none' :
'     console output :
'       'main end'
'
' - with compiler option '-e' or '-ex' or '-exx' :
'     console output :
'       'Aborting due to runtime error 2 (file not found) at line 10 of .....'

#endif '-------------------------------------------------------------------



#if Config = 2 '-----------------------------------------------------------

'' This configuration intentionally prints OPEN's direct return value.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-IO-002
Dim As Integer Result = Open("does_not_exist" For Input As #1)
If Result <> 0 Then
	Print "error code returned: " & Result
	Print "file not found (processed by 'Result = Open(.....)')"
Else
	Close #1
End If

Print "main end"
Sleep
End

' - with compiler option 'none' or '-e' or '-ex' or '-exx' :
'     console output :
'       'error code returned: 2'
'       'file not found (processed by 'Result = Open(.....)')'
'       'main end'

#endif '-------------------------------------------------------------------



#if Config = 3 '-----------------------------------------------------------

'' This configuration intentionally demonstrates a legacy ON ERROR scope.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-004 FBL-ERR-005
On Error Goto Error_Handler
'' This literal file unit is part of the legacy ON ERROR demonstration.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-IO-002
Open "does_not_exist" For Input As #1
Close #1

Print "main end"
Sleep
End

'' END above prevents normal execution from entering this teaching handler.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-006 FBL-CF-004
error_handler:
Print "file not found (processed by 'On Error Goto')"
On Error Goto 0
Print "QB-like error handling end"
Sleep
End

' - with compiler option 'none' :
'     console output :
'       'main end'
'
' - with compiler option '-e' or '-ex' or '-exx' :
'     console output :
'       'file not found (processed by 'On Error Goto')'
'       'QB-like error handling end'

#endif '-------------------------------------------------------------------



#if Config = 4 '-----------------------------------------------------------

'' This configuration deliberately combines legacy handling with OPEN results.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-004 FBL-ERR-005
On Error Goto error_handler
'' This literal file unit is part of the legacy ON ERROR demonstration.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-IO-002
Dim As Integer Result = Open("does_not_exist" For Input As #1)
If Result <> 0 Then
	Print "error code returned: " & Result
	Print "file not found (processed by 'Result = Open(.....)')"
Else
	Close #1
End If

Print "main end"
Sleep
End

'' END above prevents normal execution from entering this teaching handler.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ERR-006 FBL-CF-004
error_handler:
Print "file not found (processed by 'On Error Goto')"
On Error Goto 0
Print "QB-like error handling end"
Sleep
End

' - with compiler option 'none' or '-e' or '-ex' or '-exx' :
'     console output :
'       'error code returned: 2'
'       'file not found (processed by 'Result = Open(.....)')'
'       'main end'

#endif '-------------------------------------------------------------------
