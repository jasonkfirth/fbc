'' examples/manual/proguide/multithreading/emulatetp4.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Emulate a TLS (Thread Local Storage) and a TP (Thread Pooling) feature'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgEmulateTlsTp
'' --------

' -------------------------------------------------------------------------
' Reusable worker thread
' -------------------------------------------------------------------------

Type ThreadInitThenMultiStartData
	Dim As Function(ByVal p As Any Ptr) As String _pThread
	Dim As Any Ptr _p
	Dim As Any Ptr _mutex
	Dim As Any Ptr _startCondition
	Dim As Any Ptr _doneCondition
	Dim As Any Ptr _pt
	Dim As Integer _end
	Dim As Integer _startPending
	Dim As Integer _resultPending
	Dim As String _returnF
	Dim As UByte _state
End Type

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
		Dim As ThreadInitThenMultiStartData Ptr _pdata
		Declare Static Sub _Thread(ByVal p As Any Ptr)
		Declare Constructor(ByRef t As ThreadInitThenMultiStart)
		Declare Operator Let(ByRef t As ThreadInitThenMultiStart)
End Type

Constructor ThreadInitThenMultiStart()
	This._pdata = New ThreadInitThenMultiStartData

	If This._pdata <> 0 Then
		With *This._pdata
			._mutex = MutexCreate()

			If ._mutex <> 0 Then
				._startCondition = CondCreate()
			End If

			If ._startCondition <> 0 Then
				._doneCondition = CondCreate()
			End If

			If ._doneCondition = 0 Then
				If ._startCondition <> 0 Then CondDestroy ._startCondition
				If ._mutex <> 0 Then MutexDestroy ._mutex
				Delete This._pdata
				This._pdata = 0
			End If
		End With
	End If
End Constructor

Sub ThreadInitThenMultiStart.ThreadInit( _
	ByVal pThread As Function(ByVal As Any Ptr) As String, _
	ByVal p As Any Ptr = 0 )

	If (This._pdata = 0) OrElse (pThread = 0) Then Exit Sub

	With *This._pdata
		MutexLock ._mutex

		While (._startPending OrElse ._resultPending) AndAlso (._end = FALSE)
			CondWait ._doneCondition, ._mutex
		Wend

		If ._end = FALSE Then
			._pThread = pThread
			._p = p

			If ._pt = 0 Then
				._pt = ThreadCreate(@ThreadInitThenMultiStart._Thread, This._pdata)
			End If

			If ._pt <> 0 Then
				._state = 1
			Else
				._state = 0
			End If
		End If

		MutexUnlock ._mutex
	End With
End Sub

Sub ThreadInitThenMultiStart.ThreadStart Overload()
	If This._pdata = 0 Then Exit Sub

	With *This._pdata
		MutexLock ._mutex

		While (._startPending OrElse ._resultPending) AndAlso (._end = FALSE)
			CondWait ._doneCondition, ._mutex
		Wend

		If (._end = FALSE) AndAlso (._pt <> 0) AndAlso (._pThread <> 0) Then
			._startPending = TRUE
			._state = 1
			CondSignal ._startCondition
		End If

		MutexUnlock ._mutex
	End With
End Sub

Sub ThreadInitThenMultiStart.ThreadStart Overload(ByVal p As Any Ptr)
	If This._pdata = 0 Then Exit Sub

	With *This._pdata
		MutexLock ._mutex

		While (._startPending OrElse ._resultPending) AndAlso (._end = FALSE)
			CondWait ._doneCondition, ._mutex
		Wend

		If (._end = FALSE) AndAlso (._pt <> 0) AndAlso (._pThread <> 0) Then
			._p = p
			._startPending = TRUE
			._state = 1
			CondSignal ._startCondition
		End If

		MutexUnlock ._mutex
	End With
End Sub

Function ThreadInitThenMultiStart.ThreadWait() As String
	If This._pdata = 0 Then Return ""

	With *This._pdata
		MutexLock ._mutex

		While (._resultPending = FALSE) AndAlso (._end = FALSE) AndAlso (._pt <> 0)
			CondWait ._doneCondition, ._mutex
		Wend

		Dim As String Result

		If ._resultPending Then
			Result = ._returnF
			._resultPending = FALSE
			._state = 1
			CondBroadcast ._doneCondition
		End If

		MutexUnlock ._mutex
		Return Result
	End With
