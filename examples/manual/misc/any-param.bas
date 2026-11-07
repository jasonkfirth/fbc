'' examples/manual/misc/any-param.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'ANY'
'' Purpose: Show explicit type reinterpretation through an ANY parameter.
'' Responsibilities: Read the four-byte SINGLE representation as a LONG.
'' This file intentionally does NOT read native-width storage beyond the value.
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgAny
'' --------

'Example of ANY disabling the variable type checking
Declare Sub echo (ByRef a As Any) '' ANY disables the checking for the type of data passed to the function

Dim x As Single
x = -15
echo x                  '' Passing a single to a function that expects a 32-bit integer. The compiler does not complain!!
Sleep

Sub echo (ByRef a As Long)
  Print Hex(a)
End Sub

'' end of any-param.bas
