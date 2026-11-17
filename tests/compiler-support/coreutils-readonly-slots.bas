/'
    Project FreeBASIC Coreutils callback qualification
    File coreutils-readonly-slots.bas
    Purpose Exercise borrowed pointer slots through actual library helpers.
    Responsibilities Export native entry points for value and slot checks.
    Complete command execution has its own application qualification.
'/
#lang "fb"
#include once "crt/long.bi"

' These scalar mappings come from the production candidates. The generated
' contract contains their complete callback bodies and hash provider.
Type c2fb_c_int As Long
Type c2fb_c_long As CLong
Type c2fb_c_ulong As CULong
Type c2fb_int64 As LongInt
Type c2fb_uint64 As ULongInt

Declare Function c2fb_crt_strcmp CDecl Alias "strcmp" (ByVal a As Const ZString Ptr, ByVal b As Const ZString Ptr) As c2fb_c_int
Declare Function c2fb_crt_strchr CDecl Alias "strchr" (ByVal text As Const ZString Ptr, ByVal character As c2fb_c_int) As ZString Ptr
Declare Function c2fb_external_memeq CDecl Alias "memeq" (ByVal a As Const Any Ptr, ByVal b As Const Any Ptr, ByVal length As CULong) As Byte
Declare Function c2fb_external_hash_pjw_bare CDecl Alias "hash_pjw_bare" (ByVal data As Const Any Ptr, ByVal length As CULong) As CULong

#include once "coreutils-readonly-slots-contract.bi"

Function compareUsers CDecl Alias "fb_test_slots_compare" (ByVal a As Const Any Ptr, ByVal b As Const Any Ptr) As Long
    Return userid_compare(a, b)
End Function

Function compareEnvironment CDecl Alias "fb_test_slots_environment" (ByVal a As Const Any Ptr, ByVal b As Const Any Ptr) As Byte
    Return env_vector_slot_compare(a, b)
End Function

Function hashEnvironment CDecl Alias "fb_test_slots_hash" (ByVal slot As Const Any Ptr, ByVal tableSize As CULong) As CULong
    Return env_vector_slot_hash(slot, tableSize)
End Function

/' end of coreutils-readonly-slots.bas '/
