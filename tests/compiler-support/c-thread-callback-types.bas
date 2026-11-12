'' Project: FreeBASIC compiler regression tests
'' File: c-thread-callback-types.bas
'' Purpose: Exercise typed static workers through the native thread callback ABI.
'' Responsibilities: Check typed calls, forward methods, identity and evaluation.
'' This file intentionally does NOT share writable data between live workers.
#lang "fb"

Type WorkerState
    value As Integer
End Type

Type PackedOwner Field = 1
    prefix As UByte
    state As WorkerState
End Type

Private Sub typedWorker(ByVal state As WorkerState Ptr)
    If state <> 0 Then state->value += 7
End Sub

'' This factory is compiled but never called. An explicit integer narrowing
'' changes the callback value and must not be stripped to recover @typedWorker.
Function uninvokedNarrowFactory(ByVal context As Any Ptr) As Any Ptr
    Return ThreadCreate( _
        CPtr(Any Ptr, CUInt(CUByte(CUInt(@typedWorker)))), context)
End Function

Type ForwardWorker
    handle As Any Ptr
    Declare Constructor(ByVal state As WorkerState Ptr)
    Declare Static Sub run(ByVal state As WorkerState Ptr)
End Type

Constructor ForwardWorker(ByVal state As WorkerState Ptr)
    handle = ThreadCreate(Cast(Sub(ByVal As Any Ptr), @ForwardWorker.run), state)
End Constructor

Sub ForwardWorker.run(ByVal state As WorkerState Ptr)
    If state <> 0 Then state->value += 11
End Sub

Type GenericCallback As Sub(ByVal As Any Ptr)
Dim Shared As Integer callbackReads, contextReads
Dim Shared As WorkerState genericState

Private Sub genericWorker(ByVal context As Any Ptr)
    If context <> 0 Then Cast(WorkerState Ptr, context)->value += 13
End Sub

Private Function readCallback() As GenericCallback
    callbackReads += 1
    Return @genericWorker
End Function

Private Function readContext() As Any Ptr
    contextReads += 1
    Return @genericState
End Function

Dim As WorkerState state
Dim As UInteger workerAddress = CUInt(@typedWorker)
Dim As UByte workerByte = CUByte(CUInt(@typedWorker))
Dim As Byte workerSignedByte = CByte(CUInt(@typedWorker))
Dim As UShort workerWord = CUShort(CUInt(@typedWorker))
Dim As ULong workerDword = CULng(CUInt(@typedWorker))
If workerByte <> (workerAddress And &hFFu) Then End 10
If workerSignedByte <> CByte(workerAddress And &hFFu) Then End 11
If CBool(CUInt(@typedWorker)) <> True Then End 12
If workerWord <> (workerAddress And &hFFFFu) Then End 13
If CUInt(workerDword) <> (workerAddress And &hFFFFFFFFu) Then End 14
Dim As Sub(ByVal As WorkerState Ptr) originalWorker = @typedWorker
Dim As Any Ptr handle = ThreadCreate( _
    Cast(Sub(ByVal As Any Ptr), @typedWorker), @state)
If handle = 0 Then End 1
'' Each worker owns its record until ThreadWait joins it. Main reads only
'' after that synchronization, so the checks do not depend on polling races.
ThreadWait(handle)
If state.value <> 7 Then End 2
If originalWorker <> @typedWorker Then End 3

'' Consecutive packed owners put the same payload at different byte
'' alignments. The adapter must retain the worker's unaligned pointer view.
Dim As PackedOwner packed(0 To 7)
For index As Integer = 0 To 7
    handle = ThreadCreate(Cast(Sub(ByVal As Any Ptr), @typedWorker), @packed(index).state)
    If handle = 0 Then End 8
    ThreadWait(handle)
    If packed(index).state.value <> 7 OrElse packed(index).prefix <> 0 Then End 9
Next index

Dim As ForwardWorker forwardEntry = ForwardWorker(@state)
If forwardEntry.handle = 0 Then End 4
ThreadWait(forwardEntry.handle)
If state.value <> 18 Then End 5

handle = ThreadCreate(readCallback(), readContext())
If handle = 0 Then End 6
ThreadWait(handle)
If genericState.value <> 13 OrElse callbackReads <> 1 OrElse contextReads <> 1 Then End 7

Print "C_THREAD_CALLBACK_TYPES_PASS"

'' end of c-thread-callback-types.bas
