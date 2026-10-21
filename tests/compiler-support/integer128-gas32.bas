'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: integer128-gas32.bas
''
'' Purpose:
''
''     Exercise 128-bit arithmetic and procedure calls in the 32-bit backends.
''
'' Responsibilities:
''
''     - test unsigned wide shifts, multiplication, division and remainder
''     - test signed division and remainder through the portable runtime
''     - test 16-byte arguments and hidden-pointer results on GAS32 and LLVM i386
''     - verify parameters, stack locals, and return slots honor 16-byte alignment
''     - test callee cleanup when a wide argument precedes a narrow StdCall argument
''     - test indirect calls with two wide arguments and a wide result
''     - fail through fb_End when a result is incorrect
''
'' This file intentionally does NOT contain:
''
''     - console output or other runtime services
''

#lang "fb"

Type Integer128AddCallback As Function _
	(ByVal leftValue As UInteger<128>, ByVal rightValue As UInteger<128>) As UInteger<128>

Type Gas32Integer128Layout
	marker As UByte
	value As UInteger<128>
	suffix As UByte
End Type

Private Function gas32AdjustSigned(ByVal value As Integer<128>) As Integer<128>
	If (CUInt(@value) And 15) <> 0 Then End 15
	value += 1
	Return value
End Function

Private Function gas32LocalAlignment() As UInteger
	Dim As UInteger<128> scalarValue
	Dim As Gas32Integer128Layout recordValue
	If (CUInt(@scalarValue) And 15) <> 0 Then Return 1
	Return CUInt(@recordValue) And 15
End Function

Private Function gas32AddUnsigned(ByVal leftValue As UInteger<128>, _
								  ByVal rightValue As UInteger<128>) As UInteger<128>
	Return leftValue + rightValue
End Function

Private Function gas32AddStdCall StdCall (ByVal value As UInteger<128>, _
									  ByVal addition As UInteger) As UInteger<128>
	Return value + addition
End Function

Dim As UInteger<128> unitValue = 1
Dim As UInteger<128> highValue = unitValue Shl 100
Dim As UInteger<128> totalValue = highValue + 7
Dim As UInteger<128> productValue = highValue * 3
Dim As UInteger<128> quotientValue = highValue \ 5
Dim As UInteger<128> remainderValue = highValue Mod 5
Dim As Integer<128> negativeValue = -100
Dim As Integer<128> divisorValue = 7
Dim As Integer<128> signedQuotientValue = negativeValue \ divisorValue
Dim As Integer<128> signedRemainderValue = negativeValue Mod divisorValue

If totalValue <> ((unitValue Shl 100) + 7) Then End 1
If productValue <> ((unitValue Shl 100) * 3) Then End 2
If quotientValue <> ((unitValue Shl 100) \ 5) Then End 3
If remainderValue <> ((unitValue Shl 100) Mod 5) Then End 4
If signedQuotientValue <> -14 Then End 5
If signedRemainderValue <> -2 Then End 6

Dim As Integer<128> callInput = -41
If gas32AdjustSigned(callInput) <> -40 Then End 7
If callInput <> -41 Then End 8
If gas32AddUnsigned(highValue, 11) <> (highValue + 11) Then End 9
If gas32AddUnsigned(1, 2) <> 3 Then End 10
If gas32AddUnsigned(gas32AddUnsigned(1, 2), 4) <> 7 Then End 11
If gas32AddStdCall(highValue, 17) <> (highValue + 17) Then End 12
Dim As Integer128AddCallback addCallback = @gas32AddUnsigned
If addCallback(highValue, 11) <> (highValue + 11) Then End 13
If gas32LocalAlignment() <> 0 Then End 14

'' end of integer128-gas32.bas
