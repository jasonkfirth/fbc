'' examples/manual/proguide/multithreading/emulatetp1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Emulate a TLS (Thread Local Storage) and a TP (Thread Pooling) feature'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgEmulateTlsTp
'' --------

'' A reusable worker thread controlled by one owned mutex and two conditions.
'' StartPending protects work in progress, while ResultPending prevents a new
'' launch from replacing a result that the caller has not collected yet.

Const SAMPLE_LAST = 9

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

'---------------------------------------------------

Function UserThreadS(ByVal p As Any Ptr) As String
	Dim As UInteger Ptr pui = p
	If pui = 0 Then Return ""

	Print *pui * *pui
	Return ""
End Function

Function UserThreadF(ByVal p As Any Ptr) As String
	Dim As UInteger Ptr pui = p
	If pui = 0 Then Return ""

	Dim As UInteger c = (*pui) * (*pui)
	Return Str(c)
End Function

Dim As ThreadInitThenMultiStart Worker

If Worker.ThreadReady = FALSE Then
	Print "Unable to initialize the reusable worker."
	End 1
End If

Print "First user function executed like a thread subroutine:"
Worker.ThreadInit(@UserThreadS)

For I As UInteger = 1 To SAMPLE_LAST
	Print I & "^2 = ";
	Worker.ThreadStart(@I)
	Worker.ThreadWait()
Next I
Print

Print "Second user function executed like a thread function:"
Worker.ThreadInit(@UserThreadF)

For I As UInteger = 1 To SAMPLE_LAST
	Print I & "^2 = ";
	Worker.ThreadStart(@I)
	Print Worker.ThreadWait()
Next I
Print
