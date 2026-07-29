''
'' Project: FreeBASIC gfxlib3
'' --------------------------
''
'' File: selection-default.bas
''
'' Purpose:
''
''     Verify that ordinary graphics source continues to select gfxlib2.
''
'' Responsibilities:
''
''     - open a headless gfxlib mode through the default library
''     - prove a basic pixel round trip still works
''
'' This file intentionally does NOT contain:
''
''     - the gfxlib3 selection define
''     - hardware-dependent display setup
''

#include once "fbgfx.bi"

screenres 8, 8, 32, 1, FB.GFX_NULL
pset (3, 4), &h123456
if point(3, 4) <> &h123456 then
	end 1
end if

'' end of selection-default.bas
