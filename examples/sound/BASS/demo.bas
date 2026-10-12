' BASS example: demo.bas
' Purpose: Demonstrate music, stream, sample and volume controls.
' Responsibilities: Playback errors, asset paths and owned BASS resources.
' This example contains no recording or network streaming.
#Include "bass.bi"

' Called after successful initialization. BASS_Free releases every handle
' created by this process, including streams and sample channels.
Sub playback_error(ByRef message As Const String)
	Print message; ". BASS returned error "; BASS_ErrorGetCode()
	BASS_Free()
	End 1
End Sub

Dim As String data_directory = Command(1)
If data_directory = "" Then data_directory = ExePath() & "/../data"

' Find out which version of BASS is present.
If (HiWord(BASS_GetVersion()) <> BASSVERSION) Then
	Print "A wrong version of the BASS library has been found!"
	End 1
End If

' Initialize BASS using the default device at 44.1 KHz.
If (BASS_Init(-1, 44100, 0, 0, 0) = FALSE) Then
	Print "Could not initialize audio! BASS returned error " & BASS_ErrorGetCode()
	End 1
End If

' Load a looped example tracker module (music) file.
Dim As String musicFile = data_directory & "/example.mo3"
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
Dim As String fx1File = data_directory & "/prodigy.wav"
Dim As HSTREAM fx1Handle = BASS_StreamCreateFile(0, StrPtr(fx1File), 0, 0, 0)
If( fx1Handle = 0 ) Then
	Print "Could not load sound effect stream! BASS returned error " & BASS_ErrorGetCode()
	BASS_Free()
	End 1
End If

' Load a sound effect as a sample and allocate 16 channels at maximum for it.
' To understand the difference between streams and samples, please consult the BASS documentation.
Dim As String fx2File = fx1File
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
Dim As Integer autoplay_step

Do
	Dim As String key
	If autoplay Then
		' Use the same control paths as keyboard input, once each.
		Select Case autoplay_step
		Case 0: key = "A"
		Case 1: key = "B"
		Case 2: key = "H"
		Case 3: key = "D"
		Case 4, 5: key = "M"
		Case 6: key = "R"
		End Select
		If autoplay_step < 7 Then autoplay_step += 1
	Else
		key = UCase(Inkey)
	End If

	Select Case key
	Case "A"
		' Play sound effect 1
		If BASS_ChannelPlay(fx1Handle, 0) = 0 Then playback_error("Could not play stream")
		If autoplay Then
			If BASS_ChannelIsActive(fx1Handle) <> BASS_ACTIVE_PLAYING Then playback_error("Stream did not start")
		End If

	Case "B"
		' Play sound effect 2
		Dim As HCHANNEL fx2Handle = BASS_SampleGetChannel(fx2SampleHandle, 0)
		If fx2Handle = 0 Then playback_error("Could not allocate sample channel")
		If BASS_ChannelPlay(fx2Handle, 0) = 0 Then playback_error("Could not play sample")
		If autoplay Then
			If BASS_ChannelIsActive(fx2Handle) <> BASS_ACTIVE_PLAYING Then playback_error("Sample did not start")
		End If

	Case "M"
		' Toggle music mute status
		musicState = musicState Xor 1
		If(musicState) Then
			If BASS_ChannelPlay(musicHandle, 0) = 0 Then playback_error("Could not resume music")
			If autoplay Then
				If BASS_ChannelIsActive(musicHandle) <> BASS_ACTIVE_PLAYING Then playback_error("Music did not resume")
			End If
		Else
			If BASS_ChannelPause(musicHandle) = 0 Then playback_error("Could not pause music")
			If autoplay Then
				If BASS_ChannelIsActive(musicHandle) <> BASS_ACTIVE_PAUSED Then playback_error("Music did not pause")
			End If
		End If

	Case "R"
		' Restart music
		If BASS_ChannelPlay(musicHandle, 1) = 0 Then playback_error("Could not restart music")

	Case "H"
		' Music volume (in range [0.0, 1.0])
		Dim As Single currentVolume = 1.0
		If BASS_ChannelGetAttribute(musicHandle, BASS_ATTRIB_VOL, @currentVolume) = 0 Then playback_error("Could not read volume")
		If BASS_ChannelSetAttribute(musicHandle, BASS_ATTRIB_VOL, currentVolume * 0.5) = 0 Then playback_error("Could not halve volume")
		If autoplay Then
			Dim As Single verifiedVolume
			If BASS_ChannelGetAttribute(musicHandle, BASS_ATTRIB_VOL, @verifiedVolume) = 0 Then playback_error("Could not verify volume")
			If Abs(verifiedVolume - currentVolume * 0.5) > 0.00001 Then playback_error("Volume did not halve")
		End If

	Case "D"
		' Music volume (in range [0.0, 1.0])
		Dim As Single currentVolume = 1.0
		If BASS_ChannelGetAttribute(musicHandle, BASS_ATTRIB_VOL, @currentVolume) = 0 Then playback_error("Could not read volume")
		currentVolume *= 2
		If currentVolume > 1.0 Then currentVolume = 1.0
		If BASS_ChannelSetAttribute(musicHandle, BASS_ATTRIB_VOL, currentVolume) = 0 Then playback_error("Could not double volume")
		If autoplay Then
			Dim As Single verifiedVolume
			If BASS_ChannelGetAttribute(musicHandle, BASS_ATTRIB_VOL, @verifiedVolume) = 0 Then playback_error("Could not verify volume")
			If Abs(verifiedVolume - currentVolume) > 0.00001 Then playback_error("Volume did not double")
		End If

	Case Chr(27)
		Exit Do

	End Select

	' Print music progress
	Locate currentLine, 1
	Dim As QWORD trackerPosition = BASS_ChannelGetPosition(musicHandle, BASS_POS_MUSIC_ORDER)
	Dim As QWORD bytePosition = BASS_ChannelGetPosition(musicHandle, BASS_POS_BYTE)
	If trackerPosition = Cast(QWORD, -1) OrElse bytePosition = Cast(QWORD, -1) Then playback_error("Could not read music position")
	Dim As Double secondsPosition = BASS_ChannelBytes2Seconds(musicHandle, bytePosition)
	Print "Music position: Order: " & LoWord(trackerPosition) & ", Row: " & HiWord(trackerPosition) & " (" & CInt(secondsPosition) & " seconds)    "
	If( autoplay AndAlso autoplay_step = 7 AndAlso secondsPosition >= 1.0 ) Then
		Print "BASS autoplay controls completed: "; autoplay_step
		Exit Do
	End If

	'' Allow the audio driver to advance between status polls.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL602
	Sleep 10
Loop

' Free all resources allocated by BASS
If BASS_Free() = 0 Then End 1
' end of demo.bas