End Function

Property ThreadInitThenMultiStart.ThreadReady() As Boolean
	Return This._pdata <> 0
End Property

Property ThreadInitThenMultiStart.ThreadState() As UByte
	If This._pdata = 0 Then Return 0

	MutexLock This._pdata->_mutex
	Dim As UByte State = This._pdata->_state
	MutexUnlock This._pdata->_mutex

	Return State
End Property

Sub ThreadInitThenMultiStart._Thread(ByVal p As Any Ptr)
	Dim As ThreadInitThenMultiStartData Ptr StateData = _
	    Cast(ThreadInitThenMultiStartData Ptr, p)
	If StateData = 0 Then Exit Sub

	MutexLock StateData->_mutex

	Do
		While (StateData->_startPending = FALSE) AndAlso (StateData->_end = FALSE)
			CondWait StateData->_startCondition, StateData->_mutex
		Wend

		If StateData->_end Then Exit Do

		Dim As Function(ByVal p As Any Ptr) As String CurrentThread = StateData->_pThread
		Dim As Any Ptr CurrentParameter = StateData->_p
		StateData->_state = 2
		MutexUnlock StateData->_mutex

		Dim As String Result
		If CurrentThread <> 0 Then Result = CurrentThread(CurrentParameter)

		MutexLock StateData->_mutex
		StateData->_returnF = Result
		StateData->_startPending = FALSE
		StateData->_resultPending = TRUE
		StateData->_state = 4
		CondBroadcast StateData->_doneCondition
	Loop

	MutexUnlock StateData->_mutex
End Sub

Destructor ThreadInitThenMultiStart()
	If This._pdata <> 0 Then
		With *This._pdata
			If ._pt <> 0 Then
				MutexLock ._mutex
				._end = TRUE
				CondBroadcast ._startCondition
				CondBroadcast ._doneCondition
				MutexUnlock ._mutex
				..ThreadWait ._pt
			End If

			CondDestroy ._startCondition
			CondDestroy ._doneCondition
			MutexDestroy ._mutex
		End With

		Delete This._pdata
		This._pdata = 0
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

Type ThreadPoolingData
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
End Type

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
		Dim As ThreadPoolingData Ptr _pdata
		Declare Static Sub _Thread(ByVal p As Any Ptr)
		Declare Constructor(ByRef t As ThreadPooling)
		Declare Operator Let(ByRef t As ThreadPooling)
End Type

Constructor ThreadPooling()
	This._pdata = New ThreadPoolingData

	If This._pdata <> 0 Then
		With *This._pdata
			._mutex = MutexCreate()

			If ._mutex <> 0 Then
				._cond1 = CondCreate()
			End If

			If ._cond1 <> 0 Then
				._cond2 = CondCreate()
			End If

			If ._cond2 <> 0 Then
				._pt = ThreadCreate(@ThreadPooling._Thread, This._pdata)
			End If

			If ._pt <> 0 Then
				._ready = TRUE
			Else
				If ._cond2 <> 0 Then CondDestroy ._cond2
				If ._cond1 <> 0 Then CondDestroy ._cond1
				If ._mutex <> 0 Then MutexDestroy ._mutex
				Delete This._pdata
				This._pdata = 0
			End If
		End With
	End If
End Constructor

Sub ThreadPooling.PoolingSubmit(ByVal pThread As Function(ByVal As Any Ptr) As String, ByVal p As Any Ptr = 0)
	If (This._pdata = 0) OrElse (pThread = 0) Then Exit Sub

	With *This._pdata
		MutexLock ._mutex

		If ._end = FALSE Then
			If ._taskCount = ._capacity Then
				Dim As Integer NewCapacity = ._capacity * 2
				If NewCapacity = 0 Then NewCapacity = POOL_INITIAL_CAPACITY

				ReDim Preserve ._pThread(0 To NewCapacity - 1)
				ReDim Preserve ._p(0 To NewCapacity - 1)
				ReDim Preserve ._returnF(0 To NewCapacity - 1)
				._capacity = NewCapacity
			End If

			._pThread(._taskCount) = pThread
			._p(._taskCount) = p
			._taskCount += 1
			._state = POOL_STATE_SUBMITTED
			CondSignal ._cond2
		End If

		MutexUnlock ._mutex
	End With
