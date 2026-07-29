''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-intrinsic-location-edge-cases.bas
''
'' Purpose:
''     Exercise preprocessor intrinsic values reached through macro expansion.
''
'' Responsibilities:
''     Verify that line, function, and file callbacks use the invocation site.
''
'' This file intentionally does not contain:
''     Assumptions about a fixed source path.
''
' TEST_MODE : COMPILE_ONLY_OK

#define PP_INTRINSIC_LINE(dummy) __LINE__
#define PP_INTRINSIC_FUNCTION() __FUNCTION__
#define PP_INTRINSIC_FILE() __FILE__

'' __LINE__ must describe the call site, including a continued invocation.
#assert PP_INTRINSIC_LINE(0) = __LINE__
#assert PP_INTRINSIC_LINE( _
	0) = __LINE__

sub pp_intrinsic_location()
	const direct_function = __FUNCTION__
	const macro_function = PP_INTRINSIC_FUNCTION()
	const direct_file = __FILE__
	const macro_file = PP_INTRINSIC_FILE()

	#assert direct_function = macro_function
	#assert direct_file = macro_file
end sub

'' end of macro-intrinsic-location-edge-cases.bas
