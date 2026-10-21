'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: incomplete-extern.bas
''
'' Purpose:
''
''     Verify external objects whose C record type is incomplete in this unit.
''
'' Responsibilities:
''
''     - form an address for an external object with a forward type
''     - pass the object by reference without reading its incomplete value
''     - reject operations that need the object's size or layout
''
'' This file intentionally does NOT contain:
''
''     - a definition of the opaque record, which remains owned by C
''
#lang "fb"

Type IncompleteExternObject As IncompleteExternObject_

Extern c2fbIncompleteObject Alias "c2fb_incomplete_object" _
	As IncompleteExternObject

Declare Function c2fbIncompleteObjectAddress CDecl _
	Alias "c2fb_incomplete_object_address" () As IncompleteExternObject Ptr
Declare Function c2fbIncompleteObjectMatches CDecl _
	Alias "c2fb_incomplete_object_matches" _
	(ByRef value As IncompleteExternObject) As Long

Dim objectAddress As IncompleteExternObject Ptr = @c2fbIncompleteObject
If objectAddress = 0 Then End 1
If objectAddress <> c2fbIncompleteObjectAddress() Then End 2
If c2fbIncompleteObjectMatches(c2fbIncompleteObject) <> 1 Then End 3

Type CompleteLater As CompleteLater_
Extern c2fbCompleteLater Alias "c2fb_complete_later" As CompleteLater

Type CompleteLater_
	value As Long
End Type

If c2fbCompleteLater.value <> 73 Then End 4

'' end of incomplete-extern.bas
