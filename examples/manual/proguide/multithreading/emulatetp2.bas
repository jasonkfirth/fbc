'' examples/manual/proguide/multithreading/emulatetp2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Emulate a TLS (Thread Local Storage) and a TP (Thread Pooling) feature'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgEmulateTlsTp
'' --------

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

'---------------------------------------------------

Const PRINT_REPETITIONS = 10
Const PRINT_DELAY_MILLISECONDS = 100
Const RETURN_VALUE_COUNT = 3

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

Dim As String sa = "  Sequence #a: "
Dim As String sb = "  Sequence #b: "
Dim As String s()

Dim As ThreadPooling t

If t.PoolingReady = FALSE Then
	Print "Unable to initialize the thread pool."
	End 1
End If

t.PoolingSubmit(@UserCode1, @sa)
t.PoolingSubmit(@UserCode2)
t.PoolingSubmit(@UserCode3)
Print " Sequence #a of 3 user thread functions fully submitted "
t.PoolingWait()
Print
Print " Sequence #a completed"
Print

t.PoolingSubmit(@UserCode4, @sb)
t.PoolingSubmit(@UserCode5)
t.PoolingSubmit(@UserCode6)
Print " Sequence #b of 3 user thread functions fully submitted "
t.PoolingWait(s())
Print
Print " Sequence #b completed"
Print

Print " List of returned values from sequence #b only"
For I As Integer = 1 To RETURN_VALUE_COUNT
	Print "  " & I & ": " & s(I)
Next I
Print
