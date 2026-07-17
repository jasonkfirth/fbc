'' examples/manual/threads/threads3.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'THREADCREATE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgThreadCreate
'' --------

'' Threaded producer/consumer example using a one-item buffer.
''
'' The mutex protects ItemReady and ShutdownRequested. CondWait releases the
'' mutex while a thread waits and reacquires it before returning.

Const ITEM_COUNT = 10

Type THREAD_STATE
	Lock As Any Ptr
	CanProduce As Any Ptr
	CanConsume As Any Ptr
	ItemReady As Integer
	ShutdownRequested As Integer
End Type

Sub DestroyState( ByRef State As THREAD_STATE )
	If State.CanConsume <> 0 Then CondDestroy State.CanConsume
	If State.CanProduce <> 0 Then CondDestroy State.CanProduce
	If State.Lock <> 0 Then MutexDestroy State.Lock
End Sub

Sub RequestShutdown( ByRef State As THREAD_STATE )
	MutexLock State.Lock
	State.ShutdownRequested = TRUE
	CondBroadcast State.CanConsume
	CondBroadcast State.CanProduce
	MutexUnlock State.Lock
End Sub

Sub Consumer( ByVal Param As Any Ptr )
	Dim State As THREAD_STATE Ptr = Cast(THREAD_STATE Ptr, Param)
	If State = 0 Then Exit Sub

	For I As Integer = 0 To ITEM_COUNT - 1
		MutexLock State->Lock

		While (State->ItemReady = FALSE) AndAlso _
		      (State->ShutdownRequested = FALSE)
			CondWait State->CanConsume, State->Lock
		Wend

		If State->ShutdownRequested AndAlso (State->ItemReady = FALSE) Then
			MutexUnlock State->Lock
			Exit Sub
		End If

		Print ", consumer gets: "; I
		State->ItemReady = FALSE
		CondSignal State->CanProduce
		MutexUnlock State->Lock
	Next
End Sub

Sub Producer( ByVal Param As Any Ptr )
	Dim State As THREAD_STATE Ptr = Cast(THREAD_STATE Ptr, Param)
	If State = 0 Then Exit Sub

	For I As Integer = 0 To ITEM_COUNT - 1
		MutexLock State->Lock

		While (State->ItemReady <> FALSE) AndAlso _
		      (State->ShutdownRequested = FALSE)
			CondWait State->CanProduce, State->Lock
		Wend

		If State->ShutdownRequested Then
			MutexUnlock State->Lock
			Exit Sub
		End If

		Print "Producer puts: "; I;
		State->ItemReady = TRUE
		CondSignal State->CanConsume
		MutexUnlock State->Lock
	Next
End Sub

Dim State As THREAD_STATE
Dim As Any Ptr ConsumerId, ProducerId

State.Lock = MutexCreate
If State.Lock = 0 Then
	Print "Error creating the producer/consumer mutex."
	End 1
End If

State.CanProduce = CondCreate
If State.CanProduce = 0 Then
	DestroyState State
	Print "Error creating the producer condition."
	End 1
End If

State.CanConsume = CondCreate
If State.CanConsume = 0 Then
	DestroyState State
	Print "Error creating the consumer condition."
	End 1
End If

ConsumerId = ThreadCreate(@Consumer, @State)
If ConsumerId = 0 Then
	DestroyState State
	Print "Error creating the consumer thread."
	End 1
End If

ProducerId = ThreadCreate(@Producer, @State)
If ProducerId = 0 Then
	RequestShutdown State
	ThreadWait ConsumerId
	DestroyState State
	Print "Error creating the producer thread."
	End 1
End If

ThreadWait ProducerId
ThreadWait ConsumerId
DestroyState State
