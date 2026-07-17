'' examples/manual/memory/deallocate3.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'DEALLOCATE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgDeallocate
'' --------

Function createInteger() As Integer Ptr
   '' Allocation failure is returned as a null pointer and checked by the caller.
   '' FB-LINTER: DISABLE-NEXT-LINE FBL800
   Dim result As Integer Ptr = Allocate( Len( Integer ) )
   Return result                                         '' return pointer to newly
End Function                                             '' allocated memory

Sub destroyInteger( ByRef someIntegerPtr As Integer Ptr )
   Deallocate( someIntegerPtr )                          '' free memory back to system
   someIntegerPtr = 0                                    '' null original pointer
End Sub

Sub DeallocateExample3()
   Dim As Integer Ptr integerPtr = createInteger()       '' initialize pointer to
															 '' new memory address

   If integerPtr = 0 Then
      Print "Unable to allocate the integer."
      Exit Sub
   End If

   *integerPtr = 420                                     '' use pointer
   Print *integerPtr

   destroyInteger( integerPtr )                          '' pass pointer by reference
   Assert( integerPtr = 0 )                              '' pointer should now be null
End Sub

   DeallocateExample3()
   End 0
