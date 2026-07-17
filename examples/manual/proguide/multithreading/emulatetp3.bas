'' examples/manual/proguide/multithreading/emulatetp3.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Emulate a TLS (Thread Local Storage) and a TP (Thread Pooling) feature'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgEmulateTlsTp
'' --------

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
		Declare Property DispatchingTaskCount() As Integer
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

Property ThreadDispatching.DispatchingTaskCount() As Integer
	Return This._submittedCount
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
' Dispatcher demonstration
' -------------------------------------------------------------------------

Const PRINT_REPETITIONS = 10
Const PRINT_DELAY_MILLISECONDS = 100

Sub Prnt (ByRef s As String, ByVal p As Any Ptr)
	Dim As String Ptr ps = p
	If ps <> 0 Then Print *ps;

	For I As Integer = 1 To PRINT_REPETITIONS
		Print s;
		Sleep PRINT_DELAY_MILLISECONDS, 1
	Next I
End Sub

Function UserCode1 (ByVal p As Any Ptr) As String
	Prnt("1", p)
	Return "UserCode #1"
End Function

Function UserCode2 (ByVal p As Any Ptr) As String
	Prnt("2", p)
	Return "UserCode #2"
End Function

Function UserCode3 (ByVal p As Any Ptr) As String
	Prnt("3", p)
	Return "UserCode #3"
End Function

Function UserCode4 (ByVal p As Any Ptr) As String
	Prnt("4", p)
	Return "UserCode #4"
End Function

Function UserCode5 (ByVal p As Any Ptr) As String
	Prnt("5", p)
	Return "UserCode #5"
End Function

Function UserCode6 (ByVal p As Any Ptr) As String
	Prnt("6", p)
	Return "UserCode #6"
End Function

Sub SubmitSequence(ByRef t As ThreadDispatching, ByVal ps As String Ptr)
	t.DispatchingSubmit(@UserCode1, ps)
	t.DispatchingSubmit(@UserCode2)
	t.DispatchingSubmit(@UserCode3)
	t.DispatchingSubmit(@UserCode4)
	t.DispatchingSubmit(@UserCode5)
	t.DispatchingSubmit(@UserCode6)
End Sub

Dim As String sa = "  Sequence #a: "
Dim As String sb = "  Sequence #b: "
Dim As String sc = "  Sequence #c: "
Dim As String sd = "  Sequence #d: "
Dim As String se = "  Sequence #e: "
Dim As String sf = "  Sequence #f: "
Dim As String s()

Dim As ThreadDispatching t1
Dim As ThreadDispatching t2 = 2
Dim As ThreadDispatching t3 = 3
Dim As ThreadDispatching t4 = 4
Dim As ThreadDispatching t5 = 5
Dim As ThreadDispatching t6 = 6

Print " Sequence #a of 6 user thread functions dispatched over 1 secondary thread:"
SubmitSequence(t1, @sa)
t1.DispatchingWait()
Print
Print

Print " Sequence #b of 6 user thread functions dispatched over 2 secondary threads:"
SubmitSequence(t2, @sb)
t2.DispatchingWait()
Print
Print

Print " Sequence #c of 6 user thread functions dispatched over 3 secondary threads:"
SubmitSequence(t3, @sc)
t3.DispatchingWait()
Print
Print

Print " Sequence #d of 6 user thread functions dispatched over 4 secondary threads:"
SubmitSequence(t4, @sd)
t4.DispatchingWait()
Print
Print

Print " Sequence #e of 6 user thread functions dispatched over 5 secondary threads:"
SubmitSequence(t5, @se)
t5.DispatchingWait()
Print
Print

Print " Sequence #f of 6 user thread functions dispatched over 6 secondary threads:"
SubmitSequence(t6, @sf)
Dim As Integer ReturnValueCount = t6.DispatchingTaskCount
t6.DispatchingWait(s())
Print

Print "  List of returned values from sequence #f:"
For I As Integer = 1 To ReturnValueCount
	Print "   " & I & ": " & s(I)
Next I
