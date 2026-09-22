'' examples/manual/proguide/arrays/passing.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Passing Arrays to Procedures'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgPassingArrays
'' --------

Declare Sub splitString(ByVal As String, (Any) As String, ByVal As UByte = Asc(","))


Dim As String s = "Programmer's Guide/Variables and Datatypes/Arrays/Passing Arrays to Procedures"
Dim As String array(Any)

splitString(s, array(), Asc("/"))

Print "STRING TO SPLIT:"
Print s
Print
Print "RESULT ARRAY FROM SPLITTING:"
If UBound(array) < LBound(array) Then
	Print "(no fragments)"
Else
	For i As Integer = LBound(array) To UBound(array)
		Print i, array(i)
	Next i
End If

Sleep


Sub splitString(ByVal source As String, destination(Any) As String, ByVal delimitor As UByte)
	Dim As Integer destination_lower
	Dim As Integer destination_count
	Dim As Integer fragment_count
	Dim As Integer source_index
	Dim As Integer destination_index

	If UBound(destination) < LBound(destination) Then
		destination_lower = 0
		destination_count = 0
	Else
		destination_lower = LBound(destination)
		destination_count = UBound(destination) - destination_lower + 1
	End If

	' Count the exact output capacity before resizing so long source strings do
	' not repeatedly copy the destination array one element at a time.
	fragment_count = 1
	For source_index = 0 To Len(source) - 1
		If source[source_index] = delimitor Then fragment_count += 1
	Next source_index

	ReDim Preserve destination( _
		destination_lower To destination_lower + destination_count + fragment_count - 1)

	destination_index = destination_lower + destination_count
	Do
		Dim As Integer position = InStr(1, source, Chr(delimitor))
		If position = 0 Then
			destination(destination_index) = source
			Exit Do
		End If
		destination(destination_index) = Left(source, position - 1)
		destination_index += 1
		source = Mid(source, position + 1)
	Loop
End Sub

'' end of passing.bas
