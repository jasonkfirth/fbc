/'
	Project: FreeBASIC C backend regression
	File: c-literal-offsets.bas
	Purpose: Check bounded literal suffixes and native format diagnostics.
	Responsibilities: Preserve pointer identity, condition evaluation and casts.
	The paired C observer owns the call counter and address comparison.
'/
#lang "fb"
#include once "crt/stdio.bi"

type LiteralOffsetPicker as function cdecl(byval as long) as const zstring ptr
declare function literalOffsetArgument cdecl alias "probe_literal_offset_argument"(byval value as long) as long
declare sub literalOffsetReset cdecl alias "probe_literal_offset_reset"()
declare function literalOffsetCalls cdecl alias "probe_literal_offset_calls"() as long
declare function literalOffsetObserve cdecl alias "probe_literal_offset_observe"(byval picker as LiteralOffsetPicker) as long

function literalOffsetPick cdecl alias "probe_literal_offset_pick"(byval condition as long) as const zstring ptr
	return cptr(const zstring ptr, cptr(const ubyte ptr, strptr(!" %d")) + (-clng(literalOffsetArgument(condition) = 0)))
end function

function literalOffsetWrite cdecl alias "probe_literal_offset_write"(byval destination as zstring ptr, byval capacity as uinteger, byval condition as long, byval mode as long) as long
	select case mode
	case 0
		return snprintf(destination, capacity, cptr(const zstring ptr, cptr(const ubyte ptr, strptr(!" %d")) + (-clng(literalOffsetArgument(condition) = 0))), clng(23))
	case 1
		return snprintf(destination, capacity, cptr(const zstring ptr, (-clng(literalOffsetArgument(condition) = 0)) + cptr(const ubyte ptr, strptr(!" %d"))), clng(23))
	case else
		return snprintf(destination, capacity, cptr(const zstring ptr, cptr(const ubyte ptr, strptr(!" %d")) + (-clng(cbyte(clng(literalOffsetArgument(condition) = 0))))), clng(23))
	end select
end function

#ifndef LITERAL_OFFSET_EXPORTS
dim as zstring * 64 bufferText
dim as long written

#ifdef LITERAL_OFFSET_BAD_ARGUMENT
	written = snprintf(@bufferText, sizeof(bufferText), cptr(const zstring ptr, cptr(const ubyte ptr, strptr(!" %d")) + (-clng(literalOffsetArgument(0) = 0))), strptr("wrong"))
#elseif defined(LITERAL_OFFSET_DYNAMIC)
	written = snprintf(@bufferText, sizeof(bufferText), cptr(const zstring ptr, cptr(const ubyte ptr, strptr(!" %d")) + literalOffsetArgument(0)), clng(23))
#elseif defined(LITERAL_OFFSET_UNSIGNED)
	written = snprintf(@bufferText, sizeof(bufferText), cptr(const zstring ptr, cptr(const ubyte ptr, strptr(!" %d")) + (-clng(cubyte(clng(literalOffsetArgument(0) = 0))))), clng(23))
#elseif defined(LITERAL_OFFSET_EMPTY)
	written = snprintf(@bufferText, sizeof(bufferText), cptr(const zstring ptr, cptr(const ubyte ptr, strptr(!"")) + (-clng(literalOffsetArgument(0) = 0))), clng(23))
#elseif defined(LITERAL_OFFSET_NEGATIVE)
	written = snprintf(@bufferText, sizeof(bufferText), cptr(const zstring ptr, cptr(const ubyte ptr, strptr(!" %d")) + clng(literalOffsetArgument(0) = 0)), clng(23))
#else
	dim as long conditions(0 to 4) = {-2147483647 - 1, -1, 0, 1, 2147483647}
	for position as integer = lbound(conditions) to ubound(conditions)
		var condition = conditions(position)
		var expected = iif(condition = 0, "23", " 23")
		literalOffsetReset()
		written = snprintf(@bufferText, sizeof(bufferText), cptr(const zstring ptr, cptr(const ubyte ptr, strptr(!" %d")) + (-clng(literalOffsetArgument(condition) = 0))), clng(23))
		assert(bufferText = expected and written = len(expected))
		assert(literalOffsetCalls() = 1)

		literalOffsetReset()
		written = snprintf(@bufferText, sizeof(bufferText), cptr(const zstring ptr, (-clng(literalOffsetArgument(condition) = 0)) + cptr(const ubyte ptr, strptr(!" %d"))), clng(23))
		assert(bufferText = expected and written = len(expected))
		assert(literalOffsetCalls() = 1)

		literalOffsetReset()
		written = snprintf(@bufferText, sizeof(bufferText), cptr(const zstring ptr, cptr(const ubyte ptr, strptr(!" %d")) + (-clng(cbyte(clng(literalOffsetArgument(condition) = 0))))), clng(23))
		assert(bufferText = expected and written = len(expected))
		assert(literalOffsetCalls() = 1)
	next
	literalOffsetReset()
	assert(literalOffsetObserve(@literalOffsetPick) = -1)
	assert(literalOffsetCalls() = 2)
	print "LITERAL_OFFSETS_PASS"
#endif

#endif

/' end of c-literal-offsets.bas '/
