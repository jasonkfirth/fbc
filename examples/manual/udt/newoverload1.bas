'' examples/manual/udt/newoverload1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Operator New Overload'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpNewOverload
'' --------

Const ALIGN = 256

Type UDT
  Dim As Byte a(0 To 10 * 1024 * 1024 - 1) '' 10 megabyte fixed array
  Declare Operator New (ByVal size As UInteger) As Any Ptr
  Declare Operator Delete (ByVal buffer As Any Ptr)
  Declare Constructor ()
  Declare Destructor ()
End Type

Operator UDT.New (ByVal size As UInteger) As Any Ptr
  Print "  Overloaded New operator, with parameter size = &h" & Hex(size)
  Dim pOrig As UByte Ptr = CAllocate(ALIGN-1 + SizeOf(UDT Ptr) + size)
  If pOrig = 0 Then
    Print "  allocation failed"
    Operator = 0
    Exit Operator
  End If

  Dim pMin As UByte Ptr = pOrig + SizeOf(UDT Ptr)
  Dim p As UByte Ptr = pMin + ALIGN-1 - (CUInt(pMin + ALIGN-1) Mod ALIGN)

  '' The allocation reserves one pointer-sized header before the aligned UDT.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL525 FBL-PTR-019
  Cast(Any Ptr Ptr, p)[-1] = pOrig
  Operator = p
  Print "  real pointer = &h" & Hex(pOrig), "return pointer = &h" & Hex(p)
End Operator

Operator UDT.Delete (ByVal buffer As Any Ptr)
  Print "  Overloaded Delete operator, with parameter buffer = &h" & Hex(buffer)
  If buffer = 0 Then Exit Operator

  '' This retrieves the header reserved by UDT.New immediately before buffer.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL525 FBL-PTR-019
  Dim pOrig As Any Ptr = Cast(Any Ptr Ptr, buffer)[-1]
  Dim pOrigAddress As ULongInt = CULngInt(pOrig)

  If pOrig <> 0 Then
    Deallocate(pOrig)
    pOrig = 0
  End If
  buffer = 0
  Print "  real pointer = &h" & Hex(pOrigAddress)
End Operator

Constructor UDT ()
  Print "  Constructor, @This = &h" & Hex(@This)
End Constructor

Destructor UDT ()
  Print "  Destructor, @This = &h" & Hex(@This)
End Destructor

Print "'Dim As UDT Ptr p = New UDT'"
Dim As UDT Ptr p = New UDT

If p <> 0 Then
  Print "  p = &h" & Hex(p)

  Print "'Delete p'"
  Delete p
  p = 0
Else
  Print "  allocation failed"
End If

Sleep
