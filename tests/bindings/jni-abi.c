/*
 * FreeBASIC native library regression checks
 * File: jni-abi.c
 * Purpose: Report the JNI layout from the installed JDK's own C headers.
 * Responsibilities: Independent sizes and offsets for the FreeBASIC probe.
 * This file contains no VM initialization or duplicate JNI declarations.
 */
#include <stddef.h>
#include <stdio.h>
#include <jni.h>

int main(void)
{
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(jint), sizeof(jsize), sizeof(jlong), sizeof(jvalue),
           sizeof(JavaVMOption), sizeof(JavaVMInitArgs),
           offsetof(JavaVMInitArgs, nOptions), offsetof(JavaVMInitArgs, options),
           offsetof(JavaVMInitArgs, ignoreUnrecognized), sizeof(JavaVMAttachArgs),
           offsetof(struct JNINativeInterface_, GetPrimitiveArrayCritical),
           offsetof(struct JNINativeInterface_, GetDirectBufferAddress));
    return 0;
}
/* end of jni-abi.c */
