'' examples/manual/udt/copyconstructor.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'CONSTRUCTOR'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgConstructor
'' --------

Type UDT
  Dim As String Ptr p                     ''pointer to string
  Declare Constructor ()                  ''default constructor
  Declare Constructor (ByRef rhs As UDT)  ''copy constructor
  Declare Destructor ()                   ''destructor
End Type

Constructor UDT ()
  This.p = CAllocate(1, SizeOf(String))
  If This.p = 0 Then Exit Constructor
End Constructor

Constructor UDT (ByRef rhs As UDT)
  If rhs.p = 0 Then Exit Constructor
  This.p = CAllocate(1, SizeOf(String))
  If This.p = 0 Then Exit Constructor
  *This.p = *rhs.p
End Constructor

Destructor UDT ()
  If This.p <> 0 Then
    *This.p = ""
    Deallocate This.p
    This.p = 0
  End If
End Destructor


Dim As UDT u0
If u0.p = 0 Then
  Print "Memory allocation failed"
Else
  *u0.p = "copy constructor exists"
  Dim As UDT u = u0
  If u.p = 0 Then
    Print "Memory allocation failed"
  Else
    *u0.p = ""  ''to check the independance of the result copy with the object copied
    Print *u.p
  End If
End If
Sleep
