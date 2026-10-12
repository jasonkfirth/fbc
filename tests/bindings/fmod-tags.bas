'' FreeBASIC FMOD metadata qualification
'' File: fmod-tags.bas
'' Purpose: Qualify bounded text conversion and real SDK tag byte ranges.
'' Responsibilities: Unicode boundary cases, native C widths and cleanup.
'' This metadata-only probe does not claim audio output qualification.
'' --------

#lang "fb"
#include once "fmod_tags.bi"

Dim As String tag_data
tag_data = "A" & Chr(&he9, 0)
If fmod_tag_text(StrPtr(tag_data), Len(tag_data), 0) <> "A" & Chr(&hc3, &ha9) Then End 1
tag_data = Chr(&hff, &hfe, &h3d, &hd8, 0, &hde, 0, 0)
If fmod_tag_text(StrPtr(tag_data), Len(tag_data), 1) <> Chr(&hf0, &h9f, &h98, &h80) Then End 1
tag_data = Chr(&hd8, &h3d, &hde, 0, 0, 0)
If fmod_tag_text(StrPtr(tag_data), Len(tag_data), 2) <> Chr(&hf0, &h9f, &h98, &h80) Then End 1
tag_data = Chr(&hf0, &h9f, &h98, &h80, 0)
If fmod_tag_text(StrPtr(tag_data), Len(tag_data), 3) <> Left(tag_data, 4) Then End 1

'' Truncation, overlong UTF-8, surrogate scalars, missing BOMs and
'' excessive lengths must fail before reading outside the supplied range.
Dim invalid_utf8(0 To 4) As String = {Chr(&hc0, &h80), Chr(&he2, &h82), Chr(&hed, &ha0, &h80), Chr(&hf4, &h90, &h80, &h80), Chr(&h80)}
For i As Long = 0 To UBound(invalid_utf8)
    tag_data = invalid_utf8(i)
    If fmod_tag_text(StrPtr(tag_data), Len(tag_data), 3) <> "" Then End 1
Next i
tag_data = Chr(&hd8, &h3d, 0, &h41)
If fmod_tag_text(StrPtr(tag_data), Len(tag_data), 2) <> "" Then End 1
tag_data = Chr(&hff, &hfe, &h41)
If fmod_tag_text(StrPtr(tag_data), Len(tag_data), 1) <> "" Then End 1
tag_data = "A"
If fmod_tag_text(StrPtr(tag_data), Len(tag_data), 1) <> "" Then End 1
If fmod_tag_text(StrPtr(tag_data), FMOD_TAG_TEXT_LIMIT + 1, 0) <> "" Then End 1
If fmod_tag_text(0, 1, 0) <> "" Then End 1
Print "FMOD_TEXT_BOUNDARIES=PASS"
If Command(1) = "" Then End 0

'' No-sound initialization isolates metadata decoding from output drivers.
If FSOUND_SetOutput(FSOUND_OUTPUT_NOSOUND) = 0 Then End 2
If FSOUND_Init(44100, 4, 0) = 0 Then End 2
Dim As String sound_file = Command(1)
Dim As FSOUND_STREAM Ptr fmod_stream = FSOUND_Stream_Open(StrPtr(sound_file), FSOUND_MPEGACCURATE, 0, 0)
If fmod_stream = 0 Then
    FSOUND_Close()
    End 1
End If

Type TagCounter
    before As ULong
    count As Long
    after As ULong
End Type
Dim As TagCounter tags
tags.before = &hA1B2C3D4
tags.after = &hE5F60718
Dim As Integer result = 0
If FSOUND_Stream_GetNumTagFields(fmod_stream, @tags.count) = 0 Then result = 1
If tags.before <> &hA1B2C3D4 OrElse tags.after <> &hE5F60718 Then result = 1
If tags.count < 0 OrElse tags.count > 1024 Then result = 1
Print "COUNT "; tags.count
Dim keys(0 To 5) As String = {"TITLE", "ARTIST", "ALBUM", "TIT2", "TPE1", "TALB"}
For i As Long = 0 To 5
    Dim As Any Ptr tag_value
    Dim As TagCounter tag_field
    tag_field.before = &hA1B2C3D4
    tag_field.after = &hE5F60718
    Dim As Long tag_type = FSOUND_TAGFIELD_ID3V1
    If i >= 3 Then tag_type = FSOUND_TAGFIELD_ID3V2
    Dim As Long found = FSOUND_Stream_FindTagField(fmod_stream, tag_type, StrPtr(keys(i)), @tag_value, @tag_field.count)
    If tag_field.before <> &hA1B2C3D4 OrElse tag_field.after <> &hE5F60718 Then result = 1
    If tag_field.count < 0 OrElse tag_field.count > FMOD_TAG_TEXT_LIMIT Then result = 1
    Print "TAG "; keys(i); " "; found; " "; tag_field.count; " ";
    If found AndAlso result = 0 AndAlso tag_field.count > 0 Then
        If tag_value = 0 Then
            result = 1
        Else
            Dim As UByte Ptr tag_bytes = tag_value
            For offset As Long = 0 To tag_field.count - 1
                If offset >= 128 Then Exit For
                Print LCase(Hex(tag_bytes[offset], 2));
            Next offset
        End If
    End If
    Print
Next i
Print "TEXT TITLE "; get_tag(fmod_stream, "TITLE", "TIT2")
Print "TEXT ARTIST "; get_tag(fmod_stream, "ARTIST", "TPE1")
Print "TEXT ALBUM "; get_tag(fmod_stream, "ALBUM", "TALB")
Print "FIELD_NAMES"
print_all_tags(fmod_stream)
If FSOUND_Stream_Close(fmod_stream) = 0 Then result = 1
FSOUND_Close()
End result

'' end of fmod-tags.bas
