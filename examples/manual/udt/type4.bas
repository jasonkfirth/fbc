'' examples/manual/udt/type4.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'TYPE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgType
'' --------

Type Parent
	Private:
		Dim As String nameParent
		Declare Constructor()
		Declare Constructor(ByRef As Parent)
		Type Child
			Dim As String nameChild
			Dim As Parent Ptr ptrParent
			Declare Sub showKinship()
		End Type
		Dim As Child listChild(Any)
		Dim As Integer childCount
	Public:
		Declare Constructor(ByRef _nameParent As String)
		Declare Sub addChild(ByRef _nameChild As String)
		Declare Sub showKinships()
End Type

Constructor Parent(ByRef _nameParent As String)
	This.nameParent = _nameParent
End Constructor

Sub Parent.addChild(ByRef _nameChild As String)
	ReDim Preserve This.listChild(0 To This.childCount)
	This.listChild(This.childCount).nameChild = _nameChild
	This.listChild(This.childCount).ptrParent = @This
	This.childCount += 1
End Sub

Sub Parent.Child.showKinship()
	If This.ptrParent = 0 Then Exit Sub

	Print "'" & This.nameChild & "'" & " is child of " & "'" & This.ptrParent->nameParent & "'"
End Sub

Sub Parent.showKinships()
	For i As Integer = 0 To This.childCount - 1
		This.listChild(i).showKinship()
	Next i
End Sub


Dim As Parent p = Parent("Kennedy")
p.addChild("John Jr.")
p.addChild("Caroline")
p.addChild("Patrick")
p.showKinships()
