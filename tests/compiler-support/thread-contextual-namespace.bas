'' Project: FreeBASIC compiler backend regression tests
'' File: thread-contextual-namespace.bas
'' Purpose: Keep contextual THREAD from hiding an ordinary namespace.
'' Responsibilities: Check namespace creation, reopening and TLS prefix recognition.
'' This file intentionally does NOT contain worker threads or external dependencies.

#lang "fb"

Namespace Thread
	Const first As Integer = 17
End Namespace

Namespace Thread
	Const second As Integer = 18
End Namespace

Thread Local Dim Shared marker As Long = 7
If Thread.first <> 17 OrElse Thread.second <> 18 OrElse marker <> 7 Then End 1
Print "CONTEXTUAL_THREAD_NAMESPACE_PASS"

'' end of thread-contextual-namespace.bas
