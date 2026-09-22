'' examples/manual/datatype/zstring2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'ZSTRING'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgZstring
'' --------
''
'' Ownership:
''
'' str2 owns the fixed-size ZSTRING buffer allocated below.  The buffer has
'' space for the 12 displayed characters and its terminating null byte.

Dim As ZString Ptr str2
str2 = Allocate( 13 )
If str2 <> 0 Then
	*str2 = "hello, world"
	Print *str2
	Print Len(*str2)     'returns 12, the size of the string it contains
	Deallocate(str2)
	str2 = 0
Else
	Print "Unable to allocate the ZSTRING buffer."
End If

'' end of examples/manual/datatype/zstring2.bas
