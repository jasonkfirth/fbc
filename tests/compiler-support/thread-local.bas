'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: thread-local.bas
''
'' Purpose:
''
''     Verify that Thread Local variables keep independent storage per thread.
''
'' Responsibilities:
''
''     - write one value from each worker thread
''     - synchronize before reading the per-thread values back
''     - verify the main thread retains its own initial value
''
'' This file intentionally does NOT contain:
''
''     - native assembly backend expectations
''     - C or POSIX thread APIs
''

#lang "fb"

Thread Local Dim Shared threadLocalValue As Long = 7
Dim Shared threadLocalMutex As Any Ptr
Dim Shared threadLocalReadyCount As Long
Dim Shared threadLocalResults(0 To 1) As Long
Dim Shared threadLocalInitialValues(0 To 1) As Long

Private Sub ThreadLocalWorker(ByVal context As Any Ptr)
	Dim As Long workerIndex = CInt(context)
	threadLocalInitialValues(workerIndex) = threadLocalValue
	threadLocalValue = workerIndex + 10

	MutexLock(threadLocalMutex)
	threadLocalReadyCount += 1
	MutexUnlock(threadLocalMutex)

	Do
		Dim As Long readyCount
		MutexLock(threadLocalMutex)
		readyCount = threadLocalReadyCount
		MutexUnlock(threadLocalMutex)
		If readyCount >= 2 Then Exit Do
		Sleep 1
	Loop

	threadLocalResults(workerIndex) = threadLocalValue
End Sub

If threadLocalValue <> 7 Then End 6
threadLocalMutex = MutexCreate()
If threadLocalMutex = 0 Then End 1

Dim As Any Ptr firstThread = ThreadCreate(@ThreadLocalWorker, Cast(Any Ptr, 0))
Dim As Any Ptr secondThread = ThreadCreate(@ThreadLocalWorker, Cast(Any Ptr, 1))
If firstThread = 0 OrElse secondThread = 0 Then End 2
ThreadWait(firstThread)
ThreadWait(secondThread)

If threadLocalResults(0) <> 10 Then End 3
If threadLocalResults(1) <> 11 Then End 4
If threadLocalValue <> 7 Then End 5
If threadLocalInitialValues(0) <> 7 Then End 7
If threadLocalInitialValues(1) <> 7 Then End 8

MutexDestroy(threadLocalMutex)
Print "thread-local storage passed"

'' end of thread-local.bas
