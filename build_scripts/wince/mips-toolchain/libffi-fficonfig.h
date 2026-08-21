/*
 * FreeBASIC Windows CE MIPS libffi configuration
 * ------------------------------------------------
 *
 * File: wince/mips-toolchain/libffi-fficonfig.h
 *
 * Purpose:
 *
 *     Describe the fixed MIPS O32 Windows CE ABI to a direct libffi build.
 *
 * Responsibilities:
 *
 *     - identify available ISO C headers and functions
 *     - define the 32-bit little-endian type sizes
 *     - disable ELF unwind and executable-memory assumptions
 *
 * This file intentionally does NOT contain:
 *
 *     - host feature detection
 *     - another CPU ABI
 *     - shared-library or symbol-versioning policy
 */

#ifndef FB_WINCE_MIPS_LIBFFI_CONFIG_H
#define FB_WINCE_MIPS_LIBFFI_CONFIG_H

#define EH_FRAME_FLAGS "a"
#define HAVE_INTTYPES_H 1
#define HAVE_MEMCPY 1
#define HAVE_RO_EH_FRAME 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define LT_OBJDIR ".libs/"
#define PACKAGE "libffi"
#define PACKAGE_BUGREPORT "http://github.com/libffi/libffi/issues"
#define PACKAGE_NAME "libffi"
#define PACKAGE_STRING "libffi 3.5.2"
#define PACKAGE_TARNAME "libffi"
#define PACKAGE_URL ""
#define PACKAGE_VERSION "3.5.2"
#define SIZEOF_DOUBLE 8
#define SIZEOF_LONG_DOUBLE 8
#define SIZEOF_SIZE_T 4
#define STDC_HEADERS 1
#define VERSION "3.5.2"

#ifdef LIBFFI_ASM
#define FFI_HIDDEN(name)
#else
#define FFI_HIDDEN
#endif

#endif

/* end of wince/mips-toolchain/libffi-fficonfig.h */
