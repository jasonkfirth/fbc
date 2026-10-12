'' FreeBASIC external-library qualification
'' File: pdflib-native.bas
'' Purpose: Exercise the translated PDFlib 4.0.2 declarations.
'' Responsibilities: Native widths, callback ownership and three PDF outputs.
'' This file contains no C shim or substitute document generator.
'' The single-threaded allocator counter also covers the first null-PDF call.
#include once "pdflib.bi"

Dim Shared As UInteger live_blocks, allocations, output_bytes
Dim Shared As Integer sink, marker

Sub ErrorHandler Cdecl(ByVal p As PDF Ptr, ByVal error_type As Long, ByVal message As ZString Ptr)
    If error_type = PDF_NonfatalError Then Exit Sub
    Print "PDFlib error "; error_type; ": "; *message
    '' Returning from a fatal vendor callback would continue invalid state.
    End 2
End Sub

Function AllocateBlock Cdecl(ByVal p As PDF Ptr, ByVal bytes As UInteger, ByVal caller As ZString Ptr) As Any Ptr
    Dim As Any Ptr block = Allocate(bytes)
    If block = 0 Then End 3
    live_blocks += 1
    allocations += 1
    Return block
End Function

Function ResizeBlock Cdecl(ByVal p As PDF Ptr, ByVal old_block As Any Ptr, ByVal bytes As UInteger, ByVal caller As ZString Ptr) As Any Ptr
    If old_block = 0 Then Return AllocateBlock(p, bytes, caller)
    If bytes = 0 Then End 4
    Dim As Any Ptr block = Reallocate(old_block, bytes)
    If block = 0 Then End 3
    Return block
End Function

Sub ReleaseBlock Cdecl(ByVal p As PDF Ptr, ByVal block As Any Ptr)
    If block = 0 Then Exit Sub
    If live_blocks = 0 Then End 5
    live_blocks -= 1
    Deallocate(block)
End Sub

Function WriteOutput Cdecl(ByVal p As PDF Ptr, ByVal data_bytes As Any Ptr, ByVal bytes As UInteger) As UInteger
    If PDF_get_opaque(p) <> @marker OrElse sink = 0 Then End 6
    If bytes > 16 * 1024 * 1024 Then End 6
    If bytes > 0 Then
        If Put(#sink, , *CPtr(UByte Ptr, data_bytes), bytes) <> 0 Then Return 0
    End If
    output_bytes += bytes
    Return bytes
End Function

Function NewDocument() As PDF Ptr
    Dim As PDF Ptr p = PDF_new2(@ErrorHandler, @AllocateBlock, @ResizeBlock, @ReleaseBlock, @marker)
    If p = 0 OrElse PDF_get_opaque(p) <> @marker Then End 7
    Return p
End Function

Sub PageContents(ByVal p As PDF Ptr, ByRef logo As Const String)
    PDF_set_value(p, "compress", 0)
    PDF_begin_page(p, 320, 240)
    Dim As Long font = PDF_findfont(p, "Helvetica", "winansi", 0)
    If font < 0 Then End 8
    PDF_setfont(p, font, 14)
    PDF_show_xy(p, "PDFlib native ABI", 20, 200)
    PDF_rect(p, 20, 20, 280, 200)
    PDF_stroke(p)
    If PDF_open_image_file(p, "gif", "absent-pdflib-image.gif", "", 0) <> -1 Then End 9
    Dim As Long image = PDF_open_image_file(p, "gif", logo, "", 0)
    If image < 0 OrElse PDF_get_value(p, "imagewidth", image) <= 0 Then End 10
    PDF_place_image(p, image, 20, 80, 1)
    PDF_close_image(p, image)
    PDF_end_page(p)
    PDF_close(p)
End Sub

Print "PDFLIB_ABI="; LTrim(Str(SizeOf(Any Ptr))); ","; LTrim(Str(SizeOf(Long))); ","; _
    LTrim(Str(SizeOf(clong))); ","; LTrim(Str(SizeOf(UInteger))); ","; LTrim(Str(SizeOf(Single))); ","; _
    LTrim(Str(PDF_get_majorversion())); ","; LTrim(Str(PDF_get_minorversion()))
If Command(1) = "" Then End 0
Dim As String filename = Command(1), logo = Command(2)
Dim As Integer mode = ValInt(Command(3))
Dim As PDF Ptr p = NewDocument()
If PDF_open_file(p, "missing-pdflib-parent/denied.pdf") <> -1 Then End 11
PDF_delete(p)
If live_blocks <> 0 Then End 12
p = NewDocument()
'' A failed open also permits retrying the same object.
If PDF_open_file(p, "missing-pdflib-parent/retry.pdf") <> -1 Then End 11
If mode = 1 Then
    sink = FreeFile()
    If Open(filename For Binary Access Write As #sink) <> 0 Then End 13
    PDF_open_mem(p, @WriteOutput)
ElseIf PDF_open_file(p, IIf(mode = 2, "", filename)) <> 1 Then
    End 14
End If
PageContents(p, logo)
If mode = 2 Then
    '' PDF owns this buffer. Copy it before any further PDFlib call.
    Dim As clong bytes = 0
    Dim As ZString Ptr data_bytes = PDF_get_buffer(p, @bytes)
    If data_bytes = 0 OrElse bytes < 8 OrElse bytes > 16 * 1024 * 1024 Then End 15
    If Left(*data_bytes, 5) <> "%PDF-" Then End 15
    Dim As Integer file_number = FreeFile()
    If Open(filename For Binary Access Write As #file_number) <> 0 Then End 15
    If Put(#file_number, , *CPtr(UByte Ptr, data_bytes), bytes) <> 0 Then End 16
    If Close(file_number) <> 0 Then End 16
End If
If mode = 1 Then
    If Close(sink) <> 0 OrElse output_bytes < 8 Then End 17
End If
PDF_delete(p)
If live_blocks <> 0 OrElse allocations = 0 Then End 18
Print "PDFLIB_DOCUMENT=PASS mode="; LTrim(Str(mode)); " owned_blocks=0"
'' end of pdflib-native.bas
