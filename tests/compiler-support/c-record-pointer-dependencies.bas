'' Project: FreeBASIC C backend regression tests
'' File: c-record-pointer-dependencies.bas
'' Purpose: Keep pointer-only dependencies opaque until their layout is needed.
'' Responsibilities: Check recursive records, later completion, alignment views
''     and ordinary C argument, callback and return contracts.
'' This file intentionally does NOT change the layout of any BASIC record.

#lang "fb"

Type OpaqueDependency
	prefix As UByte
	value As Double
End Type

Type PointerHeader
	opaque As OpaqueDependency Ptr
	callback As Sub Cdecl(ByVal value As OpaqueDependency Ptr)
End Type

Type CompletedLater
	prefix As UByte
	value As Long
End Type

Type PointerBeforeValue
	value As CompletedLater Ptr
End Type

Type ValueCallbackHolder
	transform As Function Cdecl(ByVal value As CompletedLater) As CompletedLater
End Type

Type RightRecord As RightRecord_
Type LeftRecord
	value As Long
	other As RightRecord Ptr
End Type
Type RightRecord_
	other As LeftRecord Ptr
	value As Long
End Type

Type PackedFrame Field = 1
	prefix As UByte
	value As CompletedLater
End Type

Declare Function observe_header Cdecl Alias "fb_test_record_pointer_accept"(ByRef value As PointerHeader) As Long
Declare Sub prepare_header Cdecl Alias "fb_test_record_pointer_prepare"(ByRef value As PointerHeader)
Declare Function change_value Cdecl Alias "fb_test_record_pointer_change"(ByVal value As CompletedLater) As CompletedLater

'' C initializes this header before it is observed. BASIC therefore needs
'' its pointer storage layout, without accessing the opaque pointed-to record.
Dim header As PointerHeader = Any
prepare_header(header)
If observe_header(header) <> 1 Then End 11

Dim first As LeftRecord
Dim second As RightRecord
first.other = @second
second.other = @first
first.value = 41
second.value = 43
If first.other->other <> @first OrElse second.other->value <> 41 Then End 12
If first.other->value <> 43 Then End 13

Dim earlier As PointerBeforeValue
Dim completed As CompletedLater = (17, 23)
earlier.value = @completed
completed = change_value(completed)
If earlier.value->prefix <> 18 OrElse completed.value <> 52 Then End 14

Dim callbacks As ValueCallbackHolder
callbacks.transform = @change_value
completed = callbacks.transform(completed)
If completed.prefix <> 19 OrElse completed.value <> 81 Then End 15

Dim records(0 To 1) As CompletedLater
Dim cursor As CompletedLater Ptr = @records(0)
cursor += 1
cursor->prefix = 31
cursor->value = 37
If records(1).prefix <> 31 OrElse records(1).value <> 37 Then End 16

Dim packed As PackedFrame
Dim unaligned_value As CompletedLater Ptr = @packed.value
unaligned_value->prefix = 47
unaligned_value->value = 53
If packed.value.prefix <> 47 OrElse packed.value.value <> 53 Then End 17

Scope
	Type LocalHeader
		value As CompletedLater Ptr
	End Type
	Dim local_value As LocalHeader
	If local_value.value <> 0 Then End 18
End Scope

Scope
	Type LocalHeader
		prefix As UByte
		value As Long
	End Type
	Dim local_value As LocalHeader = (59, 61)
	If local_value.prefix <> 59 OrElse local_value.value <> 61 Then End 19
End Scope

Print "C_RECORD_POINTER_DEPENDENCIES_PASS"

'' end of c-record-pointer-dependencies.bas
