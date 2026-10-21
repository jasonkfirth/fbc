'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: incomplete-extern-invalid-value.bas
''
'' Purpose:
''
''     Verify incomplete external objects cannot be read or assigned by value.
''
'' Responsibilities:
''
''     - test comparisons, assignments, conversions, and array declarations
''       that require the incomplete object's size or layout
''
'' This file intentionally does NOT contain:
''
''     - a valid program; the compiler must reject both operations
''
#lang "fb"

Type IncompleteExternObject As IncompleteExternObject_
Extern c2fbIncompleteObject As IncompleteExternObject
Declare Sub c2fbIncompleteByRef CDecl _
	(ByRef value As IncompleteExternObject)

c2fbIncompleteByRef(ByVal c2fbIncompleteObject)
If c2fbIncompleteObject = c2fbIncompleteObject Then End 1
c2fbIncompleteObject = c2fbIncompleteObject
Extern c2fbIncompleteArray(0 To 1) As IncompleteExternObject
Dim invalidIncompleteObject As IncompleteExternObject
Dim invalidConvertedValue As Long = CInt(c2fbIncompleteObject)

'' end of incomplete-extern-invalid-value.bas
