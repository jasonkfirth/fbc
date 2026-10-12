'' examples/manual/libraries/cairo.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Cairo'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibcairo
'' --------

'' Purpose: Draw through a Cairo surface borrowing the FB framebuffer.
'' Responsibilities: Check native status, lock drawing and release both references.
'' This file contains no caller-owned pixel deallocation.

'' Example showing cairo being used to draw into the FB graphics window
#include Once "cairo/cairo.bi"

Const SCREEN_W = 400
Const SCREEN_H = 300

If ScreenRes(SCREEN_W, SCREEN_H, 32) <> 0 Then End 1
Dim image_pitch As Long
ScreenInfo , , , , image_pitch

'' Create a cairo drawing context, using the FB screen as surface.
Var surface = cairo_image_surface_create_for_data(ScreenPtr(), CAIRO_FORMAT_ARGB32, SCREEN_W, SCREEN_H, image_pitch)
If cairo_surface_status(surface) <> CAIRO_STATUS_SUCCESS Then
    cairo_surface_destroy(surface)
    End 1
End If

Var c = cairo_create(surface)
If cairo_status(c) <> CAIRO_STATUS_SUCCESS Then
    cairo_destroy(c)
    cairo_surface_destroy(surface)
    End 1
End If

ScreenLock()

'' Draw the entire context white.
cairo_set_source_rgba(c, 1, 1, 1, 1)
cairo_paint(c)

'' Draw a red line
cairo_set_line_width(c, 1)
cairo_set_source_rgba(c, 1, 0, 0, 1)
cairo_move_to(c, 0, 0)
cairo_line_to(c, SCREEN_W - 1, SCREEN_H - 1)
cairo_stroke(c)

cairo_surface_flush(surface)
Dim draw_status As cairo_status_t = cairo_status(c)
ScreenUnlock()
If draw_status <> CAIRO_STATUS_SUCCESS Then
    cairo_destroy(c)
    cairo_surface_destroy(surface)
    End 1
End If

Sleep

'' Clean up the cairo context
cairo_destroy(c)
cairo_surface_destroy(surface)

'' end of cairo.bas
