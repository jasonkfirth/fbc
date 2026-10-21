'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: thread-local-managed.bas
''
'' Purpose:
''
''     Verify Thread Local rejects managed storage that needs per-thread constructors.
''
'' Responsibilities:
''
''     - exercise a managed FreeBASIC string declaration
''     - exercise a dynamic initializer
''     - keep unsupported storage from being silently shared
''
'' This file intentionally does NOT contain:
''
''     - a runnable program; compilation is expected to fail
''

#lang "fb"

Private Function GetThreadLocalInitialValue() As Long
	Return 7
End Function

Thread Local Dim Shared threadLocalText As String
Thread Local Dim Shared threadLocalDynamicValue As Long = GetThreadLocalInitialValue()

'' end of thread-local-managed.bas
