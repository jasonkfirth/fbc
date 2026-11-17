/'
    Project FreeBASIC SQLite constraint regression
    File sqlite-vtab-constraints.bas
    Purpose Exercise production WHERE-term lookup and virtual-table consumers.
    Responsibilities Check cache identity, failed lookups and planner cleanup.
    The runner supplies verbatim bodies and the port's canonical records.
'/
#lang "fb"
#include once "sqlite3_db_types.bi"
#include once "sqlite3_parser_types.bi"
#include once "sqlite3_expr_types.bi"
#include once "sqlite3_vdbe_mem_types.bi"
#include once "sqlite3_where_types.bi"
#include once "sqlite3_vtab_types.bi"
#include once "sqlite3_constants.bi"

Declare Sub auditCheck CDecl Alias "fb_test_constraint_check" (ByVal condition As Long, ByVal sourceLine As Long)
Declare Sub auditTrace CDecl Alias "fb_test_constraint_trace" (ByVal operation As Long, ByVal value As ULongInt)
#define CHECK(condition) auditCheck((condition), __LINE__)
Declare Function c2fb_crt_memset CDecl Alias "memset" _
    (ByVal destination As Any Ptr, ByVal value As SQLITE_C_INT, ByVal length As SQLITE_SIZE_T) As Any Ptr

' The hidden header immediately follows IndexInfo. Its final pointer array has
' one declared element and three additional slots in this owned test record.
Type IndexBundle
    info As SqliteIndexInfo
    hidden As SqliteHiddenIndexInfo
    extraSlots(0 To 2) As Any Ptr
End Type

Dim Shared bundle As IndexBundle, database As sqlite3, parser As SqliteParseContext
Dim Shared clauses(0 To 2) As SqliteWhereClause, terms(0 To 11) As SqliteWhereTerm
Dim Shared expressions(0 To 11) As SqliteExpr, rightExpressions(0 To 11) As SqliteExpr
Dim Shared values(0 To 3) As SqliteVdbeMem, constraints(0 To 3) As SqliteIndexConstraint

Private Sub resetRecords(ByVal firstCount As Long, ByVal secondCount As Long, ByVal thirdCount As Long)
    c2fb_crt_memset(@bundle, 0, SizeOf(bundle))
    CHECK(CPtr(UByte Ptr, @bundle.hidden) - CPtr(UByte Ptr, @bundle.info) = SizeOf(SqliteIndexInfo))
    CHECK(CPtr(UByte Ptr, @bundle.extraSlots(0)) - CPtr(UByte Ptr, @bundle.hidden.aRhs(0)) = SizeOf(Any Ptr))
    For index As Long = 0 To 2
        clauses(index).a = @terms(index * 4)
        clauses(index).pOuter = IIf(index < 2, @clauses(index + 1), 0)
    Next
    clauses(0).nTerm = firstCount
    clauses(1).nTerm = secondCount
    clauses(2).nTerm = thirdCount
    For index As Long = 0 To 11
        terms(index).pExpr = @expressions(index)
        expressions(index).pRight = @rightExpressions(index)
        terms(index).prereqRight = CULngInt(1) Shl (1 + (index Mod 4))
    Next
    bundle.info.nConstraint = 4
    bundle.info.aConstraint = @constraints(0)
    bundle.hidden.pWC = @clauses(0)
    bundle.hidden.pParse = @parser
    parser.db = @database
End Sub

Private Function expectedTerm(ByVal offset As Long) As SqliteWhereTerm Ptr
    If offset < 0 Then Return 0
    For clauseIndex As Long = 0 To 2
        If clauses(clauseIndex).nTerm < 0 Then Return 0
        If offset < clauses(clauseIndex).nTerm Then
            If clauses(clauseIndex).a = 0 Then Return 0
            Return @terms(clauseIndex * 4 + offset)
        End If
        offset -= clauses(clauseIndex).nTerm
    Next
    Return 0
End Function

#ifdef SQLITE_CONSTRAINT_WHERE

