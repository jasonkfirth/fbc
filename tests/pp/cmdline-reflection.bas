' Project: FreeBASIC compiler tests
' --------------------------------
'
' File: cmdline-reflection.bas
'
' Purpose:
'
'     Verify that source #cmdline options are available through compiler
'     intrinsic defines.
'
' Responsibilities:
'
'     * check __FB_VERBOSE__ before and after -v is applied
'     * check __FB_GUI__ after -s gui is applied
'     * verify __CMDLINE__ preserves accepted source option order
'
' This test intentionally does NOT contain:
'
'     * runtime command-line argument reflection
'
' TEST_MODE : COMPILE_AND_RUN_OK

#if __CMDLINE__ <> ""
#error __CMDLINE__ should start empty
#endif

#if __FB_VERBOSE__ <> 0
#error __FB_VERBOSE__ should start disabled
#endif

#if __FB_EXPORT__ <> 0
#error __FB_EXPORT__ should start disabled
#endif

#if __FB_ENTRY__ <> ""
#error __FB_ENTRY__ should start empty
#endif

#if __FB_ARCH__ = ""
#error __FB_ARCH__ should identify the target architecture
#endif

#cmdline "-v"

#if defined(__FB_WIN32__) and not defined(__FB_WINCE__) and not defined(__FB_XBOX__)
#define CMDLINE_REFLECTION_TEST_GUI 1
#elseif defined(__FB_CYGWIN__) or defined(__FB_JS__)
#define CMDLINE_REFLECTION_TEST_GUI 1
#else
#define CMDLINE_REFLECTION_TEST_GUI 0
#endif

#if CMDLINE_REFLECTION_TEST_GUI
#cmdline "-s gui"
#endif

#if __FB_VERBOSE__ = 0
#error __FB_VERBOSE__ should be enabled by -v
#endif

#if CMDLINE_REFLECTION_TEST_GUI and (__FB_GUI__ = 0)
#error __FB_GUI__ should be enabled by -s gui
#endif

#cmdline "-maxerr ""1"""

const source_cmdline = __CMDLINE__

#if CMDLINE_REFLECTION_TEST_GUI
const expected_cmdline = "-v -s gui -maxerr ""1"""
#else
const expected_cmdline = "-v -maxerr ""1"""
#endif

if source_cmdline <> expected_cmdline then
	end 1
end if

end 0

' end of cmdline-reflection.bas
