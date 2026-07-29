''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: android-gfx2-default-smoke.bas
''
'' Purpose:
''
''     Prove that the Android packaging helper leaves ordinary graphics
''     programs on gfxlib2 when no gfxlib3 opt-in is present.
''
'' Responsibilities:
''
''     - require the public gfxlib3 marker to remain undefined
''     - open gfxlib2's deterministic null graphics mode
''     - verify a basic PSET/POINT round trip before clean shutdown
''
'' This file intentionally does NOT contain:
''
''     - a gfxlib3 source define, extension header, or compiler option
''     - a visible Android GPU mode
''     - gfxlib3 extension API references
''

#include once "fbgfx.bi"

#ifdef __FB_GFXLIB3__
	#error "ordinary Android source selected gfxlib3"
#endif

if screenres( 16, 16, 32, 1, fb.GFX_NULL ) <> 0 then end 1
pset ( 7, 9 ), &h00112233u
if culng( point( 7, 9 ) ) <> &h00112233u then
	screen 0
	end 2
end if

screen 0
print "GFX2_ANDROID_DEFAULT_PASS"
end 0

'' end of android-gfx2-default-smoke.bas
