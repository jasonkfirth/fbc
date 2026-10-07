'' Project: FreeBASIC semantic sidecar tests
'' File: pointer-index-inputs.bas
'' Purpose: Preserve built-in pointer indexing before scaling and lowering.
'' Responsibilities: Typed inputs, fields, macros and deliberate exclusions.
'' This file intentionally does NOT execute its pointer accesses.

#define AtElement(buffer, position) buffer[position]
Type IndexCell
    value As Integer
End Type
Type IndexOverride
    value As Integer
    Declare Operator [](ByVal position As Integer) As Integer
End Type

Sub IndexedInputs(ByVal position As Integer)
    Dim p As Integer Ptr, z As ZString Ptr, cellPointer As IndexCell Ptr
    Dim matrix As Integer Ptr Ptr
    Dim actualArray(0 To 1) As Integer
    Dim actualString As String = "value"
    Dim custom As IndexOverride
    Print p[0], p[position], z[1]
    Print cellPointer[position].value
    Print CPtr(Integer Ptr, 128)[2]
    Print matrix[position][0]
    Print SizeOf(p[position])
    Print AtElement(p, 3)
    Print p[1.5]
    Print actualArray(0), actualString[0], custom[1]
#if 0
    Print p[999]
#endif
End Sub

Sub IndexedStores(ByVal position As Integer)
    Dim p As Integer Ptr, matrix As Integer Ptr Ptr
    p[0] = 1
    p[position] += 1
    matrix[0] = p
    matrix[1] = matrix[0]
    AtElement(p, 4) = 2
End Sub

'' end of pointer-index-inputs.bas
