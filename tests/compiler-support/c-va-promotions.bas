/'
	Project: FreeBASIC C backend regression
	File: c-va-promotions.bas
	Purpose: Preserve native unsigned argument ranges through C variadic calls.
	Responsibilities: Check default promotions, indirect calls and format diagnostics.
	The paired C fixture supplies reference output and native argument inspection.
'/
#lang "fb"
#include once "crt/stdio.bi"

declare function promotionNative cdecl alias "fb_test_promotion_native"(byval expectedByte as long, byval expectedShort as long, ...) as long
declare function promotionCases cdecl alias "fb_test_promotion_cases"() as long
declare function promotionInput cdecl alias "fb_test_promotion_input"() as const zstring ptr

function promotionBasic cdecl alias "fb_test_promotion_basic"(byval expectedByte as long, byval expectedShort as long, ...) as long
	dim as cva_list args = any
	cva_start(args, expectedShort)
	var actualByte = cva_arg(args, long)
	var actualShort = cva_arg(args, long)
	cva_end(args)
	return (actualByte = expectedByte) and (actualShort = expectedShort)
end function

function promotionFormat cdecl alias "fb_test_promotion_format"(byval destination as zstring ptr, byval capacity as size_t, byval byteValue as ubyte, byval shortValue as ushort) as long
	if promotionNative(byteValue, shortValue, byteValue, shortValue) <> -1 then return -1
	var callback = procptr(promotionNative)
	if callback(byteValue, shortValue, byteValue, shortValue) <> -1 then return -2
	if promotionBasic(byteValue, shortValue, byteValue, shortValue) <> -1 then return -3
#ifdef PROMOTION_EXPLICIT_LONG
	return snprintf(destination, capacity, strptr(!"%x %o"), clng(byteValue), clng(shortValue))
#elseif defined(PROMOTION_WRONG_ARGUMENT)
	return snprintf(destination, capacity, strptr(!"%s"), byteValue)
#elseif defined(PROMOTION_WRONG_WIDE_ARGUMENT)
	return snprintf(destination, capacity, strptr(!"%s"), shortValue)
#elseif defined(PROMOTION_WRONG_SIGNED_ARGUMENT)
	return snprintf(destination, capacity, strptr(!"%s"), cbyte(byteValue))
#elseif defined(PROMOTION_DYNAMIC_FORMAT)
	return snprintf(destination, capacity, promotionInput(), byteValue, shortValue)
#else
	return snprintf(destination, capacity, strptr(!"%x %o %hx %ho"), byteValue, byteValue, shortValue, shortValue)
#endif
end function

#ifndef PROMOTION_EXPORTS
assert(promotionCases() = -1)
print "VA_PROMOTIONS_PASS"
#endif

/' end of c-va-promotions.bas '/
