''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: qb-input-smoke.bas
''
'' Purpose:
''
''     Verify QuickBASIC STICK and STRIG compatibility symbols with no
''     controller attached.
''
'' Responsibilities:
''
''     - open a standard QB graphics mode through gfxlib3
''     - check latched stick positions remain bounded at zero
''     - check current and latched button queries remain false
''
'' This file intentionally does NOT contain:
''
''     - physical controller input
''     - controller enumeration or axis normalization
''     - modern GETXPAD mappings
''

#lang "qb"
#define __FB_GFXLIB3__

screen 13
if stick( 0 ) <> 0 then end 1
if stick( 1 ) <> 0 then end 2
if stick( 2 ) <> 0 then end 3
if stick( 3 ) <> 0 then end 4
for query as integer = 0 to 7
	if strig( query ) <> 0 then end 5
next
screen 0
end 0

'' end of qb-input-smoke.bas
