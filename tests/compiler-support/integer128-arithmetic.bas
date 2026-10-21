'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: integer128-arithmetic.bas
''
'' Purpose:
''
''     Verify signed and unsigned 128-bit integer arithmetic across compiler
''     backends.
''
'' Responsibilities:
''
''     - exercise arithmetic, comparison, conversion, and shifts beyond 64 bits
''     - check compound assignments and integer FOR loops
''
'' This file intentionally does NOT contain:
''
''     - C calling convention checks, which are covered by integer128-abi.bas
''

#lang "fb"

Dim As Integer<128> signedValue = 2147483647
signedValue += 1
If signedValue <> 2147483648 Then End 1

Dim As Integer<128> negativeValue = -1
If (negativeValue < 0) = 0 Then End 2

Dim As Integer<128> signedRight = -8
If (signedRight Shr 2) <> -2 Then End 3

Dim As UInteger<128> oneValue = 1
Dim As UInteger<128> shiftedValue = oneValue Shl 100
If shiftedValue <= oneValue Then End 4

Dim As UInteger<128> sumValue = shiftedValue + oneValue
If (sumValue - shiftedValue) <> oneValue Then End 5

Dim As UInteger<128> productValue = (oneValue Shl 80) * 3
If (productValue Shr 80) <> 3 Then End 6

Dim As UInteger<128> quotientValue = (oneValue Shl 100) \ 5
Dim As UInteger<128> remainderValue = (oneValue Shl 100) Mod 5
If (quotientValue * 5 + remainderValue) <> (oneValue Shl 100) Then End 7

Dim As UInteger<128> lowWordMaximum = &hFFFFFFFFFFFFFFFFull
If lowWordMaximum <> ((oneValue Shl 64) - 1) Then End 8
If (lowWordMaximum Shr 64) <> 0 Then End 9

Dim As UInteger<128> unsignedNegativeOne = -1
If unsignedNegativeOne < oneValue Then End 10
If (unsignedNegativeOne Shr 127) <> 1 Then End 11

Dim As Integer<128> convertedFloat = 12.75
If convertedFloat <> 13 Then End 12
Dim As Integer<128> convertedNegativeFloat = -12.75
If convertedNegativeFloat <> -13 Then End 13
Dim As UInteger<128> convertedUnsignedFloat = 7.75
If convertedUnsignedFloat <> 8 Then End 14
Dim As UInteger<128> highUnsignedValue = oneValue Shl 127
If CDbl(highUnsignedValue) <= 0.0 Then End 17

Dim As UInteger<128> bitBase = oneValue Shl 100
Dim As UInteger<128> bitPattern = bitBase Or 15
If (bitPattern And 15) <> 15 Then End 18
If (bitPattern Xor 15) <> bitBase Then End 19
Dim As UInteger<128> equivalentValue = oneValue Eqv oneValue
If (equivalentValue Shr 127) <> 1 Then End 20
Dim As UInteger<128> implicationValue = 0 Imp oneValue
If (implicationValue Shr 127) <> 1 Then End 21

Dim As Integer<128> zeroValue = 0
If (Not zeroValue) <> -1 Then End 22
Dim As Integer<128> absoluteValue = -8
If Abs(absoluteValue) <> 8 Then End 23
If Sgn(absoluteValue) <> -1 Then End 24
If Sgn(zeroValue) <> 0 Then End 25

Dim As Integer<128> signedDividend = -100
Dim As Integer<128> signedDivisor = 7
If (signedDividend \ signedDivisor) <> -14 Then End 26
If (signedDividend Mod signedDivisor) <> -2 Then End 27

Dim As Integer<128> signedCount = 0
For signedIndex As Integer<128> = 0 To 3
	signedCount += signedIndex
Next
If signedCount <> 6 Then End 15

Dim As UInteger<128> unsignedCount = 0
For unsignedIndex As UInteger<128> = 1 To 3 Step 1
	unsignedCount += unsignedIndex
Next
If unsignedCount <> 6 Then End 16

Print "128-bit integer arithmetic passed"

'' end of integer128-arithmetic.bas
