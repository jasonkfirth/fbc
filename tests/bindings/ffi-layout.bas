'' Project: FreeBASIC external library checks
'' File: ffi-layout.bas
'' Purpose: Report the layout exposed by the FreeBASIC libffi binding.
'' Responsibilities: Emit the same measurements as the native C header probe.
'' This file contains no native ABI assumptions or libffi implementation.
#lang "fb"
#include once "ffi.bi"

print "pointer="; sizeof(any ptr)
print "ffi_arg="; sizeof(ffi_arg)
print "ffi_type="; sizeof(ffi_type)
print "ffi_type.elements="; offsetof(ffi_type, elements)
print "ffi_cif="; sizeof(ffi_cif)
print "ffi_cif.arg_types="; offsetof(ffi_cif, arg_types)
print "ffi_cif.rtype="; offsetof(ffi_cif, rtype)
print "ffi_cif.bytes="; offsetof(ffi_cif, bytes)
print "ffi_cif.flags="; offsetof(ffi_cif, flags)
print "ffi_closure="; sizeof(ffi_closure)
print "ffi_closure.cif="; offsetof(ffi_closure, cif)
print "ffi_closure.fun="; offsetof(ffi_closure, fun)
print "ffi_closure.user_data="; offsetof(ffi_closure, user_data)
print "FFI_FIRST_ABI="; FFI_FIRST_ABI
print "FFI_LAST_ABI="; FFI_LAST_ABI
print "FFI_DEFAULT_ABI="; FFI_DEFAULT_ABI
print "FFI_TRAMPOLINE_SIZE="; FFI_TRAMPOLINE_SIZE

'' end of ffi-layout.bas
