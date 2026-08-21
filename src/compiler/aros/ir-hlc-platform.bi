''
'' FreeBASIC compiler
'' ------------------
''
'' File: aros/ir-hlc-platform.bi
''
'' Purpose:
''
''     Isolate the AROS m68k GCC NaN-literal workaround.
''
'' Responsibilities:
''
''     - detect the AROS m68k C backend combination
''     - request the exact IEEE quiet-NaN payload from GCC 6.5
''     - preserve source-level unary negation
''
'' This file intentionally does NOT contain:
''
''     - generic m68k code-generation policy
''     - behavior for other AROS architectures
''     - linker or runtime-library selection
''
'' Toolchain behavior:
''
''     The AROS m68k GCC maps an empty __builtin_nan payload to all fraction
''     bits set.  FreeBASIC constants use the canonical quiet bit and a zero
''     payload, so request that representation explicitly on this target.
''

#ifndef __IR_HLC_AROS_PLATFORM_BI__
#define __IR_HLC_AROS_PLATFORM_BI__

private function irHlcArosEmitNan _
	( _
		byval dtype as integer, _
		byval isnegative as integer, _
		byref text as string _
	) as integer

	if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_AROS ) then
		return FALSE
	end if
	if( fbGetCpuFamily( ) <> FB_CPUFAMILY_M68K ) then
		return FALSE
	end if

	if( dtype = FB_DATATYPE_DOUBLE ) then
		text = "__builtin_nan( ""0x8000000000000"" )"
	else
		text = "__builtin_nanf( ""0x400000"" )"
	end if

	if( isnegative ) then
		text = "(-" + text + ")"
	end if

	function = TRUE
end function

#endif

'' end of aros/ir-hlc-platform.bi
