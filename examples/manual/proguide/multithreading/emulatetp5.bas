'' examples/manual/proguide/multithreading/emulatetp5.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Emulate a TLS (Thread Local Storage) and a TP (Thread Pooling) feature'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgEmulateTlsTp
'' --------

' -------------------------------------------------------------------------
' Reusable worker thread
' -------------------------------------------------------------------------

Type ThreadInitThenMultiStart
	Public:
		Declare Constructor()
		Declare Sub ThreadInit(ByVal pThread As Function(ByVal As Any Ptr) As String, ByVal p As Any Ptr = 0)
		Declare Sub ThreadStart Overload()
		Declare Sub ThreadStart Overload(ByVal p As Any Ptr)
		Declare Function ThreadWait() As String

		Declare Property ThreadReady() As Boolean
		Declare Property ThreadState() As UByte

		Declare Destructor()
	Private:
		Dim As Function(ByVal p As Any Ptr) As String _pThread
		Dim As Any Ptr _p
		Dim As Any Ptr _mutex
		Dim As Any Ptr _startCondition
		Dim As Any Ptr _doneCondition
		Dim As Any Ptr _pt
		Dim As Integer _end
		Dim As Integer _startPending
		Dim As Integer _resultPending
		Dim As Integer _ready
		Dim As String _returnF
		Dim As UByte _state
		Declare Static Sub _Thread(ByVal p As Any Ptr)
End Type

Constructor ThreadInitThenMultiStart()
	This._mutex = MutexCreate()

	If This._mutex <> 0 Then
		This._startCondition = CondCreate()
	End If

	If This._startCondition <> 0 Then
		This._doneCondition = CondCreate()
	End If

	If This._doneCondition <> 0 Then
		This._ready = TRUE
	Else
		If This._startCondition <> 0 Then CondDestroy This._startCondition
		If This._mutex <> 0 Then MutexDestroy This._mutex
		This._startCondition = 0
		This._mutex = 0
	End If
End Constructor

Sub ThreadInitThenMultiStart.ThreadInit( _
	ByVal pThread As Function(ByVal As Any Ptr) As String, _
	ByVal p As Any Ptr = 0 )

	If (This._ready = FALSE) OrElse (pThread = 0) Then Exit Sub

	MutexLock This._mutex

	While (This._startPending OrElse This._resultPending) AndAlso (This._end = FALSE)
		CondWait This._doneCondition, This._mutex
	Wend

	If This._end = FALSE Then
		This._pThread = pThread
		This._p = p

		If This._pt = 0 Then
			This._pt = ThreadCreate(@ThreadInitThenMultiStart._Thread, @This)
		End If

		If This._pt <> 0 Then
			This._state = 1
		Else
			This._state = 0
		End If
	End If

	MutexUnlock This._mutex
End Sub

Sub ThreadInitThenMultiStart.ThreadStart Overload()
	If This._ready = FALSE Then Exit Sub

	MutexLock This._mutex

	While (This._startPending OrElse This._resultPending) AndAlso (This._end = FALSE)
		CondWait This._doneCondition, This._mutex
	Wend

	If (This._end = FALSE) AndAlso (This._pt <> 0) AndAlso (This._pThread <> 0) Then
		This._startPending = TRUE
		This._state = 1
		CondSignal This._startCondition
	End If

	MutexUnlock This._mutex
End Sub

Sub ThreadInitThenMultiStart.ThreadStart Overload(ByVal p As Any Ptr)
	If This._ready = FALSE Then Exit Sub

	MutexLock This._mutex

	While (This._startPending OrElse This._resultPending) AndAlso (This._end = FALSE)
		CondWait This._doneCondition, This._mutex
	Wend

	If (This._end = FALSE) AndAlso (This._pt <> 0) AndAlso (This._pThread <> 0) Then
		This._p = p
		This._startPending = TRUE
		This._state = 1
		CondSignal This._startCondition
	End If

	MutexUnlock This._mutex
End Sub

Function ThreadInitThenMultiStart.ThreadWait() As String
	If This._ready = FALSE Then Return ""

	MutexLock This._mutex

	While (This._resultPending = FALSE) AndAlso _
	      (This._end = FALSE) AndAlso (This._pt <> 0)
		CondWait This._doneCondition, This._mutex
	Wend

	Dim As String Result

	If This._resultPending Then
		Result = This._returnF
		This._resultPending = FALSE
		This._state = 1
		CondBroadcast This._doneCondition
	End If

	MutexUnlock This._mutex
	Return Result
End Function

