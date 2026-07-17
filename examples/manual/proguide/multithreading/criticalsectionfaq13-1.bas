'' examples/manual/proguide/multithreading/criticalsectionfaq13-1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Critical Sections FAQ'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgMtCriticalSectionsFAQ
'' --------

'' This standalone benchmark harness deliberately shares its synchronization
'' handles and predicates with the three callbacks defined below.
'' FB-LINTER: DISABLE-NEXT-LINE FBL301
Dim Shared As Any Ptr mutex1, mutex2, mutex, cond1, cond2, pt
'' FB-LINTER: DISABLE-NEXT-LINE FBL301
Dim Shared As Integer flag1, flag2
Dim As Double t

Const FLAG_ITERATIONS = 100
Const SYNC_ITERATIONS = 100000
Const SECONDS_PER_DAY = 86400.0
Const MILLISECONDS_PER_SECOND = 1000.0
Const MICROSECONDS_PER_SECOND = 1000000.0

Function ElapsedSeconds(ByVal StartedAt As Double) As Double
	Dim As Double FinishedAt = Timer

	If FinishedAt < StartedAt Then
		FinishedAt += SECONDS_PER_DAY
	End If

	Return FinishedAt - StartedAt
End Function

'----------------------------------------------------------------------------------

#if defined(__FB_WIN32__)
	Declare Function _setTimer StdCall Lib "winmm" Alias "timeBeginPeriod" _
	    (ByVal PeriodMilliseconds As UInteger = 1) As UInteger
	Declare Function _resetTimer StdCall Lib "winmm" Alias "timeEndPeriod" _
	    (ByVal PeriodMilliseconds As UInteger = 1) As UInteger
#endif

Sub ThreadFlag(ByVal p As Any Ptr)
	For I As Integer = 1 To FLAG_ITERATIONS
		While flag1 = 0
			Sleep 1, 1
		Wend
		flag1 = 0
		' only child thread code runs (location for example)
		flag2 = 1
	Next I
End Sub

pt = ThreadCreate(@ThreadFlag)
Print "Thread synchronization latency by simple flags:"
#if defined(__FB_WIN32__)
	_setTimer()
	Print "(in high resolution OS cycle period)"
#else
	Print "(in normal resolution OS cycle period)"
#endif
t = Timer
For I As Integer = 1 To FLAG_ITERATIONS
	flag1 = 1
	While flag2 = 0
		Sleep 1, 1
	Wend
	flag2 = 0
	' only main thread code runs (location for example)
Next I
t = ElapsedSeconds(t)
#if defined(__FB_WIN32__)
	_resetTimer()
#endif
ThreadWait(pt)
Print Using "####.## milliseconds per double synchronization (round trip)"; _
            t * (MILLISECONDS_PER_SECOND / FLAG_ITERATIONS)
Print

'----------------------------------------------------------------------------------

#if defined(__FB_WIN32__)
	'' The Win32 rtlib implements FreeBASIC mutexes with semaphores, so this
	'' historical cross-thread handoff benchmark is valid only on that target.
	'' pthread mutexes used by Unix and NuttX must be unlocked by their owner.
Sub ThreadMutex(ByVal p As Any Ptr)
	For I As Integer = 1 To SYNC_ITERATIONS
		'' This Windows-only benchmark deliberately transfers semaphore ownership.
		'' FB-LINTER: DISABLE-NEXT-LINE FBL712
		MutexLock(mutex1)    '' wait for mutex unlock from main thread
		' only child thread code runs
		MutexUnlock(mutex2)  '' unlock mutex for main thread
	Next I
End Sub

mutex1 = MutexCreate()
mutex2 = MutexCreate()

'' This target-specific benchmark deliberately transfers semaphore ownership.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-PAIR-001
MutexLock(mutex1)
MutexLock(mutex2)

pt = ThreadCreate(@ThreadMutex)
Print "Thread synchronization latency by mutual exclusions:"
t = Timer
For I As Integer = 1 To SYNC_ITERATIONS
	MutexUnlock(mutex1)  '' mutex unlock for child thread
	MutexLock(mutex2)    '' wait for mutex unlock from child thread
	' only main thread code runs
Next I
t = ElapsedSeconds(t)
'' The worker participates in the target-specific semaphore handoff above.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-PAIR-002
ThreadWait(pt)
Print Using "####.## microseconds per double synchronization (round trip)"; _
            t * (MICROSECONDS_PER_SECOND / SYNC_ITERATIONS)
Print

MutexDestroy(mutex1)
MutexDestroy(mutex2)
#else
	Print "Mutex-only cross-thread handoff benchmark skipped on this target."
	Print
#endif

'----------------------------------------------------------------------------------

Sub ThreadCondVar(ByVal p As Any Ptr)
	For I As Integer = 1 To SYNC_ITERATIONS
		MutexLock(mutex)
		While flag1 = 0
			CondWait(cond1, mutex)  '' wait for conditional signal from main thread
		Wend
		flag1 = 0
		' only child thread code runs (location for example)
		flag2 = 1
		CondSignal(cond2)  '' send conditional signal to main thread
		MutexUnlock(mutex)
	Next I
End Sub

mutex = MutexCreate()
cond1 = CondCreate()
cond2 = CondCreate()

pt = ThreadCreate(@ThreadCondVar)
Print "Thread synchronization latency by conditional variables:"
t = Timer
For I As Integer = 1 To SYNC_ITERATIONS
	MutexLock(mutex)
	flag1 = 1
	CondSignal(cond1)  '' send conditional signal to main thread
	While flag2 = 0
		CondWait(Cond2, mutex)  '' wait for conditional signal from child thread
	Wend
	flag2 = 0
	' only child thread code runs (location for example)
	MutexUnlock(mutex)
Next I
t = ElapsedSeconds(t)
ThreadWait(pt)
Print Using "####.## microseconds per double synchronization (round trip)"; _
            t * (MICROSECONDS_PER_SECOND / SYNC_ITERATIONS)
Print

MutexDestroy(mutex)
CondDestroy(cond1)
CondDestroy(cond2)

'----------------------------------------------------------------------------------
