/'
    Project: FreeBASIC SQLite error-path regression
    File: sqlite-vdbe-error-context.bas
    Purpose: Exercise the production error helper with canonical port records.
    Responsibilities: Expose pointer identities, callback state and final flags.
    The runner supplies the exact production body and native callback observers.
'/
#lang "fb"
#include once "sqlite3_vdbe_core.bi"
#include once "sqlite3_api_constants.bi"
#include once "sqlite3_constants.bi"

Declare Sub sqlite3SystemError CDecl Alias "fb_test_sqlite_system_error" _
    (ByVal db As sqlite3 Ptr, ByVal errorCode As SQLITE_C_INT)
Declare Sub fbSqlite3VdbeLogAbort CDecl Alias "fb_test_sqlite_log_abort" _
    (ByVal vm As SqliteVdbe Ptr, ByVal resultCode As SQLITE_C_INT, _
    ByVal operation As SqliteVdbeOp Ptr, ByVal operations As SqliteVdbeOp Ptr)
Declare Function sqlite3VdbeExecFinish CDecl Alias "fb_test_sqlite_finish" _
    (ByVal vm As SqliteVdbe Ptr, ByVal resultCode As SQLITE_C_INT, _
    ByVal errorAction As SQLITE_C_INT) As SQLITE_C_INT
Declare Sub sqlite3VdbeLeave CDecl Alias "fb_test_sqlite_leave" (ByVal vm As SqliteVdbe Ptr)
Declare Sub probePrepare CDecl Alias "fb_test_sqlite_prepare" _
    (ByVal vm As Any Ptr, ByVal db As Any Ptr, ByVal operation As Any Ptr, _
    ByVal operations As Any Ptr, ByVal resultCode As Long, ByVal finishCode As Long, _
    ByVal afterAutoCommit As Long, ByVal errorAction As Long)

#include once "sqlite-vdbe-error-body.bi"

Sub probeSetAutoCommit CDecl Alias "fb_test_sqlite_set_autocommit" (ByVal db As Any Ptr, ByVal value As Long)
    CPtr(sqlite3 Ptr, db)->autoCommit = CUByte(value)
End Sub

Sub probeErrorCase CDecl Alias "fb_test_sqlite_error_case" _
    (ByVal hasVm As Long, ByVal hasDb As Long, ByVal resultCode As Long, _
    ByVal initialAutoCommit As Long, ByVal afterAutoCommit As Long, _
    ByVal errorAction As Long, ByVal finishCode As Long, _
    ByVal initialFlags As ULongInt, ByVal outputs As ULongInt Ptr)
    Dim database As sqlite3
    Dim vm As SqliteVdbe
    Dim operations(0 To 1) As SqliteVdbeOp
    Dim selectedVm As SqliteVdbe Ptr = 0
    database.autoCommit = CUByte(initialAutoCommit)
    database.flags = initialFlags
    vm.errorAction = CUByte(errorAction)
    If hasDb <> 0 Then vm.db = @database
    If hasVm <> 0 Then selectedVm = @vm
    probePrepare(@vm, @database, @operations(1), @operations(0), resultCode, finishCode, afterAutoCommit, errorAction)
    outputs[0] = CULng(sqlite3VdbeExecError(selectedVm, @operations(1), @operations(0), resultCode))
    outputs[1] = database.flags
    outputs[2] = database.autoCommit
    outputs[3] = vm.errorAction
End Sub

/' end of sqlite-vdbe-error-context.bas '/
