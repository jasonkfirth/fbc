'' examples/manual/libraries/spidermonkey1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'SpiderMonkey'
'' Purpose: Evaluate JavaScript using the archived SpiderMonkey 1.6 API.
'' Responsibilities: Checked engine setup, borrowed strings and owned cleanup.
'' This example contains no JavaScript engine implementation or worker threads.
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibspidermonkey
'' --------

'' Evaluating javascript code
#include Once "spidermonkey/jsapi.bi"

' The SDK borrows this class descriptor for every object's lifetime.
' A single application thread owns the runtime and its callbacks.
Dim Shared As JSClass global_class = _
( _
	@"global", 0, _
	@JS_PropertyStub, @JS_PropertyStub, @JS_PropertyStub, @JS_PropertyStub, _
	@JS_EnumerateStub, @JS_ResolveStub, @JS_ConvertStub, @JS_FinalizeStub _
)

Const TEST_SCRIPT = _
	!"function fact(n)           \n" + _
	!"{                          \n" + _
	!"    if (n <= 1)            \n" + _
	!"        return 1;          \n" + _
	!"                           \n" + _
	!"    return n * fact(n - 1);\n" + _
	!"}                          \n" + _
	!"                           \n" + _
	!"    fact(5)                \n"

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
		Dim As jsval rval
		If JS_EvaluateScript(cx, global, TEST_SCRIPT, Len(TEST_SCRIPT), "localhost", 1, @rval) = 0 Then Exit Do
		Dim As JSString Ptr result_string = JS_ValueToString(cx, rval)
		If result_string = 0 Then Exit Do
		Dim As ZString Ptr result_bytes = JS_GetStringBytes(result_string)
		If result_bytes = 0 Then Exit Do
		Print "result: " & *result_bytes
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
'' end of spidermonkey1.bas
