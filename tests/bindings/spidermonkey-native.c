/* FreeBASIC SpiderMonkey 1.6 native qualification
 * File: spidermonkey-native.c
 * Purpose: Provide actual C layouts, tagged values and engine results.
 * Responsibilities: Script evaluation, a native callback and rooted GC data.
 * This file contains no substitute JavaScript parser or engine implementation.
 */
#include <jsapi.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* This SDK is built without JS_THREADSAFE. One thread owns every runtime
 * and callback here. JSClass is borrowed by its objects for their lifetime. */
static JSClass global_class = {
    "global", 0, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static int callback_count;

static JSBool add_callback(JSContext *cx, JSObject *object, uintN argc,
                           jsval *argv, jsval *result)
{
    int32 first, second;
    (void)object;
    if (argc != 2 || !JS_ValueToInt32(cx, argv[0], &first) ||
        !JS_ValueToInt32(cx, argv[1], &second)) return JS_FALSE;
    /* These limits keep addition within the SDK's immediate integer range. */
    if (first < -1000 || first > 1000 || second < -1000 || second > 1000) return JS_FALSE;
    callback_count++;
    *result = INT_TO_JSVAL(first + second);
    return JS_TRUE;
}

static int evaluate(JSContext *cx, JSObject *global, const char *script, int32 expected)
{
    jsval result;
    int32 number;
    if (!JS_EvaluateScript(cx, global, script, strlen(script), "native-test", 1, &result)) return 0;
    return JS_ValueToInt32(cx, result, &number) && number == expected;
}

int main(void)
{
    JSRuntime *runtime = NULL;
    JSContext *context = NULL;
    JSObject *global;
    JSString *text;
    jsval rooted = JSVAL_NULL, ignored;
    int root_added = 0, status = 1;
    const char *version = JS_GetImplementationVersion();
    if (!version || strncmp(version, "JavaScript-C 1.6", 16)) return 2;
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(JSInt32), sizeof(JSUint32), sizeof(JSBool), sizeof(JSWord), sizeof(jsval),
           sizeof(JSClass), offsetof(JSClass, flags), offsetof(JSClass, addProperty),
           offsetof(JSClass, reserveSlots), sizeof(JSFunctionSpec), sizeof(JSPropertySpec), sizeof(jschar));
    if (JSVAL_INT_MIN != -1073741823 || JSVAL_INT_MAX != 1073741823 ||
        JSVAL_TO_BOOLEAN(JSVAL_TRUE) != 1 || JSVAL_TO_BOOLEAN(JSVAL_FALSE) != 0 ||
        JSVAL_IS_GCTHING(JSVAL_TRUE) || JSVAL_IS_GCTHING(INT_TO_JSVAL(17)) ||
        JSVAL_TO_INT((jsval)-33) != -17) return 2;
    runtime = JS_NewRuntime(1048576); /* One MiB GC heap budget. */
    if (!runtime) goto cleanup;
    context = JS_NewContext(runtime, 4096); /* SDK stack chunk, not thread stack. */
    if (!context) goto cleanup;
    global = JS_NewObject(context, &global_class, NULL, NULL);
    if (!global) goto cleanup;
    JS_SetGlobalObject(context, global);
    if (!JS_InitStandardClasses(context, global)) goto cleanup;
    if (!JS_DefineFunction(context, global, "add", add_callback, 2, 0)) goto cleanup;
    if (!evaluate(context, global, "function fact(n){return n<=1?1:n*fact(n-1);}fact(5)", 120) ||
        !evaluate(context, global, "add(7,10)", 17) || callback_count != 1) goto cleanup;
    if (JS_EvaluateScript(context, global, "add(5)", 6, "bad-arguments", 1, &ignored)) goto cleanup;
    JS_ClearPendingException(context);
    text = JS_NewStringCopyZ(context, "root survives GC");
    if (!text) goto cleanup;
    rooted = STRING_TO_JSVAL(text);
    if (!JSVAL_IS_GCTHING(rooted) || !JS_AddRoot(context, &rooted)) goto cleanup;
    root_added = 1;
    JS_GC(context);
    text = JSVAL_TO_STRING(rooted);
    if (!JS_GetStringBytes(text) || strcmp(JS_GetStringBytes(text), "root survives GC")) goto cleanup;
    puts("SPIDERMONKEY_NATIVE=PASS factorial=120 callback=17 root=1 rejection=1 tags=7");
    status = 0;
cleanup:
    if (root_added) JS_RemoveRoot(context, &rooted);
    if (context) JS_DestroyContext(context);
    if (runtime) JS_DestroyRuntime(runtime);
    JS_ShutDown();
    return status;
}
/* end of spidermonkey-native.c */
