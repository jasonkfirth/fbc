'' Project: FreeBASIC compiler backend regression tests
'' File: thread-local-array-provider.bas
'' Purpose: Supply initialized TLS storage to the array module regression.
'' Responsibilities: Export scalar and fixed-array templates and read active values.
'' This file intentionally does NOT create threads or import external C TLS.

#lang "fb"

Public Thread Local Dim Shared externalValue As Long = 17
Public Thread Local Dim Shared externalGrid(-2 To -1, 3 To 4) As Long = {{3, 4}, {5, 6}}

Function externalRead() As Long
	Return externalValue + externalGrid(-2, 3)
End Function

'' end of thread-local-array-provider.bas