End Sub

Sub ThreadPooling.PoolingWait Overload()
	If This._pdata = 0 Then Exit Sub

	With *This._pdata
		MutexLock ._mutex

		While (._completedCount < ._taskCount) AndAlso (._end = FALSE)
			CondWait ._cond1, ._mutex
		Wend

		For ResultIndex As Integer = 0 To ._completedCount - 1
			._returnF(ResultIndex) = ""
		Next ResultIndex

		._taskCount = 0
		._nextTask = 0
		._completedCount = 0
		._state = POOL_STATE_IDLE
		MutexUnlock ._mutex
	End With
End Sub

Sub ThreadPooling.PoolingWait Overload(values() As String)
	If This._pdata = 0 Then
		Erase values
		Exit Sub
	End If

	With *This._pdata
		MutexLock ._mutex

		While (._completedCount < ._taskCount) AndAlso (._end = FALSE)
			CondWait ._cond1, ._mutex
		Wend

		If ._completedCount > 0 Then
			ReDim values(1 To ._completedCount)

			For ResultIndex As Integer = 0 To ._completedCount - 1
				values(ResultIndex + 1) = ._returnF(ResultIndex)
				._returnF(ResultIndex) = ""
			Next ResultIndex
		Else
			Erase values
		End If

		._taskCount = 0
		._nextTask = 0
		._completedCount = 0
		._state = POOL_STATE_IDLE
		MutexUnlock ._mutex
	End With
End Sub

Property ThreadPooling.PoolingReady() As Boolean
	Return This._pdata <> 0
End Property

Property ThreadPooling.PoolingState() As UByte
	If This._pdata = 0 Then Return 0

	With *This._pdata
		MutexLock ._mutex
		Dim As UByte State = ._state

		If ._nextTask < ._taskCount Then
			State Or= POOL_STATE_QUEUED
		End If

		MutexUnlock ._mutex
		Return State
	End With
End Property

Property ThreadPooling.PoolingTaskCount() As Integer
	If This._pdata = 0 Then Return 0

	MutexLock This._pdata->_mutex
	Dim As Integer TaskCount = This._pdata->_taskCount
	MutexUnlock This._pdata->_mutex

	Return TaskCount
End Property

Sub ThreadPooling._Thread(ByVal p As Any Ptr)
	Dim As ThreadPoolingData Ptr Pool = Cast(ThreadPoolingData Ptr, p)
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
	If This._pdata <> 0 Then
		With *This._pdata
			MutexLock ._mutex
			._end = TRUE
			CondBroadcast ._cond2
			MutexUnlock ._mutex
			..ThreadWait ._pt
			CondDestroy ._cond1
			CondDestroy ._cond2
			MutexDestroy ._mutex
		End With

		Delete This._pdata
		This._pdata = 0
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
		Declare Constructor(ByRef t As ThreadDispatching)
		Declare Operator Let(ByRef t As ThreadDispatching)
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

Const TASK_SLOT_COUNT = 32
Const HEX_LABEL_COUNT = 16
Const ASCII_HEX_LETTER_OFFSET = 55
Const WORK_REPETITIONS = 2
Const WORK_ITERATIONS = 800000
Const SECONDS_PER_DAY = 86400.0

Function ElapsedSeconds(ByVal StartedAt As Double) As Double
	Dim As Double FinishedAt = Timer

	If FinishedAt < StartedAt Then
		FinishedAt += SECONDS_PER_DAY
	End If

	Return FinishedAt - StartedAt
End Function

Function UserCode (ByVal p As Any Ptr) As String
	Dim As String Ptr ps = p
	If ps = 0 Then Return ""

	Dim As Double WorkValue

	For I As Integer = 1 To WORK_REPETITIONS
		Print *ps;
		For J As Integer = 1 To WORK_ITERATIONS
			'' This calculation keeps the worker CPU-bound without sharing writable
			'' storage between threads.
			WorkValue = Tan(J) * Atn(J) * Exp(J) * Log(J)
		Next J
	Next I

	Return Str(WorkValue)
End Function

Dim As String s(0 To TASK_SLOT_COUNT - 1)
For I As Integer = 0 To HEX_LABEL_COUNT - 1
	s(I) = Str(Hex(I))
