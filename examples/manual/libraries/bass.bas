'' examples/manual/libraries/bass.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'BASS'
'' Purpose: Play a tracker module through the native BASS output device.
'' Responsibilities: Asset paths, playback errors and owned resource cleanup.
'' This example contains no network streaming or recording.
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibbass
'' --------

#include Once "bass.bi"

'' An optional filename also allows running the executable outside this tree.
Dim As String sound_file = Command(1)
If sound_file = "" Then sound_file = ExePath() & "/../../sound/data/dne_trtn.mod"

If (HiWord(BASS_GetVersion()) <> BASSVERSION) Then
	Print "BASS 2.4 required!"
	End 1
End If

If (BASS_Init(-1, 44100, 0, 0, 0) = 0) Then
	Print "Could not initialize BASS"
	End 1
End If

Dim As HMUSIC test = BASS_MusicLoad(False, StrPtr(sound_file), 0, 0, BASS_MUSIC_LOOP, 0)
If (test = 0) Then
	Print "BASS could not load '" & sound_file & "'"
	BASS_Free()
	End 1
End If

If BASS_ChannelPlay(test, False) = 0 Then
	Print "BASS could not start playback: "; BASS_ErrorGetCode()
	BASS_Free()
	End 1
End If

If( Environ( "FB_EXAMPLE_AUTOPLAY" ) <> "" ) Then
	Sleep 1500, 1
	Dim As QWORD position = BASS_ChannelGetPosition(test, BASS_POS_BYTE)
	If position = Cast(QWORD, -1) OrElse position = 0 Then
		BASS_Free()
		End 1
	End If
	Print "BASS position seconds:"; BASS_ChannelBytes2Seconds(test, position)
Else
	Print "Sound playing; waiting to keypress to stop and exit..."
	Sleep
End If

If BASS_ChannelStop(test) = 0 Then
	BASS_Free()
	End 1
End If
If BASS_MusicFree(test) = 0 Then
	BASS_Free()
	End 1
End If
If BASS_Free() = 0 Then End 1
'' end of bass.bas
