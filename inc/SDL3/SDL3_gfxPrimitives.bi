'' Project: FreeBASIC SDL addon bindings
'' File: SDL3_gfxPrimitives.bi
'' Purpose: Declare the pinned SDL3_gfx C interface.
'' Responsibilities: Preserve public types, callbacks, and header helpers.
'' This file intentionally does NOT contain: the external library implementation.
'' Translated from upstream; this is an altered source version.
''
'' SDL3_gfxPrimitives.h: graphics primitives for SDL
''
'' Copyright (C) 2012-2014  Andreas Schiffler
''
'' This software is provided 'as-is', without any express or implied
'' warranty. In no event will the authors be held liable for any damages
'' arising from the use of this software.
''
'' Permission is granted to anyone to use this software for any purpose,
'' including commercial applications, and to alter it and redistribute it
'' freely, subject to the following restrictions:
''
'' 1. The origin of this software must not be misrepresented; you must not
'' claim that you wrote the original software. If you use this software
'' in a product, an acknowledgment in the product documentation would be
'' appreciated but is not required.
''
'' 2. Altered source versions must be plainly marked as such, and must not be
'' misrepresented as being the original software.
''
'' 3. This notice may not be removed or altered from any source
'' distribution.
''
'' Andreas Schiffler -- aschiffler at ferzkopp dot net

#pragma once

#inclib "SDL3_gfx"
#include once "SDL.bi"


extern "C"

#define _SDL3_gfxPrimitives_h
const M_PI = 3.1415926535897932384626433832795
const SDL3_GFXPRIMITIVES_MAJOR = 1
const SDL3_GFXPRIMITIVES_MINOR = 0
const SDL3_GFXPRIMITIVES_MICRO = 0

declare function pixelColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval color as Uint32) as boolean
declare function pixelRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function hlineColor(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval x2 as Sint16, byval y as Sint16, byval color as Uint32) as boolean
declare function hlineRGBA(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval x2 as Sint16, byval y as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function vlineColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y1 as Sint16, byval y2 as Sint16, byval color as Uint32) as boolean
declare function vlineRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y1 as Sint16, byval y2 as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function rectangleColor(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval color as Uint32) as boolean
declare function rectangleRGBA(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function roundedRectangleColor(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval rad as Sint16, byval color as Uint32) as boolean
declare function roundedRectangleRGBA(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval rad as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function boxColor(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval color as Uint32) as boolean
declare function boxRGBA(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function roundedBoxColor(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval rad as Sint16, byval color as Uint32) as boolean
declare function roundedBoxRGBA(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval rad as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function lineColor(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval color as Uint32) as boolean
declare function lineRGBA(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function aalineColor(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval color as Uint32) as boolean
declare function aalineRGBA(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function thickLineColor(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval width as Uint8, byval color as Uint32) as boolean
declare function thickLineRGBA(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval width as Uint8, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function circleColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rad as Sint16, byval color as Uint32) as boolean
declare function circleRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rad as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function arcColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rad as Sint16, byval start as Sint16, byval end as Sint16, byval color as Uint32) as boolean
declare function arcRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rad as Sint16, byval start as Sint16, byval end as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function aacircleColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rad as Sint16, byval color as Uint32) as boolean
declare function aacircleRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rad as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function filledCircleColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval r as Sint16, byval color as Uint32) as boolean
declare function filledCircleRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rad as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function ellipseColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rx as Sint16, byval ry as Sint16, byval color as Uint32) as boolean
declare function ellipseRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rx as Sint16, byval ry as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function aaellipseColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rx as Sint16, byval ry as Sint16, byval color as Uint32) as boolean
declare function aaellipseRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rx as Sint16, byval ry as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function filledEllipseColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rx as Sint16, byval ry as Sint16, byval color as Uint32) as boolean
declare function filledEllipseRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rx as Sint16, byval ry as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function pieColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rad as Sint16, byval start as Sint16, byval end as Sint16, byval color as Uint32) as boolean
declare function pieRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rad as Sint16, byval start as Sint16, byval end as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function filledPieColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rad as Sint16, byval start as Sint16, byval end as Sint16, byval color as Uint32) as boolean
declare function filledPieRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval rad as Sint16, byval start as Sint16, byval end as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function trigonColor(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval x3 as Sint16, byval y3 as Sint16, byval color as Uint32) as boolean
declare function trigonRGBA(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval x3 as Sint16, byval y3 as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function aatrigonColor(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval x3 as Sint16, byval y3 as Sint16, byval color as Uint32) as boolean
declare function aatrigonRGBA(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval x3 as Sint16, byval y3 as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function filledTrigonColor(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval x3 as Sint16, byval y3 as Sint16, byval color as Uint32) as boolean
declare function filledTrigonRGBA(byval renderer as SDL_Renderer ptr, byval x1 as Sint16, byval y1 as Sint16, byval x2 as Sint16, byval y2 as Sint16, byval x3 as Sint16, byval y3 as Sint16, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function polygonColor(byval renderer as SDL_Renderer ptr, byval vx as const Sint16 ptr, byval vy as const Sint16 ptr, byval n as long, byval color as Uint32) as boolean
declare function polygonRGBA(byval renderer as SDL_Renderer ptr, byval vx as const Sint16 ptr, byval vy as const Sint16 ptr, byval n as long, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function aapolygonColor(byval renderer as SDL_Renderer ptr, byval vx as const Sint16 ptr, byval vy as const Sint16 ptr, byval n as long, byval color as Uint32) as boolean
declare function aapolygonRGBA(byval renderer as SDL_Renderer ptr, byval vx as const Sint16 ptr, byval vy as const Sint16 ptr, byval n as long, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function filledPolygonColor(byval renderer as SDL_Renderer ptr, byval vx as const Sint16 ptr, byval vy as const Sint16 ptr, byval n as long, byval color as Uint32) as boolean
declare function filledPolygonRGBA(byval renderer as SDL_Renderer ptr, byval vx as const Sint16 ptr, byval vy as const Sint16 ptr, byval n as long, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function texturedPolygon(byval renderer as SDL_Renderer ptr, byval vx as const Sint16 ptr, byval vy as const Sint16 ptr, byval n as long, byval texture as SDL_Surface ptr, byval texture_dx as long, byval texture_dy as long) as boolean
declare function bezierColor(byval renderer as SDL_Renderer ptr, byval vx as const Sint16 ptr, byval vy as const Sint16 ptr, byval n as long, byval s as long, byval color as Uint32) as boolean
declare function bezierRGBA(byval renderer as SDL_Renderer ptr, byval vx as const Sint16 ptr, byval vy as const Sint16 ptr, byval n as long, byval s as long, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare sub gfxPrimitivesSetFont(byval fontdata as const any ptr, byval cw as Uint32, byval ch as Uint32)
declare sub gfxPrimitivesSetFontRotation(byval rotation as Uint32)
declare function characterColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval c as boolean, byval color as Uint32) as boolean
declare function characterRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval c as boolean, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function stringColor(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval s as const zstring ptr, byval color as Uint32) as boolean
declare function stringRGBA(byval renderer as SDL_Renderer ptr, byval x as Sint16, byval y as Sint16, byval s as const zstring ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean

end extern

'' End of SDL3_gfxPrimitives.bi
