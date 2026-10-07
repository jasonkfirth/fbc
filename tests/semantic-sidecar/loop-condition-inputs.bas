' Project: FreeBASIC compiler semantic observations
' File: loop-condition-inputs.bas
' Purpose: Keep accepted loop predicates before folding and branch lowering.
' Responsibilities: Pre/post tests, bare grammar, macros and continued inputs.
' This file intentionally does NOT execute these loops.
#lang "fb"
#define LOOP_LIMIT(value) ((value) < 2.5)

Sub ObserveLoopConditions()
    Dim As Double value
    Dim As Boolean ready = True
    While LOOP_LIMIT(value)
        Exit While
    Wend
    While _
        ready
        Exit While
    Wend
    Do While value > 1.0
        Exit Do
    Loop
    Do Until ready
        Exit Do
    Loop
    Do
        Exit Do
    Loop While value <> 3.0
    Do
        Exit Do
    Loop Until ready
    Do
        Exit Do
    Loop
    While 1 < 2
        Exit While
    Wend
End Sub
' end of loop-condition-inputs.bas
