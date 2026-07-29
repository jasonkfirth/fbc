''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: mmx-control-smoke.bas
''
'' Purpose:
''
''     Verify that gfxlib3 accepts the legacy MMX SCREENCONTROL setter even
''     though its renderer never selects a CPU MMX blitter.
''
'' Responsibilities:
''
''     - query the public GET_X86_MMX_ENABLED state
''     - exercise SET_X86_MMX_ENABLED before and during a graphics mode
''     - require gfxlib3 to report its documented disabled state
''
'' This file intentionally does NOT contain:
''
''     - CPU feature detection
''     - assumptions about 32-bit gfxlib2 acceleration policy
''     - pixel rendering checks
''

#ifndef GFX2_REFERENCE
    #ifndef __FB_GFXLIB3__
        #define __FB_GFXLIB3__
    #endif
#endif

#include once "fbgfx.bi"

sub require_disabled( byval failure_code as long )
    dim as long enabled

    screencontrol fb.GET_X86_MMX_ENABLED, enabled
    if enabled <> 0 then
        print "GFX_MMX_CONTROL_FAIL " & enabled
        end failure_code
    end if
end sub

require_disabled 1
screencontrol fb.SET_X86_MMX_ENABLED, 1
require_disabled 2
screencontrol fb.SET_X86_MMX_ENABLED, 0
require_disabled 3

if screenres( 32, 24, 32, 1, fb.GFX_NULL ) <> 0 then end 4

screencontrol fb.SET_X86_MMX_ENABLED, 1
require_disabled 5
screencontrol fb.SET_X86_MMX_ENABLED, 0
require_disabled 6

screen 0
print "GFX_MMX_CONTROL_PASS"
end 0

'' end of mmx-control-smoke.bas
