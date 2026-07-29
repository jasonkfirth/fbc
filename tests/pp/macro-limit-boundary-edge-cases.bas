''
'' FreeBASIC Compiler Test Suite
'' -----------------------------
''
'' File: macro-limit-boundary-edge-cases.bas
''
'' Purpose:
''     Exercise the highest supported macro parameter and expansion depths.
''
'' Responsibilities:
''     Check both sides of the declared 32-parameter boundary and the complete
''     64-entry active expansion stack.
''
'' This file intentionally does not contain:
''     Inputs beyond the limits declared by the compiler.
''
' TEST_MODE : COMPILE_ONLY_OK

'' Thirty-one parameters are accepted today and provide the lower boundary.
#define PP_LIMIT_ARGUMENT_31( _
	a01, a02, a03, a04, a05, a06, a07, a08, _
	a09, a10, a11, a12, a13, a14, a15, a16, _
	a17, a18, a19, a20, a21, a22, a23, a24, _
	a25, a26, a27, a28, a29, a30, a31 _
) a31

#assert PP_LIMIT_ARGUMENT_31( _
	1, 2, 3, 4, 5, 6, 7, 8, _
	9, 10, 11, 12, 13, 14, 15, 16, _
	17, 18, 19, 20, 21, 22, 23, 24, _
	25, 26, 27, 28, 29, 30, 42 _
) = 42

'' FB_MAXDEFINEARGS is 32 and all related tables contain slots 0 through 31.
#define PP_LIMIT_ARGUMENT_32( _
	a01, a02, a03, a04, a05, a06, a07, a08, _
	a09, a10, a11, a12, a13, a14, a15, a16, _
	a17, a18, a19, a20, a21, a22, a23, a24, _
	a25, a26, a27, a28, a29, a30, a31, a32 _
) a32

#assert PP_LIMIT_ARGUMENT_32( _
	1, 2, 3, 4, 5, 6, 7, 8, _
	9, 10, 11, 12, 13, 14, 15, 16, _
	17, 18, 19, 20, 21, 22, 23, 24, _
	25, 26, 27, 28, 29, 30, 31, 42 _
) = 42

