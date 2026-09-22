'' examples/manual/casting/cast2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'CAST'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgCast
'' --------
''
'' Ownership:
''
'' pd is allocated with New and released with Delete.  pz is allocated with
'' Callocate and released with Deallocate.  Either allocation can fail, so it
'' is checked before the example dereferences the resulting pointer.

'' macro sizeofDerefPtr(): returns the size of the dereferenced pointer
#define sizeofDerefPtr(ptrToDeref) SizeOf(*Cast(TypeOf(ptrToDeref), 0))

'' macro typeofDerefPtr(): returns the type of the dereferenced pointer
#define typeofDerefPtr(ptrToDeref) TypeOf(*Cast(TypeOf(ptrToDeref), 0))


' Allocate dynamically memory for a Double by New
Dim As Double Ptr pd
pd = New typeofDerefPtr(pd)
If pd <> 0 Then
	*pd = 3.14159
	Print *pd
Else
	Print "Unable to allocate the Double."
End If

' Allocate dynamically memory for a Zstring*10 by Callocate
Dim As ZString Ptr pz
pz = CAllocate(10, sizeofDerefPtr(pz))
If pz <> 0 Then
	*pz = "FreeBASIC"
	Print *pz
Else
	Print "Unable to allocate the Zstring."
End If

Sleep
If pd <> 0 Then
	Delete pd
	pd = 0
End If
If pz <> 0 Then
	Deallocate(pz)
	pz = 0
End If

'' end of examples/manual/casting/cast2.bas
