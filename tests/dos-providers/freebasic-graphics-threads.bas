/' FreeBASIC DOS provider tests: freebasic-graphics-threads.bas
 * Exercise graphics updates while a floating-point worker is preempted.
 * No sockets, game code or external graphics libraries are involved.
 '/
#lang "fb"
#include once "fbgfx.bi"

Dim Shared stateLock As Any Ptr
Dim Shared stopped As Integer
Dim Shared progress As Integer
Dim Shared failed As Integer

Private Sub worker(ByVal argument As Any Ptr)
    Do
        Dim total As Double
        For i As Integer = 1 To 5000
            total += Sqr(i)
        Next
        MutexLock(stateLock)
        ' Sum(Sqr(1)..Sqr(5000)) is about 235737.408. Check the result so the
        ' floating-point work stays observable in optimized builds too.
        If Not (total > 235737.0 And total < 235738.0) Then failed = 1
        Dim finished As Integer = stopped
        progress += 1
        MutexUnlock(stateLock)
        If finished Then Exit Do
    Loop
End Sub

If ScreenRes(640, 480, 15, 2) <> 0 Then End 1
ScreenSet 1, 0
stateLock = MutexCreate()
If stateLock = 0 Then End 2
Dim threadHandle As Any Ptr = ThreadCreate(@worker)
If threadHandle = 0 Then End 3
For frame As Integer = 0 To 199
    Line (0, 0)-(639, 479), Rgb(frame, 64, 32), BF
    Draw String (16, 16), "DOS graphics and threads " & Str(frame)
    ScreenCopy 1, 0
    Sleep 1, 1
Next
MutexLock(stateLock)
stopped = 1
MutexUnlock(stateLock)
ThreadWait(threadHandle)
MutexDestroy(stateLock)
Screen 0
If progress = 0 Or failed Then End 4
Print "PASS graphics and preemptive worker"
End 0

' end of freebasic-graphics-threads.bas
