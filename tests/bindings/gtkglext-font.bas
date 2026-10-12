'' FreeBASIC native library regression checks
'' File: gtkglext-font.bas
'' Purpose: Verify that GtkGLExt's Pango font helper creates usable GL glyphs.
'' Responsibilities: Native context creation, display lists and rendered pixels.
'' This file intentionally contains no desktop capture or replacement font codec.

#lang "fb"
#include once "gtk/gtk.bi"
#include once "gtkgl/gtkgl.bi"
#include once "GL/gl.bi"

Const frame_width = 160
Const frame_height = 120
'' ASCII characters occupy the first 128 font display lists. RGBA pixels
'' need four bytes each; the fixed buffer avoids an unchecked allocation.
Const glyph_count = 128
Dim pixels(0 To frame_width * frame_height * 4 - 1) As UByte
Dim As GdkGLConfig Ptr glconfig
Dim As GtkWidget Ptr window_widget, drawing_area
Dim As GdkGLContext Ptr glcontext
Dim As GdkGLDrawable Ptr drawable
Dim As PangoFontDescription Ptr description
Dim As PangoFont Ptr font_value
Dim As GLuint list_base
Dim As Boolean context_current = False, passed = False

gtk_init(0, 0)
gtk_gl_init(0, 0)
Do
    glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB Or GDK_GL_MODE_DOUBLE)
    If glconfig = 0 Then Exit Do
    window_widget = gtk_window_new(GTK_WINDOW_TOPLEVEL)
    drawing_area = gtk_drawing_area_new()
    If window_widget = 0 OrElse drawing_area = 0 Then Exit Do
    gtk_widget_set_size_request(drawing_area, frame_width, frame_height)
    '' The library checks equality with C TRUE, not merely nonzero.
    If gtk_widget_set_gl_capability(drawing_area, glconfig, 0, CTRUE, GDK_GL_RGBA_TYPE) = 0 Then Exit Do
    gtk_container_add(GTK_CONTAINER(window_widget), drawing_area)
    gtk_widget_show_all(window_widget)
    While gtk_events_pending()
        gtk_main_iteration()
    Wend
    glcontext = gtk_widget_get_gl_context(drawing_area)
    drawable = gtk_widget_get_gl_drawable(drawing_area)
    If glcontext = 0 OrElse drawable = 0 Then Exit Do
    If gdk_gl_drawable_gl_begin(drawable, glcontext) = 0 Then Exit Do
    context_current = True

    list_base = glGenLists(glyph_count)
    If list_base = 0 OrElse list_base > &h7fffffff - glyph_count Then Exit Do
    description = pango_font_description_from_string("Monospace 18")
    If description = 0 Then Exit Do
    font_value = gdk_gl_font_use_pango_font(description, 0, glyph_count, CInt(list_base))
    If font_value = 0 Then Exit Do
    '' A helper that only returns a font cannot satisfy these GL checks.
    If glIsList(list_base + Asc("A")) = GL_FALSE Then Exit Do
    glViewport(0, 0, frame_width, frame_height)
    glDrawBuffer(GL_BACK)
    glReadBuffer(GL_BACK)
    glClearColor(0, 0, 0, 1)
    glClear(GL_COLOR_BUFFER_BIT)
    glColor3f(1, 1, 1)
    glRasterPos2f(-0.75, 0)
    glListBase(list_base)
    Dim As String text_value = "ABC"
    glCallLists(Len(text_value), GL_UNSIGNED_BYTE, StrPtr(text_value))
    glFinish()
    glReadPixels(0, 0, frame_width, frame_height, GL_RGBA, GL_UNSIGNED_BYTE, @pixels(0))
    If glGetError() <> GL_NO_ERROR Then Exit Do
    Dim As Integer lit_pixels
    For offset As Integer = 0 To UBound(pixels) Step 4
        If pixels(offset) <> 0 OrElse pixels(offset + 1) <> 0 OrElse pixels(offset + 2) <> 0 Then
            lit_pixels += 1
        End If
    Next
    passed = lit_pixels > 0
    If passed Then Print "GTKGL_FONT_PIXELS=PASS"; " pixels="; lit_pixels
Loop While False

'' GL resources require the context to remain current until they are freed.
If font_value <> 0 Then g_object_unref(font_value)
If description <> 0 Then pango_font_description_free(description)
If context_current Then
    If list_base <> 0 Then glDeleteLists(list_base, glyph_count)
    gdk_gl_drawable_gl_end(drawable)
End If
If window_widget <> 0 Then gtk_widget_destroy(window_widget)
If glconfig <> 0 Then g_object_unref(glconfig)
If passed = False Then End 1

'' end of gtkglext-font.bas
