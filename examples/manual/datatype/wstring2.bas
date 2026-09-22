'' examples/manual/datatype/wstring2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'WSTRING'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgWstring
'' --------
''
'' Ownership:
''
'' str2 owns the fixed-size WSTRING buffer allocated below.  Its storage is
'' released after the example has finished reading the displayed string.

Dim As WString Ptr str2
str2 = Allocate( 13 * Len(WString) )
If str2 <> 0 Then
	*str2 = "hello, world"
	Print *str2
	Print Len(*str2)      'returns 12, the length of the string it points to
	Deallocate(str2)
	str2 = 0
Else
	Print "Unable to allocate the WSTRING buffer."
End If

'' end of examples/manual/datatype/wstring2.bas
