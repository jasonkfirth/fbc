'' examples/manual/proguide/static-lib/varZstring.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Static Libraries'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgStaticLibraries
'' --------

'' library module: 'varZstring.bas'

'' Resource ownership:
''
'' Each instance owns _p when it is non-NULL. Assignment allocates replacement
'' storage before releasing the old buffer so an allocation failure leaves the
'' existing value available to the caller.

#include "varZstring.bi"

Constructor varZstring (ByRef z As Const ZString)
	Dim As UInteger required = CUInt(Len(z)) + 1

	This._p = 0
	This._allocated = 0
	If required = 0 Then Exit Constructor

	This._p = CAllocate(required, SizeOf(ZString))
	If This._p = 0 Then Exit Constructor

	This._allocated = required
	*This._p = z
End Constructor

Operator varZstring.Cast () ByRef As ZString
	Static As ZString * 1 empty_string = ""
	If This._p = 0 Then Return empty_string
	Return *This._p
End Operator

Operator varZstring.Let (ByRef z As Const ZString)
	Dim As UInteger required = CUInt(Len(z)) + 1

	If required = 0 Then Exit Operator
	If This._p = 0 OrElse This._allocated < required Then
		Dim As ZString Ptr replacement = CAllocate(required, SizeOf(ZString))
		If replacement = 0 Then Exit Operator

		If This._p <> 0 Then Deallocate(This._p)
		This._p = replacement
		This._allocated = required
	End If

	*This._p = z
End Operator

Property varZstring.allocated () As Integer
	Return This._allocated
End Property

Destructor varZstring ()
	If This._p <> 0 Then Deallocate(This._p)
	This._p = 0
	This._allocated = 0
End Destructor

Operator Len (ByRef v As varZstring) As Integer
	Return Len(Type<String>(v))  '' found nothing better than this
End Operator                     ''     (or: 'Return Len(Str(v))')

'' end of varZstring.bas
