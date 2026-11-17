/'
    Project: FreeBASIC SQLite virtual-table regression
    File: sqlite-vtab-transaction.bas
    Purpose: Exercise the production create and transaction helpers.
    Responsibilities: Check instance identity, transaction growth and references.
    The runner supplies verbatim production bodies and canonical port headers.
'/
#lang "fb"
#include once "sqlite3_db_types.bi"
#include once "sqlite3_hash_types.bi"
#include once "sqlite3_schema_types.bi"
#include once "sqlite3_vtab_types.bi"
#include once "sqlite3_constants.bi"

Type c2fb_callback_27 As Function CDecl(ByVal db As Any Ptr, ByVal auxiliary As Any Ptr, _
    ByVal argumentCount As SQLITE_C_INT, ByVal arguments As Const ZString Ptr Ptr, _
    ByVal vtab As SqliteVtab Ptr Ptr, ByVal message As ZString Ptr Ptr) As SQLITE_C_INT
Type c2fb_callback_58 As Function CDecl(ByVal vtab As SqliteVtab Ptr) As SQLITE_C_INT

Declare Sub auditCheck CDecl Alias "fb_test_vtab_check" (ByVal condition As Long, ByVal sourceLine As Long)
#define CHECK(condition) auditCheck((condition), __LINE__)
Declare Function sqlite3FindTable CDecl Alias "sqlite3FindTable" _
    (ByVal db As Any Ptr, ByVal tableName As Const ZString Ptr, ByVal databaseName As Const ZString Ptr) As Any Ptr
Declare Function sqlite3MPrintf CDecl Alias "sqlite3MPrintf" _
    (ByVal db As Any Ptr, ByVal format As Const ZString Ptr, ...) As ZString Ptr
Declare Function vtabCallConstructor CDecl Alias "vtabCallConstructor" _
    (ByVal db As sqlite3 Ptr, ByVal table As SqliteTable Ptr, ByVal module As SqliteModule Ptr, _
    ByVal constructCallback As c2fb_callback_27, ByVal message As ZString Ptr Ptr) As SQLITE_C_INT
Declare Function sqlite3VtabCallCreate CDecl Alias "sqlite3VtabCallCreate" _
    (ByVal db As sqlite3 Ptr, ByVal databaseIndex As SQLITE_C_INT, ByVal tableName As Const ZString Ptr, _
    ByVal message As ZString Ptr Ptr) As SQLITE_C_INT
Declare Function sqlite3GetVTable CDecl Alias "sqlite3GetVTable" _
    (ByVal db As sqlite3 Ptr, ByVal table As SqliteTable Ptr) As SqliteVTable Ptr
Declare Sub sqlite3VtabLock CDecl Alias "sqlite3VtabLock" (ByVal table As SqliteVTable Ptr)
Declare Function growVTrans CDecl Alias "growVTrans" (ByVal db As sqlite3 Ptr) As SQLITE_C_INT
Declare Sub addToVTrans CDecl Alias "addToVTrans" (ByVal db As sqlite3 Ptr, ByVal table As SqliteVTable Ptr)
Declare Function c2fb_crt_memset CDecl Alias "memset" _
    (ByVal destination As Any Ptr, ByVal value As SQLITE_C_INT, ByVal length As SQLITE_SIZE_T) As Any Ptr

' One serial case owns these borrowed addresses. The allocator models a moved
' transaction array while keeping every VTable and its connection alive.
Dim Shared database As sqlite3, otherDatabase As sqlite3, sourceTable As SqliteTable
Dim Shared moduleRecord As SqliteModule, callbacks As SqliteModuleCallbacks
Dim Shared instances(0 To 2) As SqliteVTable
Dim Shared originalSlots(0 To 19) As Any Ptr, movedSlots(0 To 19) As Any Ptr
Dim Shared moduleArguments(0 To 0) As ZString Ptr
Dim Shared moduleMode As Long, constructorResult As Long, failAllocation As Long, originalCount As Long
Dim Shared constructorCalls As Long, allocationCalls As Long, lookupCalls As Long

Function sqlite3FindTable CDecl Alias "sqlite3FindTable" _
    (ByVal db As Any Ptr, ByVal tableName As Const ZString Ptr, ByVal databaseName As Const ZString Ptr) As Any Ptr
    CHECK(db = @database AndAlso tableName = StrPtr(!"table") AndAlso databaseName = StrPtr(!"main"))
    Return @sourceTable
End Function

Function sqlite3HashFind CDecl Alias "sqlite3HashFind" _
    (ByVal hashTable As Const Hash Ptr, ByVal key As Const ZString Ptr) As Any Ptr
    CHECK(hashTable = @database.aModule AndAlso key = moduleArguments(0))
    lookupCalls += 1
    If moduleMode = 1 Then Return 0
    Return @moduleRecord
End Function

