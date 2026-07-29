''
'' Project: FreeBASIC gfxlib3
'' --------------------------
''
'' File: selection-gfx3.bas
''
'' Purpose:
''
''     Verify that the opt-in define selects the gfxlib3 archive family.
''
'' Responsibilities:
''
''     - activate gfxlib3 before the graphics declarations are included
''     - exercise the temporary gfxlib2 compatibility fallback headlessly
''
'' This file intentionally does NOT contain:
''
''     - a visible display dependency
''     - assumptions that unported entry points already use the GPU
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

screenres 8, 8, 32, 1, FB.GFX_NULL
pset (3, 4), &h654321
if point(3, 4) <> &h654321 then
	end 1
end if

'' end of selection-gfx3.bas
