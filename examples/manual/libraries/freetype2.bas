'' FreeBASIC FreeType examples
'' File: freetype2.bas
'' Purpose: Render one grayscale glyph into the FB graphics window.
'' Responsibilities: Check bitmap layout and release font resources.
'' This file contains no font engine or borrowed-buffer deallocation.
'' examples/manual/libraries/freetype2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Freetype2'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibfreetype2
'' --------

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

Dim font_file As String = Command(1)
If font_file = "" Then font_file = TTF_FONT

Dim As FT_Library library = 0
Dim As FT_Face face = 0

If (FT_Init_FreeType(@library) <> 0) Then
	Print "FT_Init_FreeType() failed" : End 1
End If

''
'' Load a font and render an '@' character on to a bitmap
''

If (FT_New_Face(library, font_file, 0, @face) <> 0) Then
	releaseFreeType library, face
	Print "FT_New_Face() failed (font file '" & font_file & "' not found?)" : End 1
End If

If (FT_Set_Pixel_Sizes(face, 0, 200) <> 0) Then
	releaseFreeType library, face
	Print "FT_Set_Pixel_Sizes() failed" : End 1
End If

If (FT_Load_Char(face, Asc("@"), FT_LOAD_DEFAULT) <> 0) Then
	releaseFreeType library, face
	Print "FT_Load_Char() failed" : End 1
End If

If (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) <> 0) Then
	releaseFreeType library, face
	Print "FT_Render_Glyph() failed" : End 1
End If

''
'' Draw the rendered bitmap
''

If ScreenRes(320, 200, 32) <> 0 Then
	releaseFreeType library, face
	Print "Could not set the requested graphics mode"
	End 1
End If

Dim As FT_Bitmap Ptr bitmap = @face->glyph->bitmap
If bitmap->pixel_mode <> FT_PIXEL_MODE_GRAY Or bitmap->num_grays <> 256 Or bitmap->buffer = NULL Then
    releaseFreeType library, face
    End 1
End If
Dim row_size As LongInt = Abs(CLngInt(bitmap->pitch))
If row_size < bitmap->width Or bitmap->width > 8192 Or bitmap->rows > 8192 Then
    releaseFreeType library, face
    End 1
End If
Dim source_top As UByte Ptr = bitmap->buffer
If bitmap->pitch < 0 Then source_top += row_size*(bitmap->rows-1)


For y As Integer = 0 To (bitmap->rows - 1)
	For x As Integer = 0 To (bitmap->Width - 1)
		Dim As Integer col = source_top[y * CLngInt(bitmap->pitch) + x]
		PSet(x, y), RGB(col, col, col)
	Next
Next

Sleep

releaseFreeType library, face

'' end of freetype2.bas
