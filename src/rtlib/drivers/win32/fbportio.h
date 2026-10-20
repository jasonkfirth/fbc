#if 0
/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/fbportio.h
 * Purpose: Provide fbportio runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 * C and BASIC tools share these definitions. Hide the documentation from the
 * BASIC parser, which does not accept C comment syntax.
 */
#endif
#define FBPORTIO_VERSION    256
#define FBPORTIO            32768
#define IOCTL_GRANT_IOPM    CTL_CODE(FBPORTIO, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_VERSION   CTL_CODE(FBPORTIO, 2049, METHOD_BUFFERED, FILE_ANY_ACCESS)

#if 0
/* end of drivers/win32/fbportio.h */
#endif
