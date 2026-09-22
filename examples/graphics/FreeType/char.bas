'' Example of rendering a char using freetype

#include "freetype2/freetype.bi"

#ifdef __FB_LINUX__
Const TTF_FONT = "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf"
#else
Const TTF_FONT = "Vera.ttf"
#endif

Sub releaseFreeType(ByRef library As FT_Library, ByRef face As FT_Face)
	If face <> 0 Then
		FT_Done_Face face
		face = 0
	End If

	If library <> 0 Then
		FT_Done_FreeType library
		library = 0
	End If

End Sub

Dim As FT_Library library = 0
Dim As FT_Face face = 0

If (FT_Init_FreeType(@library) <> 0) Then
	Print "FT_Init_FreeType() failed" : Sleep : End 1
End If

''
'' Load a font and render an '@' character on to a bitmap
''

If (FT_New_Face(library, TTF_FONT, 0, @face) <> 0) Then
	releaseFreeType library, face
	Print "FT_New_Face() failed (font file '" & TTF_FONT & "' not found?)" : Sleep : End 1
End If

If (FT_Set_Pixel_Sizes(face, 0, 200) <> 0) Then
	releaseFreeType library, face
	Print "FT_Set_Pixel_Sizes() failed" : Sleep : End 1
End If

If (FT_Load_Char(face, Asc("@"), FT_LOAD_DEFAULT) <> 0) Then
	releaseFreeType library, face
	Print "FT_Load_Char() failed" : Sleep : End 1
End If

If (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) <> 0) Then
	releaseFreeType library, face
	Print "FT_Render_Glyph() failed" : Sleep : End 1
End If

''
'' Draw the rendered bitmap
''

If ScreenRes(320, 200, 32) <> 0 Then
	releaseFreeType library, face
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

Dim As FT_Bitmap Ptr bitmap = @face->glyph->bitmap

For y As Integer = 0 To (bitmap->rows - 1)
    For x As Integer = 0 To (bitmap->width - 1)
        Dim As Integer col = bitmap->buffer[y * bitmap->pitch + x]
        PSet(x, y), RGB(col, col, col)
    Next
next

Sleep

releaseFreeType library, face
