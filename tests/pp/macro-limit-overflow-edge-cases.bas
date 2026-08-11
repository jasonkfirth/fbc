''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-limit-overflow-edge-cases.bas
''
'' Purpose:
''     Exercise the first unsupported macro parameter count.
''
'' Responsibilities:
''     Ensure that the 33rd parameter is rejected without overrunning the
''     32-entry argument table.
''
'' This file intentionally does not contain:
''     The accepted 32-parameter boundary, which is covered by the positive test.
''
' TEST_MODE : COMPILE_ONLY_FAIL

#define PP_LIMIT_ARGUMENT_33( _
	a01, a02, a03, a04, a05, a06, a07, a08, _
	a09, a10, a11, a12, a13, a14, a15, a16, _
	a17, a18, a19, a20, a21, a22, a23, a24, _
	a25, a26, a27, a28, a29, a30, a31, a32, a33 _
) a33

'' end of macro-limit-overflow-edge-cases.bas
