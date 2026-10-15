'' OpenAL-based .wav player example
'' File: wav-player.bas
'' Purpose: Play a WAV file with native OpenAL and ALUT.
'' Responsibilities: Checked playback, pause/resume input and owned cleanup.
'' This file contains no audio driver or replacement WAV decoder.

#include "AL/al.bi"
#include "AL/alut.bi"

Function CloseAL(ByVal source As ALuint, ByVal buffer As ALuint) As Integer
    Dim As Integer result = 0
    If source <> 0 Then
        alSourceStop(source)
        If alGetError() <> AL_NO_ERROR Then result = 1
        '' An attached buffer cannot be deleted. Release its source first.
        alDeleteSources(1, @source)
        If alGetError() <> AL_NO_ERROR Then result = 1
    End If
    If buffer <> 0 Then
        alDeleteBuffers(1, @buffer)
        If alGetError() <> AL_NO_ERROR Then result = 1
    End If
    If alutExit() = AL_FALSE Then result = 1
    Return result
End Function

'' Initialize OpenAL
If alutInit(0, 0) = AL_FALSE Then
    Print "Error: Could not initialize OpenAL."
    End 1
End If

''
'' Load a .wav into an OpenAL buffer
''
dim as string sound_file = Command(1)
If sound_file = "" Then sound_file = exepath( ) + "/../data/prodigy.wav"

Dim As ALuint buffer
buffer = alutCreateBufferFromFile( SOUND_FILE )

If( buffer = 0 OrElse alutGetError( ) <> ALUT_ERROR_NO_ERROR ) Then
    Print "Error: Loading the .wav failed!"
    CloseAL(0, buffer)
    End 1
End If

''
'' Setup sound sources -- each one can have a 3D position & velocity.
''
Dim As ALuint source
alGenSources(1, @source)
If source = 0 OrElse alGetError() <> AL_NO_ERROR Then
    Print "Error: Could not create the sound source."
    CloseAL(source, buffer)
    End 1
End If

Dim As ALfloat SourcePos(0 To 2) = {0.0, 0.0, 0.0}
Dim As ALfloat SourceVel(0 To 2) = {0.0, 0.0, 0.0}

alSourcei(source, AL_BUFFER, buffer)
alSourcef(source, AL_PITCH, 1.0)
alSourcef(source, AL_GAIN, 1.0)
alSourcefv(source, AL_POSITION, @SourcePos(0))
alSourcefv(source, AL_VELOCITY, @SourceVel(0))

''
'' Setup the listener's 3D position etc.
''
Dim As ALfloat listener_position(0 To 2) = {0.0, 0.0, 0.0}
Dim As ALfloat listener_velocity(0 To 2) = {0.0, 0.0, 0.0}
'' Forward and up must span a basis, so they must not be parallel.
Dim As ALfloat listener_orientation(0 To 5) = {0.0, 0.0, -1.0, _ '' look at
                                               0.0, 1.0, 0.0}    '' up vector
alListenerfv(AL_POSITION, @listener_position(0))
alListenerfv(AL_VELOCITY, @listener_velocity(0))
alListenerfv(AL_ORIENTATION, @listener_orientation(0))

''
'' Play the sound (change to AL_FALSE to disable looping)
''
alSourcei(source, AL_LOOPING, AL_TRUE)
If alGetError() <> AL_NO_ERROR Then
    Print "Error: Could not configure the sound source."
    CloseAL(source, buffer)
    End 1
End If
alSourcePlay(source)
If alGetError() <> AL_NO_ERROR Then
    Print "Error: Could not start playback."
    CloseAL(source, buffer)
    End 1
End If

Print "Sound is playing, press ESC to exit and anything else to pause..."
Dim As String key
Dim As Integer active = -1
Dim As Integer playback_error = 0
Dim As Integer autoplay = (Environ("FB_EXAMPLE_AUTOPLAY") = "1")
Dim As Double started = Timer
Do
    key = Inkey()
    If (Len(key) > 0) Then
        If (key = Chr(27)) Then
            Exit Do
        End If

        If (active) Then
            alSourcePause(source)
            active = 0
        Else
            alSourcePlay(source)
            active = -1
        End If
        If alGetError() <> AL_NO_ERROR Then
            playback_error = 1
            Exit Do
        End If
        If active Then
            Print "Resumed."
        Else
            Print "Paused."
        End If
    End If
    If autoplay Then
        Dim As Double elapsed = Timer - started
        If elapsed < 0 Then elapsed += 86400.0
        '' Unattended checks keep the real looping source active for one second.
        If elapsed >= 1.0 Then Exit Do
    End If
    Sleep 10, 1
Loop

''
'' Clean up
''
If CloseAL(source, buffer) <> 0 Then playback_error = 1
If playback_error Then End 1
Print "OpenAL playback completed."
End 0
'' end of wav-player.bas
