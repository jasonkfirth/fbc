/'
	Project: FreeBASIC C backend regression controls
	File: c-unobserved-arguments.bas
	Purpose: Omit unobserved literal addresses while preserving actual evaluation.
	Responsibilities: Narrow/wide literals, mixed parameters, indirect calls,
	native ABI entry points, getter evaluation and mutable argument boundaries.
	This file intentionally does NOT contain: Platform APIs or threading.
'/
#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_UNOBSERVED_ARGUMENTS_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as long literal_calls, mutable_calls, carrier_calls, carrier_copies, carrier_dtors
dim shared as zstring * 16 mutable_buffer = "before"

function IgnoredNarrow(byval text as zstring ptr) as long
	return 11
end function
function IgnoredWide(byval text as wstring ptr) as long
	return 13
end function
function IgnoredPair(byval first as zstring ptr, byval second as wstring ptr) as long
	return 17
end function
function MixedArgument(byval text as zstring ptr, byval value as long) as long
	return value + 1
end function

type ValueCarrier
	value as long
	declare constructor(byval initial as long)
	declare constructor(byref other as const ValueCarrier)
	declare destructor()
end type
constructor ValueCarrier(byval initial as long)
	carrier_calls += 1
	value = initial
end constructor
constructor ValueCarrier(byref other as const ValueCarrier)
	carrier_copies += 1
	value = other.value
end constructor
destructor ValueCarrier()
	carrier_dtors += 1
end destructor
function IgnoredCarrier(byval value as ValueCarrier) as long
	return 29
end function

function ReadonlyLiteral() as const zstring ptr
	literal_calls += 1
	return strptr("getter")
end function
function MutableView() as const zstring ptr
	mutable_calls += 1
	return @mutable_buffer
end function
sub MutateText(byval text as zstring ptr)
	text[0] = asc("X")
end sub
function PointerObserved(byval text as zstring ptr) as long
	return text <> 0
end function

function NativeEntry cdecl alias "r91_native_entry"(byval text as zstring ptr, byval value as long) as long
	return value + 4
end function
#ifdef CHECK_NATIVE_CALLER
declare function NativeProbe cdecl alias "r91_native_probe"() as long
	VERIFY(NativeProbe() = 42)
#endif

VERIFY(IgnoredNarrow("unused-narrow") = 11)
VERIFY(IgnoredWide(wstr("unused-wide")) = 13)
VERIFY(IgnoredPair("unused-first", wstr("unused-second")) = 17)
VERIFY(MixedArgument("unused-mixed", 41) = 42)
scope
	dim carrier as ValueCarrier = ValueCarrier(41)
	VERIFY(carrier_calls = 1)
	VERIFY(IgnoredCarrier(carrier) = 29)
	VERIFY(carrier_copies = 1)
	VERIFY(carrier_dtors = 1)
end scope
VERIFY(carrier_dtors = 2)

'' Nonliteral expressions must still run even when the parameter is ignored.
VERIFY(IgnoredNarrow(cptr(zstring ptr, ReadonlyLiteral())) = 11)
VERIFY(literal_calls = 1)
MutateText(cptr(zstring ptr, MutableView()))
VERIFY(mutable_calls = 1)
VERIFY(mutable_buffer = "Xefore")

'' The pointer value is observable, the native alias has a separate contract,
'' and an indirect call does not establish which body will receive the value.
VERIFY(PointerObserved("observed") <> 0)
VERIFY(NativeEntry("native", 38) = 42)
dim reader as function(byval as zstring ptr) as long = @IgnoredNarrow
VERIFY(reader("callback") = 11)
#ifdef CHECK_OPAQUE
asm
	nop
end asm
#endif
print "C_UNOBSERVED_ARGUMENTS_PASS"

'' end of c-unobserved-arguments.bas
