' BASS example: small.bas
' Purpose: Play a WAV stream and trigger a sample channel.
' Responsibilities: Native playback, asset paths and BASS resource cleanup.
' This example contains no recording or network input.
' Source: http://www.freebasic-portal.de/porticula/8-1-sound-musikausgabe-1384.html

#Include "bass.bi"

If HiWord(BASS_GetVersion()) <> BASSVERSION Then End 1
Dim As String data_directory = Command(1)
If data_directory = "" Then data_directory = ExePath() & "/../data"

' Initialize BASS
If( BASS_Init(-1, 44100, 0, 0, 0) = FALSE ) Then
	Print "Could not initialize BASS. BASS returned error " & BASS_ErrorGetCode()
	End 1
End If

' Load soundeffects and example music
Dim As String musicname = data_directory & "/prodigy.wav"
Dim As HSTREAM music_stream = BASS_StreamCreateFile(0, StrPtr(musicname), 0, 0, 0)
If( music_stream = 0 ) Then
	Print "Could not load music stream. BASS returned error " & BASS_ErrorGetCode()
	BASS_Free()
	End 1
End If
Dim As String soundname = musicname
Dim As HSAMPLE sound_sample = BASS_SampleLoad(0, StrPtr(soundname), 0, 0, 16, 0)
If( sound_sample = 0 ) Then
	Print "Could not load sound sample. BASS returned error " & BASS_ErrorGetCode()
	BASS_Free()
	End 1
End If
Dim soundchannel As HCHANNEL = BASS_SampleGetChannel(sound_sample, 0)
If( soundchannel = 0 ) Then
	Print "Could not allocate sound sample channel. BASS returned error " & BASS_ErrorGetCode()
	BASS_Free()
	End 1
End If

If BASS_ChannelPlay(music_stream, 0) = 0 Then
	BASS_Free()
	End 1
End If

If( Environ("FB_EXAMPLE_AUTOPLAY") <> "" ) Then
	If BASS_ChannelPlay(soundchannel, 0) = 0 Then
		BASS_Free()
		End 1
	End If
	Sleep 250, 1
	Dim As QWORD position = BASS_ChannelGetPosition(music_stream, BASS_POS_BYTE)
	If position = 0 OrElse position = Cast(QWORD, -1) Then
		BASS_Free()
		End 1
	End If
	Print "BASS stream length seconds: " & BASS_ChannelBytes2Seconds(music_stream, BASS_ChannelGetLength(music_stream, BASS_POS_BYTE))
	Print "BASS stream active: " & BASS_ChannelIsActive(music_stream)
	Print "BASS stream position seconds: " & BASS_ChannelBytes2Seconds(music_stream, position)
	If BASS_Free() = 0 Then End 1
	End 0
End If

Do
	If GetKey = 27 Then
		Exit Do                                ' ESC-Key
	End If

	If BASS_ChannelPlay(soundchannel, 0) = 0 Then
		BASS_Free()
		End 1
	End If
	'' Limit retriggering and leave time for the audio driver's worker.
	Sleep 10, 1
Loop

If BASS_Free() = 0 Then End 1
' end of small.bas
