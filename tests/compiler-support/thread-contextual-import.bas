'' Project: FreeBASIC compiler backend regression tests
'' File: thread-contextual-import.bas
'' Purpose: Keep contextual THREAD from hiding imported declarations.
'' Responsibilities: Check USING lookup alongside the THREAD LOCAL prefix.
'' This file intentionally does NOT contain worker threads or external dependencies.

#lang "fb"

Namespace ImportedNames
	Function Thread() As Integer
		Return 17
	End Function
End Namespace

Using ImportedNames
Thread Local Dim Shared marker As Long = 7
If Thread() <> 17 OrElse marker <> 7 Then End 1
Print "CONTEXTUAL_THREAD_IMPORT_PASS"

'' end of thread-contextual-import.bas
