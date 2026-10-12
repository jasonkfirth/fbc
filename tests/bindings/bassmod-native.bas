'' FreeBASIC BASSMOD native qualification
'' File: bassmod-native.bas
'' Purpose: Compare the vendor ABI and PCM decoder with the native C probe.
'' Responsibilities: Guarded buffers, bounded output and library cleanup.
'' This file contains no replacement module engine or output driver.
'' --------

#lang "fb"
#ifdef __FB_LINUX__
	#define BASSMOD_21
#endif
#include once "bassmod.bi"

Type AudioBuffer
	before As ULong
	data_values(0 To 4095) As Short
	after As ULong
End Type

Print "ABI "; SizeOf(DWORD); " "; SizeOf(BOOL); " "; SizeOf(HSYNC); " "; SizeOf(Any Ptr)
Print "VERSION "; BASSMOD_GetVersion()
If Command(1) = "" Then End 0

Dim As Long audio_device = -1
If Command(1) = "decode" Then audio_device = -3
If BASSMOD_Init(audio_device, 44100, 0) = 0 Then
	Print "INIT_ERROR="; BASSMOD_ErrorGetCode()
	End 2
End If

Dim As String module_file = Command(2)
If BASSMOD_MusicLoad(False, StrPtr(module_file), 0, 0, BASS_MUSIC_LOOP) = 0 Then
	BASSMOD_Free()
	End 1
End If

Dim As Integer result = 0
#ifdef BASSMOD_21
	'' The hotfix replaces the 2.0 amplify/pan/volume procedures with attributes.
	Dim As DWORD amplification = BASSMOD_MusicGetAttribute(BASS_MUSIC_ATTRIB_AMPLIFY)
	If amplification = Cast(DWORD, -1) Then result = 1
	If result = 0 Then
		If BASSMOD_MusicSetAttribute(BASS_MUSIC_ATTRIB_AMPLIFY, amplification) = Cast(DWORD, -1) Then result = 1
	End If
#endif

If result = 0 AndAlso audio_device = -3 Then
	Dim As AudioBuffer audio
	audio.before = &hA1B2C3D4
	audio.after = &hE5F60718
	Dim As ULong received, total
	Dim As Integer file_number = FreeFile()
	Dim As String output_file = Command(3)
	If output_file = "" OrElse Open(output_file For Binary Access Write As #file_number) <> 0 Then
		result = 1
	Else
		'' SIZEOF on an array member denotes one element. Supply the full byte
		'' capacity explicitly, matching the independently compiled C probe.
		Const BUFFER_BYTES = 4096 * SizeOf(Short)
		While total < 1024 * 1024
			received = BASSMOD_MusicDecode(@audio.data_values(0), BUFFER_BYTES)
			If received = 0 OrElse received > BUFFER_BYTES Then
				result = 1
				Exit While
			End If
			If audio.before <> &hA1B2C3D4 OrElse audio.after <> &hE5F60718 Then
				result = 1
				Exit While
			End If
			Dim As String bytes_out = String(received, 0)
			For offset As ULong = 0 To received - 1
				bytes_out[offset] = Cast(UByte Ptr, @audio.data_values(0))[offset]
			Next offset
			If Put(#file_number, , bytes_out) <> 0 Then
				result = 1
				Exit While
			End If
			total += received
		Wend
		If Close(file_number) <> 0 Then result = 1
		If result = 0 Then Print "BASSMOD_FB_DECODE=PASS bytes="; total
	End If
ElseIf result = 0 Then
	If BASSMOD_MusicPlay() = 0 Then
		result = 1
	Else
		Sleep 1200, 1
		Dim As DWORD position = BASSMOD_MusicGetPosition()
		If BASSMOD_MusicIsActive() <> BASS_ACTIVE_PLAYING OrElse _
			position = 0 OrElse position = Cast(DWORD, -1) Then result = 1
		If BASSMOD_MusicStop() = 0 Then result = 1
		If result = 0 Then Print "BASSMOD_FB_OUTPUT=PASS position="; position
	End If
End If
BASSMOD_MusicFree()
BASSMOD_Free()
End result

'' end of bassmod-native.bas
