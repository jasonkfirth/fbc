/'
    Project FreeBASIC SQLite read-only vector regression
    File sqlite-readonly-vectors.bas
    Purpose Check the production legacy callback adapter and constructor type.
    Responsibilities Preserve pointer identity, integer arguments and results.
    SQL parsing retains the engine's own execution tests.
'/
#lang "fb"
#include once "sqlite3_db_types.bi"

Declare Sub auditCheck CDecl Alias "fb_test_readonly_check" (ByVal condition As Long, ByVal sourceLine As Long)
#define CHECK(condition) auditCheck((condition), __LINE__)

Type LegacyInitCallback As Function CDecl(ByVal context As Any Ptr, ByVal count As SQLITE_C_INT, _
    ByVal values As ZString Ptr Ptr, ByVal columnNames As ZString Ptr Ptr) As SQLITE_C_INT
Declare Function invokeLegacy CDecl Alias "fb_test_readonly_invoke_legacy" _
    (ByVal callback As LegacyInitCallback, ByVal context As Any Ptr, ByVal count As SQLITE_C_INT, _
    ByVal values As ZString Ptr Ptr, ByVal columnNames As ZString Ptr Ptr) As SQLITE_C_INT
Declare Function observeConstructor CDecl Alias "fb_test_readonly_observe_constructor" _
    (ByVal db As Any Ptr, ByVal auxiliary As Any Ptr, ByVal count As SQLITE_C_INT, _
    ByVal arguments As Const ZString Const Ptr Ptr, ByVal vtab As SqliteVtab Ptr Ptr, _
    ByVal message As ZString Ptr Ptr) As SQLITE_C_INT

' One serial case owns these buffers and pointer slots. Read-only clients borrow
' their original addresses, including a null vector when selected by the caller.
Dim Shared rowBytes(0 To 5, 0 To 3) As UByte, nameBytes(0 To 5, 0 To 3) As UByte
Dim Shared rowSlots(0 To 5) As ZString Ptr, nameSlots(0 To 5) As ZString Ptr
Dim Shared expectedContext As Any Ptr, expectedCount As Long, expectedResult As Long
Dim Shared expectedRows As Const ZString Const Ptr Ptr, expectedNames As Const ZString Const Ptr Ptr
Dim Shared contextTag As Long, observedCalls As Long
Dim Shared layoutRecord As SqliteDbInitInfo

' Fixed-width output cells give the C observer one signature at both widths.
Sub initLayout CDecl Alias "fb_test_readonly_init_layout" (ByRef length As ULongInt, ByRef fieldOffset As ULongInt)
    length = SizeOf(SqliteDbInitInfo)
    fieldOffset = CUInt(@layoutRecord.azInit) - CUInt(@layoutRecord)
End Sub

Function sqlite3InitCallbackReadOnly CDecl Alias "sqlite3InitCallbackReadOnly" _
    (ByVal pContext As Any Ptr, ByVal columnCount As SQLITE_C_INT, _
    ByVal values As Const ZString Const Ptr Ptr, ByVal columnNames As Const ZString Const Ptr Ptr) As SQLITE_C_INT
    CHECK(pContext = expectedContext AndAlso columnCount = expectedCount)
    CHECK(values = expectedRows AndAlso columnNames = expectedNames)
    For index As Long = 0 To 5
        If values <> 0 Then
            CHECK(values[index] = rowSlots(index))
            CHECK(CPtr(Const UByte Ptr, values[index])[0] = 65 + index)
        End If
        If columnNames <> 0 Then
            CHECK(columnNames[index] = nameSlots(index))
            CHECK(CPtr(Const UByte Ptr, columnNames[index])[0] = 97 + index)
        End If
    Next
    observedCalls += 1
    Return expectedResult
End Function

' The runner supplies the actual constructor function-pointer declaration and
' the complete production sqlite3InitCallback adapter.
#include once "sqlite-readonly-contract.bi"

Private Function constructorProbe CDecl _
    (ByVal db As Any Ptr, ByVal auxiliary As Any Ptr, ByVal count As SQLITE_C_INT, _
    ByVal arguments As Const ZString Const Ptr Ptr, ByVal vtab As SqliteVtab Ptr Ptr, _
    ByVal message As ZString Ptr Ptr) As SQLITE_C_INT
    Return observeConstructor(db, auxiliary, count, arguments, vtab, message)
End Function

Function getConstructor CDecl Alias "fb_test_readonly_get_constructor" () As c2fb_callback_27
    Dim callback As c2fb_callback_27 = @constructorProbe
    Return callback
End Function

Sub vectorCase CDecl Alias "fb_test_readonly_vector_case" _
    (ByVal count As Long, ByVal nullContext As Long, ByVal nullRows As Long, _
    ByVal nullNames As Long, ByVal result As Long)
    For index As Long = 0 To 5
        rowBytes(index, 0) = CUByte(65 + index)
        rowBytes(index, 1) = 0
        nameBytes(index, 0) = CUByte(97 + index)
        nameBytes(index, 1) = 0
        rowSlots(index) = CPtr(ZString Ptr, @rowBytes(index, 0))
        nameSlots(index) = CPtr(ZString Ptr, @nameBytes(index, 0))
    Next
    expectedContext = IIf(nullContext, 0, @contextTag)
    expectedCount = count
    expectedRows = IIf(nullRows, 0, CPtr(Const ZString Const Ptr Ptr, @rowSlots(0)))
    expectedNames = IIf(nullNames, 0, CPtr(Const ZString Const Ptr Ptr, @nameSlots(0)))
    expectedResult = result
    observedCalls = 0
    Dim rows As ZString Ptr Ptr = IIf(nullRows, 0, @rowSlots(0))
    Dim names As ZString Ptr Ptr = IIf(nullNames, 0, @nameSlots(0))
    CHECK(invokeLegacy(@sqlite3InitCallback, expectedContext, count, rows, names) = result)
    CHECK(observedCalls = 1)
    For index As Long = 0 To 5
        CHECK(rowSlots(index) = CPtr(ZString Ptr, @rowBytes(index, 0)))
        CHECK(nameSlots(index) = CPtr(ZString Ptr, @nameBytes(index, 0)))
        CHECK(rowBytes(index, 0) = 65 + index AndAlso rowBytes(index, 1) = 0)
        CHECK(nameBytes(index, 0) = 97 + index AndAlso nameBytes(index, 1) = 0)
    Next
End Sub

' end of sqlite-readonly-vectors.bas
