'' examples/manual/proguide/opovld/member-ops.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Operator Overloading'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgOperatorOverloading
'' --------

Type Rational
	As Integer numerator, denominator

	Declare Operator Cast () As Double
	Declare Operator Cast () As String
	Declare Operator *= (ByRef rhs As Rational)
End Type

Operator Rational.cast () As Double
	'' The type is publicly constructible, so preserve a defined conversion even
	'' if a caller supplies an invalid zero denominator.
	If denominator = 0 Then Return 0

	'' The preceding guard makes this conversion division safe.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-NUM-016
	Return numerator / denominator
End Operator

Operator Rational.cast () As String
	Return numerator & "/" & denominator
End Operator

Operator Rational.*= (ByRef rhs As Rational)
	numerator *= rhs.numerator
	denominator *= rhs.denominator
End Operator

Dim As Rational r1 = (2, 3)
Dim As Rational r2 = (3, 4)
r1 *= r2
Dim As Double d = r1
Print r1, d