Dim Shared builder As SqliteWhereLoopBuilder, whereInfo As SqliteWhereInfo
Dim Shared sourceList As SqliteSrcList, newLoop As SqliteWhereLoop
Dim Shared callbackCalls As Long, releaseCalls As Long, valueReleases As Long, stringReleases As Long
Dim Shared allocationMode As Long, selectedFailure As Long, failureCall As Long, useIn As Long, retryLimit As Long
Dim Shared indexText As ZString * 8

Function allocateIndexInfo CDecl Alias "allocateIndexInfo" _
    (ByVal info As SqliteWhereInfo Ptr, ByVal clause As SqliteWhereClause Ptr, _
    ByVal unusable As sqlite3_uint64, ByVal source As SqliteSrcItem Ptr, ByVal noOmit As u16 Ptr) As SqliteIndexInfo Ptr
    CHECK(info = @whereInfo AndAlso clause = @clauses(0) AndAlso unusable = 0)
    CHECK(source = @sourceList.a(0) AndAlso noOmit <> 0)
    auditTrace(1, 0)
    *noOmit = &H1234
    If allocationMode = 1 Then Return 0
    Return @bundle.info
End Function

Function whereLoopResize CDecl Alias "whereLoopResize" _
    (ByVal db As sqlite3 Ptr, ByVal loopRecord As SqliteWhereLoop Ptr, ByVal count As SQLITE_C_INT) As SQLITE_C_INT
    CHECK(db = @database AndAlso loopRecord = @newLoop AndAlso count = bundle.info.nConstraint)
    auditTrace(2, CULngInt(count))
    Return IIf(allocationMode = 2, SQLITE_NOMEM, SQLITE_OK)
End Function

Function whereLoopAddVirtualOne CDecl Alias "whereLoopAddVirtualOne" _
    (ByVal loopBuilder As SqliteWhereLoopBuilder Ptr, ByVal prerequisite As sqlite3_uint64, _
    ByVal usable As sqlite3_uint64, ByVal excluded As u16, ByVal info As SqliteIndexInfo Ptr, _
    ByVal noOmit As u16, ByVal usesIn As SQLITE_C_INT Ptr, ByVal retry As SQLITE_C_INT Ptr) As SQLITE_C_INT
    CHECK(loopBuilder = @builder AndAlso prerequisite = 1 AndAlso info = @bundle.info)
    CHECK(noOmit = &H1234 AndAlso usesIn <> 0 AndAlso (excluded = 0 OrElse excluded = 1))
    callbackCalls += 1
    auditTrace(3 + excluded, usable)
    Dim result As Long = IIf(callbackCalls = failureCall, selectedFailure, SQLITE_OK)
    *usesIn = IIf(result = SQLITE_OK, useIn, 0)
    If retry <> 0 Then *retry = IIf(result = SQLITE_OK, retryLimit, 0)
    newLoop.prereq = IIf(usable = CULngInt(-1), CULngInt(&H1001), usable)
    Return result
End Function

Sub sqlite3ValueFree CDecl Alias "sqlite3ValueFree" (ByVal value As Any Ptr)
    CHECK(value = @values(valueReleases Mod 4))
    auditTrace(5, CULngInt(valueReleases))
    valueReleases += 1
End Sub

Sub sqlite3_free CDecl Alias "sqlite3_free" (ByVal value As Any Ptr)
    CHECK(value = @indexText)
    auditTrace(6, 0)
    stringReleases += 1
End Sub

Sub sqlite3DbFree CDecl Alias "sqlite3DbFree" (ByVal db As sqlite3 Ptr, ByVal value As Any Ptr)
    CHECK(db = @database AndAlso value = @bundle.info)
    auditTrace(7, 0)
    releaseCalls += 1
End Sub

#else

Dim Shared expectedExpression As Any Ptr, expectedSlot As Any Ptr Ptr, selectedValue As Any Ptr
Dim Shared expressionCalls As Long, misuseCalls As Long, expressionResult As Long, produceValue As Long