Property ThreadInitThenMultiStart.ThreadReady() As Boolean
	Return This._ready <> FALSE
End Property

Property ThreadInitThenMultiStart.ThreadState() As UByte
	If This._ready = FALSE Then Return 0

	MutexLock This._mutex
	Dim As UByte State = This._state
	MutexUnlock This._mutex

	Return State
End Property

Sub ThreadInitThenMultiStart._Thread(ByVal p As Any Ptr)
	Dim As ThreadInitThenMultiStart Ptr Worker = _
	    Cast(ThreadInitThenMultiStart Ptr, p)
	If Worker = 0 Then Exit Sub

	MutexLock Worker->_mutex

	Do
		While (Worker->_startPending = FALSE) AndAlso (Worker->_end = FALSE)
			CondWait Worker->_startCondition, Worker->_mutex
		Wend

		If Worker->_end Then Exit Do

		Dim As Function(ByVal p As Any Ptr) As String CurrentThread = Worker->_pThread
		Dim As Any Ptr CurrentParameter = Worker->_p
		Worker->_state = 2
		MutexUnlock Worker->_mutex

		Dim As String Result
		If CurrentThread <> 0 Then Result = CurrentThread(CurrentParameter)

		MutexLock Worker->_mutex
		Worker->_returnF = Result
		Worker->_startPending = FALSE
		Worker->_resultPending = TRUE
		Worker->_state = 4
		CondBroadcast Worker->_doneCondition
	Loop

	MutexUnlock Worker->_mutex
End Sub

Destructor ThreadInitThenMultiStart()
	If This._ready Then
		If This._pt <> 0 Then
			MutexLock This._mutex
			This._end = TRUE
			CondBroadcast This._startCondition
			CondBroadcast This._doneCondition
			MutexUnlock This._mutex
			.ThreadWait This._pt
		End If

		CondDestroy This._startCondition
		CondDestroy This._doneCondition
		MutexDestroy This._mutex
		This._ready = FALSE
	End If
End Destructor

' -------------------------------------------------------------------------
' Single-thread task pool
' -------------------------------------------------------------------------

Const POOL_INITIAL_CAPACITY = 8
Const POOL_STATE_SUBMITTED As UByte = 1
Const POOL_STATE_RUNNING As UByte = 2
Const POOL_STATE_IDLE As UByte = 4
Const POOL_STATE_QUEUED As UByte = 8
Const POOL_BUSY_MASK As UByte = 11

Type ThreadPooling
	Public:
		Declare Constructor()
		Declare Sub PoolingSubmit(ByVal pThread As Function(ByVal As Any Ptr) As String, ByVal p As Any Ptr = 0)
		Declare Sub PoolingWait Overload()
		Declare Sub PoolingWait Overload(values() As String)

		Declare Property PoolingReady() As Boolean
		Declare Property PoolingState() As UByte
		Declare Property PoolingTaskCount() As Integer

		Declare Destructor()
	Private:
		Dim As Function(ByVal p As Any Ptr) As String _pThread(Any)
		Dim As Any Ptr _p(Any)
		Dim As String _returnF(Any)
		Dim As Any Ptr _mutex
		Dim As Any Ptr _cond1
		Dim As Any Ptr _cond2
		Dim As Any Ptr _pt
		Dim As Integer _end
		Dim As Integer _ready
		Dim As Integer _capacity
		Dim As Integer _taskCount
		Dim As Integer _nextTask
		Dim As Integer _completedCount
		Dim As UByte _state
		Declare Static Sub _Thread(ByVal p As Any Ptr)
End Type

Constructor ThreadPooling()
	This._mutex = MutexCreate()

	If This._mutex <> 0 Then
		This._cond1 = CondCreate()
	End If

	If This._cond1 <> 0 Then
		This._cond2 = CondCreate()
	End If

	If This._cond2 <> 0 Then
		This._pt = ThreadCreate(@ThreadPooling._Thread, @This)
	End If

	If This._pt <> 0 Then
		This._ready = TRUE
	Else
		If This._cond2 <> 0 Then CondDestroy This._cond2
		If This._cond1 <> 0 Then CondDestroy This._cond1
		If This._mutex <> 0 Then MutexDestroy This._mutex
		This._cond2 = 0
		This._cond1 = 0
		This._mutex = 0
	End If
End Constructor

