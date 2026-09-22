'' examples/manual/operator/let.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Operator Let (Assign)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpLet
'' --------

#Include Once "fberror.bi"

'' Ownership:
'' Each UDT owns zp after its constructor succeeds. Assignment obtains the
'' replacement string before releasing the previous owned storage.

Type UDT
  Public:
	Declare Constructor (ByVal zp As Const ZString Ptr)  ''constructor with string initializer
	Declare Operator Let (ByRef rhs As UDT)              ''operator Let (assignment)
	Declare Function getString () As String              ''function to get string
	Declare Destructor ()                                ''destructor
  Private:
	Dim zp As ZString Ptr                                ''private pointer to avoid direct access
End Type

Constructor UDT (ByVal zp As Const ZString Ptr)
  This.zp = Callocate(Len(*zp) + 1)
  If This.zp = 0 Then Error FB.FB_RTERROR_OUTOFMEM
  *This.zp = *zp
End Constructor

Operator UDT.Let (ByRef rhs As UDT)
  Dim replacement As ZString Ptr

  If @This <> @rhs Then  '' check for self-assignment to avoid object destruction
	replacement = Callocate(Len(*rhs.zp) + 1)
	If replacement = 0 Then Error FB.FB_RTERROR_OUTOFMEM
	*replacement = *rhs.zp
	Deallocate(This.zp)
	This.zp = replacement
  End If
End Operator

Function UDT.getString () As String
  Return *This.zp
End Function

Destructor UDT ()
  Deallocate(This.zp)
End Destructor


Dim u As UDT = UDT("")
u = Type<UDT>("Thanks to the overloading operator Let (assign)")
Print u.getString
Sleep