Function sqlite3MisuseError CDecl Alias "sqlite3MisuseError" (ByVal sourceLine As SQLITE_C_INT) As SQLITE_C_INT
    CHECK(sourceLine = 4606)
    misuseCalls += 1
    auditTrace(8, CULngInt(sourceLine))
    Return SQLITE_MISUSE
End Function

Function sqlite3ValueFromExpr CDecl Alias "sqlite3ValueFromExpr" _
    (ByVal db As Any Ptr, ByVal expression As Any Ptr, ByVal textEncoding As UByte, _
    ByVal affinity As UByte, ByVal pOutputSlot As Any Ptr Ptr) As SQLITE_C_INT
    CHECK(db = @database AndAlso expression = expectedExpression)
    CHECK(textEncoding = database.enc AndAlso affinity = &H41)
    CHECK(pOutputSlot = expectedSlot AndAlso *pOutputSlot = 0)
    expressionCalls += 1
    auditTrace(9, CULngInt(textEncoding))
    If produceValue Then *pOutputSlot = selectedValue
    Return expressionResult
End Function

#endif

#include once "sqlite-vtab-constraints-body.bi"

Sub lookupCase CDecl Alias "fb_test_constraint_lookup" _
    (ByVal firstCount As Long, ByVal secondCount As Long, ByVal thirdCount As Long, _
    ByVal offset As Long, ByVal fault As Long)
    resetRecords(firstCount, secondCount, thirdCount)
    If fault = 1 Then clauses(0).nTerm = -1
    If fault = 2 Then clauses(0).a = 0
    Dim clause As SqliteWhereClause Ptr = IIf(fault = 3, 0, @clauses(0))
    Dim expected As SqliteWhereTerm Ptr = IIf(fault = 3, 0, expectedTerm(offset))
    CHECK(termFromWhereClause(clause, offset) = expected)
End Sub

#ifdef SQLITE_CONSTRAINT_WHERE

Sub plannerCase CDecl Alias "fb_test_constraint_planner" _
    (ByVal count As Long, ByVal offset As Long, ByVal fault As Long, ByVal allocation As Long, _
    ByVal failedCall As Long, ByVal result As Long, ByVal usesIn As Long, ByVal retry As Long, ByVal freeText As Long)
    resetRecords(2, 2, 2)
    allocationMode = allocation
    selectedFailure = result
    failureCall = failedCall
    useIn = usesIn
    retryLimit = retry
    callbackCalls = 0
    releaseCalls = 0
    valueReleases = 0
    stringReleases = 0
    bundle.info.nConstraint = count
    bundle.info.idxStr = @indexText
    bundle.info.needToFreeIdxStr = freeText
    Dim rhsSlots As Any Ptr Ptr = @bundle.hidden.aRhs(0)
    For index As Long = 0 To count - 1
        constraints(index).iTermOffset = (offset + index) Mod 6
        rhsSlots[index] = @values(index)
    Next
    If fault = 1 Then constraints(count - 1).iTermOffset = 6
    If fault = 2 Then constraints(count - 1).iTermOffset = -1
    If fault = 3 Then
        clauses(0).a = 0
        constraints(0).iTermOffset = 0
    End If
    If fault = 4 Then clauses(0).nTerm = -1
    builder.pWInfo = @whereInfo
    builder.pWC = @clauses(0)
    builder.pNew = @newLoop
    whereInfo.pParse = @parser
    whereInfo.pTabList = @sourceList
    newLoop.iTab = 0
    newLoop.prereq = 0
    Dim actual As Long = whereLoopAddVirtual(@builder, 1, 0)
    CHECK(actual = SQLITE_OK OrElse actual = selectedFailure OrElse actual = SQLITE_CORRUPT OrElse actual = SQLITE_NOMEM)
    If allocationMode = 1 Then
        CHECK(actual = SQLITE_NOMEM AndAlso releaseCalls = 0 AndAlso callbackCalls = 0)
    Else
        CHECK(releaseCalls = 1 AndAlso valueReleases = count AndAlso stringReleases = freeText)
        CHECK(bundle.info.needToFreeIdxStr = 0 AndAlso (freeText = 0 OrElse bundle.info.idxStr = 0))
        For index As Long = 0 To count - 1
            CHECK(rhsSlots[index] = 0)
        Next
        If allocationMode = 2 Then
            CHECK(actual = SQLITE_NOMEM AndAlso callbackCalls = 0)
        Else
            CHECK(newLoop.rSetup = 0 AndAlso newLoop.wsFlags = &H400 AndAlso newLoop.nLTerm = 0)
            If failedCall = 0 OrElse selectedFailure = SQLITE_OK Then
                CHECK(actual = IIf(fault = 0, SQLITE_OK, SQLITE_CORRUPT))
            End If
        End If
    End If
    auditTrace(10, CULngInt(actual))
