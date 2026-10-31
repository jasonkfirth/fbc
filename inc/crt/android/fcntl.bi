'' Project: FreeBASIC CRT headers; File: crt/android/fcntl.bi.
'' Purpose: Expose Android Bionic's common descriptor-control interfaces.
'' Responsibilities: Open flags, descriptor flags and checked native prototypes.
'' This file does not implement I/O or expose architecture-dependent flock layouts.
#ifndef __crt_android_fcntl_bi__
#define __crt_android_fcntl_bi__
#include once "crt/sys/types.bi"

#define O_ACCMODE &o3
#define O_RDONLY &o0
#define O_WRONLY &o1
#define O_RDWR &o2
#define O_CREAT &o100
#define O_EXCL &o200
#define O_NOCTTY &o400
#define O_TRUNC &o1000
#define O_APPEND &o2000
#define O_NONBLOCK &o4000
#define O_NDELAY O_NONBLOCK
#define O_DSYNC &o10000
#define O_SYNC &o4010000
#define O_CLOEXEC &o2000000

'' Bionic retains the kernel ABI flags. ARM and AArch64 differ from x86;
'' do not import the glibc x86 values or its LP64 O_LARGEFILE substitution.
'' Reference: Android NDK sysroot asm/fcntl.h and asm-generic/fcntl.h.
#if defined(__FB_ARM__) Or defined(__FB_AARCH64__)
#define O_DIRECTORY &o40000
#define O_NOFOLLOW &o100000
#define O_DIRECT &o200000
#define O_LARGEFILE &o400000
#else
#define O_DIRECTORY &o200000
#define O_NOFOLLOW &o400000
#define O_DIRECT &o40000
#define O_LARGEFILE &o100000
#endif
#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_DUPFD_CLOEXEC 1030
#define FD_CLOEXEC 1
#define F_ULOCK 0
#define F_LOCK 1
#define F_TLOCK 2
#define F_TEST 3

Extern "C"
Declare Function open_ Alias "open" (ByVal path As Const ZString Ptr, ByVal flags As Long, ...) As Long
Declare Function creat (ByVal path As Const ZString Ptr, ByVal mode As mode_t) As Long
Declare Function fcntl (ByVal descriptor As Long, ByVal operation As Long, ...) As Long
'' off_t is the Bionic public width selected by crt/sys/types.bi. lockf is
'' available from Android API 24; callers targeting older releases must gate it.
Declare Function lockf (ByVal descriptor As Long, ByVal operation As Long, ByVal length As off_t) As Long
End Extern
#endif
'' end of crt/android/fcntl.bi
