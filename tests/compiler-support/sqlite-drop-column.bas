/'
    Project: FreeBASIC SQLite ALTER regression
    File: sqlite-drop-column.bas
    Purpose: Exercise the production DROP COLUMN helper and token lookup.
    Responsibilities: Supply canonical records and check parse cleanup ownership.
    The runner supplies verbatim helpers and the complete production tokenizer.
'/
#lang "fb"
#include once "sqlite3_db_types.bi"
#include once "sqlite3_parser_types.bi"
#include once "sqlite3_schema_types.bi"
#include once "sqlite3_result.bi"

Type c2fb_callback_3 As Function CDecl(ByVal argument As Any Ptr, ByVal action As SQLITE_C_INT, _
    ByVal first As Const ZString Ptr, ByVal second As Const ZString Ptr, _
    ByVal third As Const ZString Ptr, ByVal fourth As Const ZString Ptr) As SQLITE_C_INT
Type c2fb_callback_8 As Sub CDecl(ByVal argument As Any Ptr)
Declare Sub auditCheck CDecl Alias "fb_test_drop_check" (ByVal condition As Long, ByVal sourceLine As Long)
#define CHECK(condition) auditCheck((condition), __LINE__)
Declare Sub observerPrepare CDecl Alias "fb_test_drop_prepare" (ByVal db As Any Ptr, ByVal context As Any Ptr)
Declare Sub observerCleanup CDecl Alias "fb_test_drop_cleanup" ()
Declare Function sqlite3_context_db_handle CDecl Alias "sqlite3_context_db_handle" (ByVal context As Any Ptr) As Any Ptr
Declare Function sqlite3_value_int CDecl Alias "sqlite3_value_int" (ByVal value As Any Ptr) As SQLITE_C_INT
Declare Function sqlite3_value_text CDecl Alias "sqlite3_value_text" (ByVal value As Any Ptr) As Const UByte Ptr
Declare Function renameParseSql CDecl Alias "renameParseSql" _
    (ByVal parsed As SqliteParseContext Ptr, ByVal schemaName As Const ZString Ptr, _
    ByVal db As sqlite3 Ptr, ByVal sql As Const ZString Ptr, ByVal temporarySchema As SQLITE_C_INT) As SQLITE_C_INT
Declare Sub renameParseCleanup CDecl Alias "renameParseCleanup" (ByVal parsed As SqliteParseContext Ptr)
Declare Function renameTokenFind CDecl Alias "renameTokenFind" _
    (ByVal parsed As SqliteParseContext Ptr, ByVal renameContext As Any Ptr, ByVal identity As Const Any Ptr) As SqliteRenameToken Ptr
Declare Function sqlite3GetToken CDecl Alias "sqlite3GetToken" _
    (ByVal text As Const UByte Ptr, ByVal tokenType As SQLITE_C_INT Ptr) As LongInt
Declare Function getConstraintToken CDecl Alias "getConstraintToken" _
    (ByVal text As Const UByte Ptr, ByVal tokenType As SQLITE_C_INT Ptr) As SQLITE_C_INT
Declare Function sqlite3MPrintf CDecl Alias "sqlite3MPrintf" (ByVal db As Any Ptr, ByVal format As Const ZString Ptr, ...) As ZString Ptr
Declare Function sqlite3CorruptError CDecl Alias "sqlite3CorruptError" (ByVal sourceLine As SQLITE_C_INT) As SQLITE_C_INT
Declare Sub sqlite3_result_text CDecl Alias "sqlite3_result_text" _
    (ByVal context As Any Ptr, ByVal text As Const ZString Ptr, ByVal length As SQLITE_C_INT, ByVal destructor As Any Ptr)
Declare Sub sqlite3_result_error_code CDecl Alias "sqlite3_result_error_code" (ByVal context As Any Ptr, ByVal result As SQLITE_C_INT)
Declare Sub sqlite3_free CDecl Alias "sqlite3_free" (ByVal allocation As Any Ptr)

' The observer runs serially. Every borrowed address remains live until the
' helper restores its authorizer and the final result callback has returned.
Dim Shared database As sqlite3, resultContext As SqliteResultContext, sourceTable As SqliteTable
Dim Shared columns(0 To 2) As SqliteColumn, tokens(0 To 2) As SqliteRenameToken
Dim Shared values(0 To 2) As SqliteVdbeMem, arguments(0 To 2) As SqliteVdbeMem Ptr
Dim Shared selectedSql As Const ZString Ptr, selectedSchema As Long, selectedColumn As Long
Dim Shared parseResult As Long, hasTable As Long, parseCalls As Long, cleanupCalls As Long
Dim Shared activeParse As SqliteParseContext Ptr, savedAuthorizer As SqliteAuthorizerCallback
Dim Shared tokenListHead As SqliteRenameToken Ptr

