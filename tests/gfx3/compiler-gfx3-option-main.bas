''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: compiler-gfx3-option-main.bas
''
'' Purpose:
''
''     Link a normal FreeBASIC main module against the -gfx3-selected probe.
''
'' Responsibilities:
''
''     - leave gfxlib3 selection to the probe object's metadata
''     - call the probe that checks the option-injected declaration
''     - report a stable result for the separate-compilation test
''
'' This file intentionally does NOT contain:
''
''     - an fbgfx.bi include
''     - a gfxlib3 source define or compiler option
''     - direct graphics calls
''

declare function gfx3_option_probe cdecl alias "gfx3_option_probe" () as integer

dim as integer result = gfx3_option_probe()

if result <> 0 then
	print "GFX3_OPTION_FAIL probe=" & result
	end result
end if

print "GFX3_OPTION_PASS"
end 0

'' end of compiler-gfx3-option-main.bas