Sub ThreadPooling.PoolingSubmit(ByVal pThread As Function(ByVal As Any Ptr) As String, ByVal p As Any Ptr = 0)
	If (This._ready = FALSE) OrElse (pThread = 0) Then Exit Sub

	MutexLock This._mutex

	If This._end = FALSE Then
		If This._taskCount = This._capacity Then
			Dim As Integer NewCapacity = This._capacity * 2
			If NewCapacity = 0 Then NewCapacity = POOL_INITIAL_CAPACITY

			ReDim Preserve This._pThread(0 To NewCapacity - 1)
			ReDim Preserve This._p(0 To NewCapacity - 1)
			ReDim Preserve This._returnF(0 To NewCapacity - 1)
			This._capacity = NewCapacity
		End If

		This._pThread(This._taskCount) = pThread
		This._p(This._taskCount) = p
		This._taskCount += 1
		This._state = POOL_STATE_SUBMITTED
		CondSignal This._cond2
	End If

	MutexUnlock This._mutex
End Sub

Sub ThreadPooling.PoolingWait Overload()
	If This._ready = FALSE Then Exit Sub

	MutexLock This._mutex

	While (This._completedCount < This._taskCount) AndAlso (This._end = FALSE)
		CondWait This._cond1, This._mutex
	Wend

	For ResultIndex As Integer = 0 To This._completedCount - 1
		This._returnF(ResultIndex) = ""
	Next ResultIndex

	This._taskCount = 0
	This._nextTask = 0
	This._completedCount = 0
	This._state = POOL_STATE_IDLE
	MutexUnlock This._mutex
End Sub

Sub ThreadPooling.PoolingWait Overload(values() As String)
	If This._ready = FALSE Then
		Erase values
		Exit Sub
	End If

	MutexLock This._mutex

	While (This._completedCount < This._taskCount) AndAlso (This._end = FALSE)
		CondWait This._cond1, This._mutex
	Wend

	If This._completedCount > 0 Then
		ReDim values(1 To This._completedCount)

		For ResultIndex As Integer = 0 To This._completedCount - 1
			values(ResultIndex + 1) = This._returnF(ResultIndex)
			This._returnF(ResultIndex) = ""
		Next ResultIndex
	Else
		Erase values
	End If

	This._taskCount = 0
	This._nextTask = 0
	This._completedCount = 0
	This._state = POOL_STATE_IDLE
	MutexUnlock This._mutex
End Sub

Property ThreadPooling.PoolingReady() As Boolean
	Return This._ready <> FALSE
End Property

Property ThreadPooling.PoolingState() As UByte
	If This._ready = FALSE Then Return 0

	MutexLock This._mutex
	Dim As UByte State = This._state

	If This._nextTask < This._taskCount Then
		State Or= POOL_STATE_QUEUED
	End If

	MutexUnlock This._mutex
	Return State
End Property

Property ThreadPooling.PoolingTaskCount() As Integer
	If This._ready = FALSE Then Return 0

	MutexLock This._mutex
	Dim As Integer TaskCount = This._taskCount
	MutexUnlock This._mutex

	Return TaskCount
End Property

Sub ThreadPooling._Thread(ByVal p As Any Ptr)
	Dim As ThreadPooling Ptr Pool = Cast(ThreadPooling Ptr, p)
	If Pool = 0 Then Exit Sub

	MutexLock Pool->_mutex

	Do
		While (Pool->_nextTask >= Pool->_taskCount) AndAlso (Pool->_end = FALSE)
			Pool->_state = POOL_STATE_IDLE
			CondBroadcast Pool->_cond1
			CondWait Pool->_cond2, Pool->_mutex
		Wend

		If Pool->_end Then Exit Do

		Dim As Function(ByVal p As Any Ptr) As String CurrentThread = _
		    Pool->_pThread(Pool->_nextTask)
		Dim As Any Ptr CurrentParameter = Pool->_p(Pool->_nextTask)
		Pool->_nextTask += 1
		Pool->_state = POOL_STATE_RUNNING
		MutexUnlock Pool->_mutex

		Dim As String Result
		If CurrentThread <> 0 Then Result = CurrentThread(CurrentParameter)

		MutexLock Pool->_mutex
		Pool->_returnF(Pool->_completedCount) = Result
		Pool->_completedCount += 1

		If Pool->_completedCount = Pool->_taskCount Then
			Pool->_state = POOL_STATE_IDLE
			CondBroadcast Pool->_cond1
		End If
	Loop

	MutexUnlock Pool->_mutex
End Sub

Destructor ThreadPooling()
	If This._ready Then
		MutexLock This._mutex
		This._end = TRUE
		CondBroadcast This._cond2
		MutexUnlock This._mutex
		.ThreadWait This._pt
		CondDestroy This._cond1
		CondDestroy This._cond2
		MutexDestroy This._mutex
		This._ready = FALSE
	End If
