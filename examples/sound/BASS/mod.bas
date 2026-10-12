'' FreeBASIC BASSMOD example
'' File: mod.bas
'' Purpose: Play a module with the native BASSMOD 2.x library.
'' Responsibilities: File selection, playback errors and library cleanup.
'' This example contains no module decoder or output driver implementation.
'' --------

#include once "bassmod.bi"

dim as string sound_file = command(1)
if sound_file = "" then sound_file = exepath() + "/../data/dne_trtn.mod"

If ((BASSMOD_GetVersion() And &hffff) <> 2) Then
    Print "BASSMOD version 2 required!"
    End 1
End If

If (BASSMOD_Init(-1, 44100, 0) = 0) Then
    Print "Could not initialize BASSMOD: "; BASSMOD_ErrorGetCode()
    End 1
End If

If (BASSMOD_MusicLoad(FALSE, StrPtr(sound_file), 0, 0, BASS_MUSIC_LOOP) = 0) Then
    Print "BASSMOD could not load '" & sound_file & "'"
    BASSMOD_Free()
    End 1
End If

If BASSMOD_MusicPlay() = 0 Then
    Print "Could not start playback: "; BASSMOD_ErrorGetCode()
    BASSMOD_MusicFree()
    BASSMOD_Free()
    End 1
End If

Dim As Integer playback_error = 0
If Environ("FB_EXAMPLE_AUTOPLAY") = "1" Then
    '' Wait for actual module progress without requiring keyboard input.
    Sleep 1200, 1
    Dim As DWORD position = BASSMOD_MusicGetPosition()
    If BASSMOD_MusicIsActive() <> BASS_ACTIVE_PLAYING OrElse _
        position = 0 OrElse position = Cast(DWORD, -1) Then playback_error = 1
Else
    Print "Sound playing; waiting for keypress to stop and exit..."
    Sleep
End If

If BASSMOD_MusicStop() = 0 Then playback_error = 1
BASSMOD_MusicFree()
BASSMOD_Free()
If playback_error Then End 1
Print "BASSMOD playback completed."

'' end of mod.bas
