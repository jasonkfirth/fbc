'' examples/manual/proguide/multithreading/criticalsectionfaq15.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Critical Sections FAQ'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgMtCriticalSectionsFAQ
'' --------

'' The reusable worker owns one mutex. LaunchPending and Quit are protected by
'' that mutex, while the two conditions wake the worker and its caller. The user
'' task runs without the mutex held so it cannot block control operations.

Const TASK_COUNT = 10000
Const SECONDS_PER_DAY = 86400.0
Const MICROSECONDS_PER_TASK_SCALE = 100.0

Sub UserTask(ByVal p As Any Ptr)
End Sub

Function ElapsedSeconds(ByVal StartedAt As Double) As Double
	Dim As Double FinishedAt = Timer

	If FinishedAt < StartedAt Then
		FinishedAt += SECONDS_PER_DAY
	End If

	Return FinishedAt - StartedAt
End Function

Type Thread
	Public:
		Dim As Sub(ByVal p As Any Ptr) Task
		Declare Sub Launch()
		Declare Sub Wait()
		Declare Property Ready() As Boolean
		Declare Constructor()
		Declare Destructor()
	Private:
		Dim As Any Ptr Mutex
		Dim As Any Ptr LaunchCondition
		Dim As Any Ptr DoneCondition
		Dim As Any Ptr Handle
		Dim As Boolean Quit
		Dim As Boolean LaunchPending
		Declare Static Sub Proc(ByVal Worker As Thread Ptr)
End Type

Constructor Thread()
	This.Mutex = MutexCreate

	If This.Mutex <> 0 Then
		This.LaunchCondition = CondCreate
	End If

	If This.LaunchCondition <> 0 Then
		This.DoneCondition = CondCreate
	End If

	If This.DoneCondition <> 0 Then
		This.Handle = ThreadCreate(CPtr(Any Ptr, @Thread.Proc), @This)
	End If

	If This.Handle = 0 Then
		If This.DoneCondition <> 0 Then CondDestroy This.DoneCondition
		If This.LaunchCondition <> 0 Then CondDestroy This.LaunchCondition
		If This.Mutex <> 0 Then MutexDestroy This.Mutex

		This.DoneCondition = 0
		This.LaunchCondition = 0
		This.Mutex = 0
	End If
End Constructor

Destructor Thread()
	If This.Handle <> 0 Then
		MutexLock This.Mutex
		This.Quit = TRUE
		CondSignal This.LaunchCondition
		MutexUnlock This.Mutex
		ThreadWait This.Handle
	End If

	If This.DoneCondition <> 0 Then CondDestroy This.DoneCondition
	If This.LaunchCondition <> 0 Then CondDestroy This.LaunchCondition
	If This.Mutex <> 0 Then MutexDestroy This.Mutex
End Destructor

Sub Thread.Proc(ByVal Worker As Thread Ptr)
	If Worker = 0 Then Exit Sub

	MutexLock Worker->Mutex

	Do
		While (Worker->LaunchPending = FALSE) AndAlso (Worker->Quit = FALSE)
			CondWait Worker->LaunchCondition, Worker->Mutex
		Wend

		If Worker->Quit Then Exit Do

		Dim As Sub(ByVal p As Any Ptr) CurrentTask = Worker->Task
		MutexUnlock Worker->Mutex

		If CurrentTask <> 0 Then CurrentTask(Worker)

		MutexLock Worker->Mutex
		Worker->LaunchPending = FALSE
		CondSignal Worker->DoneCondition
	Loop

	MutexUnlock Worker->Mutex
End Sub

Sub Thread.Launch()
	MutexLock This.Mutex

	While This.LaunchPending AndAlso (This.Quit = FALSE)
		CondWait This.DoneCondition, This.Mutex
	Wend

	If This.Quit = FALSE Then
		This.LaunchPending = TRUE
		CondSignal This.LaunchCondition
	End If

	MutexUnlock This.Mutex
End Sub

Sub Thread.Wait()
	MutexLock This.Mutex

	While This.LaunchPending AndAlso (This.Quit = FALSE)
		CondWait This.DoneCondition, This.Mutex
	Wend

	MutexUnlock This.Mutex
End Sub

Property Thread.Ready() As Boolean
	Return This.Handle <> 0
End Property

Print "Successive empty user tasks executed by one thread for each:"
Dim As Double t = Timer

For I As Integer = 1 To TASK_COUNT
	Dim As Any Ptr Handle = ThreadCreate(@UserTask)

	If Handle = 0 Then
		Print "Unable to create benchmark thread "; I; "."
		End 1
	End If

	ThreadWait Handle
Next I

t = ElapsedSeconds(t)
Print Using "######.### microseconds per user task"; _
            t * MICROSECONDS_PER_TASK_SCALE
Print

Print "Successive empty user tasks executed by a single reusable thread:"
t = Timer

Dim As Thread Ptr Worker = New Thread

If (Worker = 0) OrElse (Worker->Ready = FALSE) Then
	Print "Unable to create the reusable worker thread."
	Delete Worker
	End 1
End If

Worker->Task = @UserTask

For I As Integer = 1 To TASK_COUNT
	Worker->Launch()
	Worker->Wait()
Next I

Delete Worker
t = ElapsedSeconds(t)
Print Using "######.### microseconds per user task"; _
            t * MICROSECONDS_PER_TASK_SCALE
