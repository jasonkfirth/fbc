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

screen 0

'' end of api-smoke.bas
