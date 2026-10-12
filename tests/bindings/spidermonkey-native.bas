'' FreeBASIC SpiderMonkey 1.6 native qualification
'' File: spidermonkey-native.bas
'' Purpose: Compare native C layouts and execute the real JavaScript engine.
'' Responsibilities: Tagged values, callbacks, rooted GC data and cleanup.
'' This file contains no substitute JavaScript parser or engine implementation.

#lang "fb"
#include once "spidermonkey/jsapi.bi"

' The single-threaded SDK borrows this class for every object's lifetime.
Dim Shared As JSClass global_class = _
( _
	@"global", 0, _
	@JS_PropertyStub, @JS_PropertyStub, @JS_PropertyStub, @JS_PropertyStub, _
	@JS_EnumerateStub, @JS_ResolveStub, @JS_ConvertStub, @JS_FinalizeStub _
)
Dim Shared As Long callback_count

Function add_callback Cdecl(ByVal cx As JSContext Ptr, ByVal obj As JSObject Ptr, _
	ByVal argc As uintN, ByVal argv As jsval Ptr, ByVal result As jsval Ptr) As JSBool
	If argc <> 2 OrElse argv = 0 OrElse result = 0 Then Return JS_FALSE
	Dim As int32 first, second
	If JS_ValueToInt32(cx, argv[0], @first) = 0 OrElse JS_ValueToInt32(cx, argv[1], @second) = 0 Then Return JS_FALSE
	' Keep addition within the SDK's immediate integer range.
	If first < -1000 OrElse first > 1000 OrElse second < -1000 OrElse second > 1000 Then Return JS_FALSE
	callback_count += 1
	*result = INT_TO_JSVAL(first + second)
	Return JS_TRUE
End Function

Function evaluate(ByVal cx As JSContext Ptr, ByVal global As JSObject Ptr, _
	ByRef script As Const String, ByVal expected As int32) As Integer
	Dim As jsval result
	Dim As int32 number
	If JS_EvaluateScript(cx, global, StrPtr(script), Len(script), "native-test", 1, @result) = 0 Then Return 0
	If JS_ValueToInt32(cx, result, @number) = 0 Then Return 0
	Return number = expected
End Function

Function check_engine() As Integer
	Dim As ZString Ptr version = JS_GetImplementationVersion()
	If version = 0 OrElse Left(*version, 16) <> "JavaScript-C 1.6" Then Return 2
	If JSVAL_INT_MIN <> -1073741823 OrElse JSVAL_INT_MAX <> 1073741823 OrElse _
		JSVAL_TO_BOOLEAN(JSVAL_TRUE) <> 1 OrElse JSVAL_TO_BOOLEAN(JSVAL_FALSE) <> 0 OrElse _
		JSVAL_IS_GCTHING(JSVAL_TRUE) OrElse JSVAL_IS_GCTHING(INT_TO_JSVAL(17)) OrElse _
		JSVAL_TO_INT(Cast(jsval, -33)) <> -17 Then Return 2
	' The budget is one MiB; the context's 4096 bytes select its stack chunk.
	Dim As JSRuntime Ptr runtime = JS_NewRuntime(1048576)
	If runtime = 0 Then Return 3
	Dim As JSContext Ptr context = JS_NewContext(runtime, 4096)
	Dim As Integer status = 1, root_added = 0
	Dim As jsval rooted = JSVAL_NULL
	If context <> 0 Then
		Dim As JSObject Ptr global = JS_NewObject(context, @global_class, 0, 0)
		If global <> 0 Then
			JS_SetGlobalObject(context, global)
			If JS_InitStandardClasses(context, global) AndAlso _
				JS_DefineFunction(context, global, "add", @add_callback, 2, 0) <> 0 Then
				If evaluate(context, global, "function fact(n){return n<=1?1:n*fact(n-1);}fact(5)", 120) AndAlso _
					evaluate(context, global, "add(7,10)", 17) AndAlso callback_count = 1 Then
					Dim As jsval ignored
					If JS_EvaluateScript(context, global, "add(5)", 6, "bad-arguments", 1, @ignored) = 0 Then
						JS_ClearPendingException(context)
						Dim As JSString Ptr text = JS_NewStringCopyZ(context, "root survives GC")
						If text <> 0 Then
							rooted = STRING_TO_JSVAL(text)
							If JSVAL_IS_GCTHING(rooted) AndAlso JS_AddRoot(context, @rooted) Then
								root_added = 1
								JS_GC(context)
								Dim As ZString Ptr bytes = JS_GetStringBytes(JSVAL_TO_STRING(rooted))
								If bytes <> 0 AndAlso *bytes = "root survives GC" Then status = 0
							End If
						End If
					End If
				End If
			End If
		End If
		If root_added Then JS_RemoveRoot(context, @rooted)
		JS_DestroyContext(context)
	End If
	JS_DestroyRuntime(runtime)
	JS_ShutDown()
	If status = 0 Then Print "SPIDERMONKEY_NATIVE=PASS factorial=120 callback=17 root=1 rejection=1 tags=7"
	Return status
End Function

Print "ABI "; SizeOf(JSInt32); " "; SizeOf(JSUint32); " "; SizeOf(JSBool); " "; SizeOf(JSWord); " "; SizeOf(jsval); " "; _
	SizeOf(JSClass); " "; OffsetOf(JSClass, flags); " "; OffsetOf(JSClass, addProperty); " "; OffsetOf(JSClass, reserveSlots); " "; _
	SizeOf(JSFunctionSpec); " "; SizeOf(JSPropertySpec); " "; SizeOf(jschar)
End check_engine()
'' end of spidermonkey-native.bas
