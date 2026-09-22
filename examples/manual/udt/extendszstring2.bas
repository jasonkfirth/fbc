'' examples/manual/udt/extendszstring2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'EXTENDS ZSTRING'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgExtendsZstring
'' --------

#Include Once "fberror.bi"

'' Resource ownership:
''
'' vZstring owns one CAllocate buffer. setText allocates and copies a larger
'' replacement before releasing the old buffer, so a failed growth operation
'' or a self-aliasing assignment preserves the existing string.

Type vZstring Extends ZString
  Public:
	Declare Constructor (ByVal pz As Const ZString Ptr = 0)
	Declare Operator Cast () ByRef As ZString
	Declare Operator Let (ByVal pz As Const ZString Ptr)
	Declare Operator [] (ByVal index As Integer) ByRef As UByte
	Declare Destructor ()
  Private:
	Dim As ZString Ptr p
	Dim As UInteger l
	Declare Sub setText (ByVal pz As Const ZString Ptr)
End Type

Constructor vZstring (ByVal pz As Const ZString Ptr = 0)
  This.setText(pz)
End Constructor

Operator vZstring.Cast () ByRef As ZString
  Return *This.p
End Operator

Operator vZstring.Let (ByVal pz As Const ZString Ptr)
  This.setText(pz)
End Operator

Operator vZstring.[] (ByVal index As Integer) ByRef As UByte
  Return This.p[index]
End Operator

Destructor vZstring ()
  If This.p <> 0 Then
	Deallocate(This.p)
	This.p = 0
	This.l = 0
  End If
End Destructor

Sub vZstring.setText (ByVal pz As Const ZString Ptr)
  Dim As UInteger newLength
  Dim As ZString Ptr replacement

  If pz <> 0 Then newLength = Len(*pz)

  If This.p = 0 OrElse This.l < newLength Then
	replacement = CAllocate(newLength + 1, SizeOf(ZString))
	If replacement = 0 Then Error FB.FB_RTERROR_OUTOFMEM
	If pz <> 0 Then
	  *replacement = *pz
	Else
	  *replacement = ""
	End If
	If This.p <> 0 Then Deallocate(This.p)
	This.p = replacement
	This.l = newLength
  ElseIf pz <> 0 Then
	*This.p = *pz
  Else
	*This.p = ""
  End If
End Sub

Operator Len (ByRef v As vZstring) As Integer
  Return Len(Type<String>(v))        '' found nothing better than this
End Operator                         ''     (or: 'Return Len(Str(v))')

Dim As vZstring v = "FreeBASIC"
Print "'" & v & "'", Len(v)

Dim As ZString * 256 z
z = *StrPtr(v)                       '' 'error 24: Invalid data types' without 'Extends Zstring'
Print "'" & z & "'", Len(z)

v &= Space(2)
Print "'" & v & "'", Len(v)
RSet v, "FreeBASIC"                  '' 'error 24: Invalid data types' without 'Extends Zstring'
Print "'" & v & "'", Len(v)          ''     ('Cast' must return a modifiable reference)

Dim As vZstring leftJustified = Trim(v) & "  "
Dim As vZstring rightJustified = "  " & Trim(v)
Select Case v                        '' 'error 24: Invalid data types' without 'Extends Zstring'
Case leftJustified
  Print "Left justified"
Case rightJustified
  Print "Right justified"
End Select

v[0] = Asc("-")
Print "'" & v & "'", Len(v)

Print "'" & Right(v, 5) & "'"        '' since fbc 1.09.0, 'Right' supports types with 'Extends Zstring'
'Print "'" & Right(Str(v), 5) & "'"  '' before fbc 1.09.0, use this workaround (or: 'Right(Type<String>(v), 5)')

Sleep
