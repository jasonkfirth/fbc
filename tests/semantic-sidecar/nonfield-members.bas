' Project: FreeBASIC compiler semantic sidecar tests
' File: nonfield-members.bas
' Purpose: Preserve original non-field member selection roles through lowering.
' Responsibilities: Explicit and implicit receivers, queries, overloads and macros.
' Targets: Native frontend tests; GCC, Clang and LLVM backend emission only.
' Module API: MemberOverlay, BaseValue, DerivedValue and ProbeSelections.
' This file intentionally does NOT contain:
'     Runtime execution of the sample member selections.
#lang "fb"

Union MemberOverlay
    value As Long
    Static shared_value As Long
    Const constant_value = 3
    Declare Function read_value() As Long
    Declare Function read_again() As Long
    Declare Static Function static_value() As Long
    Declare Static Function choose Overload(ByVal item As Double) As Long
    Declare Function choose Overload(ByVal item As Long) As Long
End Union
Dim MemberOverlay.shared_value As Long
Function MemberOverlay.read_value() As Long
    Return value + read_again() ' NFM: expression,MemberOverlay,read_again
End Function
Function MemberOverlay.read_again() As Long
    Return constant_value ' NFM: symbol,-,constant_value
End Function
Function MemberOverlay.static_value() As Long
    Return MemberOverlay.constant_value ' NFM: symbol,-,constant_value
End Function
Function MemberOverlay.choose(ByVal item As Double) As Long
    Return CInt(item)
End Function
Function MemberOverlay.choose(ByVal item As Long) As Long
    Return item
End Function

Type BaseValue
    value As Long
    Declare Function read_base() As Long
End Type
Type DerivedValue Extends BaseValue
End Type
Function BaseValue.read_base() As Long
    Return value
End Function

' Keep the type parameter bare to exercise actual type-qualified selections.
' FB-LINTER: DISABLE-NEXT-LINE FBL-PP-001
#Define MIXED_DATA(v,t) ((v).shared_value+t.shared_value+(v).constant_value+t.constant_value)
' FB-LINTER: DISABLE-NEXT-LINE FBL-PP-001
#Define MIXED_CALL(v,t) ((v).static_value()+t.static_value())
#Define WRAPPED_CALL(v,t) MIXED_CALL(v,t)
#Define NESTED_CALL(v,t) ((v).choose(CLng(t.static_value())))

Sub ProbeSelections()
    Dim object_value As MemberOverlay
    Dim second_value As MemberOverlay
    Dim derived_value As DerivedValue
    Dim result_value As Long
    result_value = object_value.read_value() ' NFM: expression,MemberOverlay,read_value
    object_value.shared_value = 1 ' NFM: expression,MemberOverlay,shared_value
    result_value = object_value.constant_value ' NFM: expression,MemberOverlay,constant_value
    MemberOverlay.shared_value = 2 ' NFM: symbol,-,shared_value
    result_value = MemberOverlay.constant_value ' NFM: symbol,-,constant_value
    result_value = object_value.static_value() ' NFM: expression,MemberOverlay,static_value
    result_value = MemberOverlay.static_value() ' NFM: type,MemberOverlay,static_value
    result_value = object_value.choose(CLng(1)) ' NFM: expression,MemberOverlay,choose
    result_value = object_value.choose(CDbl(1)) ' NFM: expression,MemberOverlay,choose
    result_value = MemberOverlay.choose(CDbl(1)) ' NFM: type,MemberOverlay,choose
    result_value = derived_value.read_base() ' NFM: expression,DerivedValue,read_base
    result_value = SizeOf(MemberOverlay.shared_value) ' NFM: type,MemberOverlay,shared_value
    result_value = SizeOf(MemberOverlay.constant_value) ' NFM: type,MemberOverlay,constant_value
    result_value = SizeOf(MemberOverlay.read_value) ' NFM: type,MemberOverlay,read_value
    result_value = SizeOf(object_value.shared_value) ' NFM: expression,MemberOverlay,shared_value
    Dim inferred As TypeOf(object_value.constant_value) ' NFM: expression,MemberOverlay,constant_value
    With object_value
        result_value = .constant_value ' NFM: expression,MemberOverlay,constant_value
        result_value = .read_value() ' NFM: expression,MemberOverlay,read_value
    End With
    Dim object_ptr As MemberOverlay Ptr = @object_value
    result_value = object_ptr->static_value() ' NFM: expression,MemberOverlay,static_value
    result_value = MIXED_DATA(object_value,MemberOverlay)
    result_value = MIXED_CALL(object_value,MemberOverlay)
    result_value = WRAPPED_CALL(object_value,MemberOverlay)
    result_value = NESTED_CALL(object_value,MemberOverlay)
End Sub
' end of nonfield-members.bas
