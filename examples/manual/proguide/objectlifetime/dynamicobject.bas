'' examples/manual/proguide/objectlifetime/dynamicobject.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Dynamic Object and Data Lifetime'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgObjectLifetime
'' --------

#Include Once "fberror.bi"

'' Resource ownership:
''
'' complexUDT owns one ZSTRING allocation.  setText allocates replacement
'' storage before releasing the old string, so assignment and manual
'' reconstruction preserve the current value when allocation fails.

Type complexUDT
	Public:
		Declare Constructor ()
		Declare Constructor (ByVal p As ZString Ptr)
		Declare Operator Let (ByVal p As ZString Ptr)
		Declare Operator Cast () As String
		Declare Property info () As String ' allocation address, allocation size, string length
	Declare Destructor ()
	Private:
		Dim As ZString Ptr pz
		Declare Sub setText (ByVal p As ZString Ptr)
End Type

Declare Sub prntInfo_printString (ByRef u As complexUDT)


Print "'Dim Byref As complexUDT ref = *New complexUDT(""Beginning"")':"
Dim As complexUDT Ptr refPointer = New complexUDT("Beginning")
If refPointer = 0 Then
	Print "Could not allocate complexUDT"
	Sleep
	End
End If
Dim ByRef As complexUDT ref = *refPointer
prntInfo_printString(ref)

Print "'ref = """"':"
ref = ""
prntInfo_printString(ref)

Print "'ref = ""FreeBASIC""':"
ref = "FreeBASIC"
prntInfo_printString(ref)

Print "'ref = ""Programmer's Guide / Declarations / Dynamic Object and Data Lifetime""':"
ref = "Programmer's Guide / Declarations / Dynamic Object and Data Lifetime"
prntInfo_printString(ref)

Print "'ref.Destructor()':"
ref.Destructor()
prntInfo_printString(ref)

Print "'ref.Constructor()':"
ref.Constructor()
prntInfo_printString(ref)

Print "'ref.Constructor(""End"")':"
ref.Constructor("End")
prntInfo_printString(ref)

Print "'Delete @ref':"
Delete refPointer
refPointer = 0

Sleep


Constructor complexUDT ()
	Print "    complexUDT.Constructor()"
	This.setText(0)
End Constructor

Constructor complexUDT (ByVal p As ZString Ptr)
	Print "    complexUDT.Constructor(Byval As Zstring Ptr)"
	This.setText(p)
End Constructor

Operator complexUDT.Let (ByVal p As ZString Ptr)
	Print "    complexUDT.Let(Byval As Zstring Ptr)"
	This.setText(p)
End Operator

Operator complexUDT.Cast () As String
	If This.pz = 0 Then Return Chr(34) & Chr(34)
	Return """" & *This.pz & """"
End Operator

Property complexUDT.info () As String
	If This.pz = 0 Then Return "released"
	Return "&h" & Hex(This.pz, SizeOf(Any Ptr) * 2) & ", " & _     ' allocation address
			Len(*This.pz) + Sgn(Cast(Integer, This.pz)) & ", " & _ ' allocation size
			Len(*This.pz)                                          ' string length
End Property

Destructor complexUDT ()
	Print "    complexUDT.Destructor()"
	If This.pz <> 0 Then
		Deallocate(This.pz)
		This.pz = 0
	End If
End Destructor

Sub complexUDT.setText (ByVal p As ZString Ptr)
	Dim As UInteger textLength
	Dim As ZString Ptr replacement

	If p <> 0 Then
		textLength = Len(*p)
		replacement = Allocate(textLength + 1)
		If replacement = 0 Then Error FB.FB_RTERROR_OUTOFMEM
		*replacement = *p
	Else
		replacement = Allocate(1)
		If replacement = 0 Then Error FB.FB_RTERROR_OUTOFMEM
		*replacement = ""
	End If

	If This.pz <> 0 Then Deallocate(This.pz)
	This.pz = replacement
End Sub


Sub prntInfo_printString (ByRef u As complexUDT)
	Print "        " & u.info
	Print "        " & u
	Print
End Sub
