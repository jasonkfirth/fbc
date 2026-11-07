'' Project: FreeBASIC C backend regression tests
'' File: c-record-prototype-views.bas
'' Purpose: Keep forward prototype views separate from completed record accesses.
'' Responsibilities: Check method declarations, packed receivers and indirect calls.
'' This file intentionally does NOT change existing examples or depend on C padding bytes.

#lang "fb"

Type PhaseRecord

	first_value As UByte
	number As Integer
	last_value As UByte
	Declare Constructor(ByVal initial_value As Integer = 17)
	Declare Sub StoreNumber(ByVal new_value As Integer)
	Declare Function ReadNumber() As Integer

End Type

Constructor PhaseRecord(ByVal initial_value As Integer)

	first_value = 65
	number = initial_value
	last_value = 66

End Constructor

Sub PhaseRecord.StoreNumber(ByVal new_value As Integer)

	number = new_value

End Sub

Function PhaseRecord.ReadNumber() As Integer

	Return number

End Function

Function ReadThroughPointer(ByVal item As PhaseRecord Ptr) As Integer

	Return item->ReadNumber()

End Function

'' Emit the record methods before the holder's generated constructor needs
'' a by-value member layout. This exercises the prototype-only declaration.
Type PackedHolder Field = 1

	prefix As UByte
	item As PhaseRecord

End Type

Function CheckHolder(ByVal cycle As Integer) As Integer

	Dim storage As Any Ptr = Allocate(SizeOf(PackedHolder))
	If storage = 0 Then Return 128
	Dim holder As PackedHolder Ptr = New (storage) PackedHolder()
	Dim item As PhaseRecord Ptr = @holder->item
	Dim read_callback As Function(ByVal As PhaseRecord Ptr) As Integer = @ReadThroughPointer
	Dim result As Integer = 0

	'' The allocator supplies natural base alignment. The packed prefix puts
	'' the method receiver one byte beyond that boundary on each target.
	If CUInt(item) Mod SizeOf(Integer) <> 1 Then result Or= 1
	If item->ReadNumber() <> 17 Then result Or= 2
	holder->prefix = 87
	item->StoreNumber(17 + cycle)
	If read_callback(item) <> 17 + cycle Then result Or= 4
	If holder->item.number <> 17 + cycle Then result Or= 8
	If item->first_value <> 65 OrElse item->last_value <> 66 Then result Or= 16
	If holder->prefix <> 87 Then result Or= 32

	'' These records own only scalar fields. Release the original allocation
	'' after the placement-constructed value's final use.
	Deallocate(storage)
	Return result

End Function

For cycle As Integer = 1 To 32
	If CheckHolder(cycle) <> 0 Then End 1
Next

Print "C_RECORD_PROTOTYPE_VIEWS_PASS"

'' end of c-record-prototype-views.bas
