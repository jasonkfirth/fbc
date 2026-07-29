''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: compiler-gfx3-option-smoke.bas
''
'' Purpose:
''
''     Verify that the compiler's -gfx3 option selects gfxlib3 without a
''     source-level #define.
''
'' Responsibilities:
''
''     - require the option-injected __FB_GFXLIB3__ define
''     - require the gfxlib3-only GFX_VULKAN declaration
''     - provide a probe for a separately compiled default main module
''
'' This file intentionally does NOT contain:
''
''     - a source-level gfxlib3 define
''     - an application entry point
''     - GPU-renderer performance measurements
''

#include once "fbgfx.bi"

#ifndef __FB_GFXLIB3__
	#error "-gfx3 did not define __FB_GFXLIB3__"
#endif

extern "C"

function gfx3_option_probe() as integer
	if fb.GFX_VULKAN <> &h200 then
		print "GFX3_OPTION_FAIL constant"
		function = 1
		exit function
	end if

	if screenres( 16, 16, 32, 1, fb.GFX_NULL ) <> 0 then
		print "GFX3_OPTION_FAIL screenres"
		function = 2
		exit function
	end if

	screen 0
	function = 0
end function

end extern

'' end of compiler-gfx3-option-smoke.bas