Function sqlite3_context_db_handle CDecl Alias "sqlite3_context_db_handle" (ByVal context As Any Ptr) As Any Ptr
    CHECK(context = @resultContext)
    Return @database
End Function

Function sqlite3_value_int CDecl Alias "sqlite3_value_int" (ByVal value As Any Ptr) As SQLITE_C_INT
    If value = @values(0) Then Return selectedSchema
    CHECK(value = @values(2))
    Return selectedColumn
End Function

Function sqlite3_value_text CDecl Alias "sqlite3_value_text" (ByVal value As Any Ptr) As Const UByte Ptr
    CHECK(value = @values(1))
    Return CPtr(Const UByte Ptr, selectedSql)
End Function

Function renameParseSql CDecl Alias "renameParseSql" _
    (ByVal parsed As SqliteParseContext Ptr, ByVal schemaName As Const ZString Ptr, _
    ByVal db As sqlite3 Ptr, ByVal sql As Const ZString Ptr, ByVal temporarySchema As SQLITE_C_INT) As SQLITE_C_INT
    CHECK(parsed <> 0 AndAlso db = @database AndAlso sql = selectedSql)
    CHECK(schemaName = database.aDb(selectedSchema).zDbSName)
    CHECK(temporarySchema = IIf(selectedSchema = 1, 1, 0) AndAlso database.xAuth = 0)
    parseCalls += 1
    activeParse = parsed
    If hasTable Then parsed->pNewTable = @sourceTable
    parsed->pRename = tokenListHead
    Return parseResult
End Function

Sub renameParseCleanup CDecl Alias "renameParseCleanup" (ByVal parsed As SqliteParseContext Ptr)
    CHECK(parsed = activeParse AndAlso database.xAuth = 0)
    cleanupCalls += 1
    observerCleanup()
    parsed->pNewTable = 0
    parsed->pRename = 0
End Sub

Function authorizerState CDecl Alias "fb_test_drop_authorizer_restored" () As Long
    Return database.xAuth = savedAuthorizer
End Function

Private Function dummyAuthorizer CDecl(ByVal argument As Any Ptr, ByVal action As SQLITE_C_INT, _
    ByVal first As Const ZString Ptr, ByVal second As Const ZString Ptr, _
    ByVal third As Const ZString Ptr, ByVal fourth As Const ZString Ptr) As SQLITE_C_INT
    CHECK(0)
    Return 1
End Function

#include once "sqlite-drop-column-body.bi"

Sub dropCase CDecl Alias "fb_test_drop_case" _
    (ByVal sql As Const ZString Ptr, ByVal offsets As Long Ptr, ByVal closingOffset As Long, _
    ByVal columnIndex As Long, ByVal count As Long, ByVal mask As Long, ByVal reverseList As Long, _
    ByVal selectedParseResult As Long, ByVal tableExists As Long, ByVal withAuthorizer As Long, ByVal schemaIndex As Long)
    selectedSql = sql
    selectedColumn = columnIndex
    selectedSchema = schemaIndex
    parseResult = selectedParseResult
    hasTable = tableExists
    parseCalls = 0
    cleanupCalls = 0
    activeParse = 0
    savedAuthorizer = 0
    If withAuthorizer Then savedAuthorizer = @dummyAuthorizer
    database.xAuth = savedAuthorizer
    database.aDb(0).zDbSName = StrPtr(!"main")
    database.aDb(1).zDbSName = StrPtr(!"temp")
    sourceTable.nCol = CShort(count)
    sourceTable.aCol = @columns(0)
    sourceTable.u.tab.addColOffset = closingOffset
    tokenListHead = 0
    For position As Long = 0 To 2
        Dim index As Long = IIf(reverseList, position, 2 - position)
        columns(index).zCnName = StrPtr(!"column") + index
        tokens(index).p = columns(index).zCnName
        tokens(index).t.z = CPtr(UByte Ptr, CPtr(Const UByte Ptr, sql) + offsets[index])
        tokens(index).pNext = 0
        If (mask And (1 Shl index)) <> 0 Then
            tokens(index).pNext = tokenListHead
            tokenListHead = @tokens(index)
        End If
        arguments(index) = @values(index)
    Next
    observerPrepare(@database, @resultContext)
    dropColumnFunc(@resultContext, 3, @arguments(0))
    CHECK(parseCalls = 1 AndAlso cleanupCalls = 1)
    CHECK(database.xAuth = savedAuthorizer)
End Sub

/' end of sqlite-drop-column.bas '/
