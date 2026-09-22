'' examples/manual/proguide/newdelete/operators.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'New and Delete'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgNewDelete
'' --------
''
'' Resource ownership:
''
'' The overloaded New routines return a null pointer on allocation failure.
'' Ordinary New and New[] results are released with Delete and Delete[].
'' The byte buffer below provides exactly two UDT-sized placement slots; the
'' placement objects are destructed explicitly and never passed to Delete.

Declare Sub printArray (ByRef label As String = "", array() As String)


Type UDT
	Declare Constructor ()
	Declare Destructor ()
	Declare Operator New (ByVal size As UInteger) As Any Ptr    ' Operator New Overload
	Declare Operator New[] (ByVal size As UInteger) As Any Ptr  ' Operator New[] Overload
	Declare Operator Delete (ByVal buf As Any Ptr)              ' Operator Delete Overload
	Declare Operator Delete[] (ByVal buf As Any Ptr)            ' Operator Delete[] Overload
	Dim As String array (1 To 4)
End Type

Constructor UDT ()
	Static As Integer n
	Print "    Constructor"
	printArray("        init: @" & @This & " (descriptors) -> ", This.array())
	For i As Integer = 1 To 4
		This.array(i) = Chr(Asc("a") + n + i - 1)
	Next i
	printArray(" => ", This.array())
	Print
	n += 4
End Constructor

Destructor UDT ()
	Print "    Destructor"
	printArray("        erase: @" & @This & " (descriptors) -> ", This.array())
	For i As Integer = 1 To 4
		This.array(i) = ""
	Next i
	printArray(" => ", This.array())
	Print
End Destructor

Operator UDT.New (ByVal size As UInteger) As Any Ptr
	Print "    Operator New Overload"
	Dim As Any Ptr p = Allocate(size)                   ' Memory allocation (with passed size)
	If p = 0 Then
		Print "        memory allocation failed"
	Else
		Print "        memory allocation: ";
		Print size & " Bytes from @" & p
	End If
	Return p                                            ' Returning memory pointer
End Operator

Operator UDT.New[] (ByVal size As UInteger) As Any Ptr
	Print "    Operator New[] Overload"
	Dim As Any Ptr p = Allocate(size)                   ' Memory allocation (with passed size)
	If p = 0 Then
		Print "        memory allocation failed"
	Else
		Print "        memory allocation: ";
		Print size & " Bytes from @" & p
	End If
	Return p                                            ' Returning memory pointer
End Operator

Operator UDT.Delete (ByVal buf As Any Ptr)
	Dim As ULongInt bufAddress = CULngInt(buf)
	Print "    Operator Delete Overload"
	If buf <> 0 Then
		Deallocate(buf)                                 ' Memory deallocation (with passed pointer)
		buf = 0
		Print "        memory deallocation: ";
		Print "for @" & bufAddress
	End If
End Operator

Operator UDT.Delete[] (ByVal buf As Any Ptr)
	Dim As ULongInt bufAddress = CULngInt(buf)
	Print "    Operator Delete[] Overload"
	If buf <> 0 Then
		Deallocate(buf)                                 ' Memory deallocation (with passed pointer)
		buf = 0
		Print "        memory deallocation: ";
		Print "for @" & bufAddress
	End If
End Operator


Print "Operator New Expression"
Dim As UDT Ptr pu1 = New UDT         ' Operator New Expression
Print "Operator Delete Statement"
If pu1 <> 0 Then
	Delete pu1                         ' Operator Delete Statement
	pu1 = 0
Else
	Print "    Object allocation failed"
End If
Sleep

Print
Print "Operator New[] Expression"
Dim As UDT Ptr pu2 = New UDT[2]      ' Operator New[] Expression
Print "Operator Delete[] Statement"
If pu2 <> 0 Then
	Delete[] pu2                       ' Operator Delete[] Statement
	pu2 = 0
Else
	Print "    Object array allocation failed"
End If
Sleep

Dim As Byte buffer(1 To SizeOf(UDT) * 2)
Dim As Any Ptr p = @buffer(1)

Print
Print "Operator Placement New"
'' The first raw UDT slot is manually destructed before this storage is reused.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-PTR-023
Dim As UDT Ptr pu3 = New(p) UDT      ' Operator Placement New
Print "User call of Destructor"
pu3->Destructor()                    ' User Call of Destructor
pu3 = 0
Sleep

Print
Print "Operator Placement New[]"
'' The two raw UDT slots are manually destructed in the bounded loop below.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-PTR-023
Dim As UDT Ptr pu4 = New(p) UDT[2]   ' Operator Placement New[]
For i As Integer = 0 To 1
	Print "User Call of Destructor"
	'' buffer contains exactly two UDT-sized placement slots, indexed from 0 to 1.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL525
	pu4[i].Destructor()              ' User Call of Destructor
Next i
pu4 = 0
Sleep


Sub printArray (ByRef label As String = "", array() As String)
	If UBound(array) < LBound(array) Then
		Print label & "{}";
		Exit Sub
	End If
	Dim As Integer firstIndex = LBound(array)
	Dim As Integer lastIndex = UBound(array)
	Print label & "{";
	If firstIndex <= lastIndex Then
		For i As Integer = firstIndex To lastIndex
			Print """" & array(i) & """";
			If i < lastIndex Then
				Print ",";
			End If
		Next I
	End If
	Print "}";
End Sub
