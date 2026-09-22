'' examples/manual/proguide/recursion_iteration/quick_sort_algorithm.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Replace Recursion with Iteration'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgRecursionIteration
'' --------

'' Both sorting implementations share this fixed 100-byte demonstration buffer.
'' FB-LINTER: DISABLE-NEXT-LINE FBL301
Dim Shared As UByte t(0 To 99)

Sub recursiveQuicksort (ByVal L As Integer, ByVal R As Integer)
	Dim As Integer pivot = L
	Dim As Integer I = L
	Dim As Integer J = R
	Do
		If t(I) >= t(J) Then
			Swap t(I), t(J)
			pivot = L + R - pivot
		End If
		If pivot = L Then
			J = J - 1
		Else
			I = I + 1
		End If
	Loop Until I = J
	If L < I - 1 Then
		recursiveQuicksort(L, I - 1)
	End If
	If R > J + 1 Then
		recursiveQuicksort(J + 1, R)
	End If
End Sub

#include "DynamicUserStackTypeCreateMacro.bi"
DynamicUserStackTypeCreate(DynamicUserStackTypeForInteger, Integer)

Sub translationToIteraticeQuicksortStack (ByVal L As Integer, ByVal R As Integer)
	Dim As DynamicUserStackTypeForInteger S
	S.push = L : S.push = R
	While S.used > 0
		R = S.pop : L = S.pop
		Dim As Integer pivot = L
		Dim As Integer I = L
		Dim As Integer J = R
		Do
			If t(I) >= t(J) Then
				Swap t(I), t(J)
				pivot = L + R - pivot
			End If
			If pivot = L Then
				J = J - 1
			Else
				I = I + 1
			End If
		Loop Until I = J
		If L < I - 1 Then
			S.push = L : S.push = I - 1
		End If
		If R > J + 1 Then
			S.push = J + 1 : S.push = R
		End If
	Wend
End Sub



Randomize
For I As Integer = LBound(t) To UBound(t)
	t(i) = Int(Rnd * 256)
Next I
Print "raw memory:"
For K As Integer = LBound(t) To UBound(t)
	Print Using "####"; t(K);
Next K
Print

recursiveQuicksort(0, 99)

Print "sorted memory by recursion:"
For K As Integer = LBound(t) To UBound(t)
	Print Using "####"; t(K);
Next K
Print
Print

Randomize
For I As Integer = LBound(t) To UBound(t)
	t(i) = Int(Rnd * 256)
Next I
Print "raw memory:"
For K As Integer = LBound(t) To UBound(t)
	Print Using "####"; t(K);
Next K
Print

translationToIteraticeQuicksortStack(0, 99)

Print "sorted memory by iteration with stack:"
For K As Integer = LBound(t) To UBound(t)
	Print Using "####"; t(K);
Next K
Print

Sleep
