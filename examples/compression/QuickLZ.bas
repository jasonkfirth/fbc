'Real World Demonstration Program for the QuickLZ compression library:
'A command line file compression tool.

'Ownership:
'Each compression routine owns its QuickLZ state and input/output buffers.
'Every error path releases resources acquired before it, and a released pointer
'is cleared before control leaves the routine.

#include "quicklz.bi"

Declare Sub PrintUsage()
Declare Sub CompressFile( ByRef infile As String, ByRef outfile As String )
Declare Sub DecompressFile( ByRef infile As String, ByRef outfile As String )

If Len(Command(1)) <> 1 Then PrintUsage()
If Len(Command(2)) = 0 Then PrintUsage()
If Len(Command(3)) = 0 Then PrintUsage()

Select Case LCase(Command(1))
Case "c"
    CompressFile( Command(2), Command(3) )
Case "d"
    DeCompressFile( Command(2), Command(3) )
Case Else
    PrintUsage()
End Select

Sub CompressFile( ByRef infile As String, ByRef outfile As String )
    Dim As UByte Ptr inBuffer, outBuffer
    Dim As UInteger CSize, FSize
    Dim As Integer FF
    Dim As qlz_state_compress ptr state_compress = Allocate(qlz_get_setting(1))

    If state_compress = 0 Then
        Print "Unable to allocate compression state"
        End 2
    End If

    FF = FreeFile()
    If Open(infile For Binary Access Read As #FF) <> 0 Then
        Deallocate(state_compress)
        state_compress = 0
        Print "Unable to open file for input"
        End 2
    End If

    FSize = LOF(FF)
    inBuffer = Allocate(IIf(FSize = 0, 1, FSize))
    If inBuffer = 0 Then
        Close #FF
        Deallocate(state_compress)
        state_compress = 0
        Print "Unable to allocate input buffer"
        End 2
    End If
    Get #FF, , *inBuffer, FSize
    Close #FF

    outBuffer = Allocate(FSize + 400)
    If outBuffer = 0 Then
        Deallocate(inBuffer)
        inBuffer = 0
        Deallocate(state_compress)
        state_compress = 0
        Print "Unable to allocate compression buffer"
        End 2
    End If

    CSize = qlz_compress(inBuffer, outBuffer, FSize, state_compress)
    If CSize = 0 Then
        Deallocate(inBuffer)
        inBuffer = 0
        Deallocate(outBuffer)
        outBuffer = 0
        Deallocate(state_compress)
        state_compress = 0
        Print "Compression failed!"
        End 3
    End If

    FF = FreeFile()
    If Open(outfile For Binary Access Write As #FF) <> 0 Then
        Deallocate(inBuffer)
        inBuffer = 0
        Deallocate(outBuffer)
        outBuffer = 0
        Deallocate(state_compress)
        state_compress = 0
        Print "Unable to write compressed data!"
        End 4
    End If

    Put #FF, , *outBuffer, CSize
    Close #FF

    Deallocate(inBuffer)
    inBuffer = 0
    Deallocate(outBuffer)
    outBuffer = 0
    Deallocate(state_compress)
    state_compress = 0

    Print "Uncompressed file: " & FSize & " bytes"
    Print "Compressed file: " & CSize & " bytes"
    Print "Difference: " & CInt(CSize - FSize) & " bytes"
End Sub

Sub DecompressFile( ByRef infile As String, ByRef outfile As String )
    Dim As UByte Ptr inBuffer, outBuffer
    Dim As UInteger CSize, FSize
    Dim As Integer FF
    Dim state_decompress As qlz_state_decompress Ptr = Allocate(qlz_get_setting(2))

    If state_decompress = 0 Then
        Print "Unable to allocate decompression state"
        End 5
    End If

    FF = FreeFile()
    If Open(infile For Binary Access Read As #FF) <> 0 Then
        Deallocate(state_decompress)
        state_decompress = 0
        Print "Unable to open compressed file for input"
        End 5
    End If

    FSize = LOF(FF)
    inBuffer = Allocate(IIf(FSize = 0, 1, FSize))
    If inBuffer = 0 Then
        Close #FF
        Deallocate(state_decompress)
        state_decompress = 0
        Print "Unable to allocate input buffer"
        End 5
    End If
    Get #FF, , *inBuffer, FSize
    Close #FF

    outBuffer = Allocate(qlz_size_decompressed(inBuffer))
    If outBuffer = 0 Then
        Deallocate(inBuffer)
        inBuffer = 0
        Deallocate(state_decompress)
        state_decompress = 0
        Print "Unable to allocate decompression buffer"
        End 5
    End If

    CSize = qlz_decompress(inBuffer, outBuffer, state_decompress)
    If CSize = 0 Then
        Deallocate(inBuffer)
        inBuffer = 0
        Deallocate(outBuffer)
        outBuffer = 0
        Deallocate(state_decompress)
        state_decompress = 0
        Print "Decompression failed, or uncompressed file is empty!"
        End 6
    End If

    FF = FreeFile()
    If Open(outfile For Binary Access Write As #FF) <> 0 Then
        Deallocate(inBuffer)
        inBuffer = 0
        Deallocate(outBuffer)
        outBuffer = 0
        Deallocate(state_decompress)
        state_decompress = 0
        Print "Unable to write uncompressed data!"
        End 7
    End If

    Put #FF, , *outBuffer, CSize
    Close #FF

    Deallocate(inBuffer)
    inBuffer = 0
    Deallocate(outBuffer)
    outBuffer = 0
    Deallocate(state_decompress)
    state_decompress = 0

    Print "Compressed file: " & FSize & " bytes"
    Print "Uncompressed file: " & CSize & " bytes"
    Print "Difference: " & CInt(CSize - FSize) & " bytes"
End Sub

Sub PrintUsage()
    Print "QuickLZ 1.20 Demonstration Program"
    Print "Usage: qlz {c|d} infile outfile"
    Print "c flag is for compression"
    Print "d flag is for decompression"
    End 1
End Sub

' end of QuickLZ.bas