Next I
For I As Integer = HEX_LABEL_COUNT To TASK_SLOT_COUNT - 1
	s(I) = Chr(ASCII_HEX_LETTER_OFFSET + I)
Next I

'---------------------------------------------------

#macro ThreadInitThenMultiStartSequence(nbThread)
Scope
	Dim As ThreadInitThenMultiStart ts(0 To nbThread - 1)
	Print "   ";
	Dim As Double t = Timer
	For I As Integer = 0 To TASK_SLOT_COUNT - nbThread Step nbThread
		For J As Integer = 0 To nbThread - 1
			ts(J).ThreadInit(@UserCode, @s(I + J))
			ts(J).ThreadStart()
		Next J
		For J As Integer = 0 To nbThread - 1
			ts(J).ThreadWait()
		Next J
	Next I
	t = ElapsedSeconds(t)
	Print Using " : ####.## s"; t
End Scope
#endmacro

#macro ThreadPoolingSequence(nbThread)
Scope
	Dim As ThreadPooling tp(0 To nbThread - 1)
	Print "   ";
	Dim As Double t = Timer
	For I As Integer = 0 To TASK_SLOT_COUNT - nbThread Step nbThread
		For J As Integer = 0 To nbThread - 1
			tp(J).PoolingSubmit(@UserCode, @s(I + J))
		Next J
	Next I
	For I As Integer = 0 To nbThread - 1
		tp(I).PoolingWait()
	Next I
	t = ElapsedSeconds(t)
	Print Using " : ####.## s"; t
End Scope
#endmacro

#macro ThreadDispatchingSequence(nbThreadmax)
Scope
	Dim As ThreadDispatching td##nbThreadmax = nbThreadmax
	Print "   ";
	Dim As Double t = Timer
	For I As Integer = 0 To TASK_SLOT_COUNT - 1
		td##nbThreadmax.DispatchingSubmit(@UserCode, @s(I))
	Next I
	td##nbThreadmax.DispatchingWait()
	t = ElapsedSeconds(t)
	Print Using " : ####.## s"; t
End Scope
#endmacro

'---------------------------------------------------

Print "'ThreadInitThenMultiStart' with 1 secondary thread:"
ThreadInitThenMultiStartSequence(1)

Print "'ThreadPooling' with 1 secondary thread:"
ThreadPoolingSequence(1)

Print "'ThreadDispatching' with 1 secondary thread max:"
ThreadDispatchingSequence(1)
Print

'---------------------------------------------------

Print "'ThreadInitThenMultiStart' with 2 secondary threads:"
ThreadInitThenMultiStartSequence(2)

Print "'ThreadPooling' with 2 secondary threads:"
ThreadPoolingSequence(2)

Print "'ThreadDispatching' with 2 secondary threads max:"
ThreadDispatchingSequence(2)
Print

'---------------------------------------------------

Print "'ThreadInitThenMultiStart' with 4 secondary threads:"
ThreadInitThenMultiStartSequence(4)

Print "'ThreadPooling' with 4 secondary threads:"
ThreadPoolingSequence(4)

Print "'ThreadDispatching' with 4 secondary threads max:"
ThreadDispatchingSequence(4)
Print

'---------------------------------------------------

Print "'ThreadInitThenMultiStart' with 8 secondary threads:"
ThreadInitThenMultiStartSequence(8)

Print "'ThreadPooling' with 8 secondary threads:"
ThreadPoolingSequence(8)

Print "'ThreadDispatching' with 8 secondary threads max:"
ThreadDispatchingSequence(8)
Print

'---------------------------------------------------

Print "'ThreadInitThenMultiStart' with 16 secondary threads:"
ThreadInitThenMultiStartSequence(16)

Print "'ThreadPooling' with 16 secondary threads:"
ThreadPoolingSequence(16)

Print "'ThreadDispatching' with 16 secondary threads max:"
ThreadDispatchingSequence(16)
Print

'---------------------------------------------------

Print "'ThreadInitThenMultiStart' with 32 secondary threads:"
ThreadInitThenMultiStartSequence(32)

Print "'ThreadPooling' with 32 secondary threads:"
ThreadPoolingSequence(32)

Print "'ThreadDispatching' with 32 secondary threads max:"
ThreadDispatchingSequence(32)
Print