End Destructor

' -------------------------------------------------------------------------
' Multi-thread task dispatcher
' -------------------------------------------------------------------------

Type ThreadDispatching
	Public:
		Declare Constructor(ByVal nbMaxSecondaryThread As Integer = 1, ByVal nbMinSecondaryThread As Integer = 0)
		Declare Sub DispatchingSubmit(ByVal pThread As Function(ByVal As Any Ptr) As String, ByVal p As Any Ptr = 0)
		Declare Sub DispatchingWait Overload()
		Declare Sub DispatchingWait Overload(values() As String)

		Declare Property DispatchingThread() As Integer
		Declare Sub DispatchingState(state() As UByte)

		Declare Destructor()
	Private:
		Dim As Integer _nbmst
		Dim As Integer _dstnb
		Dim As Integer _poolCount
		Dim As Integer _poolCapacity
		Dim As Integer _submittedCount
		Dim As ThreadPooling Ptr _tp(Any)
End Type

Constructor ThreadDispatching(ByVal nbMaxSecondaryThread As Integer = 1, ByVal nbMinSecondaryThread As Integer = 0)
	If nbMaxSecondaryThread < 1 Then nbMaxSecondaryThread = 1
	If nbMinSecondaryThread < 0 Then nbMinSecondaryThread = 0

	This._nbmst = nbMaxSecondaryThread

	If nbMinSecondaryThread > nbMaxSecondaryThread Then
		nbMinSecondaryThread = nbMaxSecondaryThread
	End If

	If nbMinSecondaryThread > 0 Then
		ReDim This._tp(0 To nbMinSecondaryThread - 1)
		This._poolCapacity = nbMinSecondaryThread

		For PoolIndex As Integer = 0 To nbMinSecondaryThread - 1
			Dim As ThreadPooling Ptr Pool = New ThreadPooling

			If Pool <> 0 Then
				If Pool->PoolingReady Then
					This._tp(This._poolCount) = Pool
					This._poolCount += 1
				Else
					Delete Pool
				End If
			End If
		Next PoolIndex
	End If
End Constructor

Sub ThreadDispatching.DispatchingSubmit(ByVal pThread As Function(ByVal As Any Ptr) As String, ByVal p As Any Ptr = 0)
	If pThread = 0 Then Exit Sub

	Dim As Integer TargetPool = -1

	For PoolIndex As Integer = 0 To This._poolCount - 1
		If (This._tp(PoolIndex)->PoolingState And POOL_BUSY_MASK) = 0 Then
			TargetPool = PoolIndex
			Exit For
		End If
	Next PoolIndex

	If (TargetPool < 0) AndAlso (This._poolCount < This._nbmst) Then
		If This._poolCount = This._poolCapacity Then
			Dim As Integer NewCapacity = This._poolCapacity * 2
			If NewCapacity = 0 Then NewCapacity = 1
			If NewCapacity > This._nbmst Then NewCapacity = This._nbmst

			ReDim Preserve This._tp(0 To NewCapacity - 1)
			This._poolCapacity = NewCapacity
		End If

		Dim As ThreadPooling Ptr Pool = New ThreadPooling

		If Pool <> 0 Then
			If Pool->PoolingReady Then
				TargetPool = This._poolCount
				This._tp(TargetPool) = Pool
				This._poolCount += 1
			Else
				Delete Pool
			End If
		End If
	End If

	If (TargetPool < 0) AndAlso (This._poolCount > 0) Then
		If This._dstnb >= This._poolCount Then This._dstnb = 0
		TargetPool = This._dstnb
		This._dstnb = (This._dstnb + 1) Mod This._poolCount
	End If

	If TargetPool >= 0 Then
		This._tp(TargetPool)->PoolingSubmit(pThread, p)
		This._submittedCount += 1
	End If
End Sub

Sub ThreadDispatching.DispatchingWait Overload()
	For PoolIndex As Integer = 0 To This._poolCount - 1
		This._tp(PoolIndex)->PoolingWait()
	Next PoolIndex

	This._submittedCount = 0
End Sub

Sub ThreadDispatching.DispatchingWait Overload(values() As String)
	If This._submittedCount = 0 Then
		Erase values
		Exit Sub
	End If

	ReDim values(1 To This._submittedCount)

	Dim As String PoolValues()
	Dim As Integer NextValue = 1

	For PoolIndex As Integer = 0 To This._poolCount - 1
		Dim As Integer PoolResultCount = This._tp(PoolIndex)->PoolingTaskCount
		This._tp(PoolIndex)->PoolingWait(PoolValues())

		If PoolResultCount > 0 Then
			For ResultOffset As Integer = 0 To PoolResultCount - 1
				If NextValue <= This._submittedCount Then
					values(NextValue) = PoolValues(ResultOffset + 1)
					NextValue += 1
				End If
			Next ResultOffset
		End If
	Next PoolIndex

	This._submittedCount = 0
