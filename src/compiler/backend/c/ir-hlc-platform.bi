'' Project: FreeBASIC compiler - C source backend
'' -----------------------------------------
''
'' File: backend/c/ir-hlc-platform.bi
''
'' Purpose:
''
''     Dispatch exceptional target-specific C literal spellings.
''
'' Responsibilities:
''
''     - serialize typed IR, data layouts, and control flow as C
''     - apply C ABI and compiler interoperability requirements
''
'' This file intentionally does NOT contain:
''
''     - BASIC grammar or execution of the external C compiler
''

''
'' FreeBASIC compiler
'' ------------------
''
'' File: ir-hlc-platform.bi
''
'' Purpose:
''
''     Dispatch platform-specific C backend literal emission.
''
'' Responsibilities:
''
''     - include backend policy owned by platform directories
''     - offer a target-neutral hook for exceptional literal spellings
''     - leave ordinary C backend emission in ir-hlc.bas
''
'' This file intentionally does NOT contain:
''
''     - target-specific literal spellings
''     - compiler-driver or linker policy
''     - generic CPU-family behavior
''

#ifndef __IR_HLC_PLATFORM_BI__
#define __IR_HLC_PLATFORM_BI__

#include once "backend/c/platforms/aros/ir-hlc-platform.bi"

private function irHlcPlatformEmitNan _
	( _
		byval dtype as integer, _
		byval isnegative as integer, _
		byref text as string _
	) as integer

	if( irHlcArosEmitNan( dtype, isnegative, text ) ) then
		return TRUE
	end if

	function = FALSE
end function

#endif

'' end of backend/c/ir-hlc-platform.bi
