Attribute VB_Name = "Module1"


'' note: the argument must be passed by value (ByVal), VB6 seems to not
''	  	 pass the BSTR correctly by reference without a COM type-library

'' This file is consumed by VB6. Its BSTR String declaration and default VB
'' calling convention are part of that host's import syntax, not FB bindings.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ABI-002 FBL-ABI-004
Declare Function dupme Lib "fbside.dll" Alias "dupme" (ByVal arg As String) As String


Sub main()
    Dim res As String

    res = "Hello! "

    '' MsgBox is a VB6 intrinsic supplied by the hosting environment.
    '' FB-LINTER: DISABLE-NEXT-LINE FBL310
    MsgBox dupme(res)

End Sub
