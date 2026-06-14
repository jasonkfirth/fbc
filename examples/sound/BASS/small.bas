' Source: http://www.freebasic-portal.de/porticula/8-1-sound-musikausgabe-1384.html

#Include "bass.bi"

' Initialize BASS
If( BASS_Init(-1, 44100, 0, 0, 0) = FALSE ) Then
	Print "Could not initialize BASS. BASS returned error " & BASS_ErrorGetCode()
	End 1
End If

' Load soundeffects and example music
Dim As String musicname = ExePath() & "/../data/prodigy.wav"
Dim As HSTREAM music_stream = BASS_StreamCreateFile(0, StrPtr(musicname), 0, 0, 0)
If( music_stream = 0 ) Then
	Print "Could not load music stream. BASS returned error " & BASS_ErrorGetCode()
	BASS_Free()
	End 1
End If
Dim As String soundname = ExePath() & "/../data/prodigy.wav"
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

BASS_ChannelPlay(music_stream, 0)                      ' Play music

If( Environ("FB_EXAMPLE_AUTOPLAY") <> "" ) Then
	Sleep 250, 1
	Print "BASS stream length seconds: " & BASS_ChannelBytes2Seconds(music_stream, BASS_ChannelGetLength(music_stream, BASS_POS_BYTE))
	Print "BASS stream active: " & BASS_ChannelIsActive(music_stream)
	Print "BASS stream position seconds: " & BASS_ChannelBytes2Seconds(music_stream, BASS_ChannelGetPosition(music_stream, BASS_POS_BYTE))
	BASS_Free()
	End 0
End If

Do
	If GetKey = 27 Then
		Exit Do                                ' ESC-Key
	End If

	BASS_ChannelPlay(soundchannel, 0)              ' Play soundeffects
Loop

BASS_Free()                                            ' Free allocated memory
