/'
	Project: FreeBASIC C backend regression controls
	File: c-conversion-results.bas
	Purpose: Preserve typed conversion results, rounding and single evaluation.
	Responsibilities: Integer-to-float round trips, native 128-bit limits and
	comparison of the observable floating state in every rounding mode.
	This file intentionally does NOT contain: Invalid conversion execution or
	non-C backend 128-bit ABI probes. All state belongs to one test process.
'/
#lang "fb"

extern "C"
	declare function FPBegin alias "fpcr_begin"(byval mode as long) as long
	declare function FPDouble alias "fpcr_double"(byval index as long) as double
	declare function FPSingle alias "fpcr_single"(byval index as long) as single
	declare function FPLargeDouble alias "fpcr_large_double"(byval index as long) as double
	declare function FPLargeSingle alias "fpcr_large_single"(byval index as long) as single
	declare function FPExpected alias "fpcr_expected"(byval mode as long, byval index as long) as long
	declare function FPCalls alias "fpcr_calls"() as long
	declare function FPFlags alias "fpcr_flags"() as long
	declare function FPMode alias "fpcr_mode"() as long
	declare sub FPFinish alias "fpcr_finish"()
end extern

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_CONVERSION_RESULTS_FAILED", __LINE__
		end 1
	end if
#endmacro

#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif

for mode as long = 0 to 3
	for case_index as long = 0 to 15
		dim as long expected = FPExpected(mode, case_index)
		for variant as long = 0 to 9
#ifdef CHECK_NON_C_BACKEND
			if variant >= 4 then continue for
#endif
			if variant >= 8 andalso expected < 0 then continue for
			VERIFY(FPBegin(mode) = 0)
			select case variant
			case 0: VERIFY(cdbl(clngint(FPDouble(case_index))) = cdbl(expected))
			case 1: VERIFY(csng(clngint(FPDouble(case_index))) = csng(expected))
			case 2: VERIFY(cdbl(clngint(FPSingle(case_index))) = cdbl(expected))
			case 3: VERIFY(csng(clngint(FPSingle(case_index))) = csng(expected))
#ifndef CHECK_NON_C_BACKEND
			case 4: VERIFY(cint<128>(FPDouble(case_index)) = expected)
			case 5: VERIFY(cint<128>(FPSingle(case_index)) = expected)
			case 6: VERIFY(cdbl(cint<128>(FPDouble(case_index))) = cdbl(expected))
			case 7: VERIFY(csng(cint<128>(FPSingle(case_index))) = csng(expected))
			case 8: VERIFY(cuint<128>(FPDouble(case_index)) = cuint<128>(expected))
			case 9: VERIFY(cuint<128>(FPSingle(case_index)) = cuint<128>(expected))
#endif
			end select
			VERIFY(FPCalls() = 1)
			VERIFY(FPMode() = mode)
			print mode; ","; case_index; ","; variant; ","; FPFlags()
		next
	next
#ifndef CHECK_NON_C_BACKEND
	'' Each floating format's predecessor is one representable step below its
	'' upper conversion limit. Construct the expected bits with integer shifts.
	dim as uinteger<128> one = 1
	VERIFY(FPBegin(mode) = 0)
	VERIFY(cint<128>(FPLargeDouble(0)) = cast(integer<128>, one shl 127))
	VERIFY(cint<128>(FPLargeDouble(1)) = cast(integer<128>, (one shl 127) - (one shl 74)))
	VERIFY(cuint<128>(FPLargeDouble(2)) = (not cuint<128>(0)) - ((one shl 75) - 1))
	VERIFY(cint<128>(FPLargeDouble(3)) = -cast(integer<128>, one shl 100))
	VERIFY(cint<128>(FPLargeDouble(4)) = cast(integer<128>, one shl 100))
	VERIFY(cint<128>(FPLargeSingle(0)) = cast(integer<128>, one shl 127))
	VERIFY(cint<128>(FPLargeSingle(1)) = cast(integer<128>, (one shl 127) - (one shl 103)))
	VERIFY(cuint<128>(FPLargeSingle(2)) = (not cuint<128>(0)) - ((one shl 104) - 1))
	VERIFY(cint<128>(FPLargeSingle(3)) = -cast(integer<128>, one shl 100))
	VERIFY(cint<128>(FPLargeSingle(4)) = cast(integer<128>, one shl 100))
	VERIFY(FPCalls() = 10)
	VERIFY(FPMode() = mode)
#endif
next
FPFinish()
print "C_CONVERSION_RESULTS_PASS"

/' end of c-conversion-results.bas '/
