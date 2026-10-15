''
'' PortAudio Sine wave example
'' File: portaudio-sine.bas
'' Purpose: Play three sine tones through the native PortAudio API.
'' Responsibilities: Callback buffers, stream lifetime and checked device data.
'' This file contains no audio driver or device installation.
''
'' Requires libportaudio2

#include "portaudio.bi"

Const Pi as Double = 3.14159265358979323846
Const TwoPi as Double = 2.0 * Pi
Const SampleRate as Integer = 44100
Const SineSamples as Integer = SampleRate \ 10
Const MaximumSample as Double = 2147483647.0
Const AmplitudeScale as Double = 0.5

Type PlayPayload
    frequency As Single
End Type

Function PlayCallback cdecl (InputBuff as Const Any Ptr, OutputBuff as Any Ptr, _
                    FrameCount as CUlong, TimeInfo as Const PaStreamCallbackTimeInfo Ptr, _
                    StatusFlags as PaStreamCallbackFlags, UserData as Any ptr) as Long

    If (OutputBuff = 0) OrElse (UserData = 0) Then Return paAbort
    If FrameCount = 0 Then Return paContinue

    Dim as Long Ptr outBuff = OutputBuff
    Dim as PlayPayload Ptr payload = UserData
    Dim as Single frequency = payload->frequency

    For i as CULong = 0 To FrameCount - 1
        outBuff[i] = CLng(MaximumSample * AmplitudeScale * _
            Sin(TwoPi * frequency * i / SampleRate))
    Next
    Function = paContinue
End Function

Sub PortAudioFailure(ByVal pe As PaError, ByVal pa_stream As PaStream Ptr = 0)
    Print "Error: "; *Pa_GetErrorText(pe)
    '' CloseStream also aborts an active stream and joins its callback.
    '' Keep the caller's stack payload alive until this cleanup has finished.
    If pa_stream <> 0 Then Pa_CloseStream(pa_stream)
    Pa_Terminate()
    End 1
End Sub

Sub Play(Frequency as Single, Duration as Single)
    Dim as PaError pe
    Dim as PaStream Ptr pa_stream

    Dim as PlayPayload payload
    payload.frequency = Frequency

    pe = Pa_OpenDefaultStream(@pa_stream, 0, 1, paInt32, SampleRate, SineSamples, @PlayCallback, @payload)
    If pe <> paNoError Then
        PortAudioFailure(pe)
    End If

    Print "Playing "; Frequency; "Hz for "; Duration; "ms"
    pe = Pa_StartStream(pa_stream)
    If pe <> paNoError Then
        PortAudioFailure(pe, pa_stream)
    End If

    'Sleep 2
    Pa_Sleep(Duration)

    pe = Pa_StopStream(pa_stream)
    If pe <> paNoError Then
        PortAudioFailure(pe, pa_stream)
    End If

    pe = Pa_CloseStream(pa_stream)
    If pe <> paNoError Then
        PortAudioFailure(pe)
    End If
End Sub

Dim as PaError pe
Print "Initialize PortAudio ("; *Pa_GetVersionText(); ")"

pe = Pa_Initialize()
If pe <> paNoError Then
    Print "Error: "; *Pa_GetErrorText(pe)
    End 1
End If

Dim defdevice as Integer = Pa_GetDefaultOutputDevice()
If defdevice = paNoDevice Then
    Print "Error: no default output device"
    Pa_Terminate()
    End 1
End If

Dim devinfo as const PaDeviceInfo Ptr = Pa_GetDeviceInfo(defdevice)
If devinfo = 0 Then
    Print "Error: default output device information unavailable"
    Pa_Terminate()
    End 1
End If
'' Use the API that owns this device; the process default API can differ.
Dim apiinfo as const PaHostApiInfo Ptr = Pa_GetHostApiInfo(devinfo->hostApi)
If apiinfo = 0 Then
    Print "Error: output host API information unavailable"
    Pa_Terminate()
    End 1
End If
Print "Using "; *apiinfo->name; " api on default output device: "; *devinfo->name

'' Play from A4 (440Hz) for 440ms until A3 (220Hz) for 220ms
For i as Integer = 440 to 220 Step -110
    Play(i, i)
Next

Print "Shutdown PortAudio"
pe = Pa_Terminate()
If pe <> paNoError Then
    Print "Error: "; *Pa_GetErrorText(pe)
    End 1
End If

End 0
'' end of portaudio-sine.bas
