'' Project: FreeBASIC C backend regression tests
'' File: c-record-declaration-scope.bas
'' Purpose: Keep deferred record tags and their definitions in the same C scope.
'' Responsibilities: Check static pointer holders and dynamic arrays of local records.
'' This file intentionally does NOT alter existing examples or provoke allocation failure.

#lang "fb"

Function ExerciseStaticHolder() As Integer

	Type LocalItem
		first_value As UByte
		number As Integer
		last_value As UByte
	End Type

	Type LocalHolder
		item As LocalItem Ptr
	End Type

	Static holder As LocalHolder
	Dim item As LocalItem
	Dim result As Integer = 0

	'' Static storage needs the pointer type outside the local declaration.
	'' The later object must complete that same tag rather than shadow it.
	holder.item = @item
	item.first_value = 65
	item.last_value = 66
	item.number = 17
	If holder.item <> @item Then result Or= 1
	If holder.item->first_value <> 65 Then result Or= 2
	If holder.item->last_value <> 66 Then result Or= 4
	If holder.item->number <> 17 Then result Or= 8
	holder.item->number = 19
	If item.number <> 19 Then result Or= 16

	'' Do not retain a pointer after its automatic object leaves this call.
	holder.item = 0
	Return result

End Function

Function ExerciseLocalArray() As Integer

	Type LocalItem
		fixed_text As String * 10
	End Type

	Dim values() As LocalItem
	Dim result As Integer = 0
	ReDim values(0 To 1)
	If values(0).fixed_text[0] <> 32 Then result Or= 1
	If values(1).fixed_text[9] <> 32 Then result Or= 2
	values(0).fixed_text[0] = 65
	values(0).fixed_text[9] = 66
	values(1).fixed_text[0] = 67
	values(1).fixed_text[9] = 68
	ReDim Preserve values(0 To 2)
	If values(0).fixed_text[0] <> 65 Then result Or= 4
	If values(0).fixed_text[9] <> 66 Then result Or= 8
	If values(1).fixed_text[0] <> 67 Then result Or= 16
	If values(1).fixed_text[9] <> 68 Then result Or= 32
	If values(2).fixed_text[0] <> 32 Then result Or= 64
	If values(2).fixed_text[9] <> 32 Then result Or= 128
	Erase values
	Return result

End Function

For cycle As Integer = 1 To 32
	If ExerciseStaticHolder() <> 0 Then End 1
	If ExerciseLocalArray() <> 0 Then End 2
Next

Print "C_RECORD_DECLARATION_SCOPE_PASS"

'' end of c-record-declaration-scope.bas
