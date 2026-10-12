'' examples/manual/libraries/fmod1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'FMOD'
'' Purpose: Play a module with the classic FMOD 3.75 API.
'' Responsibilities: File selection, playback errors and owned module cleanup.
'' This example contains no module decoder or audio driver implementation.
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibfmod
'' --------

#include Once "fmod.bi"

Dim As String sound_file = Command(1)
If sound_file = "" Then sound_file = ExePath() & "/../../sound/data/dne_trtn.mod"

If (FSOUND_GetVersion() < FMOD_VERSION) Then
	Print "FMOD version mismatch"
	End 1
End If

If (FSOUND_Init(44100, 32, 0) = 0) Then
	Print "Could not initialize FMOD: "; FSOUND_GetError()
	End 1
End If

Dim As FMUSIC_MODULE Ptr song = FMUSIC_LoadSong(StrPtr(sound_file))
If (song = 0) Then
	Print "FMOD could not load '" & sound_file & "'"
	FSOUND_Close()
	End 1
End If

If FMUSIC_PlaySong(song) = 0 Then
	Print "Could not start playback: "; FSOUND_GetError()
	FMUSIC_FreeSong(song)
	FSOUND_Close()
	End 1
End If

Dim As Integer playback_error = 0
If Environ("FB_EXAMPLE_AUTOPLAY") = "1" Then
	Sleep 1200, 1
	If FMUSIC_IsPlaying(song) = 0 OrElse FMUSIC_GetTime(song) <= 0 Then playback_error = 1
Else
	Print "Sound playing; waiting for keypress to stop and exit..."
	Sleep
End If

If FMUSIC_IsPlaying(song) Then
	If FMUSIC_StopSong(song) = 0 Then playback_error = 1
End If
If FMUSIC_FreeSong(song) = 0 Then playback_error = 1
FSOUND_Close()
If playback_error Then End 1
Print "FMOD playback completed."

'' end of fmod1.bas