End Sub

Property ThreadDispatching.DispatchingThread() As Integer
	Return This._poolCount
End Property

Sub ThreadDispatching.DispatchingState(state() As UByte)
	If This._poolCount > 0 Then
		ReDim state(1 To This._poolCount)

		For PoolIndex As Integer = 0 To This._poolCount - 1
			state(PoolIndex + 1) = This._tp(PoolIndex)->PoolingState
		Next PoolIndex
	Else
		Erase state
	End If
End Sub

Destructor ThreadDispatching()
	For PoolIndex As Integer = 0 To This._poolCount - 1
		Delete This._tp(PoolIndex)
	Next PoolIndex
End Destructor

' -------------------------------------------------------------------------
' Benchmark workload and timing
' -------------------------------------------------------------------------

Const PROCEDURE_CALL_COUNT = 1000000
Const ELEMENTARY_THREAD_COUNT = 1000
Const REUSABLE_THREAD_COUNT = 10000
Const POOLING_TASK_COUNT = 10000
Const DISPATCHING_TASK_COUNT = 10000
Const SECONDS_PER_DAY = 86400.0
Const MILLISECONDS_PER_SECOND = 1000.0

Function ElapsedSeconds(ByVal StartedAt As Double) As Double
	Dim As Double FinishedAt = Timer

	If FinishedAt < StartedAt Then
		FinishedAt += SECONDS_PER_DAY
	End If

	Return FinishedAt - StartedAt
End Function

Sub s(ByVal p As Any Ptr)
	'' user task
End Sub

Function f(ByVal p As Any Ptr) As String
	'' user task
	Return ""
End Function

'---------------------------------------------------
'Time wasted when running a user task either by procedure calling or by various threading methods
Print "Mean time wasted when running a user task :"
Print "   either by procedure calling method,"
Print "   or by various threading methods."
Print

Scope
	Dim As Double t = Timer
	For I As Integer = 1 To PROCEDURE_CALL_COUNT
		s(0)
	Next I
	t = ElapsedSeconds(t)
	Print Using "      - Using procedure calling method        : ###.###### ms"; _
	    t * MILLISECONDS_PER_SECOND / PROCEDURE_CALL_COUNT
	Print
End Scope

Scope
	Dim As Any Ptr P
	Dim As Double t = Timer
	For I As Integer = 1 To ELEMENTARY_THREAD_COUNT
		p = ThreadCreate(@s)
		If p <> 0 Then ThreadWait(p)
	Next I
	t = ElapsedSeconds(t)
	Print Using "      - Using elementary threading method     : ###.###### ms"; _
	    t * MILLISECONDS_PER_SECOND / ELEMENTARY_THREAD_COUNT
	Print
End Scope

Scope
	Dim As ThreadInitThenMultiStart ts
	Dim As Double t = Timer
	ts.ThreadInit(@f)
	For I As Integer = 1 To REUSABLE_THREAD_COUNT
		ts.ThreadStart()
		ts.ThreadWait()
	Next I
	t = ElapsedSeconds(t)
	Print Using "      - Using ThreadInitThenMultiStart method : ###.###### ms"; _
	    t * MILLISECONDS_PER_SECOND / REUSABLE_THREAD_COUNT
End Scope

Scope
	Dim As ThreadPooling tp
	Dim As Double t = Timer
	For I As Integer = 1 To POOLING_TASK_COUNT
		tp.PoolingSubmit(@f)
	Next I
	tp.PoolingWait()
	t = ElapsedSeconds(t)
	Print Using "      - Using ThreadPooling method            : ###.###### ms"; _
	    t * MILLISECONDS_PER_SECOND / POOLING_TASK_COUNT
End Scope

Scope
	Dim As ThreadDispatching td
	Dim As Double t = Timer
	For I As Integer = 1 To DISPATCHING_TASK_COUNT
		td.DispatchingSubmit(@f)
	Next I
	td.DispatchingWait()
	t = ElapsedSeconds(t)
	Print Using "      - Using ThreadDispatching method        : ###.###### ms"; _
	    t * MILLISECONDS_PER_SECOND / DISPATCHING_TASK_COUNT
End Scope

Print