'' PP_LIMIT_DEPTH_63 expands through 64 active macro definitions. This fills
'' the macro stack without stepping beyond its declared storage.
#define PP_LIMIT_DEPTH_00 42
#define PP_LIMIT_DEPTH_01 PP_LIMIT_DEPTH_00
#define PP_LIMIT_DEPTH_02 PP_LIMIT_DEPTH_01
#define PP_LIMIT_DEPTH_03 PP_LIMIT_DEPTH_02
#define PP_LIMIT_DEPTH_04 PP_LIMIT_DEPTH_03
#define PP_LIMIT_DEPTH_05 PP_LIMIT_DEPTH_04
#define PP_LIMIT_DEPTH_06 PP_LIMIT_DEPTH_05
#define PP_LIMIT_DEPTH_07 PP_LIMIT_DEPTH_06
#define PP_LIMIT_DEPTH_08 PP_LIMIT_DEPTH_07
#define PP_LIMIT_DEPTH_09 PP_LIMIT_DEPTH_08
#define PP_LIMIT_DEPTH_10 PP_LIMIT_DEPTH_09
#define PP_LIMIT_DEPTH_11 PP_LIMIT_DEPTH_10
#define PP_LIMIT_DEPTH_12 PP_LIMIT_DEPTH_11
#define PP_LIMIT_DEPTH_13 PP_LIMIT_DEPTH_12
#define PP_LIMIT_DEPTH_14 PP_LIMIT_DEPTH_13
#define PP_LIMIT_DEPTH_15 PP_LIMIT_DEPTH_14
#define PP_LIMIT_DEPTH_16 PP_LIMIT_DEPTH_15
#define PP_LIMIT_DEPTH_17 PP_LIMIT_DEPTH_16
#define PP_LIMIT_DEPTH_18 PP_LIMIT_DEPTH_17
#define PP_LIMIT_DEPTH_19 PP_LIMIT_DEPTH_18
#define PP_LIMIT_DEPTH_20 PP_LIMIT_DEPTH_19
#define PP_LIMIT_DEPTH_21 PP_LIMIT_DEPTH_20
#define PP_LIMIT_DEPTH_22 PP_LIMIT_DEPTH_21
#define PP_LIMIT_DEPTH_23 PP_LIMIT_DEPTH_22
#define PP_LIMIT_DEPTH_24 PP_LIMIT_DEPTH_23
#define PP_LIMIT_DEPTH_25 PP_LIMIT_DEPTH_24
#define PP_LIMIT_DEPTH_26 PP_LIMIT_DEPTH_25
#define PP_LIMIT_DEPTH_27 PP_LIMIT_DEPTH_26
#define PP_LIMIT_DEPTH_28 PP_LIMIT_DEPTH_27
#define PP_LIMIT_DEPTH_29 PP_LIMIT_DEPTH_28
#define PP_LIMIT_DEPTH_30 PP_LIMIT_DEPTH_29
#define PP_LIMIT_DEPTH_31 PP_LIMIT_DEPTH_30
#define PP_LIMIT_DEPTH_32 PP_LIMIT_DEPTH_31
#define PP_LIMIT_DEPTH_33 PP_LIMIT_DEPTH_32
#define PP_LIMIT_DEPTH_34 PP_LIMIT_DEPTH_33
#define PP_LIMIT_DEPTH_35 PP_LIMIT_DEPTH_34
#define PP_LIMIT_DEPTH_36 PP_LIMIT_DEPTH_35
#define PP_LIMIT_DEPTH_37 PP_LIMIT_DEPTH_36
#define PP_LIMIT_DEPTH_38 PP_LIMIT_DEPTH_37
#define PP_LIMIT_DEPTH_39 PP_LIMIT_DEPTH_38
#define PP_LIMIT_DEPTH_40 PP_LIMIT_DEPTH_39
#define PP_LIMIT_DEPTH_41 PP_LIMIT_DEPTH_40
#define PP_LIMIT_DEPTH_42 PP_LIMIT_DEPTH_41
#define PP_LIMIT_DEPTH_43 PP_LIMIT_DEPTH_42
#define PP_LIMIT_DEPTH_44 PP_LIMIT_DEPTH_43
#define PP_LIMIT_DEPTH_45 PP_LIMIT_DEPTH_44
#define PP_LIMIT_DEPTH_46 PP_LIMIT_DEPTH_45
#define PP_LIMIT_DEPTH_47 PP_LIMIT_DEPTH_46
#define PP_LIMIT_DEPTH_48 PP_LIMIT_DEPTH_47
#define PP_LIMIT_DEPTH_49 PP_LIMIT_DEPTH_48
#define PP_LIMIT_DEPTH_50 PP_LIMIT_DEPTH_49
#define PP_LIMIT_DEPTH_51 PP_LIMIT_DEPTH_50
#define PP_LIMIT_DEPTH_52 PP_LIMIT_DEPTH_51
#define PP_LIMIT_DEPTH_53 PP_LIMIT_DEPTH_52
#define PP_LIMIT_DEPTH_54 PP_LIMIT_DEPTH_53
#define PP_LIMIT_DEPTH_55 PP_LIMIT_DEPTH_54
#define PP_LIMIT_DEPTH_56 PP_LIMIT_DEPTH_55
#define PP_LIMIT_DEPTH_57 PP_LIMIT_DEPTH_56
#define PP_LIMIT_DEPTH_58 PP_LIMIT_DEPTH_57
#define PP_LIMIT_DEPTH_59 PP_LIMIT_DEPTH_58
#define PP_LIMIT_DEPTH_60 PP_LIMIT_DEPTH_59
#define PP_LIMIT_DEPTH_61 PP_LIMIT_DEPTH_60
#define PP_LIMIT_DEPTH_62 PP_LIMIT_DEPTH_61
#define PP_LIMIT_DEPTH_63 PP_LIMIT_DEPTH_62

#assert PP_LIMIT_DEPTH_63 = 42

'' end of macro-limit-boundary-edge-cases.bas
