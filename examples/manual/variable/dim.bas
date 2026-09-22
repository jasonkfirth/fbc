'' examples/manual/variable/dim.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'DIM'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgDim
'' --------
''
'' Ownership:
''
'' s3 owns the dynamically allocated ZSTRING storage used for the final size
'' demonstration.  It is checked before use and released before the program
'' ends.

Dim a As Byte
Dim b As Short
Dim c As Integer
Dim d As LongInt
Dim au As UByte
Dim bu As UShort
Dim cu As UInteger
Dim du As ULongInt
Dim e As Single
Dim f As Double
Dim g As Integer Ptr
Dim h As Byte Ptr
Dim s1 As String * 10   '' fixed length string
Dim s2 As String        '' variable length string
Dim s3 As ZString Ptr   '' zstring

s1 = "Hello World!"
s2 = "Hello World from FreeBASIC!"
s3 = Allocate( Len( s2 ) + 1 )
If s3 <> 0 Then
	*s3 = s2
Else
	Print "Unable to allocate the ZSTRING value."
End If

Print "Byte: "; Len(a)
Print "Short: "; Len(b)
Print "Integer: "; Len(c)
Print "Longint: "; Len(d)
Print "UByte: "; Len(au)
Print "UShort: "; Len(bu)
Print "UInteger: "; Len(cu)
Print "ULongint: "; Len(du)
Print "Single: "; Len(e)
Print "Double: "; Len(f)
Print "Integer Pointer: "; Len(g)
Print "Byte Pointer: "; Len(h)
Print "Fixed String: "; Len(s1)
Print "Variable String: "; Len(s2)

If s3 <> 0 Then
	Print "ZString: "; Len(*s3)
	Deallocate(s3)
	s3 = 0
End If

'' end of examples/manual/variable/dim.bas
