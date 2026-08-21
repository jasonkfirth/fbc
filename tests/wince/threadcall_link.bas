''
'' FreeBASIC Windows CE ThreadCall link smoke test
'' ------------------------------------------------
''
'' File: tests/wince/threadcall_link.bas
''
'' Purpose:
''
''     Force the Windows CE thread launcher and ARM libffi call bridge into a
''     target executable.
''
'' Responsibilities:
''
''     - pass a typed argument through THREADCALL
''     - wait for the worker before examining its result
''     - provide a small executable suitable for emulator validation
''
'' This file intentionally does NOT contain:
''
''     - stress testing
''     - synchronization primitive coverage
''     - emulator startup policy
''

Sub set_result( ByVal result As Integer Ptr )
	*result = 42
End Sub

Dim result As Integer
Dim worker As Any Ptr

worker = ThreadCall set_result( @result )
If worker = 0 Then
	End 1
End If

ThreadWait worker
If result <> 42 Then
	End 2
End If

End 0

'' end of tests/wince/threadcall_link.bas
