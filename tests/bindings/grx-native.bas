'' FreeBASIC GRX 2.4.9 native qualification
'' File: grx-native.bas
'' Purpose: Compare the publisher ABI and a real graphics window with C.
'' Responsibilities: Context layouts, palette colors and displayed pixels.
'' This file contains no replacement renderer or keyboard injection.
'' --------

#lang "fb"
#include once "grx/grx20.bi"
#include once "grx/grxkeys.bi"
#ifdef __FB_WIN32__
    #include once "windows.bi"
    Extern "C"
        Extern hGRXWnd As HWND
    End Extern
#endif

Print "ABI "; SizeOf(Any Ptr); " "; SizeOf(GrColor); " "; SizeOf(GrKeyType); " "; _
    SizeOf(GrFrame); " "; OffsetOf(GrFrame, gf_lineoffset); " "; OffsetOf(GrFrame, gf_driver); " "; _
    SizeOf(GrContext); " "; OffsetOf(GrContext, gc_xmax); " "; SizeOf(GrMouseEvent); " "; GRX_VERSION_API
If Command(1) = "" Then End 0
If GrSetMode(GR_width_height_graphics, 320, 240) = 0 Then End 2
#ifdef __FB_WIN32__
    If hGRXWnd = 0 Then End 1
    SetWindowText(hGRXWnd, "GRX native FreeBASIC qualification")
#endif
Dim As Long cell_width = (GrSizeX() - 10) \ 32
Dim As Long cell_height = (GrSizeY() - 10) \ 8
Dim As Integer result = 0
If cell_width <= 0 OrElse cell_height <= 0 Then result = 1
GrSetRGBcolorMode()
For row As Long = 0 To 7
    For column As Long = 0 To 31
        Dim As GrColor color_value = row * 32 + column
        Dim As Long x = 5 + column * cell_width, y = 5 + row * cell_height
        If result = 0 Then
            GrFilledBox(x, y, x + cell_width - 1, y + cell_height - 1, color_value)
            If GrPixel(x + cell_width \ 2, y + cell_height \ 2) <> color_value Then result = 1
        End If
    Next column
Next row
GrSleep(200)
#ifdef __FB_WIN32__
    If result = 0 Then
        Dim As HDC window_dc = GetDC(hGRXWnd)
        If window_dc = 0 Then
            result = 1
        Else
            '' Compare the displayed GDI pixels with GRX's logical palette.
            For row As Long = 0 To 7
                For column As Long = 0 To 31
                    Dim As Long red_value, green_value, blue_value
                    GrQueryColor(row * 32 + column, @red_value, @green_value, @blue_value)
                    Dim As DWORD actual_pixel = GetPixel(window_dc, 5 + column * cell_width + cell_width \ 2, 5 + row * cell_height + cell_height \ 2)
                    '' GDI COLORREF stores red in its low byte. FreeBASIC's
                    '' RGB() uses a different packed-color representation.
                    Dim As DWORD expected_pixel = CULng(red_value) Or (CULng(green_value) Shl 8) Or (CULng(blue_value) Shl 16)
                    If actual_pixel = &hffffffff OrElse actual_pixel <> expected_pixel Then result = 1
                Next column
            Next row
            ReleaseDC(hGRXWnd, window_dc)
        End If
    End If
#endif
If GrSetMode(GR_default_text) = 0 Then result = 1
If result = 0 Then Print "GRX_FB_WINDOW=PASS colors=256"
End result

'' end of grx-native.bas
