'' Project: FreeBASIC native library regression checks
'' File: jni-abi.bas
'' Purpose: Compare JNI layouts and exercise a real VM through the binding.
'' Responsibilities: Integer arrays, critical access and direct buffer addresses.
'' This file intentionally contains no Java implementation or mock JNI table.

#lang "fb"
#include once "jni.bi"

Print "ABI "; SizeOf(jint); " "; SizeOf(jsize); " "; SizeOf(jlong); " "; SizeOf(jvalue); " "; _
	SizeOf(JavaVMOption); " "; SizeOf(JavaVMInitArgs); " "; _
	OffsetOf(JavaVMInitArgs, nOptions); " "; OffsetOf(JavaVMInitArgs, options); " "; _
	OffsetOf(JavaVMInitArgs, ignoreUnrecognized); " "; SizeOf(JavaVMAttachArgs); " "; _
	OffsetOf(JNINativeInterface_, GetPrimitiveArrayCritical); " "; _
	OffsetOf(JNINativeInterface_, GetDirectBufferAddress)

Dim As JavaVMInitArgs arguments
arguments.version = JNI_VERSION_1_4
arguments.ignoreUnrecognized = JNI_TRUE
Dim As JavaVM Ptr vm
Dim As JNIEnv Ptr env
If JNI_CreateJavaVM(@vm, @env, @arguments) <> JNI_OK Then End 1
If vm = 0 OrElse env = 0 Then End 1

Dim As Integer exit_status = 1
'' Keep borrowed storage alive through VM shutdown on every failure path.
Dim As UByte storage(0 To 15)
Do
	Dim As jintArray numbers = (*env)->NewIntArray(env, 4)
	If numbers = 0 Then Exit Do
	Dim As jint expected(0 To 3) = { -2147483647 - 1, -7, 42, 2147483647 }
	(*env)->SetIntArrayRegion(env, numbers, 0, 4, @expected(0))
	If (*env)->ExceptionCheck(env) Then Exit Do
	Dim As jint Ptr critical = (*env)->GetPrimitiveArrayCritical(env, numbers, 0)
	If critical = 0 Then Exit Do
	'' JNI forbids other VM calls while this critical pointer is held.
	critical[2] = -42
	(*env)->ReleasePrimitiveArrayCritical(env, numbers, critical, 0)
	expected(2) = -42
	Dim As jint actual(0 To 3)
	(*env)->GetIntArrayRegion(env, numbers, 0, 4, @actual(0))
	If (*env)->ExceptionCheck(env) Then Exit Do
	Dim As Boolean valid = True
	For index As Integer = 0 To 3
		If actual(index) <> expected(index) Then valid = False
	Next
	If Not valid Then Exit Do
	(*env)->DeleteLocalRef(env, numbers)

	'' The VM borrows this stack buffer only until the local reference is
	'' released below. No Java code can retain it beyond this call sequence.
	Dim As jobject buffer = (*env)->NewDirectByteBuffer(env, @storage(0), SizeOf(storage))
	If buffer = 0 Then Exit Do
	If (*env)->GetDirectBufferAddress(env, buffer) <> @storage(0) Then Exit Do
	If (*env)->GetDirectBufferCapacity(env, buffer) <> SizeOf(storage) Then Exit Do
	(*env)->DeleteLocalRef(env, buffer)
	exit_status = 0
Loop While False

If (*env)->ExceptionCheck(env) Then (*env)->ExceptionDescribe(env)
If (*vm)->DestroyJavaVM(vm) <> JNI_OK Then exit_status = 1
If exit_status = 0 Then Print "JNI_NATIVE_PASS"
End exit_status

'' end of jni-abi.bas
