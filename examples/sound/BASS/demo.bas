#Include "bass.bi"

' Find out which version of BASS is present.
If (HiWord(BASS_GetVersion()) <> BASSVERSION) Then
	Print "A wrong version of the BASS library has been found!"
	GetKey
	End
End If

' Initialize BASS using the default device at 44.1 KHz.
If (BASS_Init(-1, 44100, 0, 0, 0) = FALSE) Then
	Print "Could not initialize audio! BASS returned error " & BASS_ErrorGetCode()
	GetKey
	End
End If

' Load a looped example tracker module (music) file.
Dim As String musicFile = ExePath & "/../data/example.mo3"
Dim As HMUSIC musicHandle = BASS_MusicLoad(0, StrPtr(musicFile), 0, 0, BASS_SAMPLE_LOOP, 0)
If( musicHandle = 0 ) Then
	Print "Could not load background music! BASS returned error " & BASS_ErrorGetCode()
	BASS_Free()
	End 1
End If
' Play the loaded music from the beginning.
If( BASS_ChannelPlay(musicHandle, 0) = FALSE ) Then
	Print "Could not play background music! BASS returned error " & BASS_ErrorGetCode()
	BASS_Free()
	End 1
End If

' Load a sound effect as a stream - can be played only once at a time
Dim As String fx1File = ExePath & "/../data/prodigy.wav"
Dim As HSTREAM fx1Handle = BASS_StreamCreateFile(0, StrPtr(fx1File), 0, 0, 0)
If( fx1Handle = 0 ) Then
	Print "Could not load sound effect stream! BASS returned error " & BASS_ErrorGetCode()
	BASS_Free()
	End 1
End If

' Load a sound effect as a sample and allocate 16 channels at maximum for it.
' To understand the difference between streams and samples, please consult the BASS documentation.
Dim As String fx2File = ExePath & "/../data/prodigy.wav"
Dim As HSAMPLE fx2SampleHandle = BASS_SampleLoad(0, StrPtr(fx2File), 0, 0, 16, 0)
If( fx2SampleHandle = 0 ) Then
	Print "Could not load sound effect sample! BASS returned error " & BASS_ErrorGetCode()
	BASS_Free()
	End 1
End If

Print "Welcome to the BASS library demonstration."
Print "Press A to trigger sound effect 1."
Print "Press B to trigger sound effect 2."
Print "Press M to mute / unmute the background music."
Print "Press R to restart the background music."
Print "Press H to halve the music volume."
Print "Press D to double the music volume."
Print "Press ESC to quit."
Print

Dim As Integer musicState = 1
Dim As Integer currentLine = CsrLin
Dim As Integer autoplay = (Environ("FB_EXAMPLE_AUTOPLAY") <> "")

Do
	Dim As String key = UCase(Inkey)

	Select Case key
	Case "A"
		' Play sound effect 1
		BASS_ChannelPlay(fx1Handle, 0)

	Case "B"
		' Play sound effect 2
		Dim As HCHANNEL fx2Handle = BASS_SampleGetChannel(fx2SampleHandle, 0)
		BASS_ChannelPlay(fx2Handle, 0)

	Case "M"
		' Toggle music mute status
		musicState = musicState Xor 1
		If(musicState) Then
			BASS_ChannelPlay(musicHandle, 0)
		Else
			BASS_ChannelPause(musicHandle)
		End If

	Case "R"
		' Restart music
		BASS_ChannelPlay(musicHandle, 1)

	Case "H"
		' Music volume (in range [0.0, 1.0])
		Dim As Single currentVolume = 1.0
		BASS_ChannelGetAttribute(musicHandle, BASS_ATTRIB_VOL, @currentVolume)
		BASS_ChannelSetAttribute(musicHandle, BASS_ATTRIB_VOL, currentVolume * 0.5)

	Case "D"
		' Music volume (in range [0.0, 1.0])
		Dim As Single currentVolume = 1.0
		BASS_ChannelGetAttribute(musicHandle, BASS_ATTRIB_VOL, @currentVolume)
		BASS_ChannelSetAttribute(musicHandle, BASS_ATTRIB_VOL, currentVolume * 2)

	Case Chr(27)
		Exit Do

	End Select

	' Print music progress
	Locate currentLine, 1
	Dim As QWORD trackerPosition = BASS_ChannelGetPosition(musicHandle, BASS_POS_MUSIC_ORDER)
	Dim As Double secondsPosition = BASS_ChannelBytes2Seconds(musicHandle, BASS_ChannelGetPosition(musicHandle, BASS_POS_BYTE))
	Print "Music position: Order: " & LoWord(trackerPosition) & ", Row: " & HiWord(trackerPosition) & " (" & CInt(secondsPosition) & " seconds)    "
	If( autoplay AndAlso secondsPosition >= 1.0 ) Then Exit Do

	Sleep 10
Loop

' Free all resources allocated by BASS
BASS_Free()
