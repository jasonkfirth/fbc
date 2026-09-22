'' examples/manual/proguide/mangled_rtti_info.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OBJECT built-in and RTTI info'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgObjectRtti
'' --------
''
'' RTTI layout boundary:
''
'' This helper intentionally inspects the compiler's current RTTI layout for an
'' Object-compatible instance. Object slot 0 leads to RTTI, slot -1 selects its
'' record, slot 2 follows a base-type link, and slot 1 holds the mangled name.
'' This is generated-data inspection, not a public ABI.

Namespace oop
	Type parent Extends Object
	End Type

	Type child Extends parent
	End Type

	Type grandchild Extends child
	End Type
End Namespace

Function mangledTypeNameFromRTTI (ByVal po As Object Ptr, ByVal baseIndex As Integer = 0) As String
	' Function to get any mangled-typename in the inheritance up hierarchy
	' of the type of an instance (address: 'po') compatible with the built-in 'Object'
	'
	' ('baseIndex =  0' to get the mangled-typename of the instance)
	' ('baseIndex = -1' to get the base mangled-typename of the instance, or "" if not existing)
	' ('baseIndex = -2' to get the base.base mangled-typename of the instance, or "" if not existing)
	' (.....)
	'
		Dim As String s
		Dim As ZString Ptr pz
		If po = 0 Then Return s
		'' Object/RTTI slots follow the layout documented in this example header.
		'' FB-LINTER: DISABLE-NEXT-LINE FBL-PTR-019
		Dim As Any Ptr p = CPtr(Any Ptr Ptr Ptr, po)[0][-1]  ' Ptr to RTTI info
		If p = 0 Then Return s
		Do While baseIndex < 0
			'' RTTI slot 2 is the next base-type record in the observed layout.
			'' FB-LINTER: DISABLE-NEXT-LINE FBL525 FBL-PTR-019
			p = CPtr(Any Ptr Ptr, p)[2]                      ' Ptr to Base RTTI info of previous RTTI info
			If p = 0 Then Return s
			baseIndex += 1
		Loop
		'' RTTI slot 1 is the mangled type-name pointer in the observed layout.
		'' FB-LINTER: DISABLE-NEXT-LINE FBL525 FBL-PTR-019
		pz = CPtr(Any Ptr Ptr, p)[1]                         ' Ptr to mangled-typename
		If pz = 0 Then Return s
		s = *pz
		Return s
End Function

Dim As Object Ptr p = New oop.grandchild

If p = 0 Then
	Print "Could not allocate an oop.grandchild instance"
Else
	Print "Mangled typenames list, from RTTI info:"
	Print "  " & mangledTypeNameFromRTTI(p, 0)
	Print "  " & mangledTypeNameFromRTTI(p, -1)
	Print "  " & mangledTypeNameFromRTTI(p, -2)
	Print "  " & mangledTypeNameFromRTTI(p, -3)
	Delete p
	p = 0
End If

Sleep
