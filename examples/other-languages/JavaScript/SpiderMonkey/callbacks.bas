'' FreeBASIC SpiderMonkey callback example
'' File: callbacks.bas
'' Purpose: Expose FreeBASIC print and uppercase functions to JavaScript.
'' Responsibilities: Checked conversions, copied string results and cleanup.
'' This example contains no JavaScript engine implementation or worker threads.

'' Callback example: Functions that are used by the Javascript code,
'' but are implemented in FB.
#include once "spidermonkey/jsapi.bi"

' The SDK borrows this class descriptor for every object's lifetime.
' A single application thread owns the runtime and its callbacks.
Dim Shared As JSClass global_class = _
( _
    @"global", 0, _
    @JS_PropertyStub, @JS_PropertyStub, @JS_PropertyStub, @JS_PropertyStub, _
    @JS_EnumerateStub, @JS_ResolveStub, @JS_ConvertStub, @JS_FinalizeStub _
)

Private Function print_callback cdecl _
    ( _
        ByVal cx As JSContext Ptr, _
        ByVal obj As JSObject Ptr, _
        ByVal argc As uintN, _
        ByVal argv As jsval Ptr, _
        ByVal rval As jsval Ptr _
    ) As JSBool

    If (argc < 1 OrElse argv = 0 OrElse rval = 0) Then
        Return 0
    End If

    Dim As JSString Ptr arg_string = JS_ValueToString(cx, argv[0])
    If arg_string = 0 Then Return 0
    Dim As ZString Ptr arg_bytes = JS_GetStringBytes(arg_string)
    If arg_bytes = 0 Then Return 0
    Print *arg_bytes
    *rval = JSVAL_VOID
    Return 1
End Function

Private Function ucase_callback cdecl _
    ( _
        ByVal cx As JSContext Ptr, _
        ByVal obj As JSObject Ptr, _
        ByVal argc As uintN, _
        ByVal argv As jsval Ptr, _
        ByVal rval As jsval Ptr _
    ) As JSBool

    If (argc < 1 OrElse argv = 0 OrElse rval = 0) Then
        Return 0
    End If

    Dim As JSString Ptr arg_string = JS_ValueToString(cx, argv[0])
    If arg_string = 0 Then Return 0
    Dim As ZString Ptr arg_bytes = JS_GetStringBytes(arg_string)
    If arg_bytes = 0 Then Return 0
    ' Copy borrowed bytes before calling an API which can allocate or collect.
    Dim As String result = UCase(*arg_bytes)
    Dim As JSString Ptr result_string = JS_NewStringCopyN(cx, StrPtr(result), Len(result))
    If result_string = 0 Then Return 0
    ' The SDK owns the copy; the FreeBASIC string can leave scope safely.
    *rval = STRING_TO_JSVAL(result_string)
    Return 1
End Function

Const TEST_SCRIPT = "print(ucase('hello'));"

Function run_example() As Integer
    Dim As JSRuntime Ptr rt
    Dim As JSContext Ptr cx
    Dim As Integer status = 1
    Do
        ' One MiB GC budget; 4096 selects the context's stack chunk size.
        rt = JS_NewRuntime(1048576)
        If rt = 0 Then Exit Do
        cx = JS_NewContext(rt, 4096)
        If cx = 0 Then Exit Do
        Dim As JSObject Ptr global = JS_NewObject(cx, @global_class, NULL, NULL)
        If global = 0 Then Exit Do
        JS_SetGlobalObject(cx, global)
        If JS_InitStandardClasses(cx, global) = 0 Then Exit Do
        If JS_DefineFunction(cx, global, "print", @print_callback, 1, 0) = 0 Then Exit Do
        If JS_DefineFunction(cx, global, "ucase", @ucase_callback, 1, 0) = 0 Then Exit Do
        Dim As jsval rval
        If JS_EvaluateScript(cx, global, TEST_SCRIPT, Len(TEST_SCRIPT), "localhost", 1, @rval) = 0 Then Exit Do
        status = 0
    Loop While 0
    ' Destroy contexts before their runtime, then release process-wide state.
    If cx <> 0 Then JS_DestroyContext(cx)
    If rt <> 0 Then JS_DestroyRuntime(rt)
    JS_ShutDown()
    Return status
End Function

Dim As Integer status = run_example()
If status <> 0 Then Print "SpiderMonkey example failed"
End status
'' end of callbacks.bas
