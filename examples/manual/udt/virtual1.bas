'' examples/manual/udt/virtual1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'VIRTUAL'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgVirtual
'' --------
''
'' Purpose: Demonstrate object allocation and the topic above.
'' Responsibilities: Check allocation before member access and release owned objects.
'' This file intentionally does NOT change NEW's nullable allocation contract.

'' Example with overriding subroutines


Type Hello Extends Object
	Declare Virtual Sub hi( )
End Type

Type HelloEnglish Extends Hello
	Declare Sub hi( )            '' overriding subroutine
End Type

Type HelloFrench Extends Hello
	Declare Sub hi( )            '' overriding subroutine
End Type

Type HelloGerman Extends Hello
	Declare Sub hi( )            '' overriding subroutine
End Type


Sub Hello.hi( )
	Print "hi!"
End Sub

Sub HelloEnglish.hi( )           '' overriding subroutine
	Print "hello!"
End Sub

Sub HelloFrench.hi( )            '' overriding subroutine
	Print "Salut!"
End Sub

Sub HelloGerman.hi( )            '' overriding subroutine
	Print "Hallo!"
End Sub


Randomize( Timer( ) )

Dim As Hello Ptr h

For i As Integer = 0 To 9
	Select Case( Int( Rnd( ) * 4 ) + 1 )
	Case 1
		h = New HelloEnglish
	Case 2
		h = New HelloFrench
	Case 3
		h = New HelloGerman
	Case Else
		h = New Hello
	End Select

	'' NEW returns zero if allocation fails.
	If h = 0 Then End 1
	h->hi( )
	Delete h
Next

Sleep

'' end of virtual1.bas
