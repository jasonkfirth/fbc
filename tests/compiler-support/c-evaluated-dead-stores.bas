/'
	Project: FreeBASIC C backend regression controls
	File: c-evaluated-dead-stores.bas
	Purpose: Preserve evaluation when private result storage is unread.
	Responsibilities: Rounding, floating exceptions, calls in discarded values,
	plain floating copies and observable scalar conversion results.
	This file intentionally does NOT contain: Platform-specific floating constants.
'/

extern "C"
	declare function FPBegin cdecl alias "fp_begin" (byval mode as long) as long
	declare function FPOperand cdecl alias "fp_operand" (byval index as long) as double
	declare function FPIndex cdecl alias "fp_index" () as long
	declare function FPFlags cdecl alias "fp_flags" () as long
	declare function FPCalls cdecl alias "fp_calls" () as long
	declare function FPMode cdecl alias "fp_mode" () as long
	declare function FPExpected cdecl alias "fp_expected" (byval mode as long) as long
	declare sub FPFinish cdecl alias "fp_finish" ()
end extern

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_EVALUATED_DEAD_STORES_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as double stored_value
dim shared as double stored_array(0 to 0)

sub UnreadInteger64()
	dim as longint unused_value = FPOperand(0)
end sub

sub UnreadInteger32()
	dim as long unused_value = FPOperand(1)
end sub

sub UnreadInteger16()
	dim as short unused_value = FPOperand(0)
end sub

sub UnreadInteger8()
	dim as byte unused_value = FPOperand(0)
end sub

sub UnreadUnsigned64()
	dim as ulongint unused_value = FPOperand(0)
end sub

sub UnreadInteger128()
	dim as integer<128> unused_value = FPOperand(0)
end sub

sub UnreadUnsigned128()
	dim as uinteger<128> unused_value = FPOperand(0)
end sub

sub UnreadBoolean()
	dim as boolean unused_value = FPOperand(7)
end sub

sub UnreadUnsignedBoundary()
	dim as ulongint unused_value = FPOperand(4)
end sub

sub UnreadDivision()
	dim as double unused_value = FPOperand(2) / FPOperand(3)
end sub

sub UnreadOverflow()
	dim as double unused_value = FPOperand(4) * FPOperand(5)
end sub

sub UnreadInvalid()
	dim as double unused_value = FPOperand(6) / FPOperand(6)
end sub

sub UnreadCopy()
	dim as double unused_value = stored_value
end sub

sub UnreadIndexedCopy()
	dim as double unused_value = stored_array(FPIndex())
end sub

sub UnreadOutOfRange()
	dim as long unused_value = FPOperand(4)
end sub

'' Populate signaling-NaN copies before clearing the exception state for a case.
stored_value = FPOperand(7)
stored_array(0) = stored_value
for mode as long = 0 to 3
	for case_index as long = 0 to 15
		dim as long expected_calls = 1
		dim as long live_result = 0
		VERIFY(FPBegin(mode) = 0)
		select case case_index
		case 0: UnreadInteger64()
		case 1: UnreadInteger32()
		case 2: UnreadInteger16()
		case 3: UnreadDivision(): expected_calls = 2
		case 4: UnreadOverflow(): expected_calls = 2
		case 5: UnreadInvalid(): expected_calls = 2
		case 6: UnreadCopy(): expected_calls = 0
		case 7: UnreadIndexedCopy()
		case 8
			live_result = FPOperand(0)
			VERIFY(live_result = FPExpected(mode))
		case 9: UnreadOutOfRange()
		case 10: UnreadInteger8()
		case 11: UnreadUnsigned64()
		case 12: UnreadInteger128()
		case 13: UnreadUnsigned128()
		case 14: UnreadBoolean()
		case 15: UnreadUnsignedBoundary()
		end select
		VERIFY(FPCalls() = expected_calls)
		VERIFY(FPMode() = mode)
		print mode; ","; case_index; ","; FPFlags(); ","; FPCalls(); ","; live_result
	next
next
FPFinish()
print "C_EVALUATED_DEAD_STORES_PASS"

/' end of c-evaluated-dead-stores.bas '/