Function vtabCallConstructor CDecl Alias "vtabCallConstructor" _
    (ByVal db As sqlite3 Ptr, ByVal table As SqliteTable Ptr, ByVal module As SqliteModule Ptr, _
    ByVal constructCallback As c2fb_callback_27, ByVal message As ZString Ptr Ptr) As SQLITE_C_INT
    CHECK(db = @database AndAlso table = @sourceTable AndAlso module = @moduleRecord)
    CHECK(CPtr(Any Ptr, constructCallback) = callbacks.xCreate AndAlso message <> 0)
    constructorCalls += 1
    Return constructorResult
End Function

Function sqlite3DbRealloc CDecl Alias "sqlite3DbRealloc" _
    (ByVal db As sqlite3 Ptr, ByVal allocation As Any Ptr, ByVal length As sqlite3_uint64) As Any Ptr
    CHECK(db = @database AndAlso allocation = @originalSlots(0))
    CHECK(length = CULngInt(originalCount + 5) * SizeOf(Any Ptr))
    allocationCalls += 1
    If failAllocation Then Return 0
    For slot As Long = 0 To originalCount - 1
        movedSlots(slot) = originalSlots(slot)
    Next
    Return @movedSlots(0)
End Function

#include once "sqlite-vtab-transaction-body.bi"

Private Function dummyCreate CDecl(ByVal db As Any Ptr, ByVal auxiliary As Any Ptr, _
    ByVal argumentCount As SQLITE_C_INT, ByVal arguments As Const ZString Ptr Ptr, _
    ByVal vtab As SqliteVtab Ptr Ptr, ByVal message As ZString Ptr Ptr) As SQLITE_C_INT
    CHECK(0)
    Return 1
End Function

Private Function dummyDestroy CDecl(ByVal vtab As SqliteVtab Ptr) As SQLITE_C_INT
    CHECK(0)
    Return 1
End Function

Sub transactionCase CDecl Alias "fb_test_vtab_case" _
    (ByVal selectedModule As Long, ByVal selectedResult As Long, ByVal selectedInstance As Long, _
    ByVal selectedFailure As Long, ByVal count As Long, ByVal referenceCount As Long)
    moduleMode = selectedModule
    constructorResult = selectedResult
    failAllocation = selectedFailure
    originalCount = count
    constructorCalls = 0
    allocationCalls = 0
    lookupCalls = 0
    database.nVTrans = count
    database.aVTrans = @originalSlots(0)
    database.aDb(0).zDbSName = StrPtr(!"main")
    moduleArguments(0) = StrPtr(!"module")
    sourceTable.u.vtab.azArg = @moduleArguments(0)
    sourceTable.u.vtab.p = @instances(0)
    moduleRecord.pModule = @callbacks
    callbacks.xCreate = @dummyCreate
    callbacks.xDestroy = @dummyDestroy
    If moduleMode = 2 Then callbacks.xCreate = 0
    If moduleMode = 3 Then callbacks.xDestroy = 0
    For index As Long = 0 To 2
        instances(index).db = @otherDatabase
        instances(index).nRef = referenceCount
        instances(index).pNext = 0
        If index < 2 Then instances(index).pNext = @instances(index + 1)
    Next
    If selectedInstance < 3 Then instances(selectedInstance).db = @database
    For index As Long = 0 To 19
        originalSlots(index) = @sourceTable
        movedSlots(index) = @moduleRecord
    Next
    Dim message As ZString Ptr = 0
    Dim result As SQLITE_C_INT = sqlite3VtabCallCreate(@database, 0, StrPtr(!"table"), @message)
    Dim expectedResult As Long = 1
    If moduleMode = 0 Then expectedResult = constructorResult
    Dim shouldGrow As Long = (moduleMode = 0 AndAlso constructorResult = 0 AndAlso selectedInstance < 3)
    Dim shouldAllocate As Long = (shouldGrow AndAlso count Mod 5 = 0)
    If shouldAllocate AndAlso failAllocation Then expectedResult = 7
    Dim shouldAppend As Long = (shouldGrow AndAlso expectedResult = 0)
    CHECK(result = expectedResult AndAlso lookupCalls = 1)
    CHECK(constructorCalls = IIf(moduleMode = 0, 1, 0))
    CHECK(allocationCalls = IIf(shouldAllocate, 1, 0))
    CHECK((message <> 0) = (moduleMode <> 0))
    CHECK(database.nVTrans = count + IIf(shouldAppend, 1, 0))
    Dim slots As Any Ptr Ptr = @originalSlots(0)
    If shouldAppend AndAlso shouldAllocate Then slots = @movedSlots(0)
    CHECK(database.aVTrans = slots)
    For index As Long = 0 To 2
        Dim expectedReferences As Long = referenceCount
        If shouldAppend AndAlso index = selectedInstance Then expectedReferences += 1
        CHECK(instances(index).nRef = expectedReferences)
    Next
    For index As Long = 0 To 19
        Dim expectedPointer As Any Ptr = @sourceTable
        If shouldAppend AndAlso index = count Then
            expectedPointer = @instances(selectedInstance)
        ElseIf shouldAppend AndAlso shouldAllocate AndAlso index >= count + 1 Then
            expectedPointer = IIf(index < count + 5, 0, @moduleRecord)
        End If
        CHECK(slots[index] = expectedPointer)
    Next
End Sub

/' end of sqlite-vtab-transaction.bas '/
