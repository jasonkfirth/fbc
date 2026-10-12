'' FreeBASIC FMOD examples
'' File: fmod_tags.bi
'' Purpose: Read owned UTF-8 text from FMOD's borrowed tag data.
'' Responsibilities: C integer widths, byte limits and ID3 text encodings.
'' This file contains no file loading, audio output or stream cleanup.
'' The caller keeps the stream open throughout each metadata call.
'' --------

#pragma once
#include once "fmod.bi"

'' Keep metadata allocations bounded independently of the audio file size.
Const FMOD_TAG_TEXT_LIMIT = 4096

Private Function fmod_tag_append(ByRef text_value As String, ByRef used As Long, ByVal code_point As ULong) As Integer
    '' Unicode scalar values exclude the surrogate range and stop at U+10FFFF.
    If code_point > &h10ffff OrElse (code_point >= &hd800 AndAlso code_point <= &hdfff) Then Return False
    Dim As Long needed
    Select Case code_point
    Case Is < &h80: needed = 1
    Case Is < &h800: needed = 2
    Case Is < &h10000: needed = 3
    Case Else: needed = 4
    End Select
    If used < 0 OrElse used > Len(text_value) - needed Then Return False

    '' UTF-8 places successive six-bit groups in continuation bytes.
    Select Case needed
    Case 1
        text_value[used] = code_point
    Case 2
        text_value[used] = &hc0 Or (code_point Shr 6)
        text_value[used + 1] = &h80 Or (code_point And &h3f)
    Case 3
        text_value[used] = &he0 Or (code_point Shr 12)
        text_value[used + 1] = &h80 Or ((code_point Shr 6) And &h3f)
        text_value[used + 2] = &h80 Or (code_point And &h3f)
    Case 4
        text_value[used] = &hf0 Or (code_point Shr 18)
        text_value[used + 1] = &h80 Or ((code_point Shr 12) And &h3f)
        text_value[used + 2] = &h80 Or ((code_point Shr 6) And &h3f)
        text_value[used + 3] = &h80 Or (code_point And &h3f)
    End Select
    used += needed
    Return True
End Function

Private Function fmod_tag_text(ByVal tag_bytes As Const UByte Ptr, ByVal byte_count As Long, ByVal text_encoding As Long) As String
    If tag_bytes = 0 OrElse byte_count <= 0 OrElse byte_count > FMOD_TAG_TEXT_LIMIT Then Return ""
    '' ID3 text encoding: 0 = Latin-1, 1 = UTF-16 with BOM,
    '' 2 = UTF-16 big endian, 3 = UTF-8. Return the first text value.
    If text_encoding < 0 OrElse text_encoding > 3 Then Return ""
    Dim As Long position = 0, used = 0, big_endian = (text_encoding = 2)
    If text_encoding = 1 Then
        If byte_count < 2 Then Return ""
        If tag_bytes[0] = &hfe AndAlso tag_bytes[1] = &hff Then
            big_endian = True
        ElseIf tag_bytes[0] = &hff AndAlso tag_bytes[1] = &hfe Then
            big_endian = False
        Else
            Return ""
        End If
        position = 2
    End If
    If text_encoding = 1 OrElse text_encoding = 2 Then
        If (byte_count - position) Mod 2 <> 0 Then Return ""
    End If

    '' Latin-1 needs at most two UTF-8 bytes per input byte. UTF-16 and
    '' UTF-8 also fit this bound, including a UTF-16 surrogate pair.
    Dim As String text_value = String(byte_count * 2, 0)
    If Len(text_value) <> byte_count * 2 Then Return ""
    While position < byte_count
        Dim As ULong code_point = tag_bytes[position]
        If text_encoding = 0 Then
            position += 1
        ElseIf text_encoding = 3 Then
            Dim As Long trailing = 0
            Dim As ULong minimum = 0
            Select Case code_point
            Case Is < &h80
            Case &hc2 To &hdf: trailing = 1: minimum = &h80: code_point And= &h1f
            Case &he0 To &hef: trailing = 2: minimum = &h800: code_point And= &hf
            Case &hf0 To &hf4: trailing = 3: minimum = &h10000: code_point And= &h7
            Case Else: Return ""
            End Select
            If position + trailing >= byte_count Then Return ""
            position += 1
            For part As Long = 1 To trailing
                If (tag_bytes[position] And &hc0) <> &h80 Then Return ""
                code_point = (code_point Shl 6) Or (tag_bytes[position] And &h3f)
                position += 1
            Next part
            If code_point < minimum Then Return ""
        Else
            If position + 1 >= byte_count Then Return ""
            If big_endian Then
                code_point = (CULng(tag_bytes[position]) Shl 8) Or tag_bytes[position + 1]
            Else
                code_point = (CULng(tag_bytes[position + 1]) Shl 8) Or tag_bytes[position]
            End If
            position += 2
            If code_point >= &hd800 AndAlso code_point <= &hdbff Then
                If position + 1 >= byte_count Then Return ""
                Dim As ULong second_unit
                If big_endian Then
                    second_unit = (CULng(tag_bytes[position]) Shl 8) Or tag_bytes[position + 1]
                Else
                    second_unit = (CULng(tag_bytes[position + 1]) Shl 8) Or tag_bytes[position]
                End If
                If second_unit < &hdc00 OrElse second_unit > &hdfff Then Return ""
                code_point = &h10000 + ((code_point - &hd800) Shl 10) + second_unit - &hdc00
                position += 2
            End If
        End If
        If code_point = 0 Then Exit While
        If fmod_tag_append(text_value, used, code_point) = 0 Then Return ""
    Wend
    Return Left(text_value, used)
End Function

Private Sub print_all_tags(ByVal fmod_stream As FSOUND_STREAM Ptr)
    '' FMOD writes C int outputs, which remain 32-bit on a 64-bit PC.
    Dim As Long count
    If FSOUND_Stream_GetNumTagFields(fmod_stream, @count) = 0 Then Exit Sub
    If count < 0 OrElse count > 1024 Then Exit Sub
    For i As Long = 0 To count - 1
        Dim As Long tag_type, tag_length
        Dim As ZString Ptr tag_name
        Dim As Any Ptr tag_value
        If FSOUND_Stream_GetTagField(fmod_stream, i, @tag_type, @tag_name, @tag_value, @tag_length) Then
            If tag_name <> 0 Then Print *tag_name
        End If
    Next i
End Sub

Private Function get_tag(ByVal fmod_stream As FSOUND_STREAM Ptr, ByVal tagv1 As Const ZString Ptr, ByVal tagv2 As Const ZString Ptr) As String
    Dim As Any Ptr tag_value
    Dim As Long tag_length
    If FSOUND_Stream_FindTagField(fmod_stream, FSOUND_TAGFIELD_ID3V1, tagv1, @tag_value, @tag_length) Then
        Return fmod_tag_text(tag_value, tag_length, 0)
    End If
    If FSOUND_Stream_FindTagField(fmod_stream, FSOUND_TAGFIELD_ID3V2, tagv2, @tag_value, @tag_length) = 0 Then Return ""
    If tag_value = 0 OrElse tag_length <= 1 OrElse tag_length > FMOD_TAG_TEXT_LIMIT Then Return ""
    '' FMOD returns the whole text frame, including its encoding byte.
    Dim As Const UByte Ptr tag_bytes = tag_value
    Return fmod_tag_text(tag_bytes + 1, tag_length - 1, tag_bytes[0])
End Function

'' end of fmod_tags.bi
