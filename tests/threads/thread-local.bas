'' Project: FreeBASIC test suite
'' -----------------------------------------
''
'' File: thread-local.bas
''
'' Purpose:
''
''     Verify that THREAD LOCAL variables keep independent values per thread.
''
'' Responsibilities:
''
''     - check initialized storage in the main thread
''     - write distinct values from worker threads
''     - verify each worker retains its own value after both have written
''
'' This file intentionally does NOT contain:
''
''     - platform-specific TLS ABI expectations
''     - managed types that require per-thread constructors

#include "fbcunit.bi"

#if not defined( __FB_JS__ ) and ((not defined( __FB_DOS__ )) or ( __FB_MT__ <> 0 ))

Thread Local Dim Shared fbcTestsThreadLocalValue As Long = 7
Dim Shared fbcTestsThreadLocalMutex As Any Ptr
Dim Shared fbcTestsThreadLocalReadyCount As Long
Dim Shared fbcTestsThreadLocalStart As Long
Dim Shared fbcTestsThreadLocalResults(0 To 1) As Long
Dim Shared fbcTestsThreadLocalInitialValues(0 To 1) As Long

Private Sub fbcTestsThreadLocalWorker(ByVal context As Any Ptr)
	Dim As Long workerIndex = CInt(context)
	fbcTestsThreadLocalInitialValues(workerIndex) = fbcTestsThreadLocalValue
	fbcTestsThreadLocalValue = workerIndex + 10

	MutexLock(fbcTestsThreadLocalMutex)
	fbcTestsThreadLocalReadyCount += 1
	MutexUnlock(fbcTestsThreadLocalMutex)

	Do
		Dim As Long startWorkers
		MutexLock(fbcTestsThreadLocalMutex)
		startWorkers = fbcTestsThreadLocalStart
		MutexUnlock(fbcTestsThreadLocalMutex)
		If startWorkers <> 0 Then Exit Do
		Sleep 1
	Loop

	fbcTestsThreadLocalResults(workerIndex) = fbcTestsThreadLocalValue
End Sub

SUITE( fbc_tests.threads.thread_local )

	TEST( independent_storage )
		Dim As Any Ptr firstThread, secondThread
		Dim As Long expectedWorkers, readyWorkers
		CU_ASSERT_EQUAL( fbcTestsThreadLocalValue, 7 )

		fbcTestsThreadLocalMutex = MutexCreate()
		CU_ASSERT( fbcTestsThreadLocalMutex <> 0 )
		If fbcTestsThreadLocalMutex = 0 Then Exit Sub

		fbcTestsThreadLocalReadyCount = 0
		fbcTestsThreadLocalStart = 0
		firstThread = ThreadCreate(@fbcTestsThreadLocalWorker, Cast(Any Ptr, 0))
		secondThread = ThreadCreate(@fbcTestsThreadLocalWorker, Cast(Any Ptr, 1))
		expectedWorkers = 0
		If firstThread <> 0 Then expectedWorkers += 1
		If secondThread <> 0 Then expectedWorkers += 1
		CU_ASSERT_EQUAL( expectedWorkers, 2 )

		If expectedWorkers > 0 Then
			Do
				MutexLock(fbcTestsThreadLocalMutex)
				readyWorkers = fbcTestsThreadLocalReadyCount
				MutexUnlock(fbcTestsThreadLocalMutex)
				If readyWorkers >= expectedWorkers Then Exit Do
				Sleep 1
			Loop

			MutexLock(fbcTestsThreadLocalMutex)
			fbcTestsThreadLocalStart = -1
			MutexUnlock(fbcTestsThreadLocalMutex)
		End If

		If firstThread <> 0 Then ThreadWait(firstThread)
		If secondThread <> 0 Then ThreadWait(secondThread)

		If firstThread <> 0 Then CU_ASSERT_EQUAL( fbcTestsThreadLocalResults(0), 10 )
		If secondThread <> 0 Then CU_ASSERT_EQUAL( fbcTestsThreadLocalResults(1), 11 )
		If firstThread <> 0 Then CU_ASSERT_EQUAL( fbcTestsThreadLocalInitialValues(0), 7 )
		If secondThread <> 0 Then CU_ASSERT_EQUAL( fbcTestsThreadLocalInitialValues(1), 7 )
		CU_ASSERT_EQUAL( fbcTestsThreadLocalValue, 7 )

		MutexDestroy(fbcTestsThreadLocalMutex)
		fbcTestsThreadLocalMutex = 0
	END_TEST

END_SUITE

#endif

'' end of thread-local.bas