End Sub

#else

Sub rhsCase CDecl Alias "fb_test_constraint_rhs" _
    (ByVal firstCount As Long, ByVal secondCount As Long, ByVal thirdCount As Long, _
    ByVal offset As Long, ByVal constraintIndex As Long, ByVal cached As Long, _
    ByVal result As Long, ByVal writeValue As Long, ByVal textEncoding As Long, ByVal fault As Long)
    resetRecords(firstCount, secondCount, thirdCount)
    database.enc = CUByte(textEncoding)
    constraints(constraintIndex).iTermOffset = offset
    Dim term As SqliteWhereTerm Ptr = expectedTerm(offset)
    CHECK(term <> 0)
    Dim expression As SqliteExpr Ptr = CPtr(SqliteExpr Ptr, term->pExpr)
    If fault = 1 Then constraints(constraintIndex).iTermOffset = firstCount + secondCount + thirdCount
    If fault = 2 Then term->pExpr = 0
    If fault = 3 Then bundle.hidden.pParse = 0
    If fault = 4 Then parser.db = 0
    If fault = 5 Then bundle.info.aConstraint = 0
    If fault = 6 Then bundle.hidden.pWC = 0
    If fault = 7 Then constraints(constraintIndex).iTermOffset = -1
    If fault = 8 Then expression->pRight = 0
    Dim rhsSlots As Any Ptr Ptr = @bundle.hidden.aRhs(0)
    rhsSlots[constraintIndex] = IIf(cached, @values(constraintIndex), 0)
    expectedExpression = expression->pRight
    expectedSlot = @rhsSlots[constraintIndex]
    selectedValue = @values(constraintIndex)
    expressionResult = result
    produceValue = writeValue
    expressionCalls = 0
    misuseCalls = 0
    Dim outputValue As SqliteVdbeMem Ptr = @values((constraintIndex + 1) Mod 4)
    Dim info As SqliteIndexInfo Ptr = IIf(fault = 9, 0, @bundle.info)
    Dim outputAddress As SqliteVdbeMem Ptr Ptr = IIf(fault = 10, 0, @outputValue)
    Dim indexArgument As Long = IIf(fault = 11, -1, IIf(fault = 12, 4, constraintIndex))
    Dim actual As Long = sqlite3_vtab_rhs_value(info, indexArgument, outputAddress)
    If fault >= 9 Then
        CHECK(actual = SQLITE_MISUSE AndAlso misuseCalls = 1 AndAlso expressionCalls = 0)
        If fault <> 10 Then CHECK(outputValue = 0)
    ElseIf cached Then
        CHECK(actual = SQLITE_OK AndAlso outputValue = selectedValue AndAlso expressionCalls = 0)
    ElseIf fault <> 0 AndAlso fault <> 8 Then
        CHECK(actual = SQLITE_CORRUPT AndAlso outputValue = 0 AndAlso expressionCalls = 0)
    Else
        CHECK(expressionCalls = 1 AndAlso misuseCalls = 0)
        CHECK(outputValue = IIf(writeValue, selectedValue, 0))
        CHECK(actual = IIf(result = SQLITE_OK AndAlso writeValue = 0, SQLITE_NOTFOUND, result))
    End If
    auditTrace(10, CULngInt(actual))
End Sub

#endif

' end of sqlite-vtab-constraints.bas
