'' FreeBASIC FreeType examples
'' File: drawstr.bas
'' Purpose: Blend rendered text into the FB graphics window.
'' Responsibilities: Check glyph state, clip pixels and release font resources.
'' This file contains no font engine or application-owned glyph buffer.
'' Drawing text onto an FB screen using FreeType2, by jofers

#include "freetype2/freetype.bi"
#include once "bitmap.bi"

#ifdef __FB_LINUX__
Const TTF_FONT = "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf"
#else
Const TTF_FONT = "Vera.ttf"
#endif

Const SCREEN_W = 320
Const SCREEN_H = 240

'' Text helpers share the active FreeType handles initialized by the module.
'' FB-LINTER: DISABLE-NEXT-LINE FBL301
Dim Shared As Integer pixelsize
Dim Shared As FT_Library library
Dim Shared As FT_Face font1

Sub draw_glyph _
    ( _
        ByVal font As FT_Face, _
        ByVal x As Integer, _
        ByVal y As Integer, _
        ByVal col As UInteger _
    )

    If font = 0 Then Exit Sub

    Dim image_width As Long, image_height As Long, image_bpp As Long, image_pitch As Long
    ScreenInfo image_width, image_height, , image_bpp, image_pitch
    If image_bpp <> 4 Or ScreenPtr() = NULL Then Exit Sub
    '' The caller holds ScreenLock throughout glyph blending.
    blend_bitmap(@font->glyph->bitmap, ScreenPtr(), image_width, image_height, image_pitch, x, y, col)
End Sub

Function print_text _
    ( _
        ByVal x As Integer, _
        ByVal y As Integer, _
        ByRef text As String, _
        ByVal font As FT_Face, _
        ByVal size As Integer, _
        ByVal col As UInteger _
    ) As Integer

    '' Get rid of any alpha channel
    col = col And &h00ffffff
    If font = NULL Or size < 1 Or size > 256 Then Return 0

    '' Set font size
    If (FT_Set_Pixel_Sizes(font, size, size) <> 0) Then
        Return 0
    End If
    pixelsize = size

    '' Draw each character
    Dim As FT_GlyphSlot slot = font->glyph
    Dim As Integer penx = x
    Dim As Integer peny = y

    For i As Integer = 0 To Len(text) - 1
        '' Load character index
        Dim As FT_UInt index = FT_Get_Char_Index(font, text[i])

        '' Load character glyph
        If (FT_Load_Glyph(font, index, FT_LOAD_DEFAULT) <> 0) Then
            Return 0
        End If

        '' Render glyph
        If (FT_Render_Glyph(font->glyph, FT_RENDER_MODE_NORMAL) <> 0) Then
            Return 0
        End If

        '' Clip each bitmap independently; an off-screen glyph must not discard
        '' later characters that can still fall inside the destination.
        '' Draw the glyph bitmap to the screen
        draw_glyph(font, penx + font->glyph->bitmap_left, peny - font->glyph->bitmap_top, col)

        penx += slot->advance.x Shr 6
    Next

    Return -1
End Function

#ifndef FREETYPE_EXAMPLE_NO_MAIN
    Dim font_file As String = Command(1)
    If font_file = "" Then font_file = TTF_FONT
    If (FT_Init_FreeType(@library) <> 0) Then
        Print "FT_Init_FreeType() failed" : End 1
    End If

    If (FT_New_Face(library, font_file, 0, @font1) <> 0) Then
        FT_Done_FreeType(library)
        Print "FT_New_Face() failed (font file '" & font_file & "' not found?)" : End 1
    End If

    If ScreenRes(SCREEN_W, SCREEN_H, 32) <> 0 Then
        FT_Done_Face(font1)
        FT_Done_FreeType(library)
        End 1
    End If

    ScreenLock()
    For x As Integer = 0 To (SCREEN_W - 1)
        For y As Integer = 0 To (SCREEN_H - 1)
            PSet (x, y), x Xor y
        Next
    Next
    ScreenUnLock()

    Randomize(Timer())

    For x As Integer = 1 To 20
        ScreenLock()
        print_text(Rnd() * 200, Rnd() * 180 + 20, _
                "Hello World!", font1, _
                Rnd() * 22 + 10, _
                RGB(Rnd() * 255, Rnd() * 255, Rnd() * 255))
        ScreenUnLock()
    Next

    Sleep
    FT_Done_Face(font1)
    FT_Done_FreeType(library)
#endif

'' end of drawstr.bas
