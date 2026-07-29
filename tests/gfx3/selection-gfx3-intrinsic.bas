''
'' Project: FreeBASIC gfxlib3
'' --------------------------
''
'' File: selection-gfx3-intrinsic.bas
''
'' Purpose:
''
''     Verify gfxlib3 selection when a program uses graphics intrinsics
''     without explicitly including fbgfx.bi.
''
'' Responsibilities:
''
''     - activate gfxlib3 before the first graphics statement
''     - prove the compiler callback observes the source define
''
'' This file intentionally does NOT contain:
''
''     - constants or declarations from fbgfx.bi
''     - a visible display dependency
''

#define __FB_GFXLIB3__

'' GFX_NULL has the public value -1, but is intentionally not included here.
screenres 8, 8, 32, 1, -1
pset (5, 2), &hABCDEF
if point(5, 2) <> &hABCDEF then
	end 1
end if

'' end of selection-gfx3-intrinsic.bas
