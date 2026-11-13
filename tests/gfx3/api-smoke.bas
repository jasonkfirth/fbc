''
'' Project: FreeBASIC gfxlib3
'' --------------------------
''
'' File: api-smoke.bas
''
'' Purpose:
''
''     Exercise the first exported FreeBASIC graphics ABI slice through the
''     headless gfxlib3 backend.
''
'' Responsibilities:
''
''     - verify primitive and coordinate statements use gfxlib3 symbols
''     - verify multiple logical GPU pages and page copy
''     - remain runnable without a desktop window
''
'' This file intentionally does NOT contain:
''
''     - CPU image targets, PUT, text, palette, or input calls
''     - visible presentation assumptions
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"
#include once "fbgfx3.bi"

if screenres(16, 16, 32, 3, FB.GFX_NULL) <> 0 then end 1

pset (1, 2), rgba(18, 52, 86, 120)
if cuint(point(1, 2)) <> rgba(18, 52, 86, 120) then end 2
pset step (2, 1), &h87654321u
if cuint(point(3, 3)) <> &h87654321u then end 3
if point(2) <> 3.0 then end 4
if point(3) <> 3.0 then end 5

line (0, 15)-(15, 0), &hAABBCCDDu
if cuint(point(8, 7)) <> &hAABBCCDDu then end 6
line (2, 2)-(5, 5), &h10203040u, bf
if cuint(point(4, 4)) <> &h10203040u then end 7
circle (8, 8), 3, &h55667788u
if cuint(point(11, 8)) <> &h55667788u then end 8

view screen (2, 2)-(10, 10), &h11223344u, &h44332211u
if cuint(point(3, 3)) <> &h11223344u then end 9
view
if cuint(point(1, 2)) <> &h44332211u then end 10

window screen (0, 0)-(15, 15)
pset (15, 0), &h01020304u
window
if cuint(point(15, 0)) <> &h01020304u then end 11

if screenset(1, 2) <> 0 then end 12
pset (6, 9), &hDEADBEEFu
screencopy 1, 0
screenset 0, 0
if cuint(point(6, 9)) <> &hDEADBEEFu then end 13

'' The null backend expands compact opaque glyphs through the point fallback.
dim as fb.Gfx3Glyph glyph(0 to 0)
line (0, 0)-(15, 15), &h11223344u, bf
glyph(0).x = 2
glyph(0).y = 3
glyph(0).foreground = &hA1B2C3D4u
glyph(0).background = &h55667788u
glyph(0).width = 3
glyph(0).height = 2
glyph(0).row(0) = &h05u
glyph(0).row(1) = &h02u
if fb.Gfx3DrawGlyphs(@glyph(0), 1) <> 0 then end 14
if cuint(point(2, 3)) <> &hA1B2C3D4u then end 15
if cuint(point(3, 3)) <> &h11223344u then end 16
if cuint(point(4, 3)) <> &hA1B2C3D4u then end 17
if cuint(point(3, 4)) <> &hA1B2C3D4u then end 18

screen 0

'' end of api-smoke.bas
