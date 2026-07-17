'' examples/manual/procs/naked1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'NAKED'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgNaked
'' --------

#If defined(__FB_X86__) And Not defined(__FB_64BIT__)

'' Naked cdecl function. Parameters are pushed onto the stack in reverse order.
Function subtract_c Naked cdecl _
	( _
		ByVal a As Long, _
		ByVal b As Long _
	) As Long

	Asm
		mov eax, dword Ptr [esp+4]  '' eax = a
		Sub eax, dword Ptr [esp+8]  '' eax -= b
		ret                         '' return result in eax
	End Asm

End Function

Print subtract_c( 5, 1 ) '' 5 - 1

''---------------------------------------------------------------------------------------------------------------------

'' Naked stdcall function. Parameters are pushed in reverse order, and the
'' called procedure removes them by passing their byte count to RET.
Function subtract_s Naked stdcall _
	( _
		ByVal a As Long, _
		ByVal b As Long _
	) As Long

	Asm
		mov eax, dword Ptr [esp+4]  '' eax = a
		Sub eax, dword Ptr [esp+8]  '' eax -= b
		ret 8                       '' return result in eax and 8 bytes (2 integers) to release
	End Asm

End Function

Print subtract_s( 5, 1 ) '' 5 - 1

''---------------------------------------------------------------------------------------------------------------------

'' Naked pascal function. Parameters are pushed in declaration order, and the
'' called procedure removes them by passing their byte count to RET.
Function subtract_p Naked pascal _
	( _
		ByVal a As Long, _
		ByVal b As Long _
	) As Long

	Asm
		mov eax, dword Ptr [esp+8]  '' eax = a
		Sub eax, dword Ptr [esp+4]  '' eax -= b
		ret 8                       '' return result in eax and 8 bytes (2 longs) to release
	End Asm

End Function

Print subtract_p( 5, 1 ) '' 5 - 1

#Else

Print "This example requires a 32-bit x86 target."

#EndIf
