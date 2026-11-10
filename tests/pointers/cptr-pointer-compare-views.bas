'' Project: FreeBASIC compiler pointer backend tests
'' File: cptr-pointer-compare-views.bas
'' Purpose: Check CPtr casts do not emit incompatible C pointer comparisons.
'' This file intentionally does NOT contain: a C runtime dependency.

#cmdline "-gen gcc"
#cmdline "-Wc -Werror=compare-distinct-pointer-types"

#include once "crt/long.bi"

Type C2fbCULong As CULong

Function CompareAliasedCLongPointers CDecl _
	( ByVal output_ptr As C2fbCULong Ptr, _
	  ByVal input_ptr As Const C2fbCULong Ptr, _
	  ByVal limb_count As LongInt ) As Long
	If CPtr(C2fbCULong Ptr, output_ptr + limb_count) > input_ptr Then Return 1
	If CPtr(Const C2fbCULong Ptr, input_ptr + limb_count) > output_ptr Then Return 2
	Return 0
End Function

Function CompareBuiltinCLongPointers CDecl _
	( ByVal output_ptr As CULong Ptr, _
	  ByVal input_ptr As Const CULong Ptr, _
	  ByVal limb_count As LongInt ) As Long
	If CPtr(CULong Ptr, output_ptr + limb_count) > input_ptr Then Return 1
	If CPtr(Const CULong Ptr, input_ptr + limb_count) > output_ptr Then Return 2
	Return 0
End Function

Dim values(0 To 4) As CULong
If CompareAliasedCLongPointers(@values(0), @values(2), 2) <> 2 Then End 1
If CompareAliasedCLongPointers(@values(1), @values(0), 1) <> 1 Then End 2
If CompareAliasedCLongPointers(@values(1), @values(1), 0) <> 0 Then End 3
If CompareBuiltinCLongPointers(@values(0), @values(2), 2) <> 2 Then End 4
If CompareBuiltinCLongPointers(@values(1), @values(0), 1) <> 1 Then End 5
If CompareBuiltinCLongPointers(@values(1), @values(1), 0) <> 0 Then End 6

'' end of cptr-pointer-compare-views.bas
