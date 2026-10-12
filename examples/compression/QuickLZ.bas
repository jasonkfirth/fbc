'QuickLZ command line file compression example
'File: QuickLZ.bas
'Purpose: Compress or decompress one bounded, nonempty file.
'Responsibilities: Header validation, exact file I/O and owned buffers.
'This file contains no compression implementation or streaming protocol.
'
'Each routine releases its state and buffers before reporting failure.
'Output must be new or empty, so existing files and input are preserved.

#include "quicklz.bi"

#if QLZ_STREAMING_BUFFER <> 0
#error This file example requires independent packets, not streaming state
#endif

'Keep allocations bounded. QuickLZ accepts larger values than this example.
Const MAX_FILE_SIZE As UInteger = 16 * 1024 * 1024
'QuickLZ requires size + 400 output bytes and can use wide, unaligned reads.
'Zero padding keeps these reads within an allocated buffer on either path.
Const BUFFER_PADDING As UInteger = 400

Declare Sub PrintUsage()
Declare Function ReadFile( ByRef filename As String, ByRef buffer As UByte Ptr, ByRef count As UInteger, ByVal limit As UInteger ) As Boolean
Declare Function WriteFile( ByRef filename As String, ByVal buffer As UByte Ptr, ByVal count As UInteger ) As Boolean
Declare Sub CompressFile( ByRef infile As String, ByRef outfile As String )
Declare Sub DecompressFile( ByRef infile As String, ByRef outfile As String )

If __FB_ARGC__ <> 4 Then PrintUsage()
If Len(Command(1)) <> 1 Then PrintUsage()
If Len(Command(2)) = 0 Then PrintUsage()
If Len(Command(3)) = 0 Then PrintUsage()
If qlz_get_setting(0) <> QLZ_COMPRESSION_LEVEL OrElse qlz_get_setting(3) <> 0 Then
    Print "QuickLZ library settings do not match this example"
    End 1
End If
If qlz_get_setting(7) <> 1 OrElse qlz_get_setting(8) <> 5 OrElse qlz_get_setting(9) <> 0 Then
    Print "This example requires QuickLZ 1.5.0"
    End 1
End If

Select Case LCase(Command(1))
Case "c"
    CompressFile( Command(2), Command(3) )
Case "d"
    DecompressFile( Command(2), Command(3) )
Case Else
    PrintUsage()
End Select

Function ReadFile( ByRef filename As String, ByRef buffer As UByte Ptr, ByRef count As UInteger, ByVal limit As UInteger ) As Boolean
    Dim As Integer FF = FreeFile()
    If Open(filename For Binary Access Read As #FF) <> 0 Then Return False
    Dim As LongInt file_size = LOF(FF)
    If file_size <= 0 OrElse file_size > limit Then
        Close #FF
        Return False
    End If
    count = CUInt(file_size)
    buffer = CAllocate(count + BUFFER_PADDING)
    If buffer = 0 Then
        Close #FF
        Return False
    End If
    Dim As UInteger bytes_read
    Dim As Integer read_error = Get(#FF, , *buffer, count, bytes_read)
    Dim As Integer close_error = Close(FF)
    If read_error <> 0 OrElse close_error <> 0 OrElse bytes_read <> count Then
        Deallocate(buffer)
        buffer = 0
        Return False
    End If
    Return True
End Function

Function WriteFile( ByRef filename As String, ByVal buffer As UByte Ptr, ByVal count As UInteger ) As Boolean
    Dim As Integer FF = FreeFile()
    If Open(filename For Binary Access Read Write As #FF) <> 0 Then Return False
    If LOF(FF) <> 0 Then
        Close #FF
        Return False
    End If
    Dim As Integer write_error = Put(#FF, , *buffer, count)
    Dim As Integer close_error = Close(FF)
    Return write_error = 0 AndAlso close_error = 0
End Function

Sub CompressFile( ByRef infile As String, ByRef outfile As String )
    Dim As UByte Ptr inBuffer, outBuffer
    Dim As UInteger CSize, FSize
    Dim As qlz_state_compress Ptr state_compress
    Dim As String failure = "Unable to read a nonempty input file within the 16 MiB limit"
    Do
        If Not ReadFile(infile, inBuffer, FSize, MAX_FILE_SIZE) Then Exit Do
        failure = "Unable to allocate compression buffers"
        state_compress = CAllocate(SizeOf(qlz_state_compress))
        outBuffer = CAllocate(FSize + BUFFER_PADDING)
        If state_compress = 0 OrElse outBuffer = 0 Then Exit Do
        failure = "QuickLZ compression state size does not match the library"
        If qlz_get_setting(1) <> SizeOf(qlz_state_compress) Then Exit Do
        failure = "Compression failed"
        CSize = qlz_compress(inBuffer, outBuffer, FSize, state_compress)
        If CSize = 0 OrElse CSize > FSize + BUFFER_PADDING Then Exit Do
        failure = "Unable to write a new or empty output file"
        If Not WriteFile(outfile, outBuffer, CSize) Then Exit Do
        failure = ""
    Loop While False
    Deallocate(inBuffer)
    Deallocate(outBuffer)
    Deallocate(state_compress)
    If failure <> "" Then
        Print failure
        End 2
    End If
    Print "Uncompressed file: " & FSize & " bytes"
    Print "Compressed file: " & CSize & " bytes"
    Print "Difference: " & (CLngInt(CSize) - CLngInt(FSize)) & " bytes"
End Sub

Sub DecompressFile( ByRef infile As String, ByRef outfile As String )
    Dim As UByte Ptr inBuffer, outBuffer
    Dim As UInteger CSize, FSize, decoded_size
    Dim As qlz_state_decompress Ptr state_decompress
    Dim As String failure = "Decompression requires a QuickLZ library built with QLZ_MEMORY_SAFE"
    Do
        'A .bi macro cannot enable checks in an already-built C library.
        If qlz_get_setting(6) <> 1 Then Exit Do
        failure = "Unable to read a nonempty compressed file within the size limit"
        If Not ReadFile(infile, inBuffer, FSize, MAX_FILE_SIZE + BUFFER_PADDING) Then Exit Do
        failure = "Invalid or incompatible QuickLZ header"
        If FSize < 3 Then Exit Do
        'QuickLZ 1.5.0 header: 01SSLLHC. SS is streaming mode, LL is level,
        'H selects 3 or 9 header bytes, and C indicates compressed contents.
        If (inBuffer[0] And &hFC) <> (&h40 Or (QLZ_COMPRESSION_LEVEL Shl 2)) Then Exit Do
        Dim As UInteger header_size = 3
        If (inBuffer[0] And 2) <> 0 Then header_size = 9
        If FSize <= header_size Then Exit Do
        If qlz_size_compressed(inBuffer) <> FSize Then Exit Do
        decoded_size = qlz_size_decompressed(inBuffer)
        If decoded_size = 0 OrElse decoded_size > MAX_FILE_SIZE Then Exit Do
        'The uncompressed C path uses memcpy even with QLZ_MEMORY_SAFE.
        'Its byte count must therefore fit the actual payload.
        If (inBuffer[0] And 1) = 0 Then
            If decoded_size <> FSize - header_size Then Exit Do
        Else
            If FSize < header_size + 4 Then Exit Do
        End If
        failure = "Unable to allocate decompression buffers"
        state_decompress = CAllocate(SizeOf(qlz_state_decompress))
        outBuffer = CAllocate(decoded_size + BUFFER_PADDING)
        If state_decompress = 0 OrElse outBuffer = 0 Then Exit Do
        failure = "QuickLZ decompression state size does not match the library"
        If qlz_get_setting(2) <> SizeOf(qlz_state_decompress) Then Exit Do
        failure = "Invalid compressed data"
        CSize = qlz_decompress(inBuffer, outBuffer, state_decompress)
        If CSize <> decoded_size Then Exit Do
        failure = "Unable to write a new or empty output file"
        If Not WriteFile(outfile, outBuffer, CSize) Then Exit Do
        failure = ""
    Loop While False
    Deallocate(inBuffer)
    Deallocate(outBuffer)
    Deallocate(state_decompress)
    If failure <> "" Then
        Print failure
        End 5
    End If
    Print "Compressed file: " & FSize & " bytes"
    Print "Uncompressed file: " & CSize & " bytes"
    Print "Difference: " & (CLngInt(CSize) - CLngInt(FSize)) & " bytes"
End Sub

Sub PrintUsage()
    Print "QuickLZ 1.5.0 file example"
    Print "Usage: qlz {c|d} infile outfile"
    Print "c flag is for compression; d flag is for decompression"
    Print "Input must be nonempty and at most 16 MiB uncompressed"
    Print "Output must be a new or empty file"
    Print "Decompression requires a QLZ_MEMORY_SAFE build of the C library"
    End 1
End Sub

' end of QuickLZ.bas
